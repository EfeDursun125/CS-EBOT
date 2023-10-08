//
// Copyright (c) 2003-2009, by Yet Another POD-Bot Development Team.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// $Id:$
//

#include <core.h>

ConVar ebot_escape("ebot_zombie_escape_mode", "0");
ConVar ebot_zp_use_grenade_percent("ebot_zm_use_grenade_percent", "10");
ConVar ebot_zp_escape_distance("ebot_zm_escape_distance", "200");
ConVar ebot_zombie_speed_factor("ebot_zombie_speed_factor", "0.54");
ConVar ebot_sb_mode("ebot_sb_mode", "0");

int Bot::GetNearbyFriendsNearPosition(const Vector origin, const float radius)
{
	int count = 0;
	for (const auto& client : g_clients)
	{
		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == pev->pContainingEntity)
			continue;

		if ((client.origin, origin).GetLengthSquared() < radius)
			count++;
	}

	return count;
}

int Bot::GetNearbyEnemiesNearPosition(const Vector origin, const float radius)
{
	int count = 0;
	for (const auto& client : g_clients)
	{
		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team == m_team)
			continue;

		if ((client.origin, origin).GetLengthSquared() < radius)
			count++;
	}

	return count;
}

void Bot::FindFriendsAndEnemiens(void)
{
	edict_t* me = GetEntity();

	m_enemyDistance = FLT_MAX;
	m_friendDistance = FLT_MAX;
	m_enemiesNearCount = 0;
	m_friendsNearCount = 0;
	m_numEnemiesLeft = 0;
	m_numFriendsLeft = 0;
	m_hasEnemiesNear = false;
	m_hasFriendsNear = false;

	TraceResult tr{};
	const Vector myOrigin = pev->origin + pev->view_ofs;
	for (const auto& client : g_clients)
	{
		if (FNullEnt(client.ent))
			continue;

		if (client.ent == me)
			continue;

		if (!IsAlive(client.ent))
			continue;

		const Vector headOrigin = client.ent->v.origin + client.ent->v.view_ofs;
		if (client.team == m_team)
		{
			m_numFriendsLeft++;

			// simple check
			TraceLine(myOrigin, headOrigin, true, true, me, &tr);
			if (tr.flFraction != 1.0f)
				continue;

			m_friendsNearCount++;
			const float distance = (myOrigin - headOrigin).GetLengthSquared();
			if (distance < m_friendDistance)
			{
				m_friendDistance = distance;
				m_nearestFriend = client.ent;
			}
		}
		else
		{
			m_numEnemiesLeft++;

			if (!CheckVisibility(client.ent))
				continue;

			// we don't know this enemy, where it can be?
			if (client.ent != m_nearestEnemy)
			{
				if (!IsZombieMode())
				{
					if (GetCurrentState() != Process::Camp && !IsAttacking(client.ent) && !IsInViewCone(headOrigin))
						continue;

					if (IsBehindSmokeClouds(client.ent))
						continue;
				}

				if (IsNotAttackLab(client.ent))
					continue;
			}

			m_enemiesNearCount++;
			const float distance = (myOrigin - headOrigin).GetLengthSquared();
			if (distance < m_enemyDistance)
			{
				m_enemyDistance = distance;
				m_nearestEnemy = client.ent;
			}
		}
	}

	m_hasEnemiesNear = m_enemiesNearCount > 0;
	m_hasFriendsNear = m_friendsNearCount > 0;

	// enemyOrigin will be added with IsEnemyViewable function unlike friendOrigin
	if (m_hasEnemiesNear)
		m_enemySeeTime = engine->GetTime();

	if (m_hasFriendsNear)
	{
		m_friendOrigin = m_nearestFriend->v.origin;
		m_friendSeeTime = engine->GetTime();
	}
}

void Bot::FindEnemyEntities(void)
{
	m_entityDistance = FLT_MAX;
	m_entitiesNearCount = 0;
	m_numEntitiesLeft = 0;
	m_hasEntitiesNear = false;

	int i;
	for (i = 0; i < entityNum; i++)
	{
		if (g_entityId[i] == -1 || g_entityAction[i] != 1 || m_team == g_entityTeam[i])
			continue;

		edict_t* entity = INDEXENT(g_entityId[i]);
		if (FNullEnt(entity) || !IsAlive(entity) || entity->v.effects & EF_NODRAW || entity->v.takedamage == DAMAGE_NO)
			continue;

		m_numEntitiesLeft++;

		// simple check
		TraceResult tr{};
		const Vector origin = GetEntityOrigin(entity);
		TraceLine(pev->origin, origin, true, true, pev->pContainingEntity, &tr);
		if (tr.flFraction != 1.0f)
			continue;

		m_entitiesNearCount++;
		const float distance = (pev->origin - origin).GetLengthSquared();
		if (distance < m_entityDistance)
		{
			m_entityDistance = distance;
			m_nearestEntity = entity;
		}
	}

	m_hasEntitiesNear = m_entitiesNearCount > 0;
	if (m_hasEntitiesNear)
	{
		m_entityOrigin = GetEntityOrigin(m_nearestEntity);
		m_entitySeeTime = engine->GetTime();
	}
}

Vector Bot::GetEnemyPosition(void)
{
	// not even visible?
	if (m_visibility == Visibility::None)
		return m_enemyOrigin;

	// return last position
	if (FNullEnt(m_nearestEnemy))
		return m_enemyOrigin;

	// get enemy position initially
	Vector targetOrigin = m_nearestEnemy->v.origin;
	const float distance = (targetOrigin - pev->origin).GetLengthSquared();

	// now take in account different parts of enemy body
	if (m_visibility & (Visibility::Head | Visibility::Body)) // visible head & body
	{
		if (!IsZombieMode() && m_currentWeapon == Weapon::Awp)
			return targetOrigin;
		// now check is our skill match to aim at head, else aim at enemy body
		else if (IsZombieMode() || chanceof(m_skill) || UsesPistol())
			targetOrigin += m_nearestEnemy->v.view_ofs + Vector(0.0f, 0.0f, GetZOffset(distance));
		else
			targetOrigin += Vector(0.0f, 0.0f, GetZOffset(distance));
	}
	else if (m_visibility & Visibility::Head) // visible only head
		targetOrigin += m_nearestEnemy->v.view_ofs + Vector(0.0f, 0.0f, GetZOffset(distance));
	else if (m_visibility & Visibility::Body) // visible only body
		targetOrigin += Vector(0.0f, 0.0f, GetZOffset(distance));
	else if (m_visibility & Visibility::Other) // random part of body is visible
		targetOrigin = m_enemyOrigin;

	return targetOrigin;
}

float Bot::GetZOffset(const float distance)
{
	bool sniper = UsesSniper();
	bool pistol = UsesPistol();
	bool rifle = UsesRifle();

	bool zoomableRifle = UsesZoomableRifle();
	bool submachine = UsesSubmachineGun();
	bool shotgun = (m_currentWeapon == Weapon::Xm1014 || m_currentWeapon == Weapon::M3);
	bool m249 = m_currentWeapon == Weapon::M249;

	float result = -2.0f;

	if (distance > squaredf(512.0f))
	{
		if (sniper)
			result = 0.18f;
		else if (zoomableRifle)
			result = 1.5f;
		else if (pistol)
			result = 2.5f;
		else if (submachine)
			result = 1.5f;
		else if (rifle)
			result = 1.0f;
		else if (m249)
			result = -5.5f;
		else if (shotgun)
			result = -4.5f;
	}
	else
		return -9.0f;

	return result;
}

// bot can't hurt teammates, if friendly fire is not enabled...
bool Bot::IsFriendInLineOfFire(const float distance)
{
	if (g_gameVersion & Game::HalfLife || !engine->IsFriendlyFireOn() || GetGameMode() == GameMode::Deathmatch || GetGameMode() == GameMode::NoTeam)
		return false;

	MakeVectors(pev->v_angle);

	TraceResult tr{};
	TraceLine(EyePosition(), EyePosition() + distance * pev->v_angle.Normalize(), false, false, GetEntity(), &tr);

	if (!FNullEnt(tr.pHit))
	{
		if (IsAlive(tr.pHit) && m_team == GetTeam(tr.pHit))
		{
			if (IsValidPlayer(tr.pHit))
				return true;

			const int entityIndex = ENTINDEX(tr.pHit);
			int i;
			for (i = 0; i < entityNum; i++)
			{
				if (g_entityId[i] == -1 || g_entityAction[i] != 1)
					continue;

				if (g_entityId[i] == entityIndex)
					return true;
			}
		}
	}

	for (const auto& client : g_clients)
	{
		if (FNullEnt(client.ent))
			continue;

		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == GetEntity())
			continue;

		const Vector origin = client.ent->v.origin;
		const float friendDistance = (origin - pev->origin).GetLengthSquared();

		if (friendDistance <= distance && GetShootingConeDeviation(GetEntity(), &origin) > (friendDistance / (friendDistance + 1089.0f)))
			return true;
	}

	return false;
}

bool Bot::DoFirePause(const float distance)
{
	if (m_firePause > engine->GetTime())
		return true;

	if (m_hasEnemiesNear && IsEnemyProtectedByShield(m_nearestEnemy))
		return true;

	const float angle = (cabsf(pev->punchangle.y) + cabsf(pev->punchangle.x)) * (Math::MATH_PI * 0.00277777777f);

	// check if we need to compensate recoil
	if (ctanf(angle) * (distance + (distance * 0.25f)) > 100.0f)
	{
		const float time = engine->GetTime();
		if (m_firePause < (time - 0.4f))
			m_firePause = time + crandomfloat(0.4f, (0.4f + 1.2f * ((100 - m_skill)) * 0.01f));

		return true;
	}

	if (UsesSniper())
	{
		if (!(m_currentTravelFlags & PATHFLAG_JUMP))
			pev->button &= ~IN_JUMP;

		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;

		if (pev->speed >= pev->maxspeed && !IsZombieMode())
		{
			m_firePause = engine->GetTime() + 0.1f;
			return true;
		}
	}

	return false;
}

// this function will return true if weapon was fired, false otherwise
void Bot::FireWeapon(void)
{
	// try to switch
	if (m_currentWeapon == Weapon::Knife)
		SelectBestWeapon(true);

	const float distance = GetTargetDistance();

	// or if friend in line of fire, stop this too but do not update shoot time
	if (IsFriendInLineOfFire(distance))
		return;

	const int melee = (g_gameVersion & Game::HalfLife) ? WeaponHL::Crowbar : Weapon::Knife;

	FireDelay* delay = &g_fireDelay[0];
	WeaponSelect* selectTab = (g_gameVersion & Game::HalfLife) ? &g_weaponSelectHL[0] : &g_weaponSelect[0];

	edict_t* enemy = m_entityDistance < m_enemyDistance ? m_nearestEntity : m_nearestEnemy;

	int selectId = melee, selectIndex = 0, chosenWeaponIndex = 0;
	int weapons = pev->weapons;

	const float time = engine->GetTime();

	if (ebot_knifemode.GetBool())
		goto WeaponSelectEnd;
	else if (!FNullEnt(enemy) && chanceof(m_skill) && !IsZombieEntity(enemy) && distance < squaredf(128.0f) && (enemy->v.health < 30 || pev->health > enemy->v.health) && !IsOnLadder() && m_enemiesNearCount < 2)
		goto WeaponSelectEnd;

	// loop through all the weapons until terminator is found...
	while (selectTab[selectIndex].id)
	{
		// is the bot carrying this weapon?
		if (weapons & (1 << selectTab[selectIndex].id))
		{
			const int id = selectTab[selectIndex].id;

			// cannot be used in water...
			if (pev->waterlevel == 3 && g_weaponDefs[id].flags & ITEM_FLAG_NOFIREUNDERWATER)
				continue;

			// is enough ammo available to fire AND check is better to use pistol in our current situation...
			if (g_gameVersion & Game::HalfLife)
			{
				if (selectIndex == WeaponHL::Snark || selectIndex == WeaponHL::Gauss ||selectIndex == WeaponHL::Egon || (selectIndex == WeaponHL::HandGrenade && distance > squaredf(384.0f) && distance < squaredf(768.0f)) || (selectIndex == WeaponHL::Rpg && distance > squaredf(320.0f)) || (selectIndex == WeaponHL::Crossbow && distance > squaredf(320.0f)))
					chosenWeaponIndex = selectIndex;
				else if (selectIndex != WeaponHL::HandGrenade && selectIndex != WeaponHL::Rpg  && selectIndex != WeaponHL::Crossbow && (m_ammoInClip[id] > 0) && !IsWeaponBadInDistance(selectIndex, distance))
					chosenWeaponIndex = selectIndex;
			}
			else if (m_ammoInClip[id] > 0 && !IsWeaponBadInDistance(selectIndex, distance))
				chosenWeaponIndex = selectIndex;
		}

		selectIndex++;
	}

	selectId = selectTab[chosenWeaponIndex].id;

	// if no available weapon...
	if (chosenWeaponIndex == 0)
	{
		selectIndex = 0;

		// loop through all the weapons until terminator is found...
		while (selectTab[selectIndex].id)
		{
			const int id = selectTab[selectIndex].id;

			// cannot be used in water...
			if (pev->waterlevel == 3 && g_weaponDefs[id].flags & ITEM_FLAG_NOFIREUNDERWATER)
				continue;

			// is the bot carrying this weapon?
			if (weapons & (1 << id))
			{
				if (g_weaponDefs[id].ammo1 != -1 && m_ammo[g_weaponDefs[id].ammo1] >= selectTab[selectIndex].minPrimaryAmmo)
				{
					// available ammo found, reload weapon
					if (m_reloadState == ReloadState::Nothing || m_reloadCheckTime > time)
					{
						m_isReloading = true;
						m_reloadState = ReloadState::Primary;
						m_reloadCheckTime = time;
						RadioMessage(Radio::NeedBackup);
					}

					return;
				}
			}

			selectIndex++;
		}

		selectId = melee; // no available ammo, use knife!
	}

WeaponSelectEnd:
	// we want to fire weapon, don't reload now
	if (!m_isReloading)
	{
		m_reloadState = ReloadState::Nothing;
		m_reloadCheckTime = time + 6.0f;
	}

	if (IsZombieMode() && m_currentWeapon == melee && selectId != melee && !m_isZombieBot)
	{
		m_reloadState = ReloadState::Primary;
		m_reloadCheckTime = time + 2.5f;
		return;
	}

	if (m_currentWeapon != selectId)
	{
		SelectWeaponByName(g_weaponDefs[selectId].className);

		// reset burst fire variables
		m_firePause = 0.0f;

		return;
	}

	if (selectTab[chosenWeaponIndex].id != selectId)
	{
		chosenWeaponIndex = 0;

		// loop through all the weapons until terminator is found...
		while (selectTab[chosenWeaponIndex].id)
		{
			if (selectTab[chosenWeaponIndex].id == selectId)
				break;

			chosenWeaponIndex++;
		}
	}

	// if we're have a glock or famas vary burst fire mode
	if (!(g_gameVersion & Game::HalfLife))
	{
		CheckBurstMode(distance);

		if (HasShield() && m_shieldCheckTime < time && GetCurrentState() != Process::Camp) // better shield gun usage
		{
			if ((distance > squaredf(768.0f)) && !IsShieldDrawn())
				pev->button |= IN_ATTACK2; // draw the shield
			else if (IsShieldDrawn() || (IsValidPlayer(enemy) && enemy->v.button & IN_RELOAD))
				pev->button |= IN_ATTACK2; // draw out the shield

			m_shieldCheckTime = time + 0.5f;
		}
	}

	if (UsesSniper() && m_zoomCheckTime < time) // is the bot holding a sniper rifle?
	{
		if (distance > squaredf(1500.0f) && pev->fov >= 40.0f) // should the bot switch to the long-range zoom?
			pev->button |= IN_ATTACK2;

		else if (distance > squaredf(150.0f) && pev->fov >= 90.0f) // else should the bot switch to the close-range zoom ?
			pev->button |= IN_ATTACK2;

		else if (distance < squaredf(150.0f) && pev->fov < 90.0f) // else should the bot restore the normal view ?
			pev->button |= IN_ATTACK2;

		m_zoomCheckTime = time;
	}
	else if (UsesZoomableRifle() && m_zoomCheckTime < time && m_skill < 90) // else is the bot holding a zoomable rifle?
	{
		if (distance > squaredf(800.0f) && pev->fov >= 90.0f) // should the bot switch to zoomed mode?
			pev->button |= IN_ATTACK2;

		else if (distance < squaredf(800.0f) && pev->fov < 90.0f) // else should the bot restore the normal view?
			pev->button |= IN_ATTACK2;

		m_zoomCheckTime = time;
	}

	// need to care for burst fire?
	if (g_gameVersion & Game::HalfLife || distance < squaredf(512.0f))
	{
		if (selectId == melee)
			KnifeAttack();
		else
		{
			if (selectTab[chosenWeaponIndex].primaryFireHold) // if automatic weapon, just press attack
				pev->button |= IN_ATTACK;
			else // if not, toggle buttons
			{
				if (!(pev->oldbuttons & IN_ATTACK))
					pev->button |= IN_ATTACK;
			}
		}
	}
	else
	{
		// don't attack with knife over long distance
		if (selectId == melee)
		{
			KnifeAttack();
			return;
		}

		if (DoFirePause(distance))
			return;

		pev->button |= IN_ATTACK;
	}
}

bool Bot::KnifeAttack(const float attackDistance)
{
	if (g_gameVersion & Game::HalfLife)
	{
		pev->button |= IN_ATTACK;
		return true;
	}

	Vector origin = nullvec;
	float distance = FLT_MAX;

	if (m_enemyDistance <= m_entityDistance)
	{
		origin = m_enemyOrigin;
		distance = m_enemyDistance;
	}
	else
	{
		origin = m_entityOrigin;
		distance = m_entityDistance;
	}

	float kad1 = squaredf(pev->speed * 0.26);
	const float kad2 = squaredf(64.0f);

	if (attackDistance != 0.0f)
		kad1 = attackDistance;

	int kaMode = 0;

	if (distance < kad1)
		kaMode = 1;

	if (distance < kad2)
		kaMode += 2;

	if (kaMode > 0)
	{
		const float distanceSkipZ = (pev->origin - origin).GetLengthSquared2D();

		if (pev->origin.z > origin.z && distanceSkipZ < squaredf(64.0f))
		{
			pev->button |= IN_DUCK;
			pev->button &= ~IN_JUMP;
		}
		else
		{
			pev->button &= ~IN_DUCK;

			if (pev->origin.z + 150.0f < origin.z && distanceSkipZ < squaredf(300.0f))
				pev->button |= IN_JUMP;
		}

		if (m_isZombieBot)
		{
			if (kaMode != 2)
				pev->button |= IN_ATTACK;
			else
				pev->button |= IN_ATTACK2;
		}
		else
		{
			if (kaMode == 1)
				pev->button |= IN_ATTACK;
			else if (kaMode == 2)
				pev->button |= IN_ATTACK2;
			else if (crandomint(1, 10) < 3 || HasShield())
				pev->button |= IN_ATTACK;
			else
				pev->button |= IN_ATTACK2;
		}

		return true;
	}

	return false;
}

// this function checks, is it better to use pistol instead of current primary weapon
// to attack our enemy, since current weapon is not very good in this situation
bool Bot::IsWeaponBadInDistance(const int weaponIndex, const float distance)
{
	if (g_gameVersion & Game::HalfLife)
	{
		const int weaponID = g_weaponSelectHL[weaponIndex].id;
		if (weaponID == WeaponHL::Crowbar)
			return false;

		// shotgun is too inaccurate at long distances, so weapon is bad
		if (weaponID == WeaponHL::Shotgun && distance > squaredf(768.0f))
			return true;
	}
	else
	{
		const int weaponID = g_weaponSelect[weaponIndex].id;
		if (weaponID == Weapon::Knife)
			return false;

		// shotguns is too inaccurate at long distances, so weapon is bad
		if ((weaponID == Weapon::M3 || weaponID == Weapon::Xm1014) && distance > squaredf(768.0f))
			return true;

		if (!IsZombieMode())
		{
			if ((weaponID == Weapon::Scout || weaponID == Weapon::Awp || weaponID == Weapon::G3SG1 || weaponID == Weapon::Sg550) && distance <= squaredf(512.0f))
				return true;
		}
	}

	// check is ammo available for secondary weapon
	if (m_ammoInClip[g_weaponSelect[GetBestSecondaryWeaponCarried()].id] >= 1)
		return false;

	return false;
}

// this function returns returns true, if bot has a primary weapon
bool Bot::HasPrimaryWeapon(void)
{
	return (pev->weapons & WeaponBits_Primary) != 0;
}

bool Bot::HasSecondaryWeapon(void)
{
	return (pev->weapons & WeaponBits_SecondaryNODEFAULT) != 0;
}

// this function returns true, if bot has a tactical shield
bool Bot::HasShield(void)
{
	return cstrncmp(STRING(pev->viewmodel), "models/shield/v_shield_", 23) == 0;
}

// this function returns true, is the tactical shield is drawn
bool Bot::IsShieldDrawn(void)
{
	if (g_gameVersion & Game::HalfLife)
		return false;

	if (!HasShield())
		return false;

	return pev->weaponanim == 6 || pev->weaponanim == 7;
}

// this function returns true, if enemy protected by the shield
bool Bot::IsEnemyProtectedByShield(edict_t* enemy)
{
	if (g_gameVersion & Game::HalfLife)
		return false;

	if (FNullEnt(enemy))
		return false;

	if (IsShieldDrawn())
		return false;

	// check if enemy has shield and this shield is drawn
	if (cstrncmp(STRING(enemy->v.viewmodel), "models/shield/v_shield_", 23) == 0 && (enemy->v.weaponanim == 6 || enemy->v.weaponanim == 7))
	{
		if (::IsInViewCone(pev->origin, enemy))
			return true;
	}

	return false;
}

bool Bot::UsesShotgun(void)
{
	if (g_gameVersion & Game::HalfLife)
		return m_currentWeapon == WeaponHL::Shotgun;

	return m_currentWeapon == Weapon::M3 || m_currentWeapon == Weapon::Xm1014;
}

bool Bot::UsesSniper(void)
{
	if (g_gameVersion & Game::HalfLife)
		return m_currentWeapon == WeaponHL::Crossbow;

	return m_currentWeapon == Weapon::Awp || m_currentWeapon == Weapon::G3SG1 || m_currentWeapon == Weapon::Scout || m_currentWeapon == Weapon::Sg550;
}

bool Bot::IsSniper(void)
{
	if (g_gameVersion & Game::HalfLife)
	{
		if (pev->weapons & (1 << WeaponHL::Crossbow))
			return true;

		return false;
	}

	if (pev->weapons & (1 << Weapon::Awp))
		return true;
	else if (pev->weapons & (1 << Weapon::G3SG1))
		return true;
	else if (pev->weapons & (1 << Weapon::Scout))
		return true;
	else if (pev->weapons & (1 << Weapon::Sg550))
		return true;

	return false;
}

bool Bot::UsesRifle(void)
{
	if (g_gameVersion & Game::HalfLife)
		return m_currentWeapon == WeaponHL::Mp5_HL;

	WeaponSelect* selectTab = &g_weaponSelect[0];
	int count = 0;

	while (selectTab->id)
	{
		if (m_currentWeapon == selectTab->id)
			break;

		selectTab++;
		count++;
	}

	if (selectTab->id && count > 13)
		return true;

	return false;
}

bool Bot::UsesPistol(void)
{
	if (g_gameVersion & Game::HalfLife)
		return m_currentWeapon == WeaponHL::Glock || m_currentWeapon == WeaponHL::Python;

	WeaponSelect* selectTab = &g_weaponSelect[0];
	int count = 0;

	// loop through all the weapons until terminator is found
	while (selectTab->id)
	{
		if (m_currentWeapon == selectTab->id)
			break;

		selectTab++;
		count++;
	}

	if (selectTab->id && count < 7)
		return true;

	return false;
}

bool Bot::UsesSubmachineGun(void)
{
	if (g_gameVersion & Game::HalfLife)
		return m_currentWeapon == WeaponHL::Egon;

	return m_currentWeapon == Weapon::Mp5 || m_currentWeapon == Weapon::Tmp || m_currentWeapon == Weapon::P90 || m_currentWeapon == Weapon::Mac10 || m_currentWeapon == Weapon::Ump45;
}

bool Bot::UsesZoomableRifle(void)
{
	if (g_gameVersion & Game::HalfLife)
		return false;

	return m_currentWeapon == Weapon::Aug || m_currentWeapon == Weapon::Sg552;
}

bool Bot::UsesBadPrimary(void)
{
	if (g_gameVersion & Game::HalfLife)
		return m_currentWeapon == WeaponHL::HornetGun;

	return m_currentWeapon == Weapon::M3 || m_currentWeapon == Weapon::Ump45 || m_currentWeapon == Weapon::Mac10 || m_currentWeapon == Weapon::Tmp || m_currentWeapon == Weapon::P90;
}

int Bot::CheckGrenades(void)
{
	if (pev->weapons & (1 << Weapon::HeGrenade))
		return Weapon::HeGrenade;
	else if (pev->weapons & (1 << Weapon::FbGrenade))
		return Weapon::FbGrenade;
	else if (pev->weapons & (1 << Weapon::SmGrenade))
		return Weapon::SmGrenade;
	return -1;
}

void Bot::SelectKnife(void)
{
	if (g_gameVersion & Game::HalfLife)
	{
		if (m_currentWeapon == WeaponHL::Crowbar)
			return;

		if (m_weaponSelectDelay > engine->GetTime())
			return;

		SelectWeaponByName("weapon_crowbar");
	}
	else
	{
		// already have
		if (m_currentWeapon == Weapon::Knife)
			return;

		if (m_isBomber)
			return;

		const float time = engine->GetTime();
		if (m_walkTime > time)
			return;

		if (m_weaponSelectDelay > time)
			return;

		SelectWeaponByName("weapon_knife");
	}
}

void Bot::SelectBestWeapon(const bool force, const bool getHighest)
{
	const float time = engine->GetTime();
	if (!force)
	{
		if (m_weaponSelectDelay > time)
			return;

		if (!m_hasEnemiesNear && !m_hasEntitiesNear && (m_spawnTime + engine->GetFreezeTime() + 7.0f) > time)
			return;
	}

	WeaponSelect* selectTab = g_gameVersion & Game::HalfLife ? &g_weaponSelectHL[0] : &g_weaponSelect[0];

	int selectIndex = -1;
	int chosenWeaponIndex = -1;

	if (!getHighest)
	{
		while (selectTab[selectIndex].id)
		{
			if (!(pev->weapons & (1 << selectTab[selectIndex].id)))
			{
				selectIndex++;
				continue;
			}

			const int id = selectTab[selectIndex].id;

			// cannot be used in water...
			if (pev->waterlevel == 3 && g_weaponDefs[id].flags & ITEM_FLAG_NOFIREUNDERWATER)
				continue;

			if (g_gameVersion & Game::HalfLife)
				chosenWeaponIndex = selectIndex;
			else
			{
				bool ammoLeft = false;

				if (id == m_currentWeapon)
				{
					if (GetAmmoInClip() > 0)
						ammoLeft = true;
				}
				else if (id < 33 && id > -1 && g_weaponDefs != nullptr && m_ammo != nullptr && m_ammo[g_weaponDefs[id].ammo1] > 0)
					ammoLeft = true;

				if (ammoLeft)
					chosenWeaponIndex = selectIndex;
			}

			selectIndex++;
		}
	}

	if (chosenWeaponIndex == -1)
		chosenWeaponIndex = GetHighestWeapon();

	chosenWeaponIndex %= Const_NumWeapons + 1;
	selectIndex = chosenWeaponIndex;

	if (m_currentWeapon != selectTab[selectIndex].id)
	{
		SelectWeaponByName(selectTab[selectIndex].weaponName);
		m_isReloading = false;
		m_reloadState = ReloadState::Nothing;
		m_weaponSelectDelay = time + engine->GetTime();
	}
}

void Bot::SelectPistol(void)
{
	if (!m_isSlowThink)
		return;

	if (m_isReloading)
		return;

	const int oldWeapons = pev->weapons;
	pev->weapons &= ~WeaponBits_Primary;
	SelectBestWeapon(false, true);

	pev->weapons = oldWeapons;
}

int Bot::GetHighestWeapon(void)
{
	WeaponSelect* selectTab = (g_gameVersion & Game::HalfLife) ? &g_weaponSelectHL[0] : &g_weaponSelect[0];

	const int weapons = pev->weapons;
	int num = 0;
	int i = 0;

	// loop through all the weapons until terminator is found...
	while (selectTab->id)
	{
		// cannot be used in water...
		if (pev->waterlevel == 3 && g_weaponDefs[selectTab->id].flags & ITEM_FLAG_NOFIREUNDERWATER)
			continue;

		// is the bot carrying this weapon?
		if (weapons & (1 << selectTab->id))
			num = i;

		i++;
		selectTab++;
	}

	return num;
}

void Bot::SelectWeaponByName(const char* name)
{
	FakeClientCommand(GetEntity(), name);
}

void Bot::SelectWeaponbyNumber(const int num)
{
	FakeClientCommand(GetEntity(), g_weaponSelect[num].weaponName);
}

void Bot::CheckReload(void)
{
	if (m_isZombieBot)
	{
		m_reloadState = ReloadState::Nothing;
		m_isReloading = false;
		return;
	}

	// do not check for reload
	if (m_currentWeapon == ((g_gameVersion & Game::HalfLife) ? WeaponHL::Crowbar : Weapon::Knife))
	{
		m_reloadState = ReloadState::Nothing;
		m_isReloading = false;
		return;
	}

	const float time = engine->GetTime();
	m_isReloading = false; // update reloading status
	m_reloadCheckTime = time + 3.0f;

	if (m_reloadState != ReloadState::Nothing)
	{
		int weapons = pev->weapons;

		if (m_reloadState == ReloadState::Primary)
			weapons &= WeaponBits_Primary;
		else if (m_reloadState == ReloadState::Secondary)
			weapons &= WeaponBits_Secondary;

		if (weapons == 0)
		{
			m_reloadState++;

			if (m_reloadState > ReloadState::Secondary)
				m_reloadState = ReloadState::Nothing;

			return;
		}

		int weaponIndex = -1;
		const int maxClip = CheckMaxClip(weapons, &weaponIndex);

		if (m_ammoInClip[weaponIndex] < maxClip && g_weaponDefs[weaponIndex].ammo1 != -1 && g_weaponDefs[weaponIndex].ammo1 < 32 && m_ammo[g_weaponDefs[weaponIndex].ammo1] > 0)
		{
			if (m_currentWeapon != weaponIndex)
				SelectWeaponByName(g_weaponDefs[weaponIndex].className);

			pev->button &= ~IN_ATTACK;

			if ((pev->oldbuttons & IN_RELOAD) == ReloadState::Nothing)
				pev->button |= IN_RELOAD; // press reload button

			m_isReloading = true;
		}
		else
		{
			if (m_seeEnemyTime + 5.0f > time || m_entitySeeTime + 5.0f > time)
			{
				m_reloadState = ReloadState::Nothing;
				return;
			}

			m_reloadState++;
			if (m_reloadState > ReloadState::Secondary)
				m_reloadState = ReloadState::Nothing;
		}
	}
}

int Bot::CheckMaxClip(const int weaponId, int* weaponIndex)
{
	int maxClip = -1;
	for (int i = 1; i < Const_MaxWeapons; i++)
	{
		if (weaponId & (1 << i))
		{
			*weaponIndex = i;
			break;
		}
	}

	switch (*weaponIndex)
	{
	case Weapon::M249:
		maxClip = 100;
		break;

	case Weapon::P90:
		maxClip = 50;
		break;

	case Weapon::Galil:
		maxClip = 35;
		break;

	case Weapon::Elite:
	case Weapon::Mp5:
	case Weapon::Tmp:
	case Weapon::Mac10:
	case Weapon::M4A1:
	case Weapon::Ak47:
	case Weapon::Sg552:
	case Weapon::Aug:
	case Weapon::Sg550:
		maxClip = 30;
		break;

	case Weapon::Ump45:
	case Weapon::Famas:
		maxClip = 25;
		break;

	case Weapon::Glock18:
	case Weapon::FiveSeven:
	case Weapon::G3SG1:
		maxClip = 20;
		break;

	case Weapon::P228:
		maxClip = 13;
		break;

	case Weapon::Usp:
		maxClip = 12;
		break;

	case Weapon::Awp:
	case Weapon::Scout:
		maxClip = 10;
		break;

	case Weapon::M3:
		maxClip = 8;
		break;

	case Weapon::Deagle:
	case Weapon::Xm1014:
		maxClip = 7;
		break;
	}

	return maxClip;
}