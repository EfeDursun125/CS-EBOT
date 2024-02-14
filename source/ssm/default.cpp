#include <core.h>

void Bot::DefaultStart(void)
{
	if (m_isZombieBot)
		SelectWeaponByName("weapon_knife");

	m_isStuck = false;
	m_campIndex = -1;
}

void Bot::DefaultUpdate(void)
{
	if (m_isZombieBot)
	{
		// nearest enemy never resets to nullptr, so bot always know where are alive humans
		if (IsAlive(m_nearestEnemy) && GetTeam(m_nearestEnemy) != m_team)
		{
			if (m_hasEnemiesNear && IsEnemyReachable())
			{
				FindFriendsAndEnemiens();
				m_currentWaypointIndex = -1;
				m_navNode.Clear();
				MoveTo(m_enemyOrigin);
				LookAt(m_enemyOrigin);

				if (crandomint(1, 3) == 1)
					pev->buttons |= IN_ATTACK2;
				else if (crandomint(1, 3) == 1)
					pev->buttons |= IN_ATTACK;

				CheckStuck(pev->maxspeed + cabsf(m_strafeSpeed));
				return;
			}
			else if (!m_navNode.IsEmpty())
			{
				if (m_personality != Personality::Careful && m_isSlowThink && crandomint(1, 3) == 1 && (g_waypoint->m_paths[m_navNode.Last()].origin - m_nearestEnemy->v.origin).GetLengthSquared() > squaredf(128.0f))
					m_navNode.Clear();
				else
					FollowPath();
			}
			else
			{
				int index = g_waypoint->FindNearest(m_nearestEnemy->v.origin, 256.0f, -1, m_nearestEnemy);
				if (IsValidWaypoint(index))
					FindPath(m_currentWaypointIndex, index);
				else
				{
					index = g_waypoint->FindNearest(m_nearestEnemy->v.origin);
					if (IsValidWaypoint(index))
						FindPath(m_currentWaypointIndex, index);
					else
					{
						m_moveSpeed = pev->maxspeed;
						pev->speed = pev->maxspeed;
						m_strafeSpeed = 0.0f;
					}
				}
			}
		}
		else
		{
			if (!m_navNode.IsEmpty())
				FollowPath();
			else
			{
				int ref = FindGoal();
				FindPath(m_currentWaypointIndex, ref);
			}
		}

		if (m_isSlowThink)
		{
			FindEnemyEntities();

			if (m_hasEnemiesNear && !FNullEnt(m_nearestEnemy))
				CheckGrenadeThrow(m_nearestEnemy);
		}
		else
			FindFriendsAndEnemiens();

		UpdateLooking();
		return;
	}

	// this also stops finding path too early that causes path start at last death origin
	if (!m_buyingFinished)
	{
		m_navNode.Clear();
		return;
	}

	UpdateLooking();

	if (m_isSlowThink)
	{
		// revert the zoom to normal
		if (UsesSniper() && pev->fov != 90.0f)
			pev->buttons |= IN_ATTACK2;

		FindEnemyEntities();

		if (IsZombieMode())
		{
			if (IsValidWaypoint(m_zhCampPointIndex))
			{
				m_campIndex = m_zhCampPointIndex;
				if (SetProcess(Process::Camp, "i will camp until game ends", true, engine->GetTime() + 9999999.0f))
				{
					m_navNode.Clear();
					return;
				}
			}
			else if (!g_waypoint->m_zmHmPoints.IsEmpty())
			{
				m_campIndex = g_waypoint->m_zmHmPoints.Random();
				if (SetProcess(Process::Camp, "i will camp until game ends", true, engine->GetTime() + 9999999.0f))
				{
					m_navNode.Clear();
					return;
				}
			}
		}
		else
			CheckRadioCommands();
	}
	else
	{
		if (m_itemCheckTime < engine->GetTime())
		{
			FindItem();
			m_itemCheckTime = engine->GetTime() + (g_gameVersion & Game::HalfLife ? 1.0f : crandomfloat(1.0f, 2.0f));

			if (GetEntityOrigin(m_pickupItem) != nullvec && SetProcess(Process::Pickup, "i see good stuff to pick it up", true, engine->GetTime() + 20.0f))
				return;
		}
		else
			FindFriendsAndEnemiens();
	}

	if (IsZombieMode())
	{
		if (m_hasEnemiesNear && IsEnemyReachable())
		{
			if (m_navNode.IsEmpty())
			{
				FindEscapePath(m_currentWaypointIndex, m_nearestEnemy->v.origin);
				MoveOut(m_enemyOrigin);
				CheckStuck(pev->maxspeed);
			}
			else if (((pev->origin - g_waypoint->m_paths[m_navNode.First()].origin).GetLengthSquared() > (m_nearestEnemy->v.origin - g_waypoint->m_paths[m_navNode.First()].origin).GetLengthSquared() || 
				(pev->origin - g_waypoint->m_paths[m_navNode.Next()].origin).GetLengthSquared() > (m_nearestEnemy->v.origin - g_waypoint->m_paths[m_navNode.Next()].origin).GetLengthSquared()) && ::IsInViewCone(pev->origin, m_nearestEnemy))
			{
				m_currentWaypointIndex = -1;
				FindEscapePath(m_currentWaypointIndex, m_nearestEnemy->v.origin);
				MoveOut(m_enemyOrigin);
				CheckStuck(pev->maxspeed);
			}
			else
				FollowPath();
			return;
		}
		else if (!m_navNode.IsEmpty())
			FollowPath();
		else
		{
			int ref = FindGoal();
			FindPath(m_currentWaypointIndex, ref);
		}
	}
	else
	{
		if (m_hasEnemiesNear || m_hasEntitiesNear)
		{
			if (m_currentWeapon == Weapon::M3 || m_currentWeapon == Weapon::Xm1014)
			{
				const float time = engine->GetTime();
				if (((m_hasEnemiesNear && m_enemySeeTime + 2.0f < time) || (m_hasEntitiesNear && m_entitySeeTime + 2.0f < time)) && SetProcess(Process::Attack, "i found a target", false, time + 999999.0f))
					return;
			}
			else if (SetProcess(Process::Attack, "i found a target", false, engine->GetTime() + 999999.0f))
				return;
		}

		if (m_isBomber && m_waypoint.flags & WAYPOINT_GOAL && SetProcess(Process::Plant, "trying to plant the bomb.", false, engine->GetTime() + 12.0f))
			return;

		if (!m_navNode.IsEmpty())
			FollowPath();
		else
		{
			int ref = FindGoal();
			FindPath(m_currentWaypointIndex, ref);
		}
	}
}

void Bot::DefaultEnd(void)
{
	m_isStuck = false;
}

bool Bot::DefaultReq(void)
{
	return true;
}