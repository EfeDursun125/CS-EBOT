#include <core.h>

void Bot::ThrowHEStart(void)
{
	SelectWeaponByName("weapon_hegrenade");
}

void Bot::ThrowHEUpdate(void)
{
	edict_t* ent = nullptr;
	while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "grenade")))
	{
		if (ent->v.owner == GetEntity() && strcmp(STRING(ent->v.model) + 9, "hegrenade.mdl") == 0)
		{
			Vector grenade = CheckThrow(EyePosition(), m_throw);

			if (grenade.GetLengthSquared() < 100)
				grenade = CheckToss(EyePosition(), m_throw);

			if (grenade.GetLengthSquared() > 100)
				ent->v.velocity = grenade;

			FinishCurrentProcess("i have throwed HE grenade");
			return;
		}
	}

	LookAt(m_throw + pev->velocity * -m_frameInterval);

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
	if (!IsZombieMode() && ((pev->origin + pev->velocity * m_frameInterval) - (m_throw + pev->velocity * -m_frameInterval)).GetLengthSquared() < SquaredF(400.0f))
		return false;

	return true;
}