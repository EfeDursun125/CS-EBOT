#include <core.h>
ConVar ebot_kill_breakables("ebot_kill_breakables", "0");

void Bot::DestroyBreakableStart(void)
{
	SelectBestWeapon();
}

void Bot::DestroyBreakableUpdate(void)
{
	if (FNullEnt(m_breakableEntity) || m_breakableEntity->v.health <= 0.0f)
	{
		FinishCurrentProcess("sucsessfully destroyed a breakable");
		return;
	}

	if (ebot_kill_breakables.GetBool())
		m_breakableEntity->v.health = -1.0f;

	CheckStuck(pev->maxspeed);

	if (m_stuckWarn > 19)
	{
		FinishCurrentProcess("i'm stuck");
		return;
	}

	LookAt(m_breakable);

	if (pev->origin.z > m_breakable.z)
		m_duckTime = engine->GetTime() + 1.0f;
	else if (!IsVisible(m_breakable, GetEntity()))
		m_duckTime = engine->GetTime() + 1.0f;

	if (m_isZombieBot)
	{
		if (!(pev->buttons & IN_ATTACK2) && !(pev->oldbuttons & IN_ATTACK2))
			pev->buttons |= IN_ATTACK2;

		return;
	}

	WeaponSelect* selectTab = (g_gameVersion & Game::HalfLife) ? &g_weaponSelectHL[0] : &g_weaponSelect[0];
	if (ebot_knifemode.GetBool())
	{
		const int melee = (g_gameVersion & Game::HalfLife) ? WeaponHL::Crowbar : Weapon::Knife;
		if (m_currentWeapon == selectTab[melee].id)
		{
			if (!(pev->buttons & IN_ATTACK2) && !(pev->oldbuttons & IN_ATTACK2))
				pev->buttons |= IN_ATTACK2;
		}
		else
			SelectWeaponByName(selectTab[melee].weaponName);

		return;
	}

	int id;
	int chosenWeaponIndex = -1;
	int selectIndex = 0;
	const float distance = (pev->origin - m_breakable).GetLengthSquared();
	while (id = selectTab[selectIndex].id)
	{
		if (!(pev->weapons & (1 << id)))
		{
			selectIndex++;
			continue;
		}

		// cannot be used in water...
		// if (pev->waterlevel == 3 && g_weaponDefs[id].flags & ITEM_FLAG_NOFIREUNDERWATER)
		//	continue;

		if (g_gameVersion & Game::HalfLife)
		{
			if (selectIndex == WeaponHL::Snark || selectIndex == WeaponHL::Gauss || selectIndex == WeaponHL::Egon || (selectIndex == WeaponHL::HandGrenade && distance > squaredf(384.0f) && distance < squaredf(768.0f)) || (selectIndex == WeaponHL::Rpg && distance > squaredf(320.0f)) || (selectIndex == WeaponHL::Crossbow && distance > squaredf(320.0f)))
				chosenWeaponIndex = selectIndex;
			else if (selectIndex != WeaponHL::HandGrenade && selectIndex != WeaponHL::Rpg && selectIndex != WeaponHL::Crossbow && m_ammoInClip[id] > 0 && !IsWeaponBadInDistance(selectIndex, distance))
				chosenWeaponIndex = selectIndex;
		}
		else if (m_ammoInClip[id] > 0 && !IsWeaponBadInDistance(selectIndex, distance))
			chosenWeaponIndex = selectIndex;

		selectIndex++;
	}

	if (chosenWeaponIndex == -1)
	{
		selectIndex = 0;
		while (id = selectTab[selectIndex].id)
		{
			if (!(pev->weapons & (1 << id)))
			{
				selectIndex++;
				continue;
			}

			// cannot be used in water...
			// if (pev->waterlevel == 3 && g_weaponDefs[id].flags & ITEM_FLAG_NOFIREUNDERWATER)
			//	continue;

			if (m_ammo[g_weaponDefs[id].ammo1] > 0)
				chosenWeaponIndex = selectIndex;

			selectIndex++;
		}
	}

	if (chosenWeaponIndex == -1)
	{
		chosenWeaponIndex = (g_gameVersion & Game::HalfLife) ? WeaponHL::Crowbar : Weapon::Knife;
		if (m_currentWeapon == selectTab[chosenWeaponIndex].id)
		{
			if (!(pev->buttons & IN_ATTACK2) && !(pev->oldbuttons & IN_ATTACK2))
				pev->buttons |= IN_ATTACK2;

			return;
		}

		SelectWeaponByName(selectTab[chosenWeaponIndex].weaponName);
		return;
	}

	if (m_currentWeapon != selectTab[chosenWeaponIndex].id)
	{
		SelectWeaponByName(selectTab[chosenWeaponIndex].weaponName);
		return;
	}

	if (GetAmmoInClip() == 0)
	{
		if (!(pev->oldbuttons & IN_RELOAD))
			pev->buttons |= IN_RELOAD;

		return;
	}

	if (selectTab[chosenWeaponIndex].primaryFireHold) // if automatic weapon, just press attack
		pev->buttons |= IN_ATTACK;
	else // if not, toggle buttons
	{
		if (!(pev->oldbuttons & IN_ATTACK))
			pev->buttons |= IN_ATTACK;
	}
}

void Bot::DestroyBreakableEnd(void)
{
	m_nearestEntity = nullptr;
	m_nearestEnemy = nullptr;
	m_enemyDistance = FLT_MAX;
	m_entityDistance = FLT_MAX;
}

bool Bot::DestroyBreakableReq(void)
{
	if (m_breakableEntity->v.health <= 0.0f)
		return false;

	return true;
}