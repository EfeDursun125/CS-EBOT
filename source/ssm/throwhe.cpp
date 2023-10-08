#include <core.h>

void Bot::ThrowHEStart(void)
{
	SelectWeaponByName("weapon_hegrenade");
	pev->speed = 0.0f;
	pev->yaw_speed = 0.0f;
	pev->pitch_speed = 0.0f;
	pev->velocity = nullvec;
	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;
}

void Bot::ThrowHEUpdate(void)
{
	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;

	edict_t* ent = nullptr;
	while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "grenade")))
	{
		if (ent->v.owner == pev->pContainingEntity && cstrcmp(STRING(ent->v.model) + 9, "hegrenade.mdl") == 0)
		{
			ent->v.velocity = m_throw;
			FinishCurrentProcess("i have throwed HE grenade");
			return;
		}
	}

	LookAt(EyePosition() + m_throw);

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
	if (!IsZombieMode() && ((pev->origin + pev->velocity * m_frameInterval) - (m_throw + pev->velocity * -m_frameInterval)).GetLengthSquared() < squaredf(400.0f))
		return false;

	return true;
}