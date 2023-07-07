#include <core.h>

void Bot::ThrowFBStart(void)
{
	SelectWeaponByName("weapon_flashbang");
}

void Bot::ThrowFBUpdate(void)
{
	m_lookAt = m_throw;
	if (m_currentWeapon != WEAPON_FBGRENADE)
	{
		if (pev->weapons & (1 << WEAPON_FBGRENADE))
			SelectWeaponByName("weapon_flashbang");
		else // no grenade???
			FinishCurrentProcess("i have throwed FB grenade");
	}
	else if (!(pev->button & IN_ATTACK) && !(pev->oldbuttons & IN_ATTACK))
		pev->button |= IN_ATTACK;
}

void Bot::ThrowFBEnd(void)
{
	SelectBestWeapon();
}

bool Bot::ThrowFBReq(void)
{
	return true;
}