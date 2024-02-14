#include <core.h>

void Bot::BlindStart(void)
{

}

void Bot::BlindUpdate(void)
{
	switch (m_personality)
	{
	case Personality::Careful:
	{
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;
		break;
	}
	case Personality::Rusher:
	{
		const Vector directionOld = (m_lookAt + pev->velocity * -m_frameInterval) - (pev->origin + pev->velocity * m_frameInterval);
		const Vector directionNormal = directionOld.Normalize2D();
		SetStrafeSpeed(directionNormal, pev->maxspeed);
		m_moveAngles = directionOld.ToAngles();
		m_moveAngles.ClampAngles();
		m_moveAngles.x = -m_moveAngles.x;
		m_moveSpeed = 0.0f;
		break;
	}
	default:
		pev->buttons |= IN_ATTACK;
	}
}

void Bot::BlindEnd(void)
{

}

bool Bot::BlindReq(void)
{
	return true;
}