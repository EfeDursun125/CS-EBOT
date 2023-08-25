#include <core.h>

void Bot::JumpStart(void)
{

}

void Bot::JumpUpdate(void)
{
	LookAt(m_destOrigin);

	if (IsOnFloor() || IsOnLadder())
	{
		m_strafeSpeed = 0.0f;
		m_moveSpeed = 0.0f;

		if (GetProcessTime() > 1.0f)
			FinishCurrentProcess("jumping finished");

		return;
	}

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