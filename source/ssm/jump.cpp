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
	}
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