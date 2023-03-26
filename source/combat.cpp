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
ConVar ebot_zp_escape_distance("ebot_zm_escape_distance", "320");
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

		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team == m_team)
			continue;

		if ((client.origin - origin).GetLengthSquared() <= SquaredF(radius))
			count++;
	}

	return count;
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
	m_visibility = 0;
	m_enemyOrigin = nullvec;

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
			if (IsEnemyViewable(m_enemy, true) || IsShootableThruObstacle(m_enemy))
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
		auto origin = GetEntityOrigin(m_moveTargetEntity);
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

		if (IsEnemyViewable(entity, true, true))
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
	if (!FNullEnt(targetEntity) && !IsEnemyViewable(targetEntity, true, true))
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
			m_enemyOrigin = GetEntityOrigin(m_enemy);
			m_visibility = VISIBILITY_BODY;
			return true;
		}
	}

	if (!FNullEnt(targetEntity))
	{
		if (m_isZombieBot || ebot_knifemode.GetBool())
		{
			bool moveTotarget = true;
			int movePoint = 0;

			int srcIndex = GetEntityWaypoint(GetEntity());
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
			m_actualReactionTime = 0.0f;
			SetLastEnemy(targetEntity);

			return true;
		}

		if (m_seeEnemyTime + 3.0f < engine->GetTime() && (m_isBomber || HasHostage() || !FNullEnt(m_targetEntity)))
			RadioMessage(Radio_EnemySpotted);

		m_targetEntity = nullptr;

		if (ChanceOf(m_skill))
			m_enemySurpriseTime = engine->GetTime() + (m_actualReactionTime * 0.33333333333f);
		else
			m_enemySurpriseTime = engine->GetTime() + m_actualReactionTime;

		m_actualReactionTime = 0.0f;

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
			m_reloadState = RSTATE_PRIMARY;
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

Vector Bot::GetAimPosition(void)
{
	// npc/entity
	if (!IsValidPlayer(m_enemy))
		return m_lastEnemyOrigin = m_enemyOrigin;

	// get enemy position initially
	Vector targetOrigin = m_enemy->v.origin;

	float distance = (m_enemyOrigin - pev->origin).GetLengthSquared();

	// do not aim at head, at long distance (only if not using sniper weapon)
	if ((m_visibility & VISIBILITY_BODY) && !UsesSniper() && !UsesPistol() && (distance > (m_difficulty == 4 ? SquaredF(2400.0f) : SquaredF(1200.0f))))
		m_visibility &= ~VISIBILITY_HEAD;

	if ((m_states & STATE_HEARENEMY) && !(m_states & STATE_SEEINGENEMY))
		targetOrigin = m_lastEnemyOrigin;
	else
	{
		// now take in account different parts of enemy body
		if (m_visibility & (VISIBILITY_HEAD | VISIBILITY_BODY)) // visible head & body
		{
			// now check is our skill match to aim at head, else aim at enemy body
			if (ChanceOf(m_skill) || UsesPistol())
				targetOrigin = targetOrigin + m_enemy->v.view_ofs + Vector(0.0f, 0.0f, GetZOffset(distance));
			else
				targetOrigin = targetOrigin + Vector(0.0f, 0.0f, GetZOffset(distance));
		}
		else if (m_visibility & VISIBILITY_BODY) // visible only body
			targetOrigin = targetOrigin + Vector(0.0f, 0.0f, GetZOffset(distance));
		else if (m_visibility & VISIBILITY_OTHER) // random part of body is visible
			targetOrigin = m_enemyOrigin;
		else if (m_visibility & VISIBILITY_HEAD) // visible only head
			targetOrigin = targetOrigin + m_enemy->v.view_ofs + Vector(0.0f, 0.0f, GetZOffset(distance));
		else // something goes wrong, use last enemy origin
			targetOrigin = m_lastEnemyOrigin;

		m_lastEnemyOrigin = targetOrigin;
	}

	return m_enemyOrigin = m_lastEnemyOrigin = targetOrigin;
}

float Bot::GetZOffset(float distance)
{
	if (m_difficulty < 2)
		return -6.0f;

	bool sniper = UsesSniper();
	bool pistol = UsesPistol();
	bool rifle = UsesRifle();

	bool zoomableRifle = UsesZoomableRifle();
	bool submachine = UsesSubmachineGun();
	bool shotgun = (m_currentWeapon == WEAPON_XM1014 || m_currentWeapon == WEAPON_M3);
	bool m249 = m_currentWeapon == WEAPON_M249;

	const float BurstDistance = SquaredF(300.0f);
	const float DoubleBurstDistance = BurstDistance * 2.0f;

	float result = 6.0f;

	if (distance < SquaredF(2800.0f) && distance > DoubleBurstDistance)
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
		else if (zoomableRifle) result = -5.0f;
		else if (pistol) result = 4.5f;
		else if (submachine) result = -4.5f;
		else if (rifle) result = -4.5f;
		else if (m249) result = -6.0f;
		else if (shotgun) result = -5.0f;
	}

	return -result;
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

			int entityIndex = ENTINDEX(tr.pHit);
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

		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == GetEntity())
			continue;

		Vector origin = client.ent->v.origin;
		float friendDistance = (origin - pev->origin).GetLengthSquared();

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

bool Bot::DoFirePause(float distance)//, FireDelay *fireDelay)
{
	if (m_firePause > engine->GetTime())
		return true;

	if ((m_aimFlags & AIM_ENEMY) && m_enemyOrigin != nullvec)
	{
		if (IsEnemyProtectedByShield(m_enemy) && GetShootingConeDeviation(GetEntity(), &m_enemyOrigin) > 0.92f)
			return true;
	}

	float angle = (fabsf(pev->punchangle.y) + fabsf(pev->punchangle.x)) * (Math::MATH_PI * 0.00277777777f);

	// check if we need to compensate recoil
	if (tanf(angle) * (distance + (distance * 0.25f)) > g_skillTab[m_skill / 20].recoilAmount)
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

		if (pev->speed >= pev->maxspeed && GetGameMode() != MODE_ZP)
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
	// if using grenade stop this
	if (m_isUsingGrenade)
	{
		m_shootTime = engine->GetTime() + 0.25f;
		return;
	}

	float distance = (m_lookAt - pev->origin).GetLengthSquared(); // how far away is the enemy?

	// or if friend in line of fire, stop this too but do not update shoot time
	if (!FNullEnt(m_enemy) && IsFriendInLineOfFire(distance))
		return;

	int melee = g_gameVersion == HALFLIFE ? WEAPON_CROWBAR : WEAPON_KNIFE;

	FireDelay* delay = &g_fireDelay[0];
	WeaponSelect* selectTab = g_gameVersion == HALFLIFE ? &g_weaponSelectHL[0] : &g_weaponSelect[0];

	edict_t* enemy = m_enemy;

	int selectId = melee, selectIndex = 0, chosenWeaponIndex = 0;
	int weapons = pev->weapons;

	if (m_isZombieBot || ebot_knifemode.GetBool())
		goto WeaponSelectEnd;
	else if (!FNullEnt(enemy) && ChanceOf(m_skill) && !IsZombieEntity(enemy) && IsOnAttackDistance(enemy, 128.0f) && (enemy->v.health <= 30 || pev->health > enemy->v.health) && !IsOnLadder() && !IsGroupOfEnemies(pev->origin))
		goto WeaponSelectEnd;

	// loop through all the weapons until terminator is found...
	while (selectTab[selectIndex].id)
	{
		// is the bot carrying this weapon?
		if (weapons & (1 << selectTab[selectIndex].id))
		{
			// is enough ammo available to fire AND check is better to use pistol in our current situation...
			if (g_gameVersion == HALFLIFE)
			{
				if (selectIndex == WEAPON_SNARK || selectIndex == WEAPON_GAUSS ||selectIndex == WEAPON_EGON || (selectIndex == WEAPON_HANDGRENADE && distance > SquaredF(384.0f) && distance <= SquaredF(768.0f)) || (selectIndex == WEAPON_RPG && distance > SquaredF(320.0f)) || (selectIndex == WEAPON_CROSSBOW && distance > SquaredF(320.0f)))
					chosenWeaponIndex = selectIndex;
				else if (selectIndex != WEAPON_HANDGRENADE && selectIndex != WEAPON_RPG  && selectIndex != WEAPON_CROSSBOW && (m_ammoInClip[selectTab[selectIndex].id] > 0) && !IsWeaponBadInDistance(selectIndex, distance))
						chosenWeaponIndex = selectIndex;

			}
			else if ((m_ammoInClip[selectTab[selectIndex].id] > 0) && !IsWeaponBadInDistance(selectIndex, distance))
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
			int id = selectTab[selectIndex].id;

			// is the bot carrying this weapon?
			if (weapons & (1 << id))
			{
				if (g_weaponDefs[id].ammo1 != -1 && m_ammo[g_weaponDefs[id].ammo1] >= selectTab[selectIndex].minPrimaryAmmo)
				{
					// available ammo found, reload weapon
					if (m_reloadState == RSTATE_NONE || m_reloadCheckTime > engine->GetTime() || GetCurrentTask()->taskID != TASK_ESCAPEFROMBOMB)
					{
						m_isReloading = true;
						m_reloadState = RSTATE_PRIMARY;
						m_reloadCheckTime = engine->GetTime();
						m_fearLevel = 1.0f;
						RadioMessage(Radio_NeedBackup);
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
		m_reloadState = RSTATE_NONE;
		m_reloadCheckTime = engine->GetTime() + 5.0f;
	}

	if (IsZombieMode() && m_currentWeapon == melee && selectId != melee && !m_isZombieBot)
	{
		m_reloadState = RSTATE_PRIMARY;
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

		if (HasShield() && m_shieldCheckTime < engine->GetTime() && GetCurrentTask()->taskID != TASK_CAMP) // better shield gun usage
		{
			if ((distance > SquaredF(768.0f)) && !IsShieldDrawn())
				pev->button |= IN_ATTACK2; // draw the shield
			else if (IsShieldDrawn() || (!FNullEnt(enemy) && (enemy->v.button & IN_RELOAD)))
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
	if (g_gameVersion == HALFLIFE || distance < 256.0f || m_blindTime > engine->GetTime())
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
		const float baseDelay = delay[chosenWeaponIndex].primaryBaseDelay;
		const float minDelay = delay[chosenWeaponIndex].primaryMinDelay[abs((m_skill / engine->RandomInt(15, 20)) - 5)];
		const float maxDelay = delay[chosenWeaponIndex].primaryMaxDelay[abs((m_skill / engine->RandomInt(20, 30)) - 5)];

		if (DoFirePause(distance))//, &delay[chosenWeaponIndex]))
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
			delayTime = baseDelay + engine->RandomFloat(minDelay, maxDelay);
			m_zoomCheckTime = engine->GetTime();
		}

		if (!FNullEnt(enemy) && distance >= SquaredF(1200.0f))
		{
			if (m_visibility & (VISIBILITY_HEAD | VISIBILITY_BODY))
				delayTime -= (delayTime == 0.0f) ? 0.0f : 0.02f;
			else if (m_visibility & VISIBILITY_HEAD)
			{
				if (distance >= SquaredF(2400.0f))
					delayTime += (delayTime == 0.0f) ? 0.15f : 0.10f;
				else
					delayTime += (delayTime == 0.0f) ? 0.10f : 0.05f;
			}
			else if (m_visibility & VISIBILITY_BODY)
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
	edict_t* entity = nullptr;
	float distance = FLT_MAX;
	if (!FNullEnt(m_enemy))
	{
		entity = m_enemy;
		distance = (pev->origin - m_enemyOrigin).GetLengthSquared();
	}

	if (!FNullEnt(m_breakableEntity))
	{
		if (m_breakable == nullvec)
			m_breakable = GetEntityOrigin(m_breakableEntity);

		if ((pev->origin - m_breakable).GetLengthSquared() < distance)
		{
			entity = m_breakableEntity;
			distance = (pev->origin - m_breakable).GetLengthSquared();
		}
	}

	if (FNullEnt(entity))
		return false;

	float kad1 = 64.0f;
	float kad2 = pev->speed * 0.26;

	if (attackDistance != 0.0f)
		kad1 = attackDistance;

	int kaMode = 0;
	if (IsOnAttackDistance(entity, kad1))
		kaMode = 1;
	if (IsOnAttackDistance(entity, kad2))
		kaMode += 2;

	if (kaMode > 0)
	{
		Vector entityOrigin = GetEntityOrigin(entity);
		float distanceSkipZ = (pev->origin - entityOrigin).GetLengthSquared2D();

		if (pev->origin.z > entityOrigin.z && distanceSkipZ < SquaredF(64.0f))
		{
			pev->button |= IN_DUCK;
			m_campButtons |= IN_DUCK;
			pev->button &= ~IN_JUMP;
		}
		else
		{
			pev->button &= ~IN_DUCK;
			m_campButtons &= ~IN_DUCK;

			if (pev->origin.z + 150.0f < entityOrigin.z && distanceSkipZ < SquaredF(300.0f))
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
			else if (engine->RandomInt(1, 10) < 3 || HasShield())
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
		int weaponID = g_weaponSelect[weaponIndex].id;
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
	m_lookAt = GetAimPosition();

	if (m_enemySurpriseTime > engine->GetTime())
		return;

	float distance = (m_lookAt - EyePosition()).GetLengthSquared();  // how far away is the enemy scum?

	if (distance < SquaredF(128.0f))
	{
		if (m_currentWeapon == WEAPON_KNIFE)
		{
			if (IsOnAttackDistance(m_enemy, 64.0f))
				m_wantsToFire = true;
		}
		else
			m_wantsToFire = true;
	}
	else
	{
		if (m_currentWeapon == WEAPON_KNIFE)
			m_wantsToFire = true;
		else
		{
			float dot = GetShootingConeDeviation(GetEntity(), &m_enemyOrigin);

			if (dot < 0.90f)
				m_wantsToFire = false;
			else
			{
				float enemyDot = GetShootingConeDeviation(m_enemy, &pev->origin);

				// enemy faces bot?
				if (enemyDot >= 0.90f)
					m_wantsToFire = true;
				else
				{
					if (dot > 0.99f)
						m_wantsToFire = true;
					else
						m_wantsToFire = false;
				}
			}
		}
	}
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
		return;

	if (IsValidWaypoint(m_currentWaypointIndex) && (m_moveSpeed != 0.0f || m_strafeSpeed != 0.0f) && g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CROUCH)
		pev->button |= IN_DUCK;

	if (m_isZombieBot) // zombie ai
	{
		DeleteSearchNodes();
		m_moveSpeed = pev->maxspeed;

		if (m_isSlowThink && !(pev->flags & FL_DUCKING) && engine->RandomInt(1, 2) == 1 && !IsOnLadder() && pev->speed >= pev->maxspeed)
		{
			int random = engine->RandomInt(1, 3);
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
		Vector tempDestOrigin = nullvec;
		float tempMoveSpeed = -1.0f;

		const bool NPCEnemy = !IsValidPlayer(m_enemy);
		const bool enemyIsZombie = IsZombieEntity(m_enemy);

		const Vector enemyVel = m_enemy->v.velocity;
		float baseDistance = ebot_zp_escape_distance.GetFloat() + fabsf(m_enemy->v.speed);

		Vector myVec = pev->origin + pev->velocity * m_frameInterval;

		if (NPCEnemy || enemyIsZombie)
		{
			if (m_currentWeapon == WEAPON_KNIFE)
			{
				if (!(::IsInViewCone(myVec, m_enemy) && !NPCEnemy))
					baseDistance = -1.0f;
			}

			const Vector destOrigin = m_enemyOrigin + enemyVel * m_frameInterval;
			const float distance = (myVec - destOrigin).GetLengthSquared();
			if (m_isSlowThink && distance <= SquaredF(768.0f) && m_enemy->v.health > 100 && ChanceOf(ebot_zp_use_grenade_percent.GetInt()) && m_enemy->v.velocity.GetLengthSquared() > SquaredF(10.0f))
			{
				if (m_skill >= 50)
				{
					if (pev->weapons & (1 << WEAPON_FBGRENADE) && (m_enemy->v.speed >= m_enemy->v.maxspeed || distance <= SquaredF(384.0f)))
						ThrowFrostNade();
					else
						ThrowFireNade();
				}
				else
				{
					if (pev->weapons & (1 << WEAPON_FBGRENADE) && engine->RandomInt(1, 2) == 1)
						ThrowFrostNade();
					else
						ThrowFireNade();
				}
			}

			if (baseDistance > 0.0f && distance <= SquaredF(baseDistance))
			{
				DeleteSearchNodes();
				m_destOrigin = destOrigin;
				m_moveSpeed = -pev->maxspeed;

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

		if (m_isReloading || (m_isBomber && (engine->RandomFloat(1.0f, pev->health) <= 40.0f || !HasPrimaryWeapon())) || m_isVIP)
		{
			const int seekindex = FindCoverWaypoint(99999.0f);
			if (IsValidWaypoint(seekindex))
				PushTask(TASK_SEEKCOVER, TASKPRI_SEEKCOVER, seekindex, 4.0f, true);
			return;
		}

		float distance = ((pev->origin + pev->velocity * m_frameInterval) - m_lookAt).GetLengthSquared(); // how far away is the enemy scum?
		if (IsWeaponBadInDistance(m_currentWeapon, distance))
		{
			const int seekindex = FindCoverWaypoint(99999.0f);
			if (IsValidWaypoint(seekindex))
				PushTask(TASK_SEEKCOVER, TASKPRI_SEEKCOVER, seekindex, 12.0f, true);
			return;
		}

		if (FNullEnt(m_enemy) && IsSniper())
		{
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;
			SelectBestWeapon();

			if (UsesSniper() && m_zoomCheckTime < engine->GetTime())
			{
				if (distance > SquaredF(1500.0f) && pev->fov >= 40.0f)
					pev->button |= IN_ATTACK2;
				else if (distance > SquaredF(150.0f) && pev->fov >= 90.0f)
					pev->button |= IN_ATTACK2;
				else if (distance <= SquaredF(150.0f) && pev->fov < 90.0f)
					pev->button |= IN_ATTACK2;
				m_zoomCheckTime = engine->GetTime();
			}

			return;
		}

		const int melee = g_gameVersion == HALFLIFE ? WEAPON_CROWBAR : WEAPON_KNIFE;
		if (m_currentWeapon == melee && !FNullEnt(m_enemy))
		{
			if (distance > SquaredF(128.0f))
			{
				if (!(g_mapType & MAP_KA) && engine->RandomFloat(1.0f, pev->health) <= 20.0f)
				{
					const int seekindex = FindCoverWaypoint(99999.0f);
					if (IsValidWaypoint(seekindex))
						PushTask(TASK_SEEKCOVER, TASKPRI_SEEKCOVER, seekindex, 8.0f, true);
				}

				if (!HasNextPath())
				{
					const int nearest = g_waypoint->FindNearest(m_enemyOrigin, 99999999.0f, -1, m_enemy);
					if (IsValidWaypoint(nearest))
						FindShortestPath(m_currentWaypointIndex, nearest);
				}

				return;
			}
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
				if (!(pev->oldbuttons & IN_ATTACK2) && !m_isSlowThink && engine->RandomInt(1, 3) == 1)
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
		if (m_currentWeapon == melee) // knife?
			approach = 100;
		else if (!(m_states & STATE_SEEINGENEMY)) // if suspecting enemy stand still
			approach = 49;
		else if (m_isReloading || m_isVIP) // if reloading or vip back off
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
			GetCurrentTask()->taskID = TASK_FIGHTENEMY;
			GetCurrentTask()->canContinue = true;
			GetCurrentTask()->desire = TASKPRI_FIGHTENEMY + 1.0f;
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
			if ((m_visibility & (VISIBILITY_HEAD | VISIBILITY_BODY)) && (FNullEnt(m_enemy) || (m_enemy->v.weapons != WEAPON_KNIFE && IsVisible(src, m_enemy))))
				m_duckTime = engine->GetTime() + m_frameInterval;

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
		return;
	}

	// already have
	if (m_currentWeapon == WEAPON_KNIFE)
		return;

	auto task = GetCurrentTask()->taskID;
	if (task == TASK_THROWFBGRENADE)
		return;

	if (task == TASK_THROWFLARE)
		return;

	if (task == TASK_THROWHEGRENADE)
		return;

	if (task == TASK_THROWSMGRENADE)
		return;

	if (m_isZombieBot)
	{
		SelectWeaponByName("weapon_knife");
		return;
	}

	if (task == TASK_BLINDED)
		return;

	if (task == TASK_CAMP)
		return;

	if (task == TASK_DESTROYBREAKABLE)
		return;

	if (task == TASK_PAUSE)
		return;

	if (task == TASK_FOLLOWUSER)
		return;

	if (task == TASK_HUNTENEMY)
		return;

	if (task == TASK_GOINGFORCAMP)
	{
		if (m_personality == PERSONALITY_CAREFUL)
			return;
		else if (m_personality == PERSONALITY_NORMAL && m_skill > 50)
			return;
	}

	SelectWeaponByName("weapon_knife");
}

void Bot::SelectBestWeapon(void)
{
	if (!m_isSlowThink)
		return;

	// never change weapon while reloading if theres no enemy
	if (FNullEnt(m_enemy) && m_isReloading)
		return;

	if (g_gameVersion != HALFLIFE)
	{
		if (ebot_sb_mode.GetBool())
		{
			if (m_currentWeapon != WEAPON_HEGRENADE && m_currentWeapon != WEAPON_FBGRENADE && m_currentWeapon != WEAPON_SMGRENADE)
			{
				if (pev->weapons & (1 << WEAPON_HEGRENADE))
					SelectWeaponByName("weapon_hegrenade");
				else if (pev->weapons & (1 << WEAPON_FBGRENADE))
					SelectWeaponByName("weapon_flashbang");
				else if (pev->weapons & (1 << WEAPON_SMGRENADE))
					SelectWeaponByName("weapon_smokegrenade");
			}

			return;
		}

		if (!IsZombieMode())
		{
			if (m_numEnemiesLeft <= 0)
			{
				if (m_currentWeapon != WEAPON_KNIFE)
					SelectWeaponByName("weapon_knife");

				m_weaponSelectDelay = engine->GetTime() + 3.0f;
				return;
			}

			if (!FNullEnt(m_enemy) && GetCurrentTask()->taskID == TASK_FIGHTENEMY && (pev->origin - m_enemyOrigin).GetLengthSquared() <= SquaredF(96.0f))
			{
				if (m_currentWeapon != WEAPON_KNIFE)
					SelectWeaponByName("weapon_knife");

				m_weaponSelectDelay = engine->GetTime() + 3.0f;
				return;
			}
		}
	}

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

		int id = selectTab[selectIndex].id;

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
		m_reloadState = RSTATE_NONE;
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
			RadioMessage(Radio_StormTheFront);
		else if (m_personality == PERSONALITY_NORMAL)
			RadioMessage(Radio_StickTogether);
		else
			RadioMessage(Radio_Fallback);
	}
	else if (memberExists)
	{
		if (ChanceOf(25))
			RadioMessage(Radio_NeedBackup);
		else if (ChanceOf(25))
			RadioMessage(Radio_EnemySpotted);
		else if (ChanceOf(25))
			RadioMessage(Radio_TakingFire);
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
	// check the reload state
	if (GetCurrentTask()->taskID == TASK_ESCAPEFROMBOMB || GetCurrentTask()->taskID == TASK_PLANTBOMB || GetCurrentTask()->taskID == TASK_DEFUSEBOMB || GetCurrentTask()->taskID == TASK_PICKUPITEM || GetCurrentTask()->taskID == TASK_THROWFBGRENADE || GetCurrentTask()->taskID == TASK_THROWSMGRENADE || m_isUsingGrenade)
	{
		m_reloadState = RSTATE_NONE;
		return;
	}

	m_isReloading = false;    // update reloading status
	m_reloadCheckTime = engine->GetTime() + 5.0f;

	if (m_reloadState != RSTATE_NONE)
	{
		int weapons = pev->weapons;

		if (m_reloadState == RSTATE_PRIMARY)
			weapons &= WeaponBits_Primary;
		else if (m_reloadState == RSTATE_SECONDARY)
			weapons &= WeaponBits_Secondary;

		if (weapons == 0)
		{
			m_reloadState++;

			if (m_reloadState > RSTATE_SECONDARY)
				m_reloadState = RSTATE_NONE;

			return;
		}

		int weaponIndex = -1;
		int maxClip = CheckMaxClip(weapons, &weaponIndex);

		if (m_ammoInClip[weaponIndex] < maxClip * 0.8f && g_weaponDefs[weaponIndex].ammo1 != -1 && g_weaponDefs[weaponIndex].ammo1 < 32 && m_ammo[g_weaponDefs[weaponIndex].ammo1] > 0)
		{
			if (m_currentWeapon != weaponIndex)
				SelectWeaponByName(g_weaponDefs[weaponIndex].className);

			pev->button &= ~IN_ATTACK;

			if ((pev->oldbuttons & IN_RELOAD) == RSTATE_NONE)
				pev->button |= IN_RELOAD; // press reload button

			m_isReloading = true;
		}
		else
		{
			// if we have enemy don't reload next weapon
			if ((m_states & (STATE_SEEINGENEMY | STATE_HEARENEMY)) || m_seeEnemyTime + 5.0f > engine->GetTime())
			{
				m_reloadState = RSTATE_NONE;
				return;
			}
			m_reloadState++;

			if (m_reloadState > RSTATE_SECONDARY)
				m_reloadState = RSTATE_NONE;

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