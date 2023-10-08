#include <core.h>

void Bot::CampStart(void)
{
	DeleteSearchNodes();

	if (IsZombieMode() && IsValidWaypoint(m_zhCampPointIndex))
		m_campIndex = m_zhCampPointIndex;

	if (crandomint(1, 3) == 1)
	{
		const int random = crandomint(1, 3);
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

	const float time = engine->GetTime();
	if (m_isSlowThink)
	{
		// revert the zoom to normal
		if (pev->fov != 90.0f)
			pev->button |= IN_ATTACK2;

		FindEnemyEntities();

		if (!m_hasEnemiesNear && !g_isFakeCommand)
		{
			extern ConVar ebot_chat;
			if (ebot_chat.GetBool() && m_lastChatTime + 10.0f < time && g_lastChatTime + 5.0f < time && !RepliesToPlayer()) // bot chatting turned on?
				m_lastChatTime = time;
		}

		CheckRadioCommands();
	}
	else
	{
		if (m_itemCheckTime < time)
		{
			FindItem();
			m_itemCheckTime = time + 2.0f;

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
		else
		{
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

				if (crouch && IsVisible(m_enemyOrigin, GetEntity()))
					m_duckTime = time + 1.0f;
			}
		}
	}
	else
	{
		if (OutOfBombTimer())
			SetProcess(Process::Escape, "escaping from the bomb", true, time + GetBombTimeleft());
		else if ((m_hasEnemiesNear || m_hasEntitiesNear) && m_currentWaypointIndex != m_campIndex)
		{
			if (SetProcess(Process::Attack, "i found a target", false, time + 99999999.0f))
				return;
		}
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
				const float maxRange = zhPath->flags & WAYPOINT_CROUCH ? 125.0f : 200.0f;
				if (zhPath->mesh != 0 && ((zhPath->origin - pev->origin).GetLengthSquared2D() > squaredf(maxRange) || (zhPath->origin.z - 54.0f > pev->origin.z)))
				{
					DeleteSearchNodes();
					FindWaypoint();
					return;
				}
			}
			else if (zhPath->flags & WAYPOINT_CROUCH)
				m_duckTime = time + 1.0f;

			if (!g_waypoint->m_hmMeshPoints.IsEmpty())
			{
				if (m_currentProcessTime < time + 0.16f || m_currentProcessTime > time + 60.0f)
				{
					Array <int> MeshWaypoints;

					for (int i = 0; i <= g_waypoint->m_hmMeshPoints.GetElementNumber(); i++)
					{
						int index;
						g_waypoint->m_hmMeshPoints.GetAt(i, index);

						if (g_waypoint->GetPath(index)->mesh == 0)
							continue;

						if (zhPath->mesh != g_waypoint->GetPath(index)->mesh)
							continue;

						MeshWaypoints.Push(index);
					}

					if (!MeshWaypoints.IsEmpty())
					{
						m_prevGoalIndex = m_chosenGoalIndex;
						const int myCampPoint = MeshWaypoints.GetRandomElement();
						m_chosenGoalIndex = myCampPoint;
						m_myMeshWaypoint = myCampPoint;
						MeshWaypoints.Destroy();

						float max = 12.0f;
						if (m_hasEnemiesNear)
						{
							if (m_personality == Personality::Rusher)
								max = 16.0f;
							else if (m_personality != Personality::Careful)
								max = 8.0f;
						}

						m_currentProcessTime = time + crandomfloat(4.0f, max);
						m_campIndex = m_myMeshWaypoint;
						FindPath(m_currentWaypointIndex, m_myMeshWaypoint);
					}
				}
			}
		}
		else
			m_duckTime = time + 1.0f;
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

	if (!IsZombieMode() && IsWaypointOccupied(m_campIndex))
		return false;

	if (!CampingAllowed())
		return false;

	return true;
}