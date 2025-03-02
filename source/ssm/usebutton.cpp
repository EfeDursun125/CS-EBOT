#include "../../include/core.h"

void Bot::UseButtonStart(void)
{

}

void Bot::UseButtonUpdate(void)
{
	if (FNullEnt(m_buttonEntity))
	{
		FinishCurrentProcess("button is gone...");
		return;
	}

	const Vector buttonOrigin = GetEntityOrigin(m_buttonEntity);
	if ((buttonOrigin - pev->origin).GetLengthSquared() < squaredf(90.0f)) // near to the button?
	{
		m_lookAt = buttonOrigin;
		if (InFieldOfView(buttonOrigin - EyePosition()) < 16.0f) // facing it directly?
		{
			if (g_gameVersion & Game::Xash)
				pev->buttons |= IN_USE;
			else
				MDLL_Use(m_buttonEntity, m_myself);

			FinishCurrentProcess("i have pushed the button");
			return;
		}

		MoveTo(buttonOrigin);
	}
	else
	{
		FindFriendsAndEnemiens();
		FindEnemyEntities();
		UpdateLooking();

		if (m_navNode.HasNext())
			FollowPath();
		else
		{
			int index = g_waypoint->FindNearestToEnt(buttonOrigin, 512.0f, m_buttonEntity);
			if (IsValidWaypoint(index))
				FindShortestPath(m_currentWaypointIndex, index);
			else
				MoveTo(m_lookAt);
		}
	}
}

void Bot::UseButtonEnd(void)
{
	m_buttonEntity = nullptr;
}

bool Bot::UseButtonReq(void)
{
	if (FNullEnt(m_buttonEntity))
		return false;

	return true;
}