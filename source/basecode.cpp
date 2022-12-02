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

ConVar ebot_followuser("ebot_follow_user_max", "1");
ConVar ebot_knifemode("ebot_knife_mode", "0");
ConVar ebot_walkallow("ebot_walk_allow", "1");
ConVar ebot_stopbots("ebot_stop_bots", "0");
ConVar ebot_spraypaints("ebot_spray_paints", "1");
ConVar ebot_restrictweapons("ebot_restrict_weapons", "");
ConVar ebot_camp_min("ebot_camp_time_min", "16");
ConVar ebot_camp_max("ebot_camp_time_max", "48");
ConVar ebot_use_radio("ebot_use_radio", "1");
ConVar ebot_anti_block("ebot_anti_block", "0");
ConVar ebot_zm_dark_mode("ebot_zm_dark_mode", "0");
ConVar ebot_force_flashlight("ebot_force_flashlight", "0");
ConVar ebot_use_flare("ebot_zm_use_flares", "1");
ConVar ebot_chat_percent("ebot_chat_percent", "20");
ConVar ebot_eco_rounds("ebot_eco_rounds", "1");

ConVar ebot_chatter_path("ebot_chatter_path", "radio/bot");

// this function get the current message from the bots message queue
int Bot::GetMessageQueue(void)
{
	int message = m_messageQueue[m_actMessageIndex++];
	m_actMessageIndex &= 0x1f; // wraparound
	return message;
}

// this function put a message into the bot message queue
void Bot::PushMessageQueue(int message)
{
	if (message == CMENU_SAY)
	{
		// notify other bots of the spoken text otherwise, bots won't respond to other bots (network messages aren't sent from bots)
		int entityIndex = GetIndex();

		for (int i = 0; i < engine->GetMaxClients(); i++)
		{
			Bot* otherBot = g_botManager->GetBot(i);
			if (otherBot != nullptr && otherBot->pev != pev)
			{
				if (m_notKilled == IsAlive(otherBot->GetEntity()))
				{
					otherBot->m_sayTextBuffer.entityIndex = entityIndex;
					strcpy(otherBot->m_sayTextBuffer.sayText, m_tempStrings);
				}
			}
		}
	}

	m_messageQueue[m_pushMessageIndex++] = message;
	m_pushMessageIndex &= 0x1f; // wraparound
}

float Bot::InFieldOfView(Vector destination)
{
	float entityAngle = AngleMod(destination.ToYaw()); // find yaw angle from source to destination...
	float viewAngle = AngleMod(pev->v_angle.y); // get bot's current view angle...

	// return the absolute value of angle to destination entity
	// zero degrees means straight ahead, 45 degrees to the left or
	// 45 degrees to the right is the limit of the normal view angle
	float absoluteAngle = fabsf(viewAngle - entityAngle);

	if (absoluteAngle > 180.0f)
		absoluteAngle = 360.0f - absoluteAngle;
	
	return absoluteAngle;
}

bool Bot::IsInViewCone(Vector origin)
{
	return ::IsInViewCone(origin, GetEntity());
}

bool Bot::CheckVisibility(entvars_t* targetEntity, Vector* origin, uint8_t* bodyPart)
{
	TraceResult tr;
	*bodyPart = 0;

	Vector botHead = EyePosition();
	if (pev->flags & FL_DUCKING)
		botHead = AddVector(botHead, (VEC_HULL_MIN - VEC_DUCK_HULL_MIN));

	bool ignoreGlass = true;

	// zombies can't hit from the glass...
	if (m_isZombieBot || (m_currentWeapon == WEAPON_KNIFE && !HasPrimaryWeapon()))
		ignoreGlass = false;

	if (IsValidPlayer(ENT(targetEntity)))
	{
		Vector headOrigin = GetPlayerHeadOrigin(ENT(targetEntity));
		TraceLine(botHead, headOrigin, true, ignoreGlass, GetEntity(), &tr);
		if (tr.pHit == ENT(targetEntity) || tr.flFraction >= 1.0f)
		{
			*bodyPart |= VISIBILITY_HEAD;
			*bodyPart |= VISIBILITY_BODY;
			*origin = headOrigin;
			return (*bodyPart != 0);
		}
	}

	TraceLine(GetEntityOrigin(GetEntity()), GetEntityOrigin(ENT(targetEntity)), true, ignoreGlass, GetEntity(), &tr);
	if (tr.pHit == ENT(targetEntity) || tr.flFraction >= 1.0f)
	{
		*bodyPart |= VISIBILITY_BODY;
		*origin = tr.vecEndPos;
	}

	return (*bodyPart != 0);
}

bool Bot::IsEnemyViewable(edict_t* entity, bool setEnemy, bool checkOnly)
{
	if (FNullEnt(entity))
		return false;

	if (!IsAlive(entity))
		return false;

	if (IsNotAttackLab(entity))
		return false;

	if (m_currentWeapon != WEAPON_KNIFE && !IsZombieMode())
	{
		if (!IsInViewCone(GetEntityOrigin(entity)))
		{
			if ((!m_isZombieBot && GetGameMode() == MODE_ZP && ebot_zm_dark_mode.GetInt() == 1))
				return false;

			if (m_backCheckEnemyTime == 0.0f)
			{
				if (m_isZombieBot)
				{
					if (!FNullEnt(m_enemy))
						return false;

					if (FNullEnt(m_moveTargetEntity))
						return false;

					if (m_damageTime < engine->GetTime())
						m_backCheckEnemyTime = AddTime(engine->RandomFloat(0.5f, 1.5f));
				}
				else if (GetGameMode() == MODE_ZP)
					m_backCheckEnemyTime = AddTime(engine->RandomFloat(0.15f, 0.5f));
				else
				{
					float addTime = engine->RandomFloat(0.2f, 0.8f);
					if (m_skill == 100 && !FNullEnt(m_enemy))
						addTime = 0.2f;
					else
						addTime = AddFloat(addTime, 0.4f);

					m_backCheckEnemyTime = AddTime(addTime);
				}

				if (m_backCheckEnemyTime != 0.0f && m_backCheckEnemyTime >= engine->GetTime())
					return false;
			}
		}
	}

	Vector entityOrigin;
	uint8_t visibility;
	bool seeEntity = CheckVisibility(VARS(entity), &entityOrigin, &visibility);

	if (checkOnly)
		return seeEntity;

	if (setEnemy)
	{
		m_enemyOrigin = entityOrigin;
		m_visibility = visibility;
	}

	if (seeEntity)
	{
		m_backCheckEnemyTime = 0.0f;
		m_seeEnemyTime = engine->GetTime();

		SetLastEnemy(entity);
		return true;
	}

	return false;
}

bool Bot::ItemIsVisible(Vector destination, char* itemName)//, bool bomb)
{
	TraceResult tr;

	// trace a line from bot's eyes to destination..
	TraceLine(EyePosition(), destination, true, false, GetEntity(), &tr);

	// check if line of sight to object is not blocked (i.e. visible)
	if (tr.flFraction < 1.0f)
	{
		// check for standard items
		if (tr.flFraction > 0.97f && strcmp(STRING(tr.pHit->v.classname), itemName) == 0)
			return true;

		if (tr.flFraction > 0.95f && strncmp(STRING(tr.pHit->v.classname), "csdmw_", 6) == 0)
			return true;

		return false;
	}

	return true;
}

bool Bot::EntityIsVisible(Vector dest, bool fromBody)
{
	TraceResult tr;

	// trace a line from bot's eyes to destination...
	TraceLine(fromBody ? pev->origin - Vector(0.0f, 0.0f, 1.0f) : EyePosition(), dest, true, true, GetEntity(), &tr);

	// check if line of sight to object is not blocked (i.e. visible)
	return tr.flFraction >= 1.0f;
}

// this function controls zombie ai
void Bot::ZombieModeAi(void)
{
	// zombie bot slowdown fix
	if (pev->flags & FL_DUCKING)
	{
		pev->speed = pev->maxspeed;
		m_moveSpeed = pev->maxspeed;
	}

	if (!m_isSlowThink)
		return;

	extern ConVar ebot_random_join_quit;
	if (ebot_random_join_quit.GetBool() && m_stayTime > 0.0f && m_stayTime < engine->GetTime() && ChanceOf(25))
	{
		Kick();
		return;
	}

	edict_t* entity = nullptr;
	if (FNullEnt(m_enemy) && FNullEnt(m_moveTargetEntity))
	{
		edict_t* targetEnt = nullptr;
		float targetDistance = 99999.0f;

		// zombie improve
		for (const auto& client : g_clients)
		{
			if (!(client.flags & CFLAG_USED))
				continue;

			if (!(client.flags & CFLAG_ALIVE))
				continue;

			extern ConVar ebot_escape;
			if (GetGameMode() == MODE_ZH || ebot_escape.GetBool())
				entity = client.ent;
			else
			{
				Bot* bot = g_botManager->GetBot(client.ent);

				if (bot == nullptr || bot == this || !bot->m_notKilled)
					continue;

				if (bot->m_enemy == nullptr && bot->m_moveTargetEntity == nullptr)
					continue;

				entity = (bot->m_enemy == nullptr) ? bot->m_moveTargetEntity : bot->m_enemy;

				if (m_team == bot->m_team)
				{
					if (entity == targetEnt || m_team == GetTeam(entity))
						continue;
				}
				else
				{
					if (bot->GetEntity() == targetEnt || m_team != GetTeam(entity))
						continue;

					entity = bot->GetEntity();
				}
			}

			if (m_team == GetTeam(entity))
				continue;

			float distance = GetEntityDistance(entity);
			if (distance <= targetDistance)
			{
				targetDistance = distance;
				targetEnt = entity;
			}
		}

		if (!FNullEnt(targetEnt))
			SetMoveTarget(targetEnt);
	}
}

void Bot::ZmCampPointAction(int mode)
{
	if (!IsZombieMode())
		return;

	if (m_isZombieBot)
		return;

	if (g_waypoint->m_zmHmPoints.IsEmpty())
		return;

	float campAction = 0.0f;
	int campPointWaypointIndex = -1;
	
	if (IsValidWaypoint(m_myMeshWaypoint) && g_waypoint->IsZBCampPoint(m_myMeshWaypoint))
	{
		if (IsValidWaypoint(m_currentWaypointIndex) && m_currentWaypointIndex != m_myMeshWaypoint)
			return;

		if (mode == 1)
		{
			campAction = engine->RandomFloat(0.75f, 1.5f);
			campPointWaypointIndex = m_myMeshWaypoint;
		}
		else
		{
			campAction = 1.6f;
			campPointWaypointIndex = m_chosenGoalIndex;
		}
	}
	else if (IsValidWaypoint(m_currentWaypointIndex) && g_waypoint->IsZBCampPoint(m_currentWaypointIndex, false))
	{
		if (mode == 1)
		{
			campAction = 1.0f;
			campPointWaypointIndex = m_currentWaypointIndex;
		}
		else
		{
			campAction = 1.6f;
			campPointWaypointIndex = m_chosenGoalIndex;
		}
	}
	else if (mode == 0 && g_waypoint->IsZBCampPoint(GetCurrentTask()->data) && IsValidWaypoint(GetCurrentTask()->data))
	{
		if (&m_navNode[0] != nullptr)
		{
			PathNode* navid = &m_navNode[0];
			navid = navid->next;
			int movePoint = 0;
			while (navid != nullptr && movePoint <= 2)
			{
				movePoint++;
				if (GetCurrentTask()->data == navid->index)
				{
					auto navPoint = g_waypoint->GetPath(navid->index);
					if ((navPoint->origin - pev->origin).GetLength2D() <= 256.0f && navPoint->origin.z + 40.0f <= pev->origin.z && navPoint->origin.z - 25.0f >= pev->origin.z && IsWaypointOccupied(navid->index))
					{
						campAction = (movePoint * 1.8f);
						campPointWaypointIndex = GetCurrentTask()->data;
						break;
					}
				}

				navid = navid->next;
			}
		}
	}

	if (campAction == 0.0f || !IsValidWaypoint(m_currentWaypointIndex))
		m_checkCampPointTime = 0.0f;
	else if (m_checkCampPointTime == 0.0f && campAction != 1.0f)
		m_checkCampPointTime = engine->GetTime() + campAction;
	else if (m_checkCampPointTime < engine->GetTime() || campAction == 1.0f || (IsValidWaypoint(m_myMeshWaypoint) && (g_waypoint->GetPath(m_myMeshWaypoint)->origin - pev->origin).GetLength() <= 24.0f))
	{
		m_zhCampPointIndex = campPointWaypointIndex;

		m_campButtons = 0;
		SelectBestWeapon();
		MakeVectors(pev->v_angle);

		m_timeCamping = AddTime(9999.0f);
		PushTask(TASK_CAMP, TASKPRI_CAMP, -1, m_timeCamping, true);

		m_camp.x = g_waypoint->GetPath(m_zhCampPointIndex)->campStartX;
		m_camp.y = g_waypoint->GetPath(m_zhCampPointIndex)->campStartY;
		m_camp.z = 0;

		m_aimFlags |= AIM_CAMP;
		m_campDirection = 0;

		m_moveToGoal = false;
		m_checkTerrain = false;

		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;

		m_checkCampPointTime = 0.0f;
	}
}

void Bot::AvoidEntity(void)
{
	if (IsZombieMode() || FNullEnt(m_avoidEntity) || (m_avoidEntity->v.flags & FL_ONGROUND) || (m_avoidEntity->v.effects & EF_NODRAW))
	{
		m_avoidEntity = nullptr;
		m_needAvoidEntity = 0;
	}

	edict_t* entity = nullptr;
	int i, allEntity = 0;
	int avoidEntityId[checkEntityNum];
	for (i = 0; i < checkEntityNum; i++)
		avoidEntityId[i] = -1;

	while (!FNullEnt(entity = FIND_ENTITY_BY_CLASSNAME(entity, "grenade")))
	{
		if (strcmp(STRING(entity->v.model) + 9, "smokegrenade.mdl") == 0)
			continue;

		avoidEntityId[allEntity] = ENTINDEX(entity);
		allEntity++;

		if (allEntity >= checkEntityNum)
			break;
	}

	for (i = 0; i < entityNum; i++)
	{
		if (allEntity >= checkEntityNum)
			break;

		if (g_entityId[i] == -1 || g_entityAction[i] != 2)
			continue;

		if (m_team == g_entityTeam[i])
			continue;

		entity = INDEXENT(g_entityId[i]);
		if (FNullEnt(entity) || entity->v.effects & EF_NODRAW)
			continue;

		avoidEntityId[allEntity] = g_entityId[i];
		allEntity++;
	}

	for (i = 0; i < allEntity; i++)
	{
		if (avoidEntityId[i] == -1)
			continue;

		entity = INDEXENT(avoidEntityId[i]);

		if (strcmp(STRING(entity->v.classname), "grenade") == 0)
		{
			if (IsZombieMode() && strcmp(STRING(entity->v.model) + 9, "flashbang.mdl") == 0)
				continue;

			if (strcmp(STRING(entity->v.model) + 9, "hegrenade.mdl") == 0 && (GetTeam(entity->v.owner) == m_team && entity->v.owner != GetEntity()))
				continue;
		}

		if (InFieldOfView(GetEntityOrigin(entity) - EyePosition()) > pev->fov * 0.5f &&
			!EntityIsVisible(GetEntityOrigin(entity)))
			continue;

		if (strcmp(STRING(entity->v.model) + 9, "flashbang.mdl") == 0)
		{
			Vector position = (GetEntityOrigin(entity) - EyePosition()).ToAngles();
			if (m_skill >= 70)
			{
				pev->v_angle.y = AngleNormalize(position.y + 180.0f);
				m_canChooseAimDirection = false;
				return;
			}
		}

		if ((entity->v.flags & FL_ONGROUND) == 0)
		{
			float distance = (GetEntityOrigin(entity) - pev->origin).GetLength();
			float distanceMoved = ((GetEntityOrigin(entity) + entity->v.velocity * m_frameInterval) - pev->origin).GetLength();

			if (distanceMoved < distance && distance < 500)
			{
				if (IsValidWaypoint(m_currentWaypointIndex))
				{
					if ((GetEntityOrigin(entity) - g_waypoint->GetPath(m_currentWaypointIndex)->origin).GetLength() > distance || ((GetEntityOrigin(entity) + entity->v.velocity * m_frameInterval) - g_waypoint->GetPath(m_currentWaypointIndex)->origin).GetLength() > distanceMoved)
						continue;
				}

				MakeVectors(pev->v_angle);

				Vector dirToPoint = (pev->origin - GetEntityOrigin(entity)).Normalize2D();
				Vector rightSide = g_pGlobals->v_right.Normalize2D();

				if ((dirToPoint | rightSide) > 0)
					m_needAvoidEntity = -1;
				else
					m_needAvoidEntity = 1;

				m_avoidEntity = entity;

				return;
			}
		}
	}
}

bool Bot::IsBehindSmokeClouds(edict_t* ent)
{
	// in zombie mode, flares are counted as smoke and breaks the bot's vision
	if (IsZombieMode())
		return false;

	edict_t* pentGrenade = nullptr;
	Vector betweenUs = (GetEntityOrigin(ent) - pev->origin).Normalize();

	while (!FNullEnt(pentGrenade = FIND_ENTITY_BY_CLASSNAME(pentGrenade, "grenade")))
	{
		// if grenade is invisible don't care for it
		if ((pentGrenade->v.effects & EF_NODRAW) || !(pentGrenade->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)) || strcmp(STRING(pentGrenade->v.model) + 9, "smokegrenade.mdl"))
			continue;

		// check if visible to the bot
		if (InFieldOfView(GetEntityOrigin(ent) - EyePosition()) > pev->fov / 3 && !EntityIsVisible(GetEntityOrigin(ent)))
			continue;

		Vector betweenNade = (GetEntityOrigin(pentGrenade) - pev->origin).Normalize();
		Vector betweenResult = ((Vector(betweenNade.y, betweenNade.x, 0.0f) * 150.0f + GetEntityOrigin(pentGrenade)) - pev->origin).Normalize();

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

	WeaponSelect* weaponTab = &g_weaponSelect[0];

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

	WeaponSelect* weaponTab = &g_weaponSelect[0];

	for (int i = 0; i < Const_NumWeapons; i++)
	{
		auto id = weaponTab[*ptr].id;

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
	int hasWeapon = 0;
	int groundIndex = 0;
	int* ptr = g_weaponPrefs[m_personality];

	WeaponSelect* weaponTab = &g_weaponSelect[0];

	for (int i = 0; i < Const_NumWeapons; i++)
	{
		if (strcmp(weaponTab[*ptr].modelName, STRING(ent->v.model) + 9) == 0)
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

// this function checks if bot is blocked by a shoot able breakable in his moving direction
edict_t* Bot::FindBreakable(void)
{
	if (!m_isSlowThink)
		return m_breakableEntity;

	// already destroying one of them...
	if (GetCurrentTask()->taskID == TASK_DESTROYBREAKABLE)
		return m_breakableEntity;

	TraceResult tr{};
	TraceLine(pev->origin, pev->origin + (m_destOrigin - pev->origin).Normalize() * 72.0f, false, false, GetEntity(), &tr);

	if (tr.flFraction != 1.0f)
	{
		auto ent = tr.pHit;

		// check if this isn't a triggered (bomb) breakable and if it takes damage. if true, shoot the crap!
		if (IsShootableBreakable(ent))
		{
			m_breakable = GetEntityOrigin(ent);
			return ent;
		}
	}

	TraceLine(EyePosition(), EyePosition() + (m_destOrigin - EyePosition()).Normalize() * 72.0f, false, false, GetEntity(), &tr);

	if (tr.flFraction != 1.0f)
	{
		auto ent = tr.pHit;
		if (IsShootableBreakable(ent))
		{
			m_breakable = GetEntityOrigin(ent);
			return ent;
		}
	}

	m_breakableEntity = nullptr;
	m_breakable = nullvec;

	float distance = (pev->origin - m_waypointOrigin).GetLength();
	if (distance > 256)
		distance = 256.0f;

	edict_t* ent = nullptr;
	while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_breakable")))
	{
		if (!IsShootableBreakable(ent))
			continue;

		if ((pev->origin - GetEntityOrigin(ent)).GetLength() > distance)
			continue;

		const Vector origin = GetEntityOrigin(GetEntity());
		if (InFieldOfView(origin - EyePosition()) < pev->fov && EntityIsVisible(origin))
		{
			m_breakable = origin;
			m_breakableEntity = ent;
			m_campButtons = pev->button & IN_DUCK;

			PushTask(TASK_DESTROYBREAKABLE, TASKPRI_SHOOTBREAKABLE, -1, 1.0f, false);
			break;
		}
	}

	return m_breakableEntity;
}

// this function checks buttons for use button waypoint
edict_t* Bot::FindButton(void)
{
	float nearestDistance = FLT_MAX;
	edict_t* searchEntity = nullptr, * foundEntity = nullptr;

	// find the nearest button which can open our target
	while (!FNullEnt(searchEntity = FIND_ENTITY_BY_CLASSNAME(searchEntity, "func_button")))
	{
		Vector entityOrign = GetEntityOrigin(searchEntity);

		float distance = (pev->origin - entityOrign).GetLengthSquared();

		if (distance <= nearestDistance)
		{
			nearestDistance = distance;
			foundEntity = searchEntity;
		}
	}

	return foundEntity;
}

void Bot::FindItem(void)
{
	if (IsZombieMode() && m_currentWeapon != WEAPON_KNIFE) // if we're holding knife, mostly our guns dont have a ammo
		return;

	if (GetCurrentTask()->taskID == TASK_ESCAPEFROMBOMB)
		return;

	if (GetCurrentTask()->taskID == TASK_PLANTBOMB)
		return;

	if (GetCurrentTask()->taskID == TASK_THROWHEGRENADE)
		return;

	if (GetCurrentTask()->taskID == TASK_THROWFBGRENADE)
		return;

	if (GetCurrentTask()->taskID == TASK_THROWSMGRENADE)
		return;

	if ((!FNullEnt(m_enemy) || m_isSlowThink || m_isZombieBot) && GetCurrentTask()->taskID != TASK_DEFUSEBOMB)
		return;

	if (IsOnLadder())
	{
		m_pickupItem = nullptr;
		m_pickupType = PICKTYPE_NONE;
		return;
	}

	edict_t* ent = nullptr;

	if (!FNullEnt(m_pickupItem))
	{
		while (!FNullEnt(ent = FIND_ENTITY_IN_SPHERE(ent, pev->origin, 400.0f)))
		{
			if (ent != m_pickupItem || (ent->v.effects & EF_NODRAW) || IsValidPlayer(ent->v.owner))
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

	float minDistance = 401.0f;

	while (!FNullEnt(ent = FIND_ENTITY_IN_SPHERE(ent, pev->origin, 400.0f)))
	{
		pickupType = PICKTYPE_NONE;
		if ((ent->v.effects & EF_NODRAW) || ent == m_itemIgnore)
			continue;

		if (strncmp("hostage_entity", STRING(ent->v.classname), 14) == 0)
			pickupType = PICKTYPE_HOSTAGE;
		else if (strncmp("weaponbox", STRING(ent->v.classname), 9) == 0 && strcmp(STRING(ent->v.model) + 9, "backpack.mdl") == 0)
			pickupType = PICKTYPE_DROPPEDC4;
		else if (strncmp("weaponbox", STRING(ent->v.classname), 9) == 0 && strcmp(STRING(ent->v.model) + 9, "backpack.mdl") == 0 && !m_isUsingGrenade)
			pickupType = PICKTYPE_DROPPEDC4;
		else if ((strncmp("weaponbox", STRING(ent->v.classname), 9) == 0 || strncmp("armoury_entity", STRING(ent->v.classname), 14) == 0 || strncmp("csdm", STRING(ent->v.classname), 4) == 0) && !m_isUsingGrenade)
			pickupType = PICKTYPE_WEAPON;
		else if (strncmp("weapon_shield", STRING(ent->v.classname), 13) == 0 && !m_isUsingGrenade)
			pickupType = PICKTYPE_SHIELDGUN;
		else if (strncmp("item_thighpack", STRING(ent->v.classname), 14) == 0 && m_team == TEAM_COUNTER && !m_hasDefuser)
			pickupType = PICKTYPE_DEFUSEKIT;
		else if (strncmp("grenade", STRING(ent->v.classname), 7) == 0 && strcmp(STRING(ent->v.model) + 9, "c4.mdl") == 0)
			pickupType = PICKTYPE_PLANTEDC4;
		else
		{
			for (int i = 0; (i < entityNum && pickupType == PICKTYPE_NONE); i++)
			{
				if (g_entityId[i] == -1 || g_entityAction[i] != 3)
					continue;

				if (m_team != g_entityTeam[i] && g_entityTeam[i] != 2)
					continue;

				if (ent != INDEXENT(g_entityId[i]))
					continue;

				pickupType = PICKTYPE_GETENTITY;
			}
		}

		if (m_isZombieBot && pickupType != PICKTYPE_GETENTITY)
			continue;

		if (m_blockWeaponPickAPI && (pickupType == PICKTYPE_WEAPON || pickupType == PICKTYPE_SHIELDGUN || pickupType == PICKTYPE_SHIELDGUN))
			continue;

		if (pickupType == PICKTYPE_NONE)
			continue;

		Vector entityOrigin = GetEntityOrigin(ent);
		float distance = (pev->origin - entityOrigin).GetLength();
		if (distance > minDistance)
			continue;

		if (!ItemIsVisible(entityOrigin, const_cast <char*> (STRING(ent->v.classname))))
		{
			if (strncmp("grenade", STRING(ent->v.classname), 7) != 0 || strcmp(STRING(ent->v.model) + 9, "c4.mdl") != 0)
				continue;

			if (distance > 80.0f)
				continue;
		}

		bool allowPickup = true;
		if (pickupType == PICKTYPE_GETENTITY)
			allowPickup = true;
		else if (pickupType == PICKTYPE_WEAPON)
		{
			int weaponCarried = GetBestWeaponCarried();
			int secondaryWeaponCarried = GetBestSecondaryWeaponCarried();

			int weaponAmmoMax, secondaryWeaponAmmoMax;
			if (m_weaponClipAPI > 0)
				secondaryWeaponAmmoMax = weaponAmmoMax = m_weaponClipAPI;
			else
			{
				secondaryWeaponAmmoMax = g_weaponDefs[g_weaponSelect[secondaryWeaponCarried].id].ammo1Max;
				weaponAmmoMax = g_weaponDefs[g_weaponSelect[weaponCarried].id].ammo1Max;
			}

			if (secondaryWeaponCarried < 7 &&
				(m_ammo[g_weaponSelect[secondaryWeaponCarried].id] > 0.3 * secondaryWeaponAmmoMax) &&
				strcmp(STRING(ent->v.model) + 9, "w_357ammobox.mdl") == 0)
				allowPickup = false;
			else if (!m_isVIP && weaponCarried >= 7 &&
				(m_ammo[g_weaponSelect[weaponCarried].id] > 0.3 * weaponAmmoMax) &&
				strncmp(STRING(ent->v.model) + 9, "w_", 2) == 0)
			{
				if (strcmp(STRING(ent->v.model) + 9, "w_9mmarclip.mdl") == 0 && !(weaponCarried == WEAPON_FAMAS || weaponCarried == WEAPON_AK47 || weaponCarried == WEAPON_M4A1 || weaponCarried == WEAPON_GALIL || weaponCarried == WEAPON_AUG || weaponCarried == WEAPON_SG552))
					allowPickup = false;
				else if (strcmp(STRING(ent->v.model) + 9, "w_shotbox.mdl") == 0 && !(weaponCarried == WEAPON_M3 || weaponCarried == WEAPON_XM1014))
					allowPickup = false;
				else if (strcmp(STRING(ent->v.model) + 9, "w_9mmclip.mdl") == 0 && !(weaponCarried == WEAPON_MP5 || weaponCarried == WEAPON_TMP || weaponCarried == WEAPON_P90 || weaponCarried == WEAPON_MAC10 || weaponCarried == WEAPON_UMP45))
					allowPickup = false;
				else if (strcmp(STRING(ent->v.model) + 9, "w_crossbow_clip.mdl") == 0 && !(weaponCarried == WEAPON_AWP || weaponCarried == WEAPON_G3SG1 || weaponCarried == WEAPON_SCOUT || weaponCarried == WEAPON_SG550))
					allowPickup = false;
				else if (strcmp(STRING(ent->v.model) + 9, "w_chainammo.mdl") == 0 && weaponCarried == WEAPON_M249)
					allowPickup = false;
			}
			else if (m_isVIP || !RateGroundWeapon(ent))
				allowPickup = false;
			else if (strcmp(STRING(ent->v.model) + 9, "medkit.mdl") == 0 && (pev->health >= 100))
				allowPickup = false;
			else if ((strcmp(STRING(ent->v.model) + 9, "kevlar.mdl") == 0 || strcmp(STRING(ent->v.model) + 9, "battery.mdl") == 0) && pev->armorvalue >= 100) // armor vest
				allowPickup = false;
			else if (strcmp(STRING(ent->v.model) + 9, "flashbang.mdl") == 0 && (pev->weapons & (1 << WEAPON_FBGRENADE))) // concussion grenade
				allowPickup = false;
			else if (strcmp(STRING(ent->v.model) + 9, "hegrenade.mdl") == 0 && (pev->weapons & (1 << WEAPON_HEGRENADE))) // explosive grenade
				allowPickup = false;
			else if (strcmp(STRING(ent->v.model) + 9, "smokegrenade.mdl") == 0 && (pev->weapons & (1 << WEAPON_SMGRENADE))) // smoke grenade
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

				if (m_skill > 80 && ChanceOf(50) && GetCurrentTask()->taskID != TASK_GOINGFORCAMP && GetCurrentTask()->taskID != TASK_CAMP)
				{
					int index = FindDefendWaypoint(entityOrigin);

					m_campposition = g_waypoint->GetPath(index)->origin;

					PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, index, ebot_camp_max.GetFloat(), true);
					m_campButtons |= IN_DUCK;

					return;
				}
			}
			else if (pickupType == PICKTYPE_PLANTEDC4)
			{
				allowPickup = false;

				if (!m_defendedBomb)
				{
					m_defendedBomb = true;

					int index = FindDefendWaypoint(entityOrigin);
					float timeMidBlowup = g_timeBombPlanted + ((engine->GetC4TimerTime() / 2) + engine->GetC4TimerTime() / 4) - g_waypoint->GetTravelTime(m_moveSpeed, pev->origin, g_waypoint->GetPath(index)->origin);

					if (timeMidBlowup > engine->GetTime())
					{
						RemoveCertainTask(TASK_MOVETOPOSITION);
						RemoveCertainTask(TASK_GOINGFORCAMP);

						m_campposition = g_waypoint->GetPath(index)->origin;

						PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, index, engine->GetTime() + ebot_camp_max.GetFloat(), true);
						m_campButtons |= IN_DUCK;
					}
					else
						RadioMessage(Radio_ShesGonnaBlow);
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
					for (const auto& client : g_clients)
					{
						Bot* bot = g_botManager->GetBot(client.ent);

						if (bot != nullptr && bot->m_notKilled)
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

				if (m_skill > 80 && engine->RandomInt(0, 100) < 90)
				{
					int index = FindDefendWaypoint(entityOrigin);

					m_campposition = g_waypoint->GetPath(index)->origin;

					PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, index, engine->GetTime() + ebot_camp_max.GetFloat(), false);
					m_campButtons |= IN_DUCK;
					return;
				}
			}
			else if (pickupType == PICKTYPE_PLANTEDC4)
			{
				if (m_states & (STATE_SEEINGENEMY) || OutOfBombTimer())
				{
					allowPickup = false;
					return;
				}

				allowPickup = !IsBombDefusing(g_waypoint->GetBombPosition());
				if (!m_defendedBomb && !allowPickup)
				{
					m_defendedBomb = true;

					int index = FindDefendWaypoint(entityOrigin);
					float timeBlowup = g_timeBombPlanted + engine->GetC4TimerTime() - g_waypoint->GetTravelTime(pev->maxspeed, pev->origin, g_waypoint->GetPath(index)->origin);

					RemoveCertainTask(TASK_MOVETOPOSITION); // remove any move tasks
					RemoveCertainTask(TASK_GOINGFORCAMP);

					m_campposition = g_waypoint->GetPath(index)->origin;
					PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, index, engine->GetTime() + timeBlowup, true);
					m_campButtons &= ~IN_DUCK;
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
		for (const auto& client : g_clients)
		{
			Bot* bot = g_botManager->GetBot(client.ent);
			if (bot != nullptr && bot != this && IsAlive(bot->GetEntity()) && bot->m_pickupItem == pickupItem)
			{
				m_pickupItem = nullptr;
				m_pickupType = PICKTYPE_NONE;

				return;
			}
		}

		Vector pickupOrigin = GetEntityOrigin(pickupItem);
		if (pickupOrigin.z > EyePosition().z + 12.0f || IsDeadlyDrop(pickupOrigin))
		{
			m_pickupItem = nullptr;
			m_pickupType = PICKTYPE_NONE;

			return;
		}

		m_pickupItem = pickupItem;
	}
}

void Bot::GetCampDirection(Vector* dest)
{
	// this function check if view on last enemy position is blocked - replace with better vector then
	// mostly used for getting a good camping direction vector if not camping on a camp waypoint

	TraceResult tr;
	Vector src = EyePosition();

	TraceLine(src, *dest, true, true, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f)
	{
		float length = (tr.vecEndPos - src).GetLength();

		if (length > 10000.0f)
			return;

		float minDistance = FLT_MAX;
		float maxDistance = FLT_MAX;

		int enemyIndex = -1, tempIndex = -1;

		// find nearest waypoint to bot and position
		for (int i = 0; i < g_numWaypoints; i++)
		{
			float distance = (g_waypoint->GetPath(i)->origin - pev->origin).GetLength();

			if (distance < minDistance)
			{
				minDistance = distance;
				tempIndex = i;
			}
			distance = (g_waypoint->GetPath(i)->origin - *dest).GetLength();

			if (distance < maxDistance)
			{
				maxDistance = distance;
				enemyIndex = i;
			}
		}

		if (!IsValidWaypoint(tempIndex) || !IsValidWaypoint(enemyIndex))
			return;

		minDistance = FLT_MAX;

		int lookAtWaypoint = -1;
		Path* path = g_waypoint->GetPath(tempIndex);

		for (int i = 0; i < Const_MaxPathIndex; i++)
		{
			if (path->index[i] == -1)
				continue;

			float distance = g_waypoint->GetPathDistanceFloat(path->index[i], enemyIndex);

			if (distance < minDistance)
			{
				minDistance = distance;
				lookAtWaypoint = path->index[i];
			}
		}

		if (IsValidWaypoint(lookAtWaypoint))
			*dest = g_waypoint->GetPath(lookAtWaypoint)->origin;
	}
}

// this function depending on show boolen, shows/remove chatter, icon, on the head of bot
void Bot::SwitchChatterIcon(bool show)
{
	if (g_gameVersion == CSVER_VERYOLD)
		return;

	for (int i = 0; i < engine->GetMaxClients(); i++)
	{
		edict_t* ent = INDEXENT(i);

		if (!IsValidPlayer(ent) || IsValidBot(ent) || GetTeam(ent) != m_team)
			continue;

		MESSAGE_BEGIN(MSG_ONE, g_netMsg->GetId(NETMSG_BOTVOICE), nullptr, ent); // begin message
		WRITE_BYTE(show); // switch on/off
		WRITE_BYTE(GetIndex());
		MESSAGE_END();
	}
}

// this function inserts the radio message into the message queue
void Bot::RadioMessage(int message)
{
	if (ebot_use_radio.GetInt() <= 0)
		return;

	if (m_radiotimer > engine->GetTime())
		return;

	if (m_numFriendsLeft == 0)
		return;

	if (m_numEnemiesLeft == 0)
	{
		PlayChatterMessage(ChatterMessage::Happy);
		return;
	}

	if (GetGameMode() == MODE_DM)
		return;

	// poor bots spamming this :(
	if (IsZombieMode() && message == Radio_NeedBackup && !g_waypoint->m_zmHmPoints.IsEmpty())
		return;

	m_radioSelect = message;

	if (ebot_use_radio.GetInt() != 1)
	{
		PlayChatterMessage(GetEqualChatter(message));
		return;
	}

	PushMessageQueue(CMENU_RADIO);
	m_radiotimer = engine->GetTime() + engine->RandomFloat(m_numFriendsLeft, m_numFriendsLeft * 1.5f);
}

// this function inserts the voice message into the message queue (mostly same as above)
void Bot::PlayChatterMessage(ChatterMessage message)
{
	if (ebot_use_radio.GetInt() <= 1)
		return;

	if (g_audioTime >= engine->GetTime())
		return;

	if (m_numFriendsLeft == 0)
		return;

	if (m_radiotimer > engine->GetTime())
		return;

	if (m_lastChatterMessage == message)
		return;

	char* voice = "nothing";
	float dur = -1.0f;
	GetVoiceAndDur(message, &voice, &dur);

	if (dur == -1.0f)
		return;

	dur += 1.0f;
	m_chatterTimer = engine->GetTime() + dur;

	SwitchChatterIcon(true);
	g_audioTime = m_chatterTimer;

	for (int i = 0; i < engine->GetMaxClients(); i++)
	{
		edict_t* ent = INDEXENT(i);

		if (!IsValidPlayer(ent) || IsValidBot(ent) || GetTeam(ent) != m_team)
			continue;

		MESSAGE_BEGIN(MSG_ONE, g_netMsg->GetId(NETMSG_SENDAUDIO), nullptr, ent); // begin message
		WRITE_BYTE(GetIndex());

		if (!(pev->deadflag & DEAD_DEAD))
			WRITE_STRING(FormatBuffer("%s/%s.wav", ebot_chatter_path.GetString(), voice));

		WRITE_SHORT(m_voicePitch);
		MESSAGE_END();
	}

	m_lastChatterMessage = message;

	m_radiotimer = engine->GetTime() + engine->RandomFloat(m_numFriendsLeft / 2.0f, m_numFriendsLeft * 1.5f);
	PushMessageQueue(CMENU_RADIO);
}

// this function checks and executes pending messages
void Bot::CheckMessageQueue(void)
{
	if (m_actMessageIndex == m_pushMessageIndex)
		return;

	// get message from stack
	auto state = GetMessageQueue();

	// nothing to do?
	if (state == CMENU_IDLE || (state == CMENU_RADIO && GetGameMode() == MODE_DM))
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
			m_buyPending = true;
			m_buyingFinished = true;
			break;
		}

		m_buyPending = false;
		m_nextBuyTime = engine->GetTime() + engine->RandomFloat(0.6f, 1.2f);

		// if freezetime is very low do not delay the buy process
		if (CVAR_GET_FLOAT("mp_freezetime") <= 1.0f)
			m_nextBuyTime = engine->GetTime() + engine->RandomFloat(0.25f, 0.5f);

		// if fun-mode no need to buy
		if (ebot_knifemode.GetBool() && (ebot_eco_rounds.GetInt() != 1 || HasPrimaryWeapon()))
		{
			m_buyState = 6;
			SelectWeaponByName("weapon_knife");
		}

		// prevent vip from buying
		if (g_mapType & MAP_AS)
		{
			if (*(INFOKEY_VALUE(GET_INFOKEYBUFFER(GetEntity()), "model")) == 'v')
			{
				m_isVIP = true;
				m_buyState = 7;
			}
		}

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
			if (ChanceOf(m_skill))
				SelectWeaponByName("weapon_knife");
			return;
		}

		PushMessageQueue(CMENU_IDLE);
		PerformWeaponPurchase();

		break;

	case CMENU_RADIO:
		if (!m_notKilled)
			break;

		// if last bot radio command (global) happened just a 3 seconds ago, delay response
		if ((m_team == 0 || m_team == 1) && (g_lastRadioTime[m_team] + 3.0f < engine->GetTime()))
		{
			// if same message like previous just do a yes/no
			if (m_radioSelect != Radio_Affirmative && m_radioSelect != Radio_Negative)
			{
				if (m_radioSelect == g_lastRadio[m_team] && g_lastRadioTime[m_team] + 1.5 > engine->GetTime())
					m_radioSelect = -1;
				else
				{
					if (m_radioSelect != Radio_ReportingIn)
						g_lastRadio[m_team] = m_radioSelect;
					else
						g_lastRadio[m_team] = -1;

					for (const auto& client : g_clients)
					{
						Bot* bot = g_botManager->GetBot(client.ent);
						if (bot != nullptr && pev != bot->pev && bot->m_team == m_team)
						{
							bot->m_radioOrder = m_radioSelect;
							bot->m_radioEntity = GetEntity();
						}
					}
				}
			}

			if (m_radioSelect != -1 && ebot_use_radio.GetInt() <= 1)
			{
				if (m_radioSelect != Radio_ReportingIn)
				{
					if (m_radioSelect < Radio_GoGoGo)
						FakeClientCommand(GetEntity(), "radio1");
					else if (m_radioSelect < Radio_Affirmative) 
					{
						m_radioSelect -= Radio_GoGoGo - 1;
						FakeClientCommand(GetEntity(), "radio2");
					}
					else
					{
						m_radioSelect -= Radio_Affirmative - 1;
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
bool Bot::IsRestricted(int weaponIndex)
{
	if (IsNullString(ebot_restrictweapons.GetString()))
		return IsRestrictedAMX(weaponIndex);

	Array <String> bannedWeapons = String(ebot_restrictweapons.GetString()).Split(';');

	ITERATE_ARRAY(bannedWeapons, i)
	{
		const char* banned = STRING(GetWeaponReturn(true, nullptr, weaponIndex));

		// check is this weapon is banned
		if (strncmp(bannedWeapons[i], banned, bannedWeapons[i].GetLength()) == 0)
			return true;
	}

	if (m_buyingFinished)
		return false;

	return IsRestrictedAMX(weaponIndex);
}

// this function checks restriction set by AMX Mod, this function code is courtesy of KWo
bool Bot::IsRestrictedAMX(int weaponIndex)
{
	const char* restrictedWeapons = CVAR_GET_STRING("amx_restrweapons");
	const char* restrictedEquipment = CVAR_GET_STRING("amx_restrequipammo");

	// check for weapon restrictions
	if ((1 << weaponIndex) & (WeaponBits_Primary | WeaponBits_Secondary | WEAPON_SHIELDGUN))
	{
		if (IsNullString(restrictedWeapons))
			return false;

		int indices[] = { 4, 25, 20, -1, 8, -1, 12, 19, -1, 5, 6, 13, 23, 17, 18, 1, 2, 21, 9, 24, 7, 16, 10, 22, -1, 3, 15, 14, 0, 11 };

		// find the weapon index
		int index = indices[weaponIndex - 1];

		// validate index range
		if (index < 0 || index >= static_cast <int> (strlen(restrictedWeapons)))
			return false;

		return restrictedWeapons[index] != '0';
	}
	else // check for equipment restrictions
	{
		if (IsNullString(restrictedEquipment))
			return false;

		int indices[] = { -1, -1, -1, 3, -1, -1, -1, -1, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, -1, -1, -1, -1, -1, 0, 1, 5 };

		// find the weapon index
		int index = indices[weaponIndex - 1];

		// validate index range
		if (index < 0 || index >= static_cast <int> (strlen(restrictedEquipment)))
			return false;

		return restrictedEquipment[index] != '0';
	}
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

				if (g_gameVersion == CSVER_VERYOLD && selectedWeapon->buySelect == -1)
					continue;

				if (selectedWeapon->teamStandard != 2 && selectedWeapon->teamStandard != m_team)
					continue;

				if (IsRestricted(selectedWeapon->id))
					continue;

				gunMoney = selectedWeapon->price;

				if (playerMoney <= gunMoney)
					continue;

				int gunMode = BuyWeaponMode(selectedWeapon->id);

				if (playerMoney < gunMoney + (gunMode * 100))
					continue;

				if (likeGunId[0] == 0)
					likeGunId[0] = selectedWeapon->id;
				else
				{
					if (gunMode <= BuyWeaponMode(likeGunId[0]))
					{
						if ((BuyWeaponMode(likeGunId[1]) > BuyWeaponMode(likeGunId[0])) ||
							(BuyWeaponMode(likeGunId[1]) == BuyWeaponMode(likeGunId[0]) && (engine->RandomInt(1, 2) == 2)))
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
					weaponId = likeGunId[(engine->RandomInt(1, 7) > 3) ? 0 : 1];

				for (int i = 0; i < Const_NumWeapons; i++)
				{
					if (buyWeapon[i].id == weaponId)
					{
						FakeClientCommand(GetEntity(), "buy;menuselect %d", buyWeapon[i].buyGroup);

						if (g_gameVersion == CSVER_VERYOLD)
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
			m_reloadState = RSTATE_PRIMARY;

		break;

	case 1:
		if (pev->armorvalue < engine->RandomInt(50, 80) && (g_botManager->EconomicsValid(m_team) && HasPrimaryWeapon()))
		{
			if (m_moneyAmount > 1500 && !IsRestricted(WEAPON_KEVHELM))
				FakeClientCommand(GetEntity(), "buyequip;menuselect 2");
			else
				FakeClientCommand(GetEntity(), "buyequip;menuselect 1");
		}
		break;

	case 2:
		if ((HasPrimaryWeapon() && m_moneyAmount > engine->RandomInt(6000, 9000)))
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

				if (g_gameVersion == CSVER_VERYOLD && selectedWeapon->buySelect == -1)
					continue;

				if (selectedWeapon->teamStandard != 2 && selectedWeapon->teamStandard != m_team)
					continue;

				if (IsRestricted(selectedWeapon->id))
					continue;

				if (m_moneyAmount <= (selectedWeapon->price + 120))
					continue;

				int gunMode = BuyWeaponMode(selectedWeapon->id);

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
				WeaponSelect* buyWeapon = &g_weaponSelect[0];
				int weaponId = likeGunId;

				for (int i = 0; i < Const_NumWeapons; i++)
				{
					if (buyWeapon[i].id == weaponId)
					{
						FakeClientCommand(GetEntity(), "buy;menuselect %d", buyWeapon[i].buyGroup);

						if (g_gameVersion == CSVER_VERYOLD)
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

		if (engine->RandomInt(1, 2) == 1)
		{
			FakeClientCommand(GetEntity(), "buy;menuselect 1");

			if (engine->RandomInt(1, 2) == 1)
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
			if (g_gameVersion == CSVER_VERYOLD)
				FakeClientCommand(GetEntity(), "buyequip;menuselect 6");
			else
				FakeClientCommand(GetEntity(), "defuser"); // use alias in SteamCS
		}

		break;

	case 6:
		for (int i = 0; i <= 5; i++)
			FakeClientCommand(GetEntity(), "buyammo%d", engine->RandomInt(1, 2)); // simulate human

		if (ChanceOf(m_skill))
			FakeClientCommand(GetEntity(), "buy;menuselect 7");
		else
			FakeClientCommand(GetEntity(), "buy;menuselect 6");

		if (m_reloadState != RSTATE_PRIMARY)
			m_reloadState = RSTATE_SECONDARY;

		break;

	}

	m_buyState++;
	PushMessageQueue(CMENU_BUY);
}

int Bot::BuyWeaponMode(int weaponId)
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

// this function iven some values min and max, clamp the inputs to be inside the [min, max] range
Task* ClampDesire(Task* first, float min, float max)
{
	if (first->desire < min)
		first->desire = min;
	else if (first->desire > max)
		first->desire = max;

	return first;
}

// this function returns the behavior having the higher activation level
Task* MaxDesire(Task* first, Task* second)
{
	if (first->desire > second->desire)
		return first;

	return second;
}

// this function returns the first behavior if its activation level is anything higher than zero
Task* SubsumeDesire(Task* first, Task* second)
{
	if (first->desire > 0)
		return first;

	return second;
}

// this function returns the input behavior if it's activation level exceeds the threshold, or some default behavior otherwise
Task* ThresholdDesire(Task* first, float threshold, float desire)
{
	

	if (first->desire < threshold)
		first->desire = desire;

	return first;
}

// this function clamp the inputs to be the last known value outside the [min, max] range
float HysteresisDesire(float cur, float min, float max, float old)
{
	if (cur <= min || cur >= max)
		old = cur;

	return old;
}

// this function carried out each frame. does all of the sensing, calculates emotions and finally sets the desired action after applying all of the Filters
void Bot::SetConditions(void)
{
	m_aimFlags = 0;

	// slowly increase/decrease dynamic emotions back to their base level
	if (m_nextEmotionUpdate < engine->GetTime())
	{
		if (m_skill >= 80)
		{
			m_agressionLevel *= 2;
			m_fearLevel *= 0.5f;
		}

		if (m_agressionLevel > m_baseAgressionLevel)
			m_agressionLevel -= 0.05f;
		else
			m_agressionLevel += 0.05f;

		if (m_fearLevel > m_baseFearLevel)
			m_fearLevel -= 0.05f;
		else
			m_fearLevel += 0.05f;

		if (m_agressionLevel < 0.0f)
			m_agressionLevel = 0.0f;

		if (m_fearLevel < 0.0f)
			m_fearLevel = 0.0f;

		m_nextEmotionUpdate = engine->GetTime() + 0.5f;
	}

	// does bot see an enemy?
	if (LookupEnemy())
	{
		m_states |= STATE_SEEINGENEMY;
		SetMoveTarget(nullptr);
	}
	else
	{
		m_states &= ~STATE_SEEINGENEMY;
		SetEnemy(nullptr);
	}

	if (m_lastVictim != nullptr && (!IsAlive(m_lastVictim) || !IsValidPlayer(m_lastVictim)))
		m_lastVictim = nullptr;

	// did bot just kill an enemy?
	if (!FNullEnt(m_lastVictim))
	{
		if (GetTeam(m_lastVictim) != m_team)
		{
			// add some aggression because we just killed somebody
			m_agressionLevel += 0.1f;

			if (m_agressionLevel > 1.0f)
				m_agressionLevel = 1.0f;

			if (ChanceOf(50))
				ChatMessage(CHAT_KILL);
			else
				RadioMessage(Radio_EnemyDown);

			// if no more enemies found AND bomb planted, switch to knife to get to bombplace faster
			if (m_team == TEAM_COUNTER && m_currentWeapon != WEAPON_KNIFE && m_numEnemiesLeft == 0 && g_bombPlanted)
			{
				SelectWeaponByName("weapon_knife");

				// order team to regroup
				RadioMessage(Radio_RegroupTeam);
			}
		}
		else
			ChatMessage(CHAT_TEAMKILL, true);

		m_lastVictim = nullptr;
	}

	// check if our current enemy is still valid
	if (!FNullEnt(m_lastEnemy))
	{
		if (IsNotAttackLab(m_lastEnemy) || (!IsAlive(m_lastEnemy) && m_shootAtDeadTime < engine->GetTime()))
			SetLastEnemy(nullptr);
	}
	else
		SetLastEnemy(nullptr);

	// don't listen if seeing enemy, just checked for sounds or being blinded (because its inhuman)
	if (m_soundUpdateTime <= engine->GetTime() && m_blindTime < engine->GetTime())
	{
		ReactOnSound();
		m_soundUpdateTime = engine->GetTime() + 0.3f;
	}
	else if (m_heardSoundTime < engine->GetTime())
		m_states &= ~STATE_HEARENEMY;

	if (FNullEnt(m_enemy) && !FNullEnt(m_lastEnemy) && m_lastEnemyOrigin != nullvec && !IsZombieMode() && (pev->origin - m_lastEnemyOrigin).GetLength() < 1600.0f)
	{
		TraceResult tr;
		TraceLine(EyePosition(), m_lastEnemyOrigin, true, true, GetEntity(), &tr);

		if ((tr.flFraction >= 0.2f || tr.pHit != g_worldEdict))
		{
			m_aimFlags |= AIM_PREDICTENEMY;
			if (EntityIsVisible(m_lastEnemyOrigin))
				m_aimFlags |= AIM_LASTENEMY;
		}
	}

	CheckGrenadeThrow();

	// check if there are items needing to be used/collected
	if (m_itemCheckTime < engine->GetTime() || !FNullEnt(m_pickupItem))
	{
		m_itemCheckTime = engine->GetTime() + engine->RandomInt(2.0f, 4.0f);
		FindItem();
	}

	float tempFear = m_fearLevel;
	float tempAgression = m_agressionLevel;

	// decrease fear if teammates near
	int friendlyNum = 0;

	if (m_lastEnemyOrigin != nullvec)
		friendlyNum = GetNearbyFriendsNearPosition(pev->origin, 300) - GetNearbyEnemiesNearPosition(m_lastEnemyOrigin, 500);

	if (friendlyNum > 0)
		tempFear = tempFear * 0.5f;

	// increase/decrease fear/aggression if bot uses a sniping weapon to be more careful
	if (UsesSniper())
	{
		tempFear = tempFear * 1.5f;
		tempAgression = tempAgression * 0.5f;
	}

	// initialize & calculate the desire for all actions based on distances, emotions and other stuff
	GetCurrentTask();

	// bot found some item to use?
	if (m_pickupType != PICKTYPE_NONE && !FNullEnt(m_pickupItem) && !(m_states & STATE_SEEINGENEMY))
	{
		m_states |= STATE_PICKUPITEM;

		if (m_pickupType == PICKTYPE_BUTTON)
			g_taskFilters[TASK_PICKUPITEM].desire = 50.0f; // always pickup button
		else
		{
			float distance = (500.0f - (GetEntityOrigin(m_pickupItem) - pev->origin).GetLength()) * 0.2f;

			if (distance > 60.0f)
				distance = 60.0f;

			g_taskFilters[TASK_PICKUPITEM].desire = distance;
		}
	}
	else
	{
		m_states &= ~STATE_PICKUPITEM;
		g_taskFilters[TASK_PICKUPITEM].desire = 0.0f;
	}

	float desireLevel = 0.0f;

	// calculate desire to attack
	if (GetCurrentTask()->taskID != TASK_THROWFBGRENADE && GetCurrentTask()->taskID != TASK_THROWHEGRENADE && GetCurrentTask()->taskID != TASK_THROWSMGRENADE && (m_states & STATE_SEEINGENEMY) && ReactOnEnemy())
		g_taskFilters[TASK_FIGHTENEMY].desire = TASKPRI_FIGHTENEMY;
	else if (GetCurrentTask()->taskID != TASK_THROWFBGRENADE && GetCurrentTask()->taskID != TASK_THROWHEGRENADE && GetCurrentTask()->taskID != TASK_THROWSMGRENADE)
		g_taskFilters[TASK_FIGHTENEMY].desire = 0;

	// calculate desires to seek cover or hunt
	if (IsValidPlayer(m_lastEnemy) && m_lastEnemyOrigin != nullvec && !((g_mapType & MAP_DE) && g_bombPlanted) && !m_isBomber && (m_loosedBombWptIndex == -1 && m_team == TEAM_TERRORIST))
	{
		float distance = (m_lastEnemyOrigin - pev->origin).GetLength();
		float timeSeen = m_seeEnemyTime - engine->GetTime();
		float timeHeard = m_heardSoundTime - engine->GetTime();
		float ratio = 0.0f;
		float retreatLevel = (100.0f - pev->health) * tempFear;

		if (timeSeen > timeHeard)
		{
			timeSeen += 10.0f;
			ratio = timeSeen * 0.1f;
		}
		else
		{
			timeHeard += 10.0f;
			ratio = timeHeard * 0.1f;
		}

		if (m_isZombieBot)
			ratio = 0;
		else
		{
			if (!FNullEnt(m_enemy) && IsZombieEntity(m_enemy))
				ratio *= 10;
			else if (!FNullEnt(m_lastEnemy) && IsZombieEntity(m_lastEnemy))
				ratio *= 6;
			else if (g_bombPlanted || m_isStuck)
				ratio /= 3;
			else if (m_isVIP || m_isReloading)
				ratio *= 2;
		}

		if (distance > 500.0f || (!m_isZombieBot && ((!FNullEnt(m_enemy) && IsZombieEntity(m_enemy)) || (!FNullEnt(m_lastEnemy) && IsZombieEntity(m_lastEnemy)))))
			g_taskFilters[TASK_SEEKCOVER].desire = retreatLevel * ratio;

		// if half of the round is over, allow hunting
		// FIXME: it probably should be also team/map dependant
		if (FNullEnt(m_enemy) && (g_timeRoundMid < engine->GetTime()) && !m_isUsingGrenade && m_personality != PERSONALITY_CAREFUL && m_currentWaypointIndex != g_waypoint->FindNearest(m_lastEnemyOrigin))
		{
			desireLevel = 4096.0f - ((1.0f - tempAgression) * distance);
			desireLevel = (100 * desireLevel) / 4096.0f;
			desireLevel -= retreatLevel;

			if (desireLevel > 89)
				desireLevel = 89;

			if (IsDeathmatchMode())
				desireLevel *= 2;
			else if (IsZombieMode())
				desireLevel = 0;
			else
			{
				if (g_mapType & MAP_DE)
				{
					if ((g_bombPlanted && m_team == TEAM_COUNTER) || (!g_bombPlanted && m_team == TEAM_TERRORIST))
						desireLevel *= 1.5;
					if ((g_bombPlanted && m_team == TEAM_TERRORIST) || (!g_bombPlanted && m_team == TEAM_COUNTER))
						desireLevel *= 0.5;
				}
			}

			g_taskFilters[TASK_HUNTENEMY].desire = desireLevel;
		}
		else
			g_taskFilters[TASK_HUNTENEMY].desire = 0;
	}
	else
	{
		g_taskFilters[TASK_SEEKCOVER].desire = 0;
		g_taskFilters[TASK_HUNTENEMY].desire = 0;
	}

	// blinded behaviour
	if (m_blindTime > engine->GetTime())
		g_taskFilters[TASK_BLINDED].desire = TASKPRI_BLINDED;
	else
		g_taskFilters[TASK_BLINDED].desire = 0.0f;

	// now we've initialized all the desires go through the hard work
	// of filtering all actions against each other to pick the most
	// rewarding one to the bot.

	// FIXME: instead of going through all of the actions it might be
	// better to use some kind of decision tree to sort out impossible
	// actions.

	// most of the values were found out by trial-and-error and a helper
	// utility i wrote so there could still be some weird behaviors, it's
	// hard to check them all out.

	m_oldCombatDesire = HysteresisDesire(g_taskFilters[TASK_FIGHTENEMY].desire, 40.0, 90.0, m_oldCombatDesire);
	g_taskFilters[TASK_FIGHTENEMY].desire = m_oldCombatDesire;

	Task* taskOffensive = &g_taskFilters[TASK_FIGHTENEMY];
	Task* taskPickup = &g_taskFilters[TASK_PICKUPITEM];

	// calc survive (cover/hide)
	Task* taskSurvive = ThresholdDesire(&g_taskFilters[TASK_SEEKCOVER], 40.0, 0.0f);
	taskSurvive = SubsumeDesire(&g_taskFilters[TASK_HIDE], taskSurvive);

	taskOffensive = SubsumeDesire(taskOffensive, taskPickup); // if offensive task, don't allow picking up stuff

	Task* final = SubsumeDesire(&g_taskFilters[TASK_BLINDED], MaxDesire(taskOffensive, &g_taskFilters[TASK_HIDE])); // reason about fleeing instead

	if (m_tasks != nullptr)
		final = MaxDesire(final, m_tasks);

	PushTask(final);
}

// this function resets bot tasks stack, by removing all entries from the stack
void Bot::ResetTasks(void)
{
	if (m_tasks == nullptr)
		return; // reliability check

	Task* next = m_tasks->next;
	Task* prev = m_tasks;

	while (m_tasks != nullptr)
	{
		prev = m_tasks->prev;

		delete m_tasks;
		m_tasks = prev;
	}
	m_tasks = next;

	while (m_tasks != nullptr)
	{
		next = m_tasks->next;

		delete m_tasks;
		m_tasks = next;
	}
	m_tasks = nullptr;
}

// this function checks the tasks priorities
void Bot::CheckTasksPriorities(void)
{
	if (m_tasks == nullptr)
	{
		GetCurrentTask();
		return;
	}

	Task* oldTask = m_tasks;
	Task* prev = nullptr;
	Task* next = nullptr;

	while (m_tasks->prev != nullptr)
	{
		prev = m_tasks->prev;
		m_tasks = prev;
	}

	Task* firstTask = m_tasks;
	Task* maxDesiredTask = m_tasks;

	float maxDesire = m_tasks->desire;

	// search for the most desired task
	while (m_tasks->next != nullptr)
	{
		next = m_tasks->next;
		m_tasks = next;

		if (m_tasks->desire >= maxDesire)
		{
			maxDesiredTask = m_tasks;
			maxDesire = m_tasks->desire;
		}
	}

	m_tasks = maxDesiredTask; // now we found the most desired pushed task...

	// something was changed with priorities, check if some task doesn't need to be deleted...
	if (oldTask != maxDesiredTask)
	{
		m_tasks = firstTask;

		while (m_tasks != nullptr)
		{
			next = m_tasks->next;

			// some task has to be deleted if cannot be continued...
			if (m_tasks != maxDesiredTask && !m_tasks->canContinue)
			{
				if (m_tasks->prev != nullptr)
					m_tasks->prev->next = m_tasks->next;

				if (m_tasks->next != nullptr)
					m_tasks->next->prev = m_tasks->prev;

				delete m_tasks;
			}
			m_tasks = next;
		}
	}

	m_tasks = maxDesiredTask;

	if (IsValidWaypoint(ebot_debuggoal.GetInt()))
		m_chosenGoalIndex = ebot_debuggoal.GetInt();
	else
		m_chosenGoalIndex = m_tasks->data;

	if (!IsValidWaypoint(m_chosenGoalIndex))
		m_chosenGoalIndex = g_waypoint->m_otherPoints.GetRandomElement();
}

// this function builds task
void Bot::PushTask(BotTask taskID, float desire, int data, float time, bool canContinue)
{
	float realTime = -1.0f;

	// auto game time
	if (time != -1.0f && time < engine->GetTime())
		realTime = AddTime(time);

	Task task = { nullptr, nullptr, taskID, desire, data, realTime, canContinue };
	PushTask(&task); // use standard function to start task
}

// this function adds task pointer on the bot task stack
void Bot::PushTask(Task* task)
{
	bool newTaskDifferent = false;
	bool foundTaskExisting = false;
	bool checkPriorities = false;

	Task* oldTask = GetCurrentTask(); // remember our current task

	// at the beginning need to clean up all null tasks...
	if (m_tasks == nullptr)
	{
		m_lastCollTime = engine->GetTime() + 0.5f;

		Task* newTask = new Task;

		newTask->taskID = TASK_NORMAL;
		newTask->desire = TASKPRI_NORMAL;
		newTask->canContinue = true;
		newTask->time = 0.0f;
		newTask->data = -1;
		newTask->next = newTask->prev = nullptr;

		m_tasks = newTask;
		DeleteSearchNodes();

		if (task == nullptr)
			return;
		else if (task->taskID == TASK_NORMAL)
		{
			m_tasks->desire = TASKPRI_NORMAL;
			m_tasks->data = task->data;
			m_tasks->time = task->time;

			return;
		}
	}
	else if (task == nullptr)
		return;

	// it shouldn't happen this condition now as false...
	if (m_tasks != nullptr)
	{
		if (m_tasks->taskID == task->taskID)
		{
			if (m_tasks->data != task->data)
			{
				m_lastCollTime = engine->GetTime() + 0.5f;

				DeleteSearchNodes();
				m_tasks->data = task->data;
			}

			if (m_tasks->desire != task->desire)
			{
				m_tasks->desire = task->desire;
				checkPriorities = true;
			}
			else if (m_tasks->data == task->data)
				return;
		}
		else
		{
			// find the first task on the stack and don't allow push the new one like the same already existing one
			while (m_tasks->prev != nullptr)
			{
				m_tasks = m_tasks->prev;

				if (m_tasks->taskID == task->taskID)
				{
					foundTaskExisting = true;

					if (m_tasks->desire != task->desire)
						checkPriorities = true;

					m_tasks->desire = task->desire;
					m_tasks->data = task->data;
					m_tasks->time = task->time;
					m_tasks->canContinue = task->canContinue;
					m_tasks = oldTask;

					break; // now we may need to check the current max desire or next tasks...
				}
			}

			// now go back to the previous stack position and try to find the same task as one of "the next" ones (already pushed before and not finished yet)
			if (!foundTaskExisting && !checkPriorities)
			{
				m_tasks = oldTask;

				while (m_tasks->next != nullptr)
				{
					m_tasks = m_tasks->next;

					if (m_tasks->taskID == task->taskID)
					{
						foundTaskExisting = true;

						if (m_tasks->desire != task->desire)
							checkPriorities = true;

						m_tasks->desire = task->desire;
						m_tasks->data = task->data;
						m_tasks->time = task->time;
						m_tasks->canContinue = task->canContinue;
						m_tasks = oldTask;

						break; // now we may need to check the current max desire...
					}
				}
			}

			if (!foundTaskExisting)
				newTaskDifferent = true; // we have some new task pushed on the stack...
		}
	}
	m_tasks = oldTask;

	if (newTaskDifferent)
	{
		Task* newTask = new Task;

		newTask->taskID = task->taskID;
		newTask->desire = task->desire;
		newTask->canContinue = task->canContinue;
		newTask->time = task->time;
		newTask->data = task->data;
		newTask->next = nullptr;

		while (m_tasks->next != nullptr)
			m_tasks = m_tasks->next;

		newTask->prev = m_tasks;
		m_tasks->next = newTask;

		checkPriorities = true;
	}
	m_tasks = oldTask;

	// needs check the priorities and setup the task with the max desire...
	if (!checkPriorities)
		return;

	CheckTasksPriorities();

	// the max desired task has been changed...
	if (m_tasks != oldTask)
	{
		DeleteSearchNodes();
		m_lastCollTime = engine->GetTime() + 0.5f;

		// leader bot?
		if (newTaskDifferent && m_isLeader && m_tasks->taskID == TASK_SEEKCOVER)
			CommandTeam(); // reorganize team if fleeing

		if (newTaskDifferent && m_tasks->taskID == TASK_CAMP)
			SelectBestWeapon();
	}
}

// this function gets bot's current task
Task* Bot::GetCurrentTask(void)
{
	if (m_tasks != nullptr)
		return m_tasks;

	Task* newTask = new Task;

	newTask->taskID = TASK_NORMAL;
	newTask->desire = TASKPRI_NORMAL;
	newTask->data = -1;
	newTask->time = 0.0f;
	newTask->canContinue = true;
	newTask->next = newTask->prev = nullptr;

	m_tasks = newTask;
	m_lastCollTime = engine->GetTime() + 0.5f;
	return m_tasks;
}

// this function removes one task from the bot task stack
void Bot::RemoveCertainTask(BotTask taskID)
{
	if (m_tasks == nullptr || (m_tasks != nullptr && m_tasks->taskID == TASK_NORMAL))
		return; // since normal task can be only once on the stack, don't remove it...

	bool checkPriorities = false;

	Task* task = m_tasks;
	Task* oldTask = m_tasks;
	Task* oldPrevTask = task->prev;
	Task* oldNextTask = task->next;

	while (task->prev != nullptr)
		task = task->prev;

	while (task != nullptr)
	{
		Task* next = task->next;
		Task* prev = task->prev;

		if (task->taskID == taskID)
		{
			if (prev != nullptr)
				prev->next = next;

			if (next != nullptr)
				next->prev = prev;

			if (task == oldTask)
				oldTask = nullptr;
			else if (task == oldPrevTask)
				oldPrevTask = nullptr;
			else if (task == oldNextTask)
				oldNextTask = nullptr;

			delete task;

			checkPriorities = true;
			break;
		}
		task = next;
	}

	if (oldTask != nullptr)
		m_tasks = oldTask;
	else if (oldPrevTask != nullptr)
		m_tasks = oldPrevTask;
	else if (oldNextTask != nullptr)
		m_tasks = oldNextTask;
	else
		GetCurrentTask();

	if (checkPriorities)
		CheckTasksPriorities();
}

// this function called whenever a task is completed
void Bot::TaskComplete(void)
{
	if (m_tasks == nullptr)
	{
		DeleteSearchNodes(); // delete all path finding nodes
		return;
	}

	if (m_tasks->taskID == TASK_NORMAL)
	{
		DeleteSearchNodes(); // delete all path finding nodes

		m_tasks->data = -1;
		m_chosenGoalIndex = -1;

		return;
	}

	Task* next = m_tasks->next;
	Task* prev = m_tasks->prev;

	if (next != nullptr)
		next->prev = prev;

	if (prev != nullptr)
		prev->next = next;

	delete m_tasks;
	m_tasks = nullptr;

	if (prev != nullptr && next != nullptr)
	{
		if (prev->desire >= next->desire)
			m_tasks = prev;
		else
			m_tasks = next;
	}
	else if (prev != nullptr)
		m_tasks = prev;
	else if (next != nullptr)
		m_tasks = next;

	if (m_tasks == nullptr)
		GetCurrentTask();

	CheckTasksPriorities();
	DeleteSearchNodes();
}

void Bot::CheckGrenadeThrow(void)
{
	if (IsZombieMode() && !m_isZombieBot)
		return;

	if (ebot_knifemode.GetBool() || m_grenadeCheckTime >= engine->GetTime() || m_isUsingGrenade || GetCurrentTask()->taskID == TASK_PLANTBOMB || GetCurrentTask()->taskID == TASK_DEFUSEBOMB || m_isReloading)
	{
		m_states &= ~(STATE_THROWEXPLODE | STATE_THROWFLASH | STATE_THROWSMOKE);
		return;
	}

	m_grenadeCheckTime = AddTime(0.5f);

	int grenadeToThrow = CheckGrenades();
	if (grenadeToThrow == -1)
	{
		m_states &= ~(STATE_THROWEXPLODE | STATE_THROWFLASH | STATE_THROWSMOKE);
		m_grenadeCheckTime = AddTime(10.0f);
		return;
	}

	edict_t* targetEntity = nullptr;
	if (GetGameMode() == MODE_BASE || IsDeathmatchMode())
		targetEntity = m_lastEnemy;
	else if (IsZombieMode())
	{
		if (!FNullEnt(m_moveTargetEntity))
			targetEntity = m_moveTargetEntity;
		else if (!FNullEnt(m_enemy))
			targetEntity = m_enemy;
	}
	else
		targetEntity = m_enemy;

	if (FNullEnt(targetEntity))
	{
		m_states &= ~(STATE_THROWEXPLODE | STATE_THROWFLASH | STATE_THROWSMOKE);
		m_grenadeCheckTime = AddTime(1.0f);
		return;
	}

	if ((grenadeToThrow == WEAPON_HEGRENADE || grenadeToThrow == WEAPON_SMGRENADE) && !(m_states & (STATE_SEEINGENEMY | STATE_THROWEXPLODE | STATE_THROWFLASH | STATE_THROWSMOKE)))
	{
		float distance = (GetEntityOrigin(targetEntity) - pev->origin).GetLength();

		// is enemy to high to throw
		if ((GetEntityOrigin(targetEntity).z > AddFloat(pev->origin.z, 650.0f)) || !(targetEntity->v.flags & (FL_ONGROUND | FL_DUCKING)))
			distance = FLT_MAX; // just some crazy value

		// enemy is within a good throwing distance ?
		if (distance > (grenadeToThrow == WEAPON_SMGRENADE ? 400 : 600) && distance <= 768.0f)
		{
			// care about different types of grenades
			if (grenadeToThrow == WEAPON_HEGRENADE)
			{
				bool allowThrowing = true;

				// check for teammates
				if (GetGameMode() == MODE_BASE && GetNearbyFriendsNearPosition(GetEntityOrigin(targetEntity), 256) > 0)
					allowThrowing = false;

				if (allowThrowing && m_seeEnemyTime + 2.0 < engine->GetTime())
				{
					Vector enemyPredict = AddVector((targetEntity->v.velocity * 0.5).SkipZ(), GetEntityOrigin(targetEntity));
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
						m_throw = g_waypoint->GetPath(searchTab[count--])->origin;
						Vector src = CheckThrow(EyePosition(), m_throw);

						if (src.GetLength() <= 100.0f)
							src = CheckToss(EyePosition(), m_throw);

						if (src == nullvec)
							allowThrowing = false;
						else
							break;
					}
				}

				// start explosive grenade throwing?
				if (allowThrowing)
					m_states |= STATE_THROWEXPLODE;
				else
					m_states &= ~STATE_THROWEXPLODE;
			}
			else if (GetGameMode() == MODE_BASE && grenadeToThrow == WEAPON_SMGRENADE)
			{
				// start smoke grenade throwing?
				if ((m_states & STATE_SEEINGENEMY) && GetShootingConeDeviation(m_enemy, &pev->origin) >= 0.90)
					m_states &= ~STATE_THROWSMOKE;
				else
					m_states |= STATE_THROWSMOKE;
			}
		}
	}
	else if (grenadeToThrow == WEAPON_FBGRENADE && (GetEntityOrigin(targetEntity) - pev->origin).GetLength() <= 800 && !(m_aimFlags & AIM_ENEMY))
	{
		bool allowThrowing = true;
		Array <int> inRadius;

		g_waypoint->FindInRadius(inRadius, 256, AddVector(GetEntityOrigin(targetEntity), (targetEntity->v.velocity * 0.5).SkipZ()));

		ITERATE_ARRAY(inRadius, i)
		{
			if (GetNearbyFriendsNearPosition(g_waypoint->GetPath(i)->origin, 256) != 0)
				continue;

			m_throw = g_waypoint->GetPath(i)->origin;
			Vector src = CheckThrow(EyePosition(), m_throw);

			if (src.GetLength() <= 100)
				src = CheckToss(EyePosition(), m_throw);

			if (src == nullvec)
				continue;

			allowThrowing = true;
			break;
		}

		// start concussion grenade throwing?
		if (allowThrowing)
			m_states |= STATE_THROWFLASH;
		else
			m_states &= ~STATE_THROWFLASH;
	}

	const float randTime = AddTime(engine->RandomFloat(2.0f, 4.0f));

	if (m_states & STATE_THROWEXPLODE)
		PushTask(TASK_THROWHEGRENADE, TASKPRI_THROWGRENADE, -1, randTime, false);
	else if (m_states & STATE_THROWFLASH)
		PushTask(TASK_THROWFBGRENADE, TASKPRI_THROWGRENADE, -1, randTime, false);
	else if (m_states & STATE_THROWSMOKE)
		PushTask(TASK_THROWSMGRENADE, TASKPRI_THROWGRENADE, -1, randTime, false);
}

bool Bot::IsOnAttackDistance(edict_t* targetEntity, float distance)
{
	Vector origin = GetEntityOrigin(GetEntity());
	Vector targetOrigin = GetEntityOrigin(targetEntity);

	if ((origin - targetOrigin).GetLength() < distance)
		return true;

	return false;
}

bool Bot::EnemyIsThreat(void)
{
	if (FNullEnt(m_enemy) || GetCurrentTask()->taskID == TASK_SEEKCOVER)
		return false;

	// if bot is camping, he should be firing anyway and not leaving his position
	if (GetCurrentTask()->taskID == TASK_CAMP && m_zhCampPointIndex == -1)
		return false;

	// if enemy is near or we're aiming to him
	if (IsOnAttackDistance(m_enemy, 256.0f) || (m_currentWaypointIndex != WEAPON_KNIFE && IsInViewCone(GetEntityOrigin(m_enemy))))
		return true;

	return false;
}

bool Bot::ReactOnEnemy(void)
{
	// NO!
	if (IsOnLadder())
		return m_isEnemyReachable = false;

	if (!EnemyIsThreat())
		return m_isEnemyReachable = false;

	if (m_enemyReachableTimer < engine->GetTime())
	{
		Vector enemyHead = GetPlayerHeadOrigin(m_enemy);
		int ownIndex = m_currentWaypointIndex;
		int enemyIndex = g_waypoint->FindNearest(GetEntityOrigin(m_enemy), 9999.0f, -1, GetEntity());
		auto currentWaypoint = g_waypoint->GetPath(m_currentWaypointIndex);

		if (m_isZombieBot)
		{
			float enemyDistance = (pev->origin - enemyHead).GetLength();

			m_isEnemyReachable = false;

			if (!(currentWaypoint->flags & WAYPOINT_FALLRISK))
			{
				m_isEnemyReachable = true;
				goto last;
			}

			if (pev->flags & FL_DUCKING)
			{
				if (enemyDistance < 32.0f)
					m_isEnemyReachable = true;
				goto last;
			}

			// end of the path, before repathing check the distance if we can reach to enemy
			if (m_navNode == nullptr)
			{
				m_isEnemyReachable = (enemyDistance <= 768.0f && IsVisibleForKnifeAttack(enemyHead, GetEntity()));
				if (m_isEnemyReachable)
					goto last;
			}
			else if (ownIndex == enemyIndex)
			{
				m_isEnemyReachable = true;
				goto last;
			}
			else if (g_waypoint->GetPath(ownIndex)->flags & WAYPOINT_FALLRISK)
				goto last;
			else if (IsVisibleForKnifeAttack(enemyHead, GetEntity()))
			{
				float radius = 128.0f;
				if (IsValidWaypoint(ownIndex) && !(currentWaypoint->flags & WAYPOINT_FALLCHECK))
					radius = AddFloat(radius, currentWaypoint->radius);

				if (enemyDistance > radius)
					m_isEnemyReachable = true;

				goto last;
			}
		}
		else
		{
			m_isEnemyReachable = false;

			if (!IsValidWaypoint(ownIndex))
				ownIndex = g_waypoint->FindNearest(pev->origin, 9999.0f, -1, GetEntity());

			extern ConVar ebot_zombie_speed_factor;
			float enemyDistance = (pev->origin - (IsZombieMode() ? (enemyHead + m_enemy->v.velocity * ebot_zombie_speed_factor.GetFloat()) : enemyHead)).GetLength();

			if (IsZombieMode())
			{
				if (enemyIndex == ownIndex)
				{
					m_isEnemyReachable = true;
					goto last;
				}
				else if (pev->flags & FL_DUCKING && m_enemy->v.flags & FL_DUCKING) // danger...
				{
					extern ConVar ebot_zp_escape_distance;
					if (enemyDistance <= AddFloat(fabsf(m_enemy->v.speed), ebot_zp_escape_distance.GetFloat()))
					{
						m_isEnemyReachable = true;
						goto last;
					}
				}
				else if (g_waypoint->GetPath(ownIndex)->flags & WAYPOINT_FALLRISK)
					goto last;
				else if (GetCurrentTask()->taskID == TASK_CAMP)
				{
					if (enemyIndex == m_zhCampPointIndex)
						m_isEnemyReachable = true;
					else
					{
						extern ConVar ebot_zp_escape_distance;
						if (enemyDistance <= AddFloat(fabsf(m_enemy->v.speed), ebot_zp_escape_distance.GetFloat()))
						{
							for (int j = 0; j < Const_MaxPathIndex; j++)
							{
								Vector origin = GetBottomOrigin(GetEntity());

								TraceResult tr;
								TraceLine(Vector(origin.x, origin.y, AddFloat(origin.z, 10.0f)), enemyHead, true, false, GetEntity(), &tr);

								auto enemyWaypoint = g_waypoint->GetPath(enemyIndex);
								if (tr.flFraction == 1.0f && enemyWaypoint->index[j] == ownIndex && !(enemyWaypoint->connectionFlags[j] & PATHFLAG_JUMP))
								{
									m_isEnemyReachable = true;
									break;
								}
							}
						}
					}

					goto last;
				}
				else
				{
					Vector origin = GetBottomOrigin(GetEntity());

					TraceResult tr;
					TraceLine(Vector(origin.x, origin.y, AddFloat(origin.z, 10.0f)), enemyHead, true, false, GetEntity(), &tr);

					// human improve
					extern ConVar ebot_zp_escape_distance;
					if (enemyDistance <= AddFloat(fabsf(m_enemy->v.speed), ebot_zp_escape_distance.GetFloat()) && tr.flFraction == 1.0f)
						m_isEnemyReachable = true;
					else
						m_isEnemyReachable = false;

					goto last;
				}
			}

			float pathDist = g_waypoint->GetPathDistanceFloat(ownIndex, enemyIndex);
			if (pathDist - enemyDistance <= 128.0f && !IsOnLadder())
				m_isEnemyReachable = true;
		}

	last:
		if (m_isEnemyReachable && m_isZombieBot)
			m_enemyReachableTimer = AddTime(engine->RandomFloat(0.15f, 0.50f));
		else
			m_enemyReachableTimer = AddTime(engine->RandomFloat(0.25f, 0.75f));
	}

	if (m_isEnemyReachable)
	{
		m_navTimeset = engine->GetTime(); // override existing movement by attack movement
		return true;
	}

	return false;
}

// don't allow shooting through walls when pausing or camping
bool Bot::LastEnemyShootable(void)
{
	if (!(m_aimFlags & AIM_LASTENEMY) || FNullEnt(m_lastEnemy) || GetCurrentTask()->taskID == TASK_PAUSE || GetCurrentTask()->taskID == TASK_CAMP)
		return false;
	return GetShootingConeDeviation(GetEntity(), &m_lastEnemyOrigin) >= 0.90;
}

void Bot::CheckRadioCommands(void)
{
	if (!m_isSlowThink)
		return;

	if (m_numFriendsLeft == 0)
		return;

	// don't allow bot listen you if bot is busy
	if (GetCurrentTask()->taskID == TASK_DEFUSEBOMB || GetCurrentTask()->taskID == TASK_PICKUPITEM || GetCurrentTask()->taskID == TASK_PLANTBOMB || HasHostage() || m_isBomber || m_isVIP)
	{
		m_radioOrder = 0;
		return;
	}

	if (FNullEnt(m_radioEntity) || !IsAlive(m_radioEntity))
		return;

	// dynamic range :)
	// less bots = more teamwork required
	if (engine->RandomFloat(1.0f, 100.0f) <= 1.0f * m_numFriendsLeft / 3.0f)
		return;

	float distance = (GetEntityOrigin(m_radioEntity) - pev->origin).GetLength();

	switch (m_radioOrder)
	{
	case Radio_FollowMe:
		if (IsVisible(GetTopOrigin(m_radioEntity), GetEntity()) && FNullEnt(m_enemy) && FNullEnt(m_lastEnemy)) // game crashes...
		{
			int numFollowers = 0;

			// check if no more followers are allowed
			for (const auto& client : g_clients)
			{
				Bot* bot = g_botManager->GetBot(client.ent);

				if (bot != nullptr && bot->m_notKilled)
				{
					if (bot->m_targetEntity == m_radioEntity)
						++numFollowers;
				}
			}

			int allowedFollowers = ebot_followuser.GetInt();

			if (m_isBomber)
				allowedFollowers = 1;

			if (numFollowers < allowedFollowers)
			{
				RadioMessage(Radio_Affirmative);
				m_targetEntity = m_radioEntity;

				// don't pause/camp/follow anymore
				BotTask taskID = GetCurrentTask()->taskID;

				if (taskID == TASK_PAUSE || taskID == TASK_CAMP)
					TaskComplete();

				DeleteSearchNodes();
				PushTask(TASK_FOLLOWUSER, TASKPRI_FOLLOWUSER, -1, (pev->origin - GetEntityOrigin(m_radioEntity)).GetLength() / pev->maxspeed, true);
			}
		}
		else
			RadioMessage(Radio_Negative);

		break;

	case Radio_StickTogether:
		if (IsVisible(GetTopOrigin(m_radioEntity), GetEntity()) && FNullEnt(m_enemy) && FNullEnt(m_lastEnemy))
		{
			RadioMessage(Radio_Affirmative);
			m_targetEntity = m_radioEntity;

			// don't pause/camp/follow anymore
			BotTask taskID = GetCurrentTask()->taskID;

			if (taskID == TASK_PAUSE || taskID == TASK_CAMP || taskID == TASK_HIDE || taskID == TASK_GOINGFORCAMP)
				TaskComplete();

			DeleteSearchNodes();

			PushTask(TASK_FOLLOWUSER, TASKPRI_FOLLOWUSER, -1, 20.0f, true);
		}
		else if (FNullEnt(m_enemy) && FNullEnt(m_lastEnemy))
		{
			RadioMessage(Radio_Affirmative);

			m_position = GetEntityOrigin(m_radioEntity);

			DeleteSearchNodes();
			PushTask(TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 1.0f, true);
		}
		else
			RadioMessage(Radio_Negative);

		break;

	case Radio_CoverMe:
		// check if line of sight to object is not blocked (i.e. visible)
		if (IsVisible(pev->origin, m_radioEntity))
		{
			RadioMessage(Radio_Affirmative);

			if (GetCurrentTask()->taskID == TASK_CAMP || GetCurrentTask()->taskID == TASK_PAUSE)
				return;

			int campindex = FindDefendWaypoint(GetTopOrigin(m_radioEntity));

			if (!IsValidWaypoint(campindex))
				return;

			m_campposition = g_waypoint->GetPath(campindex)->origin;

			DeleteSearchNodes();
			PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, campindex, 9999.0f, true);
		}

		break;

	case Radio_HoldPosition:
		if (m_numEnemiesLeft > 0 && !g_waypoint->m_campPoints.IsEmpty())
		{
			int index = FindDefendWaypoint(GetTopOrigin(m_radioEntity));

			if (IsValidWaypoint(index))
			{
				RadioMessage(Radio_Affirmative);

				m_campposition = g_waypoint->GetPath(index)->origin;

				DeleteSearchNodes();
				PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, index, 9999.0f, true);
			}
			else
				RadioMessage(Radio_Negative);
		}
		else
			RadioMessage(Radio_Negative);

		break;

	case Radio_TakingFire:
		if (FNullEnt(m_targetEntity))
		{
			if (FNullEnt(m_enemy) && m_seeEnemyTime + 10.0f < engine->GetTime())
			{
				// decrease fear levels to lower probability of bot seeking cover again
				m_fearLevel -= 0.2f;

				if (m_fearLevel < 0.0f) 
					m_fearLevel = 0.0f;

				RadioMessage(Radio_Affirmative);
				m_targetEntity = m_radioEntity;
				m_position = GetEntityOrigin(m_radioEntity);

				DeleteSearchNodes();
				PushTask(TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, (pev->origin - GetEntityOrigin(m_radioEntity)).GetLength() / pev->maxspeed, true);
			}
			else
				RadioMessage(Radio_Negative);
		}

		break;

	case Radio_YouTakePoint:
		if (IsVisible(GetTopOrigin(m_radioEntity), GetEntity()) && m_isLeader)
		{
			RadioMessage(Radio_Affirmative);

			if (g_bombPlanted)
				m_position = g_waypoint->GetBombPosition();
			else
				m_position = GetEntityOrigin(m_radioEntity);

			if (ChanceOf(50))
			{
				DeleteSearchNodes();
				PushTask(TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 1.0f, true);
			}
			else
			{
				int index = FindDefendWaypoint(m_position);

				if (IsValidWaypoint(index) && !IsWaypointOccupied(index))
				{
					m_campposition = g_waypoint->GetPath(index)->origin;

					DeleteSearchNodes();
					PushTask(TASK_GOINGFORCAMP, TASKPRI_MOVETOPOSITION, -1, 1.0f, true);
				}
				else
				{
					DeleteSearchNodes();
					PushTask(TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 1.0f, true);
				}
			}
			
		}
		else
			RadioMessage(Radio_Negative);

		break;

	case Radio_EnemySpotted:
		if (m_personality == PERSONALITY_RUSHER && FNullEnt(m_enemy) && FNullEnt(m_lastEnemy)) // rusher bots will like that, they want fight!
		{
			RadioMessage(Radio_Affirmative);

			m_position = GetEntityOrigin(m_radioEntity);

			DeleteSearchNodes();
			PushTask(TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 1.0f, true);
		}
		else if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_GOAL) // he's in goal waypoint, its danger!!!
		{
			RadioMessage(Radio_Affirmative);

			m_position = GetEntityOrigin(m_radioEntity);

			DeleteSearchNodes();
			PushTask(TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 1.0f, true);
		}

		break;

	case Radio_NeedBackup:
		if ((FNullEnt(m_enemy) && IsVisible(GetPlayerHeadOrigin(m_radioEntity), GetEntity()) || distance <= 1536.0f || !m_moveToC4) && m_seeEnemyTime + 5.0f <= engine->GetTime())
		{
			m_fearLevel -= 0.1f;

			if (m_fearLevel < 0.0f)
				m_fearLevel = 0.0f;

			RadioMessage(Radio_Affirmative);

			m_targetEntity = m_radioEntity;
			m_position = GetEntityOrigin(m_radioEntity);

			DeleteSearchNodes();
			PushTask(TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, (pev->origin - GetEntityOrigin(m_radioEntity)).GetLength() / pev->maxspeed, true);
		}
		else
			RadioMessage(Radio_Negative);

		break;

	case Radio_GoGoGo:
		if (m_radioEntity == m_targetEntity)
		{
			RadioMessage(Radio_Affirmative);

			m_targetEntity = nullptr;
			m_fearLevel -= 0.2f;

			if (m_fearLevel < 0.0f)
				m_fearLevel = 0.0f;

			if (GetCurrentTask()->taskID == TASK_CAMP || GetCurrentTask()->taskID == TASK_PAUSE || GetCurrentTask()->taskID == TASK_HIDE || GetCurrentTask()->taskID == TASK_GOINGFORCAMP)
				TaskComplete();
		}
		else if (FNullEnt(m_enemy) && IsVisible(GetTopOrigin(m_radioEntity), GetEntity()) || distance <= 1536.0f)
		{
			BotTask taskID = GetCurrentTask()->taskID;

			if (taskID == TASK_PAUSE || taskID == TASK_CAMP)
			{
				TaskComplete();

				m_fearLevel -= 0.2f;

				if (m_fearLevel < 0.0f)
					m_fearLevel = 0.0f;

				RadioMessage(Radio_Affirmative);

				m_targetEntity = nullptr;
				m_position = GetTopOrigin(m_radioEntity) + (m_radioEntity->v.v_angle + (g_pGlobals->v_forward * 20));

				DeleteSearchNodes();
				PushTask(TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 1.0f, true);
			}
		}
		else if (!FNullEnt(m_doubleJumpEntity))
		{
			RadioMessage(Radio_Affirmative);
			ResetDoubleJumpState();
		}
		else
			RadioMessage(Radio_Negative);

		break;

	case Radio_ShesGonnaBlow:
		if (FNullEnt(m_enemy) && distance <= 2048.0f && g_bombPlanted && m_team == TEAM_TERRORIST)
		{
			RadioMessage(Radio_Affirmative);

			if (GetCurrentTask()->taskID == TASK_CAMP || GetCurrentTask()->taskID == TASK_PAUSE)
				TaskComplete();

			m_targetEntity = nullptr;
			PushTask(TASK_ESCAPEFROMBOMB, TASKPRI_ESCAPEFROMBOMB, -1, GetBombTimeleft(), true);
		}
		else
			RadioMessage(Radio_Negative);

		break;

	case Radio_RegroupTeam:
		// if no more enemies found AND bomb planted, switch to knife to get to bombplace faster
		if (m_team == TEAM_COUNTER && m_currentWeapon != WEAPON_KNIFE && m_numEnemiesLeft == 0 && g_bombPlanted && GetCurrentTask()->taskID != TASK_DEFUSEBOMB)
		{
			SelectWeaponByName("weapon_knife");

			DeleteSearchNodes();

			m_position = g_waypoint->GetBombPosition();
			PushTask(TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 1.0f, true);

			RadioMessage(Radio_Affirmative);
		}
		else if (m_team == TEAM_TERRORIST && m_numEnemiesLeft > 0 && g_bombPlanted && GetCurrentTask()->taskID != TASK_PLANTBOMB && GetCurrentTask()->taskID != TASK_CAMP)
		{
			TaskComplete();

			if (!m_isReloading)
				SelectBestWeapon();

			DeleteSearchNodes();

			m_position = g_waypoint->GetBombPosition();
			PushTask(TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 1.0f, true);

			RadioMessage(Radio_Affirmative);
		}
		else if (FNullEnt(m_enemy) && FNullEnt(m_lastEnemy))
		{
			TaskComplete();

			if (!m_isReloading)
				SelectBestWeapon();

			DeleteSearchNodes();

			m_position = GetEntityOrigin(m_radioEntity);
			PushTask(TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 1.0f, true);

			RadioMessage(Radio_Affirmative);
		}
		else
			RadioMessage(Radio_Negative);

		break;

	case Radio_StormTheFront:
		if ((FNullEnt(m_enemy) && IsVisible(GetTopOrigin(m_radioEntity), GetEntity())) || distance < 1024.0f)
		{
			RadioMessage(Radio_Affirmative);

			BotTask taskID = GetCurrentTask()->taskID;
			if (taskID == TASK_PAUSE || taskID == TASK_CAMP)
				TaskComplete();

			m_targetEntity = nullptr;
			m_position = GetEntityOrigin(m_radioEntity) + (m_radioEntity->v.v_angle * engine->RandomFloat(10.0f, 20.0f));

			DeleteSearchNodes();
			PushTask(TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 1.0f, true);

			m_fearLevel -= 0.3f;

			if (m_fearLevel < 0.0f)
				m_fearLevel = 0.0f;

			m_agressionLevel += 0.3f;

			if (m_agressionLevel > 1.0f)
				m_agressionLevel = 1.0f;
		}

		break;

	case Radio_Fallback:
		if ((FNullEnt(m_enemy) && IsVisible(GetPlayerHeadOrigin(m_radioEntity), GetEntity())) || distance <= 1024.0f)
		{
			m_fearLevel += 0.5f;

			if (m_fearLevel > 1.0f)
				m_fearLevel = 1.0f;

			m_agressionLevel -= 0.5f;

			if (m_agressionLevel < 0.0f)
				m_agressionLevel = 0.0f;

			if (GetCurrentTask()->taskID == TASK_CAMP && !FNullEnt(m_lastEnemy))
			{
				RadioMessage(Radio_Negative);
				GetCurrentTask()->time += engine->RandomFloat(ebot_camp_min.GetFloat(), ebot_camp_min.GetFloat());
			}
			else
			{
				BotTask taskID = GetCurrentTask()->taskID;
				if (taskID == TASK_PAUSE || taskID == TASK_CAMP || taskID == TASK_HIDE || taskID == TASK_HUNTENEMY || taskID == TASK_GOINGFORCAMP)
				{
					RadioMessage(Radio_Affirmative);
					TaskComplete();
				}

				m_targetEntity = nullptr;
				m_seeEnemyTime = engine->GetTime();

				// if bot has no enemy
				if (FNullEnt(m_lastEnemy))
				{
					float nearestDistance = 9999999.0f;

					// take nearest enemy to ordering player
					for (const auto& client : g_clients)
					{
						if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team == m_team)
							continue;

						auto origin = GetEntityOrigin(client.ent);
						float curDist = (m_radioEntity->v.origin - origin).GetLength();

						if (curDist < nearestDistance)
						{
							nearestDistance = curDist;

							m_lastEnemy = client.ent;
							m_lastEnemyOrigin = origin + pev->view_ofs;
						}
					}
				}
				else
				{
					RadioMessage(Radio_Affirmative);

					int seekindex = FindCoverWaypoint(9999.0f);
					if (IsValidWaypoint(seekindex))
						PushTask(TASK_SEEKCOVER, TASKPRI_SEEKCOVER, seekindex, 3.0f, true);

					DeleteSearchNodes();
				}
			}
		}

		break;

	case Radio_ReportTeam:
		switch (GetCurrentTask()->taskID)
		{
		case TASK_NORMAL:
			if (IsValidWaypoint(GetCurrentTask()->data))
			{
				if (!FNullEnt(m_enemy))
				{
					if (IsAlive(m_enemy))
						RadioMessage(Radio_EnemySpotted);
					else
						RadioMessage(Radio_EnemyDown);
				}
				else if (!FNullEnt(m_lastEnemy))
				{
					if (IsAlive(m_lastEnemy))
						RadioMessage(Radio_EnemySpotted);
					else
						RadioMessage(Radio_EnemyDown);
				}
				else if (m_seeEnemyTime + 10.0f > engine->GetTime())
					RadioMessage(Radio_EnemySpotted);
				else
					RadioMessage(Radio_SectorClear);
			}

			break;

		case TASK_MOVETOPOSITION:
			if (m_seeEnemyTime + 10.0f > engine->GetTime())
				RadioMessage(Radio_EnemySpotted);
			else if(FNullEnt(m_enemy) && FNullEnt(m_lastEnemy))
				RadioMessage(Radio_SectorClear);

			break;

		case TASK_CAMP:
			RadioMessage(Radio_InPosition);

			break;

		case TASK_GOINGFORCAMP:
			RadioMessage(Radio_HoldPosition);

			break;

		case TASK_PLANTBOMB:
			RadioMessage(Radio_HoldPosition);

			break;

		case TASK_DEFUSEBOMB:
			RadioMessage(Radio_CoverMe);

			break;

		case TASK_FIGHTENEMY:
			if (!FNullEnt(m_enemy))
			{
				if (IsAlive(m_enemy))
					RadioMessage(Radio_EnemySpotted);
				else
					RadioMessage(Radio_EnemyDown);
			}
			else if (m_seeEnemyTime + 10.0f > engine->GetTime())
				RadioMessage(Radio_EnemySpotted);

			break;

		default:
			if (ChanceOf(15))
				RadioMessage(Radio_ReportingIn);
			else if (ChanceOf(15))
				RadioMessage(Radio_FollowMe);
			else if (m_seeEnemyTime + 10.0f > engine->GetTime())
				RadioMessage(Radio_EnemySpotted);
			else
				RadioMessage(Radio_Negative);

			break;
		}

		break;

	case Radio_SectorClear:
		// is bomb planted and it's a ct
		if (!g_bombPlanted)
			break;

		// check if it's a ct command
		if (GetTeam(m_radioEntity) == TEAM_COUNTER && m_team == TEAM_COUNTER && IsValidBot(m_radioEntity) && g_timeNextBombUpdate < engine->GetTime())
		{
			float minDistance = 9999.0f;
			int bombPoint = -1;

			// find nearest bomb waypoint to player
			ITERATE_ARRAY(g_waypoint->m_goalPoints, i)
			{
				distance = (g_waypoint->GetPath(g_waypoint->m_goalPoints[i])->origin - GetEntityOrigin(m_radioEntity)).GetLength();

				if (distance < minDistance)
				{
					minDistance = distance;
					bombPoint = g_waypoint->m_goalPoints[i];
				}
			}

			// mark this waypoint as restricted point
			if (IsValidWaypoint(bombPoint) && !g_waypoint->IsGoalVisited(bombPoint))
			{
				// does this bot want to defuse?
				if (GetCurrentTask()->taskID == TASK_NORMAL)
				{
					// is he approaching this goal?
					if (GetCurrentTask()->data == bombPoint)
					{
						GetCurrentTask()->data = -1;
						RadioMessage(Radio_Affirmative);
					}
				}

				g_waypoint->SetGoalVisited(bombPoint);
			}

			g_timeNextBombUpdate = engine->GetTime() + 1.0f;
		}

		break;

	case Radio_GetInPosition:
		if ((FNullEnt(m_enemy) && IsVisible(GetTopOrigin(m_radioEntity), GetEntity())) || distance <= 1024.0f)
		{
			RadioMessage(Radio_Affirmative);

			if (GetCurrentTask()->taskID == TASK_CAMP)
				GetCurrentTask()->time = engine->GetTime() + engine->RandomFloat(ebot_camp_min.GetFloat(), ebot_camp_max.GetFloat());
			else
			{
				BotTask taskID = GetCurrentTask()->taskID;
				if (taskID == TASK_PAUSE || taskID == TASK_CAMP)
					TaskComplete();

				m_targetEntity = nullptr;
				m_seeEnemyTime = engine->GetTime();

				// if bot has no enemy
				if (FNullEnt(m_lastEnemy))
				{
					float nearestDistance = 9999.0f;

					// take nearest enemy to ordering player
					for (const auto& client : g_clients)
					{
						if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_USED) || client.team == m_team)
							continue;

						auto origin = GetEntityOrigin(client.ent);
						float dist = (m_radioEntity->v.origin - origin).GetLength();

						if (dist < nearestDistance)
						{
							nearestDistance = dist;
							m_lastEnemy = client.ent;
							m_lastEnemyOrigin = origin + pev->view_ofs;
						}
					}
				}

				DeleteSearchNodes();

				int index = FindDefendWaypoint(GetTopOrigin(m_radioEntity));
				m_campposition = g_waypoint->GetPath(index)->origin;
				PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, index, engine->GetTime() + 9999.0f, true);
				m_campButtons |= IN_DUCK;
			}
		}

		break;
	}

	m_radioOrder = 0; // radio command has been handled, reset
}

void Bot::SelectLeaderEachTeam(int team)
{
	Bot* botLeader = nullptr;

	if (GetGameMode() == MODE_BASE || GetGameMode() == MODE_TDM)
	{
		if (team == TEAM_TERRORIST && !g_leaderChoosen[TEAM_TERRORIST])
		{
			botLeader = g_botManager->GetHighestSkillBot(team);

			if (botLeader != nullptr)
			{
				botLeader->m_isLeader = true;

				if (ChanceOf(10))
					botLeader->PlayChatterMessage(ChatterMessage::Happy);
				else if (ChanceOf(40))
					botLeader->RadioMessage(Radio_FollowMe);
			}
		}
		else if (team == TEAM_COUNTER && !g_leaderChoosen[TEAM_COUNTER])
		{
			botLeader = g_botManager->GetHighestSkillBot(team);

			if (botLeader != nullptr)
			{
				botLeader->m_isLeader = true;

				if (ChanceOf(10))
					botLeader->PlayChatterMessage(ChatterMessage::Happy);
				else if (ChanceOf(40))
					botLeader->RadioMessage(Radio_FollowMe);
			}
		}
	}
}

float Bot::GetWalkSpeed(void)
{
	if (GetGameMode() == MODE_ZH || GetGameMode() == MODE_ZP ||
		pev->maxspeed <= 180.f || m_currentTravelFlags & PATHFLAG_JUMP ||
		pev->button & IN_JUMP || pev->oldbuttons & IN_JUMP ||
		pev->flags & FL_DUCKING || pev->button & IN_DUCK || pev->oldbuttons & IN_DUCK || IsInWater())
		return pev->maxspeed;
	return static_cast <float> ((static_cast <int> (pev->maxspeed) * 0.5f) + (static_cast <int> (pev->maxspeed) / 50)) - 18;
}

bool Bot::IsAntiBlock(edict_t* entity)
{
	if (entity->v.solid == SOLID_NOT) // auto mode
		return true;
	return false;
}

bool Bot::IsNotAttackLab(edict_t* entity)
{
	if (FNullEnt(entity))
		return true;

	if (entity->v.takedamage == DAMAGE_NO)
		return true;

	if (entity->v.rendermode == kRenderTransAlpha)
	{
		float renderamt = entity->v.renderamt;

		if (renderamt <= 30)
			return true;

		if (renderamt > 160)
			return false;

		float enemy_distance = (GetEntityOrigin(entity) - pev->origin).GetLength();
		return (renderamt <= (enemy_distance / 5));
	}

	return false;
}

void Bot::ChooseAimDirection(void)
{
	if (!m_canChooseAimDirection)
		return;

	if (m_aimStopTime > engine->GetTime() && FNullEnt(m_enemy) && FNullEnt(m_breakableEntity) && FNullEnt(m_enemyAPI))
		return;

	TraceResult tr;
	memset(&tr, 0, sizeof(TraceResult));

	unsigned int flags = m_aimFlags;

	if (!IsValidWaypoint(m_currentWaypointIndex))
		GetValidWaypoint();
	else if (m_isZombieBot && HasNextPath() && g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_ZOMBIEPUSH)
	{
		m_lookAt = pev->flags & FL_DUCKING ? m_waypointOrigin : m_destOrigin + m_moveAngles * m_frameInterval;
		m_lookAt.z = EyePosition().z;
		return;
	}

	// check if last enemy vector valid
	if (!IsZombieMode())
	{
		if (m_lastEnemyOrigin != nullvec)
		{
			if (FNullEnt(m_enemy) && (pev->origin - m_lastEnemyOrigin).GetLength() >= 1600.0f && m_seeEnemyTime + 7.0f < engine->GetTime())
			{
				TraceLine(EyePosition(), m_lastEnemyOrigin, true, true, GetEntity(), &tr);
				if (!UsesSniper() || (tr.flFraction <= 0.2f && tr.pHit == g_hostEntity))
				{
					if ((m_aimFlags & (AIM_LASTENEMY | AIM_PREDICTENEMY)) && m_wantsToFire)
						m_wantsToFire = false;

					m_lastEnemyOrigin = nullvec;
					m_aimFlags &= ~(AIM_LASTENEMY | AIM_PREDICTENEMY);

					flags &= ~(AIM_LASTENEMY | AIM_PREDICTENEMY);
				}
			}
		}
		else
		{
			m_aimFlags &= ~(AIM_LASTENEMY | AIM_PREDICTENEMY);
			flags &= ~(AIM_LASTENEMY | AIM_PREDICTENEMY);
		}

		// don't allow bot to look at danger positions under certain circumstances
		if (!(flags & (AIM_GRENADE | AIM_ENEMY | AIM_ENTITY)))
		{
			if (IsOnLadder() || IsInWater() || (m_waypointFlags & WAYPOINT_LADDER) || (m_currentTravelFlags & PATHFLAG_JUMP))
				flags &= ~(AIM_LASTENEMY | AIM_PREDICTENEMY);
		}
	}

	if (flags & AIM_OVERRIDE)
	{
		m_aimStopTime = 0.0f;
		m_lookAt = m_camp;
	}
	else if (flags & AIM_GRENADE)
		m_lookAt = m_throw + Vector(0.0f, 0.0f, 1.0f * m_grenade.z);
	else if (flags & AIM_ENEMY)
	{
		FocusEnemy();
		if (m_currentWeapon == WEAPON_KNIFE)
			SelectBestWeapon();
	}
	else if (flags & AIM_ENTITY)
		m_lookAt = m_entity;
	else if (flags & AIM_LASTENEMY)
	{
		if (IsZombieMode())
		{
			if (m_nextCampDirTime < engine->GetTime())
			{
				if (m_seeEnemyTime + 2.0f + m_difficulty > engine->GetTime() && m_lastEnemyOrigin != nullvec)
				{
					m_lookAt = m_lastEnemyOrigin;
					return;
				}

				if (m_lastEnemyOrigin != nullvec && ChanceOf(30))
					m_lookAt = m_lastEnemyOrigin;
				else
				{
					int aimIndex = GetCampAimingWaypoint();
					if (IsValidWaypoint(aimIndex))
						m_lookAt = g_waypoint->GetPath(aimIndex)->origin;
				}

				m_nextCampDirTime = engine->GetTime() + engine->RandomFloat(1.5f, 5.0f);
			}
		}
		else
			m_lookAt = m_lastEnemyOrigin;
	}
	else if (flags & AIM_PREDICTENEMY)
	{
		TraceLine(EyePosition(), m_lastEnemyOrigin, true, true, GetEntity(), &tr);
		if (((pev->origin - m_lastEnemyOrigin).GetLength() < 1600.0f || UsesSniper()) && (tr.flFraction >= 0.2f || tr.pHit != g_worldEdict))
		{
			bool recalcPath = true;

			if (!FNullEnt(m_lastEnemy) && m_trackingEdict == m_lastEnemy && m_timeNextTracking < engine->GetTime())
				recalcPath = false;

			if (recalcPath)
			{
				m_lookAt = g_waypoint->GetPath(GetAimingWaypoint(m_lastEnemyOrigin))->origin;
				m_camp = m_lookAt;

				m_timeNextTracking = engine->GetTime() + 0.5f;
				m_trackingEdict = m_lastEnemy;

				// feel free to fire if shoot able
				if (LastEnemyShootable())
					m_wantsToFire = true;
			}
			else
				m_lookAt = m_camp;
		}
		else // forget an enemy far away
			m_aimFlags &= ~AIM_PREDICTENEMY;
	}
	else if (flags & AIM_CAMP)
	{
		m_aimFlags &= ~AIM_NAVPOINT;
		if (m_damageTime + float((m_skill + 55) / 20) > engine->GetTime())
			m_lookAt = m_lastDamageOrigin;
		else if (m_seeEnemyTime + float((m_skill + 55) / 20) > engine->GetTime())
			m_lookAt = m_lastEnemyOrigin;
		else
		{
			int dangerIndex = g_exp.GetDangerIndex(m_team, m_currentWaypointIndex, m_currentWaypointIndex);
			if (IsValidWaypoint(dangerIndex) && IsVisible(g_waypoint->GetPath(dangerIndex)->origin, GetEntity()))
			{
				if ((g_waypoint->GetPath(dangerIndex)->origin - pev->origin).GetLength2D() <= 256.0f)
					m_lookAt = m_camp;
				else
					m_lookAt = g_waypoint->GetPath(dangerIndex)->origin + pev->view_ofs;
			}
			else
				m_lookAt = m_camp;
		}
	}
	else if (flags & AIM_NAVPOINT)
	{
		if (!IsValidWaypoint(m_currentWaypointIndex))
			return;

		if (!FNullEnt(m_breakableEntity) && m_breakableEntity->v.health > 0.0f)
		{
			m_lookAt = m_breakable;
			extern ConVar ebot_escape;
			if (ebot_escape.GetBool())
				m_breakableEntity->v.health = 0.0f;
		}
		else if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_LADDER)
		{
			m_aimStopTime = 0.0f;
			m_lookAt = m_destOrigin + pev->view_ofs;
		}
		else if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_USEBUTTON)
		{
			edict_t* button = FindButton();
			m_aimStopTime = 0.0f;
			if (button != nullptr)
				m_lookAt = GetEntityOrigin(button);
			else
				m_lookAt = m_destOrigin + pev->view_ofs;
		}
		else if (m_isZombieBot || m_currentWeapon == WEAPON_KNIFE)
		{
			if (HasNextPath())
				m_lookAt = g_waypoint->GetPath(m_navNode->next->index)->origin + pev->velocity + pev->view_ofs;
			else
				m_lookAt = m_destOrigin + pev->velocity + pev->view_ofs;
			m_lookAt.z = EyePosition().z;
		}
		else if (!(m_currentTravelFlags & PATHFLAG_JUMP) && !FNullEnt(m_moveTargetEntity) && m_currentWaypointIndex == m_prevGoalIndex)
			m_lookAt = GetTopOrigin(m_moveTargetEntity);
		else if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CAMP)
			m_lookAt = m_camp;
		else if (m_damageTime + float((m_skill + 50) / 25) > engine->GetTime())
			m_lookAt = m_lastDamageOrigin;
	    else if (m_seeEnemyTime + float((m_skill + 50) / 25) > engine->GetTime())
		{
			if (!FNullEnt(m_lastEnemy) && IsAlive(m_lastEnemy) && m_seeEnemyTime + float(m_skill / 27) > engine->GetTime())
				m_lookAt = m_lastEnemyOrigin;
			else
			{
				int dangerIndex = g_exp.GetDangerIndex(m_team, m_currentWaypointIndex, m_currentWaypointIndex);
				if (IsValidWaypoint(dangerIndex) && IsVisible(g_waypoint->GetPath(dangerIndex)->origin, GetEntity()))
				{
					if ((g_waypoint->GetPath(dangerIndex)->origin - pev->origin).GetLength2D() <= 256.0f)
					{
						m_lookAt = m_destOrigin + pev->view_ofs;
						m_lookAt.z = EyePosition().z;
					}
					else
						m_lookAt = g_waypoint->GetPath(dangerIndex)->origin + pev->view_ofs;
				}
			}
		}
		else if (HasNextPath())
			m_lookAt = g_waypoint->GetPath(m_navNode->next->index)->origin + pev->velocity + pev->view_ofs;
		else
			m_lookAt = m_destOrigin + pev->velocity + pev->view_ofs;
		m_lookAt.z = EyePosition().z;
	}

	if (!IsValidWaypoint(m_currentWaypointIndex))
		return;

	if (m_lookAt == nullvec)
	{
		if (HasNextPath())
			m_lookAt = g_waypoint->GetPath(m_navNode->next->index)->origin + pev->velocity + pev->view_ofs;
		else
			m_lookAt = m_destOrigin + pev->velocity + pev->view_ofs;

		if (!(g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_LADDER))
			m_lookAt.z = EyePosition().z;
		else
			m_aimStopTime = 0.0f;
	}
}

void Bot::Think(void)
{
	if (!m_buyingFinished)
		ResetCollideState();

	pev->button = 0;
	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;

	m_canChooseAimDirection = true;
	
	m_notKilled = IsAlive(GetEntity());

	m_frameInterval = engine->GetTime() - m_lastThinkTime;
	m_lastThinkTime = engine->GetTime();

	if (ebot_use_radio.GetInt() != 2 || m_chatterTimer < engine->GetTime())
		SwitchChatterIcon(false);

	if (m_slowthinktimer < engine->GetTime())
	{
		if (!(pev->flags & FL_FAKECLIENT))
			pev->flags |= FL_FAKECLIENT;

		if (m_stayTime <= 0.0f)
		{
			extern ConVar ebot_random_join_quit;
			if (ebot_random_join_quit.GetBool())
			{
				extern ConVar ebot_stay_min;
				extern ConVar ebot_stay_max;
				m_stayTime = engine->GetTime() + RANDOM_FLOAT(ebot_stay_min.GetFloat(), ebot_stay_max.GetFloat());
			}
			else
				m_stayTime = engine->GetTime() + 999999.0f;
		}

		if (m_isZombieBot && m_damageTime + 10.0f > engine->GetTime() && g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_ZOMBIEPUSH)
			m_zombiePush = true;
		else
			m_zombiePush = false;

		m_isBlocked = false;
		m_isSlowThink = true;

		m_isZombieBot = IsZombieEntity(GetEntity());
		m_team = GetTeam(GetEntity());
		m_isBomber = pev->weapons & (1 << WEAPON_C4);

		if (!FNullEnt(m_lastEnemy) && !IsAlive(m_lastEnemy))
			m_lastEnemy = nullptr;

		// at least walk randomly
		if (!IsZombieMode() && !IsValidWaypoint(GetCurrentTask()->data))
			m_chosenGoalIndex = engine->RandomInt(0, g_numWaypoints - 1);

		if (m_slowthinktimer < engine->GetTime())
			m_slowthinktimer = engine->GetTime() + engine->RandomInt(0.9f, 1.1f);

		CalculatePing();
	}
	else
		m_isSlowThink = false;

	// is bot movement enabled
	bool botMovement = false;

	if (m_notStarted) // if the bot hasn't selected stuff to start the game yet, go do that...
		StartGame(); // select team & class
	else if (!m_notKilled)
	{
		extern ConVar ebot_chat;
		extern ConVar ebot_random_join_quit;

		if (ebot_random_join_quit.GetBool() && m_stayTime > 0.0f && m_stayTime < engine->GetTime())
		{
			Kick();
			return;
		}

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
					if (strncmp(m_sayTextBuffer.lastUsedSentences[i], pickedPhrase, m_sayTextBuffer.lastUsedSentences[i].GetLength()) == 0)
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
				if (m_sayTextBuffer.lastUsedSentences.GetElementNumber() > engine->RandomInt(4, 6))
					m_sayTextBuffer.lastUsedSentences.RemoveAll();
			}
		}

		// clear the data
		if (HasNextPath())
		{
			DeleteSearchNodes();
			return;
		}
	}
	else if (m_buyingFinished && !(pev->maxspeed < 10.0f && GetCurrentTask()->taskID != TASK_PLANTBOMB && GetCurrentTask()->taskID != TASK_DEFUSEBOMB) && !ebot_stopbots.GetBool())
		botMovement = true;

	if (m_randomattacktimer < engine->GetTime() && !engine->IsFriendlyFireOn() && !HasHostage()) // simulate players with random knife attacks
	{
		if (m_isStuck && m_personality != PERSONALITY_CAREFUL)
		{
			if (m_personality == PERSONALITY_RUSHER)
				m_randomattacktimer = 0.0f;
			else
				m_randomattacktimer = engine->GetTime() + engine->RandomFloat(0.1f, 10.0f);
		}
		else if (m_personality == PERSONALITY_RUSHER)
			m_randomattacktimer = engine->GetTime() + engine->RandomFloat(0.1f, 30.0f);
		else if (m_personality == PERSONALITY_CAREFUL)
			m_randomattacktimer = engine->GetTime() + engine->RandomFloat(10.0f, 100.0f);
		else
			m_randomattacktimer = engine->GetTime() + engine->RandomFloat(0.15f, 75.0f);

		if (m_currentWeapon == WEAPON_KNIFE)
		{
			if (engine->RandomInt(1, 3) == 1)
				pev->button |= IN_ATTACK;
			else
				pev->button |= IN_ATTACK2;

			if (engine->RandomInt(1, 10) == 1)
				PlayChatterMessage(ChatterMessage::Happy);
		}
	}

	static float secondThinkTimer = 0.0f;

	// check is it time to execute think (called in every 2 seconds (not frame))
	if (secondThinkTimer < engine->GetTime())
	{
		SecondThink();
		secondThinkTimer = engine->GetTime() + 2.0f;
	}

	CheckMessageQueue(); // check for pending messages

	if (botMovement && m_notKilled)
	{
		if (GetGameMode() == MODE_BASE || IsDeathmatchMode() || GetGameMode() == MODE_NOTEAM || IsZombieMode())
			BotAI();
		else
			FunBotAI();

		MoveAction();
		DebugModeMsg();
	}

	if (!FNullEnt(m_enemy))
		m_seeEnemyTime = engine->GetTime();
}

// this function is called from main think function every 2 second (second not frame)
void Bot::SecondThink(void)
{
	m_numEnemiesLeft = GetNearbyEnemiesNearPosition(pev->origin, 99999);
	m_numFriendsLeft = GetNearbyFriendsNearPosition(pev->origin, 99999);

	if (ebot_use_flare.GetInt() == 1 && !m_isReloading && !m_isZombieBot && GetGameMode() == MODE_ZP && FNullEnt(m_enemy) && !FNullEnt(m_lastEnemy))
	{
		if (pev->weapons & (1 << WEAPON_SMGRENADE) && ChanceOf(50))
			PushTask(TASK_THROWFLARE, TASKPRI_THROWGRENADE, -1, engine->RandomFloat(0.6f, 0.9f), false);
	}

	// zp & biohazard flashlight support
	if (ebot_force_flashlight.GetInt() == 1 && !(pev->effects & EF_DIMLIGHT))
		pev->impulse = 100;

	if (m_voteMap != 0) // host wants the bots to vote for a map?
	{
		FakeClientCommand(GetEntity(), "votemap %d", m_voteMap);
		m_voteMap = 0;
	}

	if (g_bombPlanted && m_team == TEAM_COUNTER && (pev->origin - g_waypoint->GetBombPosition()).GetLength() <= 768.0f && !IsBombDefusing(g_waypoint->GetBombPosition()))
		ResetTasks();
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

	for (int i = 1; i <= engine->GetMaxClients(); i++)
	{
		edict_t* ent = INDEXENT(i);

		if (!IsValidPlayer(ent))
			continue;

		numHumans++;

		int ping, loss;
		PLAYER_CNX_STATS(ent, &ping, &loss);

		if (ping <= 0 || ping > 150)
			ping = engine->RandomInt(5, 50);

		averagePing += ping;
	}

	if (numHumans > 0)
		averagePing /= numHumans;
	else
		averagePing = engine->RandomInt(30, 60);

	int botPing = m_basePingLevel + engine->RandomInt(averagePing - averagePing * 0.2f, averagePing + averagePing * 0.2f) + engine->RandomInt(m_difficulty + 3, m_difficulty + 6);

	if (botPing <= 9)
		botPing = engine->RandomInt(9, 19);
	else if (botPing > 133)
		botPing = engine->RandomInt(99, 119);

	for (int j = 0; j < 2; j++)
	{
		for (m_pingOffset[j] = 0; m_pingOffset[j] < 4; m_pingOffset[j]++)
		{
			if ((botPing - m_pingOffset[j]) % 4 == 0)
			{
				m_ping[j] = (botPing - m_pingOffset[j]) / 4;
				break;
			}
		}
	}

	m_ping[2] = botPing;
}

void Bot::MoveAction(void)
{
	if (m_moveAIAPI)
	{
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;
		ResetCollideState();
	}

	// careful bots will stop moving when reloading if they see enemy before, they will scared!
	if(m_personality == PERSONALITY_CAREFUL && m_isReloading && FNullEnt(m_enemy) && !FNullEnt(m_lastEnemy) && IsAlive(m_lastEnemy) && !IsZombieMode())
	{
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;
		ResetCollideState();

		return;
	}

	if (!(pev->button & (IN_FORWARD | IN_BACK)))
	{
		if (m_moveSpeed > 0)
			pev->button |= IN_FORWARD;
		else if (m_moveSpeed < 0)
			pev->button |= IN_BACK;
	}

	if (!(pev->button & (IN_MOVELEFT | IN_MOVERIGHT)))
	{
		if (m_strafeSpeed > 0)
			pev->button |= IN_MOVERIGHT;
		else if (m_strafeSpeed < 0)
			pev->button |= IN_MOVELEFT;
	}
}

void Bot::TaskNormal(int i, int destIndex, Vector src)
{
	m_aimFlags |= AIM_NAVPOINT;

	// we're stuck while trying reach to human camp waypoint?
	if (IsZombieMode() && m_isStuck)
	{
		if (IsValidWaypoint(m_zhCampPointIndex) && (g_waypoint->GetPath(m_zhCampPointIndex)->origin - pev->origin).GetLength() <= 128.0f)
		{
			TraceResult tr2;
			TraceLine(pev->origin, g_waypoint->GetPath(m_zhCampPointIndex)->origin, true, false, GetEntity(), &tr2);

			// nothing blocking visibility, we can camp at here if we're blocked by own teammates
			if (tr2.flFraction == 1.0f)
				PushTask(TASK_CAMP, TASKPRI_CAMP, m_zhCampPointIndex, 9999.0f, false);
		}
	}

	if (IsValidWaypoint(m_currentWaypointIndex) && (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CAMP || g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_GOAL) && !g_bombPlanted && !HasHostage() && !m_isZombieBot && !m_isBomber && !m_isVIP && ChanceOf(m_personality == PERSONALITY_RUSHER ? 7 : m_personality == PERSONALITY_CAREFUL ? 35 : 15))
	{
		int index = FindDefendWaypoint(g_waypoint->GetPath(m_currentWaypointIndex)->origin);

		m_campposition = g_waypoint->GetPath(index)->origin;

		PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, index, ebot_camp_max.GetFloat(), false);
	}

	if ((g_mapType & MAP_CS) && m_team == TEAM_COUNTER)
	{
		int hostageWptIndex = FindHostage();

		if (IsValidWaypoint(hostageWptIndex) && m_currentWaypointIndex != hostageWptIndex)
			GetCurrentTask()->data = hostageWptIndex;
		else // no hostage? search goal waypoints
		{
			int goalindex = g_waypoint->m_goalPoints.GetRandomElement();

			if (IsValidWaypoint(goalindex))
			{
				if (m_isStuck || m_currentWaypointIndex == goalindex)
					GetCurrentTask()->data = goalindex;
			}
		}
	}

	if ((g_mapType & MAP_DE) && m_team == TEAM_TERRORIST)
	{
		if (!g_bombPlanted)
		{
			m_loosedBombWptIndex = FindLoosedBomb();

			if (IsValidWaypoint(m_loosedBombWptIndex) && m_currentWaypointIndex != m_loosedBombWptIndex && ChanceOf(50 + m_numEnemiesLeft + m_numFriendsLeft))
				GetCurrentTask()->data = m_loosedBombWptIndex;
		}
		else if (!m_defendedBomb)
		{
			int plantedBombWptIndex = g_waypoint->FindNearest(g_waypoint->GetBombPosition());

			if (IsValidWaypoint(plantedBombWptIndex) && m_currentWaypointIndex != plantedBombWptIndex)
				GetCurrentTask()->data = plantedBombWptIndex;
		}
	}

	// user forced a waypoint as a goal?
	if (IsValidWaypoint(ebot_debuggoal.GetInt()))
	{
		// check if we reached it
		if (IsVisible(g_waypoint->GetPath(ebot_debuggoal.GetInt())->origin, GetEntity()) && (g_waypoint->GetPath(m_currentWaypointIndex)->origin - pev->origin).GetLength2D() <= 24.0f && GetCurrentTask()->data == ebot_debuggoal.GetInt())
		{
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;

			m_checkTerrain = false;
			m_moveToGoal = false;

			return; // we can safely return here
		}

		if (GetCurrentTask()->data != ebot_debuggoal.GetInt())
		{
			DeleteSearchNodes();
			m_tasks->data = ebot_debuggoal.GetInt();
		}
	}

	if (IsValidWaypoint(m_waypointGoalAPI))
	{
		if (GetCurrentTask()->data != m_waypointGoalAPI)
		{
			DeleteSearchNodes();
			m_tasks->data = m_waypointGoalAPI;
		}
	}

	if (m_currentWeapon == WEAPON_KNIFE && !FNullEnt(m_enemy) && IsAlive(m_enemy) && !HasShield() && (GetEntityOrigin(GetEntity()) - GetEntityOrigin(m_enemy)).GetLength() <= 198.0f)
	{
		if (m_knifeAttackTime < engine->GetTime())
		{
			KnifeAttack();
			m_knifeAttackTime = AddTime(engine->RandomFloat(2.6f, 3.8f));
		}
	}

	if (IsShieldDrawn())
		pev->button |= IN_ATTACK2;

	if (m_reloadState == RSTATE_NONE && GetAmmo() != 0)
		m_reloadState = RSTATE_PRIMARY;

	// if bomb planted and it's a CT calculate new path to bomb point if he's not already heading for
	if (g_bombPlanted && m_team == TEAM_COUNTER && IsValidWaypoint(GetCurrentTask()->data) && !(g_waypoint->GetPath(m_tasks->data)->flags & WAYPOINT_GOAL) && GetCurrentTask()->taskID != TASK_ESCAPEFROMBOMB && (g_waypoint->GetPath(m_tasks->data)->origin - g_waypoint->GetBombPosition()).GetLength() > 128.0f)
	{
		DeleteSearchNodes();
		m_tasks->data = -1;
	}

	if (m_team == TEAM_COUNTER && !g_bombPlanted && IsValidWaypoint(m_currentWaypointIndex) && (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_GOAL) && ChanceOf(50 + m_numEnemiesLeft + m_numFriendsLeft) && GetNearbyEnemiesNearPosition(pev->origin, 768) == 0)
		RadioMessage(Radio_SectorClear);

	// reached the destination (goal) waypoint?
	if (DoWaypointNav())
	{
		TaskComplete();
		m_prevGoalIndex = -1;

		// spray logo sometimes if allowed to do so
		if (m_timeLogoSpray < engine->GetTime() && ebot_spraypaints.GetBool() && ChanceOf(50) && m_moveSpeed > GetWalkSpeed())
			PushTask(TASK_SPRAYLOGO, TASKPRI_SPRAYLOGO, -1, engine->GetTime() + 1.0f, false);

		// reached waypoint is a camp waypoint
		if ((g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CAMP) && GetGameMode() == MODE_BASE)
		{
			// check if bot has got a primary weapon and hasn't camped before
			if (HasPrimaryWeapon() && m_timeCamping + 10.0f < engine->GetTime() && !HasHostage())
			{
				bool campingAllowed = true;

				// Check if it's not allowed for this team to camp here
				if (m_team == TEAM_TERRORIST)
				{
					if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_COUNTER)
						campingAllowed = false;
				}
				else
				{
					if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_TERRORIST)
						campingAllowed = false;
				}

				// don't allow vip on as_ maps to camp + don't allow terrorist carrying c4 to camp
				if (((g_mapType & MAP_AS) && *(INFOKEY_VALUE(GET_INFOKEYBUFFER(GetEntity()), "model")) == 'v') || ((g_mapType & MAP_DE) && m_team == TEAM_TERRORIST && !g_bombPlanted && (m_isBomber)))
					campingAllowed = false;

				// check if another bot is already camping here
				if (!IsValidWaypoint(m_currentWaypointIndex) || IsWaypointOccupied(m_currentWaypointIndex))
					campingAllowed = false;

				if (campingAllowed)
				{
					m_campButtons = IN_DUCK;
					SelectBestWeapon();

					if (!(m_states & (STATE_SEEINGENEMY | STATE_HEARENEMY)) && !m_reloadState)
						m_reloadState = RSTATE_PRIMARY;

					MakeVectors(pev->v_angle);

					m_timeCamping = engine->GetTime() + engine->RandomFloat(g_skillTab[m_skill / 20].campStartDelay, g_skillTab[m_skill / 20].campEndDelay);
					PushTask(TASK_CAMP, TASKPRI_CAMP, -1, m_timeCamping, true);

					src.x = g_waypoint->GetPath(m_currentWaypointIndex)->campStartX;
					src.y = g_waypoint->GetPath(m_currentWaypointIndex)->campStartY;
					src.z = 0;

					m_camp = src;
					m_aimFlags |= AIM_CAMP;
					m_campDirection = 0;

					// tell the world we're camping
					if (ChanceOf(90))
						RadioMessage(Radio_InPosition);

					m_moveToGoal = false;
					m_checkTerrain = false;

					m_moveSpeed = 0.0f;
					m_strafeSpeed = 0.0f;
				}
			}
		}
		else if (GetGameMode() == MODE_ZP && !m_isZombieBot)
			ZmCampPointAction(1);
		else if (GetGameMode() == MODE_BASE)
		{
			// some goal waypoints are map dependant so check it out...
			if (g_mapType & MAP_CS)
			{
				if (m_team == TEAM_TERRORIST)
				{
					if (m_skill >= 80 || engine->RandomInt(0, 100) < m_skill)
					{
						int index = FindDefendWaypoint(g_waypoint->GetPath(m_currentWaypointIndex)->origin);
						m_campposition = g_waypoint->GetPath(index)->origin;
						PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, index, engine->GetTime() + ebot_camp_max.GetFloat(), true);
						m_campButtons |= IN_DUCK;
					}
				}
				else if (m_team == TEAM_COUNTER)
				{
					if (HasHostage())
					{
						// and reached a Rescue Point?
						if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_RESCUE)
						{
							for (i = 0; i < Const_MaxHostages; i++)
							{
								if (FNullEnt(m_hostages[i]))
									continue;

								if (g_waypoint->GetPath(g_waypoint->FindNearest(GetEntityOrigin(m_hostages[i])))->flags & WAYPOINT_RESCUE)
									m_hostages[i] = nullptr;
							}
						}
					}
				}
			}

			if ((g_mapType & MAP_DE) && ((g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_GOAL) || m_inBombZone) && FNullEnt(m_enemy))
			{
				// is it a terrorist carrying the bomb?
				if (m_isBomber)
				{
					if ((m_states & STATE_SEEINGENEMY) && GetNearbyFriendsNearPosition(pev->origin, 768) == 0)
					{
						// request an help also
						RadioMessage(Radio_NeedBackup);

						int index = FindDefendWaypoint(pev->origin);

						m_campposition = g_waypoint->GetPath(index)->origin;

						PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, index, engine->GetTime() + ebot_camp_max.GetFloat(), true);
					}
					else
						PushTask(TASK_PLANTBOMB, TASKPRI_PLANTBOMB, -1, 0.0, false);
				}
				else if (m_team == TEAM_COUNTER && m_timeCamping + 10.0f < engine->GetTime())
				{
					if (!g_bombPlanted && GetNearbyFriendsNearPosition(pev->origin, 250) < 4 && ChanceOf(60))
					{
						m_timeCamping = engine->GetTime() + engine->RandomFloat(g_skillTab[m_skill / 20].campStartDelay, g_skillTab[m_skill / 20].campEndDelay);
						int index = FindDefendWaypoint(g_waypoint->GetPath(m_currentWaypointIndex)->origin);
						m_campposition = g_waypoint->GetPath(index)->origin;
						PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, index, engine->GetTime() + ebot_camp_max.GetFloat(), true); // push camp task on to stack
						m_campButtons |= IN_DUCK;
					}
				}
			}
		}
	}
	else if (!GoalIsValid()) // no more nodes to follow - search new ones (or we have a momb)
	{
		m_moveSpeed = pev->maxspeed;

		DeleteSearchNodes();

		// did we already decide about a goal before?
		if (IsValidWaypoint(GetCurrentTask()->data) && !m_isBomber)
			destIndex = m_tasks->data;
		else
			destIndex = FindGoal();

		m_prevGoalIndex = destIndex;

		// remember index
		m_tasks->data = destIndex;

		// do pathfinding if it's not the current waypoint
		if (destIndex != m_currentWaypointIndex && IsValidWaypoint(destIndex))
			FindPath(m_currentWaypointIndex, destIndex);
	}
	else
	{
		if (!(pev->flags & FL_DUCKING) && m_minSpeed != pev->maxspeed)
			m_moveSpeed = m_minSpeed;

		if (GetGameMode() == MODE_ZP && !m_isZombieBot && !g_waypoint->m_zmHmPoints.IsEmpty())
		{
			ZmCampPointAction(1);
			if (IsValidWaypoint(GetCurrentTask()->data) && !(g_waypoint->IsZBCampPoint(GetCurrentTask()->data)))
			{
				m_prevGoalIndex = -1;
				GetCurrentTask()->data = -1;
			}
		}
	}

	if (engine->IsFootstepsOn() && m_skill > 50 && !(m_aimFlags & AIM_ENEMY) && (m_heardSoundTime + 13.0f >= engine->GetTime() || (m_states & (STATE_HEARENEMY))) && GetNearbyEnemiesNearPosition(pev->origin, 1024) >= 1 && !(m_currentTravelFlags & PATHFLAG_JUMP) && !(pev->button & IN_DUCK) && !(pev->flags & FL_DUCKING) && !g_bombPlanted && !m_isZombieBot)
	{
		if (FNullEnt(m_enemy)) // don't walk if theres a enemy
		{
			m_moveSpeed = GetWalkSpeed();
			if (m_currentWeapon == WEAPON_KNIFE)
				SelectBestWeapon();
		}
		else
			m_moveSpeed = pev->maxspeed;
	}

	// bot hasn't seen anything in a long time and is asking his teammates to report in or sector clear
	if ((GetGameMode() == MODE_BASE || GetGameMode() == MODE_TDM) && m_seeEnemyTime != 0.0f && m_seeEnemyTime + engine->RandomFloat(30.0f, 80.0f) < engine->GetTime() && engine->RandomInt(0, 100) < 70 && g_timeRoundStart + 20.0f < engine->GetTime() && m_askCheckTime + engine->RandomFloat(20.0, 30.0f) < engine->GetTime())
	{
		m_askCheckTime = engine->GetTime();
		if (ChanceOf(40))
			RadioMessage(Radio_SectorClear);
		else
			RadioMessage(Radio_ReportTeam);
	}
}

// this is core function that handle task execution
void Bot::RunTask(void)
{
	int destIndex, i;
	Vector src, destination;
	TraceResult tr;

	bool exceptionCaught = false;
	const float timeToBlowUp = GetBombTimeleft();
	float defuseRemainingTime = m_hasDefuser ? 7.0f : 12.0f - engine->GetTime();

	switch (GetCurrentTask()->taskID)
	{
		// normal task
	case TASK_NORMAL:
		TaskNormal(destIndex, i, src);
		break;
		// bot sprays messy logos all over the place...
	case TASK_SPRAYLOGO:
		m_aimFlags |= AIM_ENTITY;
		m_aimStopTime = 0.0f;

		// bot didn't spray this round?
		if (m_timeLogoSpray <= engine->GetTime() && m_tasks->time > engine->GetTime())
		{
			MakeVectors(pev->v_angle);
			Vector sprayOrigin = EyePosition() + (g_pGlobals->v_forward * 128);

			TraceLine(EyePosition(), sprayOrigin, true, GetEntity(), &tr);

			// no wall in front?
			if (tr.flFraction >= 1.0f)
				sprayOrigin.z -= 128.0f;

			m_entity = sprayOrigin;

			if (m_tasks->time - 0.5 < engine->GetTime())
			{
				// emit spraycan sound
				EMIT_SOUND_DYN2(GetEntity(), CHAN_VOICE, "player/sprayer.wav", 1.0, ATTN_NORM, 0, 100);
				TraceLine(EyePosition(), EyePosition() + g_pGlobals->v_forward * 128.0f, true, GetEntity(), &tr);

				// paint the actual logo decal
				DecalTrace(pev, &tr, m_logotypeIndex);
				m_timeLogoSpray = engine->GetTime() + engine->RandomFloat(30.0f, 45.0f);
			}
		}
		else
			TaskComplete();

		m_moveToGoal = false;
		m_checkTerrain = false;

		m_navTimeset = engine->GetTime();
		m_moveSpeed = 0;
		m_strafeSpeed = 0.0f;

		break;

		// hunt down enemy
	case TASK_HUNTENEMY:
		m_aimFlags |= AIM_NAVPOINT;
		m_checkTerrain = true;

		// if we've got new enemy...
		if (!FNullEnt(m_enemy) || FNullEnt(m_lastEnemy))
		{
			// forget about it...
			TaskComplete();
			m_prevGoalIndex = -1;

			SetLastEnemy(nullptr);
		}
		else if (GetTeam(m_lastEnemy) == m_team)
		{
			// don't hunt down our teammate...
			RemoveCertainTask(TASK_HUNTENEMY);
			m_prevGoalIndex = -1;
		}
		else if (DoWaypointNav()) // reached last enemy pos?
		{
			// forget about it...
			TaskComplete();
			m_prevGoalIndex = -1;

			SetLastEnemy(nullptr);
		}
		else if (!GoalIsValid()) // do we need to calculate a new path?
		{
			DeleteSearchNodes();

			// is there a remembered index?
			if (IsValidWaypoint(GetCurrentTask()->data))
				destIndex = m_tasks->data;
			else // no. we need to find a new one
				destIndex = GetEntityWaypoint(m_lastEnemy);

			// remember index
			m_prevGoalIndex = destIndex;
			m_tasks->data = destIndex;

			if (destIndex != m_currentWaypointIndex && IsValidWaypoint(destIndex))
				FindPath(m_currentWaypointIndex, destIndex);
		}

		// bots skill higher than 50?
		if (m_skill > 50 && engine->IsFootstepsOn())
		{
			// then make him move slow if near enemy
			if (!(m_currentTravelFlags & PATHFLAG_JUMP) && !m_isStuck && !m_isReloading &&
				m_currentWeapon != WEAPON_KNIFE &&
				m_currentWeapon != WEAPON_HEGRENADE &&
				m_currentWeapon != WEAPON_C4 &&
				m_currentWeapon != WEAPON_FBGRENADE &&
				m_currentWeapon != WEAPON_SHIELDGUN &&
				m_currentWeapon != WEAPON_SMGRENADE)
			{
				if (IsValidWaypoint(m_currentWaypointIndex))
				{
					if (g_waypoint->GetPath(m_currentWaypointIndex)->radius < 32 && !IsOnLadder() && !IsInWater() && m_seeEnemyTime + 4.0f > engine->GetTime() && m_skill < 80)
						pev->button |= IN_DUCK;
				}

				if (!FNullEnt(m_lastEnemy) && IsAlive(m_lastEnemy) && (m_lastEnemyOrigin - pev->origin).GetLength() <= 768.0f && !(pev->flags & FL_DUCKING))
				{
					m_moveSpeed = GetWalkSpeed();
					if (m_currentWeapon == WEAPON_KNIFE)
						SelectBestWeapon();
				}
			}
		}
		break;

		// bot seeks cover from enemy
	case TASK_SEEKCOVER:
		m_aimFlags |= AIM_NAVPOINT;

		if (m_isZombieBot)
		{
			TaskComplete();
			m_prevGoalIndex = -1;
			return;
		}

		if (!FNullEnt(m_enemy))
			DeleteSearchNodes();

		if (DoWaypointNav()) // reached final cover waypoint?
		{
			// yep. activate hide behaviour
			TaskComplete();

			m_prevGoalIndex = -1;

			// start hide task
			PushTask(TASK_HIDE, TASKPRI_HIDE, -1, engine->GetTime() + engine->RandomFloat(5.0f, 15.0f), false);
			destination = m_lastEnemyOrigin;

			// get a valid look direction
			GetCampDirection(&destination);

			m_aimFlags |= AIM_CAMP;
			m_camp = destination;
			m_campDirection = 0;

			// chosen waypoint is a camp waypoint?
			if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CAMP)
				m_campButtons = IN_DUCK;
			else
			{
				// choose a crouch or stand pos
				if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CROUCH)
					m_campButtons = IN_DUCK;
				else
					m_campButtons = 0;
			}

			if ((m_reloadState == RSTATE_NONE) && (GetAmmoInClip() < 8) && (GetAmmo() != 0))
				m_reloadState = RSTATE_PRIMARY;

			m_moveSpeed = 0;
			m_strafeSpeed = 0;

			m_moveToGoal = false;
			m_checkTerrain = true;
		}
		else if (!GoalIsValid()) // we didn't choose a cover waypoint yet or lost it due to an attack?
		{
			if (GetGameMode() != MODE_ZP)
				DeleteSearchNodes();

			if (GetGameMode() == MODE_ZP && !m_isZombieBot)
				if (FNullEnt(m_enemy) && !g_waypoint->m_zmHmPoints.IsEmpty())
					destIndex = FindGoal();
				else
					destIndex = FindCoverWaypoint(2048.0f);
			else if (IsValidWaypoint(GetCurrentTask()->data))
				destIndex = m_tasks->data;
			else
				destIndex = FindCoverWaypoint(1024.0f);

			if (!IsValidWaypoint(destIndex))
				destIndex = g_waypoint->FindFarest(pev->origin, 512.0f);

			m_campDirection = 0;
			m_prevGoalIndex = destIndex;
			m_tasks->data = destIndex;

			if (destIndex != m_currentWaypointIndex && IsValidWaypoint(destIndex))
				FindPath(m_currentWaypointIndex, destIndex);
		}

		break;

		// plain attacking
	case TASK_FIGHTENEMY:
		if (IsZombieMode() && !m_isZombieBot)
		{
			TaskNormal(i, destIndex, src);
			return;
		}

		m_moveToGoal = true;
		m_checkTerrain = true;

		if (!FNullEnt(m_enemy) || m_seeEnemyTime + 2.0f > engine->GetTime())
			CombatFight();
		else
		{
			if (!m_isZombieBot || !(g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_ZOMBIEPUSH))
			{
				SetEntityWaypoint(GetEntity());
				m_currentWaypointIndex = -1;
				DeleteSearchNodes();
				GetValidWaypoint();
			}
			
			TaskComplete();
		}

		m_navTimeset = engine->GetTime();

		break;

		// Bot is pausing
	case TASK_PAUSE:
		m_moveToGoal = false;
		m_checkTerrain = false;

		m_navTimeset = engine->GetTime();
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;

		m_aimFlags |= AIM_NAVPOINT;

		// is bot blinded and above average skill?
		if (m_viewDistance < 500.0f && m_skill > 50)
		{
			// go mad!
			m_moveSpeed = -fabsf((m_viewDistance - 500.0f) / 2.0f);

			if (m_moveSpeed < -pev->maxspeed)
				m_moveSpeed = -pev->maxspeed;

			MakeVectors(pev->v_angle);
			m_camp = EyePosition() + (g_pGlobals->v_forward * 500);

			m_aimFlags |= AIM_OVERRIDE;
			m_wantsToFire = true;
		}
		else
			pev->button |= m_campButtons;

		// stop camping if time over or gets hurt by something else than bullets
		if (m_tasks->time < engine->GetTime() || m_lastDamageType > 0)
			TaskComplete();
		break;

		// blinded (flashbanged) behaviour
	case TASK_BLINDED:
		m_moveToGoal = false;
		m_checkTerrain = false;
		m_navTimeset = engine->GetTime();

		// if bot remembers last enemy position
		if (m_skill > 70 && m_lastEnemyOrigin != nullvec && IsValidPlayer(m_lastEnemy) && !UsesSniper())
		{
			m_lookAt = m_lastEnemyOrigin; // face last enemy
			m_wantsToFire = true; // and shoot it
		}

		if (!IsValidWaypoint(m_blindCampPoint))
		{
			m_moveSpeed = m_blindMoveSpeed;
			m_strafeSpeed = m_blindSidemoveSpeed;
			pev->button |= m_blindButton;
		}
		else
		{
			m_moveToGoal = true;
			DeleteSearchNodes();
			m_campposition = g_waypoint->GetPath(m_blindCampPoint)->origin;
			PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, -1, engine->GetTime() + ebot_camp_max.GetFloat(), true);

			if (g_waypoint->GetPath(m_blindCampPoint)->vis.crouch <= g_waypoint->GetPath(m_blindCampPoint)->vis.stand)
				m_campButtons |= IN_DUCK;
			else
				m_campButtons &= ~IN_DUCK;

			m_blindCampPoint = -1;
		}

		if (m_blindTime < engine->GetTime())
			TaskComplete();

		break;

		// camping behaviour
	case TASK_GOINGFORCAMP:
		if ((m_isZombieBot || (IsZombieMode() && g_waypoint->m_zmHmPoints.IsEmpty()))
			|| (GetGameMode() == MODE_BASE && (g_mapType & MAP_CS) && m_team == TEAM_COUNTER && HasHostage())
			|| (!IsZombieMode() && !FNullEnt(m_enemy) && m_currentWeapon == WEAPON_KNIFE)
			|| (m_personality == PERSONALITY_CAREFUL && GetGameMode() == MODE_BASE && !FNullEnt(m_enemy) && m_numFriendsLeft < m_numEnemiesLeft)
			|| m_isVIP || m_isUsingGrenade || m_isBomber)
		{
			TaskComplete();
			break;
		}

		if (g_waypoint->m_campPoints.IsEmpty() || m_isStuck || HasHostage() || m_isVIP || m_isZombieBot || m_isBomber || m_numEnemiesLeft <= 1 || m_numFriendsLeft <= 1)
		{
			TaskComplete();
			return;
		}

		m_aimFlags |= AIM_NAVPOINT;

		if (m_campposition == nullvec || m_campposition == -1) // we cant...
		{
			m_campposition = g_waypoint->GetPath(g_waypoint->m_campPoints.GetRandomElement())->origin; // get random camping waypoint

			return;
		}

		if (GetGameMode() == MODE_BASE && (g_mapType & MAP_DE))
		{
			if (m_team == TEAM_COUNTER && g_bombPlanted && m_defendedBomb && !IsBombDefusing(g_waypoint->GetBombPosition()) && !OutOfBombTimer())
			{
				m_defendedBomb = false;
				TaskComplete();
				break;
			}
		}

		if (DoWaypointNav() && (pev->origin - m_campposition).GetLength() <= 20.0f) // reached destination?
		{
			TaskComplete();
			RemoveCertainTask(TASK_GOINGFORCAMP); // we're done
			RemoveCertainTask(TASK_MOVETOPOSITION); // we're done

			if (IsZombieMode())
				PushTask(TASK_CAMP, TASKPRI_CAMP, m_currentWaypointIndex, engine->GetTime() + 99999.0f, false);
			else
				PushTask(TASK_CAMP, TASKPRI_CAMP, m_currentWaypointIndex, engine->GetTime() + engine->RandomFloat(ebot_camp_min.GetFloat(), ebot_camp_max.GetFloat()), false);
		}
		else if (!GoalIsValid()) // didn't choose goal waypoint yet?
		{
			DeleteSearchNodes();

			if (IsValidWaypoint(GetCurrentTask()->data))
				destIndex = m_tasks->data;
			else
				destIndex = g_waypoint->FindNearest(m_campposition);

			if (IsValidWaypoint(destIndex))
			{
				m_prevGoalIndex = destIndex;
				m_tasks->data = destIndex;
				FindPath(m_currentWaypointIndex, destIndex);
			}
			else
				TaskComplete();
		}

		break;

		// planting the bomb right now
	case TASK_CAMP:
		if (m_isZombieBot)
		{
			TaskComplete();
			return;
		}

		if (GetGameMode() == MODE_BASE && (g_mapType & MAP_DE))
		{
			if (g_bombPlanted && OutOfBombTimer())
			{
				TaskComplete();
				PushTask(TASK_ESCAPEFROMBOMB, TASKPRI_ESCAPEFROMBOMB, -1, 2.0f, true);
			}
		}

		if (IsZombieMode())
		{
			if (m_isSlowThink && engine->RandomInt(1, 3) == 1)
				SelectBestWeapon();

			m_aimFlags |= AIM_CAMP;

			// standing still
			if (!FNullEnt(m_enemy) && m_currentWeapon != WEAPON_KNIFE && m_personality != PERSONALITY_RUSHER && pev->velocity.GetLength2D() <= 1)
			{
				bool crouch = true;
				if (m_currentWeapon != WEAPON_M3 ||
					m_currentWeapon != WEAPON_XM1014 ||
					m_currentWeapon != WEAPON_G3SG1 ||
					m_currentWeapon != WEAPON_SCOUT ||
					m_currentWeapon != WEAPON_AWP ||
					m_currentWeapon != WEAPON_M249 ||
					m_currentWeapon != WEAPON_SG550)
					crouch = false;

				if (m_personality == PERSONALITY_NORMAL && (pev->origin - GetEntityOrigin(m_enemy)).GetLength() < m_maxhearrange)
					crouch = false;

				if (crouch)
				{
					const Vector& src = pev->origin - Vector(0, 0, 18.0f);
					if (IsVisible(src, m_enemy))
						m_duckTime = engine->GetTime() + m_frameInterval;
				}
			}
		}
		else
		{
			SelectBestWeapon();
			m_aimFlags |= AIM_CAMP;
		}

		// fix
		if (GetGameMode() == MODE_BASE && (g_mapType & MAP_DE))
		{
			if (m_team == TEAM_COUNTER && g_bombPlanted && m_defendedBomb && !IsBombDefusing(g_waypoint->GetBombPosition()) && !OutOfBombTimer())
			{
				m_defendedBomb = false;
				TaskComplete();
				break;
			}
		}

		if (IsZombieMode() && IsValidWaypoint(m_zhCampPointIndex))
		{
			auto zhPath = g_waypoint->GetPath(m_zhCampPointIndex);
			float maxRange = zhPath->flags & WAYPOINT_CROUCH ? 100.0f : 230.0f;
			if (zhPath->campStartX != 0.0f && ((zhPath->origin - pev->origin).GetLength2D() > maxRange || (zhPath->origin.z - 64.0f > pev->origin.z)))
			{
				m_zhCampPointIndex = -1;
				TaskComplete();
				GetValidWaypoint();
				break;
			}

			if (!g_waypoint->m_hmMeshPoints.IsEmpty())
			{
				Array <int> MeshWaypoints;
				if (GetCurrentTask()->time > engine->GetTime() + 60.0f)
				{
					for (int i = 0; i <= g_waypoint->m_hmMeshPoints.GetElementNumber(); i++)
					{
						int index;
						g_waypoint->m_hmMeshPoints.GetAt(i, index);

						if (g_waypoint->GetPath(index)->campStartX == 0.0f)
							continue;

						if (zhPath->campStartX != g_waypoint->GetPath(index)->campStartX)
							continue;

						MeshWaypoints.Push(index);
					}

					if (!MeshWaypoints.IsEmpty())
					{
						int myCampPoint = MeshWaypoints.GetRandomElement();
						m_chosenGoalIndex = myCampPoint;
						m_prevGoalIndex = myCampPoint;
						m_myMeshWaypoint = myCampPoint;
						MeshWaypoints.RemoveAll();

						float max = 10.0f;
						if (!FNullEnt(m_enemy) || !FNullEnt(m_enemyAPI))
						{
							if (m_personality == PERSONALITY_RUSHER)
								max = 20.0f;
							else if (m_personality != PERSONALITY_CAREFUL)
								max = 15.0f;
						}

						GetCurrentTask()->time = engine->GetTime() + engine->RandomFloat(4.0f, max);
						FindPath(m_currentWaypointIndex, m_myMeshWaypoint);
					}
				}
			}
		}

		// half the reaction time if camping because you're more aware of enemies if camping
		if (IsZombieMode())
			m_idealReactionTime = 0.0f;
		else
			m_idealReactionTime = (engine->RandomFloat(g_skillTab[m_skill / 20].minSurpriseTime, g_skillTab[m_skill / 20].maxSurpriseTime)) / 2;

		if (m_nextCampDirTime < engine->GetTime())
		{
			m_nextCampDirTime = engine->GetTime() + engine->RandomFloat(2.5f, 5.0f);

			if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_SNIPER)
			{
				destination.z = 0;

				// switch from 1 direction to the other
				if (m_campDirection < 1)
				{
					destination.x = g_waypoint->GetPath(m_currentWaypointIndex)->campStartX;
					destination.y = g_waypoint->GetPath(m_currentWaypointIndex)->campStartY;
					m_campDirection ^= 1;
				}
				else
				{
					destination.x = g_waypoint->GetPath(m_currentWaypointIndex)->campEndX;
					destination.y = g_waypoint->GetPath(m_currentWaypointIndex)->campEndY;
					m_campDirection ^= 1;
				}

				// find a visible waypoint to this direction...
				// i know this is ugly hack, but i just don't want to break compatiability
				int numFoundPoints = 0;
				int foundPoints[3];
				int distanceTab[3];

				Vector dotA = (destination - pev->origin).Normalize2D();

				for (i = 0; i < g_numWaypoints; i++)
				{
					// skip invisible waypoints or current waypoint
					if (i == m_currentWaypointIndex || g_waypoint->IsVisible(m_currentWaypointIndex, i))
						continue;

					Vector dotB = (g_waypoint->GetPath(i)->origin - pev->origin).Normalize2D();

					if ((dotA | dotB) > 0.9)
					{
						int distance = static_cast <int> ((pev->origin - g_waypoint->GetPath(i)->origin).GetLength());

						if (numFoundPoints >= 3)
						{
							for (int j = 0; j < 3; j++)
							{
								if (distance > distanceTab[j])
								{
									distanceTab[j] = distance;
									foundPoints[j] = i;

									break;
								}
							}
						}
						else
						{
							foundPoints[numFoundPoints] = i;
							distanceTab[numFoundPoints] = distance;

							numFoundPoints++;
						}
					}
				}

				if (--numFoundPoints >= 0)
					m_camp = g_waypoint->GetPath(foundPoints[engine->RandomInt(0, numFoundPoints)])->origin;
				else
					m_camp = g_waypoint->GetPath(GetCampAimingWaypoint())->origin;
			}
			else
			{
				if (!FNullEnt(m_lastEnemy) && IsAlive(m_lastEnemy) && engine->RandomInt(1, 3) == 1 && IsVisible(m_lastEnemyOrigin, GetEntity()))
					m_camp = g_waypoint->GetPath(g_waypoint->FindNearest(m_lastEnemyOrigin))->origin;
				else
					m_camp = g_waypoint->GetPath(GetCampAimingWaypoint())->origin;
			}
		}

		m_checkTerrain = false;
		m_moveToGoal = false;

		ResetCollideState();

		m_idealReactionTime *= 0.5f;

		m_navTimeset = engine->GetTime();
		m_timeCamping = engine->GetTime();

		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;

		GetValidWaypoint();

		// press remembered crouch button
		pev->button |= IsZombieMode() ? m_campButtons : IN_DUCK;

		// stop camping if time over or gets hurt by something else than bullets
		if (GetCurrentTask()->time < engine->GetTime() || m_lastDamageType > 0)
			TaskComplete();

		break;

		// hiding behaviour
	case TASK_HIDE:
		if (m_isZombieBot)
		{
			TaskComplete();
			break;
		}

		m_aimFlags |= AIM_LASTENEMY;
		m_checkTerrain = false;
		m_moveToGoal = false;

		// half the reaction time if camping
		m_idealReactionTime = (engine->RandomFloat(g_skillTab[m_skill / 20].minSurpriseTime, g_skillTab[m_skill / 20].maxSurpriseTime)) / 2;

		m_navTimeset = engine->GetTime();
		m_moveSpeed = 0;
		m_strafeSpeed = 0.0f;

		GetValidWaypoint();

		if (HasShield() && !m_isReloading)
		{
			if (!IsShieldDrawn())
				pev->button |= IN_ATTACK2; // draw the shield!
			else
				pev->button |= IN_DUCK; // duck under if the shield is already drawn
		}

		// if we see an enemy and aren't at a good camping point leave the spot
		if ((m_states & STATE_SEEINGENEMY) || m_inBombZone)
		{
			if (!(g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CAMP))
			{
				TaskComplete();

				m_campButtons = 0;
				m_prevGoalIndex = -1;

				if (!FNullEnt(m_enemy) || m_seeEnemyTime + 2.0f > engine->GetTime())
					CombatFight();

				break;
			}
		}
		else if (m_lastEnemyOrigin == nullvec) // If we don't have an enemy we're also free to leave
		{
			TaskComplete();

			m_campButtons = 0;
			m_prevGoalIndex = -1;

			if (GetCurrentTask()->taskID == TASK_HIDE)
				TaskComplete();

			break;
		}

		pev->button |= m_campButtons;
		m_navTimeset = engine->GetTime();

		if (m_lastDamageType > 0 || m_tasks->time < engine->GetTime())
		{
			if (m_isReloading && (!FNullEnt(m_enemy) || !FNullEnt(m_lastEnemy)) && m_skill > 70)
				m_tasks->time += 2.0f;
			else if (IsDeathmatchMode() && pev->health <= 20.0f && m_skill > 70)
				m_tasks->time += 5.0f;
			else
				TaskComplete();
		}

		break;

		// moves to a position specified in position has a higher priority than task_normal
	case TASK_MOVETOPOSITION:
		m_aimFlags |= AIM_NAVPOINT;

		if (IsShieldDrawn())
			pev->button |= IN_ATTACK2;

		// reached destination?
		if (DoWaypointNav())
		{
			TaskComplete(); // we're done
			m_prevGoalIndex = -1;
		}

		// didn't choose goal waypoint yet?
		else if (!GoalIsValid())
		{
			DeleteSearchNodes();

			int forcedestIndex = -1;
			int goal = GetCurrentTask()->data;

			if (IsValidWaypoint(goal))
				forcedestIndex = goal;
			else if (m_position != nullvec)
				forcedestIndex = g_waypoint->FindNearest(m_position);

			if (IsValidWaypoint(forcedestIndex))
			{
				m_prevGoalIndex = forcedestIndex;
				GetCurrentTask()->data = forcedestIndex;

				FindPath(m_currentWaypointIndex, forcedestIndex);
			}
			else
				TaskComplete();
		}

		break;

		// planting the bomb right now
	case TASK_PLANTBOMB:
		m_aimFlags |= AIM_CAMP;

		if (m_isBomber) // we're still got the C4?
		{
			SelectWeaponByName("weapon_c4");
			RadioMessage(Radio_CoverMe);

			if (IsAlive(m_enemy) || !m_inBombZone)
				TaskComplete();
			else
			{
				m_moveToGoal = false;
				m_checkTerrain = false;
				m_navTimeset = engine->GetTime();

				if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CROUCH)
					pev->button |= (IN_ATTACK | IN_DUCK);
				else
					pev->button |= IN_ATTACK;

				m_moveSpeed = 0;
				m_strafeSpeed = 0;
			}
		}
		else // done with planting
		{
			TaskComplete();

			// tell teammates to move over here...
			if (m_numFriendsLeft != 0 && GetNearbyFriendsNearPosition(pev->origin, 768) <= 1)
				RadioMessage(Radio_RegroupTeam);

			DeleteSearchNodes();

			int index = FindDefendWaypoint(pev->origin);
			float halfTimer = engine->GetTime() + ((engine->GetC4TimerTime() / 2) + (engine->GetC4TimerTime() / 4));

			// push camp task on to stack
			m_campposition = g_waypoint->GetPath(index)->origin; // required for this task
			PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, index, halfTimer, true);
			m_campButtons |= IN_DUCK;
		}
		break;

		// bomb defusing behaviour
	case TASK_DEFUSEBOMB:
		if (g_waypoint->GetBombPosition() == nullvec)
		{
			exceptionCaught = true;
			g_bombPlanted = false;

			RadioMessage(Radio_SectorClear);
		}
		else if (!FNullEnt(m_enemy) && !IsZombieEntity(m_enemy))
		{
			const int friends = GetNearbyFriendsNearPosition(pev->origin, 768);
			if (friends < 2 && defuseRemainingTime < timeToBlowUp)
			{
				exceptionCaught = true;

				if ((defuseRemainingTime + 2.0f) > timeToBlowUp)
					exceptionCaught = false;

				if (GetNearbyFriendsNearPosition(pev->origin, 9999) > friends)
					RadioMessage(Radio_NeedBackup);
			}
		}
		else if (defuseRemainingTime > timeToBlowUp)
			exceptionCaught = true;

		if (exceptionCaught)
		{
			m_checkTerrain = true;
			m_moveToGoal = true;

			if (GetCurrentTask()->taskID == TASK_DEFUSEBOMB)
			{
				m_aimFlags &= ~AIM_ENTITY;
				TaskComplete();
			}

			break;
		}

		m_aimFlags |= AIM_ENTITY;

		m_destOrigin = g_waypoint->GetBombPosition();
		m_entity = g_waypoint->GetBombPosition();

		if (m_entity != nullvec)
		{
			if (m_numEnemiesLeft == 0)
				SelectWeaponByName("weapon_knife");

			if (((m_entity - pev->origin).GetLength2D()) < 60.0f)
			{
				m_moveToGoal = false;
				m_checkTerrain = false;
				m_strafeSpeed = 0.0f;
				m_moveSpeed = 0.0f;

				if (m_isReloading)
				{
					const int friendsN = GetNearbyFriendsNearPosition(pev->origin, 768);
					if (friendsN > 2 && GetNearbyEnemiesNearPosition(pev->origin, 768) < friendsN)
						SelectWeaponByName("weapon_knife");
				}

				m_aimStopTime = 0.0f;
				if (m_pickupItem != nullptr)
					MDLL_Use(m_pickupItem, GetEntity());
				else
					pev->button |= IN_USE;

				RadioMessage(Radio_CoverMe);
			}
		}

		break;

		// follow user behaviour
	case TASK_FOLLOWUSER:
		if (FNullEnt(m_targetEntity) || !IsAlive(m_targetEntity))
		{
			m_targetEntity = nullptr;
			TaskComplete();

			break;
		}

		if (m_targetEntity->v.button & IN_ATTACK)
		{
			MakeVectors(m_targetEntity->v.v_angle);
			TraceLine(GetEntityOrigin(m_targetEntity) + m_targetEntity->v.view_ofs, g_pGlobals->v_forward * 500, true, true, GetEntity(), &tr);

			m_followWaitTime = 0.0f;

			if (!FNullEnt(tr.pHit) && IsValidPlayer(tr.pHit) && GetTeam(tr.pHit) != m_team)
			{
				m_targetEntity = nullptr;
				SetLastEnemy(tr.pHit);

				TaskComplete();
				break;
			}
		}

		if (m_targetEntity->v.maxspeed != 0.0f)
		{
			if (m_targetEntity->v.maxspeed < pev->maxspeed)
			{
				m_moveSpeed = GetWalkSpeed();
				if (m_currentWeapon == WEAPON_KNIFE)
					SelectBestWeapon();
			}
			else
				m_moveSpeed = pev->maxspeed;
		}

		if (m_reloadState == RSTATE_NONE && GetAmmo() != 0)
			m_reloadState = RSTATE_PRIMARY;

		if (IsZombieMode() || ((GetEntityOrigin(m_targetEntity) - pev->origin).GetLength() > 80))
			m_followWaitTime = 0.0f;
		else
		{
			if (m_followWaitTime == 0.0f)
				m_followWaitTime = engine->GetTime();
			else
			{
				if (m_followWaitTime + 5.0f < engine->GetTime())
				{
					m_targetEntity = nullptr;

					RadioMessage(Radio_YouTakePoint);
					TaskComplete();

					break;
				}
			}
		}

		m_aimFlags |= AIM_NAVPOINT;

		if (m_targetEntity->v.maxspeed < m_moveSpeed)
		{
			m_moveSpeed = GetWalkSpeed();
			if (m_currentWeapon == WEAPON_KNIFE)
				SelectBestWeapon();
		}

		if (IsShieldDrawn())
			pev->button |= IN_ATTACK2;

		if (DoWaypointNav()) // reached destination?
			GetCurrentTask()->data = -1;

		if (!GoalIsValid()) // didn't choose goal waypoint yet?
		{
			DeleteSearchNodes();

			destIndex = GetEntityWaypoint(m_targetEntity);

			Array <int> points;
			g_waypoint->FindInRadius(points, 200, GetEntityOrigin(m_targetEntity));

			while (!points.IsEmpty())
			{
				int newIndex = points.Pop();

				// if waypoint not yet used, assign it as dest
				if (IsValidWaypoint(newIndex) && !IsWaypointOccupied(newIndex) && (newIndex != m_currentWaypointIndex))
					destIndex = newIndex;
			}

			if (IsValidWaypoint(destIndex) && destIndex != m_currentWaypointIndex)
			{
				m_prevGoalIndex = destIndex;
				m_tasks->data = destIndex;

				FindPath(m_currentWaypointIndex, destIndex);
			}
			else
			{
				m_targetEntity = nullptr;
				TaskComplete();
			}
		}
		break;

	case TASK_MOVETOTARGET:
		m_moveTargetOrigin = GetEntityOrigin(m_moveTargetEntity);

		if (FNullEnt(m_moveTargetEntity) || m_moveTargetOrigin == nullvec)
		{
			SetMoveTarget(nullptr);
			break;
		}

		if (m_currentWeapon == WEAPON_KNIFE)
			SelectBestWeapon();

		if (m_isZombieBot && FNullEnt(m_enemy) && FNullEnt(m_enemyAPI) && (pev->origin - m_moveTargetOrigin).GetLength() <= 128.0f && IsVisibleForKnifeAttack(m_moveTargetOrigin, GetEntity()))
			SetEnemy(m_moveTargetEntity);

		m_aimFlags |= AIM_NAVPOINT;
		m_destOrigin = m_moveTargetOrigin;
		m_moveSpeed = pev->maxspeed;

		if (DoWaypointNav())
			DeleteSearchNodes();

		destIndex = GetEntityWaypoint(m_moveTargetEntity);

		if (IsValidWaypoint(destIndex))
		{
			bool needMoveToTarget = false;
			if (!GoalIsValid())
				needMoveToTarget = true;
			else if (GetCurrentTask()->data != destIndex)
			{
				needMoveToTarget = true;
				if (&m_navNode[0] != nullptr)
				{
					PathNode* node = m_navNode;

					while (node->next != nullptr)
						node = node->next;

					if (node->index == destIndex)
						needMoveToTarget = false;
				}
			}

			if (needMoveToTarget && (!(pev->flags & FL_DUCKING) || m_damageTime + 1.0f < engine->GetTime() || !HasNextPath()))
			{
				DeleteSearchNodes();
				m_prevGoalIndex = destIndex;
				m_tasks->data = destIndex;
				FindPath(m_currentWaypointIndex, destIndex);
			}
		}

		break;

		// HE grenade throw behaviour
	case TASK_THROWHEGRENADE:
		m_aimFlags |= AIM_GRENADE;
		destination = m_throw;

		if (m_isZombieBot)
			TaskComplete();

		RemoveCertainTask(TASK_FIGHTENEMY);

		extern ConVar ebot_zp_escape_distance;
		if (IsZombieMode() && !FNullEnt(m_enemy))
		{
			if (m_isZombieBot)
			{
				destination = GetEntityOrigin(m_enemy);
				m_destOrigin = destination;
				m_moveSpeed = pev->maxspeed;
				m_moveToGoal = false;
			}
			else if ((GetEntityOrigin(m_enemy) - pev->origin).GetLength() <= fabsf(m_enemy->v.speed) + ebot_zp_escape_distance.GetFloat())
			{
				destination = GetEntityOrigin(m_enemy);
				m_destOrigin = destination;
				m_moveSpeed = -pev->maxspeed;
				m_moveToGoal = false;
			}
			else
			{
				destination = GetEntityOrigin(m_enemy);
				m_moveSpeed = 0.0f;
				m_moveToGoal = false;
			}
		}
		else if (!(m_states & STATE_SEEINGENEMY))
		{
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;

			m_moveToGoal = false;
		}
		else if (!FNullEnt(m_enemy))
			destination = GetEntityOrigin(m_enemy) + (m_enemy->v.velocity.SkipZ() * 0.5f);

		m_isUsingGrenade = true;
		m_checkTerrain = false;

		if (!IsZombieMode() && (pev->origin - destination).GetLength() < 400.0f)
		{
			// heck, I don't wanna blow up myself
			m_grenadeCheckTime = engine->GetTime() + Const_GrenadeTimer;

			SelectBestWeapon();
			TaskComplete();

			break;
		}

		m_grenade = CheckThrow(EyePosition(), destination);

		if (m_grenade.GetLength() < 100.0f)
			m_grenade = CheckToss(EyePosition(), destination);

		if (!IsZombieMode() && m_grenade.GetLength() <= 100.0f)
		{
			m_grenadeCheckTime = engine->GetTime() + Const_GrenadeTimer;
			m_grenade = m_lookAt;

			SelectBestWeapon();
			TaskComplete();
		}
		else
		{
			edict_t* ent = nullptr;

			while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "grenade")))
			{
				if (ent->v.owner == GetEntity() && strcmp(STRING(ent->v.model) + 9, "hegrenade.mdl") == 0)
				{
					// set the correct velocity for the grenade
					if (m_grenade.GetLength() > 100)
						ent->v.velocity = m_grenade;

					m_grenadeCheckTime = engine->GetTime() + Const_GrenadeTimer;

					SelectBestWeapon();
					TaskComplete();

					break;
				}
			}

			if (FNullEnt(ent))
			{
				if (m_currentWeapon != WEAPON_HEGRENADE)
				{
					if (pev->weapons & (1 << WEAPON_HEGRENADE))
						SelectWeaponByName("weapon_hegrenade");
				}
				else if (!(pev->oldbuttons & IN_ATTACK))
					pev->button |= IN_ATTACK;
			}
		}

		pev->button |= m_campButtons;

		break;

		// flashbang throw behavior (basically the same code like for HE's)
	case TASK_THROWFBGRENADE:
		m_aimFlags |= AIM_GRENADE;
		destination = m_throw;

		if (m_isZombieBot)
			TaskComplete();

		RemoveCertainTask(TASK_FIGHTENEMY);

		extern ConVar ebot_zp_escape_distance;
		if (IsZombieMode() && !FNullEnt(m_enemy))
		{
			if (m_isZombieBot)
			{
				destination = GetEntityOrigin(m_enemy);
				m_destOrigin = destination;
				m_moveSpeed = pev->maxspeed;
				m_moveToGoal = false;
			}
			else if ((GetEntityOrigin(m_enemy) - pev->origin).GetLength() <= fabsf(m_enemy->v.speed) + 300.0f)
			{
				destination = GetEntityOrigin(m_enemy);
				m_destOrigin = destination;
				m_moveSpeed = -pev->maxspeed;
				m_moveToGoal = false;
			}
			else
			{
				destination = GetEntityOrigin(m_enemy);
				m_moveSpeed = pev->maxspeed;
				m_moveToGoal = true;
			}
		}
		else if (!(m_states & STATE_SEEINGENEMY))
		{
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;

			m_moveToGoal = false;
		}
		else if (!FNullEnt(m_enemy))
			destination = GetEntityOrigin(m_enemy) + (m_enemy->v.velocity.SkipZ() * 0.5);

		m_isUsingGrenade = true;
		m_checkTerrain = false;

		m_grenade = CheckThrow(EyePosition(), destination);

		if (m_grenade.GetLength() < 100.0f)
			m_grenade = CheckToss(pev->origin, destination);

		if (!IsZombieMode() && m_grenade.GetLength() <= 100.0f)
		{
			m_grenadeCheckTime = engine->GetTime() + Const_GrenadeTimer;
			m_grenade = m_lookAt;

			SelectBestWeapon();
			TaskComplete();
		}
		else
		{
			edict_t* ent = nullptr;
			while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "grenade")))
			{
				if (ent->v.owner == GetEntity() && strcmp(STRING(ent->v.model) + 9, "flashbang.mdl") == 0)
				{
					// set the correct velocity for the grenade
					if (m_grenade.GetLength() > 100)
						ent->v.velocity = m_grenade;

					m_grenadeCheckTime = engine->GetTime() + Const_GrenadeTimer;

					SelectBestWeapon();
					TaskComplete();

					break;
				}
			}

			if (FNullEnt(ent))
			{
				if (m_currentWeapon != WEAPON_FBGRENADE)
				{
					if (pev->weapons & (1 << WEAPON_FBGRENADE))
						SelectWeaponByName("weapon_flashbang");
				}
				else if (!(pev->oldbuttons & IN_ATTACK))
					pev->button |= IN_ATTACK;
			}
		}

		pev->button |= m_campButtons;

		break;

		// smoke grenade throw behavior
		// a bit different to the others because it mostly tries to throw the sg on the ground
	case TASK_THROWSMGRENADE:
		m_aimFlags |= AIM_GRENADE;
		RemoveCertainTask(TASK_FIGHTENEMY);

		if (!(m_states & STATE_SEEINGENEMY))
		{
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;

			m_moveToGoal = false;
		}

		m_checkTerrain = false;
		m_isUsingGrenade = true;

		src = m_lastEnemyOrigin - pev->velocity;

		// predict where the enemy is in 0.5 secs
		if (!FNullEnt(m_enemy))
			src = src + m_enemy->v.velocity * 0.5f;

		m_grenade = (src - EyePosition()).Normalize();

		if (m_tasks->time < engine->GetTime() + 0.5)
		{
			m_aimFlags &= ~AIM_GRENADE;
			m_states &= ~STATE_THROWSMOKE;

			TaskComplete();
			break;
		}

		if (m_currentWeapon != WEAPON_SMGRENADE)
		{
			if (pev->weapons & (1 << WEAPON_SMGRENADE))
			{
				SelectWeaponByName("weapon_smokegrenade");
				m_tasks->time = engine->GetTime() + Const_GrenadeTimer;
			}
			else
				m_tasks->time = engine->GetTime() + 0.1f;
		}
		else if (!(pev->oldbuttons & IN_ATTACK))
			pev->button |= IN_ATTACK;
		else
		{
			SelectBestWeapon();
			TaskComplete();
		}

		break;

	case TASK_THROWFLARE:
		m_aimFlags |= AIM_GRENADE;
		destination = m_throw;

		if (m_isZombieBot)
			TaskComplete();

		RemoveCertainTask(TASK_FIGHTENEMY);

		if (!(m_states & STATE_SEEINGENEMY))
		{
			if (!FNullEnt(m_lastEnemy))
				destination = m_lastEnemyOrigin;

			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;

			m_moveToGoal = false;
		}
		else if (!FNullEnt(m_enemy))
			destination = GetEntityOrigin(m_enemy) + (m_enemy->v.velocity.SkipZ() * 0.5f);

		m_isUsingGrenade = true;
		m_checkTerrain = false;

		m_grenade = CheckThrow(EyePosition(), destination);

		if (m_grenade.GetLength() < 100)
			m_grenade = CheckToss(EyePosition(), destination);

		if (m_grenade.GetLength() <= 100)
		{
			m_grenadeCheckTime = engine->GetTime() + Const_GrenadeTimer;
			m_grenade = m_lookAt;

			SelectBestWeapon();
			TaskComplete();
		}
		else
		{
			edict_t* ent = nullptr;

			while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "grenade")))
			{
				if (ent->v.owner == GetEntity() && strcmp(STRING(ent->v.model) + 9, "smokegrenade.mdl") == 0)
				{
					// set the correct velocity for the grenade
					if (m_grenade.GetLength() > 100)
						ent->v.velocity = m_grenade;

					m_grenadeCheckTime = engine->GetTime() + Const_GrenadeTimer;

					SelectBestWeapon();
					TaskComplete();

					break;
				}
			}

			if (FNullEnt(ent))
			{
				if (m_currentWeapon != WEAPON_SMGRENADE)
				{
					if (pev->weapons & (1 << WEAPON_SMGRENADE))
						SelectWeaponByName("weapon_smokegrenade");
				}
				else if (!(pev->oldbuttons & IN_ATTACK))
					pev->button |= IN_ATTACK;
			}
		}

		pev->button |= m_campButtons;

		break;

		// bot helps human player (or other bot) to get somewhere
	case TASK_DOUBLEJUMP:
		if (FNullEnt(m_doubleJumpEntity) || !IsAlive(m_doubleJumpEntity) || !IsVisible(GetEntityOrigin(m_doubleJumpEntity), GetEntity()) || (m_aimFlags & AIM_ENEMY) || (IsValidWaypoint(m_travelStartIndex) && GetCurrentTask()->time + (g_waypoint->GetTravelTime(m_moveSpeed, g_waypoint->GetPath(m_travelStartIndex)->origin, m_doubleJumpOrigin) + 11.0f) < engine->GetTime()))
		{
			ResetDoubleJumpState();

			return;
		}

		m_aimFlags |= AIM_NAVPOINT;

		if (m_jumpReady)
		{
			m_moveToGoal = false;
			m_checkTerrain = false;

			m_navTimeset = engine->GetTime();
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;

			bool inJump = (m_doubleJumpEntity->v.button & IN_JUMP) || (m_doubleJumpEntity->v.oldbuttons & IN_JUMP);

			if (m_duckForJump < engine->GetTime())
				pev->button |= IN_DUCK;
			else if (inJump && !(pev->oldbuttons & IN_JUMP))
				pev->button |= IN_JUMP;

			const auto& srcb = pev->origin + Vector(0.0f, 0.0f, 45.0f);
			const auto& dest = srcb + Vector(0.0f, pev->angles.y, 0.0f) + (g_pGlobals->v_up) * 256.0f;

			TraceResult trc{};
			TraceLine(srcb, dest, false, true, GetEntity(), &trc);

			if (trc.flFraction < 1.0f && trc.pHit == m_doubleJumpEntity && inJump)
			{
				m_duckForJump = engine->GetTime() + engine->RandomFloat(3.0f, 5.0f);
				m_tasks->time = engine->GetTime();
			}

			return;
		}

		if (pev->button & IN_DUCK && IsOnFloor())
		{
			m_moveSpeed = pev->maxspeed;
			m_destOrigin = GetEntityOrigin(m_doubleJumpEntity);
			m_moveToGoal = false;
		}

		if (m_currentWaypointIndex == m_prevGoalIndex)
			m_destOrigin = m_doubleJumpOrigin;

		if (DoWaypointNav())
			GetCurrentTask()->data = -1;

		// didn't choose goal waypoint yet?
		if (!GoalIsValid())
		{
			DeleteSearchNodes();

			int boostIndex = g_waypoint->FindNearest(m_doubleJumpOrigin);

			if (IsValidWaypoint(boostIndex))
			{
				m_prevGoalIndex = boostIndex;
				m_travelStartIndex = m_currentWaypointIndex;

				GetCurrentTask()->data = boostIndex;

				// always take the shortest path
				FindPath(m_currentWaypointIndex, boostIndex);

				if (m_currentWaypointIndex == boostIndex)
					m_jumpReady = true;
			}
			else
				ResetDoubleJumpState();
		}

		break;

		// escape from bomb behaviour
	case TASK_ESCAPEFROMBOMB:
		m_aimFlags |= AIM_NAVPOINT;

		if (!g_bombPlanted || DoWaypointNav())
		{
			TaskComplete();

			if (m_numEnemiesLeft != 0)
			{
				if (IsShieldDrawn())
					pev->button |= IN_ATTACK2;
			}

			PushTask(TASK_PAUSE, TASKPRI_CAMP, -1, AddTime(10.0f), true);
		}
		else if (!GoalIsValid())
		{
			if (m_currentWeapon != WEAPON_KNIFE && m_numEnemiesLeft == 0)
				SelectWeaponByName("weapon_knife");

			destIndex = -1;

			DeleteSearchNodes();

			float safeRadius = 2048.0f, minPathDistance = 4096.0f;
			for (i = 0; i < g_numWaypoints; i++)
			{
				if ((g_waypoint->GetPath(i)->origin - g_waypoint->GetBombPosition()).GetLength() < safeRadius)
					continue;

				float pathDistance = g_waypoint->GetPathDistanceFloat(m_currentWaypointIndex, i);

				if (minPathDistance > pathDistance)
				{
					minPathDistance = pathDistance;
					destIndex = i;
				}
			}

			if (IsValidWaypoint(destIndex))
				destIndex = g_waypoint->FindFarest(pev->origin, safeRadius);

			m_prevGoalIndex = destIndex;
			m_tasks->data = destIndex;
			FindPath(m_currentWaypointIndex, destIndex);
		}

		break;

		// shooting breakables in the way action
	case TASK_DESTROYBREAKABLE:
		m_aimFlags |= AIM_OVERRIDE;

		// breakable destroyed?
		if (FNullEnt(m_breakableEntity) || !IsShootableBreakable(m_breakableEntity))
		{
			TaskComplete();
			return;
		}

		pev->button |= m_campButtons;

		m_checkTerrain = false;
		m_moveToGoal = false;
		m_navTimeset = engine->GetTime();
		m_camp = m_breakable;

		// is bot facing the breakable?
		if (GetShootingConeDeviation(GetEntity(), &m_breakable) >= 0.90f)
		{
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;

			if (!m_isZombieBot)
				SelectBestWeapon();
			else
				KnifeAttack();

			m_wantsToFire = true;
			m_shootTime = engine->GetTime();
		}
		else
		{
			m_checkTerrain = true;
			m_moveToGoal = true;

			TaskComplete();
		}

		break;

		// picking up items and stuff behaviour
	case TASK_PICKUPITEM:
		if (FNullEnt(m_pickupItem))
		{
			m_pickupItem = nullptr;
			TaskComplete();

			break;
		}

		destination = GetEntityOrigin(m_pickupItem);
		m_destOrigin = destination;
		m_entity = destination;

		if (m_moveSpeed <= 0)
			m_moveSpeed = pev->maxspeed;

		// find the distance to the item
		float itemDistance = (destination - pev->origin).GetLength();

		switch (m_pickupType)
		{
		case PICKTYPE_GETENTITY:
			m_aimFlags |= AIM_NAVPOINT;

			if (FNullEnt(m_pickupItem) || (GetTeam(m_pickupItem) != -1 && m_team != GetTeam(m_pickupItem)))
			{
				m_pickupItem = nullptr;
				m_pickupType = PICKTYPE_NONE;
			}
			break;

		case PICKTYPE_WEAPON:
			m_aimFlags |= AIM_NAVPOINT;

			if (m_blockWeaponPickAPI)
			{
				m_pickupItem = nullptr;
				TaskComplete();
				break;
			}

			// near to weapon?
			if (itemDistance < 60)
			{
				for (i = 0; i < 7; i++)
				{
					if (strcmp(g_weaponSelect[i].modelName, STRING(m_pickupItem->v.model) + 9) == 0)
						break;
				}

				if (i < 7)
				{
					// secondary weapon. i.e., pistol
					int weaponID = 0;

					for (i = 0; i < 7; i++)
					{
						if (pev->weapons & (1 << g_weaponSelect[i].id))
							weaponID = i;
					}

					if (weaponID > 0)
					{
						SelectWeaponbyNumber(weaponID);
						FakeClientCommand(GetEntity(), "drop");

						if (HasShield()) // If we have the shield...
							FakeClientCommand(GetEntity(), "drop"); // discard both shield and pistol
					}

					EquipInBuyzone(0);
				}
				else
				{
					// primary weapon
					int weaponID = GetHighestWeapon();

					if ((weaponID > 6) || HasShield())
					{
						SelectWeaponbyNumber(weaponID);
						FakeClientCommand(GetEntity(), "drop");
					}

					EquipInBuyzone(0);
				}

				CheckSilencer(); // check the silencer

				if (IsValidWaypoint(m_currentWaypointIndex))
				{
					if (itemDistance > g_waypoint->GetPath(m_currentWaypointIndex)->radius)
					{
						SetEntityWaypoint(GetEntity());
						m_currentWaypointIndex = -1;
						GetValidWaypoint();
					}
				}
			}

			break;

		case PICKTYPE_SHIELDGUN:
			m_aimFlags |= AIM_NAVPOINT;

			if (HasShield() || m_blockWeaponPickAPI)
			{
				m_pickupItem = nullptr;
				break;
			}
			else if (itemDistance < 60) // near to shield?
			{
				// get current best weapon to check if it's a primary in need to be dropped
				int weaponID = GetHighestWeapon();

				if (weaponID > 6)
				{
					SelectWeaponbyNumber(weaponID);
					FakeClientCommand(GetEntity(), "drop");

					if (IsValidWaypoint(m_currentWaypointIndex))
					{
						if (itemDistance > g_waypoint->GetPath(m_currentWaypointIndex)->radius)
						{
							SetEntityWaypoint(GetEntity());
							m_currentWaypointIndex = -1;
							GetValidWaypoint();
						}
					}
				}
			}
			break;

		case PICKTYPE_PLANTEDC4:
			m_aimFlags |= AIM_ENTITY;

			if (m_team == TEAM_COUNTER && itemDistance < 80)
			{
				// notify team of defusing
				if (GetNearbyFriendsNearPosition(pev->origin, 600) < 1 && m_numFriendsLeft >= 1)
					RadioMessage(Radio_NeedBackup);

				m_moveToGoal = false;
				m_checkTerrain = false;

				m_moveSpeed = 0.0f;
				m_strafeSpeed = 0.0f;

				PushTask(TASK_DEFUSEBOMB, TASKPRI_DEFUSEBOMB, -1, 0.0, false);
			}
			break;

		case PICKTYPE_HOSTAGE:
			m_aimStopTime = 0.0f;
			m_aimFlags |= AIM_ENTITY;
			src = EyePosition();

			if (!IsAlive(m_pickupItem) || m_team != TEAM_COUNTER)
			{
				// don't pickup dead hostages
				m_pickupItem = nullptr;
				TaskComplete();

				break;
			}

			if (itemDistance < 60)
			{
				// use game dll function to make sure the hostage is correctly 'used'
				MDLL_Use(m_pickupItem, GetEntity());

				for (i = 0; i < Const_MaxHostages; i++)
				{
					if (FNullEnt(m_hostages[i])) // store pointer to hostage so other bots don't steal from this one or bot tries to reuse it
					{
						m_hostages[i] = m_pickupItem;
						m_pickupItem = nullptr;

						break;
					}
				}

				m_itemCheckTime = engine->GetTime() + 0.1f;
				m_lastCollTime = engine->GetTime() + 0.1f; // also don't consider being stuck
			}
			break;

		case PICKTYPE_DEFUSEKIT:
			m_aimFlags |= AIM_NAVPOINT;

			if (m_hasDefuser || m_team != TEAM_COUNTER)
				// if (m_hasDefuser)
			{
				m_pickupItem = nullptr;
				m_pickupType = PICKTYPE_NONE;
			}
			break;

		case PICKTYPE_BUTTON:
			m_aimFlags |= AIM_ENTITY;

			if (FNullEnt(m_pickupItem) || m_buttonPushTime < engine->GetTime()) // it's safer...
			{
				TaskComplete();
				m_pickupType = PICKTYPE_NONE;

				break;
			}

			// find angles from bot origin to entity...
			src = EyePosition();
			float angleToEntity = InFieldOfView(destination - src);

			if (itemDistance < 90) // near to the button?
			{
				m_moveSpeed = 0.0f;
				m_strafeSpeed = 0.0f;
				m_moveToGoal = false;
				m_checkTerrain = false;

				if (angleToEntity <= 10) // facing it directly?
				{
					MDLL_Use(m_pickupItem, GetEntity());

					m_pickupItem = nullptr;
					m_pickupType = PICKTYPE_NONE;
					m_buttonPushTime = engine->GetTime() + 3.0f;

					TaskComplete();
				}
			}
			break;
		}
		break;
	}
}

void Bot::DebugModeMsg(void)
{
	int debugMode = ebot_debug.GetInt();
	if (FNullEnt(g_hostEntity) || debugMode <= 0 || debugMode == 2)
		return;

	static float timeDebugUpdate = 0.0f;

	int specIndex = g_hostEntity->v.iuser2;
	if (specIndex != ENTINDEX(GetEntity()))
		return;

	static int index, goal, taskID;

	if (m_tasks != nullptr)
	{
		if (taskID != m_tasks->taskID || index != m_currentWaypointIndex || goal != m_tasks->data || timeDebugUpdate < engine->GetTime())
		{
			taskID = m_tasks->taskID;
			index = m_currentWaypointIndex;
			goal = m_tasks->data;

			char taskName[80];

			switch (taskID)
			{
			case TASK_NORMAL:
				sprintf(taskName, "Normal");
				break;

			case TASK_PAUSE:
				sprintf(taskName, "Pause");
				break;

			case TASK_MOVETOPOSITION:
				sprintf(taskName, "Move To Position");
				break;

			case TASK_FOLLOWUSER:
				sprintf(taskName, "Follow User");
				break;

			case TASK_MOVETOTARGET:
				sprintf(taskName, "Move To Target");
				break;

			case TASK_PICKUPITEM:
				sprintf(taskName, "Pickup Item");
				break;

			case TASK_CAMP:
				sprintf(taskName, "Camping");
				break;

			case TASK_PLANTBOMB:
				sprintf(taskName, "Plant Bomb");
				break;

			case TASK_DEFUSEBOMB:
				sprintf(taskName, "Defuse Bomb");
				break;

			case TASK_FIGHTENEMY:
				sprintf(taskName, "Attack Enemy");
				break;

			case TASK_HUNTENEMY:
				sprintf(taskName, "Hunt Enemy");
				break;

			case TASK_SEEKCOVER:
				sprintf(taskName, "Seek Cover");
				break;

			case TASK_THROWHEGRENADE:
				sprintf(taskName, "Throw HE Grenade");
				break;

			case TASK_THROWFBGRENADE:
				sprintf(taskName, "Throw Flash Grenade");
				break;

			case TASK_THROWSMGRENADE:
				sprintf(taskName, "Throw Smoke Grenade");
				break;

			case TASK_DOUBLEJUMP:
				sprintf(taskName, "Perform Double Jump");
				break;

			case TASK_ESCAPEFROMBOMB:
				sprintf(taskName, "Escape From Bomb");
				break;

			case TASK_DESTROYBREAKABLE:
				sprintf(taskName, "Shooting To Breakable");
				break;

			case TASK_HIDE:
				sprintf(taskName, "Hide");
				break;

			case TASK_BLINDED:
				sprintf(taskName, "Blinded");
				break;

			case TASK_SPRAYLOGO:
				sprintf(taskName, "Spray Logo");
				break;

			case TASK_GOINGFORCAMP:
				sprintf(taskName, "Going To Camp Spot");
				break;
			}

			char weaponName[80], aimFlags[32], botType[32];
			char enemyName[80], pickName[80];

			if (IsAlive(m_enemy))
				sprintf(enemyName, "[E]: %s", GetEntityName(m_enemy));
			else if (IsAlive(m_moveTargetEntity))
				sprintf(enemyName, "[MT]: %s", GetEntityName(m_moveTargetEntity));
			else if (IsAlive(m_lastEnemy))
				sprintf(enemyName, "[LE]: %s", GetEntityName(m_lastEnemy));
			else
				sprintf(enemyName, ": %s", GetEntityName(nullptr));

			sprintf(pickName, "%s", GetEntityName(m_pickupItem));

			WeaponSelect* selectTab = &g_weaponSelect[0];
			char weaponCount = 0;

			while (m_currentWeapon != selectTab->id && weaponCount < Const_NumWeapons)
			{
				selectTab++;
				weaponCount++;
			}

			// set the aim flags
			sprintf(aimFlags, "%s%s%s%s%s%s%s%s",
				m_aimFlags & AIM_NAVPOINT ? "NavPoint " : "",
				m_aimFlags & AIM_CAMP ? "CampPoint " : "",
				m_aimFlags & AIM_PREDICTENEMY ? "PredictEnemy " : "",
				m_aimFlags & AIM_LASTENEMY ? "LastEnemy " : "",
				m_aimFlags & AIM_ENTITY ? "Entity " : "",
				m_aimFlags & AIM_ENEMY ? "Enemy " : "",
				m_aimFlags & AIM_GRENADE ? "Grenade " : "",
				m_aimFlags & AIM_OVERRIDE ? "Override " : "");

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

			int client = ENTINDEX(GetEntity()) - 1;

			char outputBuffer[512];
			sprintf(outputBuffer, "\n\n\n\n\n\n\n Game Mode: %s"
				"\n [%s] \n Task: %s  AimFlags:%s \n"
				"Weapon: %s  Clip: %d   Ammo: %d \n"
				"Type: %s  Money: %d  Bot AI: %s \n"
				"Enemy%s  Pickup: %s  \n\n"

				"CurrentIndex: %d  GoalIndex: %d  TD: %d \n"
				"Nav: %d  Next Nav: %d \n"
				"GEWI: %d GEWI2: %d \n"
				"Move Speed: %2.f  Strafe Speed: %2.f \n "
				"Check Terran: %d  Stuck: %d \n",
				gamemodName,
				GetEntityName(GetEntity()), taskName, aimFlags,
				&weaponName[7], GetAmmoInClip(), GetAmmo(),
				botType, m_moneyAmount, m_isZombieBot ? "Zombie" : "Normal",
				enemyName, pickName,

				m_currentWaypointIndex, m_prevGoalIndex, m_tasks->data,
				navIndex[0], navIndex[1],
				g_clients[client].wpIndex, g_clients[client].wpIndex2,
				m_moveSpeed, m_strafeSpeed,
				m_checkTerrain, m_isStuck);

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
			WRITE_STRING(const_cast <const char*> (&outputBuffer[0]));
			MESSAGE_END();

			timeDebugUpdate = engine->GetTime() + 1.0f;
		}

		if (m_moveTargetOrigin != nullvec && !FNullEnt(m_moveTargetEntity))
			engine->DrawLine(g_hostEntity, EyePosition(), m_moveTargetOrigin, Color(0, 255, 0, 255), 10, 0, 5, 1, LINE_SIMPLE);

		if (m_enemyOrigin != nullvec && !FNullEnt(m_enemy))
			engine->DrawLine(g_hostEntity, EyePosition(), m_enemyOrigin, Color(255, 0, 0, 255), 10, 0, 5, 1, LINE_SIMPLE);

		if (m_destOrigin != nullvec)
			engine->DrawLine(g_hostEntity, pev->origin, m_destOrigin, Color(0, 0, 255, 255), 10, 0, 5, 1, LINE_SIMPLE);

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
				for (int j = 0; j < Const_MaxPathIndex; j++)
				{
					if (path->index[j] == node->index && path->connectionFlags[j] & PATHFLAG_JUMP)
					{
						jumpPoint = true;
						break;
					}
				}

				if (jumpPoint)
					engine->DrawLine(g_hostEntity, src, g_waypoint->GetPath(node->index)->origin,
						Color(255, 0, 0, 255), 15, 0, 8, 1, LINE_SIMPLE);
				else
				{
					engine->DrawLine(g_hostEntity, src, g_waypoint->GetPath(node->index)->origin,
						Color(255, 100, 55, 255), 15, 0, 8, 1, LINE_SIMPLE);

					engine->DrawLine(g_hostEntity, src - Vector(0.0f, 0.0f, 35.0f), src + Vector(0.0f, 0.0f, 35.0f),
						Color(0, 255, 0, 255), 15, 0, 8, 1, LINE_SIMPLE);
				}
			}
		}

		if (IsValidWaypoint(m_prevWptIndex))
		{
			src = g_waypoint->GetPath(m_prevWptIndex)->origin;
			engine->DrawLine(g_hostEntity, src - Vector(0.0f, 0.0f, 35.0f), src + Vector(0.0f, 0.0f, 35.0f),
				Color(255, 0, 0, 255), 15, 0, 8, 1, LINE_SIMPLE);
		}

		if (IsValidWaypoint(m_currentWaypointIndex))
		{
			src = g_waypoint->GetPath(m_currentWaypointIndex)->origin;
			engine->DrawLine(g_hostEntity, src - Vector(0.0f, 0.0f, 35.0f), src + Vector(0.0f, 0.0f, 35.0f),
				Color(0, 255, 0, 255), 15, 0, 8, 1, LINE_SIMPLE);
		}

		if (IsValidWaypoint(m_prevGoalIndex))
		{
			src = g_waypoint->GetPath(m_prevGoalIndex)->origin;
			engine->DrawLine(g_hostEntity, src - Vector(0.0f, 0.0f, 35.0f), src + Vector(0.0f, 0.0f, 35.0f),
				Color(0, 255, 255, 255), 15, 0, 8, 1, LINE_SIMPLE);
		}
	}
}

void Bot::FunBotAI(void)
{
	// idk why
}

// this function gets called each frame and is the core of all bot ai. from here all other subroutines are called
void Bot::BotAI(void)
{
	float movedDistance = 2.0f; // length of different vector (distance bot moved)
	TraceResult tr;

	if (m_checkKnifeSwitch && m_buyingFinished && m_spawnTime + engine->RandomFloat(4.0f, 8.0f) < engine->GetTime())
	{
		m_checkKnifeSwitch = false;

		if (!IsZombieMode())
		{
			if (ebot_spraypaints.GetBool() && engine->RandomInt(1, 10) < 2)
				PushTask(TASK_SPRAYLOGO, TASKPRI_SPRAYLOGO, -1, engine->GetTime() + 1.0f, false);

			if (!IsDeathmatchMode() && ChanceOf(m_personality == PERSONALITY_RUSHER ? 99 : m_personality == PERSONALITY_CAREFUL ? 33 : 66) && !m_isReloading && (g_mapType & (MAP_CS | MAP_DE | MAP_ES | MAP_AS)))
				SelectWeaponByName("weapon_knife");
		}
	}

	// check if we already switched weapon mode
	if (m_checkWeaponSwitch && m_buyingFinished && m_spawnTime + engine->RandomFloat(2.0f, 3.5f) < engine->GetTime())
	{
		if (HasShield() && IsShieldDrawn())
			pev->button |= IN_ATTACK2;
		else
		{
			switch (m_currentWeapon)
			{
			case WEAPON_M4A1:
			case WEAPON_USP:
				CheckSilencer();
				break;

			case WEAPON_FAMAS:
			case WEAPON_GLOCK18:
				if (ChanceOf(50))
					pev->button |= IN_ATTACK2;
				break;
			}
		}

		// select a leader bot for this team
		if (GetGameMode() == MODE_BASE)
			SelectLeaderEachTeam(m_team);

		m_checkWeaponSwitch = false;
	}

	// warning: the following timers aren't frame independent so it varies on slower/faster computers

	// increase reaction time
	m_actualReactionTime += 0.2f;

	if (m_actualReactionTime > m_idealReactionTime)
		m_actualReactionTime = m_idealReactionTime;

	// bot could be blinded by flashbang or smoke, recover from it
	m_viewDistance += 3.0f;

	if (m_viewDistance > m_maxViewDistance)
		m_viewDistance = m_maxViewDistance;

	if (m_blindTime > engine->GetTime())
		m_maxViewDistance = 4096.0f;

	m_moveSpeed = pev->maxspeed;

	if (m_prevTime <= engine->GetTime())
	{
		// see how far bot has moved since the previous position...
		movedDistance = (m_prevOrigin - pev->origin).GetLength();

		// save current position as previous
		m_prevOrigin = pev->origin;
		m_prevTime = engine->GetTime() + 0.2f;
	}

	// if there's some radio message to respond, check it
	if (m_radioOrder != 0 && (GetGameMode() == MODE_BASE || GetGameMode() == MODE_TDM || (IsZombieMode() && !m_isZombieBot && g_waypoint->m_zmHmPoints.IsEmpty())))
		CheckRadioCommands();

	// do all sensing, calculate/filter all actions here
	SetConditions();

	if (IsZombieMode() && m_isZombieBot)
		ZombieModeAi();

	Vector src, dest;

	m_checkTerrain = true;
	m_moveToGoal = true;
	m_wantsToFire = false;

	//AvoidGrenades (); // avoid flyings grenades
	AvoidEntity();
	m_isUsingGrenade = false;

	RunTask(); // execute current task
	ChooseAimDirection();

	// the bots wants to fire at something?
	if (m_wantsToFire && !m_isUsingGrenade && m_shootTime <= engine->GetTime())
		FireWeapon(); // if bot didn't fire a bullet try again next frame

	 // check for reloading
	if (m_reloadCheckTime <= engine->GetTime())
		CheckReload();

	// set the reaction time (surprise momentum) different each frame according to skill
	m_idealReactionTime = engine->RandomFloat(g_skillTab[m_skill / 20].minSurpriseTime, g_skillTab[m_skill / 20].maxSurpriseTime);

	Vector directionOld = m_destOrigin - (pev->origin + pev->velocity * m_frameInterval);
	Vector directionNormal = directionOld.Normalize();
	Vector direction = directionNormal;
	directionNormal.z = 0.0f;

	if (m_destOrigin != nullvec)
	{
		m_moveAngles = directionOld.ToAngles();
		m_moveAngles.ClampAngles();
		m_moveAngles.x *= -1.0f; // invert for engine
	}

	if (!IsOnLadder())
	{
		if (IsZombieMode())
		{
			if (m_currentWeapon == WEAPON_KNIFE)
				SelectBestWeapon();

			if (!FNullEnt(m_enemy) && m_isEnemyReachable)
			{
				m_moveToGoal = false; // don't move to goal
				m_navTimeset = engine->GetTime();
				CombatFight();
			}

			extern ConVar ebot_escape;
			if (ebot_escape.GetBool())
			{
				if (FNullEnt(m_enemy) && m_seeEnemyTime + 2.0f < engine->GetTime())
					SelectWeaponByName("weapon_knife");
			}
		}
		else if (!FNullEnt(m_enemy) && GetCurrentTask()->taskID != TASK_CAMP)
		{
			m_moveToGoal = false; // don't move to goal
			m_navTimeset = engine->GetTime();
			CombatFight();
		}
	}

	if ((g_mapType & MAP_DE) && g_bombPlanted && m_notKilled && OutOfBombTimer())
	{
		if (GetCurrentTask()->taskID != TASK_ESCAPEFROMBOMB && GetCurrentTask()->taskID != TASK_CAMP)
		{
			if ((g_waypoint->GetPath(m_currentWaypointIndex)->origin - g_waypoint->GetBombPosition()).GetLength() <= 1024.0f)
			{
				TaskComplete();
				PushTask(TASK_ESCAPEFROMBOMB, TASKPRI_ESCAPEFROMBOMB, -1, 0.0f, true);
			}
		}
	}

	// if we're careful bot we can camp when reaching goal waypoint
	if ((g_mapType & MAP_DE) && m_personality == PERSONALITY_CAREFUL && g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_GOAL && ChanceOf(10))
	{
		int index = FindDefendWaypoint(pev->origin);
		m_campposition = g_waypoint->GetPath(index)->origin;
		PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, -1, ebot_camp_max.GetFloat(), true);
	}

	// allowed to move to a destination position?
	if (m_moveToGoal && !m_moveAIAPI)
	{
		GetValidWaypoint();

		// press duck button if we need to
		if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CROUCH && !(g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CAMP))
			pev->button |= IN_DUCK;

		// use button waypoints
		if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_USEBUTTON)
		{
			if ((pev->origin - g_waypoint->GetPath(m_currentWaypointIndex)->origin).GetLength() <= 100.0f)
			{
				edict_t* button = FindButton();
				if (!m_isSlowThink)
				{
					if (button != nullptr)
						MDLL_Use(button, GetEntity());
					else
						pev->button |= IN_USE;
				}
			}
		}

		if (IsInWater()) // special movement for swimming here
		{
			// check if we need to go forward or back press the correct buttons
			if (InFieldOfView(m_destOrigin - EyePosition()) > 90.0f)
				pev->button |= IN_BACK;
			else
				pev->button |= IN_FORWARD;

			if (m_moveAngles.x > 60.0f)
				pev->button |= IN_DUCK;
			else if (m_moveAngles.x < -60.0f)
				pev->button |= IN_JUMP;
		}
	}

	CheckFall();
	CheckTerrain(directionNormal, movedDistance);

	// must avoid a grenade?
	if (m_needAvoidEntity != 0)
	{
		// Don't duck to get away faster
		pev->button &= ~IN_DUCK;

		m_moveSpeed = -pev->maxspeed;
		m_strafeSpeed = pev->maxspeed * m_needAvoidEntity;
	}

	// time to reach waypoint
	if (m_navTimeset + GetEstimatedReachTime() < engine->GetTime() && m_moveToGoal)
	{
		if (FNullEnt(m_enemy) || !IsValidWaypoint(m_currentWaypointIndex) || (g_waypoint->GetPath(m_currentWaypointIndex)->radius > 0 && (pev->origin - g_waypoint->GetPath(m_currentWaypointIndex)->origin).GetLength() > (g_waypoint->GetPath(m_currentWaypointIndex)->radius) * 6))
			GetValidWaypoint();

		if (FNullEnt(m_enemy))
		{
			// clear these pointers, bot mingh be stuck getting to them
			if (!FNullEnt(m_pickupItem) && m_pickupType != PICKTYPE_PLANTEDC4)
				m_itemIgnore = m_pickupItem;

			m_pickupItem = nullptr;
			m_breakableEntity = nullptr;
			m_breakable = nullvec;
			m_itemCheckTime = engine->GetTime() + 5.0f;
			m_pickupType = PICKTYPE_NONE;
		}
	}

	bool OnLadderNoDuck = false;
	if (IsOnLadder() || (IsValidWaypoint(m_currentWaypointIndex) && g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_LADDER))
	{
		if (!(g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CROUCH))
			OnLadderNoDuck = true;
	}

	if (OnLadderNoDuck)
	{
		m_campButtons &= ~IN_DUCK;
		pev->button &= ~IN_DUCK;
	}
	else if (m_duckTime > engine->GetTime())
		pev->button |= IN_DUCK;

	if (pev->button & IN_JUMP)
	{
		if (m_currentTravelFlags & PATHFLAG_JUMP)
		{
			Vector point1Origin, point2Origin;
			if (IsValidWaypoint(m_prevWptIndex))
				point1Origin = g_waypoint->GetPath(m_prevWptIndex)->origin;
			else if (IsOnFloor())
				point1Origin = pev->origin;

			if (IsValidWaypoint(m_currentWaypointIndex))
				point2Origin = g_waypoint->GetPath(m_currentWaypointIndex)->origin;

			if (point1Origin != nullvec && point2Origin != nullvec)
			{
				if ((point1Origin - point2Origin).GetLength() >= 100.0f)
					m_jumpTime = engine->GetTime() + engine->RandomFloat(0.8f, 1.2f);
				else if (point1Origin.z > point2Origin.z)
					m_jumpTime = engine->GetTime();
				else
					m_jumpTime = engine->GetTime() + engine->RandomFloat(0.5f, 0.8f);
			}
		}
		else
			m_jumpTime = engine->GetTime() + engine->RandomFloat(0.3f, 0.5f);
	}

	if (m_jumpTime >= engine->GetTime() && !IsOnFloor() && !IsInWater() && !IsOnLadder())
		pev->button |= IN_DUCK;

	// save the previous speed (for checking if stuck)
	m_prevSpeed = fabsf(m_moveSpeed);
	m_lastDamageType = -1; // reset damage
}

void Bot::ChatMessage(int type, bool isTeamSay)
{
	extern ConVar ebot_chat;

	if (g_chatFactory[type].IsEmpty() || !ebot_chat.GetBool())
		return;

	const char* pickedPhrase = g_chatFactory[type].GetRandomElement();

	if (IsNullString(pickedPhrase))
		return;

	PrepareChatMessage(const_cast <char*> (pickedPhrase));
	PushMessageQueue(isTeamSay ? CMENU_TEAMSAY : CMENU_SAY);
}

bool Bot::HasHostage(void)
{
	for (auto hostage : m_hostages)
	{
		if (!FNullEnt(hostage))
		{
			// don't care about dead hostages
			if (hostage->v.health <= 0.0f || (pev->origin - GetEntityOrigin(hostage)).GetLength() > 600.0f)
			{
				hostage = nullptr;
				continue;
			}

			return true;
		}
	}

	return false;
}

void Bot::ResetCollideState(void)
{
	m_probeTime = 0.0f;
	m_collideTime = 0.0f;

	m_collisionState = COSTATE_UNDECIDED;
	m_collStateIndex = 0;

	m_isStuck = false;

	for (int i = 0; i < 4; i++)
		m_collideMoves[i] = 0;
}

int Bot::GetAmmo(void)
{
	if (g_weaponDefs[m_currentWeapon].ammo1 == -1)
		return 0;

	return m_ammo[g_weaponDefs[m_currentWeapon].ammo1];
}

void Bot::TakeDamage(edict_t* inflictor, int /*damage*/, int /*armor*/, int bits)
{
	if (FNullEnt(inflictor) || inflictor == GetEntity())
		return;

	if (m_blindTime > engine->GetTime())
		return;

	if (!IsValidPlayer(inflictor))
		return;

	m_lastDamageOrigin = GetPlayerHeadOrigin(inflictor);
	m_damageTime = engine->GetTime() + 1.0f;

	if (GetTeam(inflictor) == m_team)
		return;

	if (!IsZombieMode() && m_currentWeapon == WEAPON_KNIFE)
		SelectBestWeapon();

	m_lastDamageType = bits;

	// remove some tasks
	RemoveCertainTask(TASK_HIDE);
	RemoveCertainTask(TASK_PAUSE);
	RemoveCertainTask(TASK_PLANTBOMB);
	RemoveCertainTask(TASK_DOUBLEJUMP);
	RemoveCertainTask(TASK_SPRAYLOGO);

	if (m_isZombieBot)
	{
		if (FNullEnt(m_enemy) && FNullEnt(m_moveTargetEntity))
		{
			if (IsEnemyViewable(inflictor, false, true))
				SetMoveTarget(inflictor);
			else
				goto lastly;
		}

		return;
	}

	if (FNullEnt(m_enemy))
	{
	lastly:

		SetLastEnemy(inflictor);

		m_seeEnemyTime = engine->GetTime();
	}
}

// this function gets called by network message handler, when screenfade message get's send
// it's used to make bot blind froumd the grenade
void Bot::TakeBlinded(Vector fade, int alpha)
{
	if (fade.x != 255 || fade.y != 255 || fade.z != 255 || alpha <= 170)
		return;

	SetEnemy(nullptr);

	m_maxViewDistance = engine->RandomFloat(10.0f, 20.0f);
	m_blindTime = engine->GetTime() + static_cast <float> (alpha - 200) / 16.0f;

	m_blindCampPoint = FindDefendWaypoint(GetEntityOrigin(GetEntity()));
	if ((g_waypoint->GetPath(m_blindCampPoint)->origin - GetEntityOrigin(GetEntity())).GetLength() >= 512.0f)
		m_blindCampPoint = -1;

	if (IsValidWaypoint(m_blindCampPoint))
		return;

	m_blindMoveSpeed = -pev->maxspeed;
	m_blindSidemoveSpeed = 0.0f;

	if (ChanceOf(50))
		m_blindSidemoveSpeed = pev->maxspeed;
	else
		m_blindSidemoveSpeed = -pev->maxspeed;

	if (m_personality == PERSONALITY_RUSHER)
		m_blindMoveSpeed = -GetWalkSpeed();
	else if (m_personality == PERSONALITY_CAREFUL)
		m_blindMoveSpeed = -pev->maxspeed;
	else
	{
		m_blindMoveSpeed = 0.0f;
		m_blindButton = IN_DUCK;
	}
}

// this function, asks bot to discard his current primary weapon (or c4) to the user that requsted it with /drop*
// command, very useful, when i don't have money to buy anything... )
void Bot::DiscardWeaponForUser(edict_t* user, bool discardC4)
{
	if (IsAlive(user) && m_moneyAmount >= 2000 && HasPrimaryWeapon() && (GetEntityOrigin(user) - pev->origin).GetLength() <= 240)
	{
		m_aimFlags |= AIM_ENTITY;
		m_lookAt = GetEntityOrigin(user);

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
		RadioMessage(Radio_Negative);
		ChatSay(false, FormatBuffer("Sorry %s, i don't want discard my %s to you!", GetEntityName(user), discardC4 ? "bomb" : "weapon"));
	}
}

void Bot::ResetDoubleJumpState(void)
{
	TaskComplete();
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
	float gravity = engine->GetGravity() * 0.55f;

	end = end - pev->velocity;
	end.z -= 15.0f;

	if (fabsf(end.z - start.z) > 500.0f)
		return nullvec;

	Vector midPoint = start + (end - start) * 0.5f;
	TraceHull(midPoint, midPoint + Vector(0.0f, 0.0f, 500.0f), true, head_hull, ENT(pev), &tr);

	if (tr.flFraction < 1.0f)
	{
		midPoint = tr.vecEndPos;
		midPoint.z = tr.pHit->v.absmin.z - 1.0f;
	}

	if ((midPoint.z < start.z) || (midPoint.z < end.z))
		return nullvec;

	float timeOne = Q_rsqrt((midPoint.z - start.z) / (0.5f * gravity));
	float timeTwo = Q_rsqrt((midPoint.z - end.z) / (0.5f * gravity));

	if (timeOne < 0.1)
		return nullvec;

	Vector nadeVelocity = (end - start) / (timeOne + timeTwo);
	nadeVelocity.z = gravity * timeOne;

	Vector apex = start + nadeVelocity * timeOne;
	apex.z = midPoint.z;

	TraceHull(start, apex, false, head_hull, ENT(pev), &tr);

	if (tr.flFraction < 1.0f || tr.fAllSolid)
		return nullvec;

	TraceHull(end, apex, true, head_hull, ENT(pev), &tr);

	if (tr.flFraction != 1.0f)
	{
		float dot = -(tr.vecPlaneNormal | (apex - end).Normalize());

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

	float gravity = engine->GetGravity() * 0.55f;
	float time = nadeVelocity.GetLength() / 195.0f;

	if (time < 0.01f)
		return nullvec;
	else if (time > 2.0f)
		time = 1.2f;

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
		float dot = -(tr.vecPlaneNormal | (apex - end).Normalize());

		if (dot > 0.7f || tr.flFraction < 0.8f)
			return nullvec;
	}

	return nadeVelocity * 0.7793f;
}

// this function checks if bomb is can be heard by the bot, calculations done by manual testing
Vector Bot::CheckBombAudible(void)
{
	if (!g_bombPlanted || (GetCurrentTask()->taskID == TASK_ESCAPEFROMBOMB))
		return nullvec; // reliability check

	Vector bombOrigin = g_waypoint->GetBombPosition();

	if (m_skill > 90)
		return bombOrigin;

	float timeElapsed = ((engine->GetTime() - g_timeBombPlanted) / engine->GetC4TimerTime()) * 100;
	float desiredRadius = 768.0f;

	// start the manual calculations
	if (timeElapsed > 85.0f)
		desiredRadius = 4096.0f;
	else if (timeElapsed > 68.0f)
		desiredRadius = 2048.0f;
	else if (timeElapsed > 52.0f)
		desiredRadius = 1280.0f;
	else if (timeElapsed > 28.0f)
		desiredRadius = 1024.0f;

	// we hear bomb if length greater than radius
	if (desiredRadius < (pev->origin - bombOrigin).GetLength2D())
		return bombOrigin;

	return nullvec;
}

void Bot::MoveToVector(Vector to)
{
	if (to == nullvec)
		return;

	FindPath(m_currentWaypointIndex, g_waypoint->FindNearest(to));
}

void Bot::RunPlayerMovement(void)
{
	// the purpose of this function is to compute, according to the specified computation
	// method, the msec value which will be passed as an argument of pfnRunPlayerMove. This
	// function is called every frame for every bot, since the RunPlayerMove is the function
	// that tells the engine to put the bot character model in movement. This msec value
	// tells the engine how long should the movement of the model extend inside the current
	// frame. It is very important for it to be exact, else one can experience bizarre
	// problems, such as bots getting stuck into each others. That's because the model's
	// bounding boxes, which are the boxes the engine uses to compute and detect all the
	// collisions of the model, only exist, and are only valid, while in the duration of the
	// movement. That's why if you get a pfnRunPlayerMove for one bot that lasts a little too
	// short in comparison with the frame's duration, the remaining time until the frame
	// elapses, that bot will behave like a ghost : no movement, but bullets and players can
	// pass through it. Then, when the next frame will begin, the stucking problem will arise !

	m_msecVal = static_cast <uint8_t> ((engine->GetTime() - m_msecInterval) * 1000.0f);
	m_msecInterval = engine->GetTime();

	(*g_engfuncs.pfnRunPlayerMove) (GetEntity(),
		m_moveAnglesForRunMove, m_moveSpeedForRunMove, m_strafeSpeedForRunMove, 0.0f,
		static_cast <unsigned short> (pev->button),
		0,
		static_cast <uint8_t> (m_msecVal));
}

// this function checks burst mode, and switch it depending distance to to enemy
void Bot::CheckBurstMode(float distance)
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
		int random = (m_personality == PERSONALITY_RUSHER ? 33 : m_personality == PERSONALITY_CAREFUL ? 99 : 66);

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

	float timeLeft = ((g_timeBombPlanted + engine->GetC4TimerTime()) - engine->GetTime());

	if (timeLeft < 0.0f)
		return 0.0f;

	return timeLeft;
}

float Bot::GetEstimatedReachTime(void)
{
	if (!IsValidWaypoint(m_currentWaypointIndex) || (m_isZombieBot && !m_isStuck && m_damageTime >= engine->GetTime()))
		return engine->GetTime();

	float estimatedTime = 5.0f; // time to reach next waypoint

	// calculate 'real' time that we need to get from one waypoint to another
	if (m_currentWaypointIndex >= 0 && m_currentWaypointIndex < g_numWaypoints && m_prevWptIndex >= 0 && m_prevWptIndex < g_numWaypoints)
	{
		const float distance = (g_waypoint->GetPath(m_prevWptIndex)->origin - g_waypoint->GetPath(m_currentWaypointIndex)->origin).GetLength();

		// caclulate estimated time
		estimatedTime = 5.0f * distance / fabsf(pev->maxspeed);

		// check for special waypoints, that can slowdown our movement
		if ((g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CROUCH) || (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_LADDER) || (pev->button & IN_DUCK))
			estimatedTime *= 3.0f;

		// check for too low values
		if (estimatedTime < 3.0f)
			estimatedTime = 3.0f;

		// check for too high values
		if (estimatedTime > 8.0f)
			estimatedTime = 8.0f;
	}

	return estimatedTime + 1.0f;
}

bool Bot::OutOfBombTimer(void)
{
	if (!(g_mapType & MAP_DE))
		return false;

	if (!m_isSlowThink)
		return false;

	if (!IsValidWaypoint(m_currentWaypointIndex) || m_hasProgressBar || GetCurrentTask()->taskID == TASK_ESCAPEFROMBOMB || GetCurrentTask()->taskID == TASK_DEFUSEBOMB)
		return false; // if CT bot already start defusing, or already escaping, return false

	// calculate left time
	float timeLeft = GetBombTimeleft();

	// if time left greater than 10, no need to do other checks
	if (timeLeft >= 10.0f)
		return false;
	else if (m_team == TEAM_TERRORIST)
		return true;

	const Vector& bombOrigin = g_waypoint->GetBombPosition();

	// bot will belive still had a chance
	if ((m_hasDefuser && IsVisible(bombOrigin, GetEntity())) || (bombOrigin - pev->origin).GetLength() <= 512.0f)
		return false;

	bool hasTeammatesWithDefuserKit = false;
	// check if our teammates has defusal kit
	if (m_numFriendsLeft > 0)
	{
		for (const auto& client : g_clients)
		{
			Bot* bot = g_botManager->GetBot(client.ent); // temporaly pointer to bot

			// search players with defuse kit
			if (bot != nullptr && bot->m_team == TEAM_COUNTER && bot->m_hasDefuser && (bombOrigin - bot->pev->origin).GetLength() <= 512)
			{
				hasTeammatesWithDefuserKit = true;
				break;
			}
		}
	}

	// add reach time to left time
	float reachTime = g_waypoint->GetTravelTime(pev->maxspeed, g_waypoint->GetPath(m_currentWaypointIndex)->origin, bombOrigin);

	// for counter-terrorist check alos is we have time to reach position plus average defuse time
	if ((timeLeft < reachTime + 8.0f && !m_hasDefuser && !hasTeammatesWithDefuserKit) || (timeLeft < reachTime + 4.0f && m_hasDefuser))
		return true;

	if ((m_hasDefuser ? 5.0f : 10.0f) + GetEstimatedReachTime() < GetBombTimeleft())
		return true;

	return false; // return false otherwise
}

void Bot::ReactOnSound(void)
{
	if (IsZombieMode())
		return;

	if (!m_isSlowThink)
		return;

	if (!FNullEnt(m_enemy))
		return;

	if (g_clients[GetIndex()].timeSoundLasting <= engine->GetTime())
		return;

	edict_t* player = nullptr;
	// loop through all enemy clients to check for hearable stuff
	for (const auto& client : g_clients)
	{
		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.ent == GetEntity())
			continue;

		if (client.ent->v.flags & FL_DUCKING)
			continue;

		float distance = (client.soundPosition - pev->origin).GetLength();
		float hearingDistance = client.hearingDistance;

		if (distance > hearingDistance || hearingDistance >= 2048.0f || distance > m_maxhearrange)
			continue;
		
		// focus to nearest sound.
		float maxdist = 999999.0f;
		if (distance < maxdist)
		{
			player = client.ent;
			maxdist = distance;
		}
	}

	// did the bot hear someone ?
	if (!FNullEnt(player))
	{
		// change to best weapon if heard something
		if (!(m_states & STATE_SEEINGENEMY) && IsOnFloor() && m_currentWeapon != WEAPON_C4 && m_currentWeapon != WEAPON_HEGRENADE && m_currentWeapon != WEAPON_SMGRENADE && m_currentWeapon != WEAPON_FBGRENADE && !ebot_knifemode.GetBool())
			SelectBestWeapon();

		m_heardSoundTime = engine->GetTime();
		m_states |= STATE_HEARENEMY;

		m_lookAt = GetEntityOrigin(player);

		if (m_team != GetTeam(player) || GetGameMode() == MODE_DM)
		{
			// didn't bot already have an enemy ? take this one...
			if (m_lastEnemyOrigin == nullvec || m_lastEnemy == nullptr)
				SetLastEnemy(player);
			else // bot had an enemy, check if it's the heard one
			{
				if (player == m_lastEnemy)
				{
					// bot sees enemy ? then bail out !
					if (m_states & STATE_SEEINGENEMY)
						return;

					SetLastEnemy(player);
				}
				else
				{
					// if bot had an enemy but the heard one is nearer, take it instead
					float distance = (m_lastEnemyOrigin - pev->origin).GetLength();
					if (distance <= (GetEntityOrigin(player) - pev->origin).GetLength() && m_seeEnemyTime + 2.0f < engine->GetTime())
						return;

					SetLastEnemy(player);
				}
			}

			// check if heard enemy can be seen
			if (FNullEnt(m_enemy) && m_lastEnemy == player && m_seeEnemyTime + 3.0f > engine->GetTime() && m_skill >= 60 && IsShootableThruObstacle(player))
			{
				SetEnemy(player);
				SetLastEnemy(player);

				m_states |= STATE_SEEINGENEMY;
				m_seeEnemyTime = engine->GetTime();
			}
		}
	}
}

bool Bot::IsShootableBreakable(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	if (FClassnameIs(ent, "func_breakable") || (FClassnameIs(ent, "func_pushable") && (ent->v.spawnflags & SF_PUSH_BREAKABLE)) || (FClassnameIs(ent, "func_wall") && ent->v.health <= 500))
	{
		if (ent->v.takedamage != DAMAGE_NO && ent->v.impulse <= 0 && !(ent->v.flags & FL_WORLDBRUSH) && !(ent->v.spawnflags & SF_BREAK_TRIGGER_ONLY))
			return (ent->v.movetype == MOVETYPE_PUSH || ent->v.movetype == MOVETYPE_PUSHSTEP);
	}

	return false;
}

// this function is gets called when bot enters a buyzone, to allow bot to buy some stuff
void Bot::EquipInBuyzone(int iBuyCount)
{
	static float lastEquipTime = 0.0f;

	// if bot is in buy zone, try to buy ammo for this weapon...
	if (lastEquipTime + 15.0f < engine->GetTime() && m_inBuyZone && g_timeRoundStart + engine->RandomFloat(10.0f, 20.0f) + engine->GetBuyTime() < engine->GetTime() && !g_bombPlanted && m_moneyAmount > 700)
	{
		m_buyingFinished = false;
		m_buyState = iBuyCount;

		// push buy message
		PushMessageQueue(CMENU_BUY);

		m_nextBuyTime = engine->GetTime();
		lastEquipTime = engine->GetTime();
	}
}

bool Bot::IsBombDefusing(Vector bombOrigin)
{
	// this function finds if somebody currently defusing the bomb.
	if (m_numFriendsLeft <= 0)
		return false;

	if (!g_bombPlanted)
		return false;

	bool defusingInProgress = false;
	constexpr float distanceToBomb = 75.0f;

	for (const auto& client : g_clients)
	{
		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE))
			continue;

		Bot* bot = g_botManager->GetBot(client.ent);

		float bombDistance = (client.origin - bombOrigin).GetLength();
		if (bot != nullptr && bot->m_notKilled)
		{
			if (m_team != bot->m_team || bot->GetCurrentTask()->taskID == TASK_ESCAPEFROMBOMB || bot->GetCurrentTask()->taskID == TASK_CAMP || bot->GetCurrentTask()->taskID == TASK_SEEKCOVER)
				continue; // skip other mess

			// if close enough, mark as progressing
			if (bombDistance < distanceToBomb && (bot->GetCurrentTask()->taskID == TASK_DEFUSEBOMB || bot->m_hasProgressBar))
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