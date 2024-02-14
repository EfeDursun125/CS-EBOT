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
			cvar_t* maxVel = g_engfuncs.pfnCVarGetPointer("sv_maxvelocity");
			if (maxVel)
			{
				const float fVel = maxVel->value;
				Vector fixedVel;
				fixedVel.x = cclampf(m_throw.x, -fVel, fVel);
				fixedVel.y = cclampf(m_throw.y, -fVel, fVel);
				fixedVel.z = cclampf(m_throw.z, -fVel, fVel);
				ent->v.velocity = fixedVel;
			}
			else
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
	else if (!(pev->buttons & IN_ATTACK) && !(pev->oldbuttons & IN_ATTACK))
		pev->buttons |= IN_ATTACK;
}

void Bot::ThrowSMEnd(void)
{
	SelectBestWeapon();
}

bool Bot::ThrowSMReq(void)
{
	return true;
}