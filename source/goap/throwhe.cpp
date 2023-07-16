#include <core.h>

void Bot::ThrowHEStart(void)
{
	SelectWeaponByName("weapon_hegrenade");

	TraceResult tr{};
	TraceHull(EyePosition(), m_lookAt, false, point_hull, GetEntity(), &tr);
	if (tr.flFraction != 1.0f)
	{
		const int index = g_waypoint->FindNearest(m_lookAt, 9999999.0f, -1, GetEntity());
		if (IsValidWaypoint(index))
			m_lookAt = g_waypoint->GetPath(index)->origin;
	}
}

void Bot::ThrowHEUpdate(void)
{
	LookAt(m_lookAt);

	if (m_currentWeapon != Weapon::HeGrenade)
	{
		if (pev->weapons & (1 << Weapon::HeGrenade))
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