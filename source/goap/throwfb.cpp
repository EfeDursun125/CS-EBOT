#include <core.h>

void Bot::ThrowFBStart(void)
{
	SelectWeaponByName("weapon_flashbang");

	TraceResult tr{};
	TraceHull(EyePosition(), m_lookAt, false, point_hull, GetEntity(), &tr);
	if (tr.flFraction != 1.0f)
	{
		const int index = g_waypoint->FindNearest(m_lookAt, 9999999.0f, -1, GetEntity());
		if (IsValidWaypoint(index))
			m_lookAt = g_waypoint->GetPath(index)->origin;
	}
}

void Bot::ThrowFBUpdate(void)
{
	//m_lookAt = m_throw;

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