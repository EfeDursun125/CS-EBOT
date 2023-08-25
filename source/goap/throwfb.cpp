#include <core.h>

void Bot::ThrowFBStart(void)
{
	SelectWeaponByName("weapon_flashbang");
}

void Bot::ThrowFBUpdate(void)
{
	edict_t* ent = nullptr;
	while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "grenade")))
	{
		if (ent->v.owner == GetEntity() && strcmp(STRING(ent->v.model) + 9, "flashbang.mdl") == 0)
		{
			Vector grenade = CheckThrow(EyePosition(), m_throw);

			if (grenade.GetLengthSquared() < 100)
				grenade = CheckToss(EyePosition(), m_throw);

			if (grenade.GetLengthSquared() > 100)
				ent->v.velocity = grenade;

			FinishCurrentProcess("i have throwed FB grenade");
			return;
		}
	}

	LookAt(m_throw + pev->velocity * -m_frameInterval);

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