//
// Copyright (c) 2003-2009, by Yet Another POD-Bot Development Team.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// $Id:$
//
// Based on SyPB
//

#include <core.h>

// console variables
ConVar ebot_debug("ebot_debug", "0");
ConVar ebot_debuggoal("ebot_debug_goal", "-1");
ConVar ebot_gamemod("ebot_gamemode", "0");

ConVar ebot_followuser("ebot_follow_user_max", "2");
ConVar ebot_knifemode("ebot_knife_mode", "0");
ConVar ebot_walkallow("ebot_walk_allow", "1");
ConVar ebot_stopbots("ebot_stop_bots", "0");
ConVar ebot_spraypaints("ebot_spray_paints", "1");
ConVar ebot_restrictweapons("ebot_restrict_weapons", "");
ConVar ebot_camp_min("ebot_camp_time_min", "16");
ConVar ebot_camp_max("ebot_camp_time_max", "48");
ConVar ebot_use_radio("ebot_use_radio", "2");
ConVar ebot_force_flashlight("ebot_force_flashlight", "0");
ConVar ebot_use_flare("ebot_zm_use_flares", "1");
ConVar ebot_eco_rounds("ebot_eco_rounds", "1");
ConVar ebot_buy_weapons("ebot_buy_weapons", "1");
ConVar ebot_prefer_better_pickup("ebot_prefer_better_pickup", "1");
ConVar ebot_chatter_path("ebot_chatter_path", "radio/bot");

// this function get the current message from the bots message queue
int Bot::GetMessageQueue(void)
{
	const int message = m_messageQueue[m_actMessageIndex++];
	m_actMessageIndex &= 0x1f; // wraparound
	return message;
}

// this function put a message into the bot message queue
void Bot::PushMessageQueue(const int message)
{
	if (message == CMENU_SAY)
	{
		// notify other bots of the spoken text otherwise, bots won't respond to other bots (network messages aren't sent from bots)
		for (const auto& bot : g_botManager->m_bots)
		{
			if (!bot)
				continue;

			if (bot == this)
				continue;

			if (m_isAlive != bot->m_isAlive)
				continue;

			bot->m_sayTextBuffer.entityIndex = m_index;
			cstrcpy(bot->m_sayTextBuffer.sayText, m_tempStrings);
		}
	}
	else if (message == CMENU_BUY && g_gameVersion & Game::HalfLife)
	{
		m_inBuyZone = false;
		m_buyingFinished = true;
		m_isVIP = false;
		m_buyState = 7;
		return;
	}

	m_messageQueue[m_pushMessageIndex++] = message;
	m_pushMessageIndex &= 0x1f; // wraparound
}

float Bot::InFieldOfView(const Vector& destination)
{
	const float absoluteAngle = cabsf(AngleMod(pev->v_angle.y) - AngleMod(destination.ToYaw()));
	if (absoluteAngle > 180.0f)
		return 360.0f - absoluteAngle;
	return absoluteAngle;
}

bool Bot::IsInViewCone(const Vector& origin)
{
	return ::IsInViewCone(origin, GetEntity());
}

#define vis 0.98f
#define edgeOffset 13.0f
bool Bot::CheckVisibility(edict_t* targetEntity)
{
	m_visibility = Visibility::None;
	if (FNullEnt(targetEntity))
		return false;

	TraceResult tr{};
	const Vector eyes = EyePosition();

	Vector spot = targetEntity->v.origin;
	edict_t* self = GetEntity();
	bool ignoreGlass = true;

	// zombies can't hit from the glass...
	if (m_isZombieBot)
		ignoreGlass = false;

	TraceLine(eyes, spot, true, ignoreGlass, self, &tr);
	if (tr.flFraction > vis || tr.pHit == targetEntity)
	{
		m_visibility |= Visibility::Body;
		m_enemyOrigin = spot;
		GetEnemyPosition();
	}

	// check top of head
	spot.z += 25.0f;
	TraceLine(eyes, spot, true, ignoreGlass, self, &tr);

	if (tr.flFraction > vis || tr.pHit == targetEntity)
	{
		m_visibility |= Visibility::Head;
		m_enemyOrigin = spot;
		GetEnemyPosition();
	}

	if (m_visibility != Visibility::None)
		return true;

	if ((targetEntity->v.flags & FL_DUCKING))
		spot.z = targetEntity->v.origin.z - 14.0f;
	else
		spot.z = targetEntity->v.origin.z - 34.0f;

	TraceLine(eyes, spot, true, ignoreGlass, self, &tr);
	if (tr.flFraction > vis || tr.pHit == targetEntity)
	{
		m_visibility |= Visibility::Other;
		m_enemyOrigin = spot;
		return true;
	}

	const Vector dir = (targetEntity->v.origin - pev->origin).Normalize2D();
	const Vector perp(-dir.y, dir.x, 0.0f);
	spot = targetEntity->v.origin + Vector(perp.x * edgeOffset, perp.y * edgeOffset, 0);

	TraceLine(eyes, spot, true, ignoreGlass, self, &tr);
	if (tr.flFraction > vis || tr.pHit == targetEntity)
	{
		m_visibility |= Visibility::Other;
		m_enemyOrigin = spot;
		return true;
	}

	spot = targetEntity->v.origin - Vector(perp.x * edgeOffset, perp.y * edgeOffset, 0);
	TraceLine(eyes, spot, true, ignoreGlass, self, &tr);

	if (tr.flFraction > vis || tr.pHit == targetEntity)
	{
		m_visibility |= Visibility::Other;
		m_enemyOrigin = spot;
		return true;
	}

	return false;
}

bool Bot::IsEnemyViewable(edict_t* player)
{
	return CheckVisibility(player);
}

bool Bot::ItemIsVisible(const Vector& destination, char* itemName)
{
	TraceResult tr{};

	// trace a line from bot's eyes to destination..
	TraceLine(EyePosition(), destination, true, false, GetEntity(), &tr);

	// check if line of sight to object is not blocked (i.e. visible)
	if (tr.flFraction < 1.0f)
	{
		// check for standard items
		if (g_gameVersion & Game::HalfLife)
		{
			if (tr.flFraction > 0.95f && !FNullEnt(tr.pHit) && cstrcmp(STRING(tr.pHit->v.classname), itemName) == 0)
				return true;
		}
		else
		{
			if (tr.flFraction > 0.97f && !FNullEnt(tr.pHit) && cstrcmp(STRING(tr.pHit->v.classname), itemName) == 0)
				return true;

			if (tr.flFraction > 0.95f && !FNullEnt(tr.pHit) && cstrncmp(STRING(tr.pHit->v.classname), "csdmw_", 6) == 0)
				return true;
		}

		return false;
	}

	return true;
}

bool Bot::EntityIsVisible(const Vector& dest, const bool fromBody)
{
	TraceResult tr{};
	TraceLine(fromBody ? pev->origin - Vector(0.0f, 0.0f, 1.0f) : EyePosition(), dest, true, true, GetEntity(), &tr);
	return tr.flFraction >= 1.0f;
}

bool Bot::IsBehindSmokeClouds(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	const char* classname;
	Vector entOrigin, pentOrigin, betweenUs, betweenNade, betweenResult;
	edict_t* pentGrenade = nullptr;
	while (!FNullEnt(pentGrenade = FIND_ENTITY_BY_CLASSNAME(pentGrenade, "grenade")))
	{
		classname = STRING(pentGrenade->v.model) + 9;
		if (classname[0] != 's' || classname[1] != 'm')
			continue;

		if (charToInt(classname) != 1408)
			continue;

		// if grenade is invisible don't care for it
		if (pentGrenade->v.effects & EF_NODRAW || !(pentGrenade->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)))
			continue;

		entOrigin = GetEntityOrigin(ent);
		if (InFieldOfView(entOrigin - EyePosition()) > pev->fov * 0.33333333333f && !EntityIsVisible(entOrigin))
			continue;

		pentOrigin = GetEntityOrigin(pentGrenade);
		betweenUs = (entOrigin - pev->origin).Normalize();
		betweenNade = (pentOrigin - pev->origin).Normalize();
		betweenResult = ((Vector(betweenNade.y, betweenNade.x, 0.0f) * 150.0f + pentOrigin) - pev->origin).Normalize();
		if ((betweenNade | betweenUs) > (betweenNade | betweenResult))
			return true;
	}

	return false;
}

// this function returns the best weapon of this bot (based on personality prefs)
int Bot::GetBestWeaponCarried(void)
{
	int* ptr = m_weaponPrefs;
	int weaponIndex = 0;
	int weapons = pev->weapons;

	const WeaponSelect* weaponTab = &g_weaponSelect[0];

	// take the shield in account
	if (HasShield())
		weapons |= (1 << Weapon::Shield);

	int i;
	for (i = 0; i < Const_NumWeapons; i++)
	{
		if (weapons & (1 << weaponTab[*ptr].id))
			weaponIndex = i;

		ptr++;
	}

	return weaponIndex;
}

// this function returns the best secondary weapon of this bot (based on personality prefs)
int Bot::GetBestSecondaryWeaponCarried(void)
{
	int* ptr = m_weaponPrefs;
	int weaponIndex = 0;
	int weapons = pev->weapons;

	// take the shield in account
	if (HasShield())
		weapons |= (1 << Weapon::Shield);

	const WeaponSelect* weaponTab = &g_weaponSelect[0];

	int i;
	int id;
	for (i = 0; i < Const_NumWeapons; i++)
	{
		id = weaponTab[*ptr].id;
		if (weapons & (1 << id) && (id == Weapon::Usp || id == Weapon::Glock18 || id == Weapon::Deagle || id == Weapon::P228 || id == Weapon::Elite || id == Weapon::FiveSeven))
		{
			weaponIndex = i;
			break;
		}

		ptr++;
	}

	return weaponIndex;
}

// this function compares weapons on the ground to the one the bot is using
bool Bot::RateGroundWeapon(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	int hasWeapon = 0;
	int groundIndex = 0;
	int* ptr = m_weaponPrefs;
	const WeaponSelect* weaponTab = &g_weaponSelect[0];

	int i;
	for (i = 0; i < Const_NumWeapons; i++)
	{
		if (cstrcmp(weaponTab[*ptr].modelName, STRING(ent->v.model) + 9) == 0)
		{
			groundIndex = i;
			break;
		}

		ptr++;
	}

	if (IsRestricted(weaponTab[groundIndex].id) && HasPrimaryWeapon())
		return false;

	if (groundIndex < 7)
		hasWeapon = GetBestSecondaryWeaponCarried();
	else
		hasWeapon = GetBestWeaponCarried();

	return groundIndex > hasWeapon;
}

// this function checks buttons for use button waypoint
edict_t* Bot::FindButton(void)
{
	float nearestDistance = FLT_MAX, distance;
	edict_t* searchEntity = nullptr, * foundEntity = nullptr;

	// find the nearest button which can open our target
	while (!FNullEnt(searchEntity = FIND_ENTITY_IN_SPHERE(searchEntity, pev->origin, 512.0f)))
	{
		if (cstrncmp("func_button", STRING(searchEntity->v.classname), 12) == 0 || cstrncmp("func_rot_button", STRING(searchEntity->v.classname), 16) == 0)
		{
			distance = (pev->origin - GetEntityOrigin(searchEntity)).GetLengthSquared();
			if (distance < nearestDistance)
			{
				nearestDistance = distance;
				foundEntity = searchEntity;
			}
		}
	}

	return foundEntity;
}

bool Bot::AllowPickupItem(void)
{
	if (m_hasEnemiesNear)
		return false;

	if (m_hasEntitiesNear)
		return false;

	if (IsOnLadder())
		return false;

	return true;
}

void Bot::FindItem(void)
{
	if (!AllowPickupItem())
	{
		m_pickupItem = nullptr;
		m_pickupType = PickupType::None;
		return;
	}

	edict_t* ent = nullptr;

	if (!FNullEnt(m_pickupItem))
	{
		while (!FNullEnt(ent = FIND_ENTITY_IN_SPHERE(ent, pev->origin, 512.0f)))
		{
			if (ent != m_pickupItem || ent->v.effects & EF_NODRAW || IsValidPlayer(ent->v.owner) || IsValidPlayer(ent))
				continue; // someone owns this weapon or it hasn't re spawned yet

			if (ItemIsVisible(GetEntityOrigin(ent), const_cast<char*>(STRING(ent->v.classname))))
				return;

			break;
		}
	}

	edict_t* pickupItem = nullptr;
	m_pickupItem = nullptr;
	m_pickupType = PickupType::None;
	ent = nullptr;
	pickupItem = nullptr;

	bool allowPickup = false;
	PickupType pickupType = PickupType::None;
	float distance, minDistance = squaredf(512.0f);
	Vector entityOrigin;
	int i, i2;

	while (!FNullEnt(ent = FIND_ENTITY_IN_SPHERE(ent, pev->origin, 512.0f)))
	{
		pickupType = PickupType::None;
		if (ent->v.effects & EF_NODRAW || ent == m_itemIgnore || IsValidPlayer(ent) || ent == GetEntity()) // we can't pickup a player...
			continue;

		if (pev->health < pev->max_health && cstrncmp("item_healthkit", STRING(ent->v.classname), 14) == 0)
			pickupType = PickupType::GetEntity;
		else if (pev->health < pev->max_health && cstrncmp("func_healthcharger", STRING(ent->v.classname), 18) == 0 && ent->v.frame == 0)
		{
			entityOrigin = GetEntityOrigin(ent);
			if ((pev->origin - entityOrigin).GetLengthSquared() < squaredf(128.0f))
			{
				if (g_gameVersion & Game::Xash)
					pev->buttons |= IN_USE;
				else
					MDLL_Use(ent, GetEntity());
			}

			LookAt(entityOrigin);
			pickupType = PickupType::GetEntity;
			m_moveSpeed = pev->maxspeed;
			m_strafeSpeed = 0.0f;
		}
		else if (pev->armorvalue < 100 && cstrncmp("item_battery", STRING(ent->v.classname), 12) == 0)
			pickupType = PickupType::GetEntity;
		else if (pev->armorvalue < 100 && cstrncmp("func_recharge", STRING(ent->v.classname), 13) == 0 && ent->v.frame == 0)
		{
			entityOrigin = GetEntityOrigin(ent);
			if ((pev->origin - entityOrigin).GetLengthSquared() < squaredf(128.0f))
			{
				if (g_gameVersion & Game::Xash)
					pev->buttons |= IN_USE;
				else
					MDLL_Use(ent, GetEntity());
			}

			LookAt(entityOrigin);
			pickupType = PickupType::GetEntity;
			m_moveSpeed = pev->maxspeed;
			m_strafeSpeed = 0.0f;
		}
		else if (g_gameVersion & Game::HalfLife)
		{
			if (m_currentWeapon != WeaponHL::Snark && cstrncmp("monster_snark", STRING(ent->v.classname), 13) == 0)
			{
				m_hasEntitiesNear = true;
				m_nearestEntity = ent;
				LookAt(GetEntityOrigin(ent));
				m_moveSpeed = -pev->maxspeed;
				m_strafeSpeed = 0.0f;

				if (!(pev->buttons & IN_ATTACK) && !(pev->oldbuttons & IN_ATTACK))
					pev->buttons |= IN_ATTACK;
			}
			else if (cstrncmp("weapon_", STRING(ent->v.classname), 7) == 0)
				pickupType = PickupType::GetEntity;
			else if (cstrncmp("ammo_", STRING(ent->v.classname), 5) == 0)
				pickupType = PickupType::GetEntity;
			else if (cstrncmp("weaponbox", STRING(ent->v.classname), 9) == 0)
				pickupType = PickupType::GetEntity;
		}
		else if (cstrncmp("hostage_entity", STRING(ent->v.classname), 14) == 0)
			pickupType = PickupType::Hostage;
		else if (cstrncmp("weaponbox", STRING(ent->v.classname), 9) == 0 && cstrcmp(STRING(ent->v.model) + 9, "backpack.mdl") == 0)
			pickupType = PickupType::DroppedC4;
		else if (cstrncmp("weaponbox", STRING(ent->v.classname), 9) == 0 && cstrcmp(STRING(ent->v.model) + 9, "backpack.mdl") == 0)
			pickupType = PickupType::DroppedC4;
		else if ((cstrncmp("weaponbox", STRING(ent->v.classname), 9) == 0 || cstrncmp("armoury_entity", STRING(ent->v.classname), 14) == 0 || cstrncmp("csdm", STRING(ent->v.classname), 4) == 0))
			pickupType = PickupType::Weapon;
		else if (cstrncmp("weapon_shield", STRING(ent->v.classname), 13) == 0)
			pickupType = PickupType::Shield;
		else if (m_team == Team::Counter && !m_hasDefuser && cstrncmp("item_thighpack", STRING(ent->v.classname), 14) == 0)
			pickupType = PickupType::DefuseKit;
		else if (cstrncmp("grenade", STRING(ent->v.classname), 7) == 0 && cstrcmp(STRING(ent->v.model) + 9, "c4.mdl") == 0)
			pickupType = PickupType::PlantedC4;
		else
		{
			for (i = 0; (i < entityNum && pickupType == PickupType::None); i++)
			{
				if (g_entityId[i] == -1 || g_entityAction[i] != 3)
					continue;

				if (g_entityTeam[i] != Team(0) && m_team != g_entityTeam[i])
					continue;

				if (ent != INDEXENT(g_entityId[i]))
					continue;

				pickupType = PickupType::GetEntity;
			}
		}

		if (pickupType == PickupType::None)
			continue;

		entityOrigin = GetEntityOrigin(ent);
		distance = (pev->origin - entityOrigin).GetLengthSquared();
		if (distance > minDistance)
			continue;

		if (!ItemIsVisible(entityOrigin, const_cast<char*>(STRING(ent->v.classname))))
		{
			if (cstrncmp("grenade", STRING(ent->v.classname), 7) != 0 || cstrcmp(STRING(ent->v.model) + 9, "c4.mdl") != 0)
				continue;

			if (distance > squaredf(128.0f))
				continue;
		}

		allowPickup = true;
		if (pickupType == PickupType::GetEntity)
			allowPickup = true;
		else if (pickupType == PickupType::Weapon)
		{
			i = GetBestWeaponCarried();
			i2 = GetBestSecondaryWeaponCarried();

			if (i2 < 7 && (m_ammo[g_weaponSelect[i2].id] > 0.3f * g_weaponDefs[g_weaponSelect[i2].id].ammo1Max) && cstrcmp(STRING(ent->v.model) + 9, "w_357ammobox.mdl") == 0)
				allowPickup = false;
			else if (!m_isVIP && i >= 7 && (m_ammo[g_weaponSelect[i].id] > 0.3f * g_weaponDefs[g_weaponSelect[i].id].ammo1Max) && cstrncmp(STRING(ent->v.model) + 9, "w_", 2) == 0)
			{
				if (cstrcmp(STRING(ent->v.model) + 9, "w_9mmarclip.mdl") == 0 && !(i == Weapon::Famas || i == Weapon::Ak47 || i == Weapon::M4A1 || i == Weapon::Galil || i == Weapon::Aug || i == Weapon::Sg552))
					allowPickup = false;
				else if (cstrcmp(STRING(ent->v.model) + 9, "w_shotbox.mdl") == 0 && !(i == Weapon::M3 || i == Weapon::Xm1014))
					allowPickup = false;
				else if (cstrcmp(STRING(ent->v.model) + 9, "w_9mmclip.mdl") == 0 && !(i == Weapon::Mp5 || i == Weapon::Tmp || i == Weapon::P90 || i == Weapon::Mac10 || i == Weapon::Ump45))
					allowPickup = false;
				else if (cstrcmp(STRING(ent->v.model) + 9, "w_crossbow_clip.mdl") == 0 && !(i == Weapon::Awp || i == Weapon::G3SG1 || i == Weapon::Scout || i == Weapon::Sg550))
					allowPickup = false;
				else if (cstrcmp(STRING(ent->v.model) + 9, "w_chainammo.mdl") == 0 && i == Weapon::M249)
					allowPickup = false;
			}
			else if (pev->health >= pev->max_health && cstrcmp(STRING(ent->v.model) + 9, "medkit.mdl") == 0)
				allowPickup = false;
			else if (pev->armorvalue >= 100 && (cstrcmp(STRING(ent->v.model) + 9, "kevlar.mdl") == 0 || cstrcmp(STRING(ent->v.model) + 9, "battery.mdl") == 0)) // armor vest
				allowPickup = false;
			else if (cstrcmp(STRING(ent->v.model) + 9, "flashbang.mdl") == 0 && (pev->weapons & (1 << Weapon::FbGrenade))) // concussion grenade
				allowPickup = false;
			else if (cstrcmp(STRING(ent->v.model) + 9, "hegrenade.mdl") == 0 && (pev->weapons & (1 << Weapon::HeGrenade))) // explosive grenade
				allowPickup = false;
			else if (cstrcmp(STRING(ent->v.model) + 9, "smokegrenade.mdl") == 0 && (pev->weapons & (1 << Weapon::SmGrenade))) // smoke grenade
				allowPickup = false;
			else if (m_isVIP || !ebot_prefer_better_pickup.GetBool() || !RateGroundWeapon(ent))
				allowPickup = false;
		}
		else if (pickupType == PickupType::Shield)
		{
			if ((pev->weapons & (1 << Weapon::Elite)) || HasShield() || m_isVIP || (HasPrimaryWeapon() && !RateGroundWeapon(ent)))
				allowPickup = false;
		}
		else if (m_team != Team::Terrorist && m_team != Team::Counter)
			allowPickup = false;
		else if (m_team == Team::Terrorist)
		{
			if (pickupType == PickupType::DroppedC4)
			{
				allowPickup = true;
				m_destOrigin = entityOrigin;
			}
			else if (pickupType == PickupType::Hostage)
			{
				m_itemIgnore = ent;
				allowPickup = false;
			}
			else if (pickupType == PickupType::PlantedC4)
			{
				allowPickup = false;

				i = FindDefendWaypoint(entityOrigin);
				if (IsValidWaypoint(i))
				{
					const float timeMidBlowup = g_timeBombPlanted + ((engine->GetC4TimerTime() * 0.5f) + engine->GetC4TimerTime() * 0.25f) - g_waypoint->GetTravelTime(m_moveSpeed, pev->origin, g_waypoint->GetPath(i)->origin);
					if (timeMidBlowup > engine->GetTime())
					{
						m_campIndex = i;
						SetProcess(Process::Camp, "i will defend the bomb", true, engine->GetTime() + timeMidBlowup);
					}
					else
						RadioMessage(Radio::ShesGonnaBlow);
				}
			}
		}
		else if (m_team == Team::Counter)
		{
			if (pickupType == PickupType::Hostage)
			{
				if (GetEntityOrigin(ent) == nullvec || ent->v.health <= 0.0f || ent->v.speed > 1.0f || ent->v.effects & EF_NODRAW)
					allowPickup = false; // never pickup dead/moving hostages
				else
				{
					for (const auto& bot : g_botManager->m_bots)
					{
						if (!bot)
							continue;

						if (!bot->m_isAlive)
							continue;

						for (const auto& hostage : bot->m_hostages)
						{
							if (hostage != ent)
								continue;

							allowPickup = false;
							break;
						}
					}
				}
			}
			else if (pickupType == PickupType::DroppedC4)
			{
				m_itemIgnore = ent;
				allowPickup = false;

				if (m_personality != Personality::Rusher && (m_personality != Personality::Normal || m_skill > 49))
				{
					i = FindDefendWaypoint(entityOrigin);
					if (IsValidWaypoint(i))
					{
						m_campIndex = i;
						SetProcess(Process::Camp, "i will defend the dropped bomb", true, engine->GetTime() + ebot_camp_max.GetFloat());
					}
				}
			}
			else if (pickupType == PickupType::PlantedC4)
			{
				if (OutOfBombTimer())
					allowPickup = false;
				else
				{
					allowPickup = !IsBombDefusing(g_waypoint->GetBombPosition());
					if (!allowPickup)
					{
						i = FindDefendWaypoint(entityOrigin);
						if (IsValidWaypoint(i))
						{
							m_campIndex = i;
							SetProcess(Process::Camp, "i will protect my teammate", true, engine->GetTime() + ebot_camp_max.GetFloat());
						}
					}
				}
			}
		}

		if (allowPickup)
		{
			minDistance = distance;
			pickupItem = ent;
			m_pickupType = pickupType;
		}
	}

	if (!FNullEnt(pickupItem))
	{
		if (GetGameMode() == GameMode::Deathmatch || GetGameMode() == GameMode::NoTeam)
		{
			for (const auto& bot : g_botManager->m_bots)
			{
				if (bot && bot != this && bot->m_isAlive && bot->m_pickupItem == pickupItem && bot->m_team == m_team)
				{
					m_pickupItem = nullptr;
					m_pickupType = PickupType::None;
					return;
				}
			}
		}

		entityOrigin = GetEntityOrigin(pickupItem);
		if (entityOrigin == nullvec || entityOrigin.z > EyePosition().z + 12.0f || IsDeadlyDrop(entityOrigin))
		{
			m_pickupItem = nullptr;
			m_pickupType = PickupType::None;
			return;
		}

		m_pickupItem = pickupItem;
	}
}

// this function depending on show boolen, shows/remove chatter, icon, on the head of bot
void Bot::SwitchChatterIcon(const bool show)
{
	if (ebot_use_radio.GetInt() != 2)
		return;

	if (!(g_gameVersion & Game::CStrike))
		return;

	for (const auto& client : g_clients)
	{
		if (client.team != m_team || !IsValidPlayer(client.ent) || IsValidBot(client.ent))
			continue;

		MESSAGE_BEGIN(MSG_ONE, g_netMsg->GetId(NETMSG_BOTVOICE), nullptr, client.ent); // begin message
		WRITE_BYTE(show); // switch on/off
		WRITE_BYTE(m_index);
		MESSAGE_END();
	}
}

// this function inserts the radio message into the message queue
void Bot::RadioMessage(const int message)
{
	if (g_gameVersion & Game::HalfLife)
		return;

	if (ebot_use_radio.GetInt() < 1)
		return;

	if (m_radiotimer > engine->GetTime())
		return;

	if (!m_numFriendsLeft)
		return;

	if (!m_numEnemiesLeft)
	{
		PlayChatterMessage(ChatterMessage::Happy);
		return;
	}

	if (GetGameMode() == GameMode::Deathmatch)
		return;

	// stop spamming
	if (m_radioEntity == GetEntity())
		return;

	m_radioSelect = message;
	if (ebot_use_radio.GetInt() > 1)
	{
		PlayChatterMessage(GetEqualChatter(message));
		return;
	}

	PushMessageQueue(CMENU_RADIO);
	m_radiotimer = engine->GetTime() + crandomfloat(m_numFriendsLeft, m_numFriendsLeft * 1.5f);
}

// this function inserts the voice message into the message queue (mostly same as above)
void Bot::PlayChatterMessage(const ChatterMessage& message)
{
	if (ebot_use_radio.GetInt() < 2)
		return;

	if (g_audioTime > engine->GetTime())
		return;

	if (!m_numFriendsLeft)
		return;

	if (m_radiotimer > engine->GetTime())
		return;

	if (m_lastChatterMessage == message)
		return;

	char* voice = GetVoice(message);
	if (!voice)
		return;

	char buffer[1024];
	FormatBuffer(buffer, "%s/sound/%s/%s.wav", GetModName(), ebot_chatter_path.GetString(), voice);
	File fp(buffer, "rb");
	if (!fp.IsValid())
		return;

	struct WavHeader
	{
		char chunk_id[4]{};
		int chunk_size{};
		char format[4]{};
		char subchunk1_id[4]{};
		int subchunk1_size{};
		short int audio_format{};
		short int num_channels{};
		int sample_rate{};
		int byte_rate{};
		short int block_align{};
		short int bits_per_sample{};
		char subchunk2_id[4]{};
		int subchunk2_size{};
	};

	WavHeader header;
	fp.Read(&header, sizeof(header));

	const float dur = ((static_cast<float>(header.subchunk2_size) / (static_cast<float>(header.num_channels) * (static_cast<float>(header.bits_per_sample) * 0.125f))) / static_cast<float>(header.sample_rate)) + 0.75f;
	if (dur < 0.2f || dur > 12.0f)
		return;

	m_chatterTimer = engine->GetTime() + dur;
	SwitchChatterIcon(true);

	FormatBuffer(buffer, "%s/%s.wav", ebot_chatter_path.GetString(), voice);

	for (const auto& client : g_clients)
	{
		if (client.team != m_team || IsValidBot(client.ent) || FNullEnt(client.ent))
			continue;

		MESSAGE_BEGIN(MSG_ONE, g_netMsg->GetId(NETMSG_SENDAUDIO), nullptr, client.ent); // begin message
		WRITE_BYTE(m_index);
		WRITE_STRING(buffer);
		WRITE_SHORT(m_voicePitch);
		MESSAGE_END();
	}

	m_lastChatterMessage = message;
	m_radiotimer = engine->GetTime() + crandomfloat(m_numFriendsLeft, m_numFriendsLeft * 1.5f);
	PushMessageQueue(CMENU_RADIO);
	g_audioTime = m_chatterTimer;
}

// this function checks and executes pending messages
void Bot::CheckMessageQueue(void)
{
	if (m_actMessageIndex == m_pushMessageIndex)
		return;

	// get message from stack
	switch (GetMessageQueue())
	{
	case CMENU_IDLE:
		break;
	case CMENU_BUY: // general buy message
	{
		// buy weapon
		if (m_nextBuyTime > engine->GetTime())
		{
			// keep sending message
			PushMessageQueue(CMENU_BUY);
			return;
		}

		// if fun-mode no need to buy
		if (!ebot_buy_weapons.GetBool() || (ebot_knifemode.GetBool() && (ebot_eco_rounds.GetInt() != 1 || HasPrimaryWeapon())))
		{
			m_buyState = 7;
			m_buyingFinished = true;
			m_inBuyZone = false;
			if (chanceof(m_skill) && !m_isBomber && !m_isVIP)
				SelectWeaponByName("weapon_knife");

			return;
		}

		if (!m_inBuyZone)
		{
			m_buyState = 7;
			m_buyingFinished = true;

			if (chanceof(m_skill) && !m_isBomber && !m_isVIP)
				SelectWeaponByName("weapon_knife");
			else
				SelectBestWeapon();

			break;
		}

		// if freezetime is very low do not delay the buy process
		if (engine->GetFreezeTime() < 2.0f)
			m_nextBuyTime = engine->GetTime() + crandomfloat(0.16f, 0.32f);
		else
			m_nextBuyTime = engine->GetTime() + crandomfloat(0.25f, 1.25f);

		// prevent vip from buying
		if (g_mapType == MAP_AS)
		{
			if (*(INFOKEY_VALUE(GET_INFOKEYBUFFER(GetEntity()), "model")) == 'v')
			{
				m_isVIP = true;
				m_buyState = 7;
				m_inBuyZone = false;
			}
			else
				m_isVIP = false;
		}
		else
		{
			m_isVIP = false;

			// prevent teams from buying on fun maps
			if (g_mapType == MAP_KA)
			{
				ebot_knifemode.SetInt(1);
				m_buyState = 7;
				m_buyingFinished = true;
				m_inBuyZone = false;
				return;
			}
			else if (g_mapType == MAP_KA || g_mapType ==  MAP_FY || g_mapType == MAP_AWP || (g_mapType == MAP_ES && m_team == Team::Terrorist))
			{
				m_buyState = 7;
				m_buyingFinished = true;
				m_inBuyZone = false;
				return;
			}
		}

		if (m_buyState > 6)
		{
			m_buyState = 7;
			m_buyingFinished = true;
			m_inBuyZone = false;

			if (chanceof(m_skill) && !m_isBomber && !m_isVIP)
				SelectWeaponByName("weapon_knife");
			else
				SelectBestWeapon();

			// TODO: if we are the most rich on the team, drop a weapon for the most poor ebot on the team...

			return;
		}

		PushMessageQueue(CMENU_IDLE);
		PerformWeaponPurchase();

		break;
	}
	case CMENU_RADIO:
	{
		if (!m_isAlive)
			break;

		if (GetGameMode() == GameMode::Original || g_gameVersion & Game::CStrike)
			break;

		if (g_audioTime + 1.0f > engine->GetTime())
			break;

		if (m_chatterTimer + 1.0f > engine->GetTime())
			break;

		// if last bot radio command (global) happened just a 3 seconds ago, delay response
		if ((m_team == Team::Terrorist || m_team == Team::Counter) && (g_lastRadioTime[m_team] + 3.0f < engine->GetTime()))
		{
			// if same message like previous just do a yes/no
			if (m_radioSelect != Radio::Affirmative && m_radioSelect != Radio::Negative)
			{
				if (m_radioSelect == g_lastRadio[m_team] && g_lastRadioTime[m_team] + 1.5 > engine->GetTime())
					m_radioSelect = Radio::Nothin;
				else
				{
					if (m_radioSelect != Radio::ReportingIn)
						g_lastRadio[m_team] = m_radioSelect;
					else
						g_lastRadio[m_team] = Radio::Nothin;

					for (const auto& bot : g_botManager->m_bots)
					{
						if (bot && bot != this && bot->m_team == m_team)
						{
							bot->m_radioOrder = m_radioSelect;
							bot->m_radioEntity = GetEntity();
						}
					}
				}
			}

			if (m_radioSelect != Radio::Nothin)
			{
				if (m_radioSelect != Radio::ReportingIn)
				{
					if (m_radioSelect < Radio::GoGoGo)
						FakeClientCommand(GetEntity(), "radio1");
					else if (m_radioSelect < Radio::Affirmative)
					{
						m_radioSelect -= Radio::GoGoGo - 1;
						FakeClientCommand(GetEntity(), "radio2");
					}
					else
					{
						m_radioSelect -= Radio::Affirmative - 1;
						FakeClientCommand(GetEntity(), "radio3");
					}

					// select correct menu item for this radio message
					FakeClientCommand(GetEntity(), "menuselect %d", m_radioSelect);
				}
			}

			g_lastRadioTime[m_team] = engine->GetTime(); // store last radio usage
		}
		else
			PushMessageQueue(CMENU_RADIO);

		break;
	} // team independent saytext
	case CMENU_SAY:
	{
		ChatSay(false, m_tempStrings);
		break;
	} // team dependent saytext
	case CMENU_TEAMSAY:
	{
		if (GetGameMode() == GameMode::Original || g_gameVersion & Game::CStrike)
			break;

		ChatSay(true, m_tempStrings);
		break;
	}
	}
}

// this function checks for weapon restrictions
bool Bot::IsRestricted(const int weaponIndex)
{
	if (IsNullString(ebot_restrictweapons.GetString()))
		return false;

	if (m_buyingFinished)
		return false;

	Array <String> bannedWeapons = String(ebot_restrictweapons.GetString()).Split(';');
	ITERATE_ARRAY(bannedWeapons, i)
	{
		// check is this weapon is banned
		if (cstrncmp(bannedWeapons[i], STRING(GetWeaponReturn(true, nullptr, weaponIndex)), bannedWeapons[i].GetLength()) == 0)
			return true;
	}

	return false;
}

void Bot::PerformWeaponPurchase(void)
{
	m_nextBuyTime = engine->GetTime();
	WeaponSelect* selectedWeapon = nullptr;

	switch (m_buyState)
	{
	case 0:
	{
		if (!m_favoritePrimary.IsEmpty() && !HasPrimaryWeapon() && !HasShield())
		{
			int i, id;
			char* name;
			for (i = 0; i < m_favoritePrimary.Size(); i++)
			{
				if (HasPrimaryWeapon())
				{
					FakeClientCommand(GetEntity(), "primammo");
					break;
				}

				if (HasShield())
					break;

				name = m_favoritePrimary.Get(i);
				if (IsNullString(name))
					continue;

				id = cclamp(GetWeaponID(name), 0, Const_MaxWeapons);
				if (IsRestricted(id))
					continue;

				if (pev->weapons & (1 << id))
					continue;

				FakeClientCommand(GetEntity(), "%s", name);
			}
		}

		break;
	}
	case 1:
	{
		if ((g_botManager->EconomicsValid(m_team) || HasPrimaryWeapon()) && pev->armorvalue < crandomfloat(40.0f, 80.0f))
		{
			if (m_moneyAmount > 1600 && !IsRestricted(Weapon::KevlarHelmet))
				FakeClientCommand(GetEntity(), "buyequip;menuselect 2");
			else if (m_moneyAmount > 800 && !IsRestricted(Weapon::Kevlar))
				FakeClientCommand(GetEntity(), "buyequip;menuselect 1");
		}

		break;
	}
	case 2:
	{
		if (!m_favoriteSecondary.IsEmpty() && !HasSecondaryWeapon() && (HasPrimaryWeapon() || HasShield()))
		{
			int i, id;
			char* name;
			for (i = 0; i < m_favoriteSecondary.Size(); i++)
			{
				if (HasSecondaryWeapon())
				{
					FakeClientCommand(GetEntity(), "secammo");
					break;
				}

				name = m_favoriteSecondary.Get(i);
				if (IsNullString(name))
					continue;

				id = cclamp(GetWeaponID(name), 0, Const_MaxWeapons);
				if (IsRestricted(id))
					continue;

				if (pev->weapons & (1 << id))
					continue;

				FakeClientCommand(GetEntity(), "%s", name);
			}
		}

		break;
	}
	case 3:
	{
		if (!HasPrimaryWeapon() && !IsRestricted(Weapon::Shield) && !chanceof(m_skill))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 8");
		}

		if (crandomint(1, 2) == 1)
		{
			FakeClientCommand(GetEntity(), "buy;menuselect 1");

			if (crandomint(1, 2) == 1)
				FakeClientCommand(GetEntity(), "menuselect 4");
			else
				FakeClientCommand(GetEntity(), "menuselect 5");
		}

		break;
	}
	case 4:
	{
		if (chanceof(m_skill / 2) && !IsRestricted(Weapon::HeGrenade))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 4");
		}

		if (chanceof(m_skill / 2) && !IsRestricted(Weapon::FbGrenade))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 3");
		}

		if (chanceof(m_skill / 2) && !IsRestricted(Weapon::FbGrenade))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 3");
		}

		if (chanceof(m_skill / 2) && !IsRestricted(Weapon::SmGrenade))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 5");
		}

		break;
	}
	case 5:
	{
		if (g_mapType == MAP_DE && m_team == Team::Counter && chanceof(m_skill) && m_moneyAmount > 800 && !IsRestricted(Weapon::Defuser))
		{
			if (g_gameVersion & Game::Xash)
				FakeClientCommand(GetEntity(), "buyequip;menuselect 6");
			else
				FakeClientCommand(GetEntity(), "defuser"); // use alias in SteamCS
		}

		break;
	}
	case 6:
	{
		int i;
		for (i = 0; i <= 5; i++)
			FakeClientCommand(GetEntity(), "buyammo%d", crandomint(1, 2)); // simulate human

		if (chanceof(m_skill))
			FakeClientCommand(GetEntity(), "buy;menuselect 7");
		else
			FakeClientCommand(GetEntity(), "buy;menuselect 6");

		if (!m_favoriteStuff.IsEmpty())
		{
			char* name;
			for (i = 0; i < m_favoriteStuff.Size(); i++)
			{
				name = m_favoriteStuff.Get(i);
				if (IsNullString(name))
					continue;

				if (IsRestricted(GetWeaponID(name)))
					continue;

				FakeClientCommand(GetEntity(), "%s", name);
			}
		}

		break;
	}
	}

	m_buyState++;
	PushMessageQueue(CMENU_BUY);
}

bool Bot::CheckGrenadeThrow(edict_t* targetEntity)
{
	if (IsZombieMode())
		return false;

	if (FNullEnt(targetEntity))
		return false;

	const int grenadeToThrow = CheckGrenades();
	if (grenadeToThrow == -1)
		return false;

	m_throw = nullvec;
	int th = -1;
	const Vector targetOrigin = GetBottomOrigin(targetEntity);
	if (grenadeToThrow == Weapon::HeGrenade || grenadeToThrow == Weapon::SmGrenade)
	{
		float distance = (targetOrigin - pev->origin).GetLengthSquared();

		// is enemy to high to throw
		if ((targetOrigin.z > (pev->origin.z + 650.0f)) || !(targetEntity->v.flags & (FL_ONGROUND | FL_DUCKING)))
			distance = FLT_MAX; // just some crazy value

		// enemy is within a good throwing distance ?
		if (distance > (grenadeToThrow == Weapon::SmGrenade ? squaredf(400.0f) : squaredf(600.0f)) && distance < squaredf(1200.0f))
		{
			// care about different types of grenades
			if (grenadeToThrow == Weapon::HeGrenade)
			{
				bool allowThrowing = true;

				// check for teammates
				if (engine->IsFriendlyFireOn() && (GetGameMode() == GameMode::Original || GetGameMode() == GameMode::TeamDeathmatch) && GetNearbyFriendsNearPosition(targetOrigin, squaredf(256.0f)))
					allowThrowing = false;

				if (allowThrowing && m_seeEnemyTime + 2.0f < engine->GetTime())
				{
					Vector enemyPredict = ((targetEntity->v.velocity * 0.5f).SkipZ() + targetOrigin);
					int searchTab[4], count = 4;

					float searchRadius = targetEntity->v.velocity.GetLength2D();

					// check the search radius
					if (searchRadius < 128.0f)
						searchRadius = 128.0f;

					// search waypoints
					g_waypoint->FindInRadius(enemyPredict, searchRadius, searchTab, &count);

					while (count)
					{
						allowThrowing = true;

						// check the throwing
						const Vector eyePosition = EyePosition();
						const Vector wpOrigin = g_waypoint->GetPath(searchTab[count--])->origin;
						Vector src = CheckThrow(eyePosition, wpOrigin);

						if (src.GetLengthSquared() < squaredf(100.0f))
							src = CheckToss(eyePosition, wpOrigin);

						if (src == nullvec)
							allowThrowing = false;

						m_throw = src;
						break;
					}
				}

				// start explosive grenade throwing?
				if (allowThrowing)
					th = 1;
			}
			else if (grenadeToThrow == Weapon::SmGrenade && GetShootingConeDeviation(targetEntity, pev->origin) > 0.9f)
				th = 2;
		}
	}
	else if (grenadeToThrow == Weapon::FbGrenade && (targetOrigin - pev->origin).GetLengthSquared() < squaredf(800.0f))
	{
		bool allowThrowing = true;
		MiniArray <int16_t> inRadius;

		g_waypoint->FindInRadius(inRadius, 256.0f, targetOrigin + (targetEntity->v.velocity * 0.5f).SkipZ());

		int16_t i;
		Vector wpOrigin, eyePosition, src;
		for (i = 0; i < inRadius.Size(); i++)
		{
			if ((GetGameMode() == GameMode::Original || GetGameMode() == GameMode::TeamDeathmatch) && GetNearbyFriendsNearPosition(g_waypoint->GetPath(i)->origin, squaredf(256.0f)))
				continue;

			wpOrigin = g_waypoint->GetPath(i)->origin;
			eyePosition = EyePosition();
			src = CheckThrow(eyePosition, wpOrigin);

			if (src.GetLengthSquared() < squaredf(100.0f))
				src = CheckToss(eyePosition, wpOrigin);

			if (src == nullvec)
				continue;

			m_throw = src;
			allowThrowing = true;
			break;
		}

		// start concussion grenade throwing?
		if (allowThrowing)
			th = 3;
	}

	if (m_throw == nullvec)
		return false;

	switch (th)
	{
	case 1:
	{
		SetProcess(Process::ThrowHE, "throwing HE grenade", false, engine->GetTime() + 5.0f);
		break;
	}
	case 2:
	{
		SetProcess(Process::ThrowSM, "throwing SM grenade", false, engine->GetTime() + 5.0f);
		break;
	}
	case 3:
	{
		SetProcess(Process::ThrowFB, "throwing FB grenade", false, engine->GetTime() + 5.0f);
		break;
	}
	}

	return true;
}

bool Bot::IsEnemyReachable(void)
{
	if (FNullEnt(m_nearestEnemy))
		return m_isEnemyReachable = false;

	if (m_isZombieBot)
	{
		if ((pev->origin - m_nearestEnemy->v.origin).GetLengthSquared() > squaredf(416.0f + cabsf(pev->speed * 0.5f)))
			return m_isEnemyReachable = false;
	}
	else
	{
		if (pev->flags & FL_DUCKING || m_nearestEnemy->v.flags & FL_DUCKING) // diff 1
		{
			if ((pev->origin - (m_nearestEnemy->v.origin + m_nearestEnemy->v.velocity.SkipZ() * 0.54f)).GetLengthSquared() > squaredf(128.0f))
				return m_isEnemyReachable = false;
		}
		else if ((pev->origin - (m_nearestEnemy->v.origin + m_nearestEnemy->v.velocity.SkipZ() * 0.54f)).GetLengthSquared() > squaredf(192.0f))
			return m_isEnemyReachable = false;
	}

	if (pev->waterlevel < 2)
	{
		if (m_nearestEnemy->v.origin.z > (pev->origin.z + 62.0f) || m_nearestEnemy->v.origin.z < (pev->origin.z - 100.0f))
			return m_isEnemyReachable = false;
	}

	// be smart
	if (POINT_CONTENTS(m_nearestEnemy->v.origin) == CONTENTS_LAVA)
		return m_isEnemyReachable = false;

	TraceResult tr{};
	TraceHull(pev->origin , m_nearestEnemy->v.origin, true, head_hull, pev->pContainingEntity, &tr);

	// we're not sure, return the current one
	if (tr.fAllSolid)
		return m_isEnemyReachable;

	if (tr.fStartSolid)
		return m_isEnemyReachable = false;

	if (!FNullEnt(tr.pHit))
	{
		if (cstrcmp("func_illusionary", STRING(tr.pHit->v.classname)) == 0)
			return m_isEnemyReachable = false;

		if (GetTeam(tr.pHit) != m_team)
		{
			if (!IsDeadlyDrop(((pev->origin + m_nearestEnemy->v.origin)) * 0.5f))
				return m_isEnemyReachable = true;
			else if (tr.flFraction == 1.0f)
			{
				if (tr.fInWater)
					return m_isEnemyReachable = true;
				else
					return m_isEnemyReachable = false;
			}
		}
	}

	if (tr.flFraction == 1.0f && (!tr.fInWater || !IsDeadlyDrop(((pev->origin + m_nearestEnemy->v.origin)) * 0.5f)))
		return m_isEnemyReachable = true;

	return m_isEnemyReachable = false;
}

bool Bot::IsEnemyReachableToPosition(const Vector& origin)
{
	TraceResult tr{};
	for (const auto& client : g_clients)
	{
		if (client.ent == pev->pContainingEntity)
			continue;

		if (client.team == m_team)
			continue;

		if (!IsAlive(client.ent))
			continue;

		if (pev->flags & FL_DUCKING && client.ent->v.flags & FL_DUCKING) // diff 2
		{
			if ((origin - (client.ent->v.origin + client.ent->v.velocity * 0.54f)).GetLengthSquared() > squaredf(128.0f))
				continue;
		}
		else if ((origin - (client.ent->v.origin + client.ent->v.velocity * 0.54f)).GetLengthSquared() > squaredf(192.0f))
			continue;

		if (pev->waterlevel < 2)
		{
			if (client.ent->v.origin.z > (origin.z + 62.0f) || client.ent->v.origin.z < (origin.z - 100.0f))
				continue;
		}

		TraceHull(origin, client.ent->v.origin, true, head_hull, pev->pContainingEntity, &tr);

		// don't take a risk
		if (tr.fAllSolid)
			return true;

		if (tr.fStartSolid)
			continue;

		if (!FNullEnt(tr.pHit))
		{
			if (cstrcmp("func_illusionary", STRING(tr.pHit->v.classname)) == 0)
				continue;

			if (GetTeam(tr.pHit) != m_team)
			{
				if (!IsDeadlyDrop(((origin + client.ent->v.origin)) * 0.5f))
					return true;
				else if (tr.flFraction == 1.0f)
				{
					if (tr.fInWater)
						return true;
					else
						continue;
				}
			}
		}

		if (tr.flFraction == 1.0f && (!tr.fInWater || !IsDeadlyDrop(((origin + client.ent->v.origin)) * 0.5f)))
			return true;
	}

	return false;
}

bool Bot::IsFriendReachableToPosition(const Vector& origin)
{
	TraceResult tr{};
	for (const auto& client : g_clients)
	{
		if (client.ent == pev->pContainingEntity)
			continue;

		if (client.team != m_team)
			continue;

		if (!IsAlive(client.ent))
			continue;

		if (pev->flags & FL_DUCKING && client.ent->v.flags & FL_DUCKING) // diff 2
		{
			if ((origin - (client.ent->v.origin + client.ent->v.velocity * 0.54f)).GetLengthSquared() > squaredf(128.0f))
				continue;
		}
		else if ((origin - (client.ent->v.origin + client.ent->v.velocity * 0.54f)).GetLengthSquared() > squaredf(192.0f))
			continue;

		if (pev->waterlevel < 2)
		{
			if (client.ent->v.origin.z > (origin.z + 62.0f) || client.ent->v.origin.z < (origin.z - 100.0f))
				continue;
		}

		TraceHull(origin, client.ent->v.origin, true, head_hull, pev->pContainingEntity, &tr);

		// don't take a risk
		if (tr.fAllSolid)
			return true;

		if (tr.fStartSolid)
			continue;

		if (!FNullEnt(tr.pHit))
		{
			if (cstrcmp("func_illusionary", STRING(tr.pHit->v.classname)) == 0)
				continue;

			if (GetTeam(tr.pHit) != m_team)
			{
				if (!IsDeadlyDrop(((origin + client.ent->v.origin)) * 0.5f))
					return true;
				else if (tr.flFraction == 1.0f)
				{
					if (tr.fInWater)
						return true;
					else
						continue;
				}
			}
		}

		if (tr.flFraction == 1.0f && (!tr.fInWater || !IsDeadlyDrop(((origin + client.ent->v.origin)) * 0.5f)))
			return true;
	}

	return false;
}

bool Bot::CanIReachToPosition(const Vector& origin)
{
	if (pev->flags & FL_DUCKING)
	{
		if ((pev->origin - origin).GetLengthSquared() > squaredf(128.0f))
			return false;
	}
	else if ((pev->origin - origin).GetLengthSquared() > squaredf(192.0f))
		return false;

	if (pev->waterlevel < 2)
	{
		if (pev->origin.z > (origin.z + 62.0f) || pev->origin.z < (origin.z - 100.0f))
			return false;
	}

	TraceResult tr{};
	TraceHull(pev->origin, origin, true, head_hull, pev->pContainingEntity, &tr);

	if (tr.fAllSolid)
		return false;

	if (tr.fStartSolid)
		return false;

	if (!FNullEnt(tr.pHit))
	{
		if (cstrcmp("func_illusionary", STRING(tr.pHit->v.classname)) == 0)
			return false;

		if (GetTeam(tr.pHit) != m_team)
		{
			if (!IsDeadlyDrop((pev->origin + origin) * 0.5f))
				return true;
			else if (tr.flFraction == 1.0f)
			{
				if (tr.fInWater)
					return true;
				else
					return false;
			}
		}
	}

	if (tr.flFraction == 1.0f && (!tr.fInWater || !IsDeadlyDrop((pev->origin + origin) * 0.5f)))
		return true;

	return false;
}

int Bot::FindNearestReachableWaypoint(const float maxDistance)
{
	Vector origin;
	int i, index = -1;
	float distance, maxDist = squaredf(maxDistance);
	if (m_isZombieBot)
	{
		bool reachable = false;
		for (i = 0; i < g_numWaypoints; i++)
		{
			origin = g_waypoint->m_paths[i].origin;
			distance = (origin - pev->origin).GetLengthSquared();
			if (distance < maxDist)
			{
				if (IsWaypointOccupied(i))
					continue;

				if (!CanIReachToPosition(origin))
					continue;

				// if we're zombie, we more like to want closer/reachable to humans!
				if (!reachable)
					reachable = IsEnemyReachableToPosition(origin);
				else if (!IsEnemyReachableToPosition(origin))
					continue;

				index = i;
				maxDist = distance;
			}
		}
	}
	else
	{
		for (i = 0; i < g_numWaypoints; i++)
		{
			origin = g_waypoint->m_paths[i].origin;
			distance = (origin - pev->origin).GetLengthSquared();
			if (distance < maxDist)
			{
				if (IsWaypointOccupied(i))
					continue;

				if (!CanIReachToPosition(origin))
					continue;

				if (IsEnemyReachableToPosition(origin))
					continue;

				index = i;
				maxDist = distance;
			}
		}
	}

	return index;
}

void Bot::CheckRadioCommands(void)
{
	if (m_isBomber)
		return;

	if (!m_numFriendsLeft)
		return;

	if (m_radioOrder == Radio::Affirmative || m_radioOrder == Radio::Negative)
		return;

	if (!IsAlive(m_radioEntity))
		return;

	if (m_radioOrder != Radio::ReportTeam && m_radioOrder != Radio::ShesGonnaBlow && !FNullEnt(m_nearestFriend) && m_nearestFriend != m_radioEntity)
	{
		// dynamic range :)
		// less bots = more teamwork required
		if (crandomint(0, m_numFriendsLeft + 1) > 5)
		{
			RadioMessage(Radio::Negative);
			return;
		}
	}

	uint8_t mode = 1;
	switch (m_radioOrder)
	{
	case Radio::FollowMe:
	{
		mode = 2;
		break;
	}
	case Radio::StickTogether:
	{
		mode = 2;
		break;
	}
	case Radio::CoverMe:
	{
		if (g_bombPlanted || m_isBomber)
			mode = 3;
		else
			mode = 2;
		break;
	}
	case Radio::HoldPosition:
	{
		if (g_bombPlanted)
		{
			if (m_inBombZone)
				mode = 3;
		}
		else
			mode = 3;
		break;
	}
	case Radio::TakingFire:
	{
		mode = 2;
		break;
	}
	case Radio::YouTakePoint:
	{
		if ((!g_bombPlanted && m_inBombZone && m_team == Team::Counter) || (g_bombPlanted && m_inBombZone && m_team == Team::Terrorist))
			mode = 3;
		else
			mode = 2;
		break;
	}
	case Radio::EnemySpotted:
	{
		if (m_inBombZone)
			mode = 3;
		else
			mode = 2;
		break;
	}
	case Radio::NeedBackup:
	{
		mode = 2;
		break;
	}
	case Radio::GoGoGo:
	{
		if (GetCurrentState() == Process::Camp)
			FinishCurrentProcess("my teammate told me move");
		RadioMessage(Radio::Affirmative);
		SetWalkTime(-1.0f);
		break;
	}
	case Radio::ShesGonnaBlow:
	{
		if (OutOfBombTimer())
		{
			RadioMessage(Radio::Affirmative);
			SetProcess(Process::Escape, "my teammate told me i must escape from the bomb", true, engine->GetTime() + GetBombTimeleft());
		}
		else
			RadioMessage(Radio::Negative);
		break;
	}
	case Radio::RegroupTeam:
	{
		mode = 2;
		break;
	}
	case Radio::StormTheFront:
	{
		if (crandomint(1, 2) == 1)
			RadioMessage(Radio::Affirmative);
		else
			RadioMessage(Radio::Negative);
		break;
	}
	case Radio::Fallback:
	{
		RadioMessage(Radio::Negative);
		break;
	}
	case Radio::ReportTeam:
	{
		if (m_enemySeeTime + 6.0f < engine->GetTime())
		{
			if (!IsAlive(m_nearestEnemy))
				RadioMessage(Radio::EnemyDown);
			else if (m_hasEnemiesNear)
			{
				if (!FNullEnt(m_nearestEnemy) && IsAttacking(m_nearestEnemy))
					RadioMessage(Radio::TakingFire);
				else
					RadioMessage(Radio::NeedBackup);
			}
			else
				RadioMessage(Radio::EnemySpotted);
		}
		else if (GetCurrentState() == Process::Camp)
			RadioMessage(Radio::InPosition);
		else if (m_enemySeeTime + 16.0f < engine->GetTime())
			RadioMessage(Radio::SectorClear);
		break;
	}
	case Radio::SectorClear:
	{
		if (GetCurrentState() == Process::Camp && m_enemySeeTime + 12.0f < engine->GetTime())
		{
			RadioMessage(Radio::Affirmative);
			FinishCurrentProcess("my teammate told me sector is clear");
		}
		break;
	}
	case Radio::GetInPosition:
	{
		mode = 3;
		break;
	}
	}

	switch (mode)
	{
	case 2:
	{
		int index = g_waypoint->FindNearest(m_radioEntity->v.origin, 99999999.0f, -1, m_radioEntity);
		if (IsValidWaypoint(index))
		{
			RadioMessage(Radio::Affirmative);
			FindPath(m_currentWaypointIndex, index);
		}
		else
			RadioMessage(Radio::Negative);

		break;
	}
	case 3:
	{
		const int index = FindDefendWaypoint(m_radioEntity->v.origin + m_radioEntity->v.view_ofs);
		if (IsValidWaypoint(index))
		{
			RadioMessage(Radio::Affirmative);
			m_campIndex = index;
			SetProcess(Process::Camp, "i will hold this position for my teammate", true, engine->GetTime() + crandomfloat(ebot_camp_min.GetFloat(), ebot_camp_max.GetFloat()));
		}
		else
			RadioMessage(Radio::Negative);

		break;
	}
	default:
		RadioMessage(Radio::Negative);
	}

	m_radioOrder = 0; // radio command has been handled, reset
}

void Bot::SetWalkTime(const float time)
{
	if (g_gameVersion & Game::HalfLife)
		return;

	if (!ebot_walkallow.GetBool())
		return;

	if (IsZombieMode())
		return;

	if (!m_numEnemiesLeft)
		return;

	if (!m_buyingFinished)
		return;

	const float time2 = engine->GetTime();
	if ((m_spawnTime + engine->GetFreezeTime() + 7.0f) > time2)
		return;

	m_walkTime = time2 + (time + 1.0f - (pev->health / pev->max_health));
}

float Bot::GetMaxSpeed(void)
{
	if (m_waypoint.flags & WAYPOINT_LADDER)
	{
		if (IsOnLadder())
		{
			float maxSpeed = (pev->origin - m_destOrigin).GetLength();
			if (maxSpeed > pev->maxspeed)
				maxSpeed = pev->maxspeed;
			else
			{
				const float minSpeed = pev->maxspeed * 0.502f;
				if (maxSpeed < minSpeed)
					maxSpeed = minSpeed;
			}

			m_moveSpeed = maxSpeed;
			return maxSpeed;
		}
		else
		{
			float minSpeed = pev->maxspeed * 0.502f;
			m_moveSpeed = minSpeed;

			if (CheckWallOnRight())
				m_strafeSpeed = -minSpeed;

			if (CheckWallOnLeft())
			{
				if (m_strafeSpeed == 0.0f)
					m_strafeSpeed = minSpeed;
				else
					m_strafeSpeed = 0.0f;
			}

			return minSpeed;
		}
	}

	if (!IsOnFloor())
		return pev->maxspeed;

	if (pev->flags & FL_DUCKING)
	{
		if (!IsOnLadder())
			pev->speed = pev->maxspeed;
		return pev->maxspeed;
	}

	if (m_walkTime > engine->GetTime())
		return pev->maxspeed * 0.502f;

	return pev->maxspeed;
}

bool Bot::IsNotAttackLab(edict_t* entity)
{
	if (FNullEnt(entity))
		return true;

	if (entity->v.flags & FL_NOTARGET)
		return true;

	if (entity->v.takedamage == DAMAGE_NO)
		return true;

	if (entity->v.rendermode == kRenderTransAlpha)
	{
		// enemy fires gun
		if (entity->v.weapons & WeaponBits_Primary || (entity->v.weapons & WeaponBits_Secondary && entity->v.buttons & IN_ATTACK) || entity->v.oldbuttons & IN_ATTACK)
			return false;

		const float renderamt = entity->v.renderamt;
		if (renderamt < 32.0f)
			return true;

		if (renderamt > 168.0f)
			return false;

		return (squaredf(renderamt) < (entity->v.origin - pev->origin).GetLengthSquared2D());
	}

	return false;
}

static float tempTimer;
void Bot::BaseUpdate(void)
{
	// i'm really tired of getting random pev is nullptr debug logs...
	// might seems ugly and useless, but... i have made some experiments with it,
	// still not sure exactly why, but bad third party server plugins can cause this
	// always use quality check on this...
	if (!pev || !pev->pContainingEntity)
		return;

	tempTimer = engine->GetTime();
	if (m_baseUpdate < tempTimer)
	{
		// reset
		m_updateLooking = false;
		pev->impulse = 0;
		pev->buttons = 0;
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;

		// if the bot hasn't selected stuff to start the game yet, go do that...
		if (m_notStarted)
			StartGame();
		else
		{
			if (m_slowthinktimer > tempTimer)
				m_isSlowThink = false;
			else
			{
				m_isSlowThink = true;
				CheckSlowThink();
				m_slowthinktimer = tempTimer + crandomfloat(0.9f, 1.1f);

				if (!m_isAlive)
				{
					if (!g_isFakeCommand)
					{
						extern ConVar ebot_random_join_quit;
						if (ebot_random_join_quit.GetBool() && m_stayTime > 0.0f && m_stayTime < tempTimer)
						{
							Kick();
							return;
						}

						extern ConVar ebot_chat;
						if (ebot_chat.GetBool() && m_lastChatTime + 10.0f < tempTimer && g_lastChatTime + 5.0f < tempTimer && !RepliesToPlayer()) // bot chatting turned on?
						{
							m_lastChatTime = tempTimer;
							if (!g_chatFactory[CHAT_DEAD].IsEmpty())
							{
								g_lastChatTime = tempTimer;

								char* pickedPhrase = g_chatFactory[CHAT_DEAD].GetRandomElement();
								bool sayBufferExists = false;

								// search for last messages, sayed
								ITERATE_ARRAY(m_sayTextBuffer.lastUsedSentences, i)
								{
									if (cstrncmp(m_sayTextBuffer.lastUsedSentences[i], pickedPhrase, m_sayTextBuffer.lastUsedSentences[i].GetLength()) == 0)
										sayBufferExists = true;
								}

								if (!sayBufferExists)
								{
									PrepareChatMessage(pickedPhrase);
									PushMessageQueue(CMENU_SAY);

									// add to ignore list
									m_sayTextBuffer.lastUsedSentences.Push(pickedPhrase);
								}

								// clear the used line buffer every now and then
								if (m_sayTextBuffer.lastUsedSentences.GetElementNumber() > crandomint(4, 6))
									m_sayTextBuffer.lastUsedSentences.Destroy();
							}
						}

						if (g_gameVersion & Game::HalfLife && !(pev->buttons & IN_ATTACK) && !(pev->oldbuttons & IN_ATTACK))
							pev->buttons |= IN_ATTACK;
					}
				}
			}

			if (m_isAlive)
			{
				if (g_gameVersion & Game::HalfLife)
				{
					// idk why ???
					if (pev->maxspeed < 11.0f)
					{
						const cvar_t* maxSpeed = g_engfuncs.pfnCVarGetPointer("sv_maxspeed");
						if (maxSpeed)
							pev->maxspeed = maxSpeed->value;
						else // default is 270
							pev->maxspeed = 270.0f;
					}
				}

				if (m_buyingFinished && !ebot_stopbots.GetBool())
				{
					UpdateProcess();
					MoveAction();
					DebugModeMsg();
				}
				else
					ResetStuck();
			}
		}

		// check for pending messages
		CheckMessageQueue();

		// avoid frame drops
		m_frameInterval = tempTimer - m_frameDelay;
		m_frameDelay = tempTimer;
		m_baseUpdate = tempTimer + 0.1f;
	}
	else
	{
		float delta = tempTimer - m_aimInterval;
		m_aimInterval += delta;

		if (delta > 0.05f)
			delta = 0.05f;

		if (m_navNode.CanFollowPath())
		{
			m_strafeSpeed = 0.0f;
			m_moveSpeed = GetMaxSpeed();
			m_pathInterval = delta;
			DoWaypointNav();
			CheckStuck(m_moveSpeed + cabsf(m_strafeSpeed), m_pathInterval);
			m_moveAngles = (m_destOrigin - pev->origin).ToAngles();
			m_moveAngles.x = -m_moveAngles.x; // invert for engine
		}

		if (m_lookVelocity != nullvec)
		{
			m_lookAt.x += m_lookVelocity.x * delta;
			m_lookAt.y += m_lookVelocity.y * delta;
			m_lookAt.z += m_lookVelocity.z * delta;
		}

		// adjust all body and view angles to face an absolute vector
		Vector direction = (m_lookAt - EyePosition()).ToAngles() + pev->punchangle;
		direction.x = -direction.x; // invert for engine

		const float angleDiffPitch = AngleNormalize(direction.x - pev->v_angle.x);
		const float angleDiffYaw = AngleNormalize(direction.y - pev->v_angle.y);
		const float lockn = 0.128f / delta;

		if (cabsf(angleDiffYaw) < lockn)
		{
			m_lookYawVel = 0.0f;
			pev->v_angle.y = AngleNormalize(direction.y);
		}
		else
		{
			const float fskill = static_cast<float>(m_skill);
			const float accelerate = fskill * 40.0f;
			m_lookYawVel += delta * cclampf((fskill * 4.0f * angleDiffYaw) - (fskill * 0.4f * m_lookYawVel), -accelerate, accelerate);
			pev->v_angle.y += delta * m_lookYawVel;
		}

		if (cabsf(angleDiffPitch) < lockn)
		{
			m_lookPitchVel = 0.0f;
			pev->v_angle.x = AngleNormalize(direction.x);
		}
		else
		{
			const float fskill = static_cast<float>(m_skill);
			const float accelerate = fskill * 40.0f;
			m_lookPitchVel += delta * cclampf(fskill * 8.0f * angleDiffPitch - (fskill * 0.4f * m_lookPitchVel), -accelerate, accelerate);
			pev->v_angle.x += delta * m_lookPitchVel;
		}

		if (pev->v_angle.x < -89.0f)
			pev->v_angle.x = -89.0f;
		else if (pev->v_angle.x > 89.0f)
			pev->v_angle.x = 89.0f;

		// set the body angles to point the gun correctly
		pev->angles.x = -pev->v_angle.x * 0.33333333333f;
		pev->angles.y = pev->v_angle.y;

		// run playermovement
		if (pev->pContainingEntity)
			PLAYER_RUN_MOVE(pev->pContainingEntity, m_moveAngles, m_moveSpeed, m_strafeSpeed, 0.0f, static_cast<uint16_t>(pev->buttons), static_cast<uint16_t>(pev->impulse), static_cast<uint8_t>(cclampf(((tempTimer - m_msecInterval) * 1000.0f), 0.0f, 255.0f)));
		m_msecInterval = tempTimer;
	}
}

void Bot::CheckSlowThink(void)
{
	CalculatePing();

	tempTimer = engine->GetTime();
	if (m_stayTime < 0.0f)
	{
		extern ConVar ebot_random_join_quit;
		if (ebot_random_join_quit.GetBool())
		{
			extern ConVar ebot_stay_min;
			extern ConVar ebot_stay_max;
			m_stayTime = tempTimer + crandomfloat(ebot_stay_min.GetFloat(), ebot_stay_max.GetFloat());
		}
		else
			m_stayTime = tempTimer + 999999.0f;
	}

	if (m_isZombieBot)
	{
		m_isBomber = false;
		SelectKnife();
	}
	else
	{
		if (pev->weapons & (1 << Weapon::C4))
			m_isBomber = true;
		else
			m_isBomber = false;

		if (!m_hasEnemiesNear && !m_hasEntitiesNear && GetCurrentState() != Process::ThrowHE && GetCurrentState() != Process::ThrowFB && GetCurrentState() != Process::ThrowSM && !(pev->buttons & IN_ATTACK) && !(pev->oldbuttons & IN_ATTACK))
		{
			m_isEnemyReachable = false;

			// check reloadable weapons
			WeaponSelect* selectTab = (g_gameVersion & Game::HalfLife) ? &g_weaponSelectHL[0] : &g_weaponSelect[0];
			int id;
			int chosenWeaponIndex = -1;
			int selectIndex = 0;
			while (id = selectTab[selectIndex].id)
			{
				if (!(pev->weapons & (1 << id)))
				{
					selectIndex++;
					continue;
				}

				if (m_ammo[g_weaponDefs[id].ammo1] > 0 && m_ammoInClip[id] < g_weaponDefs[id].ammo1Max)
					chosenWeaponIndex = selectIndex;

				selectIndex++;
			}

			if (chosenWeaponIndex != -1)
			{
				if (m_currentWeapon != selectTab[chosenWeaponIndex].id)
					SelectWeaponByName(selectTab[chosenWeaponIndex].weaponName);
				else if (!(pev->oldbuttons & IN_RELOAD))
					pev->buttons |= IN_RELOAD;
			}
		}
	}

	extern ConVar ebot_ignore_enemies;
	if (!ebot_ignore_enemies.GetBool() && m_randomattacktimer < tempTimer && !engine->IsFriendlyFireOn() && !HasHostage()) // simulate players with random knife attacks
	{
		if (m_isStuck && m_personality != Personality::Careful)
		{
			if (m_personality == Personality::Rusher)
				m_randomattacktimer = 0.0f;
			else
				m_randomattacktimer = tempTimer + crandomfloat(0.1f, 10.0f);
		}
		else if (m_personality == Personality::Rusher)
			m_randomattacktimer = tempTimer + crandomfloat(0.1f, 30.0f);
		else if (m_personality == Personality::Careful)
			m_randomattacktimer = tempTimer + crandomfloat(10.0f, 100.0f);
		else
			m_randomattacktimer = tempTimer + crandomfloat(0.15f, 75.0f);

		if (m_currentWeapon == Weapon::Knife)
		{
			if (crandomint(1, 3) == 1)
				pev->buttons |= IN_ATTACK;
			else
				pev->buttons |= IN_ATTACK2;
		}
	}

	if (!pev->pContainingEntity)
	{
		pev->pContainingEntity = GetEntity();
		return;
	}

	m_isZombieBot = IsZombieEntity(pev->pContainingEntity);
	m_team = GetTeam(pev->pContainingEntity);
	m_isAlive = IsAlive(pev->pContainingEntity);
	m_index = GetIndex();

	if (ebot_use_radio.GetInt() == 2)
	{
		if (m_chatterTimer < engine->GetTime())
			SwitchChatterIcon(false);
	}

	if (g_gameVersion != Game::HalfLife)
	{
		// don't forget me :(
		/*if (ebot_use_flare.GetBool() && !m_isZombieBot && GetGameMode() == GameMode::ZombiePlague && FNullEnt(m_enemy) && !FNullEnt(m_lastEnemy))
		{
			if (pev->weapons & (1 << Weapon::SmGrenade) && chanceof(40))
				PushTask(TASK_THROWFLARE, TASKPRI_THROWGRENADE, -1, crandomfloat(0.6f, 0.9f), false);
		}*/

		// zp & biohazard flashlight support
		if (ebot_force_flashlight.GetBool())
		{
			if (!m_isZombieBot && !(pev->effects & EF_DIMLIGHT) && pev->impulse != 100)
				pev->impulse = 100;
		}
		else
		{
			if (pev->effects & EF_DIMLIGHT && pev->impulse != 100)
				pev->impulse = 100;
		}

	}
	else
	{
		m_startAction = CMENU_IDLE;
		m_numFriendsLeft = 0;
	}

	// host wants the bots to vote for a map?
	if (m_voteMap)
	{
		FakeClientCommand(GetEntity(), "votemap %d", m_voteMap);
		m_voteMap = 0;
	}
}

bool Bot::IsAttacking(const edict_t* player)
{
	return (player->v.buttons & IN_ATTACK || player->v.oldbuttons & IN_ATTACK);
}

void Bot::UpdateLooking(void)
{
	if (m_hasEnemiesNear || m_hasEntitiesNear)
	{
		if (m_entityDistance < m_enemyDistance)
		{
			if (!FNullEnt(m_nearestEntity))
			{
				LookAt(m_entityOrigin, m_nearestEntity->v.velocity);
				FireWeapon();
				return;
			}
		}
		else if (IsAlive(m_nearestEnemy))
		{
			LookAt(m_enemyOrigin, m_nearestEnemy->v.velocity);
			FireWeapon();
			return;
		}
	}
	else
	{
		const float val = IsZombieMode() ? 8.0f : 4.0f;
		if (m_enemySeeTime + val > engine->GetTime() && m_team != GetTeam(m_nearestEnemy))
		{
			if (IsAlive(m_nearestEnemy))
				LookAt(m_nearestEnemy->v.origin + m_nearestEnemy->v.view_ofs);
			else
				LookAt(m_enemyOrigin);

			return;
		}
	}

	LookAtAround();
}

float Bot::GetTargetDistance(void)
{
	if (m_entityDistance < m_enemyDistance)
		return m_entityDistance;

	return m_enemyDistance;
}

void Bot::LookAtAround(void)
{
	m_updateLooking = true;
	if (m_waypoint.flags & WAYPOINT_LADDER || IsOnLadder())
	{
		m_lookAt = m_destOrigin + pev->view_ofs;
		m_pauseTime = 0.0f;
		return;
	}
	else if (m_waypoint.flags & WAYPOINT_USEBUTTON)
	{
		const Vector origin = m_waypoint.origin;
		if ((pev->origin - origin).GetLengthSquared() < squaredf(80.0f))
		{
			if (g_gameVersion & Game::Xash)
			{
				m_lookAt = origin;
				if (!(pev->buttons & IN_USE) && !(pev->oldbuttons & IN_USE))
					pev->buttons |= IN_USE;
				return;
			}
			else
			{
				edict_t* button = FindButton();
				if (button) // no need to look at, thanks to the game...
					MDLL_Use(button, pev->pContainingEntity);
				else
				{
					m_lookAt = origin;
					if (!(pev->buttons & IN_USE) && !(pev->oldbuttons & IN_USE))
						pev->buttons |= IN_USE;
					return;
				}
			}
		}
	}
	else if (!m_isZombieBot && m_hasFriendsNear && !FNullEnt(m_nearestFriend) && IsAttacking(m_nearestFriend) && cstrncmp(STRING(m_nearestFriend->v.viewmodel), "models/v_knife", 14) != 0)
	{
		auto bot = g_botManager->GetBot(m_nearestFriend);
		if (bot)
		{
			m_lookAt = bot->m_lookAt;

			if (bot->m_hasEnemiesNear)
			{
				edict_t* enemy = bot->m_nearestEnemy;
				if (!FNullEnt(enemy))
				{
					CheckGrenadeThrow(enemy);

					int index;
					edict_t* ent = bot->GetEntity();
					if ((m_currentWeapon == Weapon::M3 || m_currentWeapon == Weapon::Xm1014 || m_currentWeapon == Weapon::M249))
					{
						index = g_waypoint->FindNearest(GetEntityOrigin(enemy), 999999.0f, -1, ent);
						if (IsValidWaypoint(index) && m_navNode.Last() != index)
						{
							RadioMessage(Radio::GetInPosition);
							FindPath(m_currentWaypointIndex, index, enemy);
							index = FindDefendWaypoint(EyePosition());
							if (IsValidWaypoint(index))
							{
								bot->RadioMessage(Radio::Affirmative);
								bot->m_campIndex = index;
								bot->SetProcess(Process::Camp, "i will hold this position for my teammate's plan, my teammate will flank the enemy!", true, engine->GetTime() + ebot_camp_max.GetFloat());
							}
							else
								bot->RadioMessage(Radio::Negative);
						}
					}

					if (bot->GetCurrentState() == Process::Camp)
					{
						index = g_waypoint->FindNearest(m_lookAt, 999999.0f, -1, ent);
						if (IsValidWaypoint(index) && m_navNode.Last() != index)
						{
							RadioMessage(Radio::GetInPosition);
							FindPath(m_currentWaypointIndex, index);
						}
					}
					else if (GetCurrentState() == Process::Camp)
					{
						index = g_waypoint->FindNearest(m_lookAt, 999999.0f, -1, ent);
						if (IsValidWaypoint(index) && m_navNode.Last() != index)
						{
							FinishCurrentProcess("i need to help my friend");
							RadioMessage(Radio::Fallback);
							FindPath(m_currentWaypointIndex, index);
						}
					}
				}
			}
		}
		else
		{
			TraceResult tr{};
			const auto eyePosition = m_nearestFriend->v.origin + m_nearestFriend->v.view_ofs;
			MakeVectors(m_nearestFriend->v.angles);
			TraceLine(eyePosition, eyePosition + g_pGlobals->v_forward * 2000.0f, false, false, m_nearestFriend, &tr);

			if (tr.pHit != pev->pContainingEntity)
				m_lookAt = GetEntityOrigin(tr.pHit);
			else
				m_lookAt = tr.vecEndPos;
		}

		m_pauseTime = engine->GetTime() + crandomfloat(2.0f, 5.0f);
		SetWalkTime(7.0f);

		if (m_currentWeapon == Weapon::Knife)
			SelectBestWeapon();

		return;
	}

	const float time = engine->GetTime();
	if (m_pauseTime > time)
		return;

	if (m_isSlowThink)
	{
		if (m_isZombieBot)
		{
			if (GetGameMode() == GameMode::ZombieHell)
			{
				if (!IsAlive(m_nearestEnemy) || GetTeam(m_nearestEnemy) == m_team)
				{
					int goal;
					for (const auto& client : g_clients)
					{
						if (FNullEnt(client.ent))
							continue;

						// we can see our friends in radar... let bots act like that.
						if (client.team == m_team)
							continue;

						if (!(client.flags & CFLAG_USED))
							continue;

						if (!(client.flags & CFLAG_ALIVE))
							continue;

						if (client.ent->v.flags & FL_NOTARGET)
							continue;

						if (!m_hasEnemiesNear && (FNullEnt(m_nearestEnemy) || GetTeam(m_nearestEnemy) == m_team))
						{
							m_nearestEnemy = client.ent;
							if (!m_navNode.IsEmpty())
							{
								goal = g_waypoint->FindNearest(m_nearestEnemy->v.origin, 9999999999.0f, -1, m_nearestEnemy);
								if (IsValidWaypoint(goal) && m_navNode.Last() != goal)
									m_navNode.Clear();
							}
						}

						m_pauseTime = time + crandomfloat(2.0f, 5.0f);
						m_lookAt = client.ent->v.origin + client.ent->v.view_ofs;
						CheckGrenadeThrow(client.ent);
						return;
					}
				}
			}
			else if (!IsAlive(m_nearestEnemy) || GetTeam(m_nearestEnemy) == m_team)
			{
				float maxDist;
				int goal;
				for (const auto& client : g_clients)
				{
					if (FNullEnt(client.ent))
						continue;

					// we can see our friends in radar... let bots act like that.
					if (client.team == m_team)
						continue;

					if (!(client.flags & CFLAG_USED))
						continue;

					if (!(client.flags & CFLAG_ALIVE))
						continue;

					if (client.ent->v.flags & FL_NOTARGET)
						continue;

					maxDist = 1280.0f;
					if (!IsAttacking(client.ent))
					{
						if ((client.ent->v.flags & FL_DUCKING))
							continue;

						if (client.ent->v.speed < client.ent->v.maxspeed * 0.66f)
							continue;

						if (chanceof(m_senseChance))
							maxDist = 768.0f;
						else
							maxDist = 256.0f;
					}

					if (((pev->origin + pev->velocity) - (client.ent->v.origin + client.ent->v.velocity)).GetLengthSquared() > squaredf(maxDist))
						continue;

					if (!m_hasEnemiesNear && (FNullEnt(m_nearestEnemy) || GetTeam(m_nearestEnemy) == m_team))
					{
						m_nearestEnemy = client.ent;
						if (!m_navNode.IsEmpty())
						{
							goal = g_waypoint->FindNearest(m_nearestEnemy->v.origin, 9999999999.0f, -1, m_nearestEnemy);
							if (IsValidWaypoint(goal) && m_navNode.Last() != goal)
								m_navNode.Clear();
						}
					}

					m_pauseTime = time + crandomfloat(2.0f, 5.0f);
					m_lookAt = client.ent->v.origin + client.ent->v.view_ofs;
					CheckGrenadeThrow(client.ent);
					return;
				}
			}
		}
		else if (chanceof(m_senseChance)) // who's footsteps is this or fire sounds?
		{
			int index;
			float maxDist;
			for (const auto& client : g_clients)
			{
				if (FNullEnt(client.ent))
					continue;

				// we can see our friends in radar... let bots act like that.
				if (client.team == m_team)
					continue;

				if (!(client.flags & CFLAG_USED))
					continue;

				if (!(client.flags & CFLAG_ALIVE))
					continue;

				if (client.ent->v.flags & FL_NOTARGET)
					continue;

				maxDist = 1280.0f;
				if (!IsAttacking(client.ent))
				{
					if ((client.ent->v.flags & FL_DUCKING))
						continue;

					if (client.ent->v.speed < client.ent->v.maxspeed * 0.66f)
						continue;

					maxDist = 768.0f;
				}

				if (((pev->origin + pev->velocity) - (client.ent->v.origin + client.ent->v.velocity)).GetLengthSquared() > squaredf(maxDist))
					continue;

				if (m_skill > (m_personality == Personality::Rusher ? 30 : 10))
				{
					SetWalkTime(7.0f);
					SelectBestWeapon();
				}

				m_pauseTime = time + crandomfloat(2.0f, 5.0f);
				m_lookAt = client.ent->v.origin + client.ent->v.view_ofs;
				if (!CheckGrenadeThrow(client.ent) && !IsZombieMode())
				{
					if (pev->health > client.ent->v.health || m_currentWeapon == Weapon::M3 || m_currentWeapon == Weapon::Xm1014 || m_currentWeapon == Weapon::M249)
					{
						index = m_navNode.Last();
                        FindPath(m_currentWaypointIndex, index, client.ent);
					}
					else
					{
						index = FindDefendWaypoint(EyePosition());
                        if (IsValidWaypoint(index))
                        {
                            m_campIndex = index;
                            SetProcess(Process::Camp, "i'm waiting my enemy", true, time + ebot_camp_max.GetFloat());
                        }
						else if (chanceof(m_skill))
							m_duckTime = m_pauseTime;
					}
				}

				return;
			}

			// be careful
			if (!IsZombieMode() && (pev->origin - m_avgDeathOrigin).GetLengthSquared() < squaredf(512.0f))
			{
				SetWalkTime(7.0f);
				SelectBestWeapon();
			}
		}
	}

	if (m_isZombieBot || m_searchTime > time)
	{
		if (m_navNode.HasNext())
		{
			const Path* next = g_waypoint->GetPath(m_navNode.Next());
			if (next)
				m_lookAt = next->origin + pev->view_ofs;
		}

		return;
	}

	const Vector nextFrame = EyePosition() + pev->velocity;
	Vector selectRandom;
	Vector bestLookPos = nextFrame;
	bestLookPos.x += crandomfloat(-1024.0f, 1024.0f);
	bestLookPos.y += crandomfloat(-1024.0f, 1024.0f);
	bestLookPos.z += crandomfloat(-256.0f, 256.0f);

	const int index = g_waypoint->FindNearestInCircle(bestLookPos, 999999.0f);
	if (IsValidWaypoint(index) && m_knownWaypointIndex[0] != index && m_knownWaypointIndex[1] != index && m_knownWaypointIndex[2] != index && m_knownWaypointIndex[3] != index && m_knownWaypointIndex[4] != index && m_knownWaypointIndex[5] != index)
	{
		const Path* pointer = g_waypoint->GetPath(index);
		if (pointer)
		{
			const Vector waypointOrigin = pointer->origin;
			const float waypointRadius = static_cast<float>(pointer->radius);
			selectRandom.x = waypointOrigin.x + crandomfloat(-waypointRadius, waypointRadius);
			selectRandom.y = waypointOrigin.y + crandomfloat(-waypointRadius, waypointRadius);
			selectRandom.z = waypointOrigin.z + pev->view_ofs.z;
			m_knownWaypointIndex[0] = index;
			m_knownWaypointIndex[1] = m_knownWaypointIndex[0];
			m_knownWaypointIndex[2] = m_knownWaypointIndex[1];
			m_knownWaypointIndex[3] = m_knownWaypointIndex[2];
			m_knownWaypointIndex[4] = m_knownWaypointIndex[3];
			m_knownWaypointIndex[5] = m_knownWaypointIndex[4];
		}
	}

	auto isVisible = [&](void)
	{
		TraceResult tr{};
		TraceLine(EyePosition(), selectRandom, true, true, pev->pContainingEntity, &tr);
		if (tr.flFraction == 1.0f)
			return true;

		return false;
	};

	if ((pev->origin - selectRandom).GetLengthSquared() > squaredf(512.0f) && isVisible())
	{
		m_lookAt = selectRandom;
		m_pauseTime = time + crandomfloat(1.05f, 1.45f);
	}

	m_searchTime = time + 0.25f;
}

void Bot::CalculatePing(void)
{
	extern ConVar ebot_ping;
	if (!ebot_ping.GetBool())
		return;

	// save cpu power if no one is lookin' at scoreboard...
	if (g_fakePingUpdate < engine->GetTime())
		return;

	int averagePing = 0;
	int numHumans = 0;
	int ping, loss;

	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (IsValidBot(client.index))
			continue;

		numHumans++;
		PLAYER_CNX_STATS(client.ent, &ping, &loss);

		if (!ping || ping > 150)
			ping = crandomint(5, 50);

		averagePing += ping;
	}

	if (numHumans)
		averagePing /= numHumans;
	else
		averagePing = crandomint(30, 60);

	int botPing = m_basePingLevel + crandomint(averagePing - averagePing * 0.2f, averagePing + averagePing * 0.2f) + crandomint(m_difficulty + 3, m_difficulty + 6);
	if (botPing < 9)
		botPing = crandomint(9, 19);
	else if (botPing > 133)
		botPing = crandomint(99, 119);

	int j;
	for (j = 0; j < 2; j++)
	{
		for (m_pingOffset[j] = 0; m_pingOffset[j] < 4; m_pingOffset[j]++)
		{
			if (!((botPing - m_pingOffset[j]) % 4))
			{
				m_ping[j] = (botPing - m_pingOffset[j]) * 0.25f;
				break;
			}
		}
	}

	m_ping[2] = botPing;
}

void Bot::MoveAction(void)
{
	const float time = engine->GetTime();
	if (pev->buttons & IN_JUMP)
		m_jumpTime = time;

	if (m_jumpTime + 2.0f > time)
	{
		if (!IsOnFloor() && !IsInWater() && !IsOnLadder())
		{
			pev->flags |= FL_DUCKING;
			pev->buttons |= IN_DUCK;
		}
	}
	else
	{
		if (m_moveSpeed > 0.0f)
			pev->buttons |= IN_FORWARD;
		else if (m_moveSpeed < 0.0f)
			pev->buttons |= IN_BACK;

		if (m_strafeSpeed > 0.0f)
			pev->buttons |= IN_MOVERIGHT;
		else if (m_strafeSpeed < 0.0f)
			pev->buttons |= IN_MOVELEFT;

		if (m_duckTime > time)
			pev->buttons |= IN_DUCK;
	}
}

void Bot::DebugModeMsg(void)
{
	const int debugMode = ebot_debug.GetInt();
	if (debugMode <= 0 || debugMode == 2 || FNullEnt(g_hostEntity))
		return;

	static float timeDebugUpdate = 0.0f;

	const int specIndex = g_hostEntity->v.iuser2;
	if (specIndex != m_index)
		return;

	static int index{}, goal{};
	static Process processID{}, rememberedProcessID{};

	if (processID != m_currentProcess || rememberedProcessID != m_rememberedProcess || index != m_currentWaypointIndex || goal != m_navNode.Last() || timeDebugUpdate < engine->GetTime())
	{
		processID = m_currentProcess;
		index = m_currentWaypointIndex;
		goal = m_navNode.Last();

		char processName[80];
		sprintf(processName, GetProcessName(m_currentProcess));

		char rememberedProcessName[80];
		sprintf(rememberedProcessName, GetProcessName(m_rememberedProcess));

		char weaponName[80], botType[32];
		char enemyName[80], friendName[80];

		if (IsAlive(m_nearestEnemy))
			sprintf(enemyName, "%s", GetEntityName(m_nearestEnemy));
		else if (IsAlive(m_nearestEntity))
			sprintf(enemyName, "%s", GetEntityName(m_nearestEntity));
		else
			sprintf(enemyName, "%s", GetEntityName(nullptr));

		if (IsAlive(m_nearestFriend))
			sprintf(friendName, "%s", GetEntityName(m_nearestFriend));
		else
			sprintf(friendName, "%s", GetEntityName(nullptr));

		WeaponSelect* selectTab = &g_weaponSelect[0];
		char weaponCount = 0;

		while (m_currentWeapon != selectTab->id && weaponCount < Const_NumWeapons)
		{
			selectTab++;
			weaponCount++;
		}

		// set the bot type
		sprintf(botType, "%s%s%s", m_personality == Personality::Rusher ? "Rusher" : "",
			m_personality == Personality::Careful ? "Careful" : "",
			m_personality == Personality::Normal ? "Normal" : "");

		if (weaponCount >= Const_NumWeapons)
		{
			// prevent printing unknown message from known weapons
			switch (m_currentWeapon)
			{
			case Weapon::HeGrenade:
			{
				sprintf(weaponName, "weapon_hegrenade");
				break;
			}
			case Weapon::FbGrenade:
			{
				sprintf(weaponName, "weapon_flashbang");
				break;
			}
			case Weapon::SmGrenade:
			{
				sprintf(weaponName, "weapon_smokegrenade");
				break;
			}
			case Weapon::C4:
			{
				sprintf(weaponName, "weapon_c4");
				break;
			}
			default:
				sprintf(weaponName, "Unknown! (%d)", m_currentWeapon);
			}
		}
		else
			sprintf(weaponName, selectTab->weaponName);

		char gamemodName[80];
		switch (GetGameMode())
		{
		case GameMode::Original:
		{
			sprintf(gamemodName, "Original");
			break;
		}
		case GameMode::Deathmatch:
		{
			sprintf(gamemodName, "Deathmatch");
			break;
		}
		case GameMode::ZombiePlague:
		{
			sprintf(gamemodName, "Zombie Mode");
			break;
		}
		case GameMode::NoTeam:
		{
			sprintf(gamemodName, "No Team");
			break;
		}
		case GameMode::ZombieHell:
		{
			sprintf(gamemodName, "Zombie Hell");
			break;
		}
		case GameMode::TeamDeathmatch:
		{
			sprintf(gamemodName, "Team Deathmatch");
			break;
		}
		default:
			sprintf(gamemodName, "Unknown Mode");
		}

		const int client = m_index - 1;
		char outputBuffer[512];
		sprintf(outputBuffer, "\n\n\n\n\n\n\n Game Mode: %s"
			"\n [%s] \n Process: %s  RE-Process: %s \n"
			"Weapon: %s  Clip: %d  Ammo: %d \n"
			"Type: %s  Money: %d  Total Path: %d \n"
			"Enemy: %s  Friend: %s \n\n"

			"Current Index: %d  Goal Index: %d  Enemy Reachable: %s \n"
			"Nav: %d  Next Nav: %d \n"
			//"GEWI: %d  GEWI2: %d \n"
			"Move Speed: %2.f  Strafe Speed: %2.f \n "
			"Stuck Warnings: %d  Stuck: %s \n",
			gamemodName,
			GetEntityName(GetEntity()), processName, rememberedProcessName,
			&weaponName[7], GetAmmoInClip(), m_ammo[g_weaponDefs[m_currentWeapon].ammo1],
			botType, m_moneyAmount, m_navNode.Length(),
			enemyName, friendName,

			m_currentWaypointIndex, goal, m_isEnemyReachable ? "Yes" : "No",
			m_navNode.IsEmpty() ? -1 : m_navNode.First(), m_navNode.HasNext() ? m_navNode.Next() : -1,
			//g_clients[client].wpIndex, g_clients[client].wpIndex2,
			m_moveSpeed, m_strafeSpeed,
			m_stuckWarn, m_isStuck ? "Yes" : "No");

		MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, nullptr, g_hostEntity);
		WRITE_BYTE(TE_TEXTMESSAGE);
		WRITE_BYTE(1);
		WRITE_SHORT(FixedSigned16(-1.0f, (1 << 13)));
		WRITE_SHORT(FixedSigned16(0.0f, (1 << 13)));
		WRITE_BYTE(0);
		WRITE_BYTE(m_team == Team::Counter ? 0 : 255);
		WRITE_BYTE(100);
		WRITE_BYTE(m_team != Team::Counter ? 0 : 255);
		WRITE_BYTE(0);
		WRITE_BYTE(255);
		WRITE_BYTE(255);
		WRITE_BYTE(255);
		WRITE_BYTE(0);
		WRITE_SHORT(FixedUnsigned16(0.0f, (1 << 8)));
		WRITE_SHORT(FixedUnsigned16(0.0f, (1 << 8)));
		WRITE_SHORT(FixedUnsigned16(1.0f, (1 << 8)));
		WRITE_STRING(const_cast<const char*>(&outputBuffer[0]));
		MESSAGE_END();

		timeDebugUpdate = engine->GetTime() + 1.0f;
	}

	if (m_hasEnemiesNear && m_enemyOrigin != nullvec)
		engine->DrawLine(g_hostEntity, EyePosition(), m_enemyOrigin, Color(255, 0, 0, 255), 10, 0, 5, 1, LINE_SIMPLE);

	int16_t i;
	for (i = 0; i < m_navNode.Length() && i + 1 < m_navNode.Length(); ++i)
		engine->DrawLine(g_hostEntity, g_waypoint->GetPath(m_navNode.Get(i))->origin, g_waypoint->GetPath(m_navNode.Get(i + 1))->origin, Color(255, 100, 55, 255), 15, 0, 8, 1, LINE_SIMPLE);

	if (IsValidWaypoint(m_currentWaypointIndex))
	{
		if (m_currentTravelFlags & PATHFLAG_JUMP)
			engine->DrawLine(g_hostEntity, pev->origin, m_waypoint.origin, Color(255, 0, 0, 255), 15, 0, 8, 1, LINE_SIMPLE);
		else if (m_currentTravelFlags & PATHFLAG_DOUBLE)
			engine->DrawLine(g_hostEntity, pev->origin, m_waypoint.origin, Color(0, 0, 255, 255), 15, 0, 8, 1, LINE_SIMPLE);
		else
			engine->DrawLine(g_hostEntity, pev->origin, m_waypoint.origin, Color(255, 100, 55, 255), 15, 0, 8, 1, LINE_SIMPLE);
	}
}

void Bot::ChatMessage(const int type, const bool isTeamSay)
{
	extern ConVar ebot_chat;
	if (!ebot_chat.GetBool() || g_chatFactory[type].IsEmpty())
		return;

	const char* pickedPhrase = g_chatFactory[type].GetRandomElement();
	if (IsNullString(pickedPhrase))
		return;

	PrepareChatMessage(const_cast<char*>(pickedPhrase));
	PushMessageQueue(isTeamSay ? CMENU_TEAMSAY : CMENU_SAY);
}

bool Bot::HasHostage(void)
{
	if (g_mapType != MAP_CS)
		return false;

	if (m_team != Team::Counter)
		return false;

	Vector origin;
	for (auto& hostage : m_hostages)
	{
		origin = GetEntityOrigin(hostage);
		if (origin == nullvec || hostage->v.effects & EF_NODRAW || hostage->v.health <= 0.0f || (pev->origin - origin).GetLengthSquared() > squaredf(600.0f))
		{
			hostage = nullptr;
			continue;
		}

		return true;
	}

	return false;
}

// this function gets called by network message handler, when screenfade message get's send
// it's used to make bot blind froumd the grenade
void Bot::TakeBlinded(const Vector& fade, const int alpha)
{
	if (fade.x != 255 || fade.y != 255 || fade.z != 255 || alpha <= 170)
		return;

	SetProcess(Process::Blind, "i'm blind", false, engine->GetTime() + crandomfloat(3.2f, 6.4f));
}

// this function, asks bot to discard his current primary weapon (or c4) to the user that requsted it with /drop*
// command, very useful, when i don't have money to buy anything... )
void Bot::DiscardWeaponForUser(edict_t* user, const bool discardC4)
{
	if (!m_buyingFinished)
		return;

	if (!IsAlive(user))
		return;

	if (m_moneyAmount > 2000 && HasPrimaryWeapon())
	{
		const Vector userOrigin = GetPlayerHeadOrigin(user);
		if ((userOrigin - pev->origin).GetLengthSquared() < squaredf(240.0f))
		{
			LookAt(userOrigin, user->v.velocity);

			if (discardC4)
			{
				SelectWeaponByName("weapon_c4");
				FakeClientCommand(GetEntity(), "drop");
			}
			else
			{
				SelectBestWeapon();
				FakeClientCommand(GetEntity(), "drop");
			}

			m_pickupItem = nullptr;
			m_pickupType = PickupType::None;
			m_itemCheckTime = engine->GetTime() + 5.0f;

			if (m_inBuyZone)
			{
				m_buyingFinished = false;
				m_buyState = 0;
				PushMessageQueue(CMENU_BUY);
				m_nextBuyTime = engine->GetTime();
			}
		}
	}

	char buffer[512];
	FormatBuffer(buffer, "Sorry %s, but i don't want discard my %s to you!", GetEntityName(user), discardC4 ? "bomb" : "weapon");
	ChatSay(false, buffer);
	RadioMessage(Radio::Negative);
}

void Bot::ResetDoubleJumpState(void)
{
	m_navNode.Clear();
	m_doubleJumpEntity = nullptr;
	m_doubleJumpOrigin = nullvec;
	m_jumpReady = false;
}

Vector Bot::CheckToss(const Vector& start, const Vector& stop)
{
	Vector end = stop - pev->velocity;
	end.z -= 15.0f;

	if (cabsf(end.z - start.z) > 500.0f)
		return nullvec;

	TraceResult tr{};
	Vector midPoint = start + (end - start) * 0.5f;
	TraceHull(midPoint, midPoint + Vector(0.0f, 0.0f, 500.0f), true, head_hull, pev->pContainingEntity, &tr);

	if (tr.flFraction < 1.0f && tr.pHit)
	{
		midPoint = tr.vecEndPos;
		midPoint.z = tr.pHit->v.absmin.z - 1.0f;
	}

	if (midPoint.z < start.z || midPoint.z < end.z)
		return nullvec;

	const float gravity = engine->GetGravity() * 0.55f;
	const float timeOne = csqrtf((midPoint.z - start.z) / (0.5f * gravity));
	if (timeOne < 0.1f)
		return nullvec;

	Vector velocity = (end - start) / (timeOne + csqrtf((midPoint.z - end.z) / (0.5f * gravity)));
	velocity.z = gravity * timeOne;

	Vector apex = start + velocity * timeOne;
	apex.z = midPoint.z;

	TraceHull(start, apex, false, head_hull, pev->pContainingEntity, &tr);

	if (tr.flFraction < 1.0f || tr.fAllSolid)
		return nullvec;

	TraceHull(end, apex, true, head_hull, pev->pContainingEntity, &tr);

	if (tr.flFraction != 1.0f)
	{
		if (-(tr.vecPlaneNormal | (apex - end).Normalize()) > 0.75f || tr.flFraction < 0.8f)
			return nullvec;
	}

	return velocity * 0.777f;
}

Vector Bot::CheckThrow(const Vector& start, const Vector& end)
{
	Vector velocity = end - start;
	float time = velocity.GetLength() * 0.00512820512f;
	if (time < 0.01f)
		return nullvec;

	if (time > 2.0f)
		time = 1.2f;

	const float gravity = engine->GetGravity() * 0.55f;
	const float half = time * 0.5f;

	velocity = velocity * (1.0f / time);
	velocity.z += gravity * half * half;

	Vector apex = start + (end - start) * 0.5f;
	apex.z += 0.5f * gravity * half;

	TraceResult tr{};
	TraceHull(start, apex, false, head_hull, pev->pContainingEntity, &tr);

	if (tr.flFraction != 1.0f)
		return nullvec;

	TraceHull(end, apex, true, head_hull, pev->pContainingEntity, &tr);

	if (tr.flFraction != 1.0f || tr.fAllSolid)
	{
		const float dot = -(tr.vecPlaneNormal | (apex - end).Normalize());
		if (dot > 0.75f || tr.flFraction < 0.8f)
			return nullvec;
	}

	return velocity * 0.7793f;
}

// this function checks burst mode, and switch it depending distance to to enemy
void Bot::CheckBurstMode(const float distance)
{
	if (HasShield())
		return; // no checking when shiled is active

	// if current weapon is glock, disable burstmode on long distances, enable it else
	if (m_currentWeapon == Weapon::Glock18 && distance < 300.0f && m_weaponBurstMode == BurstMode::Disabled)
		pev->buttons |= IN_ATTACK2;
	else if (m_currentWeapon == Weapon::Glock18 && distance >= 30.0f && m_weaponBurstMode == BurstMode::Enabled)
		pev->buttons |= IN_ATTACK2;

	// if current weapon is famas, disable burstmode on short distances, enable it else
	if (m_currentWeapon == Weapon::Famas && distance > 400.0f && m_weaponBurstMode == BurstMode::Disabled)
		pev->buttons |= IN_ATTACK2;
	else if (m_currentWeapon == Weapon::Famas && distance <= 400.0f && m_weaponBurstMode == BurstMode::Enabled)
		pev->buttons |= IN_ATTACK2;
}

void Bot::CheckSilencer(void)
{
	if ((m_currentWeapon == Weapon::Usp && m_skill < 90) || m_currentWeapon == Weapon::M4A1 && !HasShield())
	{
		const int random = (m_personality == Personality::Rusher ? 33 : m_personality == Personality::Careful ? 99 : 66);
		if (chanceof(m_currentWeapon == Weapon::Usp ? random / 3 : random))
		{
			if (pev->weaponanim > 6) // is the silencer not attached...
				pev->buttons |= IN_ATTACK2; // attach the silencer
		}
		else if (pev->weaponanim <= 6) // is the silencer attached...
			pev->buttons |= IN_ATTACK2; // detach the silencer
	}
}

float Bot::GetBombTimeleft(void)
{
	if (!g_bombPlanted)
		return 0.0f;

	const float timeLeft = ((g_timeBombPlanted + engine->GetC4TimerTime()) - engine->GetTime());
	if (timeLeft < 0.0f)
		return 0.0f;

	return timeLeft;
}

float Bot::GetEstimatedReachTime(void)
{
	// calculate 'real' time that we need to get from one waypoint to another
	if (IsValidWaypoint(m_currentWaypointIndex) && IsValidWaypoint(m_prevWptIndex[0]))
	{
		// caclulate estimated time
		float estimatedTime = 5.0f * ((g_waypoint->GetPath(m_prevWptIndex[0])->origin - m_destOrigin).GetLengthSquared() / squaredf(pev->maxspeed));

		// check for special waypoints, that can slowdown our movement
		if ((m_waypoint.flags & WAYPOINT_CROUCH) || (m_waypoint.flags & WAYPOINT_LADDER) || (pev->buttons & IN_DUCK))
			estimatedTime *= 3.0f;

		// check for too low values
		if (estimatedTime < 3.0f)
			estimatedTime = 3.0f;

		// check for too high values
		else if (estimatedTime > 8.0f)
			estimatedTime = 8.0f;
	}

	return 6.0f;
}

bool Bot::CampingAllowed(void)
{
	if (IsZombieMode())
	{
		if (m_isZombieBot || g_waypoint->m_zmHmPoints.IsEmpty())
			return false;

		return true;
	}
	else if (g_waypoint->m_campPoints.IsEmpty())
		return false;

	if (GetGameMode() == GameMode::Original)
	{
		if (m_isBomber || m_isVIP)
			return false;

		if (g_mapType != MAP_DE)
		{
			// never camp on 1v1
			if (m_numEnemiesLeft < 2 || !m_numFriendsLeft)
				return false;
		}
		else if (OutOfBombTimer())
			return false;

		if (m_team == Team::Counter)
		{
			if (g_mapType == MAP_CS)
			{
				if (g_timeRoundMid < engine->GetTime() || HasHostage())
					return false;
			}
			else if (g_bombPlanted)
			{
				const Vector bomb = g_waypoint->GetBombPosition();
				if (bomb != nullvec && !IsBombDefusing(bomb))
					return false;
			}
		}
		else
		{
			if (g_mapType == MAP_DE)
			{
				if (g_bombPlanted)
				{
					const Vector bomb = g_waypoint->GetBombPosition();

					// where is the bomb?
					if (bomb == nullvec || IsBombDefusing(bomb))
						return false;

					return true;
				}
				else
				{
					if (m_isSlowThink)
						m_loosedBombWptIndex = FindLoosedBomb();

					if (IsValidWaypoint(m_loosedBombWptIndex))
						return false;
				}
			}
		}

		if (m_personality == Personality::Rusher)
		{
			if (m_numFriendsLeft < m_numEnemiesLeft && !FNullEnt(m_nearestEnemy))
				return false;
			else if (m_numFriendsLeft > m_numEnemiesLeft && FNullEnt(m_nearestEnemy))
				return false;
		}
	}

	return true;
}

bool Bot::OutOfBombTimer(void)
{
	if (g_mapType != MAP_DE)
		return false;

	if (!g_bombPlanted)
		return false;

	if (GetCurrentState() == Process::Defuse || m_hasProgressBar || GetCurrentState() == Process::Escape)
		return false; // if CT bot already start defusing, or already escaping, return false

	// calculate left time
	const float timeLeft = GetBombTimeleft();

	// if time left greater than 12, no need to do other checks
	if (timeLeft > 12.0f)
		return false;
	else if (m_team == Team::Terrorist)
		return true;

	const Vector& bombOrigin = g_waypoint->GetBombPosition();
	if (bombOrigin == nullvec)
		return true;

	// bot will belive still had a chance
	if ((m_hasDefuser && IsVisible(bombOrigin, GetEntity())) || (bombOrigin - pev->origin).GetLengthSquared() < squaredf(512.0f))
		return false;

	bool hasTeammatesWithDefuserKit = false;
	// check if our teammates has defusal kit
	if (m_numFriendsLeft)
	{
		for (const auto& bot : g_botManager->m_bots)
		{
			// search players with defuse kit
			if (bot && bot->m_team == Team::Counter && bot->m_hasDefuser && (bombOrigin - bot->pev->origin).GetLengthSquared() < squaredf(512.0f))
			{
				hasTeammatesWithDefuserKit = true;
				break;
			}
		}
	}

	// add reach time to left time
	const float reachTime = g_waypoint->GetTravelTime(pev->maxspeed, m_waypoint.origin, bombOrigin);

	// for counter-terrorist check alos is we have time to reach position plus average defuse time
	if ((timeLeft < reachTime && !m_hasDefuser && !hasTeammatesWithDefuserKit) || (timeLeft < reachTime && m_hasDefuser))
		return true;

	if ((m_hasDefuser ? 6.0f : 12.0f) + GetEstimatedReachTime() < GetBombTimeleft())
		return true;

	return false; // return false otherwise
}

bool Bot::IsBombDefusing(const Vector& bombOrigin)
{
	// this function finds if somebody currently defusing the bomb.
	if (!g_bombPlanted)
		return false;

	// easier way
	if (g_bombDefusing)
		return true;

	Bot* bot;
	bool defusingInProgress = false;
	const float distanceToBomb = squaredf(140.0f);

	for (const auto& client : g_clients)
	{
		if (FNullEnt(client.ent))
			continue;

		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE))
			continue;

		bot = g_botManager->GetBot(client.index);
		if (bot)
		{
			if (m_team != bot->m_team || bot->GetCurrentState() == Process::Escape || bot->GetCurrentState() == Process::Camp)
				continue; // skip other mess

			// if close enough, mark as progressing
			if ((client.ent->v.origin - bombOrigin).GetLengthSquared() < distanceToBomb && (bot->GetCurrentState() == Process::Defuse || bot->m_hasProgressBar))
			{
				defusingInProgress = true;
				break;
			}

			continue;
		}

		// take in account peoples too
		if (client.team == m_team)
		{
			// if close enough, mark as progressing
			if ((client.ent->v.origin - bombOrigin).GetLengthSquared() < distanceToBomb && ((client.ent->v.buttons | client.ent->v.oldbuttons) & IN_USE))
			{
				defusingInProgress = true;
				break;
			}

			continue;
		}
	}

	return defusingInProgress;
}

int Bot::GetWeaponID(const char* weapon)
{
	int id = Weapon::KevlarHelmet;
	if (cstrcmp(weapon, "p228"))
		id = Weapon::P228;
	else if (cstrcmp(weapon, "shield"))
		id = Weapon::Shield;
	else if (cstrcmp(weapon, "scout"))
		id = Weapon::Scout;
	else if (cstrcmp(weapon, "hegren"))
		id = Weapon::HeGrenade;
	else if (cstrcmp(weapon, "xm1014"))
		id = Weapon::Xm1014;
	else if (cstrcmp(weapon, "mac10"))
		id = Weapon::Mac10;
	else if (cstrcmp(weapon, "aug"))
		id = Weapon::Aug;
	else if (cstrcmp(weapon, "sgren"))
		id = Weapon::SmGrenade;
	else if (cstrcmp(weapon, "elites"))
		id = Weapon::Elite;
	else if (cstrcmp(weapon, "fiveseven"))
		id = Weapon::FiveSeven;
	else if (cstrcmp(weapon, "ump45"))
		id = Weapon::Ump45;
	else if (cstrcmp(weapon, "sg550"))
		id = Weapon::Sg550;
	else if (cstrcmp(weapon, "galil"))
		id = Weapon::Galil;
	else if (cstrcmp(weapon, "famas"))
		id = Weapon::Famas;
	else if (cstrcmp(weapon, "usp"))
		id = Weapon::Usp;
	else if (cstrcmp(weapon, "glock18"))
		id = Weapon::Glock18;
	else if (cstrcmp(weapon, "glock"))
		id = Weapon::Glock18;
	else if (cstrcmp(weapon, "awp"))
		id = Weapon::Awp;
	else if (cstrcmp(weapon, "mp5"))
		id = Weapon::Mp5;
	else if (cstrcmp(weapon, "m249"))
		id = Weapon::M249;
	else if (cstrcmp(weapon, "m3"))
		id = Weapon::M3;
	else if (cstrcmp(weapon, "m4a1"))
		id = Weapon::M4A1;
	else if (cstrcmp(weapon, "tmp"))
		id = Weapon::Tmp;
	else if (cstrcmp(weapon, "g3sg1"))
		id = Weapon::G3SG1;
	else if (cstrcmp(weapon, "flash"))
		id = Weapon::FbGrenade;
	else if (cstrcmp(weapon, "flashbang"))
		id = Weapon::FbGrenade;
	else if (cstrcmp(weapon, "deagle"))
		id = Weapon::Deagle;
	else if (cstrcmp(weapon, "sg552"))
		id = Weapon::Sg552;
	else if (cstrcmp(weapon, "ak47"))
		id = Weapon::Ak47;
	else if (cstrcmp(weapon, "p90"))
		id = Weapon::P90;
	else if (cstrcmp(weapon, "vesthelmet"))
		id = Weapon::KevlarHelmet;
	else if (cstrcmp(weapon, "vesthelm"))
		id = Weapon::KevlarHelmet;
	else if (cstrcmp(weapon, "vest"))
		id = Weapon::Kevlar;
	else if (cstrcmp(weapon, "defuser"))
		id = Weapon::Defuser;

	return id;
}
