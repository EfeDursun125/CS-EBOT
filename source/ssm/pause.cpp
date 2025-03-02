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
}

void Bot::PauseUpdate(void)
{
	m_updateLooking = false;
	IgnoreCollisionShortly();
}

void Bot::PauseEnd(void)
{

}

bool Bot::PauseReq(void)
{
	if (IsOnLadder())
		return false;

	if (IsInWater())
		return false;

	return true;
}