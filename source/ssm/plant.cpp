#include <core.h>

void Bot::PlantStart(void)
{
	RadioMessage(Radio::CoverMe);
}

void Bot::PlantUpdate(void)
{
	SelectWeaponByName("weapon_c4");

	pev->buttons |= (IN_ATTACK | IN_DUCK);

	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;

	if (g_bombPlanted || !(pev->weapons & (1 << Weapon::C4)))
	{
		g_bombPlanted = true;
		ResetStuck();
		FinishCurrentProcess("succsesfully planted the bomb");
		m_isBomber = false;
		return;
	}

	/*if (!m_hasProgressBar && !m_inBombZone)
	{
		FinishCurrentProcess("i'm not at the plant area");
		return;
	}*/
}

void Bot::PlantEnd(void)
{
	switch (crandomint(1, 4))
	{
	case 1:
	{
		RadioMessage(Radio::RegroupTeam);
		break;
	}
	case 2:
	{
		RadioMessage(Radio::HoldPosition);
		break;
	}
	case 3:
	{
		RadioMessage(Radio::GetInPosition);
		break;
	}
	case 4:
	{
		RadioMessage(Radio::StickTogether);
		break;
	}
	}

	m_navNode.Clear();
	FindWaypoint();
}

bool Bot::PlantReq(void)
{
	if (m_hasEnemiesNear)
		return false;

	if (m_hasEntitiesNear)
		return false;

	if (g_bombPlanted)
		return false;

	if (!m_isBomber)
		return false;

	//if (!m_hasProgressBar && !m_inBombZone) trust the waypointer
	//	return false;

	if (!IsOnFloor())
		return false;

	return true;
}