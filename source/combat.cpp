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

#include <core.h>

ConVar ebot_escape("ebot_zombie_escape_mode", "0");
ConVar ebot_zp_use_grenade_percent("ebot_zm_use_grenade_percent", "10");
ConVar ebot_zp_escape_distance("ebot_zm_escape_distance", "200");
ConVar ebot_zombie_speed_factor("ebot_zombie_speed_factor", "0.54");
ConVar ebot_sb_mode("ebot_sb_mode", "0");

int Bot::GetNearbyFriendsNearPosition(Vector origin, float radius)
{
	if (GetGameMode() == MODE_DM)
		return 0;

	if (origin == nullvec)
		return 0;

	int count = 0;
	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (client.ent == nullptr)
			continue;

		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == GetEntity())
			continue;

		if ((client.origin - origin).GetLengthSquared() <= SquaredF(radius))
			count++;
	}

	return count;
}

int Bot::GetNearbyEnemiesNearPosition(Vector origin, float radius)
{
	if (origin == nullvec)
		return 0;

	int count = 0;
	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (client.ent == nullptr)
			continue;

		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team == m_team)
			continue;

		if ((client.origin - origin).GetLengthSquared() <= SquaredF(radius))
			count++;
	}

	return count;
}

void Bot::FindEnemyEntities(void)
{
	m_entityDistance = FLT_MAX;
	m_entitiesNearCount = 0;
	m_hasEntitiesNear = false;

	for (int i = engine->GetMaxClients() + 1; i < entityNum; i++)
	{
		if (g_entityId[i] == -1 || g_entityAction[i] != 1 || m_team == g_entityTeam[i])
			continue;

		edict_t* entity = INDEXENT(g_entityId[i]);
		if (FNullEnt(entity) || !IsAlive(entity) || entity->v.effects & EF_NODRAW || entity->v.takedamage == DAMAGE_NO)
			continue;

		// simple check
		TraceResult tr;
		const Vector origin = GetEntityOrigin(entity);
		TraceLine(pev->origin, origin, true, true, GetEntity(), &tr);
		if (tr.flFraction != 1.0f)
			continue;

		m_entitiesNearCount++;
		const float distance = (pev->origin - origin).GetLengthSquared();
		if (distance < m_entityDistance)
		{
			m_entityDistance = distance;
			m_nearestEntity = entity;
		}
	}

	m_hasEntitiesNear = m_entitiesNearCount > 0;
	if (m_hasEntitiesNear)
	{
		m_entityOrigin = GetEntityOrigin(m_nearestEntity);
		m_entitySeeTime = engine->GetTime();
	}
}

void Bot::FindFriendsAndEnemiens(void)
{
	m_enemyDistance = FLT_MAX;
	m_friendDistance = FLT_MAX;
	m_enemiesNearCount = 0;
	m_friendsNearCount = 0;
	m_hasEnemiesNear = false;
	m_hasFriendsNear = false;

	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (client.ent == nullptr)
			continue;

		if (client.ent == GetEntity())
			continue;

		if (!IsAlive(client.ent))
			continue;

		if (client.team == m_team)
		{
			// simple check
			TraceResult tr;
			TraceLine(pev->origin, client.origin, true, true, GetEntity(), &tr);
			if (tr.flFraction != 1.0f)
				continue;

			m_friendsNearCount++;
			const float distance = (pev->origin - client.origin).GetLengthSquared();
			if (distance < m_friendDistance)
			{
				m_friendDistance = distance;
				m_nearestFriend = client.ent;
			}
		}
		else
		{
			if (!IsEnemyViewable(client.ent))
				continue;

			// we don't know this enemy, where it can be?
			if (!m_isZombieBot && client.ent != m_nearestEnemy)
			{
				if (!IsAttacking(client.ent) && !IsInViewCone(client.origin))
					continue;

				if (IsBehindSmokeClouds(client.ent))
					continue;
			}

			m_enemiesNearCount++;
			const float distance = (pev->origin - client.origin).GetLengthSquared();
			if (distance < m_enemyDistance)
			{
				m_enemyDistance = distance;
				m_nearestEnemy = client.ent;
			}
		}
	}

	m_hasEnemiesNear = m_enemiesNearCount > 0;
	m_hasFriendsNear = m_friendsNearCount > 0;

	// enemyOrigin will be added with IsEnemyViewable function unlike friendOrigin
	if (m_hasEnemiesNear)
		m_enemySeeTime = engine->GetTime();

	if (m_hasFriendsNear)
	{
		m_friendOrigin = m_nearestFriend->v.origin;
		m_friendSeeTime = engine->GetTime();
	}
}

void Bot::ResetCheckEnemy()
{
	int i;
	edict_t* entity = nullptr;
	m_checkEnemyNum = 0;
	for (i = 0; i < checkEnemyNum; i++)
	{
		m_allEnemy[i] = nullptr;
		m_allEnemyDistance[i] = FLT_MAX;

		m_checkEnemy[i] = nullptr;
		m_checkEnemyDistance[i] = FLT_MAX;
	}

	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (client.ent == nullptr)
			continue;

		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team == m_team)
			continue;

		entity = client.ent;

		m_allEnemy[m_checkEnemyNum] = entity;
		m_allEnemyDistance[m_checkEnemyNum] = (pev->origin - client.origin).GetLengthSquared();
		m_checkEnemyNum++;
	}

	for (i = 0; i < entityNum; i++)
	{
		if (g_entityId[i] == -1 || g_entityAction[i] != 1 || m_team == g_entityTeam[i])
			continue;

		entity = INDEXENT(g_entityId[i]);
		if (FNullEnt(entity) || !IsAlive(entity) || entity->v.effects & EF_NODRAW || entity->v.takedamage == DAMAGE_NO)
			continue;

		m_allEnemy[m_checkEnemyNum] = entity;
		m_allEnemyDistance[m_checkEnemyNum] = (pev->origin - GetEntityOrigin(entity)).GetLengthSquared();
		m_checkEnemyNum++;
	}

	for (i = 0; i < m_checkEnemyNum; i++)
	{
		for (int y = 0; y < checkEnemyNum; y++)
		{
			if (m_allEnemyDistance[i] > m_checkEnemyDistance[y])
				continue;

			if (m_allEnemyDistance[i] == m_checkEnemyDistance[y])
			{
				Vector myVec = pev->origin + pev->velocity * m_frameInterval;
				if ((myVec - GetEntityOrigin(m_allEnemy[i])).GetLengthSquared2D() > (myVec - GetEntityOrigin(m_checkEnemy[y])).GetLengthSquared2D())
					continue;
			}

			for (int z = m_checkEnemyNum - 1; z >= y; z--)
			{
				if (z == m_checkEnemyNum - 1 || m_checkEnemy[z] == nullptr)
					continue;

				m_checkEnemy[z + 1] = m_checkEnemy[z];
				m_checkEnemyDistance[z + 1] = m_checkEnemyDistance[z];
			}

			m_checkEnemy[y] = m_allEnemy[i];
			m_checkEnemyDistance[y] = m_allEnemyDistance[i];

			break;
		}
	}
}

bool Bot::LookupEnemy(void)
{
	m_visibility = Visibility::None;
	if (m_blindTime > engine->GetTime())
		return false;

	int i;
	edict_t* entity = nullptr;
	edict_t* targetEntity = nullptr;
	float enemy_distance = FLT_MAX;
	edict_t* oneTimeCheckEntity = nullptr;

	if (!FNullEnt(m_lastEnemy))
	{
		if (!IsAlive(m_lastEnemy) || (m_team == GetTeam(m_lastEnemy)) || IsNotAttackLab(m_lastEnemy))
			SetLastEnemy(nullptr);
	}

	if (!FNullEnt(m_enemy))
	{
		if (!IsAlive(m_enemy) || m_team == GetTeam(m_enemy) || IsNotAttackLab(m_enemy))
		{
			SetEnemy(nullptr);
			SetLastEnemy(nullptr);
			m_enemyUpdateTime = 0.0f;

			if (GetGameMode() == MODE_DM)
				m_fearLevel += 0.15f;
		}

		if ((m_enemyUpdateTime > engine->GetTime()))
		{
			if (IsEnemyViewable(m_enemy) || IsShootableThruObstacle(m_enemy))
			{
				m_aimFlags |= AIM_ENEMY;
				return true;
			}

			oneTimeCheckEntity = m_enemy;
		}

		targetEntity = m_enemy;
		enemy_distance = (pev->origin - m_enemyOrigin).GetLengthSquared();
	}
	else if (!FNullEnt(m_moveTargetEntity))
	{
		const Vector origin = GetEntityOrigin(m_moveTargetEntity);
		if (m_team == GetTeam(m_moveTargetEntity) || !IsAlive(m_moveTargetEntity) || origin == nullvec)
			SetMoveTarget(nullptr);

		targetEntity = m_moveTargetEntity;
		enemy_distance = (pev->origin - origin).GetLengthSquared();
	}

	ResetCheckEnemy();

	for (i = 0; i < m_checkEnemyNum; i++)
	{
		if (m_checkEnemy[i] == nullptr)
			continue;

		entity = m_checkEnemy[i];
		if (entity == oneTimeCheckEntity)
			continue;

		if (m_blindRecognizeTime < engine->GetTime() && IsBehindSmokeClouds(entity))
			m_blindRecognizeTime = engine->GetTime() + engine->RandomFloat(2.0f, 3.0f);

		if (m_blindRecognizeTime >= engine->GetTime())
			continue;

		if (IsValidPlayer(entity) && IsEnemyProtectedByShield(entity))
			continue;

		if (IsEnemyViewable(entity))
		{
			enemy_distance = m_checkEnemyDistance[i];
			targetEntity = entity;
			oneTimeCheckEntity = entity;

			break;
		}
	}

	if (!FNullEnt(m_moveTargetEntity) && m_moveTargetEntity != targetEntity)
	{
		if (m_currentWaypointIndex != GetEntityWaypoint(targetEntity))
		{
			const float distance = (pev->origin - GetEntityOrigin(m_moveTargetEntity)).GetLengthSquared();
			if (distance <= SquaredF(enemy_distance + 400.0f))
			{
				const int targetWpIndex = GetEntityWaypoint(targetEntity);
				bool shortDistance = false;

				const Path* path = g_waypoint->GetPath(m_currentWaypointIndex);
				for (int j = 0; j < Const_MaxPathIndex; j++)
				{
					if (path->index[j] != targetWpIndex)
						continue;

					if (path->connectionFlags[j] & PATHFLAG_JUMP)
						break;

					shortDistance = true;
				}

				if (shortDistance == false)
				{
					enemy_distance = distance;
					targetEntity = nullptr;
				}
			}
		}
	}

	// last checking
	if (!FNullEnt(targetEntity) && !IsEnemyViewable(targetEntity))
		targetEntity = nullptr;

	if (!FNullEnt(m_enemy) && FNullEnt(targetEntity))
	{
		if (m_isZombieBot || (ebot_knifemode.GetBool() && targetEntity == m_moveTargetEntity))
		{
			g_botsCanPause = false;

			SetMoveTarget(m_enemy);
			return false;
		}
		else if (IsShootableThruObstacle(m_enemy))
		{
			m_visibility = Visibility::None;
			return true;
		}
	}

	if (!FNullEnt(targetEntity))
	{
		if (m_isZombieBot || ebot_knifemode.GetBool())
		{
			bool moveTotarget = true;
			int movePoint = 0;

			const int srcIndex = GetEntityWaypoint(GetEntity());
			const int destIndex = GetEntityWaypoint(targetEntity);
			if ((m_currentTravelFlags & PATHFLAG_JUMP))
				movePoint = 10;
			else if (srcIndex == destIndex || m_currentWaypointIndex == destIndex)
				moveTotarget = false;
			else
			{
				Path* path;
				while (srcIndex != destIndex && movePoint <= 3 && srcIndex >= 0 && destIndex >= 0)
				{
					path = g_waypoint->GetPath(srcIndex);
					if (!IsValidWaypoint(srcIndex))
						continue;

					movePoint++;
					for (int j = 0; j < Const_MaxPathIndex; j++)
					{
						if (path->index[j] == srcIndex &&
							path->connectionFlags[j] & PATHFLAG_JUMP)
						{
							movePoint += 3;
							break;
						}
					}
				}
			}

			enemy_distance = (GetEntityOrigin(targetEntity) - pev->origin).GetLengthSquared();
			if ((enemy_distance <= SquaredF(150.0f) && movePoint <= 1) || (targetEntity == m_moveTargetEntity && movePoint <= 2))
			{
				moveTotarget = false;
				if (targetEntity == m_moveTargetEntity && movePoint <= 1)
					m_enemyUpdateTime = engine->GetTime() + 4.0f;
			}

			if (moveTotarget)
			{
				KnifeAttack();

				if (targetEntity != m_moveTargetEntity)
				{
					g_botsCanPause = false;

					m_targetEntity = nullptr;
					SetMoveTarget(targetEntity);
				}

				return false;
			}

			if (m_enemyUpdateTime < engine->GetTime() + 3.0f)
				m_enemyUpdateTime = engine->GetTime() + 2.5f;
		}

		g_botsCanPause = true;
		m_aimFlags |= AIM_ENEMY;

		if (targetEntity == m_enemy)
		{
			m_seeEnemyTime = engine->GetTime();
			SetLastEnemy(targetEntity);

			return true;
		}

		if (m_seeEnemyTime + 3.0f < engine->GetTime() && (m_isBomber || HasHostage() || !FNullEnt(m_targetEntity)))
			RadioMessage(Radio::EnemySpotted);

		m_targetEntity = nullptr;

		SetEnemy(targetEntity);
		SetLastEnemy(m_enemy);
		m_seeEnemyTime = engine->GetTime();

		if (!m_isZombieBot)
			m_enemyUpdateTime = engine->GetTime() + 1.0f;
		else
			m_enemyUpdateTime = engine->GetTime() + 0.64f;

		return true;
	}

	if ((m_aimFlags <= AIM_PREDICTENEMY && m_seeEnemyTime + 4.0f < engine->GetTime() && !(m_states & (STATE_SEEINGENEMY | STATE_HEARENEMY)) && FNullEnt(m_lastEnemy) && FNullEnt(m_enemy) && GetCurrentTask()->taskID != TASK_DESTROYBREAKABLE && GetCurrentTask()->taskID != TASK_PLANTBOMB && GetCurrentTask()->taskID != TASK_DEFUSEBOMB) || g_roundEnded)
	{
		if (!m_reloadState)
			m_reloadState = ReloadState::Primary;
	}

	if ((UsesSniper() || UsesZoomableRifle()) && m_zoomCheckTime + 1.0f < engine->GetTime())
	{
		if (pev->fov < 90)
			pev->button |= IN_ATTACK2;
		else
			m_zoomCheckTime = 0.0f;
	}

	return false;
}

Vector Bot::GetEnemyPosition(void)
{
	// not even visible?
	if (m_visibility == Visibility::None)
		return m_enemyOrigin;

	// get enemy position initially
	Vector targetOrigin = m_nearestEnemy->v.origin;

	const float distance = (targetOrigin - pev->origin).GetLengthSquared();

	// now take in account different parts of enemy body
	if (m_visibility & (Visibility::Head | Visibility::Body)) // visible head & body
	{
		// now check is our skill match to aim at head, else aim at enemy body
		if (IsZombieMode() || ChanceOf(m_skill) || UsesPistol())
			targetOrigin += m_nearestEnemy->v.view_ofs + Vector(0.0f, 0.0f, GetZOffset(distance));
		else
			targetOrigin += Vector(0.0f, 0.0f, GetZOffset(distance));
	}
	else if (m_visibility & Visibility::Head) // visible only head
		targetOrigin += m_nearestEnemy->v.view_ofs + Vector(0.0f, 0.0f, GetZOffset(distance));
	else if (m_visibility & Visibility::Body) // visible only body
		targetOrigin += Vector(0.0f, 0.0f, GetZOffset(distance));
	else if (m_visibility & Visibility::Other) // random part of body is visible
		targetOrigin = m_enemyOrigin;

	return targetOrigin;
}

float Bot::GetZOffset(float distance)
{
	if (m_difficulty < 1)
		return -4.0f;

	const bool sniper = UsesSniper();
	const bool pistol = UsesPistol();
	const bool rifle = UsesRifle();

	const bool zoomableRifle = UsesZoomableRifle();
	const bool submachine = UsesSubmachineGun();
	const bool shotgun = (m_currentWeapon == WEAPON_XM1014 || m_currentWeapon == WEAPON_M3);
	const bool m249 = m_currentWeapon == WEAPON_M249;

	const float BurstDistance = SquaredF(300.0f);
	const float DoubleBurstDistance = BurstDistance * 2.0f;

	float result = 4.0f;

	if (distance <= SquaredF(2800.0f) && distance > DoubleBurstDistance)
	{
		if (sniper) result = 3.5f;
		else if (zoomableRifle) result = 4.5f;
		else if (pistol) result = 6.5f;
		else if (submachine) result = 5.5f;
		else if (rifle) result = 5.5f;
		else if (m249) result = 2.5f;
		else if (shotgun) result = 10.5f;
	}
	else if (distance > BurstDistance && distance <= DoubleBurstDistance)
	{
		if (sniper) result = 3.5f;
		else if (zoomableRifle) result = 3.5f;
		else if (pistol) result = 6.5f;
		else if (submachine) result = 3.5f;
		else if (rifle) result = 1.6f;
		else if (m249) result = -1.0f;
		else if (shotgun) result = 10.0f;
	}
	else if (distance < BurstDistance)
	{
		if (sniper) result = 4.5f;
		else if (zoomableRifle) result = 5.0f;
		else if (pistol) result = 4.5f;
		else if (submachine) result = 4.5f;
		else if (rifle) result = 4.5f;
		else if (m249) result = 6.0f;
		else if (shotgun) result = 5.0f;
	}

	return result;
}

// bot can't hurt teammates, if friendly fire is not enabled...
bool Bot::IsFriendInLineOfFire(float distance)
{
	if (g_gameVersion == HALFLIFE || !engine->IsFriendlyFireOn() || GetGameMode() == MODE_DM || GetGameMode() == MODE_NOTEAM)
		return false;

	MakeVectors(pev->v_angle);

	TraceResult tr;
	TraceLine(EyePosition(), EyePosition() + distance * pev->v_angle.Normalize(), false, false, GetEntity(), &tr);

	if (!FNullEnt(tr.pHit))
	{
		if (IsAlive(tr.pHit) && m_team == GetTeam(tr.pHit))
		{
			if (IsValidPlayer(tr.pHit))
				return true;

			const int entityIndex = ENTINDEX(tr.pHit);
			for (int i = 0; i < entityNum; i++)
			{
				if (g_entityId[i] == -1 || g_entityAction[i] != 1)
					continue;

				if (g_entityId[i] == entityIndex)
					return true;
			}
		}
	}

	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (client.ent == nullptr)
			continue;

		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == GetEntity())
			continue;

		const Vector origin = client.ent->v.origin;
		const float friendDistance = (origin - pev->origin).GetLengthSquared();

		if (friendDistance <= distance && GetShootingConeDeviation(GetEntity(), &origin) > (friendDistance / (friendDistance + 1089.0f)))
			return true;
	}

	return false;
}

int CorrectGun(int weaponID)
{
	if (GetGameMode() != MODE_BASE)
		return 0;

	if (weaponID == WEAPON_AUG || weaponID == WEAPON_M4A1 || weaponID == WEAPON_SG552 || weaponID == WEAPON_AK47 || weaponID == WEAPON_FAMAS || weaponID == WEAPON_GALIL)
		return 2;
	else if (weaponID == WEAPON_SG552 || weaponID == WEAPON_G3SG1)
		return 3;

	return 0;
}

bool Bot::IsShootableThruObstacle(edict_t* entity)
{
	if (!IsValidPlayer(entity) || IsZombieEntity(entity))
		return false;

	if (entity->v.health >= 60.0f)
		return false;

	int currentWeaponPenetrationPower = CorrectGun(m_currentWeapon);
	if (currentWeaponPenetrationPower == 0)
		return false;

	TraceResult tr;
	Vector dest = GetEntityOrigin(entity);

	float obstacleDistance = 0.0f;

	TraceLine(EyePosition(), dest, true, GetEntity(), &tr);

	if (tr.fStartSolid)
	{
		Vector source = tr.vecEndPos;

		TraceLine(dest, source, true, GetEntity(), &tr);
		if (tr.flFraction != 1.0f)
		{
			if ((tr.vecEndPos - dest).GetLengthSquared() > SquaredF(800.0f))
				return false;

			if (tr.vecEndPos.z >= dest.z + 200.0f)
				return false;

			if (dest.z >= tr.vecEndPos.z + 200.0f)
				return false;

			obstacleDistance = (tr.vecEndPos - source).GetLengthSquared();
		}
	}

	if (obstacleDistance > 0.0f)
	{
		while (currentWeaponPenetrationPower > 0.0f)
		{
			if (obstacleDistance > SquaredF(75.0f))
			{
				obstacleDistance -= SquaredF(75.0f);
				currentWeaponPenetrationPower--;
				continue;
			}

			return true;
		}
	}

	return false;
}

bool Bot::DoFirePause(const float distance)
{
	if (m_firePause > engine->GetTime())
		return true;

	if (m_hasEnemiesNear && IsEnemyProtectedByShield(m_nearestEnemy))
		return true;

	const float angle = (fabsf(pev->punchangle.y) + fabsf(pev->punchangle.x)) * (Math::MATH_PI * 0.00277777777f);

	// check if we need to compensate recoil
	if (tanf(angle) * (distance + (distance * 0.25f)) > 100.0f)
	{
		if (m_firePause < (engine->GetTime() - 0.4))
			m_firePause = engine->GetTime() + engine->RandomFloat(0.4f, (0.4f + 1.2f * ((100 - m_skill)) * 0.01f));

		return true;
	}

	if (UsesSniper())
	{
		if (!(m_currentTravelFlags & PATHFLAG_JUMP))
			pev->button &= ~IN_JUMP;

		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;

		if (pev->speed >= pev->maxspeed && !IsZombieMode())
		{
			m_firePause = engine->GetTime() + 0.1f;
			return true;
		}
	}

	return false;
}

// this function will return true if weapon was fired, false otherwise
void Bot::FireWeapon(void)
{
	const float distance = m_enemyDistance <= m_entityDistance ? m_enemyDistance : m_entityDistance;

	// or if friend in line of fire, stop this too but do not update shoot time
	if (IsFriendInLineOfFire(distance))
		return;

	const int melee = g_gameVersion == HALFLIFE ? WEAPON_CROWBAR : WEAPON_KNIFE;

	FireDelay* delay = &g_fireDelay[0];
	WeaponSelect* selectTab = g_gameVersion == HALFLIFE ? &g_weaponSelectHL[0] : &g_weaponSelect[0];

	edict_t* enemy = m_enemyDistance <= m_entityDistance ? m_nearestEnemy : m_nearestEntity;

	int selectId = melee, selectIndex = 0, chosenWeaponIndex = 0;
	int weapons = pev->weapons;

	if (ebot_knifemode.GetBool())
		goto WeaponSelectEnd;
	else if (!FNullEnt(enemy) && ChanceOf(m_skill) && !IsZombieEntity(enemy) && distance <= SquaredF(128.0f) && (enemy->v.health <= 30 || pev->health > enemy->v.health) && !IsOnLadder() && !IsGroupOfEnemies(enemy->v.origin))
		goto WeaponSelectEnd;

	// loop through all the weapons until terminator is found...
	while (selectTab[selectIndex].id)
	{
		// is the bot carrying this weapon?
		if (weapons & (1 << selectTab[selectIndex].id))
		{
			const int id = selectTab[selectIndex].id;

			// cannot be used in water...
			if (pev->waterlevel == 3 && g_weaponDefs[id].flags & ITEM_FLAG_NOFIREUNDERWATER)
				continue;

			// is enough ammo available to fire AND check is better to use pistol in our current situation...
			if (g_gameVersion == HALFLIFE)
			{
				if (selectIndex == WEAPON_SNARK || selectIndex == WEAPON_GAUSS ||selectIndex == WEAPON_EGON || (selectIndex == WEAPON_HANDGRENADE && distance > SquaredF(384.0f) && distance <= SquaredF(768.0f)) || (selectIndex == WEAPON_RPG && distance > SquaredF(320.0f)) || (selectIndex == WEAPON_CROSSBOW && distance > SquaredF(320.0f)))
					chosenWeaponIndex = selectIndex;
				else if (selectIndex != WEAPON_HANDGRENADE && selectIndex != WEAPON_RPG  && selectIndex != WEAPON_CROSSBOW && (m_ammoInClip[id] > 0) && !IsWeaponBadInDistance(selectIndex, distance))
						chosenWeaponIndex = selectIndex;

			}
			else if ((m_ammoInClip[id] > 0) && !IsWeaponBadInDistance(selectIndex, distance))
				chosenWeaponIndex = selectIndex;
		}

		selectIndex++;
	}

	selectId = selectTab[chosenWeaponIndex].id;

	// if no available weapon...
	if (chosenWeaponIndex == 0)
	{
		selectIndex = 0;

		// loop through all the weapons until terminator is found...
		while (selectTab[selectIndex].id)
		{
			const int id = selectTab[selectIndex].id;

			// cannot be used in water...
			if (pev->waterlevel == 3 && g_weaponDefs[id].flags & ITEM_FLAG_NOFIREUNDERWATER)
				continue;

			// is the bot carrying this weapon?
			if (weapons & (1 << id))
			{
				if (g_weaponDefs[id].ammo1 != -1 && m_ammo[g_weaponDefs[id].ammo1] >= selectTab[selectIndex].minPrimaryAmmo)
				{
					// available ammo found, reload weapon
					if (m_reloadState == ReloadState::Nothing || m_reloadCheckTime > engine->GetTime())
					{
						m_isReloading = true;
						m_reloadState = ReloadState::Primary;
						m_reloadCheckTime = engine->GetTime();
						RadioMessage(Radio::NeedBackup);
					}

					return;
				}
			}

			selectIndex++;
		}

		selectId = melee; // no available ammo, use knife!
	}

WeaponSelectEnd:
	// we want to fire weapon, don't reload now
	if (!m_isReloading)
	{
		m_reloadState = ReloadState::Nothing;
		m_reloadCheckTime = engine->GetTime() + 6.0f;
	}

	if (IsZombieMode() && m_currentWeapon == melee && selectId != melee && !m_isZombieBot)
	{
		m_reloadState = ReloadState::Primary;
		m_reloadCheckTime = engine->GetTime() + 2.5f;
		return;
	}

	if (m_currentWeapon != selectId)
	{
		SelectWeaponByName(g_weaponDefs[selectId].className);

		// reset burst fire variables
		m_firePause = 0.0f;
		m_timeLastFired = 0.0f;

		return;
	}

	if (selectTab[chosenWeaponIndex].id != selectId)
	{
		chosenWeaponIndex = 0;

		// loop through all the weapons until terminator is found...
		while (selectTab[chosenWeaponIndex].id)
		{
			if (selectTab[chosenWeaponIndex].id == selectId)
				break;

			chosenWeaponIndex++;
		}
	}

	// if we're have a glock or famas vary burst fire mode
	if (g_gameVersion != HALFLIFE)
	{
		CheckBurstMode(distance);

		if (HasShield() && m_shieldCheckTime < engine->GetTime() && GetProcess() != Process::Camp) // better shield gun usage
		{
			if ((distance > SquaredF(768.0f)) && !IsShieldDrawn())
				pev->button |= IN_ATTACK2; // draw the shield
			else if (IsShieldDrawn() || (IsValidPlayer(enemy) && (enemy->v.button & IN_RELOAD)))
				pev->button |= IN_ATTACK2; // draw out the shield

			m_shieldCheckTime = engine->GetTime() + 2.0f;
		}
	}

	if (UsesSniper() && m_zoomCheckTime < engine->GetTime()) // is the bot holding a sniper rifle?
	{
		if (distance > SquaredF(1500.0f) && pev->fov >= 40.0f) // should the bot switch to the long-range zoom?
			pev->button |= IN_ATTACK2;

		else if (distance > SquaredF(150.0f) && pev->fov >= 90.0f) // else should the bot switch to the close-range zoom ?
			pev->button |= IN_ATTACK2;

		else if (distance <= SquaredF(150.0f) && pev->fov < 90.0f) // else should the bot restore the normal view ?
			pev->button |= IN_ATTACK2;

		m_zoomCheckTime = engine->GetTime();
	}
	else if (UsesZoomableRifle() && m_zoomCheckTime < engine->GetTime() && m_skill < 90) // else is the bot holding a zoomable rifle?
	{
		if (distance > SquaredF(800.0f) && pev->fov >= 90.0f) // should the bot switch to zoomed mode?
			pev->button |= IN_ATTACK2;

		else if (distance <= SquaredF(800.0f) && pev->fov < 90.0f) // else should the bot restore the normal view?
			pev->button |= IN_ATTACK2;

		m_zoomCheckTime = engine->GetTime();
	}

	// need to care for burst fire?
	if (g_gameVersion == HALFLIFE || distance <= SquaredF(512.0f) || m_blindTime > engine->GetTime())
	{
		if (selectId == melee)
			KnifeAttack();
		else
		{
			if (selectTab[chosenWeaponIndex].primaryFireHold) // if automatic weapon, just press attack
				pev->button |= IN_ATTACK;
			else // if not, toggle buttons
			{
				if ((pev->oldbuttons & IN_ATTACK) == 0)
					pev->button |= IN_ATTACK;
			}
		}

		if (pev->button & IN_ATTACK)
			m_shootTime = engine->GetTime();
	}
	else
	{
		if (DoFirePause(distance))
			return;

		// don't attack with knife over long distance
		if (selectId == melee)
		{
			KnifeAttack();
			return;
		}

		float delayTime = 0.0f;
		if (selectTab[chosenWeaponIndex].primaryFireHold)
		{
			m_zoomCheckTime = engine->GetTime();
			pev->button |= IN_ATTACK;  // use primary attack
		}
		else
		{
			pev->button |= IN_ATTACK;  // use primary attack
			const float baseDelay = delay[chosenWeaponIndex].primaryBaseDelay;
			const float minDelay = delay[chosenWeaponIndex].primaryMinDelay[abs((m_skill / CRandomInt(15, 20)) - 5)];
			const float maxDelay = delay[chosenWeaponIndex].primaryMaxDelay[abs((m_skill / CRandomInt(20, 30)) - 5)];
			delayTime = baseDelay + engine->RandomFloat(minDelay, maxDelay);
			m_zoomCheckTime = engine->GetTime();
		}

		if (distance >= SquaredF(1200.0f))
		{
			if (m_visibility & (Visibility::Head | Visibility::Body))
				delayTime -= (delayTime == 0.0f) ? 0.0f : 0.02f;
			else if (m_visibility & Visibility::Head)
			{
				if (distance >= SquaredF(2400.0f))
					delayTime += (delayTime == 0.0f) ? 0.15f : 0.10f;
				else
					delayTime += (delayTime == 0.0f) ? 0.10f : 0.05f;
			}
			else if (m_visibility & Visibility::Body)
			{
				if (distance >= SquaredF(2400.0f))
					delayTime += (delayTime == 0.0f) ? 0.12f : 0.08f;
				else
					delayTime += (delayTime == 0.0f) ? 0.08f : 0.0f;
			}
			else
			{
				if (distance >= SquaredF(2400.0f))
					delayTime += (delayTime == 0.0f) ? 0.18f : 0.15f;
				else
					delayTime += (delayTime == 0.0f) ? 0.15f : 0.10f;
			}
		}

		m_shootTime = engine->GetTime() + delayTime;
	}
}

bool Bot::KnifeAttack(float attackDistance)
{
	Vector origin = nullvec;
	float distance = FLT_MAX;

	if (m_enemyDistance <= m_entityDistance)
	{
		origin = m_enemyOrigin;
		distance = m_enemyDistance;
	}
	else
	{
		origin = m_entityOrigin;
		distance = m_entityDistance;
	}

	float kad1 = SquaredF(pev->speed * 0.26);
	const float kad2 = SquaredF(64.0f);

	if (attackDistance != 0.0f)
		kad1 = attackDistance;

	int kaMode = 0;

	if (distance <= kad1)
		kaMode = 1;

	if (distance <= kad2)
		kaMode += 2;

	if (kaMode > 0)
	{
		const float distanceSkipZ = (pev->origin - origin).GetLengthSquared2D();

		if (pev->origin.z > origin.z && distanceSkipZ < SquaredF(64.0f))
		{
			pev->button |= IN_DUCK;
			m_campButtons |= IN_DUCK;
			pev->button &= ~IN_JUMP;
		}
		else
		{
			pev->button &= ~IN_DUCK;
			m_campButtons &= ~IN_DUCK;

			if (pev->origin.z + 150.0f < origin.z && distanceSkipZ < SquaredF(300.0f))
				pev->button |= IN_JUMP;
		}

		if (m_isZombieBot)
		{
			if (kaMode != 2)
				pev->button |= IN_ATTACK;
			else
				pev->button |= IN_ATTACK2;
		}
		else
		{
			if (kaMode == 1)
				pev->button |= IN_ATTACK;
			else if (kaMode == 2)
				pev->button |= IN_ATTACK2;
			else if (CRandomInt(1, 10) < 3 || HasShield())
				pev->button |= IN_ATTACK;
			else
				pev->button |= IN_ATTACK2;
		}

		return true;
	}

	return false;
}

// this function checks, is it better to use pistol instead of current primary weapon
// to attack our enemy, since current weapon is not very good in this situation
bool Bot::IsWeaponBadInDistance(int weaponIndex, float distance)
{
	if (g_gameVersion == HALFLIFE)
	{
		int weaponID = g_weaponSelect[weaponIndex].id;
		if (weaponID == WEAPON_CROWBAR)
			return false;

		// shotgun is too inaccurate at long distances, so weapon is bad
		if (weaponID == WEAPON_SHOTGUN && distance > SquaredF(768.0f))
			return true;
	}
	else
	{
		const int weaponID = g_weaponSelect[weaponIndex].id;
		if (weaponID == WEAPON_KNIFE)
			return false;

		// shotguns is too inaccurate at long distances, so weapon is bad
		if ((weaponID == WEAPON_M3 || weaponID == WEAPON_XM1014) && distance > SquaredF(768.0f))
			return true;

		if (!IsZombieMode())
		{
			if ((weaponID == WEAPON_SCOUT || weaponID == WEAPON_AWP || weaponID == WEAPON_G3SG1 || weaponID == WEAPON_SG550) && distance <= SquaredF(512.0f))
				return true;
		}
	}

	// check is ammo available for secondary weapon
	if (m_ammoInClip[g_weaponSelect[GetBestSecondaryWeaponCarried()].id] >= 1)
		return false;

	return false;
}

void Bot::FocusEnemy(void)
{
	m_lookAt = GetEnemyPosition();

	if (m_currentWeapon == WEAPON_KNIFE)
	{
		if (IsOnAttackDistance(m_enemy, 72.0f))
			m_wantsToFire = true;
		else
			m_wantsToFire = false;
	} // when we start firing stop checks
	else if (!m_wantsToFire && GetShootingConeDeviation(GetEntity(), &m_lookAt) >= 0.80f)
		m_wantsToFire = true;
}

void Bot::CombatFight(void)
{
	// anti crash
	if (!FNullEnt(m_enemy))
	{
		// our enemy can change teams in fun modes
		if (m_team == GetTeam(m_enemy))
		{
			SetEnemy(nullptr);
			return;
		}

		// our last enemy can change teams in fun modes
		if (m_team == GetTeam(m_lastEnemy))
		{
			SetLastEnemy(m_enemy);
			return;
		}
	}
	else
	{
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;
		if (IsValidWaypoint(m_cachedWaypointIndex))
			ChangeWptIndex(m_cachedWaypointIndex);
		return;
	}

	if (IsValidWaypoint(m_currentWaypointIndex) && (m_moveSpeed != 0.0f || m_strafeSpeed != 0.0f) && g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CROUCH)
		pev->button |= IN_DUCK;

	if (m_isZombieBot) // zombie ai
	{
		DeleteSearchNodes();
		m_moveSpeed = pev->maxspeed;

		if (m_isSlowThink && !(pev->flags & FL_DUCKING) && CRandomInt(1, 2) == 1 && !IsOnLadder() && pev->speed >= pev->maxspeed)
		{
			const int random = CRandomInt(1, 3);
			if (random == 1)
				pev->button |= IN_JUMP;
			else if (random == 2)
				pev->button |= IN_DUCK;
		}
		else if (!m_isSlowThink)
			pev->button |= IN_ATTACK;

		m_destOrigin = m_enemyOrigin + m_enemy->v.velocity * ebot_zombie_speed_factor.GetFloat();

		if (!(pev->flags & FL_DUCKING))
			m_waypointOrigin = m_destOrigin;
	}
	else if (IsZombieMode()) // human ai
	{
		const bool NPCEnemy = !IsValidPlayer(m_enemy);
		const bool enemyIsZombie = IsZombieEntity(m_enemy);
		bool escape = true;

		if (NPCEnemy || enemyIsZombie)
		{
			if (m_currentWeapon == WEAPON_KNIFE)
			{
				if (!(::IsInViewCone(pev->origin + pev->velocity * m_frameInterval, m_enemy) && !NPCEnemy))
					escape = false;
			}

			if (m_isSlowThink && !NPCEnemy && m_enemy->v.health > 100 && ChanceOf(ebot_zp_use_grenade_percent.GetInt()) && m_enemy->v.velocity.GetLengthSquared() > SquaredF(10.0f))
			{
				if (m_skill >= 50)
				{
					if (pev->weapons & (1 << WEAPON_FBGRENADE) && (m_enemy->v.speed >= m_enemy->v.maxspeed || (pev->origin - m_enemyOrigin).GetLengthSquared() <= SquaredF(384.0f)))
						ThrowFrostNade();
					else
						ThrowFireNade();
				}
				else
				{
					if (pev->weapons & (1 << WEAPON_FBGRENADE) && CRandomInt(1, 2) == 1)
						ThrowFrostNade();
					else
						ThrowFireNade();
				}
			}

			DeleteSearchNodes();
			m_destOrigin = m_enemy->v.origin + pev->velocity * -m_frameInterval;

			if (escape)
				m_moveSpeed = -pev->maxspeed;
			else
				m_moveSpeed = pev->maxspeed;

			Vector directionOld = m_destOrigin - (pev->origin + pev->velocity * m_frameInterval);
			Vector directionNormal = directionOld.Normalize();
			Vector direction = directionNormal;
			directionNormal.z = 0.0f;
			SetStrafeSpeed(directionNormal, pev->maxspeed);

			m_moveAngles = directionOld.ToAngles();

			m_moveAngles.ClampAngles();
			m_moveAngles.x *= -1.0f; // invert for engine

			if (pev->button & IN_DUCK)
				pev->button &= ~IN_DUCK;
		}
	}
	else if (GetCurrentTask()->taskID != TASK_CAMP && GetCurrentTask()->taskID != TASK_SEEKCOVER && GetCurrentTask()->taskID != TASK_ESCAPEFROMBOMB)
	{
		if (g_gameVersion == HALFLIFE && fabsf(pev->speed) >= (pev->maxspeed * 0.54f) && !(pev->oldbuttons & IN_JUMP) && !(pev->oldbuttons & IN_DUCK))
		{
			if (m_personality == PERSONALITY_CAREFUL)
			{
				if (!FNullEnt(m_enemy) && (m_enemy->v.button & IN_ATTACK || m_enemy->v.oldbuttons & IN_ATTACK))
				{
					pev->button |= IN_DUCK;
					pev->button |= IN_JUMP;
				}
			}
			else if (m_personality == PERSONALITY_RUSHER)
			{
				if (pev->button & IN_ATTACK || pev->oldbuttons & IN_ATTACK)
				{
					pev->button |= IN_DUCK;
					pev->button |= IN_JUMP;
				}
			}
			else
			{
				pev->button |= IN_DUCK;
				pev->button |= IN_JUMP;
			}
		}

		const float distance = ((pev->origin + pev->velocity * m_frameInterval) - m_lookAt).GetLengthSquared();
		const int melee = g_gameVersion == HALFLIFE ? WEAPON_CROWBAR : WEAPON_KNIFE;
		if (m_currentWeapon == melee)
		{
			if (distance > SquaredF(128.0f))
			{
				SelectBestWeapon();
				m_destOrigin = m_enemyOrigin;
				m_moveSpeed = pev->maxspeed;
			}
			else
			{
				if (!(g_mapType & MAP_KA) && engine->RandomFloat(1.0f, pev->health) <= 20.0f)
				{
					const int seekindex = FindCoverWaypoint(999999.0f);
					if (IsValidWaypoint(seekindex))
					{
						PushTask(TASK_SEEKCOVER, TASKPRI_SEEKCOVER, seekindex, 8.0f, true);
						return;
					}
				}

				if (!HasNextPath())
				{
					const int nearest = g_waypoint->FindNearest(m_enemyOrigin, 999999.0f, -1, m_enemy);
					if (IsValidWaypoint(nearest))
						FindShortestPath(m_currentWaypointIndex, nearest);
					else
					{
						m_destOrigin = m_enemyOrigin;
						m_moveSpeed = pev->maxspeed;
					}
				}
				else
					m_moveToGoal = true;
			}

			return;
		}
		else
		{
			if (distance <= SquaredF(256.0f)) // get back!
			{
				m_moveSpeed = -pev->maxspeed;
				return;
			}
		}

		if (g_gameVersion == HALFLIFE)
		{
			if (m_currentWeapon == WEAPON_MP5_HL && distance > SquaredF(300.0f) && distance <= SquaredF(800.0f))
			{
				if (!(pev->oldbuttons & IN_ATTACK2) && !m_isSlowThink && CRandomInt(1, 3) == 1)
					pev->button |= IN_ATTACK2;
			}
			else if (m_currentWeapon == WEAPON_CROWBAR && m_personality != PERSONALITY_CAREFUL)
				pev->button |= IN_ATTACK;
		}

		DeleteSearchNodes();
		m_destOrigin = m_enemyOrigin - m_lastWallOrigin;
		m_waypointOrigin = nullvec;
		m_currentWaypointIndex = -1;

		int approach;
		if (!(m_states & STATE_SEEINGENEMY)) // if suspecting enemy stand still
			approach = 49;
		else if (g_gameVersion != HALFLIFE && (m_isReloading || m_isVIP)) // if reloading or vip back off
			approach = 29;
		else
		{
			approach = static_cast <int> (pev->health * m_agressionLevel);
			if (UsesSniper() && approach > 49)
				approach = 49;
		}

		// only take cover when bomb is not planted and enemy can see the bot or the bot is VIP
		if (approach < 30 && !g_bombPlanted && (::IsInViewCone(m_enemyOrigin, GetEntity()) || m_isVIP))
		{
			m_moveSpeed = -pev->maxspeed;
			PushTask(TASK_SEEKCOVER, TASKPRI_SEEKCOVER, -1, 0.0f, true);
		}
		else if (m_currentWeapon != melee) // if enemy cant see us, we never move
			m_moveSpeed = 0.0f;
		else if (approach >= 50 || UsesBadPrimary() || (!FNullEnt(m_enemy) && IsBehindSmokeClouds(m_enemy))) // we lost him?
			m_moveSpeed = pev->maxspeed;
		else
			m_moveSpeed = pev->maxspeed;

		if (UsesSniper())
		{
			m_fightStyle = 1;
			m_lastFightStyleCheck = engine->GetTime();
		}
		else if (UsesRifle() || UsesSubmachineGun())
		{
			if (m_lastFightStyleCheck + 0.5f < engine->GetTime())
			{
				if (ChanceOf(75))
				{
					if (distance < SquaredF(768.0f))
						m_fightStyle = 0;
					else if (distance < SquaredF(1024.0f))
					{
						if (ChanceOf(UsesSubmachineGun() ? 50 : 30))
							m_fightStyle = 0;
						else
							m_fightStyle = 1;
					}
					else
					{
						if (ChanceOf(UsesSubmachineGun() ? 80 : 93))
							m_fightStyle = 1;
						else
							m_fightStyle = 0;
					}
				}

				m_lastFightStyleCheck = engine->GetTime();
			}
		}
		else
		{
			if (m_lastFightStyleCheck + 0.5f < engine->GetTime())
			{
				if (ChanceOf(75))
				{
					if (ChanceOf(50))
						m_fightStyle = 0;
					else
						m_fightStyle = 1;
				}

				m_lastFightStyleCheck = engine->GetTime();
			}
		}

		if (m_fightStyle == 0 || ((pev->button & IN_RELOAD) || m_isReloading) || (UsesPistol() && distance < SquaredF(768.0f)) || m_currentWeapon == melee)
		{
			if (m_strafeSetTime < engine->GetTime())
			{
				// to start strafing, we have to first figure out if the target is on the left side or right side
				if (!FNullEnt(m_enemy))
					MakeVectors(m_enemy->v.v_angle);

				const Vector& dirToPoint = (pev->origin - m_enemyOrigin).Normalize2D();
				const Vector& rightSide = g_pGlobals->v_right.Normalize2D();

				if ((dirToPoint | rightSide) < 0)
					m_combatStrafeDir = 1;
				else
					m_combatStrafeDir = 0;

				if (ChanceOf(30))
					m_combatStrafeDir = (m_combatStrafeDir == 1 ? 0 : 1);

				m_strafeSetTime = engine->GetTime() + engine->RandomFloat(0.5f, 3.0f);
			}

			if (m_combatStrafeDir == 0)
			{
				if (!CheckWallOnLeft())
					m_strafeSpeed = -pev->maxspeed;
				else
				{
					m_combatStrafeDir = 1;
					m_strafeSetTime = engine->GetTime() + 1.5f;
				}
			}
			else
			{
				if (!CheckWallOnRight())
					m_strafeSpeed = pev->maxspeed;
				else
				{
					m_combatStrafeDir = 0;
					m_strafeSetTime = engine->GetTime() + 1.5f;
				}
			}

			if (m_jumpTime + 10.0f < engine->GetTime() && !IsOnLadder() && ChanceOf(m_isReloading ? 5 : 2) && pev->velocity.GetLength2D() > float(m_skill + 50) && !UsesSniper())
				pev->button |= IN_JUMP;

			if (m_moveSpeed > 0.0f && distance > SquaredF(512.0f) && m_currentWeapon != WEAPON_KNIFE)
				m_moveSpeed = 0.0f;

			if (m_currentWeapon == WEAPON_KNIFE)
				m_strafeSpeed = 0.0f;
		}
		else if (m_fightStyle == 1)
		{
			const Vector& src = pev->origin - Vector(0, 0, 18.0f);
			if (!(m_visibility & (Visibility::Head | Visibility::Body)) && IsVisible(src, m_enemy))
				m_duckTime = engine->GetTime() + (m_frameInterval * 2.0f);

			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;
			m_navTimeset = engine->GetTime();
		}

		if (m_duckTime > engine->GetTime())
		{
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;
		}

		if (m_isReloading)
		{
			m_moveSpeed = -pev->maxspeed;
			m_duckTime = engine->GetTime() - 2.0f;
		}

		if (!IsInWater() && !IsOnLadder() && (m_moveSpeed > 0.0f || m_strafeSpeed >= 0.0f))
		{
			MakeVectors(pev->v_angle);

			if (IsDeadlyDrop(pev->origin + (g_pGlobals->v_forward * m_moveSpeed * 0.2f) + (g_pGlobals->v_right * m_strafeSpeed * 0.2f) + (pev->velocity * m_frameInterval)))
			{
				m_strafeSpeed = -m_strafeSpeed;
				m_moveSpeed = -m_moveSpeed;

				pev->button &= ~IN_JUMP;
			}
		}
	}

	IgnoreCollisionShortly();
}

// this function returns returns true, if bot has a primary weapon
bool Bot::HasPrimaryWeapon(void)
{
	return (pev->weapons & WeaponBits_Primary) != 0;
}

// this function returns true, if bot has a tactical shield
bool Bot::HasShield(void)
{
	return strncmp(STRING(pev->viewmodel), "models/shield/v_shield_", 23) == 0;
}

// this function returns true, is the tactical shield is drawn
bool Bot::IsShieldDrawn(void)
{
	if (g_gameVersion == HALFLIFE)
		return false;

	if (!HasShield())
		return false;

	return pev->weaponanim == 6 || pev->weaponanim == 7;
}

// this function returns true, if enemy protected by the shield
bool Bot::IsEnemyProtectedByShield(edict_t* enemy)
{
	if (g_gameVersion == HALFLIFE)
		return false;

	if (FNullEnt(enemy))
		return false;

	if (IsShieldDrawn())
		return false;

	// check if enemy has shield and this shield is drawn
	if (strncmp(STRING(enemy->v.viewmodel), "models/shield/v_shield_", 23) == 0 && (enemy->v.weaponanim == 6 || enemy->v.weaponanim == 7))
	{
		if (::IsInViewCone(pev->origin, enemy))
			return true;
	}

	return false;
}

bool Bot::UsesSniper(void)
{
	if (g_gameVersion == HALFLIFE)
		return m_currentWeapon == WEAPON_CROSSBOW;

	return m_currentWeapon == WEAPON_AWP || m_currentWeapon == WEAPON_G3SG1 || m_currentWeapon == WEAPON_SCOUT || m_currentWeapon == WEAPON_SG550;
}

bool Bot::IsSniper(void)
{
	if (g_gameVersion == HALFLIFE)
	{
		if (pev->weapons & (1 << WEAPON_CROSSBOW))
			return true;
	}

	if (pev->weapons & (1 << WEAPON_AWP))
		return true;
	else if (pev->weapons & (1 << WEAPON_G3SG1))
		return true;
	else if (pev->weapons & (1 << WEAPON_SCOUT))
		return true;
	else if (pev->weapons & (1 << WEAPON_SG550))
		return true;

	return false;
}

bool Bot::UsesRifle(void)
{
	if (g_gameVersion == HALFLIFE)
		return m_currentWeapon == WEAPON_MP5_HL;

	WeaponSelect* selectTab = &g_weaponSelect[0];
	int count = 0;

	while (selectTab->id)
	{
		if (m_currentWeapon == selectTab->id)
			break;

		selectTab++;
		count++;
	}

	if (selectTab->id && count > 13)
		return true;

	return false;
}

bool Bot::UsesPistol(void)
{
	if (g_gameVersion == HALFLIFE)
		return m_currentWeapon == WEAPON_GLOCK || m_currentWeapon == WEAPON_PYTHON;

	WeaponSelect* selectTab = &g_weaponSelect[0];
	int count = 0;

	// loop through all the weapons until terminator is found
	while (selectTab->id)
	{
		if (m_currentWeapon == selectTab->id)
			break;

		selectTab++;
		count++;
	}

	if (selectTab->id && count < 7)
		return true;

	return false;
}

bool Bot::UsesSubmachineGun(void)
{
	if (g_gameVersion == HALFLIFE)
		return m_currentWeapon == WEAPON_EGON;

	return m_currentWeapon == WEAPON_MP5 || m_currentWeapon == WEAPON_TMP || m_currentWeapon == WEAPON_P90 || m_currentWeapon == WEAPON_MAC10 || m_currentWeapon == WEAPON_UMP45;
}

bool Bot::UsesZoomableRifle(void)
{
	if (g_gameVersion == HALFLIFE)
		return false;

	return m_currentWeapon == WEAPON_AUG || m_currentWeapon == WEAPON_SG552;
}

bool Bot::UsesBadPrimary(void)
{
	if (g_gameVersion == HALFLIFE)
		return m_currentWeapon == WEAPON_HORNETGUN;

	return m_currentWeapon == WEAPON_M3 || m_currentWeapon == WEAPON_UMP45 || m_currentWeapon == WEAPON_MAC10 || m_currentWeapon == WEAPON_TMP || m_currentWeapon == WEAPON_P90;
}

void Bot::ThrowFireNade(void)
{
	if (pev->weapons & (1 << WEAPON_HEGRENADE))
		PushTask(TASK_THROWHEGRENADE, TASKPRI_THROWGRENADE, -1, engine->RandomFloat(0.6f, 0.9f), false);
}

void Bot::ThrowFrostNade(void)
{
	if (pev->weapons & (1 << WEAPON_FBGRENADE))
		PushTask(TASK_THROWFBGRENADE, TASKPRI_THROWGRENADE, -1, engine->RandomFloat(0.6f, 0.9f), false);
}

int Bot::CheckGrenades(void)
{
	if (pev->weapons & (1 << WEAPON_HEGRENADE))
		return WEAPON_HEGRENADE;
	else if (pev->weapons & (1 << WEAPON_FBGRENADE))
		return WEAPON_FBGRENADE;
	else if (pev->weapons & (1 << WEAPON_SMGRENADE))
		return WEAPON_SMGRENADE;
	return -1;
}

void Bot::SelectKnife(void)
{
	if (g_gameVersion == HALFLIFE)
	{
		if (m_currentWeapon == WEAPON_CROWBAR)
			return;

		SelectWeaponByName("weapon_crowbar");
	}
	else
	{
		// already have
		if (m_currentWeapon == WEAPON_KNIFE)
			return;

		SelectWeaponByName("weapon_knife");
	}
}

void Bot::SelectBestWeapon(void)
{
	if (m_weaponSelectDelay >= engine->GetTime())
		return;

	WeaponSelect* selectTab = g_gameVersion == HALFLIFE ? &g_weaponSelectHL[0] : &g_weaponSelect[0];

	int selectIndex = -1;
	int chosenWeaponIndex = -1;

	while (selectTab[selectIndex].id)
	{
		if (!(pev->weapons & (1 << selectTab[selectIndex].id)))
		{
			selectIndex++;
			continue;
		}

		const int id = selectTab[selectIndex].id;

		// cannot be used in water...
		if (pev->waterlevel == 3 && g_weaponDefs[id].flags & ITEM_FLAG_NOFIREUNDERWATER)
			continue;

		if (g_gameVersion == HALFLIFE)
			chosenWeaponIndex = selectIndex;
		else
		{
			bool ammoLeft = false;

			if (id == m_currentWeapon)
			{
				// dont be fool
				if (m_isReloading)
					ammoLeft = false;
				else if (GetAmmoInClip() > 0)
					ammoLeft = true;
			}
			else if (m_ammo[g_weaponDefs[id].ammo1] > 0)
				ammoLeft = true;

			if (ammoLeft)
				chosenWeaponIndex = selectIndex;
		}

		selectIndex++;
	}

	if (chosenWeaponIndex == -1)
		chosenWeaponIndex = GetHighestWeapon();

	chosenWeaponIndex %= Const_NumWeapons + 1;
	selectIndex = chosenWeaponIndex;

	if (m_currentWeapon != selectTab[selectIndex].id)
	{
		SelectWeaponByName(selectTab[selectIndex].weaponName);
		m_isReloading = false;
		m_reloadState = ReloadState::Nothing;
		m_weaponSelectDelay = engine->GetTime() + 6.0f;
	}
}

void Bot::SelectPistol(void)
{
	if (!m_isSlowThink)
		return;

	if (m_isReloading)
		return;

	int oldWeapons = pev->weapons;

	pev->weapons &= ~WeaponBits_Primary;
	SelectBestWeapon();

	pev->weapons = oldWeapons;
}

int Bot::GetHighestWeapon(void)
{
	WeaponSelect* selectTab = g_gameVersion == HALFLIFE ? &g_weaponSelectHL[0] : &g_weaponSelect[0];

	int weapons = pev->weapons;
	int num = 0;
	int i = 0;

	// loop through all the weapons until terminator is found...
	while (selectTab->id)
	{
		// is the bot carrying this weapon?
		if (weapons & (1 << selectTab->id))
			num = i;

		i++;
		selectTab++;
	}

	return num;
}

void Bot::SelectWeaponByName(const char* name)
{
	FakeClientCommand(GetEntity(), name);
}

void Bot::SelectWeaponbyNumber(int num)
{
	FakeClientCommand(GetEntity(), g_weaponSelect[num].weaponName);
}

void Bot::CommandTeam(void)
{
	if (GetGameMode() != MODE_BASE && GetGameMode() != MODE_TDM)
		return;

	// prevent spamming
	if (m_timeTeamOrder > engine->GetTime())
		return;

	bool memberNear = false;
	bool memberExists = false;

	// search teammates seen by this bot
	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (client.ent == nullptr)
			continue;

		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == GetEntity())
			continue;

		memberExists = true;

		if (EntityIsVisible(client.origin))
		{
			memberNear = true;
			break;
		}
	}

	if (memberNear && ChanceOf(50)) // has teammates ?
	{
		if (m_personality == PERSONALITY_RUSHER)
			RadioMessage(Radio::StormTheFront);
		else if (m_personality == PERSONALITY_NORMAL)
			RadioMessage(Radio::StickTogether);
		else
			RadioMessage(Radio::Fallback);
	}
	else if (memberExists)
	{
		if (ChanceOf(25))
			RadioMessage(Radio::NeedBackup);
		else if (ChanceOf(25))
			RadioMessage(Radio::EnemySpotted);
		else if (ChanceOf(25))
			RadioMessage(Radio::TakingFire);
	}

	m_timeTeamOrder = engine->GetTime() + engine->RandomFloat(10.0f, 30.0f);
}

bool Bot::IsGroupOfEnemies(Vector location, int numEnemies, float radius)
{
	if (m_numEnemiesLeft <= 0)
		return false;

	if (location == nullvec)
		return false;

	int numPlayers = 0;

	// search the world for enemy players...
	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (client.ent == nullptr)
			continue;

		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team == m_team)
			continue;

		if ((client.origin - location).GetLengthSquared() <= SquaredF(radius))
		{
			if (numPlayers++ > numEnemies)
				return true;
		}
	}

	return false;
}

void Bot::CheckReload(void)
{
	if (m_isZombieBot)
	{
		m_reloadState = ReloadState::Nothing;
		m_isReloading = false;
		return;
	}

	// do not check for reload
	if (m_currentWeapon == g_gameVersion == HALFLIFE ? WEAPON_CROWBAR : WEAPON_KNIFE)
	{
		m_reloadState = ReloadState::Nothing;
		return;
	}

	m_isReloading = false; // update reloading status
	m_reloadCheckTime = AddTime(3.0f);

	if (m_reloadState != ReloadState::Nothing)
	{
		int weapons = pev->weapons;

		if (m_reloadState == ReloadState::Primary)
			weapons &= WeaponBits_Primary;
		else if (m_reloadState == ReloadState::Secondary)
			weapons &= WeaponBits_Secondary;

		if (weapons == 0)
		{
			m_reloadState++;

			if (m_reloadState > ReloadState::Secondary)
				m_reloadState = ReloadState::Nothing;

			return;
		}

		int weaponIndex = -1;
		const int maxClip = CheckMaxClip(weapons, &weaponIndex);

		if (m_ammoInClip[weaponIndex] < maxClip && g_weaponDefs[weaponIndex].ammo1 != -1 && g_weaponDefs[weaponIndex].ammo1 < 32 && m_ammo[g_weaponDefs[weaponIndex].ammo1] > 0)
		{
			if (m_currentWeapon != weaponIndex)
				SelectWeaponByName(g_weaponDefs[weaponIndex].className);

			pev->button &= ~IN_ATTACK;

			if ((pev->oldbuttons & IN_RELOAD) == ReloadState::Nothing)
				pev->button |= IN_RELOAD; // press reload button

			m_isReloading = true;
		}
		else
		{
			// if we have enemy don't reload next weapon
			if ((m_states & (STATE_SEEINGENEMY | STATE_HEARENEMY)) || m_seeEnemyTime + 5.0f > engine->GetTime())
			{
				m_reloadState = ReloadState::Nothing;
				return;
			}

			m_reloadState++;

			if (m_reloadState > ReloadState::Secondary)
				m_reloadState = ReloadState::Nothing;

			return;
		}
	}
}

int Bot::CheckMaxClip(int weaponId, int* weaponIndex)
{
	int maxClip = -1;
	for (int i = 1; i < Const_MaxWeapons; i++)
	{
		if (weaponId & (1 << i))
		{
			*weaponIndex = i;
			break;
		}
	}

	InternalAssert(weaponIndex);

	switch (*weaponIndex)
	{
	case WEAPON_M249:
		maxClip = 100;
		break;

	case WEAPON_P90:
		maxClip = 50;
		break;

	case WEAPON_GALIL:
		maxClip = 35;
		break;

	case WEAPON_ELITE:
	case WEAPON_MP5:
	case WEAPON_TMP:
	case WEAPON_MAC10:
	case WEAPON_M4A1:
	case WEAPON_AK47:
	case WEAPON_SG552:
	case WEAPON_AUG:
	case WEAPON_SG550:
		maxClip = 30;
		break;

	case WEAPON_UMP45:
	case WEAPON_FAMAS:
		maxClip = 25;
		break;

	case WEAPON_GLOCK18:
	case WEAPON_FN57:
	case WEAPON_G3SG1:
		maxClip = 20;
		break;

	case WEAPON_P228:
		maxClip = 13;
		break;

	case WEAPON_USP:
		maxClip = 12;
		break;

	case WEAPON_AWP:
	case WEAPON_SCOUT:
		maxClip = 10;
		break;

	case WEAPON_M3:
		maxClip = 8;
		break;

	case WEAPON_DEAGLE:
	case WEAPON_XM1014:
		maxClip = 7;
		break;
	}

	return maxClip;
}