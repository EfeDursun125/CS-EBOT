#include <core.h>

void Bot::JumpStart(void)
{

}

void Bot::JumpUpdate(void)
{
	UpdateLooking();

	if (IsOnFloor() || IsOnLadder())
	{
		m_strafeSpeed = 0.0f;
		m_moveSpeed = 0.0f;

		if (GetProcessTime() > 1.25f)
			FinishCurrentProcess("jumping finished");

		return;
	}

	if (m_stuckWarn > 1 && m_navNode != nullptr)
		DoWaypointNav();

	const Vector directionOld = (m_destOrigin + pev->velocity * -m_frameInterval) - (pev->origin + pev->velocity * m_frameInterval);
	const Vector directionNormal = directionOld.Normalize2D();
	SetStrafeSpeedNoCost(directionNormal, pev->maxspeed);
	m_moveAngles = directionOld.ToAngles();
	m_moveAngles.ClampAngles();
	m_moveAngles.x = -m_moveAngles.x;
	m_moveSpeed = pev->maxspeed;
}

void Bot::JumpEnd(void)
{

}

bool Bot::JumpReq(void)
{
	if (IsInWater())
		return false;

	return true;
}