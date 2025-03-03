#include "../../include/core.h"

void Bot::ThrowFBStart(void)
{
	SelectWeaponByName("weapon_flashbang");
}

void Bot::ThrowFBUpdate(void)
{
	edict_t* ent = nullptr;
	while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "grenade")))
	{
		if (ent->v.owner == m_myself && cstrcmp(STRING(ent->v.model) + 9, "flashbang.mdl") == 0)
		{
			cvar_t* maxVel = g_engfuncs.pfnCVarGetPointer("sv_maxvelocity");
			if (maxVel)
			{
				const float fVel = maxVel->value;
				if (!Math::FltZero(fVel))
				{
					Vector fixedVel;
					fixedVel.x = cclampf(m_throw.x, -fVel, fVel);
					fixedVel.y = cclampf(m_throw.y, -fVel, fVel);
					fixedVel.z = cclampf(m_throw.z, -fVel, fVel);
					if (!fixedVel.IsNull())
						ent->v.velocity = fixedVel;
				}
			}
			else if (!m_throw.IsNull())
				ent->v.velocity = m_throw;

			FinishCurrentProcess("i have throwed FB grenade");
			return;
		}
	}

	LookAt(EyePosition() + m_throw);

	if (m_currentWeapon != Weapon::FbGrenade)
	{
		if (!(m_oldButtons & IN_ATTACK) && pev->weapons & (1 << Weapon::FbGrenade))
			SelectWeaponByName("weapon_flashbang");
		else // no grenade???
			FinishCurrentProcess("i have throwed FB grenade");
	}
	else if (m_isSlowThink && !(m_buttons & IN_ATTACK) && !(m_oldButtons & IN_ATTACK))
		m_buttons |= IN_ATTACK;
}

void Bot::ThrowFBEnd(void)
{
	SelectBestWeapon();
}

bool Bot::ThrowFBReq(void)
{
	if (!IsOnFloor())
		return false;

	return true;
}