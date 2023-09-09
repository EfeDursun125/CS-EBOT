#include <core.h>

void Bot::ThrowSMStart(void)
{
	SelectWeaponByName("weapon_smokegrenade");
	pev->speed = 0.0f;
	pev->yaw_speed = 0.0f;
	pev->pitch_speed = 0.0f;
	pev->velocity = nullvec;
	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;
}

void Bot::ThrowSMUpdate(void)
{
	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;

	edict_t* ent = nullptr;
	while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "grenade")))
	{
		if (ent->v.owner == pev->pContainingEntity && cstrcmp(STRING(ent->v.model) + 9, "smokegrenade.mdl") == 0)
		{
			ent->v.velocity = m_throw;
			FinishCurrentProcess("i have throwed SM grenade");
			return;
		}
	}

	LookAt(EyePosition() + m_throw);

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