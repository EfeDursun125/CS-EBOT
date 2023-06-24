#include <core.h>

void Bot::PauseStart(void)
{

}

void Bot::PauseUpdate(void)
{
	if (m_isSlowThink)
	{
		FindEnemyEntities();
		FindFriendsAndEnemiens();
		if (m_hasEnemiesNear || m_hasEntitiesNear)
		{
			FinishCurrentProcess("enemies found me");
			return;
		}
	}

	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;
}

void Bot::PauseEnd(void)
{

}

bool Bot::PauseReq(void)
{
	if (m_hasEnemiesNear)
		return false;

	if (m_hasEntitiesNear)
		return false;

	// do not pause on ladder or in the water
	if (!IsOnFloor())
		return false;

	return true;
}