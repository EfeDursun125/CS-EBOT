#include <core.h>

void Bot::DefuseStart(void)
{
	RadioMessage(Radio::CoverMe);
	g_bombDefusing = true;
}

void Bot::DefuseUpdate(void)
{
	const Vector bombOrigin = g_waypoint->GetBombPosition();
	if (bombOrigin == nullvec)
	{
		g_bombPlanted = false;
		RadioMessage(Radio::SectorClear);
		FinishCurrentProcess("where's the bomb?");
		return;
	}

	LookAt(bombOrigin);

	if (EyePosition().z < bombOrigin.z)
		pev->buttons |= IN_USE;
	else
		pev->buttons |= (IN_USE | IN_DUCK);

	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;

	if (!g_bombPlanted)
		FinishCurrentProcess("i have defused the bomb");
}

void Bot::DefuseEnd(void)
{
	g_bombDefusing = false;
}

bool Bot::DefuseReq(void)
{
	if (m_hasEnemiesNear)
		return false;

	if (m_hasEntitiesNear)
		return false;

	if (!g_bombPlanted)
		return false;

	if (m_team != Team::Counter)
		return false;

	if (g_waypoint->GetBombPosition() == nullvec)
		return false;

	if (!IsOnFloor())
		return false;

	return true;
}