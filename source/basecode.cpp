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
ConVar ebot_avoid_grenades("ebot_avoid_grenades", "1");
ConVar ebot_breakable_health_limit("ebot_breakable_health_limit", "3000.0");

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
			if (bot != nullptr && bot != this)
			{
				if (m_isAlive == bot->m_isAlive)
				{
					bot->m_sayTextBuffer.entityIndex = m_index;
					cstrcpy(bot->m_sayTextBuffer.sayText, m_tempStrings);
				}
			}
		}
	}
	else if (message == CMENU_BUY && g_gameVersion == HALFLIFE)
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

	TraceResult tr;
	const Vector eyes = EyePosition();

	Vector spot = targetEntity->v.origin;
	edict_t* self = GetEntity();

	constexpr float vis = 0.9f;
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
	TraceResult tr;

	// trace a line from bot's eyes to destination..
	TraceLine(EyePosition(), destination, true, false, GetEntity(), &tr);

	// check if line of sight to object is not blocked (i.e. visible)
	if (tr.flFraction < 1.0f)
	{
		// check for standard items
		if (g_gameVersion == HALFLIFE)
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
	TraceResult tr;

	// trace a line from bot's eyes to destination...
	TraceLine(fromBody ? pev->origin - Vector(0.0f, 0.0f, 1.0f) : EyePosition(), dest, true, true, GetEntity(), &tr);

	// check if line of sight to object is not blocked (i.e. visible)
	return tr.flFraction >= 1.0f;
}

bool Bot::IsBehindSmokeClouds(edict_t* ent)
{
	// in zombie mode, flares are counted as smoke and breaks the bot's vision
	if (IsZombieMode())
		return false;

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
	int* ptr = g_weaponPrefs[m_personality];
	int weaponIndex = 0;
	int weapons = pev->weapons;

	const WeaponSelect* weaponTab = &g_weaponSelect[0];

	// take the shield in account
	if (HasShield())
		weapons |= (1 << WEAPON_SHIELDGUN);

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
	int* ptr = g_weaponPrefs[m_personality];
	int weaponIndex = 0;
	int weapons = pev->weapons;

	// take the shield in account
	if (HasShield())
		weapons |= (1 << WEAPON_SHIELDGUN);

	const WeaponSelect* weaponTab = &g_weaponSelect[0];

	for (int i = 0; i < Const_NumWeapons; i++)
	{
		const int id = weaponTab[*ptr].id;
		if ((weapons & (1 << static_cast<int>(weaponTab[*ptr].id))) && (id == WEAPON_USP || id == WEAPON_GLOCK18 || id == WEAPON_DEAGLE || id == WEAPON_P228 || id == WEAPON_ELITE || id == WEAPON_FN57))
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
	int* ptr = g_weaponPrefs[m_personality];

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

	if (IsZombieMode() && HasPrimaryWeapon() && m_currentWeapon != WEAPON_KNIFE)
		return false;

	return true;
}

void Bot::FindItem(void)
{
	if (!AllowPickupItem())
	{
		m_pickupItem = nullptr;
		m_pickupType = PICKTYPE_NONE;
		return;
	}

	edict_t* ent = nullptr;

	if (!FNullEnt(m_pickupItem))
	{
		while (!FNullEnt(ent = FIND_ENTITY_IN_SPHERE(ent, pev->origin, 512.0f)))
		{
			if (ent != m_pickupItem || ent->v.effects & EF_NODRAW || IsValidPlayer(ent->v.owner) || IsValidPlayer(ent))
				continue; // someone owns this weapon or it hasn't re spawned yet

			if (ItemIsVisible(GetEntityOrigin(ent), const_cast <char*> (STRING(ent->v.classname))))
				return;

			break;
		}
	}

	edict_t* pickupItem = nullptr;
	m_pickupItem = nullptr;
	m_pickupType = PICKTYPE_NONE;
	ent = nullptr;
	pickupItem = nullptr;

	PickupType pickupType = PICKTYPE_NONE;

	float minDistance = SquaredF(512.0f);

	while (!FNullEnt(ent = FIND_ENTITY_IN_SPHERE(ent, pev->origin, 512.0f)))
	{
		pickupType = PICKTYPE_NONE;
		if (ent->v.effects & EF_NODRAW || ent == m_itemIgnore || IsValidPlayer(ent) || ent == GetEntity()) // we can't pickup a player...
			continue;

		if (pev->health < pev->max_health && cstrncmp("item_healthkit", STRING(ent->v.classname), 14) == 0)
			pickupType = PICKTYPE_GETENTITY;
		else if (pev->health < pev->max_health && cstrncmp("func_healthcharger", STRING(ent->v.classname), 18) == 0 && ent->v.frame == 0)
		{
			const auto origin = GetEntityOrigin(ent);
			if ((pev->origin - origin).GetLengthSquared() <= SquaredF(128.0f))
			{
				if (g_gameVersion == CSVER_XASH)
					pev->button |= IN_USE;
				else
					MDLL_Use(ent, GetEntity());
			}

			m_lookAt = origin;
			pickupType = PICKTYPE_GETENTITY;
			m_moveToGoal = false;
			m_checkTerrain = false;
			m_moveSpeed = pev->maxspeed;
			m_strafeSpeed = 0.0f;
		}
		else if (pev->armorvalue < 100 && cstrncmp("item_battery", STRING(ent->v.classname), 12) == 0)
			pickupType = PICKTYPE_GETENTITY;
		else if (pev->armorvalue < 100 && cstrncmp("func_recharge", STRING(ent->v.classname), 13) == 0 && ent->v.frame == 0)
		{
			const auto origin = GetEntityOrigin(ent);
			if ((pev->origin - origin).GetLengthSquared() <= SquaredF(128.0f))
			{
				if (g_gameVersion == CSVER_XASH)
					pev->button |= IN_USE;
				else
					MDLL_Use(ent, GetEntity());
			}

			m_lookAt = origin;
			pickupType = PICKTYPE_GETENTITY;
			m_moveToGoal = false;
			m_checkTerrain = false;
			m_moveSpeed = pev->maxspeed;
			m_strafeSpeed = 0.0f;
		}
		else if (g_gameVersion == HALFLIFE)
		{
			if (m_currentWeapon != WEAPON_SNARK && cstrncmp("monster_snark", STRING(ent->v.classname), 13) == 0)
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
				pickupType = PICKTYPE_GETENTITY;
			else if (cstrncmp("ammo_", STRING(ent->v.classname), 5) == 0)
				pickupType = PICKTYPE_GETENTITY;
			else if (cstrncmp("weaponbox", STRING(ent->v.classname), 9) == 0)
				pickupType = PICKTYPE_GETENTITY;
		}
		else if (cstrncmp("hostage_entity", STRING(ent->v.classname), 14) == 0)
			pickupType = PICKTYPE_HOSTAGE;
		else if (cstrncmp("weaponbox", STRING(ent->v.classname), 9) == 0 && cstrcmp(STRING(ent->v.model) + 9, "backpack.mdl") == 0)
			pickupType = PICKTYPE_DROPPEDC4;
		else if (cstrncmp("weaponbox", STRING(ent->v.classname), 9) == 0 && cstrcmp(STRING(ent->v.model) + 9, "backpack.mdl") == 0 && !m_isUsingGrenade)
			pickupType = PICKTYPE_DROPPEDC4;
		else if ((cstrncmp("weaponbox", STRING(ent->v.classname), 9) == 0 || cstrncmp("armoury_entity", STRING(ent->v.classname), 14) == 0 || cstrncmp("csdm", STRING(ent->v.classname), 4) == 0) && !m_isUsingGrenade)
			pickupType = PICKTYPE_WEAPON;
		else if (cstrncmp("weapon_shield", STRING(ent->v.classname), 13) == 0 && !m_isUsingGrenade)
			pickupType = PICKTYPE_SHIELDGUN;
		else if (cstrncmp("item_thighpack", STRING(ent->v.classname), 14) == 0 && m_team == TEAM_COUNTER && !m_hasDefuser)
			pickupType = PICKTYPE_DEFUSEKIT;
		else if (cstrncmp("grenade", STRING(ent->v.classname), 7) == 0 && cstrcmp(STRING(ent->v.model) + 9, "c4.mdl") == 0)
			pickupType = PICKTYPE_PLANTEDC4;
		else
		{
			for (int i = 0; (i < entityNum && pickupType == PICKTYPE_NONE); i++)
			{
				if (g_entityId[i] == -1 || g_entityAction[i] != 3)
					continue;

				if (g_entityTeam[i] != 0 && m_team != g_entityTeam[i])
					continue;

				if (ent != INDEXENT(g_entityId[i]))
					continue;

				pickupType = PICKTYPE_GETENTITY;
			}
		}

		if (pickupType == PICKTYPE_NONE)
			continue;

		const Vector entityOrigin = GetEntityOrigin(ent);
		const float distance = (pev->origin - entityOrigin).GetLengthSquared();
		if (distance > minDistance)
			continue;

		if (!ItemIsVisible(entityOrigin, const_cast <char*> (STRING(ent->v.classname))))
		{
			if (cstrncmp("grenade", STRING(ent->v.classname), 7) != 0 || cstrcmp(STRING(ent->v.model) + 9, "c4.mdl") != 0)
				continue;

			if (distance > SquaredF(128.0f))
				continue;
		}

		bool allowPickup = true;
		if (pickupType == PICKTYPE_GETENTITY)
			allowPickup = true;
		else if (pickupType == PICKTYPE_WEAPON)
		{
			const int weaponCarried = GetBestWeaponCarried();
			const int secondaryWeaponCarried = GetBestSecondaryWeaponCarried();
			const int secondaryWeaponAmmoMax = g_weaponDefs[g_weaponSelect[secondaryWeaponCarried].id].ammo1Max;
			const int weaponAmmoMax = g_weaponDefs[g_weaponSelect[weaponCarried].id].ammo1Max;

			if (secondaryWeaponCarried < 7 && (m_ammo[g_weaponSelect[secondaryWeaponCarried].id] > 0.3 * secondaryWeaponAmmoMax) && cstrcmp(STRING(ent->v.model) + 9, "w_357ammobox.mdl") == 0)
				allowPickup = false;
			else if (!m_isVIP && weaponCarried >= 7 && (m_ammo[g_weaponSelect[weaponCarried].id] > 0.3 * weaponAmmoMax) && cstrncmp(STRING(ent->v.model) + 9, "w_", 2) == 0)
			{
				if (cstrcmp(STRING(ent->v.model) + 9, "w_9mmarclip.mdl") == 0 && !(weaponCarried == WEAPON_FAMAS || weaponCarried == WEAPON_AK47 || weaponCarried == WEAPON_M4A1 || weaponCarried == WEAPON_GALIL || weaponCarried == WEAPON_AUG || weaponCarried == WEAPON_SG552))
					allowPickup = false;
				else if (cstrcmp(STRING(ent->v.model) + 9, "w_shotbox.mdl") == 0 && !(weaponCarried == WEAPON_M3 || weaponCarried == WEAPON_XM1014))
					allowPickup = false;
				else if (cstrcmp(STRING(ent->v.model) + 9, "w_9mmclip.mdl") == 0 && !(weaponCarried == WEAPON_MP5 || weaponCarried == WEAPON_TMP || weaponCarried == WEAPON_P90 || weaponCarried == WEAPON_MAC10 || weaponCarried == WEAPON_UMP45))
					allowPickup = false;
				else if (cstrcmp(STRING(ent->v.model) + 9, "w_crossbow_clip.mdl") == 0 && !(weaponCarried == WEAPON_AWP || weaponCarried == WEAPON_G3SG1 || weaponCarried == WEAPON_SCOUT || weaponCarried == WEAPON_SG550))
					allowPickup = false;
				else if (cstrcmp(STRING(ent->v.model) + 9, "w_chainammo.mdl") == 0 && weaponCarried == WEAPON_M249)
					allowPickup = false;
			}
			else if (m_isVIP || !RateGroundWeapon(ent))
				allowPickup = false;
			else if (cstrcmp(STRING(ent->v.model) + 9, "medkit.mdl") == 0 && (pev->health >= 100))
				allowPickup = false;
			else if ((cstrcmp(STRING(ent->v.model) + 9, "kevlar.mdl") == 0 || cstrcmp(STRING(ent->v.model) + 9, "battery.mdl") == 0) && pev->armorvalue >= 100) // armor vest
				allowPickup = false;
			else if (cstrcmp(STRING(ent->v.model) + 9, "flashbang.mdl") == 0 && (pev->weapons & (1 << WEAPON_FBGRENADE))) // concussion grenade
				allowPickup = false;
			else if (cstrcmp(STRING(ent->v.model) + 9, "hegrenade.mdl") == 0 && (pev->weapons & (1 << WEAPON_HEGRENADE))) // explosive grenade
				allowPickup = false;
			else if (cstrcmp(STRING(ent->v.model) + 9, "smokegrenade.mdl") == 0 && (pev->weapons & (1 << WEAPON_SMGRENADE))) // smoke grenade
				allowPickup = false;
		}
		else if (pickupType == PICKTYPE_SHIELDGUN)
		{
			if ((pev->weapons & (1 << WEAPON_ELITE)) || HasShield() || m_isVIP || (HasPrimaryWeapon() && !RateGroundWeapon(ent)))
				allowPickup = false;
		}
		else if (m_team != TEAM_TERRORIST && m_team != TEAM_COUNTER)
			allowPickup = false;
		else if (m_team == TEAM_TERRORIST)
		{
			if (pickupType == PICKTYPE_DROPPEDC4)
			{
				allowPickup = true;
				m_destOrigin = entityOrigin;
			}
			else if (pickupType == PICKTYPE_HOSTAGE)
			{
				m_itemIgnore = ent;
				allowPickup = false;
			}
			else if (pickupType == PICKTYPE_PLANTEDC4)
			{
				allowPickup = false;

				if (!m_defendedBomb)
				{
					m_defendedBomb = true;

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
		}
		else if (m_team == TEAM_COUNTER)
		{
			if (pickupType == PICKTYPE_HOSTAGE)
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
			else if (pickupType == PICKTYPE_DROPPEDC4)
			{
				m_itemIgnore = ent;
				allowPickup = false;

				if (m_personality != PERSONALITY_RUSHER && (m_personality != PERSONALITY_NORMAL || m_skill > 49))
				{
					const int index = FindDefendWaypoint(entityOrigin);
					if (IsValidWaypoint(index))
					{
						m_campIndex = index;
						SetProcess(Process::Camp, "i will defend the dropped bomb", true, AddTime(ebot_camp_max.GetFloat()));
					}
				}
			}
			else if (pickupType == PICKTYPE_PLANTEDC4)
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
				if (bot != nullptr && bot != this && bot->m_isAlive && bot->m_pickupItem == pickupItem && bot->m_team == m_team)
				{
					m_pickupItem = nullptr;
					m_pickupType = PICKTYPE_NONE;
					return;
				}
			}
		}

		const Vector pickupOrigin = GetEntityOrigin(pickupItem);
		if (pickupOrigin == nullvec || pickupOrigin.z > EyePosition().z + 12.0f || IsDeadlyDrop(pickupOrigin))
		{
			m_pickupItem = nullptr;
			m_pickupType = PICKTYPE_NONE;
			return;
		}

		m_pickupItem = pickupItem;
	}
}

// this function inserts the radio message into the message queue
void Bot::RadioMessage(const int message)
{
	if (g_gameVersion == HALFLIFE)
		return;

	if (ebot_use_radio.GetInt() <= 0)
		return;

	if (m_radiotimer > engine->GetTime())
		return;

	if (m_numFriendsLeft <= 0)
		return;

	if (m_numEnemiesLeft <= 0)
		return;

	if (GetGameMode() == MODE_DM)
		return;

	m_radioSelect = message;
	PushMessageQueue(CMENU_RADIO);
	m_radiotimer = engine->GetTime() + CRandomFloat(m_numFriendsLeft, m_numFriendsLeft * 1.5f);
}

// this function checks and executes pending messages
void Bot::CheckMessageQueue(void)
{
	if (m_actMessageIndex == m_pushMessageIndex)
		return;

	// get message from stack
	const auto state = GetMessageQueue();

	// nothing to do?
	if (state == CMENU_IDLE || (state == CMENU_RADIO && (GetGameMode() == MODE_DM || g_gameVersion == HALFLIFE)))
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

		m_nextBuyTime = engine->GetTime() + CRandomFloat(0.6f, 1.2f);

		// if freezetime is very low do not delay the buy process
		if (engine->GetFreezeTime() <= 1.0f)
			m_nextBuyTime = engine->GetTime() + CRandomFloat(0.2f, 0.4f);

		// if fun-mode no need to buy
		if (ebot_knifemode.GetBool() && (ebot_eco_rounds.GetInt() != 1 || HasPrimaryWeapon()))
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
		if ((g_mapType & MAP_ES) && m_team == TEAM_TERRORIST)
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
		if ((m_team == 0 || m_team == 1) && (g_lastRadioTime[m_team] + 3.0f < engine->GetTime()))
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
						if (bot != nullptr && bot != this && bot->m_team == m_team)
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
	if (g_weaponSelect[25].teamStandard != 1 || m_moneyAmount < 4000 || IsNullString(ebot_restrictweapons.GetString()))
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

	if (m_currentWeapon == WEAPON_SCOUT && m_moneyAmount > 5000)
		return true;
	else if (m_currentWeapon == WEAPON_MP5 && m_moneyAmount > 6000)
		return true;
	else if (m_currentWeapon == WEAPON_M3 || m_currentWeapon == WEAPON_XM1014 && m_moneyAmount > 4000)
		return true;

	return false;
}

void Bot::PerformWeaponPurchase(void)
{
	m_nextBuyTime = engine->GetTime();
	WeaponSelect* selectedWeapon = nullptr;

	int* ptr = g_weaponPrefs[m_personality] + Const_NumWeapons;

	switch (m_buyState)
	{
	case 0:
		if ((!HasShield() && !HasPrimaryWeapon()) && (g_botManager->EconomicsValid(m_team) || IsMorePowerfulWeaponCanBeBought()))
		{
			int gunMoney = 0, playerMoney = m_moneyAmount;
			int likeGunId[2] = { 0, 0 };
			int loadTime = 0;

			do
			{
				ptr--;
				gunMoney = 0;

				InternalAssert(*ptr > -1);
				InternalAssert(*ptr < Const_NumWeapons);

				selectedWeapon = &g_weaponSelect[*ptr];
				loadTime++;

				if (selectedWeapon->buyGroup == 1)
					continue;

				if ((g_mapType & MAP_AS) && selectedWeapon->teamAS != 2 && selectedWeapon->teamAS != m_team)
					continue;

				if ((g_gameVersion == CSVER_VERYOLD || g_gameVersion == CSVER_XASH) && selectedWeapon->buySelect == -1)
					continue;

				if (selectedWeapon->teamStandard != 2 && selectedWeapon->teamStandard != m_team)
					continue;

				if (IsRestricted(selectedWeapon->id))
					continue;

				gunMoney = selectedWeapon->price;

				if (playerMoney <= gunMoney)
					continue;

				const int gunMode = BuyWeaponMode(selectedWeapon->id);

				if (playerMoney < gunMoney + (gunMode * 125))
					continue;

				if (likeGunId[0] == 0)
					likeGunId[0] = selectedWeapon->id;
				else
				{
					if (gunMode <= BuyWeaponMode(likeGunId[0]))
					{
						if ((BuyWeaponMode(likeGunId[1]) > BuyWeaponMode(likeGunId[0])) || (BuyWeaponMode(likeGunId[1]) == BuyWeaponMode(likeGunId[0]) && (CRandomInt(1, 2) == 2)))
							likeGunId[1] = likeGunId[0];

						likeGunId[0] = selectedWeapon->id;
					}
					else
					{
						if (likeGunId[1] != 0)
						{
							if (gunMode <= BuyWeaponMode(likeGunId[1]))
								likeGunId[1] = selectedWeapon->id;
						}
						else
							likeGunId[1] = selectedWeapon->id;
					}
				}
			} while (loadTime < Const_NumWeapons);

			if (likeGunId[0] != 0)
			{
				WeaponSelect* buyWeapon = &g_weaponSelect[0];
				int weaponId = likeGunId[0];
				if (likeGunId[1] != 0)
					weaponId = likeGunId[(CRandomInt(1, 7) > 3) ? 0 : 1];

				for (int i = 0; i < Const_NumWeapons; i++)
				{
					if (buyWeapon[i].id == weaponId)
					{
						FakeClientCommand(GetEntity(), "buy;menuselect %d", buyWeapon[i].buyGroup);

						if (g_gameVersion == CSVER_VERYOLD || g_gameVersion == CSVER_XASH)
							FakeClientCommand(GetEntity(), "menuselect %d", buyWeapon[i].buySelect);
						else
						{
							if (m_team == TEAM_TERRORIST)
								FakeClientCommand(GetEntity(), "menuselect %d", buyWeapon[i].newBuySelectT);
							else
								FakeClientCommand(GetEntity(), "menuselect %d", buyWeapon[i].newBuySelectCT);
						}
					}
				}
			}
		}
		else if (HasPrimaryWeapon() && !HasShield())
			m_reloadState = ReloadState::Primary;

		break;

	case 1:
		if (pev->armorvalue < CRandomInt(40, 80) && (g_botManager->EconomicsValid(m_team) || HasPrimaryWeapon()))
		{
			if (m_moneyAmount > 1600 && !IsRestricted(WEAPON_KEVHELM))
				FakeClientCommand(GetEntity(), "buyequip;menuselect 2");
			else
				FakeClientCommand(GetEntity(), "buyequip;menuselect 1");
		}
		break;

	case 2:
		if ((HasPrimaryWeapon() && m_moneyAmount > CRandomInt(6000, 9000)))
		{
			int likeGunId = 0;
			int loadTime = 0;
			do
			{
				ptr--;

				InternalAssert(*ptr > -1);
				InternalAssert(*ptr < Const_NumWeapons);

				selectedWeapon = &g_weaponSelect[*ptr];
				loadTime++;

				if (selectedWeapon->buyGroup != 1)
					continue;

				if ((g_mapType & MAP_AS) && selectedWeapon->teamAS != 2 && selectedWeapon->teamAS != m_team)
					continue;

				if ((g_gameVersion == CSVER_VERYOLD || g_gameVersion == CSVER_XASH) && selectedWeapon->buySelect == -1)
					continue;

				if (selectedWeapon->teamStandard != 2 && selectedWeapon->teamStandard != m_team)
					continue;

				if (IsRestricted(selectedWeapon->id))
					continue;

				if (m_moneyAmount <= (selectedWeapon->price + 125))
					continue;

				const int gunMode = BuyWeaponMode(selectedWeapon->id);

				if (likeGunId == 0)
				{
					if ((pev->weapons & ((1 << WEAPON_USP) | (1 << WEAPON_GLOCK18))))
					{
						if (gunMode < 2)
							likeGunId = selectedWeapon->id;
					}
					else
						likeGunId = selectedWeapon->id;
				}
				else
				{
					if (gunMode < BuyWeaponMode(likeGunId))
						likeGunId = selectedWeapon->id;
				}
			} while (loadTime < Const_NumWeapons);

			if (likeGunId != 0)
			{
				const WeaponSelect* buyWeapon = &g_weaponSelect[0];
				for (int i = 0; i < Const_NumWeapons; i++)
				{
					if (buyWeapon[i].id == likeGunId)
					{
						FakeClientCommand(GetEntity(), "buy;menuselect %d", buyWeapon[i].buyGroup);

						if (g_gameVersion == CSVER_VERYOLD || g_gameVersion == CSVER_XASH)
							FakeClientCommand(GetEntity(), "menuselect %d", buyWeapon[i].buySelect);
						else
						{
							if (m_team == TEAM_TERRORIST)
								FakeClientCommand(GetEntity(), "menuselect %d", buyWeapon[i].newBuySelectT);
							else
								FakeClientCommand(GetEntity(), "menuselect %d", buyWeapon[i].newBuySelectCT);
						}
					}
				}
			}
		}
		break;

	case 3:
		if (!HasPrimaryWeapon() && !ChanceOf(m_skill) && !IsRestricted(WEAPON_SHIELDGUN))
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
		if (ChanceOf(m_skill) && !IsRestricted(WEAPON_HEGRENADE))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 4");
		}

		if (ChanceOf(m_skill) && g_botManager->EconomicsValid(m_team) && !IsRestricted(WEAPON_FBGRENADE))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 3");
		}

		if (ChanceOf(m_skill) && g_botManager->EconomicsValid(m_team) && !IsRestricted(WEAPON_FBGRENADE))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 3");
		}

		if (ChanceOf(m_skill) && g_botManager->EconomicsValid(m_team) && !IsRestricted(WEAPON_SMGRENADE))
		{
			FakeClientCommand(GetEntity(), "buyequip");
			FakeClientCommand(GetEntity(), "menuselect 5");
		}

		break;

	case 5:
		if ((g_mapType & MAP_DE) && m_team == TEAM_COUNTER && ChanceOf(m_skill) && m_moneyAmount > 200 && !IsRestricted(WEAPON_DEFUSER))
		{
			if (g_gameVersion == CSVER_VERYOLD || g_gameVersion == CSVER_XASH)
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
	case WEAPON_SHIELDGUN:
		gunMode = 8;
		break;

	case WEAPON_TMP:
	case WEAPON_UMP45:
	case WEAPON_P90:
	case WEAPON_MAC10:
	case WEAPON_SCOUT:
	case WEAPON_M3:
	case WEAPON_M249:
	case WEAPON_FN57:
	case WEAPON_P228:
		gunMode = 5;
		break;

	case WEAPON_XM1014:
	case WEAPON_G3SG1:
	case WEAPON_SG550:
	case WEAPON_GALIL:
	case WEAPON_ELITE:
	case WEAPON_SG552:
	case WEAPON_AUG:
		gunMode = 4;
		break;

	case WEAPON_MP5:
	case WEAPON_FAMAS:
	case WEAPON_USP:
	case WEAPON_GLOCK18:
		gunMode = 3;
		break;

	case WEAPON_AWP:
	case WEAPON_DEAGLE:
		gunMode = 2;
		break;

	case WEAPON_AK47:
	case WEAPON_M4A1:
		gunMode = 1;
		break;
	}

	return gunMode;
}

void Bot::CheckGrenadeThrow(void)
{
	const int grenadeToThrow = CheckGrenades();
	if (grenadeToThrow == -1 || FNullEnt(m_nearestEnemy))
		return;

	const Vector targetOrigin = m_enemyOrigin;
	if ((grenadeToThrow == WEAPON_HEGRENADE || grenadeToThrow == WEAPON_SMGRENADE))
	{
		float distance = (targetOrigin - pev->origin).GetLengthSquared();

		// is enemy to high to throw
		if ((targetOrigin.z > (pev->origin.z + 650.0f)) || !(m_nearestEnemy->v.flags & (FL_PARTIALGROUND | FL_DUCKING)))
			distance = FLT_MAX; // just some crazy value

		// enemy is within a good throwing distance ?
		if (distance > (grenadeToThrow == WEAPON_SMGRENADE ? SquaredF(400.0f) : SquaredF(600.0f)) && distance <= SquaredF(800.0f))
		{
			bool allowThrowing = true;

			if (allowThrowing)
			{
				const Vector enemyPredict = (m_nearestEnemy->v.velocity * 0.5f).SkipZ() + targetOrigin;
				int searchTab[4], count = 4;

				float searchRadius = m_nearestEnemy->v.velocity.GetLength2D();

				// check the search radius
				if (searchRadius < 128.0f)
					searchRadius = 128.0f;

				// search waypoints
				g_waypoint->FindInRadius(enemyPredict, searchRadius, searchTab, &count);

				while (count > 0)
				{
					allowThrowing = true;

					// check the throwing
					m_throw = g_waypoint->GetPath(searchTab[count--])->origin;
					Vector src = CheckThrow(EyePosition(), m_throw);

					if (src.GetLengthSquared() <= SquaredF(100.0f))
						src = CheckToss(EyePosition(), m_throw);

					if (src == nullvec)
						allowThrowing = false;
					else
						break;
				}
			}

			// start explosive grenade throwing?
			if (allowThrowing)
			{
				if (grenadeToThrow == WEAPON_HEGRENADE)
				{
					if (SetProcess(Process::ThrowHE, "throwing HE grenade", false, AddTime(10.0f)))
						return;
				}
				else
				{
					if (SetProcess(Process::ThrowSM, "throwing SM grenade", false, AddTime(10.0f)))
						return;
				}
			}
		}
	}
	else if (grenadeToThrow == WEAPON_FBGRENADE && (targetOrigin - pev->origin).GetLengthSquared() <= SquaredF(800.0f))
	{
		bool allowThrowing = true;
		Array <int> inRadius;

		g_waypoint->FindInRadius(inRadius, 256.0f, (targetOrigin + (m_nearestEnemy->v.velocity * 0.5f).SkipZ()));

		ITERATE_ARRAY(inRadius, i)
		{
			m_throw = g_waypoint->GetPath(i)->origin;
			Vector src = CheckThrow(EyePosition(), m_throw);

			if (src.GetLengthSquared() <= SquaredF(100.0f))
				src = CheckToss(EyePosition(), m_throw);

			if (src == nullvec)
				continue;

			allowThrowing = true;
			break;
		}

		// start concussion grenade throwing?
		if (allowThrowing)
			SetProcess(Process::ThrowFB, "throwing FB grenade", false, AddTime(10.0f));
	}
}

bool Bot::IsEnemyReachable(void)
{
	if (m_enemyReachableTimer > engine->GetTime())
		return m_isEnemyReachable;

	// NO!
	if (IsOnLadder())
	{
		m_isEnemyReachable = false;
		m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
		return m_isEnemyReachable;
	}

	if (m_currentTravelFlags & PATHFLAG_JUMP)
	{
		m_isEnemyReachable = false;
		m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
		return m_isEnemyReachable;
	}

	if (!m_hasEnemiesNear || FNullEnt(m_nearestEnemy))
	{
		m_isEnemyReachable = false;
		m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
		return m_isEnemyReachable;
	}

	if (!m_isEnemyReachable && (!(m_nearestEnemy->v.flags & FL_ONGROUND) || (m_nearestEnemy->v.flags & FL_PARTIALGROUND && !(m_nearestEnemy->v.flags & FL_DUCKING))))
	{
		m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
		return m_isEnemyReachable;
	}

	m_isEnemyReachable = false;

	const int ownIndex = IsValidWaypoint(m_currentWaypointIndex) ? m_currentWaypointIndex : (m_currentWaypointIndex = g_waypoint->FindNearest(pev->origin, 999999.0f, -1, GetEntity()));
	const int enemyIndex = g_waypoint->FindNearest(m_enemyOrigin, 999999.0f, -1, GetEntity());
	const auto currentWaypoint = g_waypoint->GetPath(ownIndex);

	if (m_isZombieBot)
	{
		if (ownIndex == enemyIndex)
		{
			m_isEnemyReachable = true;
			m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
			return m_isEnemyReachable;
		}
		else if (currentWaypoint->flags & WAYPOINT_FALLRISK || currentWaypoint->flags & WAYPOINT_ZOMBIEPUSH)
		{
			m_isEnemyReachable = false;
			m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
			return m_isEnemyReachable;
		}

		const float enemyDistance = (pev->origin - m_enemyOrigin).GetLengthSquared();

		if (pev->flags & FL_DUCKING)
		{
			if (enemyDistance < SquaredF(54.0f))
				m_isEnemyReachable = true;

			pev->speed = pev->maxspeed;
			m_moveSpeed = pev->maxspeed;

			m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
			return m_isEnemyReachable;
		}

		// end of the path, before repathing check the distance if we can reach to enemy
		if (!HasNextPath())
		{
			TraceResult tr;
			TraceHull(pev->origin, m_enemyOrigin, true, head_hull, GetEntity(), &tr);

			if (tr.flFraction == 1.0f || (enemyDistance < SquaredF(125.0f) && tr.pHit == m_nearestEnemy))
			{
				m_isEnemyReachable = true;
				m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
				return m_isEnemyReachable;
			}
		}
		else
		{
			float radius = pev->maxspeed;
			if (!(currentWaypoint->flags & WAYPOINT_FALLCHECK))
				radius += static_cast<float>(currentWaypoint->radius * 4);

			if (enemyDistance < SquaredF(radius))
			{
				TraceResult tr;
				TraceHull(pev->origin, m_enemyOrigin, true, head_hull, GetEntity(), &tr);

				if (tr.flFraction == 1.0f)
				{
					m_isEnemyReachable = true;
					m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
					return m_isEnemyReachable;
				}
			}
		}
	}
	else
	{
		if (enemyIndex == ownIndex)
		{
			m_isEnemyReachable = true;
			m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
			return m_isEnemyReachable;
		}
		else if (currentWaypoint->flags & WAYPOINT_FALLRISK)
		{
			m_isEnemyReachable = false;
			m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
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
				m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
				return m_isEnemyReachable;
			}
		}
		else if (GetProcess() == Process::Camp)
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

						const auto enemyWaypoint = g_waypoint->GetPath(enemyIndex);
						if (tr.flFraction == 1.0f)
						{
							m_isEnemyReachable = true;
							m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
							return m_isEnemyReachable;
						}
					}
				}
			}

			m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
			return m_isEnemyReachable;
		}
		else if (enemyDistance < escapeDist)
		{
			TraceResult tr;
			TraceHull(pev->origin, m_enemyOrigin, true, head_hull, GetEntity(), &tr);

			if (tr.flFraction == 1.0f)
			{
				m_isEnemyReachable = true;
				m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
				return m_isEnemyReachable;
			}
		}
	}

	m_enemyReachableTimer = AddTime(CRandomFloat(0.33f, 0.66f));
	return m_isEnemyReachable;
}

void Bot::CheckRadioCommands(void)
{
	if (!m_isSlowThink)
		return;

	if (m_numFriendsLeft == 0)
		return;

	if (FNullEnt(m_radioEntity) || !IsAlive(m_radioEntity))
		return;

	// dynamic range :)
	// less bots = more teamwork required
	if (CRandomFloat(1.0f, 100.0f) <= 1.0f * m_numFriendsLeft * 0.33333333333f)
		return;

	float distance = (GetEntityOrigin(m_radioEntity) - pev->origin).GetLengthSquared();

	switch (m_radioOrder)
	{
	case Radio::FollowMe:
		RadioMessage(Radio::Negative);

		break;

	case Radio::StickTogether:
		RadioMessage(Radio::Negative);

		break;

	case Radio::CoverMe:
		RadioMessage(Radio::Negative);

		break;

	case Radio::HoldPosition:
		RadioMessage(Radio::Negative);

		break;

	case Radio::TakingFire:
		RadioMessage(Radio::Negative);

		break;

	case Radio::YouTakePoint:
		RadioMessage(Radio::Negative);

		break;

	case Radio::EnemySpotted:
		RadioMessage(Radio::Negative);

		break;

	case Radio::NeedBackup:
		RadioMessage(Radio::Negative);

		break;

	case Radio::GoGoGo:
		RadioMessage(Radio::Negative);

		break;

	case Radio::ShesGonnaBlow:
		RadioMessage(Radio::Negative);

		break;

	case Radio::RegroupTeam:
		RadioMessage(Radio::Negative);

		break;

	case Radio::StormTheFront:
		RadioMessage(Radio::Negative);

		break;

	case Radio::Fallback:
		RadioMessage(Radio::Negative);

		break;

	case Radio::ReportTeam:
		/*switch (GetCurrentTask()->taskID)
		{
		case TASK_NORMAL:
			if (IsValidWaypoint(GetCurrentTask()->data))
			{
				if (!FNullEnt(m_enemy))
				{
					if (IsAlive(m_enemy))
						RadioMessage(Radio::EnemySpotted);
					else
						RadioMessage(Radio::EnemyDown);
				}
				else if (!FNullEnt(m_lastEnemy))
				{
					if (IsAlive(m_lastEnemy))
						RadioMessage(Radio::EnemySpotted);
					else
						RadioMessage(Radio::EnemyDown);
				}
				else if (m_seeEnemyTime + 10.0f > engine->GetTime())
					RadioMessage(Radio::EnemySpotted);
				else
					RadioMessage(Radio::SectorClear);
			}

			break;

		case TASK_MOVETOPOSITION:
			if (m_seeEnemyTime + 10.0f > engine->GetTime())
				RadioMessage(Radio::EnemySpotted);
			else if (FNullEnt(m_enemy) && FNullEnt(m_lastEnemy))
				RadioMessage(Radio::SectorClear);

			break;

		case TASK_CAMP:
			RadioMessage(Radio::InPosition);

			break;

		case TASK_GOINGFORCAMP:
			RadioMessage(Radio::HoldPosition);

			break;

		case TASK_PLANTBOMB:
			RadioMessage(Radio::HoldPosition);

			break;

		case TASK_DEFUSEBOMB:
			RadioMessage(Radio::CoverMe);

			break;

		case TASK_FIGHTENEMY:
			if (!FNullEnt(m_enemy))
			{
				if (IsAlive(m_enemy))
					RadioMessage(Radio::EnemySpotted);
				else
					RadioMessage(Radio::EnemyDown);
			}
			else if (m_seeEnemyTime + 10.0f > engine->GetTime())
				RadioMessage(Radio::EnemySpotted);

			break;

		default:
			if (ChanceOf(15))
				RadioMessage(Radio::ReportingIn);
			else if (ChanceOf(15))
				RadioMessage(Radio::FollowMe);
			else if (m_seeEnemyTime + 10.0f > engine->GetTime())
				RadioMessage(Radio::EnemySpotted);
			else
				RadioMessage(Radio::Negative);

			break;
		}*/

		break;

	case Radio::SectorClear:
		// is bomb planted and it's a ct
		RadioMessage(Radio::Negative);

		break;

	case Radio::GetInPosition:
		RadioMessage(Radio::Negative);

		break;
	}

	m_radioOrder = 0; // radio command has been handled, reset
}

void Bot::SetWalkTime(const float time)
{
	if (g_gameVersion == HALFLIFE)
		return;

	if (!ebot_walkallow.GetBool())
		return;

	if (IsZombieMode())
		return;

	if (m_numEnemiesLeft <= 0)
		return;

	if (!m_buyingFinished)
		return;

	if ((m_spawnTime + engine->GetFreezeTime() + 7.0f) > engine->GetTime())
		return;

	m_walkTime = AddTime(time + 1.0f - (pev->health / pev->max_health));
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
	const byte adjustedMSec = static_cast<byte>(cminf(250.0f, (engine->GetTime() - m_msecInterval) * 1000.0f));
	m_msecInterval = engine->GetTime();
	PLAYER_RUN_MOVE(GetEntity(), m_moveAngles, m_moveSpeed, m_strafeSpeed, 0.0f, static_cast <unsigned short> (pev->button), pev->impulse, adjustedMSec);

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
		if (m_slowthinktimer < engine->GetTime())
		{
			m_isSlowThink = true;
			CheckSlowThink();
			if (m_slowthinktimer < engine->GetTime())
				m_slowthinktimer = AddTime(CRandomFloat(0.9f, 1.1f));
		}
		else
			m_isSlowThink = false;

		if (!m_isAlive)
		{
			if (!g_isFakeCommand)
			{
				extern ConVar ebot_random_join_quit;
				if (ebot_random_join_quit.GetBool() && m_stayTime > 0.0f && m_stayTime < engine->GetTime())
				{
					Kick();
					return;
				}

				extern ConVar ebot_chat;
				if (ebot_chat.GetBool() && !RepliesToPlayer() && m_lastChatTime + 10.0f < engine->GetTime() && g_lastChatTime + 5.0f < engine->GetTime()) // bot chatting turned on?
				{
					m_lastChatTime = engine->GetTime();
					if (ChanceOf(ebot_chat_percent.GetInt()) && !g_chatFactory[CHAT_DEAD].IsEmpty())
					{
						g_lastChatTime = engine->GetTime();

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
							m_sayTextBuffer.lastUsedSentences.RemoveAll();
					}
				}

				if (g_gameVersion == HALFLIFE && !(pev->oldbuttons & IN_ATTACK))
					pev->button |= IN_ATTACK;
			}

			m_aimInterval = engine->GetTime();
		}
		else
		{
			if (g_gameVersion == HALFLIFE)
			{
				// idk why ???
				if (pev->maxspeed <= 10.0f)
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
				FacePosition();
				DebugModeMsg();
			}
			else
				m_aimInterval = engine->GetTime();
		}
	}

	// check for pending messages
	CheckMessageQueue();

	// avoid frame drops
	m_frameInterval = engine->GetTime() - m_frameDelay;
	m_frameDelay = engine->GetTime();
}

void Bot::CheckSlowThink(void)
{
	CalculatePing();

	if (m_stayTime <= 0.0f)
	{
		extern ConVar ebot_random_join_quit;
		if (ebot_random_join_quit.GetBool())
		{
			extern ConVar ebot_stay_min;
			extern ConVar ebot_stay_max;
			m_stayTime = AddTime(CRandomFloat(ebot_stay_min.GetFloat(), ebot_stay_max.GetFloat()));
		}
		else
			m_stayTime = AddTime(999999.0f);
	}

	edict_t* ent = GetEntity();
	m_isZombieBot = IsZombieEntity(ent);
	m_team = GetTeam(ent);
	m_isAlive = IsAlive(ent);

	if (m_isZombieBot)
	{
		m_isBomber = false;
		SelectKnife();
	}
	else
	{
		if (pev->weapons & (1 << WEAPON_C4))
			m_isBomber = true;
		else
			m_isBomber = false;
	}

	extern ConVar ebot_ignore_enemies;
	if (!ebot_ignore_enemies.GetBool() && m_randomattacktimer < engine->GetTime() && !engine->IsFriendlyFireOn() && !HasHostage()) // simulate players with random knife attacks
	{
		if (m_isStuck && m_personality != PERSONALITY_CAREFUL)
		{
			if (m_personality == PERSONALITY_RUSHER)
				m_randomattacktimer = 0.0f;
			else
				m_randomattacktimer = AddTime(CRandomFloat(0.1f, 10.0f));
		}
		else if (m_personality == PERSONALITY_RUSHER)
			m_randomattacktimer = AddTime(CRandomFloat(0.1f, 30.0f));
		else if (m_personality == PERSONALITY_CAREFUL)
			m_randomattacktimer = AddTime(CRandomFloat(10.0f, 100.0f));
		else
			m_randomattacktimer = AddTime(CRandomFloat(0.15f, 75.0f));

		if (m_currentWeapon == WEAPON_KNIFE)
		{
			if (CRandomInt(1, 3) == 1)
				pev->button |= IN_ATTACK;
			else
				pev->button |= IN_ATTACK2;
		}
	}
}

bool Bot::IsAttacking(const edict_t* player)
{
	return (player->v.button & IN_ATTACK || player->v.oldbuttons & IN_ATTACK);
}

void Bot::UpdateLooking(void)
{
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
		if (m_enemySeeTime + val > engine->GetTime())
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
	if (m_enemyDistance <= m_entityDistance)
		m_lookAt = GetEnemyPosition();
	else
		m_lookAt = m_entityOrigin;
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
		if ((pev->origin - origin).GetLengthSquared() <= SquaredF(80.0f))
		{
			if (g_gameVersion == CSVER_XASH)
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
					MDLL_Use(button, GetEntity());
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
			/*if (IsValidWaypoint(bot->m_chosenGoalIndex) && m_currentWaypointIndex != bot->m_chosenGoalIndex)
			{
				m_prevGoalIndex = m_chosenGoalIndex;
				m_chosenGoalIndex = bot->m_chosenGoalIndex;
				DeleteSearchNodes();
				bot->SetProcess(Process::Pause, "waiting my friend", true, (pev->origin - m_friendOrigin).GetLength2D() / m_nearestFriend->v.maxspeed);
			}*/
		}
		else
		{
			TraceResult tr;
			const auto eyePosition = m_nearestFriend->v.origin + m_nearestFriend->v.view_ofs;
			MakeVectors(m_nearestFriend->v.angles);
			TraceLine(eyePosition, eyePosition + g_pGlobals->v_forward * 2000.0f, false, false, m_nearestFriend, &tr);

			if (tr.pHit)
			{
				if (tr.pHit != GetEntity())
					m_lookAt = GetEntityOrigin(tr.pHit);
			}
			else
				m_lookAt = tr.vecEndPos;
		}

		m_pauseTime = AddTime(CRandomFloat(2.0f, 5.0f));
		SetWalkTime(7.0f);

		if (m_currentWeapon == WEAPON_KNIFE)
			SelectBestWeapon();

		return;
	}

	if (m_pauseTime > engine->GetTime())
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

			if ((pev->origin - client.ent->v.origin).GetLengthSquared() > SquaredF(maxDist))
				continue;

			Vector playerArea;
			if (m_isZombieBot)
			{
				playerArea = client.ent->v.origin + client.ent->v.view_ofs;
				if (!m_hasEnemiesNear && (FNullEnt(m_nearestEnemy) || GetTeam(m_nearestEnemy) == m_team))
				{
					m_nearestEnemy = client.ent;
					DeleteSearchNodes();
				}
			}
			else
				playerArea = client.ent->v.origin + client.ent->v.view_ofs;

			const int skill = m_personality == PERSONALITY_RUSHER ? 30 : 10;
			if (m_skill > skill)
			{
				SetWalkTime(7.0f);
				SelectBestWeapon();
			}

			m_pauseTime = AddTime(CRandomFloat(2.0f, 5.0f));
			m_lookAt = playerArea;

			return;
		}
	}

	Vector selectRandom;
	if (m_searchTime < engine->GetTime())
	{
		Vector bestLookPos = EyePosition();
		bestLookPos.x += CRandomFloat(-1024.0f, 1024.0f);
		bestLookPos.y += CRandomFloat(-1024.0f, 1024.0f);
		bestLookPos.z += CRandomFloat(-256.0f, 256.0f);

		const int index = g_waypoint->FindNearestInCircle(bestLookPos, 999999.0f);
		if (IsValidWaypoint(index))
		{
			const Path* pointer = g_waypoint->GetPath(index);
			const Vector waypointOrigin = pointer->origin;
			const float waypointRadius = static_cast<float>(pointer->radius);
			selectRandom.x = waypointOrigin.x + CRandomFloat(-waypointRadius, waypointRadius);
			selectRandom.y = waypointOrigin.y + CRandomFloat(-waypointRadius, waypointRadius);
			selectRandom.z = waypointOrigin.z + 36.0f;
		}

		m_searchTime = AddTime(0.25f);
	}

	auto isVisible = [&](void)
	{
		TraceResult tr;
		TraceLine(EyePosition(), selectRandom, true, true, GetEntity(), &tr);
		if (tr.flFraction == 1.0f)
			return true;

		return false;
	};

	if ((pev->origin - selectRandom).GetLengthSquared() > SquaredF(512.0f) && isVisible())
	{
		if (m_pauseTime < engine->GetTime())
		{
			const Vector eyePosition = EyePosition();
			m_lookAt.x = selectRandom.x + ((selectRandom.x - eyePosition.x) * 256.0f);
			m_lookAt.y = selectRandom.y + ((selectRandom.y - eyePosition.y) * 256.0f);
			m_lookAt.z = selectRandom.z + 36.0f;
			m_pauseTime = AddTime(CRandomFloat(1.0f, 2.0f));
		}
	}
	else if (m_isSlowThink && pev->velocity.GetLengthSquared2D() > SquaredF(24.0f))
	{
		const int index = g_waypoint->FindNearestInCircle(m_destOrigin, 999999.0f);
		if (IsValidWaypoint(index))
		{
			const Vector eyePosition = EyePosition();
			const Path* pointer = g_waypoint->GetPath(index);
			const Vector waypointOrigin = pointer->origin;
			const float waypointRadius = static_cast<float>(pointer->radius);
			m_lookAt.x = waypointOrigin.x + CRandomFloat(-waypointRadius, waypointRadius) + ((m_destOrigin.x - eyePosition.x) * 1024.0f);
			m_lookAt.y = waypointOrigin.y + CRandomFloat(-waypointRadius, waypointRadius) + ((m_destOrigin.y - eyePosition.y) * 1024.0f);
			m_lookAt.z = waypointOrigin.z + 36.0f;
		}
	}
}

void Bot::SecondThink(void)
{
	if (g_gameVersion != HALFLIFE)
	{
		// Don't forget me :(
		/*if (ebot_use_flare.GetBool() && !m_isReloading && !m_isZombieBot && GetGameMode() == MODE_ZP && FNullEnt(m_enemy) && !FNullEnt(m_lastEnemy))
		{
			if (pev->weapons & (1 << WEAPON_SMGRENADE) && ChanceOf(40))
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

	if (m_voteMap != 0 && !g_isFakeCommand) // host wants the bots to vote for a map?
	{
		FakeClientCommand(GetEntity(), "votemap %d", m_voteMap);
		m_voteMap = 0;
	}
}

void Bot::CalculatePing(void)
{
	if (g_gameVersion == CSVER_VERYOLD)
		return;

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

	if (botPing <= 9)
		botPing = CRandomInt(9, 19);
	else if (botPing > 133)
		botPing = CRandomInt(99, 119);

	for (int j = 0; j < 2; j++)
	{
		for (m_pingOffset[j] = 0; m_pingOffset[j] < 4; m_pingOffset[j]++)
		{
			if ((botPing - m_pingOffset[j]) % 4 == 0)
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
	if (specIndex != m_index)
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
		sprintf(botType, "%s%s%s", m_personality == PERSONALITY_RUSHER ? "Rusher" : "",
			m_personality == PERSONALITY_CAREFUL ? "Careful" : "",
			m_personality == PERSONALITY_NORMAL ? "Normal" : "");

		if (weaponCount >= Const_NumWeapons)
		{
			// prevent printing unknown message from known weapons
			switch (m_currentWeapon)
			{
			case WEAPON_HEGRENADE:
				sprintf(weaponName, "weapon_hegrenade");
				break;

			case WEAPON_FBGRENADE:
				sprintf(weaponName, "weapon_flashbang");
				break;

			case WEAPON_SMGRENADE:
				sprintf(weaponName, "weapon_smokegrenade");
				break;

			case WEAPON_C4:
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
		case MODE_BASE:
			sprintf(gamemodName, "Normal");
			break;

		case MODE_DM:
			sprintf(gamemodName, "Deathmatch");
			break;

		case MODE_ZP:
			sprintf(gamemodName, "Zombie Mode");
			break;

		case MODE_NOTEAM:
			sprintf(gamemodName, "No Team");
			break;

		case MODE_ZH:
			sprintf(gamemodName, "Zombie Hell");
			break;

		case MODE_TDM:
			sprintf(gamemodName, "Team Deathmatch");
			break;

		default:
			sprintf(gamemodName, "UNKNOWN MODE");
		}

		PathNode* navid = &m_navNode[0];
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

			navid = navid->next;
		}

		const int client = m_index - 1;

		char outputBuffer[512];
		sprintf(outputBuffer, "\n\n\n\n\n\n\n Game Mode: %s"
			"\n [%s] \n Process: %s  RE-Process: %s \n"
			"Weapon: %s  Clip: %d  Ammo: %d \n"
			"Type: %s  Money: %d  Heuristic: %s \n"
			"Enemy: %s  Friend: %s \n\n"

			"Current Index: %d  Goal Index: %d  Prev Goal Index: %d \n"
			"Nav: %d  Next Nav: %d \n"
			"GEWI: %d  GEWI2: %d \n"
			"Move Speed: %2.f  Strafe Speed: %2.f \n "
			"Stuck Warnings: %d  Stuck: %s \n",
			gamemodName,
			GetEntityName(GetEntity()), processName, rememberedProcessName,
			&weaponName[7], GetAmmoInClip(), GetAmmo(),
			botType, m_moneyAmount, GetHeuristicName(),
			enemyName, friendName,

			m_currentWaypointIndex, goal, m_prevGoalIndex,
			navIndex[0], navIndex[1],
			g_clients[client].wpIndex, g_clients[client].wpIndex2,
			m_moveSpeed, m_strafeSpeed,
			m_stuckWarn, m_isStuck ? "Yes" : "No");

		MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, nullptr, g_hostEntity);
		WRITE_BYTE(TE_TEXTMESSAGE);
		WRITE_BYTE(1);
		WRITE_SHORT(FixedSigned16(-1, 1 << 13));
		WRITE_SHORT(FixedSigned16(0, 1 << 13));
		WRITE_BYTE(0);
		WRITE_BYTE(m_team == TEAM_COUNTER ? 0 : 255);
		WRITE_BYTE(100);
		WRITE_BYTE(m_team != TEAM_COUNTER ? 0 : 255);
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

		timeDebugUpdate = AddTime(1.0f);
	}

	if (m_hasEnemiesNear && m_enemyOrigin != nullvec)
		engine->DrawLine(g_hostEntity, EyePosition(), m_enemyOrigin, Color(255, 0, 0, 255), 10, 0, 5, 1, LINE_SIMPLE);

	if (m_destOrigin != nullvec)
		engine->DrawLine(g_hostEntity, pev->origin, m_destOrigin, Color(0, 0, 255, 255), 10, 0, 5, 1, LINE_SIMPLE);

	if (m_stuckArea != nullvec && m_stuckWarn > 0)
		engine->DrawLine(g_hostEntity, pev->origin, m_stuckArea, Color(255, 0, 0, 255), 10, 0, 5, 1, LINE_SIMPLE);

	// now draw line from source to destination
	PathNode* node = &m_navNode[0];

	Vector src = nullvec;
	while (node != nullptr)
	{
		Path* path = g_waypoint->GetPath(node->index);
		src = path->origin;
		node = node->next;

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
	for (auto hostage : m_hostages)
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
	if (g_weaponDefs[m_currentWeapon].ammo1 == -1)
		return 0;

	return m_ammo[g_weaponDefs[m_currentWeapon].ammo1];
}

// this function gets called by network message handler, when screenfade message get's send
// it's used to make bot blind froumd the grenade
void Bot::TakeBlinded(const Vector fade, const int alpha)
{
	if (fade.x != 255 || fade.y != 255 || fade.z != 255 || alpha <= 170)
		return;

	// TODO: fix me
}

// this function, asks bot to discard his current primary weapon (or c4) to the user that requsted it with /drop*
// command, very useful, when i don't have money to buy anything... )
void Bot::DiscardWeaponForUser(edict_t* user, const bool discardC4)
{
	if (FNullEnt(user))
		return;

	const Vector userOrigin = GetEntityOrigin(user);
	if (IsAlive(user) && m_moneyAmount >= 2000 && HasPrimaryWeapon() && (userOrigin - pev->origin).GetLengthSquared() <= SquaredF(240.0f))
	{
		m_lookAt = userOrigin;

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
		m_pickupType = PICKTYPE_NONE;
		m_itemCheckTime = engine->GetTime() + 5.0f;

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
	m_duckForJump = 0.0f;
	m_doubleJumpOrigin = nullvec;
	m_travelStartIndex = -1;
	m_jumpReady = false;
}

// this function returns the velocity at which an object should looped from start to land near end.
// returns null vector if toss is not feasible
Vector Bot::CheckToss(const Vector& start, Vector end)
{
	TraceResult tr;

	end = end - pev->velocity;
	end.z -= 15.0f;

	if (cabsf(end.z - start.z) > 500.0f)
		return nullvec;

	Vector midPoint = start + (end - start) * 0.5f;
	TraceHull(midPoint, midPoint + Vector(0.0f, 0.0f, 500.0f), true, head_hull, GetEntity(), &tr);

	if (tr.flFraction < 1.0f)
	{
		midPoint = tr.vecEndPos;
		if (!FNullEnt(tr.pHit))
			midPoint.z = tr.pHit->v.absmin.z - 1.0f;
	}

	if ((midPoint.z < start.z) || (midPoint.z < end.z))
		return nullvec;

	const float gravity = engine->GetGravity() * 0.5f;
	const float half = 0.5f * gravity;
	const float timeOne = csqrtf((midPoint.z - start.z) / half);
	const float timeTwo = csqrtf((midPoint.z - end.z) / half);

	if (timeOne < 0.1)
		return nullvec;

	Vector nadeVelocity = (end - start) / (timeOne + timeTwo);
	nadeVelocity.z = gravity * timeOne;

	Vector apex = start + nadeVelocity * timeOne;
	apex.z = midPoint.z;

	TraceHull(start, apex, false, head_hull, GetEntity(), &tr);

	if (tr.flFraction < 1.0f || tr.fAllSolid)
		return nullvec;

	TraceHull(end, apex, true, head_hull, GetEntity(), &tr);

	if (tr.flFraction != 1.0f)
	{
		const float dot = -(tr.vecPlaneNormal | (apex - end).Normalize());
		if (dot > 0.7f || tr.flFraction < 0.8f) // 60 degrees
			return nullvec;
	}

	return nadeVelocity * 0.777f;
}

// this function returns the velocity vector at which an object should be thrown from start to hit end returns null vector if throw is not feasible
Vector Bot::CheckThrow(const Vector& start, Vector end)
{
	Vector nadeVelocity = (end - start);
	TraceResult tr;
	
	float time = nadeVelocity.GetLengthSquared() / SquaredF(196.0f);
	if (time < 0.01f)
		return nullvec;
	else if (time > 2.0f)
		time = 1.2f;

	const float gravity = engine->GetGravity() * 0.5f;
	nadeVelocity = nadeVelocity * (1.0f / time);
	nadeVelocity.z += gravity * time * 0.5f;

	Vector apex = start + (end - start) * 0.5f;
	apex.z += 0.5f * gravity * (time * 0.5f) * (time * 0.5f);

	TraceLine(start, apex, true, false, GetEntity(), &tr);

	if (tr.flFraction != 1.0f)
		return nullvec;

	TraceLine(end, apex, true, false, GetEntity(), &tr);

	if (tr.flFraction != 1.0f || tr.fAllSolid)
	{
		const float dot = -(tr.vecPlaneNormal | (apex - end).Normalize());
		if (dot > 0.7f || tr.flFraction < 0.8f)
			return nullvec;
	}

	return nadeVelocity * 0.7793f;
}

// this function checks burst mode, and switch it depending distance to to enemy
void Bot::CheckBurstMode(const float distance)
{
	if (HasShield())
		return; // no checking when shiled is active

	// if current weapon is glock, disable burstmode on long distances, enable it else
	if (m_currentWeapon == WEAPON_GLOCK18 && distance < 300.0f && m_weaponBurstMode == BURST_DISABLED)
		pev->button |= IN_ATTACK2;
	else if (m_currentWeapon == WEAPON_GLOCK18 && distance >= 30.0f && m_weaponBurstMode == BURST_ENABLED)
		pev->button |= IN_ATTACK2;

	// if current weapon is famas, disable burstmode on short distances, enable it else
	if (m_currentWeapon == WEAPON_FAMAS && distance > 400.0f && m_weaponBurstMode == BURST_DISABLED)
		pev->button |= IN_ATTACK2;
	else if (m_currentWeapon == WEAPON_FAMAS && distance <= 400.0f && m_weaponBurstMode == BURST_ENABLED)
		pev->button |= IN_ATTACK2;
}

void Bot::CheckSilencer(void)
{
	if ((m_currentWeapon == WEAPON_USP && m_skill < 90) || m_currentWeapon == WEAPON_M4A1 && !HasShield())
	{
		const int random = (m_personality == PERSONALITY_RUSHER ? 33 : m_personality == PERSONALITY_CAREFUL ? 99 : 66);

		// aggressive bots don't like the silencer
		if (ChanceOf(m_currentWeapon == WEAPON_USP ? random / 3 : random))
		{
			if (pev->weaponanim > 6) // is the silencer not attached...
				pev->button |= IN_ATTACK2; // attach the silencer
		}
		else
		{
			if (pev->weaponanim <= 6) // is the silencer attached...
				pev->button |= IN_ATTACK2; // detach the silencer
		}
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
		if (estimatedTime > 8.0f)
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

	if (GetGameMode() == MODE_BASE)
	{
		if (m_isBomber || m_isVIP || m_isUsingGrenade)
			return false;

		if (!(g_mapType & MAP_DE))
		{
			if (m_numEnemiesLeft <= 1 || m_numFriendsLeft <= 0)
				return false;
		}
		else if (OutOfBombTimer())
			return false;

		if (m_team == TEAM_COUNTER)
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

		if (m_personality == PERSONALITY_RUSHER)
		{
			if (!FNullEnt(m_lastEnemy) && m_numFriendsLeft < m_numEnemiesLeft)
				return false;
			else if (FNullEnt(m_lastEnemy) && m_numFriendsLeft > m_numEnemiesLeft)
				return false;
		}
	}

	if (ebot_followuser.GetInt() > 0 && (m_radioOrder == Radio::FollowMe || GetProcess() == Process::Camp))
	{
		int numFollowers = 0;

		// check if no more followers are allowed
		for (const auto& bot : g_botManager->m_bots)
		{
			if (bot != nullptr && bot->m_isAlive)
			{
				if (bot->m_targetEntity == m_radioEntity)
					numFollowers++;
			}
		}

		// don't camp if bots following
		if (numFollowers > 0)
			return false;
	}

	return true;
}

bool Bot::OutOfBombTimer(void)
{
	if (!(g_mapType & MAP_DE))
		return false;

	if (!g_bombPlanted)
		return false;

	if (GetProcess() == Process::Defuse || m_hasProgressBar || GetProcess() == Process::Escape)
		return false; // if CT bot already start defusing, or already escaping, return false

	// calculate left time
	const float timeLeft = GetBombTimeleft();

	// if time left greater than 12, no need to do other checks
	if (timeLeft >= 12.0f)
		return false;
	else if (m_team == TEAM_TERRORIST)
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
			if (bot != nullptr && bot->m_team == TEAM_COUNTER && bot->m_hasDefuser && (bombOrigin - bot->pev->origin).GetLengthSquared() <= SquaredF(512.0f))
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

bool Bot::IsShootableBreakable(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	if ((FClassnameIs(ent, "func_breakable") || (FClassnameIs(ent, "func_pushable") && (ent->v.spawnflags & SF_PUSH_BREAKABLE)) || FClassnameIs(ent, "func_wall")) && ent->v.health <= ebot_breakable_health_limit.GetFloat())
	{
		if (ent->v.takedamage != DAMAGE_NO && ent->v.impulse <= 0 && !(ent->v.flags & FL_WORLDBRUSH) && !(ent->v.spawnflags & SF_BREAK_TRIGGER_ONLY))
			return (ent->v.movetype == MOVETYPE_PUSH || ent->v.movetype == MOVETYPE_PUSHSTEP);
	}

	return false;
}

// this function is gets called when bot enters a buyzone, to allow bot to buy some stuff
void Bot::EquipInBuyzone(const int iBuyCount)
{
	if (g_gameVersion == HALFLIFE)
		return;

	static float lastEquipTime = 0.0f;

	// if bot is in buy zone, try to buy ammo for this weapon...
	if (lastEquipTime + 15.0f < engine->GetTime() && m_inBuyZone && g_timeRoundStart + CRandomFloat(10.0f, 20.0f) + engine->GetBuyTime() < engine->GetTime() && !g_bombPlanted && m_moneyAmount > 800)
	{
		m_buyingFinished = false;
		m_buyState = iBuyCount;

		// push buy message
		PushMessageQueue(CMENU_BUY);

		m_nextBuyTime = engine->GetTime();
		lastEquipTime = engine->GetTime();
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
			if (m_team != bot->m_team || bot->GetProcess() == Process::Escape || bot->GetProcess() == Process::Camp)
				continue; // skip other mess

			// if close enough, mark as progressing
			if (bombDistance <= distanceToBomb && (bot->GetProcess() == Process::Defuse || bot->m_hasProgressBar))
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
			if (bombDistance <= distanceToBomb && ((client.ent->v.button | client.ent->v.oldbuttons) & IN_USE))
			{
				defusingInProgress = true;
				break;
			}

			continue;
		}
	}

	return defusingInProgress;
}