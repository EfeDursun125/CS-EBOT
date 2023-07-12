#include <core.h>

ConVar ebot_escape("ebot_zombie_escape_mode", "0");
ConVar ebot_zp_use_grenade_percent("ebot_zm_use_grenade_percent", "10");
ConVar ebot_zp_escape_distance("ebot_zm_escape_distance", "200");
ConVar ebot_zombie_speed_factor("ebot_zombie_speed_factor", "0.54");
ConVar ebot_sb_mode("ebot_sb_mode", "0");

void Bot::FindFriendsAndEnemiens(void)
{
	m_enemyDistance = FLT_MAX;
	m_friendDistance = FLT_MAX;
	m_enemiesNearCount = 0;
	m_friendsNearCount = 0;
	m_numEnemiesLeft = 0;
	m_numFriendsLeft = 0;
	m_hasEnemiesNear = false;
	m_hasFriendsNear = false;

	for (const auto& client : g_clients)
	{
		if (FNullEnt(client.ent))
			continue;

		if (client.ent == GetEntity())
			continue;

		if (!IsAlive(client.ent))
			continue;

		if (client.team == m_team)
		{
			m_numFriendsLeft++;

			// simple check
			const Vector friendOrigin = client.ent->v.origin + client.ent->v.view_ofs;
			TraceResult tr{};
			TraceLine(pev->origin, friendOrigin, true, true, GetEntity(), &tr);
			if (tr.flFraction != 1.0f)
				continue;

			m_friendsNearCount++;
			const float distance = (pev->origin - friendOrigin).GetLengthSquared();
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
			if (!m_isZombieBot && client.ent != m_nearestEnemy)
			{
				if (GetProcess() != Process::Camp && !IsAttacking(client.ent) && !IsInViewCone(client.ent->v.origin))
					continue;

				if (IsBehindSmokeClouds(client.ent))
					continue;

				if (IsNotAttackLab(client.ent))
					continue;
			}

			m_enemiesNearCount++;
			const float distance = (pev->origin - client.ent->v.origin).GetLengthSquared();
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

	for (int i = engine->GetMaxClients() + 1; i < entityNum; i++)
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
		TraceLine(pev->origin, origin, true, true, GetEntity(), &tr);
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
		if (!IsZombieMode() && m_currentWeapon == WEAPON_AWP)
			return targetOrigin;
		// now check is our skill match to aim at head, else aim at enemy body
		else if (IsZombieMode() || ChanceOf(m_skill) || UsesPistol())
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

float Bot::GetZOffset(float distance)
{
	if (m_difficulty < 1)
		return -4.0f;

	const bool sniper = UsesSniper();
	const bool pistol = UsesPistol();
	const bool rifle = UsesRifle();

	const bool zoomableRifle = UsesZoomableRifle();
	const bool submachine = UsesSubmachineGun();
	const bool shotgun = (m_currentWeapon == WEAPON_XM1014 || m_currentWeapon == WEAPON_M3);
	const bool m249 = m_currentWeapon == WEAPON_M249;

	const float BurstDistance = SquaredF(300.0f);
	const float DoubleBurstDistance = BurstDistance * 2.0f;

	float result = 4.0f;

	if (distance <= SquaredF(2800.0f) && distance > DoubleBurstDistance)
	{
		if (sniper) result = 3.5f;
		else if (zoomableRifle) result = 4.5f;
		else if (pistol) result = 6.5f;
		else if (submachine) result = 5.5f;
		else if (rifle) result = 5.5f;
		else if (m249) result = 2.5f;
		else if (shotgun) result = 10.5f;
	}
	else if (distance > BurstDistance && distance <= DoubleBurstDistance)
	{
		if (sniper) result = 3.5f;
		else if (zoomableRifle) result = 3.5f;
		else if (pistol) result = 6.5f;
		else if (submachine) result = 3.5f;
		else if (rifle) result = 1.6f;
		else if (m249) result = -1.0f;
		else if (shotgun) result = 10.0f;
	}
	else if (distance < BurstDistance)
	{
		if (sniper) result = 4.5f;
		else if (zoomableRifle) result = 5.0f;
		else if (pistol) result = 4.5f;
		else if (submachine) result = 4.5f;
		else if (rifle) result = 4.5f;
		else if (m249) result = 6.0f;
		else if (shotgun) result = 5.0f;
	}

	return result;
}

// bot can't hurt teammates, if friendly fire is not enabled...
bool Bot::IsFriendInLineOfFire(const float distance)
{
	if (g_gameVersion == HALFLIFE || !engine->IsFriendlyFireOn() || GetGameMode() == MODE_DM || GetGameMode() == MODE_NOTEAM)
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
			for (int i = 0; i < entityNum; i++)
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

int CorrectGun(int weaponID)
{
	if (GetGameMode() != MODE_BASE)
		return 0;

	if (weaponID == WEAPON_AUG || weaponID == WEAPON_M4A1 || weaponID == WEAPON_SG552 || weaponID == WEAPON_AK47 || weaponID == WEAPON_FAMAS || weaponID == WEAPON_GALIL)
		return 2;
	else if (weaponID == WEAPON_SG552 || weaponID == WEAPON_G3SG1)
		return 3;

	return 0;
}

bool Bot::DoFirePause(const float distance)
{
	if (m_firePause > engine->GetTime())
		return true;

	if (m_hasEnemiesNear && IsEnemyProtectedByShield(m_nearestEnemy))
		return true;

	const float angle = (cabsf(pev->punchangle.y) + cabsf(pev->punchangle.x)) * (Math::MATH_PI * 0.00277777777f);

	// check if we need to compensate recoil
	if (tanf(angle) * (distance + (distance * 0.25f)) > 100.0f)
	{
		if (m_firePause < (engine->GetTime() - 0.4))
			m_firePause = AddTime(CRandomFloat(0.4f, (0.4f + 1.2f * ((100 - m_skill)) * 0.01f)));

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
			m_firePause = AddTime(0.1f);
			return true;
		}
	}

	return false;
}

// this function will return true if weapon was fired, false otherwise
void Bot::FireWeapon(void)
{
	// try to switch
	if (m_currentWeapon == WEAPON_KNIFE)
		SelectBestWeapon();

	const float distance = m_enemyDistance <= m_entityDistance ? m_enemyDistance : m_entityDistance;

	// or if friend in line of fire, stop this too but do not update shoot time
	if (IsFriendInLineOfFire(distance))
		return;

	const int melee = g_gameVersion == HALFLIFE ? WEAPON_CROWBAR : WEAPON_KNIFE;

	FireDelay* delay = &g_fireDelay[0];
	WeaponSelect* selectTab = g_gameVersion == HALFLIFE ? &g_weaponSelectHL[0] : &g_weaponSelect[0];

	edict_t* enemy = m_enemyDistance <= m_entityDistance ? m_nearestEnemy : m_nearestEntity;

	int selectId = melee, selectIndex = 0, chosenWeaponIndex = 0;
	int weapons = pev->weapons;

	if (ebot_knifemode.GetBool())
		goto WeaponSelectEnd;
	else if (!FNullEnt(enemy) && ChanceOf(m_skill) && !IsZombieEntity(enemy) && distance <= SquaredF(128.0f) && (enemy->v.health <= 30 || pev->health > enemy->v.health) && !IsOnLadder() && m_enemiesNearCount <= 1)
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
			if (g_gameVersion == HALFLIFE)
			{
				if (selectIndex == WEAPON_SNARK || selectIndex == WEAPON_GAUSS ||selectIndex == WEAPON_EGON || (selectIndex == WEAPON_HANDGRENADE && distance > SquaredF(384.0f) && distance <= SquaredF(768.0f)) || (selectIndex == WEAPON_RPG && distance > SquaredF(320.0f)) || (selectIndex == WEAPON_CROSSBOW && distance > SquaredF(320.0f)))
					chosenWeaponIndex = selectIndex;
				else if (selectIndex != WEAPON_HANDGRENADE && selectIndex != WEAPON_RPG  && selectIndex != WEAPON_CROSSBOW && (m_ammoInClip[id] > 0) && !IsWeaponBadInDistance(selectIndex, distance))
						chosenWeaponIndex = selectIndex;

			}
			else if ((m_ammoInClip[id] > 0) && !IsWeaponBadInDistance(selectIndex, distance))
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
					if (m_reloadState == ReloadState::Nothing || m_reloadCheckTime > engine->GetTime())
					{
						m_isReloading = true;
						m_reloadState = ReloadState::Primary;
						m_reloadCheckTime = engine->GetTime();
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
		m_reloadCheckTime = AddTime(6.0f);
	}

	if (IsZombieMode() && m_currentWeapon == melee && selectId != melee && !m_isZombieBot)
	{
		m_reloadState = ReloadState::Primary;
		m_reloadCheckTime = AddTime(2.5f);
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
	if (g_gameVersion != HALFLIFE)
	{
		CheckBurstMode(distance);

		if (HasShield() && m_shieldCheckTime < engine->GetTime() && GetProcess() != Process::Camp) // better shield gun usage
		{
			if ((distance > SquaredF(768.0f)) && !IsShieldDrawn())
				pev->button |= IN_ATTACK2; // draw the shield
			else if (IsShieldDrawn() || (IsValidPlayer(enemy) && enemy->v.button & IN_RELOAD))
				pev->button |= IN_ATTACK2; // draw out the shield

			m_shieldCheckTime = AddTime(engine->GetTime());
		}
	}

	if (UsesSniper() && m_zoomCheckTime < engine->GetTime()) // is the bot holding a sniper rifle?
	{
		if (distance > SquaredF(1500.0f) && pev->fov >= 40.0f) // should the bot switch to the long-range zoom?
			pev->button |= IN_ATTACK2;

		else if (distance > SquaredF(150.0f) && pev->fov >= 90.0f) // else should the bot switch to the close-range zoom ?
			pev->button |= IN_ATTACK2;

		else if (distance <= SquaredF(150.0f) && pev->fov < 90.0f) // else should the bot restore the normal view ?
			pev->button |= IN_ATTACK2;

		m_zoomCheckTime = engine->GetTime();
	}
	else if (UsesZoomableRifle() && m_zoomCheckTime < engine->GetTime() && m_skill < 90) // else is the bot holding a zoomable rifle?
	{
		if (distance > SquaredF(800.0f) && pev->fov >= 90.0f) // should the bot switch to zoomed mode?
			pev->button |= IN_ATTACK2;

		else if (distance <= SquaredF(800.0f) && pev->fov < 90.0f) // else should the bot restore the normal view?
			pev->button |= IN_ATTACK2;

		m_zoomCheckTime = engine->GetTime();
	}

	// need to care for burst fire?
	if (g_gameVersion == HALFLIFE || distance <= SquaredF(512.0f))
	{
		if (selectId == melee)
			KnifeAttack();
		else
		{
			if (selectTab[chosenWeaponIndex].primaryFireHold) // if automatic weapon, just press attack
				pev->button |= IN_ATTACK;
			else // if not, toggle buttons
			{
				if ((pev->oldbuttons & IN_ATTACK) == 0)
					pev->button |= IN_ATTACK;
			}
		}
	}
	else
	{
		if (DoFirePause(distance))
			return;

		// don't attack with knife over long distance
		if (selectId == melee)
		{
			KnifeAttack();
			return;
		}

		float delayTime = 0.0f;
		if (selectTab[chosenWeaponIndex].primaryFireHold)
		{
			m_zoomCheckTime = engine->GetTime();
			pev->button |= IN_ATTACK;  // use primary attack
		}
		else
		{
			pev->button |= IN_ATTACK;  // use primary attack
			const float baseDelay = delay[chosenWeaponIndex].primaryBaseDelay;
			const float minDelay = delay[chosenWeaponIndex].primaryMinDelay[cabs((m_skill / CRandomInt(10, 20)) - 5)];
			const float maxDelay = delay[chosenWeaponIndex].primaryMaxDelay[cabs((m_skill / CRandomInt(20, 30)) - 5)];
			delayTime = baseDelay + CRandomFloat(minDelay, maxDelay);
			m_zoomCheckTime = engine->GetTime();
		}

		if (distance >= SquaredF(1200.0f))
		{
			if (m_visibility & (Visibility::Head | Visibility::Body))
				delayTime -= (delayTime == 0.0f) ? 0.0f : 0.02f;
			else if (m_visibility & Visibility::Head)
			{
				if (distance >= SquaredF(2400.0f))
					delayTime += (delayTime == 0.0f) ? 0.15f : 0.10f;
				else
					delayTime += (delayTime == 0.0f) ? 0.10f : 0.05f;
			}
			else if (m_visibility & Visibility::Body)
			{
				if (distance >= SquaredF(2400.0f))
					delayTime += (delayTime == 0.0f) ? 0.12f : 0.08f;
				else
					delayTime += (delayTime == 0.0f) ? 0.08f : 0.0f;
			}
			else
			{
				if (distance >= SquaredF(2400.0f))
					delayTime += (delayTime == 0.0f) ? 0.18f : 0.15f;
				else
					delayTime += (delayTime == 0.0f) ? 0.15f : 0.10f;
			}
		}
	}
}

bool Bot::KnifeAttack(const float attackDistance)
{
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

	float kad1 = SquaredF(pev->speed * 0.26);
	const float kad2 = SquaredF(64.0f);

	if (attackDistance != 0.0f)
		kad1 = attackDistance;

	int kaMode = 0;

	if (distance <= kad1)
		kaMode = 1;

	if (distance <= kad2)
		kaMode += 2;

	if (kaMode > 0)
	{
		const float distanceSkipZ = (pev->origin - origin).GetLengthSquared2D();

		if (pev->origin.z > origin.z && distanceSkipZ < SquaredF(64.0f))
		{
			pev->button |= IN_DUCK;
			pev->button &= ~IN_JUMP;
		}
		else
		{
			pev->button &= ~IN_DUCK;

			if (pev->origin.z + 150.0f < origin.z && distanceSkipZ < SquaredF(300.0f))
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
			else if (CRandomInt(1, 10) < 3 || HasShield())
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
	if (g_gameVersion == HALFLIFE)
	{
		const int weaponID = g_weaponSelectHL[weaponIndex].id;
		if (weaponID == WEAPON_CROWBAR)
			return false;

		// shotgun is too inaccurate at long distances, so weapon is bad
		if (weaponID == WEAPON_SHOTGUN && distance > SquaredF(768.0f))
			return true;
	}
	else
	{
		const int weaponID = g_weaponSelect[weaponIndex].id;
		if (weaponID == WEAPON_KNIFE)
			return false;

		// shotguns is too inaccurate at long distances, so weapon is bad
		if ((weaponID == WEAPON_M3 || weaponID == WEAPON_XM1014) && distance > SquaredF(768.0f))
			return true;

		if (!IsZombieMode())
		{
			if ((weaponID == WEAPON_SCOUT || weaponID == WEAPON_AWP || weaponID == WEAPON_G3SG1 || weaponID == WEAPON_SG550) && distance <= SquaredF(512.0f))
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

// this function returns true, if bot has a tactical shield
bool Bot::HasShield(void)
{
	return cstrncmp(STRING(pev->viewmodel), "models/shield/v_shield_", 23) == 0;
}

// this function returns true, is the tactical shield is drawn
bool Bot::IsShieldDrawn(void)
{
	if (g_gameVersion == HALFLIFE)
		return false;

	if (!HasShield())
		return false;

	return pev->weaponanim == 6 || pev->weaponanim == 7;
}

// this function returns true, if enemy protected by the shield
bool Bot::IsEnemyProtectedByShield(edict_t* enemy)
{
	if (g_gameVersion == HALFLIFE)
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

bool Bot::UsesSniper(void)
{
	if (g_gameVersion == HALFLIFE)
		return m_currentWeapon == WEAPON_CROSSBOW;

	return m_currentWeapon == WEAPON_AWP || m_currentWeapon == WEAPON_G3SG1 || m_currentWeapon == WEAPON_SCOUT || m_currentWeapon == WEAPON_SG550;
}

bool Bot::IsSniper(void)
{
	if (g_gameVersion == HALFLIFE)
	{
		if (pev->weapons & (1 << WEAPON_CROSSBOW))
			return true;

		return false;
	}

	if (pev->weapons & (1 << WEAPON_AWP))
		return true;
	else if (pev->weapons & (1 << WEAPON_G3SG1))
		return true;
	else if (pev->weapons & (1 << WEAPON_SCOUT))
		return true;
	else if (pev->weapons & (1 << WEAPON_SG550))
		return true;

	return false;
}

bool Bot::UsesRifle(void)
{
	if (g_gameVersion == HALFLIFE)
		return m_currentWeapon == WEAPON_MP5_HL;

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
	if (g_gameVersion == HALFLIFE)
		return m_currentWeapon == WEAPON_GLOCK || m_currentWeapon == WEAPON_PYTHON;

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
	if (g_gameVersion == HALFLIFE)
		return m_currentWeapon == WEAPON_EGON;

	return m_currentWeapon == WEAPON_MP5 || m_currentWeapon == WEAPON_TMP || m_currentWeapon == WEAPON_P90 || m_currentWeapon == WEAPON_MAC10 || m_currentWeapon == WEAPON_UMP45;
}

bool Bot::UsesZoomableRifle(void)
{
	if (g_gameVersion == HALFLIFE)
		return false;

	return m_currentWeapon == WEAPON_AUG || m_currentWeapon == WEAPON_SG552;
}

bool Bot::UsesBadPrimary(void)
{
	if (g_gameVersion == HALFLIFE)
		return m_currentWeapon == WEAPON_HORNETGUN;

	return m_currentWeapon == WEAPON_M3 || m_currentWeapon == WEAPON_UMP45 || m_currentWeapon == WEAPON_MAC10 || m_currentWeapon == WEAPON_TMP || m_currentWeapon == WEAPON_P90;
}

int Bot::CheckGrenades(void)
{
	if (pev->weapons & (1 << WEAPON_HEGRENADE))
		return WEAPON_HEGRENADE;
	else if (pev->weapons & (1 << WEAPON_FBGRENADE))
		return WEAPON_FBGRENADE;
	else if (pev->weapons & (1 << WEAPON_SMGRENADE))
		return WEAPON_SMGRENADE;
	return -1;
}

void Bot::SelectKnife(void)
{
	if (g_gameVersion == HALFLIFE)
	{
		if (m_currentWeapon == WEAPON_CROWBAR)
			return;

		if (m_weaponSelectDelay > engine->GetTime())
			return;

		SelectWeaponByName("weapon_crowbar");
	}
	else
	{
		// already have
		if (m_currentWeapon == WEAPON_KNIFE)
			return;

		if (m_isBomber)
			return;

		if (m_walkTime > engine->GetTime())
			return;

		if (m_weaponSelectDelay > engine->GetTime())
			return;

		SelectWeaponByName("weapon_knife");
	}
}

void Bot::SelectBestWeapon(void)
{
	if (m_weaponSelectDelay >= engine->GetTime())
		return;

	if (!m_hasEnemiesNear && !m_hasEntitiesNear && (m_spawnTime + engine->GetFreezeTime() + 7.0f) > engine->GetTime())
		return;

	WeaponSelect* selectTab = g_gameVersion == HALFLIFE ? &g_weaponSelectHL[0] : &g_weaponSelect[0];

	int selectIndex = -1;
	int chosenWeaponIndex = -1;

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

		if (g_gameVersion == HALFLIFE)
			chosenWeaponIndex = selectIndex;
		else
		{
			bool ammoLeft = false;

			if (id == m_currentWeapon)
			{
				// dont be fool
				if (m_isReloading)
					ammoLeft = false;
				else if (GetAmmoInClip() > 0)
					ammoLeft = true;
			}
			else if (m_ammo[g_weaponDefs[id].ammo1] > 0)
				ammoLeft = true;

			if (ammoLeft)
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
		m_isReloading = false;
		m_reloadState = ReloadState::Nothing;
		m_weaponSelectDelay = AddTime(engine->GetTime());
	}
}

void Bot::SelectPistol(void)
{
	if (!m_isSlowThink)
		return;

	if (m_isReloading)
		return;

	int oldWeapons = pev->weapons;

	pev->weapons &= ~WeaponBits_Primary;
	SelectBestWeapon();

	pev->weapons = oldWeapons;
}

int Bot::GetHighestWeapon(void)
{
	WeaponSelect* selectTab = g_gameVersion == HALFLIFE ? &g_weaponSelectHL[0] : &g_weaponSelect[0];

	int weapons = pev->weapons;
	int num = 0;
	int i = 0;

	// loop through all the weapons until terminator is found...
	while (selectTab->id)
	{
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
	if (m_currentWeapon == g_gameVersion == HALFLIFE ? WEAPON_CROWBAR : WEAPON_KNIFE)
	{
		m_reloadState = ReloadState::Nothing;
		m_isReloading = false;
		return;
	}

	m_isReloading = false; // update reloading status
	m_reloadCheckTime = AddTime(3.0f);

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
			// TODO: is this good for zombie?
			// if we have enemy don't reload next weapon
			/*if (m_seeEnemyTime + 5.0f > engine->GetTime())
			{
				m_reloadState = ReloadState::Nothing;
				return;
			}*/

			m_reloadState++;

			if (m_reloadState > ReloadState::Secondary)
				m_reloadState = ReloadState::Nothing;

			return;
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

	InternalAssert(weaponIndex);

	switch (*weaponIndex)
	{
	case WEAPON_M249:
		maxClip = 100;
		break;

	case WEAPON_P90:
		maxClip = 50;
		break;

	case WEAPON_GALIL:
		maxClip = 35;
		break;

	case WEAPON_ELITE:
	case WEAPON_MP5:
	case WEAPON_TMP:
	case WEAPON_MAC10:
	case WEAPON_M4A1:
	case WEAPON_AK47:
	case WEAPON_SG552:
	case WEAPON_AUG:
	case WEAPON_SG550:
		maxClip = 30;
		break;

	case WEAPON_UMP45:
	case WEAPON_FAMAS:
		maxClip = 25;
		break;

	case WEAPON_GLOCK18:
	case WEAPON_FN57:
	case WEAPON_G3SG1:
		maxClip = 20;
		break;

	case WEAPON_P228:
		maxClip = 13;
		break;

	case WEAPON_USP:
		maxClip = 12;
		break;

	case WEAPON_AWP:
	case WEAPON_SCOUT:
		maxClip = 10;
		break;

	case WEAPON_M3:
		maxClip = 8;
		break;

	case WEAPON_DEAGLE:
	case WEAPON_XM1014:
		maxClip = 7;
		break;
	}

	return maxClip;
}