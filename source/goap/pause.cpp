#include <core.h>

void Bot::PauseStart(void)
{
	ResetStuck();
}

void Bot::PauseUpdate(void)
{
	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;
	FindEnemyEntities();
	FindFriendsAndEnemiens();
	if (m_hasEnemiesNear || m_hasEntitiesNear)
	{
		FinishCurrentProcess("enemies found me");
		return;
	}
}

void Bot::PauseEnd(void)
{

}

bool Bot::PauseReq(void)
{
	if (!IsZombieMode())
	{
		if (m_hasEnemiesNear)
			return false;

		if (m_hasEntitiesNear)
			return false;

		// do not pause on ladder or in the water
		if (!IsOnFloor())
			return false;
	}

	if (IsOnLadder())
		return false;

	if (IsInWater())
		return false;

	return true;
}