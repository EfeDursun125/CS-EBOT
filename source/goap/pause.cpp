#include <core.h>

void Bot::PauseStart(void)
{
	ResetStuck();
}

void Bot::PauseUpdate(void)
{
	if (IsOnFloor() && m_jumpTime + 0.1f < engine->GetTime())
	{
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;

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
	}
	else
	{
		const Vector directionOld = m_destOrigin - (pev->origin + pev->velocity * m_frameInterval);
		m_moveAngles = directionOld.ToAngles();
		m_moveAngles.ClampAngles();
		m_moveSpeed = pev->maxspeed;
		SetStrafeSpeedNoCost(directionOld, pev->maxspeed);
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