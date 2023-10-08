#include <core.h>

void Bot::EscapeStart(void)
{
	DeleteSearchNodes();
	RadioMessage(Radio::ShesGonnaBlow);
}

void Bot::EscapeUpdate(void)
{
	FindEnemyEntities();
	FindFriendsAndEnemiens();
	UpdateLooking();

	if (m_currentWaypointIndex == m_chosenGoalIndex)
		SetProcess(Process::Pause, "i have escaped from the bomb", true, engine->GetTime() + 99999999.0f);
	else
	{
		if (!m_hasEntitiesNear && m_numEnemiesLeft <= 0)
			SelectKnife();

		m_walkTime = 0.0f; // RUN FOR YOUR LIFE!
		if (!GoalIsValid())
		{
			Vector bombOrigin = g_waypoint->GetBombPosition();
			if (bombOrigin == nullvec)
				bombOrigin = pev->origin;

			const int index = g_waypoint->FindFarest(bombOrigin, 2048.0f);
			if (IsValidWaypoint(index))
			{
				m_prevGoalIndex = index;
				m_chosenGoalIndex = index;
				FollowPath(index);
			}
			else // WHERE WE MUST GO???
				m_chosenGoalIndex = crandomint(0, g_numWaypoints - 1);
		}
		else
			FollowPath(m_chosenGoalIndex);
	}
}

void Bot::EscapeEnd(void)
{
	if (m_numEnemiesLeft > 0)
	{
		if (IsShieldDrawn())
			pev->button |= IN_ATTACK2;
		else
			SelectBestWeapon();
	}
}

bool Bot::EscapeReq(void)
{
	return true;
}