#include <core.h>

void Bot::ThrowSMStart(void)
{
	SelectWeaponByName("weapon_smokegrenade");
}

void Bot::ThrowSMUpdate(void)
{
	m_lookAt = m_throw;
	if (m_currentWeapon != WEAPON_SMGRENADE)
	{
		if (pev->weapons & (1 << WEAPON_SMGRENADE))
			SelectWeaponByName("weapon_smokegrenade");
		else // no grenade???
			FinishCurrentProcess("i have throwed SM grenade");
	}
	else if (!(pev->button & IN_ATTACK) && !(pev->oldbuttons & IN_ATTACK))
		pev->button |= IN_ATTACK;
}

void Bot::ThrowSMEnd(void)
{
	SelectBestWeapon();
}

bool Bot::ThrowSMReq(void)
{
	return true;
}