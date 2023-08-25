#include <core.h>

void Bot::DefaultStart(void)
{
	if (m_isZombieBot)
		SelectWeaponByName("weapon_knife");
}

void Bot::DefaultUpdate(void)
{
	if (m_isZombieBot)
	{
		if (m_isSlowThink)
		{
			FindEnemyEntities();

			if (m_hasEnemiesNear && !FNullEnt(m_nearestEnemy))
				CheckGrenadeThrow(m_nearestEnemy);
		}
		else
			FindFriendsAndEnemiens();

		// nearest enemy never resets to nullptr, so bot always know where are humans
		if (!FNullEnt(m_nearestEnemy) && GetTeam(m_nearestEnemy) != m_team)
		{
			if (m_hasEnemiesNear && IsEnemyReachable())
			{
				m_currentWaypointIndex = -1;
				DeleteSearchNodes();
				MoveTo(m_enemyOrigin);
				LookAt(m_enemyOrigin);

				if (CRandomInt(1, 3) == 1)
					pev->button |= IN_ATTACK2;
				else
					pev->button |= IN_ATTACK;

				CheckStuck(pev->maxspeed);

				return;
			}
			else
				FollowPath(m_nearestEnemy->v.origin);
		}
		else
		{
			if (!GoalIsValid())
				FindGoal();

			FollowPath(m_chosenGoalIndex);
		}

		UpdateLooking();
	}
	else
	{
		UpdateLooking();

		if (m_isSlowThink)
		{
			// revert the zoom to normal
			if (UsesSniper() && pev->fov != 90.0f)
				pev->button |= IN_ATTACK2;

			FindEnemyEntities();

			if (IsValidWaypoint(m_zhCampPointIndex))
			{
				m_campIndex = m_zhCampPointIndex;
				if (SetProcess(Process::Camp, "i will camp until game ends", true, AddTime(9999999.0f)))
				{
					DeleteSearchNodes();
					return;
				}
			}

			CheckRadioCommands();
		}
		else
		{
			const float time = engine->GetTime();
			if (m_itemCheckTime < engine->GetTime())
			{
				FindItem();
				m_itemCheckTime = time + (g_gameVersion & Game::HalfLife ? 1.0f : CRandomFloat(1.0f, 2.0f));

				if (GetEntityOrigin(m_pickupItem) != nullvec && SetProcess(Process::Pickup, "i see good stuff to pick it up", true, time + 20.0f))
					return;
			}
			else
			{
				CheckReload();
				FindFriendsAndEnemiens();
			}
		}

		if (IsZombieMode())
		{
			if (m_hasEnemiesNear && IsEnemyReachable())
			{
				m_currentWaypointIndex = -1;
				DeleteSearchNodes();
				MoveOut(m_enemyOrigin);
				CheckStuck(pev->maxspeed);
				return;
			}
			else if (!GoalIsValid())
				FindGoal();

			FollowPath(m_chosenGoalIndex);
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
				else if (SetProcess(Process::Attack, "i found a target", false, AddTime(999999.0f)))
					return;
			}
			else if (m_isBomber && m_waypointFlags & WAYPOINT_GOAL && m_navNode == nullptr)
			{
				if (SetProcess(Process::Plant, "trying to plant the bomb.", false, AddTime(12.0f)))
					return;
			}

			if (!GoalIsValid())
				FindGoal();

			FollowPath(m_chosenGoalIndex);
		}
	}
}

void Bot::DefaultEnd(void)
{
	m_isReloading = false;
}

bool Bot::DefaultReq(void)
{
	return true;
}