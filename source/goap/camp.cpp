#include <core.h>

void Bot::CampStart(void)
{
	DeleteSearchNodes();

	if (IsZombieMode() && IsValidWaypoint(m_zhCampPointIndex))
		m_campIndex = m_zhCampPointIndex;

	if (CRandomInt(1, 3) == 1)
	{
		const int random = CRandomInt(1, 3);
		if (random == 1)
			RadioMessage(Radio::InPosition);
		else if (random == 2)
			RadioMessage(Radio::GetInPosition);
		else
			RadioMessage(Radio::HoldPosition);
	}
}

void Bot::CampUpdate(void)
{
	if (!CampingAllowed())
	{
		FinishCurrentProcess("camping is not allowed for me");
		return;
	}

	UpdateLooking();

	if (m_isSlowThink)
	{
		// revert the zoom to normal
		if (pev->fov != 90.0f)
			pev->button |= IN_ATTACK2;

		FindEnemyEntities();

		if (!m_hasEnemiesNear)
		{
			extern ConVar ebot_chat;
			if (ebot_chat.GetBool() && !RepliesToPlayer() && m_lastChatTime + 10.0f < engine->GetTime() && g_lastChatTime + 5.0f < engine->GetTime()) // bot chatting turned on?
				m_lastChatTime = engine->GetTime();
		}
	}
	else
	{
		if (m_itemCheckTime < engine->GetTime())
		{
			FindItem();
			m_itemCheckTime = engine->GetTime() + engine->RandomFloat(1.25f, 2.5f);

			if (GetEntityOrigin(m_pickupItem) != nullvec && SetProcess(Process::Pickup, "i see good stuff to pick it up", true, 20.0f))
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
		if (m_hasEnemiesNear && m_enemyDistance <= SquaredF(300.0f))
		{
			SelectBestWeapon();
			m_currentWaypointIndex = -1;
			DeleteSearchNodes();
			MoveOut(m_enemyOrigin);
			return;
		}
		else
		{
			m_aimFlags |= AIM_CAMP;

			// standing still
			if (m_hasEnemiesNear && m_currentWeapon != WEAPON_KNIFE && m_personality != PERSONALITY_RUSHER && pev->velocity.GetLengthSquared2D() <= 18.0f)
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

				if (m_personality == PERSONALITY_NORMAL && m_enemyDistance < SquaredF(512.0f))
					crouch = false;

				if (crouch && IsVisible(m_enemyOrigin, GetEntity()))
					m_duckTime = engine->GetTime() + 1.0f;
			}
		}
	}
	else
	{
		if ((m_hasEnemiesNear || m_hasEntitiesNear) && m_currentWaypointIndex != m_campIndex)
		{
			if (SetProcess(Process::Attack, "i found a target", false, 999999.0f))
				return;
		}

		SelectBestWeapon();
	}

	FollowPath(m_campIndex);

	if (m_currentWaypointIndex == m_campIndex)
	{
		ResetStuck();

		if (IsZombieMode())
		{
			const Path* zhPath = g_waypoint->GetPath(m_campIndex);
			if (m_isSlowThink)
			{
				float maxRange = zhPath->flags & WAYPOINT_CROUCH ? 100.0f : 230.0f;
				if (zhPath->campStartX != 0.0f && ((zhPath->origin - pev->origin).GetLengthSquared2D() > SquaredF(maxRange) || (zhPath->origin.z - 64.0f > pev->origin.z)))
				{
					FindWaypoint();
					DeleteSearchNodes();
					return;
				}
			}
			else if (zhPath->flags & WAYPOINT_CROUCH)
				m_duckTime = AddTime(1.0f);

			if (!g_waypoint->m_hmMeshPoints.IsEmpty())
			{
				Array <int> MeshWaypoints;
				if (m_currentProcessTime > engine->GetTime() + 60.0f)
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
						m_prevGoalIndex = m_chosenGoalIndex;
						const int myCampPoint = MeshWaypoints.GetRandomElement();
						m_chosenGoalIndex = myCampPoint;
						m_myMeshWaypoint = myCampPoint;
						MeshWaypoints.RemoveAll();

						float max = 12.0f;
						if (!FNullEnt(m_enemy))
						{
							if (m_personality == PERSONALITY_RUSHER)
								max = 16.0f;
							else if (m_personality != PERSONALITY_CAREFUL)
								max = 8.0f;
						}

						m_currentProcessTime = AddTime(engine->RandomFloat(4.0f, max));
						FindPath(m_currentWaypointIndex, m_myMeshWaypoint);
					}
				}
			}
		}
		else
			m_duckTime = AddTime(1.0f);
	}
}

void Bot::CampEnd(void)
{
	m_campIndex = -1;
}

bool Bot::CampReq(void)
{
	if (m_isBomber)
		return false;

	if (m_isVIP)
		return false;

	if (HasHostage())
		return false;

	if (!IsValidWaypoint(m_campIndex))
		return false;

	return CampingAllowed();
}