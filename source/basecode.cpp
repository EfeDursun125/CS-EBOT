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
ConVar ebot_use_radio("ebot_use_radio", "1");
ConVar ebot_force_flashlight("ebot_force_flashlight", "0");
ConVar ebot_use_flare("ebot_zm_use_flares", "1");
ConVar ebot_chat_percent("ebot_chat_percent", "20");
ConVar ebot_eco_rounds("ebot_eco_rounds", "1");

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
			if (bot != nullptr && bot.get() != this)
			{
				if (m_isAlive == bot->m_isAlive)
				{
					bot->m_sayTextBuffer.entityIndex = GetIndex();
					cstrcpy(bot->m_sayTextBuffer.sayText, m_tempStrings);
				}
			}
		}
	}
	else if (message == CMENU_BUY && g_gameVersion & Game::HalfLife)
	{
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
	const float entityAngle = AngleMod(destination.ToYaw()); // find yaw angle from source to destination...
	const float viewAngle = AngleMod(pev->v_angle.y); // get bot's current view angle...

	// return the absolute value of angle to destination entity
	// zero degrees means straight ahead, 45 degrees to the left or
	// 45 degrees to the right is the limit of the normal view angle
	const float absoluteAngle = cabsf(viewAngle - entityAngle);

	if (absoluteAngle > 180.0f)
		return 360.0f - absoluteAngle;

	return absoluteAngle;
}

bool Bot::IsInViewCone(const Vector& origin)
{
	return ::IsInViewCone(origin, GetEntity());
}

bool Bot::CheckVisibility(edict_t* targetEntity)
{
	m_visibility = Visibility::None;
	if (FNullEnt(targetEntity))
		return false;

	TraceResult tr{};
	const Vector eyes = EyePosition();

	Vector spot = targetEntity->v.origin;
	edict_t* self = GetEntity();

	constexpr float vis = 0.96f;
	bool ignoreGlass = true;

	// zombies can't hit from the glass...
	if (m_isZombieBot)
		ignoreGlass = false;

	TraceLine(eyes, spot, true, ignoreGlass, self, &tr);

	if (tr.flFraction > vis || tr.pHit == targetEntity)
	{
		m_visibility |= Visibility::Body;
		m_enemyOrigin = spot;
	}

	// check top of head
	spot.z += 25.0f;
	TraceLine(eyes, spot, true, ignoreGlass, self, &tr);

	if (tr.flFraction > vis || tr.pHit == targetEntity)
	{
		m_visibility |= Visibility::Head;
		m_enemyOrigin = spot;
	}

	if (m_visibility != Visibility::None)
		return true;

	constexpr auto standFeet = 34.0f;
	constexpr auto crouchFeet = 14.0f;

	if ((targetEntity->v.flags & FL_DUCKING))
		spot.z = targetEntity->v.origin.z - crouchFeet;
	else
		spot.z = targetEntity->v.origin.z - standFeet;

	TraceLine(eyes, spot, true, ignoreGlass, self, &tr);

	if (tr.flFraction > vis || tr.pHit == targetEntity)
	{
		m_visibility |= Visibility::Other;
		m_enemyOrigin = spot;
		return true;
	}

	constexpr float edgeOffset = 13.0f;
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

bool Bot::ItemIsVisible(Vector destination, char* itemName)
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

bool Bot::EntityIsVisible(Vector dest, const bool fromBody)
{
	TraceResult tr{};

	// trace a line from bot's eyes to destination...
	TraceLine(fromBody ? pev->origin - Vector(0.0f, 0.0f, 1.0f) : EyePosition(), dest, true, true, GetEntity(), &tr);

	// check if line of sight to object is not blocked (i.e. visible)
	return tr.flFraction >= 1.0f;
}

bool Bot::IsBehindSmokeClouds(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	edict_t* pentGrenade = nullptr;
	while (!FNullEnt(pentGrenade = FIND_ENTITY_BY_CLASSNAME(pentGrenade, "grenade")))
	{
		// if grenade is invisible don't care for it
		if ((pentGrenade->v.effects & EF_NODRAW) || !(pentGrenade->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)) || cstrcmp(STRING(pentGrenade->v.model) + 9, "smokegrenade.mdl"))
			continue;

		const Vector entOrigin = GetEntityOrigin(ent);

		// check if visible to the bot
		if (InFieldOfView(entOrigin - EyePosition()) > pev->fov * 0.33333333333f && !EntityIsVisible(entOrigin))
			continue;

		const Vector pentOrigin = GetEntityOrigin(pentGrenade);
		const Vector betweenUs = (entOrigin - pev->origin).Normalize();
		const Vector betweenNade = (pentOrigin - pev->origin).Normalize();
		const Vector betweenResult = ((Vector(betweenNade.y, betweenNade.x, 0.0f) * 150.0f + pentOrigin) - pev->origin).Normalize();

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

	for (int i = 0; i < Const_NumWeapons; i++)
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

	for (int i = 0; i < Const_NumWeapons; i++)
	{
		const int id = weaponTab[*ptr].id;
		if ((weapons & (1 << static_cast<int>(weaponTab[*ptr].id))) && (id == Weapon::Usp || id == Weapon::Glock18 || id == Weapon::Deagle || id == Weapon::P228 || id == Weapon::Elite || id == Weapon::FiveSeven))
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

	for (int i = 0; i < Const_NumWeapons; i++)
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
	float nearestDistance = FLT_MAX;
	edict_t* searchEntity = nullptr, * foundEntity = nullptr;

	// find the nearest button which can open our target
	while (!FNullEnt(searchEntity = FIND_ENTITY_IN_SPHERE(searchEntity, pev->origin, 512.0f)))
	{
		if (cstrncmp("func_button", STRING(searchEntity->v.classname), 11) == 0 || cstrncmp("func_rot_button", STRING(searchEntity->v.classname), 15) == 0)
		{
			const float distance = (pev->origin - GetEntityOrigin(searchEntity)).GetLengthSquared();
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

	if (IsZombieMode() && HasPrimaryWeapon() && m_currentWeapon != Weapon::Knife)
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

	PickupType pickupType = PickupType::None;

	float minDistance = SquaredF(512.0f);

	while (!FNullEnt(ent = FIND_ENTITY_IN_SPHERE(ent, pev->origin, 512.0f)))
	{
		pickupType = PickupType::None;
		if (ent->v.effects & EF_NODRAW || ent == m_itemIgnore || IsValidPlayer(ent) || ent == GetEntity()) // we can't pickup a player...
			continue;

		if (pev->health < pev->max_health && cstrncmp("item_healthkit", STRING(ent->v.classname), 14) == 0)
			pickupType = PickupType::GetEntity;
		else if (pev->health < pev->max_health && cstrncmp("func_healthcharger", STRING(ent->v.classname), 18) == 0 && ent->v.frame == 0)
		{
			const auto origin = GetEntityOrigin(ent);
			if ((pev->origin - origin).GetLengthSquared() <= SquaredF(128.0f))
			{
				if (g_gameVersion & Game::Xash)
					pev->button |= IN_USE;
				else
					MDLL_Use(ent, GetEntity());
			}

			m_lookAt = origin;
			pickupType = PickupType::GetEntity;
			m_moveSpeed = pev->maxspeed;
			m_strafeSpeed = 0.0f;
		}
		else if (pev->armorvalue < 100 && cstrncmp("item_battery", STRING(ent->v.classname), 12) == 0)
			pickupType = PickupType::GetEntity;
		else if (pev->armorvalue < 100 && cstrncmp("func_recharge", STRING(ent->v.classname), 13) == 0 && ent->v.frame == 0)
		{
			const auto origin = GetEntityOrigin(ent);
			if ((pev->origin - origin).GetLengthSquared() <= SquaredF(128.0f))
			{
				if (g_gameVersion & Game::Xash)
					pev->button |= IN_USE;
				else
					MDLL_Use(ent, GetEntity());
			}

			m_lookAt = origin;
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
				m_lookAt = GetEntityOrigin(ent);
				m_moveSpeed = -pev->maxspeed;
				m_strafeSpeed = 0.0f;

				if (!(pev->oldbuttons & IN_ATTACK))
					pev->button |= IN_ATTACK;
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
			for (int i = 0; (i < entityNum && pickupType == PickupType::None); i++)
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

		const Vector entityOrigin = GetEntityOrigin(ent);
		const float distance = (pev->origin - entityOrigin).GetLengthSquared();
		if (distance > minDistance)
			continue;

		if (!ItemIsVisible(entityOrigin, const_cast<char*>(STRING(ent->v.classname))))
		{
			if (cstrncmp("grenade", STRING(ent->v.classname), 7) != 0 || cstrcmp(STRING(ent->v.model) + 9, "c4.mdl") != 0)
				continue;

			if (distance > SquaredF(128.0f))
				continue;
		}

		bool allowPickup = true;
		if (pickupType == PickupType::GetEntity)
			allowPickup = true;
		else if (pickupType == PickupType::Weapon)
		{
			const int weaponCarried = GetBestWeaponCarried();
			const int secondaryWeaponCarried = GetBestSecondaryWeaponCarried();
			const int secondaryWeaponAmmoMax = g_weaponDefs[g_weaponSelect[secondaryWeaponCarried].id].ammo1Max;
			const int weaponAmmoMax = g_weaponDefs[g_weaponSelect[weaponCarried].id].ammo1Max;

			if (secondaryWeaponCarried < 7 && (m_ammo[g_weaponSelect[secondaryWeaponCarried].id] > 0.3 * secondaryWeaponAmmoMax) && cstrcmp(STRING(ent->v.model) + 9, "w_357ammobox.mdl") == 0)
				allowPickup = false;
			else if (!m_isVIP && weaponCarried >= 7 && (m_ammo[g_weaponSelect[weaponCarried].id] > 0.3 * weaponAmmoMax) && cstrncmp(STRING(ent->v.model) + 9, "w_", 2) == 0)
			{
				if (cstrcmp(STRING(ent->v.model) + 9, "w_9mmarclip.mdl") == 0 && !(weaponCarried == Weapon::Famas || weaponCarried == Weapon::Ak47 || weaponCarried == Weapon::M4A1 || weaponCarried == Weapon::Galil || weaponCarried == Weapon::Aug || weaponCarried == Weapon::Sg552))
					allowPickup = false;
				else if (cstrcmp(STRING(ent->v.model) + 9, "w_shotbox.mdl") == 0 && !(weaponCarried == Weapon::M3 || weaponCarried == Weapon::Xm1014))
					allowPickup = false;
				else if (cstrcmp(STRING(ent->v.model) + 9, "w_9mmclip.mdl") == 0 && !(weaponCarried == Weapon::Mp5 || weaponCarried == Weapon::Tmp || weaponCarried == Weapon::P90 || weaponCarried == Weapon::Mac10 || weaponCarried == Weapon::Ump45))
					allowPickup = false;
				else if (cstrcmp(STRING(ent->v.model) + 9, "w_crossbow_clip.mdl") == 0 && !(weaponCarried == Weapon::Awp || weaponCarried == Weapon::G3SG1 || weaponCarried == Weapon::Scout || weaponCarried == Weapon::Sg550))
					allowPickup = false;
				else if (cstrcmp(STRING(ent->v.model) + 9, "w_chainammo.mdl") == 0 && weaponCarried == Weapon::M249)
					allowPickup = false;
			}
			else if (m_isVIP || !RateGroundWeapon(ent))
				allowPickup = false;
			else if (cstrcmp(STRING(ent->v.model) + 9, "medkit.mdl") == 0 && (pev->health >= 100))
				allowPickup = false;
			else if ((cstrcmp(STRING(ent->v.model) + 9, "kevlar.mdl") == 0 || cstrcmp(STRING(ent->v.model) + 9, "battery.mdl") == 0) && pev->armorvalue >= 100) // armor vest
				allowPickup = false;
			else if (cstrcmp(STRING(ent->v.model) + 9, "flashbang.mdl") == 0 && (pev->weapons & (1 << Weapon::FbGrenade))) // concussion grenade
				allowPickup = false;
			else if (cstrcmp(STRING(ent->v.model) + 9, "hegrenade.mdl") == 0 && (pev->weapons & (1 << Weapon::HeGrenade))) // explosive grenade
				allowPickup = false;
			else if (cstrcmp(STRING(ent->v.model) + 9, "smokegrenade.mdl") == 0 && (pev->weapons & (1 << Weapon::SmGrenade))) // smoke grenade
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

				const int index = FindDefendWaypoint(entityOrigin);
				if (IsValidWaypoint(index))
				{
					const float timeMidBlowup = g_timeBombPlanted + ((engine->GetC4TimerTime() * 0.5f) + engine->GetC4TimerTime() * 0.25f) - g_waypoint->GetTravelTime(m_moveSpeed, pev->origin, g_waypoint->GetPath(index)->origin);

					if (timeMidBlowup > engine->GetTime())
					{
						m_campIndex = index;
						SetProcess(Process::Camp, "i will defend the bomb", true, AddTime(timeMidBlowup));
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
				if (FNullEnt(ent) || ent->v.health <= 0 || ent->v.speed > 1.0f)
					allowPickup = false; // never pickup dead/moving hostages
				else
				{
					for (const auto& bot : g_botManager->m_bots)
					{
						if (bot != nullptr && bot->m_isAlive)
						{
							for (const auto& hostage : bot->m_hostages)
							{
								if (hostage == ent)
								{
									allowPickup = false;
									break;
								}
							}
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
					const int index = FindDefendWaypoint(entityOrigin);
					if (IsValidWaypoint(index))
					{
						m_campIndex = index;
						SetProcess(Process::Camp, "i will defend the dropped bomb", true, AddTime(ebot_camp_max.GetFloat()));
					}
				}
			}
			else if (pickupType == PickupType::PlantedC4)
			{
				if (OutOfBombTimer())
				{
					allowPickup = false;
					return;
				}

				allowPickup = !IsBombDefusing(g_waypoint->GetBombPosition());
				if (!allowPickup)
				{
					const int index = FindDefendWaypoint(entityOrigin);
					if (IsValidWaypoint(index))
					{
						m_campIndex = index;
						SetProcess(Process::Camp, "i will protect my teammate", true, AddTime(ebot_camp_max.GetFloat()));
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
		if (!IsDeathmatchMode())
		{
			for (const auto& bot : g_botManager->m_bots)
			{
				if (bot != nullptr && bot.get() != this && bot->m_isAlive && bot->m_pickupItem == pickupItem && bot->m_team == m_team)
				{
					m_pickupItem = nullptr;
					m_pickupType = PickupType::None;
					return;
				}
			}
		}

		const Vector pickupOrigin = GetEntityOrigin(pickupItem);
		if (pickupOrigin == nullvec || pickupOrigin.z > EyePosition().z + 12.0f || IsDeadlyDrop(pickupOrigin))
		{
			m_pickupItem = nullptr;
			m_pickupType = PickupType::None;
			return;
		}

		m_pickupItem = pickupItem;
	}
}

// this function inserts the radio message into the message queue
void Bot::RadioMessage(const int message)
{
	if (g_gameVersion & Game::HalfLife)
		return;

	if (ebot_use_radio.GetInt() <= 0)
		return;

	if (m_radiotimer > engine->GetTime())
		return;

	if (m_numFriendsLeft <= 0)
		return;

	if (m_numEnemiesLeft <= 0)
		return;

	if (GetGameMode() == GameMode::Deathmatch)
		return;

	// stop spamming
	if (m_radioEntity == GetEntity())
		return;

	m_radioSelect = message;
	PushMessageQueue(CMENU_RADIO);
	m_radiotimer = AddTime(CRandomFloat(m_numFriendsLeft, m_numFriendsLeft * 1.5f));
}

// this function checks and executes pending messages
void Bot::CheckMessageQueue(void)
{
	if (m_actMessageIndex == m_pushMessageIndex)
		return;

	// get message from stack
	const auto state = GetMessageQueue();

	// nothing to do?
	if (state == CMENU_IDLE || (state == CMENU_RADIO && (GetGameMode() == GameMode::Deathmatch || !IsCStrike())))
		return;

	switch (state)
	{
	case CMENU_BUY: // general buy message
		// buy weapon
		if (m_nextBuyTime > engine->GetTime())
		{
			// keep sending message
			PushMessageQueue(CMENU_BUY);
			return;
		}

		if (!m_inBuyZone)
		{
			m_buyState = 7;
			m_buyingFinished = true;

			if (ChanceOf(m_skill) && !m_isBomber && !m_isVIP)
				SelectWeaponByName("weapon_knife");

			break;
		}

		// if freezetime is very low do not delay the buy process
		if (engine->GetFreezeTime() < 2.0f)
			m_nextBuyTime = AddTime(CRandomFloat(0.2f, 0.4f));
		else
			m_nextBuyTime = AddTime(CRandomFloat(0.6f, 1.2f));

		// if fun-mode no need to buy
		if (ebot_knifemode.GetBool() && (ebot_eco_rounds.GetBool() && HasPrimaryWeapon()))
		{
			m_buyState = 6;
			if (ChanceOf(m_skill))
				SelectKnife();
		}

		// prevent vip from buying
		if (g_mapType & MAP_AS)
		{
			if (*(INFOKEY_VALUE(GET_INFOKEYBUFFER(GetEntity()), "model")) == 'v')
			{
				m_isVIP = true;
				m_buyState = 7;
			}
			else
				m_isVIP = false;
		}
		else
			m_isVIP = false;

		// prevent terrorists from buying on es maps
		if ((g_mapType & MAP_ES) && m_team == Team::Terrorist)
			m_buyState = 76;

		// prevent teams from buying on fun maps
		if (g_mapType & (MAP_KA | MAP_FY | MAP_AWP))
		{
			m_buyState = 7;

			if (g_mapType & MAP_KA)
				ebot_knifemode.SetInt(1);
		}

		if (m_buyState > 6)
		{
			m_buyingFinished = true;

			if (ChanceOf(m_skill) && !m_isBomber && !m_isVIP)
				SelectWeaponByName("weapon_knife");

			return;
		}

		PushMessageQueue(CMENU_IDLE);
		PerformWeaponPurchase();

		break;

	case CMENU_RADIO:
		if (!m_isAlive)
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
						if (bot != nullptr && bot.get() != this && bot->m_team == m_team)
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

		// team independent saytext
	case CMENU_SAY:
		ChatSay(false, m_tempStrings);
		break;

		// team dependent saytext
	case CMENU_TEAMSAY:
		ChatSay(true, m_tempStrings);
		break;

	default:
		return;
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
		const char* banned = STRING(GetWeaponReturn(true, nullptr, weaponIndex));

		// check is this weapon is banned
		if (cstrncmp(bannedWeapons[i], banned, bannedWeapons[i].GetLength()) == 0)
			return true;
	}

	return false;
}

// this function determines currently owned primary weapon, and checks if bot has
// enough money to buy more powerful weapon
bool Bot::IsMorePowerfulWeaponCanBeBought(void)
{
	// if bot is not rich enough or non-standard weapon mode enabled return false
	if (m_moneyAmount < 4000 || IsNullString(ebot_restrictweapons.GetString()))
		return false;

	// also check if bot has really bad weapon, maybe it's time to change it
	if (UsesBadPrimary())
		return true;

	Array <String> bannedWeapons = String(ebot_restrictweapons.GetString()).Split(';');

	// check if its banned
	ITERATE_ARRAY(bannedWeapons, i)
	{
		if (m_currentWeapon == GetWeaponReturn(false, bannedWeapons[i]))
			return true;
	}

	if (m_currentWeapon == Weapon::Scout && m_moneyAmount > 5000)
		return true;
	else if (m_currentWeapon == Weapon::Mp5 && m_moneyAmount > 6000)
		return true;
	else if (m_currentWeapon == Weapon::M3 || m_currentWeapon == Weapon::Xm1014 && m_moneyAmount > 4000)
		return true;

	return false;
}

void Bot::PerformWeaponPurchase(void)
{
	m_nextBuyTime = engine->GetTime();
	WeaponSelect* selectedWeapon = nullptr;

	switch (m_buyState)
	{
	case 0:
		if (!m_favoritePrimary.IsEmpty() && !HasPrimaryWeapon() && !HasShield())
		{
			for (int i = 0; i < m_favoritePrimary.GetElementNumber(); i++)
			{
				if (HasPrimaryWeapon())
				{
					FakeClientCommand(GetEntity(), "primammo");
					break;
				}

				if (HasShield())
					break;

				if (cstrlen(m_favoritePrimary.GetAt(i)) < 1)
					continue;

				const int id = cmin(GetWeaponID(m_favoritePrimary.GetAt(i)), 32);
				if (IsRestricted(id))
					continue;

				if (pev->weapons & (1 << id))
					continue;

				FakeClientCommand(GetEntity(), "%s", (char*)m_favoritePrimary.GetAt(i));
			}
		}

		break;

	case 1:
		if ((g_botManager->EconomicsValid(m_team) || HasPrimaryWeapon()) && pev->armorvalue < CRandomFloat(40.0f, 80.0f))
		{
			if (m_moneyAmount > 1600 && !IsRestricted(Weapon::KevlarHelmet))
				FakeClientCommand(GetEntity(), "buyequip;menuselect 2");
			else if (m_moneyAmount > 800 && !IsRestricted(Weapon::Kevlar))
				FakeClientCommand(GetEntity(), "buyequip;menuselect 1");
		}
		break;

	case 2:
		if (!m_favoriteSecondary.IsEmpty() && !HasSecondaryWeapon() && (HasPrimaryWeapon() || HasShield()))
		{
			for (int i = 0; i < m_favoriteSecondary.GetElementNumber(); i++)
			{
				if (HasSecondaryWeapon())
				{
					FakeClientCommand(GetEntity(), "secammo");
					break;
				}

				if (cstrlen(m_favoriteSecondary.GetAt(i)) < 1)
					continue;

				const int id = cmin(GetWeaponID(m_favoriteSecondary.GetAt(i)), 32);
				if (IsRestricted(id))
					continue;

				if (pev->weapons & (1 << id))
					continue;

				FakeClientCommand(GetEntity(), "%s", (char*)m_favoriteSecondary.GetAt(i));
			}
		}

		break;

	case 3:
		if (!HasPrimaryWeapon() && !IsRestricted(Weapon::Shield) && !ChanceOf(m_skill))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 8");
		}

		if (CRandomInt(1, 2) == 1)
		{
			FakeClientCommand(GetEntity(), "buy;menuselect 1");

			if (CRandomInt(1, 2) == 1)
				FakeClientCommand(GetEntity(), "menuselect 4");
			else
				FakeClientCommand(GetEntity(), "menuselect 5");
		}

		break;

	case 4:
		if (ChanceOf(m_skill / 2) && !IsRestricted(Weapon::HeGrenade))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 4");
		}

		if (ChanceOf(m_skill / 2) && !IsRestricted(Weapon::FbGrenade))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 3");
		}

		if (ChanceOf(m_skill / 2) && !IsRestricted(Weapon::FbGrenade))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 3");
		}

		if (ChanceOf(m_skill / 2) && !IsRestricted(Weapon::SmGrenade))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 5");
		}

		break;

	case 5:
		if ((g_mapType & MAP_DE) && m_team == Team::Counter && ChanceOf(m_skill) && m_moneyAmount > 800 && !IsRestricted(Weapon::Defuser))
		{
			if (g_gameVersion & Game::Xash)
				FakeClientCommand(GetEntity(), "buyequip;menuselect 6");
			else
				FakeClientCommand(GetEntity(), "defuser"); // use alias in SteamCS
		}

		break;

	case 6:
		for (int i = 0; i <= 5; i++)
			FakeClientCommand(GetEntity(), "buyammo%d", CRandomInt(1, 2)); // simulate human

		if (ChanceOf(m_skill))
			FakeClientCommand(GetEntity(), "buy;menuselect 7");
		else
			FakeClientCommand(GetEntity(), "buy;menuselect 6");

		if (m_reloadState != ReloadState::Primary)
			m_reloadState = ReloadState::Secondary;

		if (!m_favoriteStuff.IsEmpty())
		{
			for (int i = 0; i < m_favoriteStuff.GetElementNumber(); i++)
			{
				if (cstrlen(m_favoriteStuff.GetAt(i)) < 1)
					continue;

				if (IsRestricted(GetWeaponID(m_favoriteStuff.GetAt(i))))
					continue;

				FakeClientCommand(GetEntity(), "%s", (char*)m_favoriteStuff.GetAt(i));
			}
		}

		break;

	}

	m_buyState++;
	PushMessageQueue(CMENU_BUY);
}

int Bot::BuyWeaponMode(const int weaponId)
{
	int gunMode = 10;
	switch (weaponId)
	{
	case Weapon::Shield:
		gunMode = 8;
		break;

	case Weapon::Tmp:
	case Weapon::Ump45:
	case Weapon::P90:
	case Weapon::Mac10:
	case Weapon::Scout:
	case Weapon::M3:
	case Weapon::M249:
	case Weapon::FiveSeven:
	case Weapon::P228:
		gunMode = 5;
		break;

	case Weapon::Xm1014:
	case Weapon::G3SG1:
	case Weapon::Sg550:
	case Weapon::Galil:
	case Weapon::Elite:
	case Weapon::Sg552:
	case Weapon::Aug:
		gunMode = 4;
		break;

	case Weapon::Mp5:
	case Weapon::Famas:
	case Weapon::Usp:
	case Weapon::Glock18:
		gunMode = 3;
		break;

	case Weapon::Awp:
	case Weapon::Deagle:
		gunMode = 2;
		break;

	case Weapon::Ak47:
	case Weapon::M4A1:
		gunMode = 1;
		break;
	}

	return gunMode;
}

void Bot::CheckGrenadeThrow(edict_t* targetEntity)
{
	if (FNullEnt(targetEntity))
		return;

	const int grenadeToThrow = CheckGrenades();
	if (grenadeToThrow == -1)
		return;

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
		if (distance > (grenadeToThrow == Weapon::SmGrenade ? SquaredF(400.0f) : SquaredF(600.0f)) && distance < SquaredF(1200.0f))
		{
			// care about different types of grenades
			if (grenadeToThrow == Weapon::HeGrenade)
			{
				bool allowThrowing = true;

				// check for teammates
				if ((GetGameMode() == GameMode::Original || GetGameMode() == GameMode::TeamDeathmatch) && GetNearbyFriendsNearPosition(targetOrigin, SquaredF(256.0f)) > 0)
					allowThrowing = false;

				if (allowThrowing && m_seeEnemyTime + 2.0 < engine->GetTime())
				{
					Vector enemyPredict = ((targetEntity->v.velocity * 0.5).SkipZ() + targetOrigin);
					int searchTab[4], count = 4;

					float searchRadius = targetEntity->v.velocity.GetLength2D();

					// check the search radius
					if (searchRadius < 128.0f)
						searchRadius = 128.0f;

					// search waypoints
					g_waypoint->FindInRadius(enemyPredict, searchRadius, searchTab, &count);

					while (count > 0)
					{
						allowThrowing = true;

						// check the throwing
						const Vector eyePosition = EyePosition();
						const Vector wpOrigin = g_waypoint->GetPath(searchTab[count--])->origin;
						Vector src = CheckThrow(eyePosition, wpOrigin);

						if (src.GetLengthSquared() < SquaredF(100.0f))
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
			else if (grenadeToThrow == Weapon::SmGrenade && GetShootingConeDeviation(targetEntity, &pev->origin) >= 0.9f)
				th = 2;
		}
	}
	else if (grenadeToThrow == Weapon::FbGrenade && (targetOrigin - pev->origin).GetLengthSquared() < SquaredF(800.0f))
	{
		bool allowThrowing = true;
		Array <int> inRadius;

		g_waypoint->FindInRadius(inRadius, 256.0f, targetOrigin + (targetEntity->v.velocity * 0.5).SkipZ());

		ITERATE_ARRAY(inRadius, i)
		{
			if (GetNearbyFriendsNearPosition(g_waypoint->GetPath(i)->origin, SquaredF(256.0f)) != 0)
				continue;

			const Vector wpOrigin = g_waypoint->GetPath(i)->origin;
			const Vector eyePosition = EyePosition();
			Vector src = CheckThrow(eyePosition, wpOrigin);

			if (src.GetLengthSquared() < SquaredF(100.0f))
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
		return;

	switch (th)
	{
	case 1:
		SetProcess(Process::ThrowHE, "throwing HE grenade", false, AddTime(10.0f));
		break;
	case 2:
		SetProcess(Process::ThrowSM, "throwing SM grenade", false, AddTime(10.0f));
		break;
	case 3:
		SetProcess(Process::ThrowFB, "throwing FB grenade", false, AddTime(10.0f));
		break;
	}
}

bool Bot::IsEnemyReachable(void)
{
	const float time = engine->GetTime();
	if (m_enemyReachableTimer > time)
		return m_isEnemyReachable;

	if (!m_isEnemyReachable && (!(m_nearestEnemy->v.flags & FL_ONGROUND) || (m_nearestEnemy->v.flags & FL_PARTIALGROUND && !(m_nearestEnemy->v.flags & FL_DUCKING))))
	{
		m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
		return m_isEnemyReachable;
	}

	m_isEnemyReachable = false;

	if (IsOnLadder())
	{
		m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
		return m_isEnemyReachable;
	}

	if (m_currentTravelFlags & PATHFLAG_JUMP || m_waypointFlags & WAYPOINT_JUMP)
	{
		m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
		return m_isEnemyReachable;
	}

	if (!m_hasEnemiesNear || FNullEnt(m_nearestEnemy))
	{
		m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
		return m_isEnemyReachable;
	}

	edict_t* me = GetEntity();
	const int ownIndex = IsValidWaypoint(m_currentWaypointIndex) ? m_currentWaypointIndex : g_waypoint->FindNearest(pev->origin, 999999.0f, -1, me);
	const int enemyIndex = g_waypoint->FindNearest(m_enemyOrigin, 999999.0f, -1, me);
	const auto currentWaypoint = g_waypoint->GetPath(ownIndex);

	if (currentWaypoint->flags & WAYPOINT_FALLRISK)
	{
		m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
		return m_isEnemyReachable;
	}

	if (m_isZombieBot)
	{
		if (ownIndex == enemyIndex)
		{
			m_isEnemyReachable = true;
			m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
			return m_isEnemyReachable;
		}
		else
		{
			if (m_personality != Personality::Careful)
			{
				// TODO: you know
			}

			m_isEnemyReachable = false;
		}

		const float enemyDistance = (pev->origin - m_enemyOrigin).GetLengthSquared();
		if (pev->flags & FL_DUCKING)
		{
			if (enemyDistance < SquaredF(54.0f) || !HasNextPath())
				m_isEnemyReachable = true;

			pev->speed = pev->maxspeed;
			m_moveSpeed = pev->maxspeed;

			m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
			return m_isEnemyReachable;
		}

		// end of the path, before repathing check the distance if we can reach to enemy
		if (!HasNextPath())
		{
			m_isEnemyReachable = enemyDistance < SquaredF(512.0f);
			if (m_isEnemyReachable)
			{
				m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
				return m_isEnemyReachable;
			}
		}
		else
		{
			float radius = pev->maxspeed;
			if (!(currentWaypoint->flags & WAYPOINT_FALLCHECK) && !(currentWaypoint->flags & WAYPOINT_FALLRISK))
				radius += currentWaypoint->radius * 4.0f;

			if (enemyDistance < SquaredF(radius))
			{
				TraceResult tr;
				TraceHull(pev->origin, m_enemyOrigin, true, head_hull, me, &tr);

				if (tr.flFraction == 1.0f)
				{
					m_isEnemyReachable = true;
					m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
					return m_isEnemyReachable;
				}
			}
		}
	}
	else if (m_hasEnemiesNear && !FNullEnt(m_nearestEnemy))
	{
		m_isEnemyReachable = false;
		if (IsZombieMode())
		{
			if (enemyIndex == ownIndex)
			{
				m_isEnemyReachable = true;
				m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
				return m_isEnemyReachable;
			}

			const Vector enemyVel = m_nearestEnemy->v.velocity;
			const float enemySpeed = cabsf(m_nearestEnemy->v.speed);

			const Vector enemyHead = GetPlayerHeadOrigin(m_nearestEnemy);
			const Vector myVec = pev->origin + pev->velocity * m_frameInterval;

			const float enemyDistance = (myVec - (enemyHead + enemyVel * m_frameInterval)).GetLengthSquared();

			extern ConVar ebot_zp_escape_distance;
			const float escapeDist = SquaredF(enemySpeed + ebot_zp_escape_distance.GetFloat());

			if (pev->flags & FL_DUCKING) // danger...
			{
				if (enemyDistance < escapeDist)
				{
					m_isEnemyReachable = true;
					m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
					return m_isEnemyReachable;
				}
			}
			else if (GetCurrentState() == Process::Camp)
			{
				if (enemyIndex == m_zhCampPointIndex)
					m_isEnemyReachable = true;
				else
				{
					if (enemyDistance < escapeDist)
					{
						for (int j = 0; j < Const_MaxPathIndex; j++)
						{
							const auto enemyWaypoint = g_waypoint->GetPath(enemyIndex);
							if (enemyWaypoint->index[j] != -1 && enemyWaypoint->index[j] == ownIndex && !(enemyWaypoint->connectionFlags[j] & PATHFLAG_JUMP))
							{
								m_isEnemyReachable = true;
								break;
							}
						}

						if (!m_isEnemyReachable)
						{
							const Vector origin = GetBottomOrigin(GetEntity());

							TraceResult tr;
							TraceLine(Vector(origin.x, origin.y, (origin.z + (pev->flags & FL_DUCKING) ? 6.0f : 12.0f)), enemyHead, true, true, GetEntity(), &tr);
							if (tr.flFraction == 1.0f)
							{
								m_isEnemyReachable = true;
								m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
								return m_isEnemyReachable;
							}
						}
					}
				}

				m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
				return m_isEnemyReachable;
			}
			else if (enemyDistance < escapeDist)
			{
				m_isEnemyReachable = true;
				m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
				return m_isEnemyReachable;
			}
		}
	}

	m_enemyReachableTimer = time + CRandomFloat(0.33f, 0.66f);
	return m_isEnemyReachable;
}

void Bot::CheckRadioCommands(void)
{
	if (IsZombieMode())
		return;

	if (m_isBomber)
		return;

	if (m_numFriendsLeft <= 0)
		return;

	if (m_radioOrder == Radio::Affirmative || m_radioOrder != Radio::Negative)
		return;

	if (FNullEnt(m_radioEntity) || !IsAlive(m_radioEntity))
		return;

	if (m_radioOrder != Radio::ReportTeam && m_radioOrder != Radio::ShesGonnaBlow && !FNullEnt(m_nearestFriend) && m_nearestFriend != m_radioEntity)
	{
		// dynamic range :)
		// less bots = more teamwork required
		if (CRandomInt(0, m_numFriendsLeft + 1) > 5)
		{
			RadioMessage(Radio::Negative);
			return;
		}
	}

	int mode = 1;

	switch (m_radioOrder)
	{
	case Radio::FollowMe:
		mode = 2;
		break;
	case Radio::StickTogether:
		mode = 2;
		break;
	case Radio::CoverMe:
		if (g_bombPlanted || m_isBomber)
			mode = 3;
		else
			mode = 2;
		break;
	case Radio::HoldPosition:
		if (g_bombPlanted)
		{
			if (m_inBombZone)
				mode = 3;
		}
		else
			mode = 3;
		break;
	case Radio::TakingFire:
		mode = 2;
		break;
	case Radio::YouTakePoint:
		if ((!g_bombPlanted && m_inBombZone && m_team == Team::Counter) || (g_bombPlanted && m_inBombZone && m_team == Team::Terrorist))
			mode = 3;
		else
			mode = 2;
		break;
	case Radio::EnemySpotted:
		if (m_inBombZone)
			mode = 3;
		else
			mode = 2;
		break;
	case Radio::NeedBackup:
		mode = 2;
	case Radio::GoGoGo:
		if (GetCurrentState() == Process::Camp)
			FinishCurrentProcess("my teammate told me move");
		RadioMessage(Radio::Affirmative);
		SetWalkTime(-1.0f);
	case Radio::ShesGonnaBlow:
		if (OutOfBombTimer())
		{
			RadioMessage(Radio::Affirmative);
			SetProcess(Process::Escape, "my teammate told me i must escape from the bomb", true, AddTime(GetBombTimeleft()));
		}
		else
			RadioMessage(Radio::Negative);
		break;
	case Radio::RegroupTeam:
		mode = 2;
		break;
	case Radio::StormTheFront:
		if (CRandomInt(1, 2) == 1)
			RadioMessage(Radio::Affirmative);
		else
			RadioMessage(Radio::Negative);
		break;
	case Radio::Fallback:
		RadioMessage(Radio::Negative);
		break;
	case Radio::ReportTeam:
		if (m_enemySeeTime + 6.0f < engine->GetTime())
		{
			if (!FNullEnt(m_nearestEnemy) && !IsAlive(m_nearestEnemy))
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
	case Radio::SectorClear:
		if (GetCurrentState() == Process::Camp && m_enemySeeTime + 12.0f < engine->GetTime())
		{
			RadioMessage(Radio::Affirmative);
			FinishCurrentProcess("my teammate told me sector is clear");
		}
		break;
	case Radio::GetInPosition:
		mode = 3;
		break;
	}

	if (mode == 2)
	{
		const int index = g_waypoint->FindNearest(m_radioEntity->v.origin, 99999999.0f, -1, m_radioEntity);
		if (IsValidWaypoint(index))
		{
			RadioMessage(Radio::Affirmative);
			FindPath(m_currentWaypointIndex, index);
		}
		else
			RadioMessage(Radio::Negative);
	}
	else if (mode == 3)
	{
		const int index = FindDefendWaypoint(m_radioEntity->v.origin + m_radioEntity->v.view_ofs);
		if (IsValidWaypoint(index))
		{
			RadioMessage(Radio::Affirmative);
			m_campIndex = index;
			SetProcess(Process::Camp, "i will hold this position for my teammate", true, AddTime(CRandomFloat(ebot_camp_min.GetFloat(), ebot_camp_max.GetFloat())));
		}
		else
			RadioMessage(Radio::Negative);
	}
	else
		RadioMessage(Radio::Negative);

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

	if (m_numEnemiesLeft <= 0)
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
	if (!IsOnFloor())
		return pev->maxspeed;

	if (m_walkTime > engine->GetTime())
		return pev->maxspeed * 0.502f;

	return pev->maxspeed;
}

bool Bot::IsNotAttackLab(edict_t* entity)
{
	if (FNullEnt(entity))
		return true;

	if (entity->v.takedamage == DAMAGE_NO)
		return true;

	if (entity->v.rendermode == kRenderTransAlpha)
	{
		// enemy fires gun
		if ((entity->v.weapons & WeaponBits_Primary) || (entity->v.weapons & WeaponBits_Secondary) && (entity->v.button & IN_ATTACK) || (entity->v.oldbuttons & IN_ATTACK))
			return false;

		const float renderamt = entity->v.renderamt;

		if (renderamt <= 30.0f)
			return true;

		if (renderamt > 160.0f)
			return false;

		return (SquaredF(renderamt) < (entity->v.origin - pev->origin).GetLengthSquared2D());
	}

	return false;
}

void Bot::BaseUpdate(void)
{
	// run playermovement
	const float time = engine->GetTime();
	const uint8_t adjustedMSec = static_cast<uint8_t>(cminf(250.0f, (time - m_msecInterval) * 1000.0f));
	m_msecInterval = time;
	PLAYER_RUN_MOVE(pev->pContainingEntity, m_moveAngles, m_moveSpeed, m_strafeSpeed, 0.0f, static_cast<unsigned short>(pev->button), pev->impulse, adjustedMSec);

	// reset
	pev->impulse = 0;
	pev->button = 0;
	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;

	// is bot movement enabled
	bool botMovement = false;

	// if the bot hasn't selected stuff to start the game yet, go do that...
	// select team & class
	if (m_notStarted)
		StartGame();
	else
	{
		if (m_slowthinktimer > time)
			m_isSlowThink = false;
		else
		{
			m_isSlowThink = true;
			pev->flags |= FL_FAKECLIENT;
			CheckSlowThink();
			m_slowthinktimer = time + CRandomFloat(0.9f, 1.1f);
		}

		if (!m_isAlive)
		{
			if (!g_isFakeCommand)
			{
				extern ConVar ebot_random_join_quit;
				if (ebot_random_join_quit.GetBool() && m_stayTime > 0.0f && m_stayTime < time)
				{
					Kick();
					return;
				}

				extern ConVar ebot_chat;
				if (ebot_chat.GetBool() && !RepliesToPlayer() && m_lastChatTime + 10.0f < time && g_lastChatTime + 5.0f < time) // bot chatting turned on?
				{
					m_lastChatTime = time;
					if (ChanceOf(ebot_chat_percent.GetInt()) && !g_chatFactory[CHAT_DEAD].IsEmpty())
					{
						g_lastChatTime = time;

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
						if (m_sayTextBuffer.lastUsedSentences.GetElementNumber() > CRandomInt(4, 6))
							m_sayTextBuffer.lastUsedSentences.Destroy();
					}
				}

				if (g_gameVersion & Game::HalfLife && !(pev->oldbuttons & IN_ATTACK))
					pev->button |= IN_ATTACK;
			}
		}
		else
		{
			if (g_gameVersion & Game::HalfLife)
			{
				// idk why ???
				if (pev->maxspeed < 11.0f)
				{
					const auto maxSpeed = g_engfuncs.pfnCVarGetPointer("sv_maxspeed");
					if (maxSpeed != nullptr)
						pev->maxspeed = maxSpeed->value;
					else // default is 270
						pev->maxspeed = 270.0f;
				}

				if (!ebot_stopbots.GetBool())
					botMovement = true;
			}
			else
			{
				if (!m_buyingFinished)
					ResetStuck();

				if (m_buyingFinished && !ebot_stopbots.GetBool())
					botMovement = true;
			}

			if (botMovement)
			{
				UpdateProcess();
				MoveAction();
				DebugModeMsg();
			}
		}
	}

	// check for pending messages
	CheckMessageQueue();

	// avoid frame drops
	m_frameInterval = time - m_frameDelay;
	m_frameDelay = time;
}

void Bot::CheckSlowThink(void)
{
	CalculatePing();

	const float time = engine->GetTime();
	if (m_stayTime < 0.0f)
	{
		extern ConVar ebot_random_join_quit;
		if (ebot_random_join_quit.GetBool())
		{
			extern ConVar ebot_stay_min;
			extern ConVar ebot_stay_max;
			m_stayTime = time + CRandomFloat(ebot_stay_min.GetFloat(), ebot_stay_max.GetFloat());
		}
		else
			m_stayTime = time + 999999.0f;
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
	}

	extern ConVar ebot_ignore_enemies;
	if (!ebot_ignore_enemies.GetBool() && m_randomattacktimer < time && !engine->IsFriendlyFireOn() && !HasHostage()) // simulate players with random knife attacks
	{
		if (m_isStuck && m_personality != Personality::Careful)
		{
			if (m_personality == Personality::Rusher)
				m_randomattacktimer = 0.0f;
			else
				m_randomattacktimer = time + CRandomFloat(0.1f, 10.0f);
		}
		else if (m_personality == Personality::Rusher)
			m_randomattacktimer = time + CRandomFloat(0.1f, 30.0f);
		else if (m_personality == Personality::Careful)
			m_randomattacktimer = time + CRandomFloat(10.0f, 100.0f);
		else
			m_randomattacktimer = time + CRandomFloat(0.15f, 75.0f);

		if (m_currentWeapon == Weapon::Knife)
		{
			if (CRandomInt(1, 3) == 1)
				pev->button |= IN_ATTACK;
			else
				pev->button |= IN_ATTACK2;
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
}

bool Bot::IsAttacking(const edict_t* player)
{
	return (player->v.button & IN_ATTACK || player->v.oldbuttons & IN_ATTACK);
}

void Bot::UpdateLooking(void)
{
	FacePosition();

	if (m_hasEnemiesNear || m_hasEntitiesNear)
	{
		LookAtEnemies();
		CheckReload();
		FireWeapon();
		return;
	}
	else
	{
		const float val = IsZombieMode() ? 8.0f : 4.0f;
		if (m_enemySeeTime + val > engine->GetTime() && m_team != GetTeam(m_nearestEnemy))
		{
			if (!FNullEnt(m_nearestEnemy) && IsAlive(m_nearestEnemy))
				m_lookAt = m_nearestEnemy->v.origin + m_nearestEnemy->v.view_ofs;
			else
				m_lookAt = m_enemyOrigin;

			return;
		}
	}

	LookAtAround();
}

void Bot::LookAtEnemies(void)
{
	if (m_entityDistance < m_enemyDistance)
		m_lookAt = m_entityOrigin;
	else
		m_lookAt = GetEnemyPosition();
}

float Bot::GetTargetDistance(void)
{
	if (m_entityDistance < m_enemyDistance)
		return m_entityDistance;

	return m_enemyDistance;
}

void Bot::LookAtAround(void)
{
	if (m_waypointFlags & WAYPOINT_LADDER || IsOnLadder())
	{
		m_lookAt = m_destOrigin + pev->view_ofs;
		m_pauseTime = 0.0f;
		return;
	}
	else if (m_waypointFlags & WAYPOINT_USEBUTTON)
	{
		const Vector origin = g_waypoint->GetPath(m_currentWaypointIndex)->origin;
		if ((pev->origin - origin).GetLengthSquared() < SquaredF(80.0f))
		{
			if (g_gameVersion & Game::Xash)
			{
				m_lookAt = origin;
				if (!(pev->button & IN_USE) && !(pev->oldbuttons & IN_USE))
					pev->button |= IN_USE;
				return;
			}
			else
			{
				edict_t* button = FindButton();
				if (button != nullptr) // no need to look at, thanks to the game...
					MDLL_Use(button, pev->pContainingEntity);
				else
				{
					m_lookAt = origin;
					if (!(pev->button & IN_USE) && !(pev->oldbuttons & IN_USE))
						pev->button |= IN_USE;
					return;
				}
			}
		}
	}
	else if (!m_isZombieBot && m_hasFriendsNear && !FNullEnt(m_nearestFriend) && IsAttacking(m_nearestFriend) && cstrncmp(STRING(m_nearestFriend->v.viewmodel), "models/v_knife", 14) != 0)
	{
		auto bot = g_botManager->GetBot(m_nearestFriend);
		if (bot != nullptr)
		{
			m_lookAt = bot->m_lookAt;

			if (bot->m_hasEnemiesNear)
			{
				edict_t* enemy = bot->m_nearestEnemy;
				if (!FNullEnt(enemy))
				{
					CheckGrenadeThrow(enemy);

					edict_t* ent = bot->GetEntity();
					if ((m_currentWeapon == Weapon::M3 || m_currentWeapon == Weapon::Xm1014 || m_currentWeapon == Weapon::M249))
					{
						const int index = g_waypoint->FindNearest(GetEntityOrigin(enemy), 999999.0f, -1, ent);
						if (IsValidWaypoint(index) && m_chosenGoalIndex != index)
						{
							RadioMessage(Radio::GetInPosition);
							m_chosenGoalIndex = index;
							FindPath(m_currentWaypointIndex, m_chosenGoalIndex, enemy);
							const int index = FindDefendWaypoint(EyePosition());
							if (IsValidWaypoint(index))
							{
								bot->RadioMessage(Radio::Affirmative);
								bot->m_campIndex = index;
								bot->m_chosenGoalIndex = index;
								bot->SetProcess(Process::Camp, "i will hold this position for my teammate's plan, my teammate will flank the enemy!", true, AddTime(ebot_camp_max.GetFloat()));
							}
							else
								bot->RadioMessage(Radio::Negative);
						}
					}

					if (bot->GetCurrentState() == Process::Camp)
					{
						const int index = g_waypoint->FindNearest(m_lookAt, 999999.0f, -1, ent);
						if (IsValidWaypoint(index) && m_chosenGoalIndex != index)
						{
							RadioMessage(Radio::GetInPosition);
							m_chosenGoalIndex = index;
							FindPath(m_currentWaypointIndex, m_chosenGoalIndex);
							m_chosenGoalIndex = bot->m_chosenGoalIndex;
						}
					}
					else if (GetCurrentState() == Process::Camp)
					{
						const int index = g_waypoint->FindNearest(m_lookAt, 999999.0f, -1, ent);
						if (IsValidWaypoint(index) && m_chosenGoalIndex != index)
						{
							FinishCurrentProcess("i need to help my friend");
							RadioMessage(Radio::Fallback);
							m_chosenGoalIndex = index;
							FindPath(m_currentWaypointIndex, m_chosenGoalIndex);
							m_chosenGoalIndex = bot->m_chosenGoalIndex;
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

		m_pauseTime = AddTime(CRandomFloat(2.0f, 5.0f));
		SetWalkTime(7.0f);

		if (m_currentWeapon == Weapon::Knife)
			SelectBestWeapon();

		return;
	}

	const float time = engine->GetTime();
	if (m_pauseTime > time)
		return;

	if (m_isSlowThink && ChanceOf(m_senseChance)) // who's footsteps is this or fire sounds?
	{
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

			float maxDist = 1280.0f;
			if (!IsAttacking(client.ent))
			{
				if ((client.ent->v.flags & FL_DUCKING))
					continue;

				if (client.ent->v.speed < client.ent->v.maxspeed * 0.66f)
					continue;

				maxDist = 768.0f;
			}

			if (((pev->origin + pev->velocity) - (client.ent->v.origin + client.ent->v.velocity)).GetLengthSquared() > SquaredF(maxDist))
				continue;

			if (m_isZombieBot)
			{
				if (!m_hasEnemiesNear && (FNullEnt(m_nearestEnemy) || GetTeam(m_nearestEnemy) == m_team))
				{
					m_nearestEnemy = client.ent;
					DeleteSearchNodes();
				}
			}

			const int skill = m_personality == Personality::Rusher ? 30 : 10;
			if (m_skill > skill)
			{
				SetWalkTime(7.0f);
				SelectBestWeapon();
			}

			m_pauseTime = time + CRandomFloat(2.0f, 5.0f);
			m_lookAt = client.ent->v.origin + client.ent->v.view_ofs;
			CheckGrenadeThrow(client.ent);

			return;
		}
	}
	
	if (m_navNode != nullptr && m_navNode->next != nullptr)
	{
		const PathNode* third = m_navNode->next->next.get();
		if (third != nullptr)
			m_lookAt = g_waypoint->GetPath(third->index)->origin + pev->view_ofs;
	}

	if (m_isZombieBot || m_searchTime > time)
		return;

	const Vector nextFrame = EyePosition() + pev->velocity;
	Vector selectRandom;
	Vector bestLookPos = nextFrame;
	bestLookPos.x += CRandomFloat(-1024.0f, 1024.0f);
	bestLookPos.y += CRandomFloat(-1024.0f, 1024.0f);
	bestLookPos.z += CRandomFloat(-256.0f, 256.0f);

	const int index = g_waypoint->FindNearestInCircle(bestLookPos, 999999.0f);
	if (IsValidWaypoint(index) && m_knownWaypointIndex[0] != index && m_knownWaypointIndex[1] != index && m_knownWaypointIndex[2] != index && m_knownWaypointIndex[3] != index && m_knownWaypointIndex[4] != index && m_knownWaypointIndex[5] != index)
	{
		const Path* pointer = g_waypoint->GetPath(index);
		const Vector waypointOrigin = pointer->origin;
		const float waypointRadius = static_cast<float>(pointer->radius);
		selectRandom.x = waypointOrigin.x + CRandomFloat(-waypointRadius, waypointRadius);
		selectRandom.y = waypointOrigin.y + CRandomFloat(-waypointRadius, waypointRadius);
		selectRandom.z = waypointOrigin.z + pev->view_ofs.z;
		m_knownWaypointIndex[0] = index;
		m_knownWaypointIndex[1] = m_knownWaypointIndex[0];
		m_knownWaypointIndex[2] = m_knownWaypointIndex[1];
		m_knownWaypointIndex[3] = m_knownWaypointIndex[2];
		m_knownWaypointIndex[4] = m_knownWaypointIndex[3];
		m_knownWaypointIndex[5] = m_knownWaypointIndex[4];
	}

	auto isVisible = [&](void)
	{
		TraceResult tr{};
		TraceLine(EyePosition(), selectRandom, true, true, pev->pContainingEntity, &tr);
		if (tr.flFraction == 1.0f)
			return true;

		return false;
	};

	if ((pev->origin - selectRandom).GetLengthSquared() > SquaredF(512.0f) && isVisible())
	{
		m_lookAt = selectRandom;
		m_pauseTime = time + CRandomFloat(1.0f, 1.5f);
	}

	m_searchTime = time + 0.25f;
}

void Bot::SecondThink(void)
{
	if (g_gameVersion != Game::HalfLife)
	{
		// don't forget me :(
		/*if (ebot_use_flare.GetBool() && !m_isReloading && !m_isZombieBot && GetGameMode() == GameMode::ZombiePlague && FNullEnt(m_enemy) && !FNullEnt(m_lastEnemy))
		{
			if (pev->weapons & (1 << Weapon::SmGrenade) && ChanceOf(40))
				PushTask(TASK_THROWFLARE, TASKPRI_THROWGRENADE, -1, CRandomFloat(0.6f, 0.9f), false);
		}*/

		// zp & biohazard flashlight support
		if (ebot_force_flashlight.GetBool() && !m_isZombieBot && !(pev->effects & EF_DIMLIGHT))
			pev->impulse = 100;
	}
	else
	{
		m_startAction = CMENU_IDLE;
		m_numFriendsLeft = 0;
	}

	if (m_voteMap != 0) // host wants the bots to vote for a map?
	{
		FakeClientCommand(GetEntity(), "votemap %d", m_voteMap);
		m_voteMap = 0;
	}
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

	for (const auto& client : g_clients)
	{
		if (FNullEnt(client.ent))
			continue;

		if (!(client.flags & CFLAG_USED))
			continue;

		if (IsValidBot(client.ent))
			continue;

		numHumans++;

		int ping, loss;
		PLAYER_CNX_STATS(client.ent, &ping, &loss);

		if (ping <= 0 || ping > 150)
			ping = CRandomInt(5, 50);

		averagePing += ping;
	}

	if (numHumans > 0)
		averagePing /= numHumans;
	else
		averagePing = CRandomInt(30, 60);

	int botPing = m_basePingLevel + CRandomInt(averagePing - averagePing * 0.2f, averagePing + averagePing * 0.2f) + CRandomInt(m_difficulty + 3, m_difficulty + 6);
	if (botPing < 9)
		botPing = CRandomInt(9, 19);
	else if (botPing > 133)
		botPing = CRandomInt(99, 119);

	m_ping = (botPing + m_pingOffset) / 2;
	m_pingOffset = botPing;
}

void Bot::MoveAction(void)
{
	const float time = engine->GetTime();
	if (pev->button & IN_JUMP)
		m_jumpTime = time;

	if (m_jumpTime + 2.0f > time)
	{
		if (!IsOnFloor() && !IsInWater() && !IsOnLadder())
			pev->button |= IN_DUCK;
	}
	else
	{
		if (m_moveSpeed > 0.0f)
			pev->button |= IN_FORWARD;
		else if (m_moveSpeed < 0.0f)
			pev->button |= IN_BACK;

		if (m_strafeSpeed > 0.0f)
			pev->button |= IN_MOVERIGHT;
		else if (m_strafeSpeed < 0.0f)
			pev->button |= IN_MOVELEFT;

		if (m_duckTime > time)
			pev->button |= IN_DUCK;
	}
}

void Bot::DebugModeMsg(void)
{
	const int debugMode = ebot_debug.GetInt();
	if (debugMode <= 0 || debugMode == 2 || FNullEnt(g_hostEntity))
		return;

	static float timeDebugUpdate = 0.0f;

	const int specIndex = g_hostEntity->v.iuser2;
	if (specIndex != GetIndex())
		return;

	static int index, goal;
	static Process processID, rememberedProcessID;

	if (processID != m_currentProcess || rememberedProcessID != m_rememberedProcess || index != m_currentWaypointIndex || goal != m_chosenGoalIndex || timeDebugUpdate < engine->GetTime())
	{
		processID = m_currentProcess;
		index = m_currentWaypointIndex;
		goal = m_chosenGoalIndex;

		char processName[80];
		sprintf(processName, GetProcessName(m_currentProcess));

		char rememberedProcessName[80];
		sprintf(rememberedProcessName, GetProcessName(m_rememberedProcess));

		char weaponName[80], botType[32];
		char enemyName[80], friendName[80];

		if (!FNullEnt(m_nearestEnemy) && IsAlive(m_nearestEnemy))
			sprintf(enemyName, "%s", GetEntityName(m_nearestEnemy));
		else if (!FNullEnt(m_nearestEntity) && IsAlive(m_nearestEntity))
			sprintf(enemyName, "%s", GetEntityName(m_nearestEntity));
		else
			sprintf(enemyName, "%s", GetEntityName(nullptr));

		if (!FNullEnt(m_nearestFriend) && IsAlive(m_nearestFriend))
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
				sprintf(weaponName, "weapon_hegrenade");
				break;

			case Weapon::FbGrenade:
				sprintf(weaponName, "weapon_flashbang");
				break;

			case Weapon::SmGrenade:
				sprintf(weaponName, "weapon_smokegrenade");
				break;

			case Weapon::C4:
				sprintf(weaponName, "weapon_c4");
				break;

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
			sprintf(gamemodName, "Normal");
			break;

		case GameMode::Deathmatch:
			sprintf(gamemodName, "Deathmatch");
			break;

		case GameMode::ZombiePlague:
			sprintf(gamemodName, "Zombie Mode");
			break;

		case GameMode::NoTeam:
			sprintf(gamemodName, "No Team");
			break;

		case GameMode::ZombieHell:
			sprintf(gamemodName, "Zombie Hell");
			break;

		case GameMode::TeamDeathmatch:
			sprintf(gamemodName, "Team Deathmatch");
			break;

		default:
			sprintf(gamemodName, "UNKNOWN MODE");
		}

		PathNode* navid = &m_navNode.get()[0];
		int navIndex[2] = { 0, 0 };

		while (navid != nullptr)
		{
			if (navIndex[0] == 0)
				navIndex[0] = navid->index;
			else
			{
				navIndex[1] = navid->index;
				break;
			}

			navid = navid->next.get();
		}

		const int client = GetIndex() - 1;

		char outputBuffer[512];
		sprintf(outputBuffer, "\n\n\n\n\n\n\n Game Mode: %s"
			"\n [%s] \n Process: %s  RE-Process: %s \n"
			"Weapon: %s  Clip: %d  Ammo: %d \n"
			"Type: %s  Money: %d  Heuristic: %s \n"
			"Enemy: %s  Friend: %s \n\n"

			"Current Index: %d  Goal Index: %d  Prev Goal Index: %d \n"
			"Nav: %d  Next Nav: %d \n"
			//"GEWI: %d  GEWI2: %d \n"
			"Move Speed: %2.f  Strafe Speed: %2.f \n "
			"Stuck Warnings: %d  Stuck: %s \n",
			gamemodName,
			GetEntityName(GetEntity()), processName, rememberedProcessName,
			&weaponName[7], GetAmmoInClip(), GetAmmo(),
			botType, m_moneyAmount, GetHeuristicName(),
			enemyName, friendName,

			m_currentWaypointIndex, goal, m_prevGoalIndex,
			navIndex[0], navIndex[1],
			//g_clients[client].wpIndex, g_clients[client].wpIndex2,
			m_moveSpeed, m_strafeSpeed,
			m_stuckWarn, m_isStuck ? "Yes" : "No");

		if (g_messageEnded)
		{
			MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, nullptr, g_hostEntity);
			WRITE_BYTE(TE_TEXTMESSAGE);
			WRITE_BYTE(1);
			WRITE_SHORT(FixedSigned16(-1, 1 << 13));
			WRITE_SHORT(FixedSigned16(0, 1 << 13));
			WRITE_BYTE(0);
			WRITE_BYTE(m_team == Team::Counter ? 0 : 255);
			WRITE_BYTE(100);
			WRITE_BYTE(m_team != Team::Counter ? 0 : 255);
			WRITE_BYTE(0);
			WRITE_BYTE(255);
			WRITE_BYTE(255);
			WRITE_BYTE(255);
			WRITE_BYTE(0);
			WRITE_SHORT(FixedUnsigned16(0, 1 << 8));
			WRITE_SHORT(FixedUnsigned16(0, 1 << 8));
			WRITE_SHORT(FixedUnsigned16(1.0, 1 << 8));
			WRITE_STRING(const_cast<const char*>(&outputBuffer[0]));
			MESSAGE_END();
		}

		timeDebugUpdate = AddTime(1.0f);
	}

	if (m_hasEnemiesNear && m_enemyOrigin != nullvec)
		engine->DrawLine(g_hostEntity, EyePosition(), m_enemyOrigin, Color(255, 0, 0, 255), 10, 0, 5, 1, LINE_SIMPLE);

	if (m_destOrigin != nullvec)
		engine->DrawLine(g_hostEntity, pev->origin, m_destOrigin, Color(0, 0, 255, 255), 10, 0, 5, 1, LINE_SIMPLE);

	// now draw line from source to destination
	PathNode* node = &m_navNode.get()[0];
	while (node != nullptr)
	{
		Path* path = g_waypoint->GetPath(node->index);
		const Vector src = path->origin;
		node = node->next.get();

		if (node != nullptr)
		{
			bool jumpPoint = false;
			bool boostPoint = false;
			for (int j = 0; j < Const_MaxPathIndex; j++)
			{
				if (path->index[j] == node->index)
				{
					if (path->connectionFlags[j] & PATHFLAG_JUMP)
					{
						jumpPoint = true;
						break;
					}
					else if (path->connectionFlags[j] & PATHFLAG_DOUBLE)
					{
						boostPoint = true;
						break;
					}
				}
			}

			if (jumpPoint)
				engine->DrawLine(g_hostEntity, src, g_waypoint->GetPath(node->index)->origin, Color(255, 0, 0, 255), 15, 0, 8, 1, LINE_SIMPLE);
			else if (boostPoint)
				engine->DrawLine(g_hostEntity, src, g_waypoint->GetPath(node->index)->origin, Color(0, 0, 255, 255), 15, 0, 8, 1, LINE_SIMPLE);
			else
				engine->DrawLine(g_hostEntity, src, g_waypoint->GetPath(node->index)->origin, Color(255, 100, 55, 255), 15, 0, 8, 1, LINE_SIMPLE);
		}
	}

	if (IsValidWaypoint(m_currentWaypointIndex))
	{
		if (m_currentTravelFlags & PATHFLAG_JUMP)
			engine->DrawLine(g_hostEntity, pev->origin, g_waypoint->GetPath(m_currentWaypointIndex)->origin, Color(255, 0, 0, 255), 15, 0, 8, 1, LINE_SIMPLE);
		else if (m_currentTravelFlags & PATHFLAG_DOUBLE)
			engine->DrawLine(g_hostEntity, pev->origin, g_waypoint->GetPath(m_currentWaypointIndex)->origin, Color(0, 0, 255, 255), 15, 0, 8, 1, LINE_SIMPLE);
		else
			engine->DrawLine(g_hostEntity, pev->origin, g_waypoint->GetPath(m_currentWaypointIndex)->origin, Color(255, 100, 55, 255), 15, 0, 8, 1, LINE_SIMPLE);
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
	for (auto& hostage : m_hostages)
	{
		if (!FNullEnt(hostage))
		{
			// don't care about dead hostages
			if (hostage->v.health <= 0.0f || (pev->origin - GetEntityOrigin(hostage)).GetLengthSquared() > SquaredF(600.0f))
			{
				hostage = nullptr;
				continue;
			}

			return true;
		}
	}

	return false;
}

int Bot::GetAmmo(void)
{
	if (g_weaponDefs[m_currentWeapon].ammo1 < 0)
		return 0;

	return m_ammo[g_weaponDefs[m_currentWeapon].ammo1];
}

// this function gets called by network message handler, when screenfade message get's send
// it's used to make bot blind froumd the grenade
void Bot::TakeBlinded(const Vector fade, const int alpha)
{
	if (fade.x != 255 || fade.y != 255 || fade.z != 255 || alpha <= 170)
		return;

	SetProcess(Process::Blind, "i'm blind", false, AddTime(CRandomFloat(3.0f, 6.0f)));
}

// this function, asks bot to discard his current primary weapon (or c4) to the user that requsted it with /drop*
// command, very useful, when i don't have money to buy anything... )
void Bot::DiscardWeaponForUser(edict_t* user, const bool discardC4)
{
	if (!m_buyingFinished && FNullEnt(user))
		return;

	const Vector userOrigin = GetEntityOrigin(user);
	if (IsAlive(user) && m_moneyAmount >= 2000 && HasPrimaryWeapon() && (userOrigin - pev->origin).GetLengthSquared() < SquaredF(240.0f))
	{
		m_lookAt = userOrigin;

		if (discardC4)
		{
			SelectWeaponByName("weapon_c4");
			FakeClientCommand(GetEntity(), "drop");
		}
		else
		{
			SelectBestWeapon(true, true);
			FakeClientCommand(GetEntity(), "drop");
		}

		m_pickupItem = nullptr;
		m_pickupType = PickupType::None;
		m_itemCheckTime = AddTime(5.0f);

		if (m_inBuyZone)
		{
			m_buyingFinished = false;
			m_buyState = 0;

			PushMessageQueue(CMENU_BUY);
			m_nextBuyTime = engine->GetTime();
		}
	}
	else
	{
		RadioMessage(Radio::Negative);
		ChatSay(false, FormatBuffer("Sorry %s, i don't want discard my %s to you!", GetEntityName(user), discardC4 ? "bomb" : "weapon"));
	}
}

void Bot::ResetDoubleJumpState(void)
{
	DeleteSearchNodes();
	m_doubleJumpEntity = nullptr;
	m_doubleJumpOrigin = nullvec;
	m_jumpReady = false;
}

Vector Bot::CheckToss(const Vector& start, const Vector& stop)
{
	// this function returns the velocity at which an object should looped from start to land near end.
	// returns nullvec if toss is not feasible
	TraceResult tr{};
	const float gravity = engine->GetGravity() * 0.55f;

	Vector end = stop - pev->velocity;
	end.z -= 15.0f;

	if (cabsf(end.z - start.z) > 500.0f)
		return nullvec;

	Vector midPoint = start + (end - start) * 0.5f;
	TraceHull(midPoint, midPoint + Vector(0.0f, 0.0f, 500.0f), true, head_hull, pev->pContainingEntity, &tr);

	if (tr.flFraction < 1.0f && tr.pHit)
	{
		midPoint = tr.vecEndPos;
		midPoint.z = tr.pHit->v.absmin.z - 1.0f;
	}

	if (midPoint.z < start.z || midPoint.z < end.z)
		return nullvec;

	const float timeOne = csqrtf((midPoint.z - start.z) / (0.5f * gravity));
	const float timeTwo = csqrtf((midPoint.z - end.z) / (0.5f * gravity));

	if (timeOne < 0.1f)
		return nullvec;

	Vector velocity = (end - start) / (timeOne + timeTwo);
	velocity.z = gravity * timeOne;

	Vector apex = start + velocity * timeOne;
	apex.z = midPoint.z;

	TraceHull(start, apex, false, head_hull, pev->pContainingEntity, &tr);

	if (tr.flFraction < 1.0f || tr.fAllSolid)
		return nullvec;

	TraceHull(end, apex, true, head_hull, pev->pContainingEntity, &tr);

	if (tr.flFraction != 1.0f)
	{
		const float dot = -(tr.vecPlaneNormal | (apex - end).Normalize());
		if (dot > 0.75f || tr.flFraction < 0.8f)
			return nullvec;
	}

	return velocity * 0.777f;
}

Vector Bot::CheckThrow(const Vector& start, const Vector& end)
{
	// this function returns the velocity vector at which an object should be thrown from start to hit end.
	// returns nullvec if throw is not feasible.
	Vector velocity = end - start;
	TraceResult tr{};

	const float gravity = engine->GetGravity() * 0.55f;
	float time = velocity.GetLength() * 0.00512820512f;

	if (time < 0.01f)
		return nullvec;

	if (time > 2.0f)
		time = 1.2f;

	const float half = time * 0.5f;

	velocity = velocity * (1.0f / time);
	velocity.z += gravity * half * half;

	Vector apex = start + (end - start) * 0.5f;
	apex.z += 0.5f * gravity * half;

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
		pev->button |= IN_ATTACK2;
	else if (m_currentWeapon == Weapon::Glock18 && distance >= 30.0f && m_weaponBurstMode == BurstMode::Enabled)
		pev->button |= IN_ATTACK2;

	// if current weapon is famas, disable burstmode on short distances, enable it else
	if (m_currentWeapon == Weapon::Famas && distance > 400.0f && m_weaponBurstMode == BurstMode::Disabled)
		pev->button |= IN_ATTACK2;
	else if (m_currentWeapon == Weapon::Famas && distance <= 400.0f && m_weaponBurstMode == BurstMode::Enabled)
		pev->button |= IN_ATTACK2;
}

void Bot::CheckSilencer(void)
{
	if ((m_currentWeapon == Weapon::Usp && m_skill < 90) || m_currentWeapon == Weapon::M4A1 && !HasShield())
	{
		const int random = (m_personality == Personality::Rusher ? 33 : m_personality == Personality::Careful ? 99 : 66);
		if (ChanceOf(m_currentWeapon == Weapon::Usp ? random / 3 : random))
		{
			if (pev->weaponanim > 6) // is the silencer not attached...
				pev->button |= IN_ATTACK2; // attach the silencer
		}
		else if (pev->weaponanim <= 6) // is the silencer attached...
			pev->button |= IN_ATTACK2; // detach the silencer
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
	float estimatedTime = 6.0f; // time to reach next waypoint

	// calculate 'real' time that we need to get from one waypoint to another
	if (IsValidWaypoint(m_currentWaypointIndex) && IsValidWaypoint(m_prevWptIndex[0]))
	{
		const float distance = (g_waypoint->GetPath(m_prevWptIndex[0])->origin - m_destOrigin).GetLengthSquared();

		// caclulate estimated time
		estimatedTime = 5.0f * (distance / SquaredF(pev->maxspeed));

		// check for special waypoints, that can slowdown our movement
		if ((m_waypointFlags & WAYPOINT_CROUCH) || (m_waypointFlags & WAYPOINT_LADDER) || (pev->button & IN_DUCK))
			estimatedTime *= 3.0f;

		// check for too low values
		if (estimatedTime < 3.0f)
			estimatedTime = 3.0f;

		// check for too high values
		else if (estimatedTime > 8.0f)
			estimatedTime = 8.0f;
	}

	return estimatedTime;
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

		if (!(g_mapType & MAP_DE))
		{
			if (m_numEnemiesLeft <= 1 || m_numFriendsLeft <= 0)
				return false;
		}
		else if (OutOfBombTimer())
			return false;

		if (m_team == Team::Counter)
		{
			if (g_mapType & MAP_CS && HasHostage())
				return false;
			else if (g_bombPlanted && g_mapType & MAP_DE && !IsBombDefusing(g_waypoint->GetBombPosition()))
				return false;
		}
		else
		{
			if (g_mapType & MAP_DE)
			{
				if (g_bombPlanted)
				{
					const Vector bomb = g_waypoint->GetBombPosition();

					// where is the bomb?
					if (bomb == nullvec)
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
			if (!FNullEnt(m_nearestEnemy) && m_numFriendsLeft < m_numEnemiesLeft)
				return false;
			else if (FNullEnt(m_nearestEnemy) && m_numFriendsLeft > m_numEnemiesLeft)
				return false;
		}
	}

	return true;
}

bool Bot::OutOfBombTimer(void)
{
	if (!(g_mapType & MAP_DE))
		return false;

	if (!g_bombPlanted)
		return false;

	if (GetCurrentState() == Process::Defuse || m_hasProgressBar || GetCurrentState() == Process::Escape)
		return false; // if CT bot already start defusing, or already escaping, return false

	// calculate left time
	const float timeLeft = GetBombTimeleft();

	// if time left greater than 12, no need to do other checks
	if (timeLeft >= 12.0f)
		return false;
	else if (m_team == Team::Terrorist)
		return true;

	const Vector& bombOrigin = g_waypoint->GetBombPosition();

	// bot will belive still had a chance
	if ((m_hasDefuser && IsVisible(bombOrigin, GetEntity())) || (bombOrigin - pev->origin).GetLengthSquared() <= SquaredF(512.0f))
		return false;

	bool hasTeammatesWithDefuserKit = false;
	// check if our teammates has defusal kit
	if (m_numFriendsLeft > 0)
	{
		for (const auto& bot : g_botManager->m_bots)
		{
			// search players with defuse kit
			if (bot != nullptr && bot->m_team == Team::Counter && bot->m_hasDefuser && (bombOrigin - bot->pev->origin).GetLengthSquared() <= SquaredF(512.0f))
			{
				hasTeammatesWithDefuserKit = true;
				break;
			}
		}
	}

	// add reach time to left time
	const float reachTime = g_waypoint->GetTravelTime(pev->maxspeed, g_waypoint->GetPath(m_currentWaypointIndex)->origin, bombOrigin);

	// for counter-terrorist check alos is we have time to reach position plus average defuse time
	if ((timeLeft < reachTime && !m_hasDefuser && !hasTeammatesWithDefuserKit) || (timeLeft < reachTime && m_hasDefuser))
		return true;

	if ((m_hasDefuser ? 6.0f : 12.0f) + GetEstimatedReachTime() < GetBombTimeleft())
		return true;

	return false; // return false otherwise
}

// this function is gets called when bot enters a buyzone, to allow bot to buy some stuff
void Bot::EquipInBuyzone(const int iBuyCount)
{
	if (g_gameVersion & Game::HalfLife)
		return;

	static float lastEquipTime = 0.0f;
	const float time = engine->GetTime();

	// if bot is in buy zone, try to buy ammo for this weapon...
	if (lastEquipTime + 15.0f < time && m_inBuyZone && g_timeRoundStart + CRandomFloat(10.0f, 20.0f) + engine->GetBuyTime() < time && !g_bombPlanted && m_moneyAmount > 800)
	{
		m_buyingFinished = false;
		m_buyState = iBuyCount;

		// push buy message
		PushMessageQueue(CMENU_BUY);

		m_nextBuyTime = time;
		lastEquipTime = time;
	}
}

bool Bot::IsBombDefusing(const Vector bombOrigin)
{
	// this function finds if somebody currently defusing the bomb.
	if (!g_bombPlanted)
		return false;

	// easier way
	if (g_bombDefusing)
		return true;

	bool defusingInProgress = false;
	const float distanceToBomb = SquaredF(140.0f);

	for (const auto& client : g_clients)
	{
		if (FNullEnt(client.ent))
			continue;

		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE))
			continue;

		Bot* bot = g_botManager->GetBot(client.index);

		const float bombDistance = (client.ent->v.origin - bombOrigin).GetLengthSquared();
		if (bot != nullptr)
		{
			if (m_team != bot->m_team || bot->GetCurrentState() == Process::Escape || bot->GetCurrentState() == Process::Camp)
				continue; // skip other mess

			// if close enough, mark as progressing
			if (bombDistance < distanceToBomb && (bot->GetCurrentState() == Process::Defuse || bot->m_hasProgressBar))
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
			if (bombDistance < distanceToBomb && ((client.ent->v.button | client.ent->v.oldbuttons) & IN_USE))
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
	if (cstrncmp(weapon, "p228", 4))
		id = Weapon::P228;
	else if (cstrncmp(weapon, "shield", 6))
		id = Weapon::Shield;
	else if (cstrncmp(weapon, "scout", 5))
		id = Weapon::Scout;
	else if (cstrncmp(weapon, "hegren", 6))
		id = Weapon::HeGrenade;
	else if (cstrncmp(weapon, "xm1014", 6))
		id = Weapon::Xm1014;
	else if (cstrncmp(weapon, "mac10", 5))
		id = Weapon::Mac10;
	else if (cstrncmp(weapon, "aug", 3))
		id = Weapon::Aug;
	else if (cstrncmp(weapon, "sgren", 5))
		id = Weapon::SmGrenade;
	else if (cstrncmp(weapon, "elites", 6))
		id = Weapon::Elite;
	else if (cstrncmp(weapon, "fiveseven", 9))
		id = Weapon::FiveSeven;
	else if (cstrncmp(weapon, "ump45", 5))
		id = Weapon::Ump45;
	else if (cstrncmp(weapon, "sg550", 5))
		id = Weapon::Sg550;
	else if (cstrncmp(weapon, "galil", 5))
		id = Weapon::Galil;
	else if (cstrncmp(weapon, "famas", 5))
		id = Weapon::Famas;
	else if (cstrncmp(weapon, "usp", 3))
		id = Weapon::Usp;
	else if (cstrncmp(weapon, "glock18", 7))
		id = Weapon::Glock18;
	else if (cstrncmp(weapon, "glock", 5))
		id = Weapon::Glock18;
	else if (cstrncmp(weapon, "awp", 3))
		id = Weapon::Awp;
	else if (cstrncmp(weapon, "mp5", 3))
		id = Weapon::Mp5;
	else if (cstrncmp(weapon, "m249", 4))
		id = Weapon::M249;
	else if (cstrncmp(weapon, "m3", 2))
		id = Weapon::M3;
	else if (cstrncmp(weapon, "m4a1", 4))
		id = Weapon::M4A1;
	else if (cstrncmp(weapon, "tmp", 3))
		id = Weapon::Tmp;
	else if (cstrncmp(weapon, "g3sg1", 5))
		id = Weapon::G3SG1;
	else if (cstrncmp(weapon, "flash", 5))
		id = Weapon::FbGrenade;
	else if (cstrncmp(weapon, "flashbang", 9))
		id = Weapon::FbGrenade;
	else if (cstrncmp(weapon, "deagle", 6))
		id = Weapon::Deagle;
	else if (cstrncmp(weapon, "sg552", 5))
		id = Weapon::Sg552;
	else if (cstrncmp(weapon, "ak47", 4))
		id = Weapon::Ak47;
	else if (cstrncmp(weapon, "p90", 3))
		id = Weapon::P90;
	else if (cstrncmp(weapon, "vesthelmet", 10))
		id = Weapon::KevlarHelmet;
	else if (cstrncmp(weapon, "vesthelm", 8))
		id = Weapon::KevlarHelmet;
	else if (cstrncmp(weapon, "vest", 4))
		id = Weapon::Kevlar;
	else if (cstrncmp(weapon, "defuser", 7))
		id = Weapon::Defuser;

	return id;
}