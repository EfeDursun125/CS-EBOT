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

int Bot::GetNearbyFriendsNearPosition(const Vector& origin, const float radius)
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

int Bot::GetNearbyEnemiesNearPosition(const Vector& origin, const float radius)
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
	m_enemyDistance = FLT_MAX;
	m_friendDistance = FLT_MAX;
	m_enemiesNearCount = 0;
	m_friendsNearCount = 0;
	m_numEnemiesLeft = 0;
	m_numFriendsLeft = 0;

	float distance;
	TraceResult tr{};
	Vector headOrigin;
	const Vector myOrigin = pev->origin + pev->view_ofs;

	if (m_isZombieBot)
	{
		const bool needTarget = (!IsAlive(m_nearestEnemy) || GetTeam(m_nearestEnemy) == m_team);
		const float maxRange = squaredf(512.0f);
		for (const auto& client : g_clients)
		{
			if (client.ent == pev->pContainingEntity)
				continue;

			if (!IsAlive(client.ent))
				continue;

			if (client.ent->v.flags & FL_NOTARGET)
				continue;

			headOrigin = client.ent->v.origin + client.ent->v.view_ofs;
			if (client.team == m_team)
			{
				m_numFriendsLeft++;

				// simple check
				TraceLine(myOrigin, headOrigin, true, true, pev->pContainingEntity, &tr);
				if (tr.flFraction != 1.0f)
					continue;

				m_friendsNearCount++;
				distance = (myOrigin - headOrigin).GetLengthSquared();
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

				distance = (myOrigin - headOrigin).GetLengthSquared();
				if (!needTarget)
				{
					if (IsNotAttackLab(client.ent))
						continue;

					if (distance > maxRange && !IsInViewCone(headOrigin))
						continue;
				}

				m_enemiesNearCount++;
				if (distance < m_enemyDistance)
				{
					m_enemyDistance = distance;
					m_nearestEnemy = client.ent;
				}
			}
		}
	}
	else
	{
		for (const auto& client : g_clients)
		{
			if (client.ent == pev->pContainingEntity)
				continue;

			if (!IsAlive(client.ent))
				continue;

			if (client.ent->v.flags & FL_NOTARGET)
				continue;

			headOrigin = client.ent->v.origin + client.ent->v.view_ofs;
			if (client.team == m_team)
			{
				m_numFriendsLeft++;

				// simple check
				TraceLine(myOrigin, headOrigin, true, true, pev->pContainingEntity, &tr);
				if (tr.flFraction != 1.0f)
					continue;

				m_friendsNearCount++;
				distance = (myOrigin - headOrigin).GetLengthSquared();
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
					if (IsNotAttackLab(client.ent))
						continue;

					if (!IsZombieMode())
					{
						if (GetCurrentState() != Process::Camp && !IsAttacking(client.ent) && !IsInViewCone(headOrigin))
							continue;

						if (IsBehindSmokeClouds(client.ent))
							continue;
					}
				}

				m_enemiesNearCount++;
				distance = (myOrigin - headOrigin).GetLengthSquared();
				if (distance < m_enemyDistance)
				{
					m_enemyDistance = distance;
					m_nearestEnemy = client.ent;
				}
			}
		}
	}

	m_hasEnemiesNear = m_enemiesNearCount;
	m_hasFriendsNear = m_friendsNearCount;

	// enemyOrigin will be added with IsEnemyViewable function unlike friendOrigin
	if (m_hasEnemiesNear)
		m_enemySeeTime = engine->GetTime();
	else
		m_isEnemyReachable = false;

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
	Vector origin;
	float distance;
	edict_t* entity;
	TraceResult tr{};

	for (i = 0; i < entityNum; i++)
	{
		if (g_entityId[i] == -1 || g_entityAction[i] != 1 || m_team == g_entityTeam[i])
			continue;

		entity = INDEXENT(g_entityId[i]);
		if (!IsAlive(entity) || entity->v.flags & FL_NOTARGET || entity->v.effects & EF_NODRAW || entity->v.takedamage == DAMAGE_NO)
			continue;

		m_numEntitiesLeft++;

		// simple check
		origin = GetEntityOrigin(entity);
		TraceLine(pev->origin, origin, true, true, pev->pContainingEntity, &tr);
		if (tr.flFraction != 1.0f)
			continue;

		m_entitiesNearCount++;
		distance = (pev->origin - origin).GetLengthSquared();
		if (distance < m_entityDistance)
		{
			m_entityDistance = distance;
			m_nearestEntity = entity;
		}
	}

	m_hasEntitiesNear = m_entitiesNearCount;
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

	// npc/entity
	if (!IsValidPlayer(m_nearestEnemy))
		return m_enemyOrigin;

	// its too hard to aim head while climbing the ladder at top speed
	// just try to aim center this is easier for the bot
	if (IsOnLadder())
		return m_nearestEnemy->v.origin;

	// get enemy position initially
	Vector targetOrigin = m_nearestEnemy->v.origin;

	// now take in account different parts of enemy body
	if (m_visibility & (Visibility::Head | Visibility::Body)) // visible head & body
	{
		// aim to body with awp, because it deals to much damage
		if (!IsZombieMode() && m_currentWeapon == Weapon::Awp)
			return targetOrigin;
		// now check is our skill match to aim at head, else aim at enemy body
		else if (IsZombieMode() || chanceof(m_skill) || UsesPistol())
			targetOrigin += m_nearestEnemy->v.view_ofs + Vector(0.0f, 0.0f, GetZOffset((targetOrigin - pev->origin).GetLengthSquared()));
		else
			targetOrigin += Vector(0.0f, 0.0f, GetZOffset((targetOrigin - pev->origin).GetLengthSquared()));
	}
	else if (m_visibility & Visibility::Head) // visible only head
		targetOrigin += m_nearestEnemy->v.view_ofs + Vector(0.0f, 0.0f, GetZOffset((targetOrigin - pev->origin).GetLengthSquared()));
	else if (m_visibility & Visibility::Body) // visible only body
		targetOrigin += Vector(0.0f, 0.0f, GetZOffset((targetOrigin - pev->origin).GetLengthSquared()));
	else // random part of body is visible
		targetOrigin = m_enemyOrigin;

	return m_enemyOrigin = targetOrigin;
}

float Bot::GetZOffset(const float distance)
{
	if (m_skill < 30)
		return 0.0f;

	if (distance > squaredf(2800.0f))
		return 3.5f;

	if (distance > squaredf(600.0f))
	{
		if (UsesSniper())
			return 3.5f;
		else if (UsesZoomableRifle())
			return 4.5f;
		else if (UsesPistol())
			return 6.5f;
		else if (UsesSubmachineGun())
			return 5.5f;
		else if (UsesRifle())
			return 5.5f;
		else if (m_currentWeapon == Weapon::M249)
			return 2.5f;
		else if (UsesShotgun())
			return 10.5f;
	}
	else if (distance > squaredf(300.0f))
	{
		if (UsesSniper())
			return 3.5f;
		else if (UsesZoomableRifle())
			return 3.5f;
		else if (UsesPistol())
			return 6.5f;
		else if (UsesSubmachineGun())
			return 3.5f;
		else if (UsesRifle())
			return 1.6f;
		else if (m_currentWeapon == Weapon::M249)
			return -1.0f;
		else if (UsesShotgun())
			return 10.0f;
	}
	else
	{
		if (UsesSniper())
			return 4.5f;
		else if (UsesZoomableRifle())
			return -5.0f;
		else if (UsesPistol())
			return 4.5f;
		else if (UsesSubmachineGun())
			return -4.5f;
		else if (UsesRifle())
			return -4.5f;
		else if (m_currentWeapon == Weapon::M249)
			return -6.0f;
		else if (UsesShotgun())
			return -5.0f;
	}

	return 3.5f;
}

// bot can't hurt teammates, if friendly fire is not enabled...
bool Bot::IsFriendInLineOfFire(const float distance)
{
	if (g_gameVersion & Game::HalfLife || !engine->IsFriendlyFireOn() || GetGameMode() == GameMode::Deathmatch || GetGameMode() == GameMode::NoTeam)
		return false;

	MakeVectors(pev->v_angle);

	TraceResult tr{};
	Vector origin = EyePosition();
	TraceLine(origin, origin + distance * pev->v_angle.Normalize(), false, false, pev->pContainingEntity, &tr);

	if (IsAlive(tr.pHit) && m_team == GetTeam(tr.pHit))
	{
		if (IsValidPlayer(tr.pHit))
			return true;

		int i;
		const int entityIndex = ENTINDEX(tr.pHit);
		for (i = 0; i < entityNum; i++)
		{
			if (g_entityId[i] == -1 || g_entityAction[i] != 1)
				continue;

			if (g_entityId[i] == entityIndex)
				return true;
		}
	}

	float friendDistance;
	for (const auto& client : g_clients)
	{
		if (FNullEnt(client.ent))
			continue;

		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == pev->pContainingEntity)
			continue;

		origin = client.ent->v.origin;
		friendDistance = (origin - pev->origin).GetLengthSquared();
		if (friendDistance < distance && GetShootingConeDeviation(GetEntity(), origin) > (friendDistance / (friendDistance + 1089.0f)))
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

	// check if we need to compensate recoil
	if (ctanf((cabsf(pev->punchangle.y) + cabsf(pev->punchangle.x)) * 0.00872664625f) * (distance + (distance * 0.25f)) > 100.0f)
	{
		const float time = engine->GetTime();
		if (m_firePause < (time - 0.4f))
			m_firePause = time + crandomfloat(0.4f, (0.4f + 1.25f * ((100 - m_skill)) * 0.0125f));

		return true;
	}

	if (UsesSniper())
	{
		if (!(m_currentTravelFlags & PATHFLAG_JUMP))
			pev->buttons &= ~IN_JUMP;

		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;

		if (pev->speed >= pev->maxspeed && !IsZombieMode())
		{
			m_firePause = engine->GetTime() + 0.125f;
			return true;
		}
	}

	return false;
}

// this function will return true if weapon was fired, false otherwise
void Bot::FireWeapon(void)
{
	const float distance = GetTargetDistance();
	if (IsFriendInLineOfFire(distance))
		return;

	WeaponSelect* selectTab = (g_gameVersion & Game::HalfLife) ? &g_weaponSelectHL[0] : &g_weaponSelect[0];
	if (ebot_knifemode.GetBool())
	{
		const int melee = (g_gameVersion & Game::HalfLife) ? WeaponHL::Crowbar : Weapon::Knife;
		if (m_currentWeapon == selectTab[melee].id)
			KnifeAttack();
		else
			SelectWeaponByName(selectTab[melee].weaponName);

		return;
	}

	int id;
	int chosenWeaponIndex = -1;
	int selectIndex = 0;
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
			else if (selectIndex != WeaponHL::HandGrenade && selectIndex != WeaponHL::Rpg && selectIndex != WeaponHL::Crossbow && m_ammoInClip[id] && !IsWeaponBadInDistance(selectIndex, distance))
				chosenWeaponIndex = selectIndex;
		}
		else if (m_ammoInClip[id] && !IsWeaponBadInDistance(selectIndex, distance))
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

			if (m_ammo[g_weaponDefs[id].ammo1])
				chosenWeaponIndex = selectIndex;

			selectIndex++;
		}
	}

	if (chosenWeaponIndex == -1)
	{
		chosenWeaponIndex = (g_gameVersion & Game::HalfLife) ? WeaponHL::Crowbar : Weapon::Knife;
		if (m_currentWeapon == selectTab[chosenWeaponIndex].id)
		{
			KnifeAttack();
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

	const float time = engine->GetTime();
	if (UsesSniper() && m_zoomCheckTime < time) // is the bot holding a sniper rifle?
	{
		if (distance > squaredf(1500.0f) && pev->fov >= 40.0f) // should the bot switch to the long-range zoom?
			pev->buttons |= IN_ATTACK2;
		else if (distance > squaredf(150.0f) && pev->fov >= 90.0f) // else should the bot switch to the close-range zoom ?
			pev->buttons |= IN_ATTACK2;
		else if (distance < squaredf(150.0f) && pev->fov < 90.0f) // else should the bot restore the normal view ?
			pev->buttons |= IN_ATTACK2;

		m_zoomCheckTime = time;
	}
	else if (UsesZoomableRifle() && m_zoomCheckTime < time && m_skill < 90) // else is the bot holding a zoomable rifle?
	{
		if (distance > squaredf(800.0f) && pev->fov >= 90.0f) // should the bot switch to zoomed mode?
			pev->buttons |= IN_ATTACK2;
		else if (distance < squaredf(800.0f) && pev->fov < 90.0f) // else should the bot restore the normal view?
			pev->buttons |= IN_ATTACK2;

		m_zoomCheckTime = time;
	}

	if (!GetAmmoInClip())
	{
		if (!(pev->oldbuttons & IN_RELOAD))
			pev->buttons |= IN_RELOAD; // press reload button

		return;
	}

	// need to care for burst fire?
	if (g_gameVersion & Game::HalfLife)
	{
		if (selectTab[chosenWeaponIndex].primaryFireHold) // if automatic weapon, just press attack
			pev->buttons |= IN_ATTACK;
		else // if not, toggle buttons
		{
			if (!(pev->oldbuttons & IN_ATTACK))
				pev->buttons |= IN_ATTACK;
		}
	}
	else
	{
		CheckBurstMode(distance);

		if (HasShield() && m_shieldCheckTime < time && GetCurrentState() != Process::Camp) // better shield gun usage
		{
			if ((distance > squaredf(768.0f)) && !IsShieldDrawn())
				pev->buttons |= IN_ATTACK2; // draw the shield
			else if (IsShieldDrawn())
				pev->buttons |= IN_ATTACK2; // draw out the shield
			else
			{
				edict_t* enemy = m_entityDistance < m_enemyDistance ? m_nearestEntity : m_nearestEnemy;
				if (IsValidPlayer(enemy) && enemy->v.buttons & IN_RELOAD)
					pev->buttons |= IN_ATTACK2; // draw out the shield
			}

			m_shieldCheckTime = time + 0.5f;
		}

		if (distance < squaredf(384.0f))
		{
			if (selectTab[chosenWeaponIndex].primaryFireHold) // if automatic weapon, just press attack
				pev->buttons |= IN_ATTACK;
			else // if not, toggle buttons
			{
				if (!(pev->oldbuttons & IN_ATTACK))
					pev->buttons |= IN_ATTACK;
			}
			return;
		}

		if (DoFirePause(distance))
			return;

		float delayTime = 0.0f;
		if (selectTab[chosenWeaponIndex].primaryFireHold)
		{
			m_zoomCheckTime = time;
			pev->buttons |= IN_ATTACK; // use primary attack
		}
		else
		{
			if (!(pev->oldbuttons & IN_ATTACK))
				pev->buttons |= IN_ATTACK;

			const int fireDelay = cclamp(cabs((m_skill / 20) - 5), 0, 6);
			FireDelay* delay = &g_fireDelay[0];
			delayTime = delay[chosenWeaponIndex].primaryBaseDelay + crandomfloat(delay[chosenWeaponIndex].primaryMinDelay[fireDelay], delay[chosenWeaponIndex].primaryMaxDelay[fireDelay]);
			m_zoomCheckTime = time;
		}

		if (distance > squaredf(1200.0f))
		{
			if (m_visibility & (Visibility::Head | Visibility::Body))
				delayTime -= (delayTime == 0.0f) ? 0.0f : 0.02f;
			else if (m_visibility & Visibility::Head)
			{
				if (distance > squaredf(2400.0f))
					delayTime += (delayTime == 0.0f) ? 0.15f : 0.10f;
				else
					delayTime += (delayTime == 0.0f) ? 0.10f : 0.05f;
			}
			else if (m_visibility & Visibility::Body)
			{
				if (distance > squaredf(2400.0f))
					delayTime += (delayTime == 0.0f) ? 0.12f : 0.08f;
				else
					delayTime += (delayTime == 0.0f) ? 0.08f : 0.0f;
			}
			else
			{
				if (distance > squaredf(2400.0f))
					delayTime += (delayTime == 0.0f) ? 0.18f : 0.15f;
				else
					delayTime += (delayTime == 0.0f) ? 0.15f : 0.10f;
			}
		}

		m_firePause = time + delayTime;
	}
}

bool Bot::KnifeAttack(const float attackDistance)
{
	if (g_gameVersion & Game::HalfLife)
	{
		pev->buttons |= IN_ATTACK;
		return true;
	}

	Vector origin = nullvec;
	float distance = FLT_MAX;

	if (m_enemyDistance < m_entityDistance)
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

	if (kaMode)
	{
		if (pev->origin.z > origin.z && (pev->origin - origin).GetLengthSquared2D() < squaredf(64.0f))
		{
			pev->buttons |= IN_DUCK;
			pev->buttons &= ~IN_JUMP;
		}
		else
		{
			pev->buttons &= ~IN_DUCK;
			if (pev->origin.z + 150.0f < origin.z && (pev->origin - origin).GetLengthSquared2D() < squaredf(300.0f))
				pev->buttons |= IN_JUMP;
		}

		if (m_isZombieBot)
		{
			if (kaMode != 2)
				pev->buttons |= IN_ATTACK;
			else
				pev->buttons |= IN_ATTACK2;
		}
		else
		{
			if (kaMode == 1)
				pev->buttons |= IN_ATTACK;
			else if (kaMode == 2)
				pev->buttons |= IN_ATTACK2;
			else if (crandomint(1, 10) < 3 || HasShield())
				pev->buttons |= IN_ATTACK;
			else
				pev->buttons |= IN_ATTACK2;
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

		SelectWeaponByName("weapon_crowbar");
	}
	else
	{
		// already have
		if (m_currentWeapon == Weapon::Knife)
			return;

		if (m_isBomber)
			return;

		if (m_walkTime > engine->GetTime())
			return;

		SelectWeaponByName("weapon_knife");
	}
}

void Bot::SelectBestWeapon(void)
{
	WeaponSelect* selectTab = (g_gameVersion & Game::HalfLife) ? &g_weaponSelectHL[0] : &g_weaponSelect[0];
	int chosenWeaponIndex = -1;
	int selectIndex = 0;
	
	int id;
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
			chosenWeaponIndex = selectIndex;
		else
		{
			if (m_ammo[g_weaponDefs[id].ammo1])
				chosenWeaponIndex = selectIndex;
		}

		selectIndex++;
	}

	if (chosenWeaponIndex == -1)
		chosenWeaponIndex = GetHighestWeapon();

	chosenWeaponIndex %= Const_NumWeapons + 1;
	selectIndex = chosenWeaponIndex;

	if (m_currentWeapon != selectTab[selectIndex].id)
	{
		SelectWeaponByName(selectTab[selectIndex].weaponName);
		if (!(pev->oldbuttons & IN_RELOAD))
			pev->buttons |= IN_RELOAD; // press reload button
	}
}

void Bot::SelectPistol(void)
{
	const int oldWeapons = pev->weapons;
	pev->weapons &= ~WeaponBits_Primary;
	SelectBestWeapon();
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
		// if (pev->waterlevel == 3 && g_weaponDefs[selectTab->id].flags & ITEM_FLAG_NOFIREUNDERWATER)
		//	continue;

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

int Bot::GetMaxClip(const int& id)
{
	switch (id)
	{
	case Weapon::M249:
		return 100;
	case Weapon::P90:
		return 50;
	case Weapon::Galil:
		return 35;
	case Weapon::Elite:
	case Weapon::Mp5:
	case Weapon::Tmp:
	case Weapon::Mac10:
	case Weapon::M4A1:
	case Weapon::Ak47:
	case Weapon::Sg552:
	case Weapon::Aug:
	case Weapon::Sg550:
		return 30;
	case Weapon::Ump45:
	case Weapon::Famas:
		return 25;
	case Weapon::Glock18:
	case Weapon::FiveSeven:
	case Weapon::G3SG1:
		return 20;
	case Weapon::P228:
		return 13;
	case Weapon::Usp:
		return 12;
	case Weapon::Awp:
	case Weapon::Scout:
		return 10;
	case Weapon::M3:
		return 8;
	case Weapon::Deagle:
	case Weapon::Xm1014:
		return 7;
	}

	return 30;
}