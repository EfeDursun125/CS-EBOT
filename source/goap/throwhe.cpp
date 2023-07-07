#include <core.h>

void Bot::ThrowHEStart(void)
{
	SelectWeaponByName("weapon_hegrenade");
}

void Bot::ThrowHEUpdate(void)
{
	m_lookAt = m_throw;
	if (m_currentWeapon != WEAPON_HEGRENADE)
	{
		if (pev->weapons & (1 << WEAPON_HEGRENADE))
			SelectWeaponByName("weapon_hegrenade");
		else // no grenade???
			FinishCurrentProcess("i have throwed HE grenade");
	}
	else if (!(pev->button & IN_ATTACK) && !(pev->oldbuttons & IN_ATTACK))
		pev->button |= IN_ATTACK;
}

void Bot::ThrowHEEnd(void)
{
	SelectBestWeapon();
}

bool Bot::ThrowHEReq(void)
{
	return true;
}