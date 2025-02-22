#include "../../include/core.h"

void Bot::DefaultStart(void)
{
	m_isEnemyReachable = false;
}

void Bot::DefaultUpdate(void)
{
	if (m_isZombieBot)
	{
		// nearest enemy never resets to nullptr, so bot always know where are alive humans
		if (IsAlive(m_moveTarget) && GetTeam(m_moveTarget) != m_team)
		{
			if (m_hasEnemiesNear && m_isEnemyReachable && CheckVisibility(m_nearestEnemy))
			{
				MoveTo(m_enemyOrigin + m_nearestEnemy->v.velocity * m_frameInterval);
				LookAt(m_enemyOrigin, m_nearestEnemy->v.velocity);

				if (m_isSlowThink)
				{
					FindEnemyEntities();
					FindFriendsAndEnemiens();
					CheckReachable();
					FindWaypoint();
				}
				else
					KnifeAttack();

				return;
			}
			else if (!m_navNode.IsEmpty())
			{
				if (m_isSlowThink && m_navNode.HasNext() && (g_waypoint->m_paths[m_navNode.Last()].origin - pev->origin).GetLengthSquared() < (g_waypoint->m_paths[m_navNode.Last()].origin - m_moveTarget->v.origin).GetLengthSquared())
				{
					KnifeAttack();
					int index = g_waypoint->FindNearest(m_moveTarget->v.origin, 768.0f, -1, m_moveTarget);
					if (IsValidWaypoint(index))
					{
						m_currentGoalIndex = index;
						FindPath(m_currentWaypointIndex, index);
					}
				}
				else
					FollowPath();
			}
			else
			{
				int index = g_waypoint->FindNearest(m_moveTarget->v.origin, 2048.0f, -1, m_moveTarget);
				if (IsValidWaypoint(index))
				{
					m_currentGoalIndex = index;
					FindPath(m_currentWaypointIndex, index);
				}
				else
				{
					index = g_waypoint->FindNearestInCircle(m_moveTarget->v.origin);
					if (IsValidWaypoint(index))
					{
						m_currentGoalIndex = index;
						FindPath(m_currentWaypointIndex, index);
					}
					else
					{
						index = crandomint(1, g_numWaypoints - 2);
						m_currentGoalIndex = index;
						FindPath(m_currentWaypointIndex, index);
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
				KnifeAttack();
				int ref = FindGoalZombie();
				FindPath(m_currentWaypointIndex, ref);
			}
		}

		if (m_isSlowThink)
		{
			m_zhCampPointIndex = -1;

			FindEnemyEntities();
			FindFriendsAndEnemiens();

			CheckReachable();

			if (m_hasEnemiesNear && !FNullEnt(m_nearestEnemy) && CheckGrenadeThrow(m_nearestEnemy))
				return;
		}
		else
			UpdateLooking();
	}
	else
	{
		UpdateLooking();

		if (m_isSlowThink)
		{
			FindEnemyEntities();
			FindFriendsAndEnemiens();
			CheckReachable();

			// revert the zoom to normal
			if (!m_hasEnemiesNear && !m_hasEntitiesNear && UsesSniper() && pev->fov != 90.0f)
				m_buttons |= IN_ATTACK2;
		}
		else if (m_hasEnemiesNear && m_isEnemyReachable)
		{
			if (!m_navNode.HasNext())
			{
				// find new safe spot
				m_zhCampPointIndex = -1;
				FindGoalHuman();

				// use known waypoint first, then switch to auto
				FindEscapePath(m_currentWaypointIndex, m_nearestEnemy->v.origin);
				m_currentWaypointIndex = -1;

				MoveOut(m_enemyOrigin);
			}
			else if (((pev->origin - g_waypoint->m_paths[m_navNode.First()].origin).GetLengthSquared() > (m_nearestEnemy->v.origin - g_waypoint->m_paths[m_navNode.First()].origin).GetLengthSquared() ||
				(pev->origin - g_waypoint->m_paths[m_navNode.Next()].origin).GetLengthSquared() > (m_nearestEnemy->v.origin - g_waypoint->m_paths[m_navNode.Next()].origin).GetLengthSquared()) && ::IsInViewCone(pev->origin, m_nearestEnemy))
			{
				FindEscapePath(m_currentWaypointIndex, m_nearestEnemy->v.origin);
				MoveOut(m_enemyOrigin);
			}
			else
				FollowPath();

			return;
		}

		if (m_currentWaypointIndex == m_zhCampPointIndex && IsValidWaypoint(m_zhCampPointIndex))
		{
			if (!m_navNode.IsEmpty())
			{
				if (m_hasFriendsNear && !FNullEnt(m_nearestFriend) && (pev->origin - m_nearestFriend->v.origin).GetLengthSquared() < squaredf(48.0f))
				{
					const Bot* bot = g_botManager->GetBot(m_nearestFriend);
					if (bot && bot->m_navNode.IsEmpty() && m_zhCampPointIndex == bot->m_zhCampPointIndex)
					{
						m_navNode.Clear();
						MoveTo(m_nearestFriend->v.origin);
						return;
					}
				}

				FollowPath();
				return;
			}

			const Path zhPath = g_waypoint->m_paths[m_zhCampPointIndex];
			if (!(zhPath.flags & WAYPOINT_ZMHMCAMP) && !(zhPath.flags & WAYPOINT_HMCAMPMESH))
			{
				m_moveSpeed = pev->maxspeed;
				m_zhCampPointIndex = -1;
				FindGoalHuman();
				return;
			}

			if (m_isSlowThink)
			{
				const float maxRange = zhPath.flags & WAYPOINT_CROUCH ? 125.0f : 200.0f;
				if (((zhPath.origin - pev->origin).GetLengthSquared2D() > squaredf(maxRange) || (zhPath.origin.z - 54.0f > pev->origin.z)))
				{
					FindWaypoint();
					FindPath(m_currentWaypointIndex, m_currentGoalIndex);
					return;
				}
			}
			else if (zhPath.flags & WAYPOINT_CROUCH)
				m_duckTime = engine->GetTime() + 1.0f;

			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;

			ResetStuck();

			if (!g_waypoint->m_hmMeshPoints.IsEmpty())
			{
				const float time2 = engine->GetTime();
				if (m_currentProcessTime < time2 + 1.0f || m_currentProcessTime > time2 + 60.0f)
				{
					int16_t i, index, myCampPoint;
					MiniArray <int16_t> MeshWaypoints;

					for (i = 0; i < g_waypoint->m_hmMeshPoints.Size(); i++)
					{
						index = g_waypoint->m_hmMeshPoints.Get(i);
						if (!g_waypoint->GetPath(index)->mesh)
							continue;

						if (zhPath.mesh != g_waypoint->GetPath(index)->mesh)
							continue;

						MeshWaypoints.Push(index);
					}

					if (!MeshWaypoints.IsEmpty())
					{
						myCampPoint = MeshWaypoints.Random();
						m_myMeshWaypoint = myCampPoint;
						MeshWaypoints.Destroy();

						float max = 16.0f;
						if (m_hasEnemiesNear)
						{
							if (m_personality == Personality::Rusher)
								max = 20.0f;
							else if (m_personality != Personality::Careful)
								max = 12.0f;
						}

						m_currentProcessTime = time2 + crandomfloat(8.0f, max);
						m_zhCampPointIndex = m_myMeshWaypoint;
						m_currentGoalIndex = m_zhCampPointIndex;
						FindPath(m_currentWaypointIndex, m_myMeshWaypoint);
					}
				}
			}

			// standing still
			if (m_hasEnemiesNear && m_currentWeapon != Weapon::Knife && m_personality != Personality::Rusher && pev->velocity.GetLengthSquared2D() < 20.0f)
			{
				bool crouch = true;
				if (m_currentWeapon != Weapon::M3 ||
					m_currentWeapon != Weapon::Xm1014 ||
					m_currentWeapon != Weapon::G3SG1 ||
					m_currentWeapon != Weapon::Scout ||
					m_currentWeapon != Weapon::Awp ||
					m_currentWeapon != Weapon::M249 ||
					m_currentWeapon != Weapon::Sg550)
					crouch = false;

				if (m_personality == Personality::Normal && m_enemyDistance < squaredf(512.0f))
					crouch = false;

				if (crouch && IsVisible(m_enemyOrigin, m_myself))
					m_duckTime = engine->GetTime() + 1.0f;
			}
		}
		else if (!m_navNode.IsEmpty())
			FollowPath();
		else if (IsValidWaypoint(m_zhCampPointIndex))
			FindPath(m_currentWaypointIndex, m_zhCampPointIndex);
		else
			m_zhCampPointIndex = FindGoalHuman();
	}
}

void Bot::DefaultEnd(void)
{

}

bool Bot::DefaultReq(void)
{
	return true;
}