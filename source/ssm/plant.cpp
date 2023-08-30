#include <core.h>

void Bot::PlantStart(void)
{
	RadioMessage(Radio::CoverMe);
}

void Bot::PlantUpdate(void)
{
	if (!m_inBombZone)
	{
		FinishCurrentProcess("i'm not at the plant area");
		return;
	}

	SelectWeaponByName("weapon_c4");

	pev->button |= (IN_ATTACK | IN_DUCK);

	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;

	if (g_bombPlanted)
		FinishCurrentProcess("succsesfully planted the bomb");
}

void Bot::PlantEnd(void)
{
	const int random = CRandomInt(1, 4);
	switch (random)
	{
	case 1:
		RadioMessage(Radio::RegroupTeam);
	case 2:
		RadioMessage(Radio::HoldPosition);
	case 3:
		RadioMessage(Radio::GetInPosition);
	case 4:
		RadioMessage(Radio::StickTogether);
	}

	m_isBomber = false;
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

	if (!IsOnFloor())
		return false;

	return true;
}