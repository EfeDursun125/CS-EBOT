#include <core.h>

void Bot::ThrowSMStart(void)
{
	SelectWeaponByName("weapon_smokegrenade");
}

void Bot::ThrowSMUpdate(void)
{
	LookAt(m_throw + pev->velocity * -m_frameInterval);

	if (m_currentWeapon != Weapon::SmGrenade)
	{
		if (pev->weapons & (1 << Weapon::SmGrenade))
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