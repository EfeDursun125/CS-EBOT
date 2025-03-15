#include "../../include/core.h"

void Bot::PauseStart(void)
{
	m_duckTime = 0.0f;
	m_navNode.Stop();
	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;
	pev->speed = 0.0f;
	m_jumpReady = false;
	m_waitForLanding = false;
	IgnoreCollisionShortly();
}

void Bot::PauseUpdate(void)
{
	m_updateLooking = false;
	m_isSlowThink = false;
}

void Bot::PauseEnd(void)
{
	const float speed = pev->maxspeed * 0.5f;
	if (speed > 10.0f)
	{
		const Vector vel = (m_waypoint.origin - pev->origin).Normalize2D();
		if (!vel.IsNull())
		{
			pev->velocity.x = vel.x * speed;
			pev->velocity.y = vel.y * speed;
		}
	}
}

bool Bot::PauseReq(void)
{
	if (IsOnLadder())
		return false;

	if (IsInWater())
		return false;

	return true;
}