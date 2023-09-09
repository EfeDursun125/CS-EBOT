#include <core.h>

void Bot::ThrowFBStart(void)
{
	SelectWeaponByName("weapon_flashbang");
	pev->speed = 0.0f;
	pev->yaw_speed = 0.0f;
	pev->pitch_speed = 0.0f;
	pev->velocity = nullvec;
	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;
}

void Bot::ThrowFBUpdate(void)
{
	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;

	edict_t* ent = nullptr;
	while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "grenade")))
	{
		if (ent->v.owner == pev->pContainingEntity && cstrcmp(STRING(ent->v.model) + 9, "flashbang.mdl") == 0)
		{
			ent->v.velocity = m_throw;
			FinishCurrentProcess("i have throwed FB grenade");
			return;
		}
	}

	LookAt(EyePosition() + m_throw);

	if (m_currentWeapon != Weapon::FbGrenade)
	{
		if (pev->weapons & (1 << Weapon::FbGrenade))
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