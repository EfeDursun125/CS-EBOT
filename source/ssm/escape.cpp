#include <core.h>

void Bot::EscapeStart(void)
{
	m_navNode.Clear();
	RadioMessage(Radio::ShesGonnaBlow);
}

void Bot::EscapeUpdate(void)
{
	FindEnemyEntities();
	FindFriendsAndEnemiens();
	UpdateLooking();

	if (m_currentWaypointIndex == m_navNode.Last())
		SetProcess(Process::Pause, "i have escaped from the bomb", true, engine->GetTime() + 99999999.0f);
	else
	{
		if (!m_hasEntitiesNear && m_numEnemiesLeft <= 0)
			SelectKnife();

		m_walkTime = 0.0f; // RUN FOR YOUR LIFE!
		if (m_navNode.IsEmpty())
		{
			Vector bombOrigin = g_waypoint->GetBombPosition();
			if (bombOrigin == nullvec)
				bombOrigin = pev->origin;

			
			int index = g_waypoint->FindFarest(bombOrigin, 2048.0f);
			if (IsValidWaypoint(index))
				FindPath(m_currentWaypointIndex, index);
			else // WHERE WE MUST GO???
				FindEscapePath(m_currentWaypointIndex, bombOrigin);
		}
		else
			FollowPath();
	}
}

void Bot::EscapeEnd(void)
{
	if (m_numEnemiesLeft > 0)
	{
		if (IsShieldDrawn())
			pev->buttons |= IN_ATTACK2;
		else
			SelectBestWeapon();
	}
}

bool Bot::EscapeReq(void)
{
	return true;
}