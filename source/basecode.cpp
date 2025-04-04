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
// Based on SyPB
//

#include "../include/core.h"
#include "../include/tinythread.h"

// console variables
ConVar ebot_debug("ebot_debug", "0");

ConVar ebot_stopbots("ebot_stop_bots", "0");
ConVar ebot_force_flashlight("ebot_force_flashlight", "0");
ConVar ebot_use_flare("ebot_use_flares", "1");
ConVar ebot_use_grenade_percent("ebot_use_grenade_percent", "60");
ConVar ebot_check_enemy_rendering("ebot_check_enemy_rendering", "0");
ConVar ebot_check_enemy_invincibility("ebot_check_enemy_invincibility", "0");
ConVar ebot_aim_trace_consider_glass("ebot_aim_trace_consider_glass", "0");

float Bot::InFieldOfView(const Vector& destination)
{
	const float absoluteAngle = cabsf(AngleMod(pev->v_angle.y) - AngleMod(destination.ToYaw()));
	if (absoluteAngle > 180.0f)
		return 360.0f - absoluteAngle;
	return absoluteAngle;
}

bool Bot::IsInViewCone(const Vector& origin)
{
	return ::IsInViewCone(origin, m_myself);
}

bool Bot::CheckVisibility(edict_t* targetEntity)
{
	if (FNullEnt(targetEntity))
		return false;

	TraceResult tr;
	const Vector eyes = EyePosition();
	Vector spot = (targetEntity->v.absmin + (targetEntity->v.size * 0.5f)) + targetEntity->v.view_ofs;

	const int ignoreFlags = m_isZombieBot ? TraceIgnore::Nothing : (ebot_aim_trace_consider_glass.GetBool() ? TraceIgnore::Monsters : TraceIgnore::Everything);
	TraceLine(eyes, spot, ignoreFlags, m_myself, &tr);
	if (tr.flFraction > 0.99f || (!FNullEnt(tr.pHit) && tr.pHit == targetEntity))
	{
		m_enemyOrigin = spot;
		return true;
	}

	return false;
}

bool Bot::IsEnemyViewable(edict_t* player)
{
	return CheckVisibility(player);
}

// this function checks buttons for use button waypoint
edict_t* Bot::FindButton(void)
{
	float nearestDistance = 9999999.0f, distance;
	edict_t* searchEntity = nullptr, *foundEntity = nullptr;

	// find the nearest button which can open our target
	while (!FNullEnt(searchEntity = FIND_ENTITY_IN_SPHERE(searchEntity, pev->origin, 512.0f)))
	{
		if (!cstrncmp("func_button", STRING(searchEntity->v.classname), 12) || !cstrncmp("func_rot_button", STRING(searchEntity->v.classname), 16))
		{
			distance = (pev->origin - GetEntityOrigin(searchEntity)).GetLengthSquared();
			if (distance < nearestDistance)
			{
				nearestDistance = distance;
				foundEntity = searchEntity;
			}
		}
	}

	return foundEntity;
}

bool Bot::CheckGrenadeThrow(edict_t* targetEntity)
{
	if (!m_hasEnemiesNear)
		return false;

	if (!chanceof(ebot_use_grenade_percent.GetInt()))
		return false;

	if (FNullEnt(targetEntity))
		return false;

	const int grenadeToThrow = CheckGrenades();
	if (grenadeToThrow == -1)
		return false;

	m_throw = nullvec;
	int th = -1;
	const Vector targetOrigin = GetBottomOrigin(targetEntity);
	if (grenadeToThrow == Weapon::HeGrenade)
	{
		float distance = (targetOrigin - pev->origin).GetLengthSquared();

		// is enemy to high to throw
		if ((targetOrigin.z > (pev->origin.z + 650.0f)) || !(targetEntity->v.flags & (FL_ONGROUND | FL_DUCKING)))
			distance = 9999999.0f; // just some crazy value

		// enemy is within a good throwing distance ?
		if (distance > squaredf(600.0f) && distance < squaredf(1200.0f))
		{
			bool allowThrowing = true;
			if (allowThrowing && m_seeEnemyTime + 2.0f < engine->GetTime())
			{
				Vector enemyPredict = ((targetEntity->v.velocity * 0.5f).SkipZ() + targetOrigin);
				int16_t searchTab[4], count = 4;

				float searchRadius = targetEntity->v.velocity.GetLength2D();

				// check the search radius
				if (searchRadius < 128.0f)
					searchRadius = 128.0f;

				// search waypoints
				g_waypoint->FindInRadius(enemyPredict, searchRadius, searchTab, &count);

				while (count)
				{
					allowThrowing = true;

					// check the throwing
					const Vector eyePosition = EyePosition();
					const Vector wpOrigin = g_waypoint->GetPath(searchTab[count--])->origin;
					Vector src = CheckThrow(eyePosition, wpOrigin);

					if (src.GetLengthSquared() < squaredf(100.0f))
						src = CheckToss(eyePosition, wpOrigin);

					if (src.IsNull())
						allowThrowing = false;

					m_throw = src;
					break;
				}
			}

			// start explosive grenade throwing?
			if (allowThrowing)
				th = 1;
		}
	}
	else if (grenadeToThrow == Weapon::FbGrenade && (targetOrigin - pev->origin).GetLengthSquared() < squaredf(800.0f))
	{
		bool allowThrowing = true;
		CArray<int16_t>inRadius;

		g_waypoint->FindInRadius(inRadius, 256.0f, targetOrigin + (targetEntity->v.velocity * 0.5f).SkipZ());

		int16_t i;
		Vector wpOrigin, eyePosition, src;
		for (i = 0; i < inRadius.Size(); i++)
		{
			wpOrigin = g_waypoint->GetPath(i)->origin;
			eyePosition = EyePosition();
			src = CheckThrow(eyePosition, wpOrigin);

			if (src.GetLengthSquared() < squaredf(100.0f))
				src = CheckToss(eyePosition, wpOrigin);

			if (src.IsNull())
				continue;

			m_throw = src;
			allowThrowing = true;
			break;
		}

		// start concussion grenade throwing?
		if (allowThrowing)
			th = 2;
	}

	if (m_throw.IsNull())
		return false;

	switch (th)
	{
		case 1:
		{
			SetProcess(Process::ThrowHE, "throwing HE grenade", false, engine->GetTime() + 5.0f);
			break;
		}
		case 2:
		{
			SetProcess(Process::ThrowFB, "throwing FB grenade", false, engine->GetTime() + 5.0f);
			break;
		}
	}

	return true;
}

bool Bot::CheckReachable(void)
{
	if (FNullEnt(m_nearestEnemy))
		return m_isEnemyReachable = false;

	// path matrix returns 0 if we are on the same waypoint
	if (Math::FltZero(m_enemyDistance))
		return m_isEnemyReachable = true;

	const int nearest = g_clients[ENTINDEX(m_nearestEnemy) - 1].wp;
	if (m_currentWaypointIndex == nearest || m_prevWptIndex[0] == nearest)
		return m_isEnemyReachable = true;

	if (m_isZombieBot)
	{
		if (m_navNode.IsEmpty())
		{
			if ((pev->origin - m_nearestEnemy->v.origin).GetLengthSquared() < squaredf(96.0f))
				return m_isEnemyReachable = true;
		}

		if (m_waypoint.flags & WAYPOINT_ZOMBIEPUSH)
		{
			if ((pev->origin - m_nearestEnemy->v.origin).GetLengthSquared() < squaredf(32.0f))
				return m_isEnemyReachable = true;

			return m_isEnemyReachable = false;
		}

		if (pev->flags & FL_DUCKING || m_nearestEnemy->v.flags & FL_DUCKING) // diff 1
		{
			if ((pev->origin - m_nearestEnemy->v.origin).GetLengthSquared() > squaredf(96.0f))
				return m_isEnemyReachable = false;
		}
		else if ((pev->origin - m_nearestEnemy->v.origin).GetLengthSquared() > squaredf(768.0f))
			return m_isEnemyReachable = false;
	}
	else
	{
		if (pev->flags & FL_DUCKING && m_nearestEnemy->v.flags & FL_DUCKING) // diff 2
		{
			if ((pev->origin - m_nearestEnemy->v.origin).GetLengthSquared() > squaredf(128.0f))
				return m_isEnemyReachable = false;
		}
		else if ((pev->origin - (m_nearestEnemy->v.origin + m_nearestEnemy->v.velocity.SkipZ() * 0.54f)).GetLengthSquared() > squaredf(192.0f))
			return m_isEnemyReachable = false;
	}

	if (pev->waterlevel < 2)
	{
		if (m_nearestEnemy->v.origin.z > (pev->origin.z + 62.0f) || m_nearestEnemy->v.origin.z < (pev->origin.z - 100.0f))
			return m_isEnemyReachable = false;
	}

	TraceResult tr;
	TraceHull(pev->origin , m_nearestEnemy->v.origin, TraceIgnore::Nothing, head_hull, m_myself, &tr);

	// we're not sure, return the current one
	if (tr.fAllSolid)
		return m_isEnemyReachable;

	if (tr.fStartSolid)
		return m_isEnemyReachable = false;

	if (!FNullEnt(tr.pHit))
	{
		if (!cstrcmp("func_illusionary", STRING(tr.pHit->v.classname)))
			return m_isEnemyReachable = false;

		if (!cstrcmp("func_wall", STRING(tr.pHit->v.classname)))
			return m_isEnemyReachable = false;

		if (GetTeam(tr.pHit) != m_team)
		{
			if (!g_waypoint->MustJump(pev->origin, m_nearestEnemy->v.origin))
				return m_isEnemyReachable = true;
			else if (tr.flFraction >= 1.0f)
			{
				if (tr.fInWater)
					return m_isEnemyReachable = true;
				else
					return m_isEnemyReachable = false;
			}
		}
	}

	if (tr.flFraction >= 1.0f && !g_waypoint->MustJump(pev->origin, m_nearestEnemy->v.origin))
		return m_isEnemyReachable = true;

	return m_isEnemyReachable = false;
}

bool Bot::IsEnemyReachableToPosition(const Vector& origin)
{
	TraceResult tr;
	for (const Clients& client : g_clients)
	{
		if (client.ent == m_myself)
			continue;

		if (client.team == m_team)
			continue;

		if (!IsAlive(client.ent))
			continue;

		if (pev->flags & FL_DUCKING && client.ent->v.flags & FL_DUCKING) // diff 2
		{
			if ((origin - client.ent->v.origin).GetLengthSquared() > squaredf(64.0f))
				continue;
		}
		else if ((origin - (client.ent->v.origin + client.ent->v.velocity * 0.54f)).GetLengthSquared() > squaredf(192.0f))
			continue;

		if (pev->waterlevel < 2)
		{
			if (client.ent->v.origin.z > (origin.z + 62.0f) || client.ent->v.origin.z < (origin.z - 100.0f))
				continue;
		}

		TraceHull(origin, client.ent->v.origin, TraceIgnore::Nothing, head_hull, m_myself, &tr);

		// don't take a risk
		if (tr.fAllSolid)
			return true;

		if (tr.fStartSolid)
			continue;

		if (!FNullEnt(tr.pHit))
		{
			if (!cstrcmp("func_illusionary", STRING(tr.pHit->v.classname)))
				continue;

			if (!cstrcmp("func_wall", STRING(tr.pHit->v.classname)))
				continue;

			if (GetTeam(tr.pHit) != m_team)
			{
				if (!g_waypoint->MustJump(origin, client.ent->v.origin))
					return true;
				else if (tr.flFraction >= 1.0f)
				{
					if (tr.fInWater)
						return true;
					else
						continue;
				}
			}
		}

		if (tr.flFraction >= 1.0f && (!tr.fInWater || !g_waypoint->MustJump(origin, client.ent->v.origin)))
			return true;
	}

	return false;
}

bool Bot::IsFriendReachableToPosition(const Vector& origin)
{
	TraceResult tr;
	for (const Clients& client : g_clients)
	{
		if (client.ent == m_myself)
			continue;

		if (client.team != m_team)
			continue;

		if (!IsAlive(client.ent))
			continue;

		if (pev->flags & FL_DUCKING && client.ent->v.flags & FL_DUCKING) // diff 2
		{
			if ((origin - client.ent->v.origin).GetLengthSquared() > squaredf(64.0f))
				continue;
		}
		else if ((origin - (client.ent->v.origin + client.ent->v.velocity * 0.54f)).GetLengthSquared() > squaredf(192.0f))
			continue;

		if (pev->waterlevel < 2)
		{
			if (client.ent->v.origin.z > (origin.z + 62.0f) || client.ent->v.origin.z < (origin.z - 100.0f))
				continue;
		}

		TraceHull(origin, client.ent->v.origin, TraceIgnore::Nothing, head_hull, m_myself, &tr);

		// don't take a risk
		if (tr.fAllSolid)
			return true;

		if (tr.fStartSolid)
			continue;

		if (!FNullEnt(tr.pHit))
		{
			if (!cstrcmp("func_illusionary", STRING(tr.pHit->v.classname)))
				continue;

			if (!cstrcmp("func_wall", STRING(tr.pHit->v.classname)))
				continue;

			if (GetTeam(tr.pHit) != m_team)
			{
				if (!g_waypoint->MustJump(origin, client.ent->v.origin))
					return true;
				else if (tr.flFraction >= 1.0f)
				{
					if (tr.fInWater)
						return true;
					else
						continue;
				}
			}
		}

		if (tr.flFraction >= 1.0f && (!tr.fInWater || !g_waypoint->MustJump(origin, client.ent->v.origin)))
			return true;
	}

	return false;
}

bool Bot::CanIReachToPosition(const Vector& origin)
{
	if (pev->flags & FL_DUCKING)
	{
		if ((pev->origin - origin).GetLengthSquared() > squaredf(64.0f))
			return false;
	}
	else if ((pev->origin - origin).GetLengthSquared() > squaredf(192.0f))
		return false;

	if (pev->waterlevel < 2)
	{
		if (pev->origin.z > (origin.z + 62.0f) || pev->origin.z < (origin.z - 100.0f))
			return false;
	}

	TraceResult tr;
	TraceHull(pev->origin, origin, TraceIgnore::Nothing, head_hull, m_myself, &tr);

	if (tr.fAllSolid)
		return false;

	if (tr.fStartSolid)
		return false;

	if (!FNullEnt(tr.pHit))
	{
		if (!cstrcmp("func_illusionary", STRING(tr.pHit->v.classname)))
			return false;

		if (!cstrcmp("func_wall", STRING(tr.pHit->v.classname)))
			return false;

		if (GetTeam(tr.pHit) != m_team)
		{
			if (!g_waypoint->MustJump(pev->origin, origin))
				return true;
			else if (tr.flFraction >= 1.0f)
			{
				if (tr.fInWater)
					return true;
				else
					return false;
			}
		}
	}

	if (tr.flFraction >= 1.0f && (!tr.fInWater || !g_waypoint->MustJump(pev->origin, origin)))
		return true;

	return false;
}

bool Bot::IsEnemyInvincible(edict_t* entity)
{
	if (!ebot_check_enemy_invincibility.GetBool() || FNullEnt(entity))
		return false;

	const entvars_t& v = entity->v;
	if (v.solid < SOLID_BBOX)
		return true;

	if (v.flags & FL_GODMODE)
		return true;

	if (v.takedamage == DAMAGE_NO)
		return true;

	return false;
}

bool Bot::IsEnemyHidden(edict_t* entity)
{
	if (!ebot_check_enemy_rendering.GetBool() || FNullEnt(entity))
		return false;

	const entvars_t& v = entity->v;
	const bool enemyHasGun = (v.weapons & WeaponBits_Primary) || (v.weapons & WeaponBits_Secondary);
	const bool enemyGunfire = (v.buttons & IN_ATTACK) || (v.oldbuttons & IN_ATTACK);

	if ((v.renderfx == kRenderFxExplode || (v.effects & EF_NODRAW)) && (!enemyGunfire || !enemyHasGun))
		return true;

	if ((v.renderfx == kRenderFxExplode || (v.effects & EF_NODRAW)) && enemyGunfire && enemyHasGun)
		return false;

	if (v.renderfx != kRenderFxHologram && v.renderfx != kRenderFxExplode && v.rendermode != kRenderNormal)
	{
		if (v.renderfx == kRenderFxGlowShell)
		{
			if (v.renderamt <= 20.0f && v.rendercolor.x <= 20.0f && v.rendercolor.y <= 20.0f && v.rendercolor.z <= 20.0f)
			{
				if (!enemyGunfire || !enemyHasGun)
					return true;
				return false;
			}
			else if (!enemyGunfire && v.renderamt <= 60.0f && v.rendercolor.x <= 60.f && v.rendercolor.y <= 60.0f && v.rendercolor.z <= 60.0f)
				return true;
		}
		else if (v.renderamt <= 20.0f)
		{
			if (!enemyGunfire || !enemyHasGun)
				return true;
			return false;
		}
		else if (!enemyGunfire && v.renderamt <= 60.0f)
			return true;
	}

	return false;
}

void Bot::BaseUpdate(void)
{
	if (!pev)
		return;

	if (FNullEnt(m_myself))
	{
		m_myself = GetEntity();
		return;
	}

	const float tempTimer = engine->GetTime();
	if (m_baseUpdate < tempTimer)
	{
		// avoid frame drops
		m_frameInterval = tempTimer - m_frameDelay;
		m_frameDelay = tempTimer;
		m_baseUpdate = tempTimer + 0.1f;

		if (m_frameInterval < 0.0001f)
			return;

		pev->buttons = m_buttons;
		m_oldButtons = m_buttons;
		m_impulse = 0;
		m_buttons = 0;
		m_updateLooking = false;

		// if the bot hasn't selected stuff to start the game yet, go do that...
		if (m_notStarted)
			StartGame();
		else
		{
			if (m_isAlive)
			{
				if (!ebot_stopbots.GetBool())
				{
					DebugModeMsg();
					m_moveSpeed = 0.0f;
					m_strafeSpeed = 0.0f;
					UpdateProcess();
					MoveAction();
				}
				else
				{
					m_moveSpeed = 0.0f;
					m_strafeSpeed = 0.0f;
					ResetStuck();
				}
			}

			if (m_slowThinkTimer > tempTimer)
				m_isSlowThink = false;
			else
			{
				m_isSlowThink = true;
				CheckSlowThink();
				m_slowThinkTimer = tempTimer + crandomfloat(0.9f, 1.1f);
			}
		}
	}
	else
	{
		m_pathInterval = cminf(tempTimer - m_aimInterval, 0.05f);
		m_aimInterval = tempTimer;

		if (m_pathInterval < 0.0001f)
			return;

		if (pev->flags && pev->flags & FL_FROZEN)
		{
			m_msecVal = 0;
			pev->buttons = 0;
			pev->impulse = 0;
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;
		}
		else
		{
			m_msecVal = static_cast<uint8_t>((tempTimer - m_msecInterval) * 1000.0f);
			if (m_msecVal > static_cast<uint8_t>(255))
				m_msecVal = static_cast<uint8_t>(255);

			pev->buttons = static_cast<int>(m_buttons);
			pev->impulse = static_cast<int>(m_impulse);

			m_msecInterval = tempTimer;

			if (m_navNode.CanFollowPath() && CheckWaypoint())
			{
				m_strafeSpeed = 0.0f;
				m_moveSpeed = pev->maxspeed;
				DoWaypointNav();
				const Vector directionOld = ((m_destOrigin + pev->velocity * -m_pathInterval) - (pev->origin + pev->velocity * m_pathInterval)).Normalize2D();
				if (directionOld.GetLengthSquared2D() > 1.0f)
				{
					m_moveAngles = directionOld.ToAngles();
					m_moveAngles.x = -m_moveAngles.x; // invert for engine
					m_moveAngles.ClampAngles();
				}
				CheckStuck(directionOld, m_pathInterval);
			}
		}

		if (!m_lookVelocity.IsNull())
		{
			m_lookAt.x += m_lookVelocity.x * m_pathInterval;
			m_lookAt.y += m_lookVelocity.y * m_pathInterval;
			m_lookAt.z += m_lookVelocity.z * m_pathInterval;
		}

		// adjust all body and view angles to face an absolute vector
		Vector direction = (m_lookAt - EyePosition()).ToAngles() + pev->punchangle;
		direction.x = -direction.x; // invert for engine

		const float angleDiffPitch = AngleNormalize(direction.x - pev->v_angle.x);
		const float angleDiffYaw = AngleNormalize(direction.y - pev->v_angle.y);
		const float lockn = 0.128f / m_pathInterval;

		if (cabsf(angleDiffYaw) < lockn)
		{
			m_lookYawVel = 0.0f;
			pev->v_angle.y = AngleNormalize(direction.y);
		}
		else
		{
			const float fskill = static_cast<float>(m_skill);
			const float accelerate = fskill * 40.0f;
			m_lookYawVel += m_pathInterval * cclampf((fskill * 4.0f * angleDiffYaw) - (fskill * 0.4f * m_lookYawVel), -accelerate, accelerate);
			pev->v_angle.y += m_pathInterval * m_lookYawVel;
		}

		if (cabsf(angleDiffPitch) < lockn)
		{
			m_lookPitchVel = 0.0f;
			pev->v_angle.x = AngleNormalize(direction.x);
		}
		else
		{
			const float fskill = static_cast<float>(m_skill);
			const float accelerate = fskill * 40.0f;
			m_lookPitchVel += m_pathInterval * cclampf(fskill * 8.0f * angleDiffPitch - (fskill * 0.4f * m_lookPitchVel), -accelerate, accelerate);
			pev->v_angle.x += m_pathInterval * m_lookPitchVel;
		}

		// set the body angles to point the gun correctly
		pev->v_angle.ClampAngles();
		pev->angles.x = -pev->v_angle.x * 0.33333333333f;
		pev->angles.y = pev->v_angle.y;
		pev->angles.ClampAngles();

		// FIXME: crash
		g_engfuncs.pfnRunPlayerMove(m_myself, m_moveAngles, m_moveSpeed, m_strafeSpeed, 0.0f, m_buttons, m_impulse, m_msecVal);
	}
}

int GetMaxClip(const int& id)
{
	switch (id)
	{
		case Weapon::M249:
			return 90;
		case Weapon::P90:
			return 45;
		case Weapon::Galil:
			return 32;
		case Weapon::Ump45:
		case Weapon::Famas:
			return 23;
		case Weapon::Glock18:
		case Weapon::FiveSeven:
		case Weapon::G3SG1:
			return 19;
		case Weapon::P228:
			return 12;
		case Weapon::Usp:
			return 11;
		case Weapon::Awp:
		case Weapon::Scout:
			return 9;
		case Weapon::M3:
			return 7;
		case Weapon::Deagle:
		case Weapon::Xm1014:
			return 6;
		default:
			return 27;
	}

	return 27;
}

void Bot::CheckSlowThink(void)
{
	const float tempTimer = engine->GetTime();
	if (m_waypointTime < tempTimer)
	{
		if (m_currentWaypointIndex == m_zhCampPointIndex || m_currentWaypointIndex == m_myMeshWaypoint)
			m_waypointTime = tempTimer + 7.0f;
		else
		{
			m_navNode.Clear();
			FindWaypoint();
		}
	}
	else
		CalculatePing();

	if (m_isZombieBot)
		SelectKnife();
	else
	{
		if (!m_currentWeapon)
			SelectBestWeapon();
		else if (!m_hasEnemiesNear && !m_hasEntitiesNear && GetCurrentState() != Process::ThrowHE && GetCurrentState() != Process::ThrowFB && GetCurrentState() != Process::ThrowSM && !(m_buttons & IN_ATTACK) && !(m_oldButtons & IN_ATTACK))
		{
			m_isEnemyReachable = false;
			if (crandomint(1, 3) != 1)
			{
				if (!(m_buttons & IN_RELOAD) && !(m_oldButtons & IN_RELOAD))
					m_buttons |= IN_RELOAD;
			}
			else
			{
				WeaponSelect* selectTab = &g_weaponSelect[0];
				int i, id;
				int chosenWeaponIndex = -1;
				for (i = Const_NumWeapons; i; i--)
				{
					id = selectTab[i].id;
					if (pev->weapons & (1 << id))
					{
						if (m_ammo[g_weaponDefs[id].ammo1] && m_ammoInClip[id] != -1 && m_ammoInClip[id] < GetMaxClip(id))
						{
							chosenWeaponIndex = i;
							break;
						}
					}
				}

				if (chosenWeaponIndex != -1)
				{
					if (m_currentWeapon != selectTab[chosenWeaponIndex].id)
						SelectWeaponByName(selectTab[chosenWeaponIndex].weaponName);
				}
				else if (m_currentWaypointIndex == m_zhCampPointIndex || m_currentWaypointIndex == m_myMeshWaypoint)
					SelectBestWeapon();
				else if (m_navNode.CanFollowPath() && m_navNode.HasNext())
					SelectKnife();
			}
		}
	}
	
	if (!g_roundEnded && m_randomAttackTimer < tempTimer && g_DelayTimer < tempTimer) // simulate players with random knife attacks
	{
		extern ConVar ebot_ignore_enemies;
		if (!ebot_ignore_enemies.GetBool())
		{
			if (m_isStuck && m_personality != Personality::Careful)
			{
				if (m_personality == Personality::Rusher)
					m_randomAttackTimer = 0.0f;
				else
					m_randomAttackTimer = tempTimer + crandomfloat(0.1f, 10.0f);
			}
			else if (m_personality == Personality::Rusher)
				m_randomAttackTimer = tempTimer + crandomfloat(0.1f, 30.0f);
			else if (m_personality == Personality::Careful)
				m_randomAttackTimer = tempTimer + crandomfloat(10.0f, 100.0f);
			else
				m_randomAttackTimer = tempTimer + crandomfloat(0.15f, 75.0f);

			if (m_currentWeapon == Weapon::Knife)
			{
				if (crandomint(1, 3) == 1)
					m_buttons |= IN_ATTACK;
				else
					m_buttons |= IN_ATTACK2;
			}
		}
	}

	if (g_roundEnded)
	{
		m_team = Team::Terrorist;
		m_isZombieBot = true;
	}
	else
	{
		m_team = GetTeam(m_myself);
		if (m_isZombieBot != IsZombieEntity(m_myself))
		{
			m_isZombieBot = IsZombieEntity(m_myself);
			m_navNode.Clear();
			FindWaypoint();
			FindEnemyEntities();
			FindFriendsAndEnemiens();
		}

		if (!m_isZombieBot && IsValidWaypoint(m_zhCampPointIndex) && !g_waypoint->m_zmHmPoints.IsEmpty())
		{
			const Path zhPath = g_waypoint->m_paths[m_zhCampPointIndex];
			if (!(zhPath.flags & WAYPOINT_ZMHMCAMP) && !(zhPath.flags & WAYPOINT_HMCAMPMESH))
			{
				m_zhCampPointIndex = -1;
				FindGoalHuman();
			}
		}

		if (m_hasEnemiesNear)
		{
			if ((m_isEnemyReachable || m_currentWaypointIndex == m_zhCampPointIndex) && chanceof(ebot_use_grenade_percent.GetInt()) && !FNullEnt(m_nearestEnemy))
				CheckGrenadeThrow(m_nearestEnemy);
		}
		else
		{
			if (ebot_use_flare.GetBool() && !g_roundEnded && g_DelayTimer < tempTimer && !m_isZombieBot && !m_hasEnemiesNear && !FNullEnt(m_nearestEnemy))
			{
				if (pev->weapons & (1 << Weapon::SmGrenade) && chanceof(10) && (pev->origin - m_lookAt).GetLengthSquared2D() > squaredf(256.0f))
				{
					m_throw = m_lookAt;
					SetProcess(Process::ThrowSM, "throwing flare", false, tempTimer + 5.0f);
				}
			}
		}
	}

	m_isAlive = IsAlive(m_myself);
	m_index = GetIndex() - 1;

	// zp & biohazard flashlight support
	if (ebot_force_flashlight.GetBool())
	{
		if (!m_isZombieBot && !(pev->effects & EF_DIMLIGHT) && m_impulse != 100)
			m_impulse = 100;
	}
	else
	{
		if (pev->effects & EF_DIMLIGHT && m_impulse != 100)
			m_impulse = 100;
	}
}

bool Bot::IsAttacking(const edict_t* player)
{
	return (player->v.buttons & IN_ATTACK || player->v.oldbuttons & IN_ATTACK);
}

void Bot::UpdateLooking(void)
{
	if (m_isZombieBot)
	{
		if (m_hasEntitiesNear && m_entityDistance < 384.0f && (m_entityDistance < m_enemyDistance || !m_hasEnemiesNear))
		{
			if (!FNullEnt(m_nearestEntity))
			{
				LookAt(GetBoxOrigin(m_nearestEntity), m_nearestEntity->v.velocity);
				FireWeapon(m_entityDistance);
				return;
			}
		}

		if (m_hasEnemiesNear && (m_isEnemyReachable || m_enemyDistance < 384.0f) && IsAlive(m_nearestEnemy))
		{
			if (CheckVisibility(m_nearestEnemy))
			{
				LookAt(m_enemyOrigin, m_nearestEnemy->v.velocity);
				FireWeapon(m_enemyDistance);
			}
			else
				LookAt(m_enemyOrigin);
			return;
		}

		LookAtAround();
		return;
	}

	if (m_hasEntitiesNear && (m_entityDistance < m_enemyDistance || !m_hasEnemiesNear))
	{
		if (!FNullEnt(m_nearestEntity))
		{
			LookAt(GetBoxOrigin(m_nearestEntity), m_nearestEntity->v.velocity);
			FireWeapon(m_entityDistance);
			return;
		}
	}

	if (m_hasEnemiesNear && IsAlive(m_nearestEnemy))
	{
		if (CheckVisibility(m_nearestEnemy))
		{
			LookAt(m_enemyOrigin, m_nearestEnemy->v.velocity);
			FireWeapon(m_enemyDistance);
		}
		else
			LookAt(m_enemyOrigin);
		return;
	}

	if (m_enemySeeTime + 8.0f > engine->GetTime() && m_team != GetTeam(m_nearestEnemy))
	{
		if (IsAlive(m_nearestEnemy))
			LookAt(m_nearestEnemy->v.origin + m_nearestEnemy->v.view_ofs, m_nearestEnemy->v.velocity);
		else
			LookAt(m_enemyOrigin);

		return;
	}

	LookAtAround();
}

void Bot::LookAtAround(void)
{
	m_updateLooking = true;
	m_lookVelocity = nullvec;
	if (m_waypoint.flags & WAYPOINT_USEBUTTON)
	{
		const Vector origin = m_waypoint.origin;
		if ((pev->origin - origin).GetLengthSquared() < squaredf(80.0f))
		{
			if (g_gameVersion & Game::Xash)
			{
				m_lookAt = origin;
				if (!(m_buttons & IN_USE) && !(m_oldButtons & IN_USE))
					m_buttons |= IN_USE;
				return;
			}
			else
			{
				edict_t* button = FindButton();
				if (button) // no need to look at, thanks to the game...
					MDLL_Use(button, m_myself);
				else
				{
					m_lookAt = origin;
					if (!(m_buttons & IN_USE) && !(m_oldButtons & IN_USE))
						m_buttons |= IN_USE;
					return;
				}
			}
		}
	}
	else if (m_waypoint.flags & WAYPOINT_LADDER || IsOnLadder())
	{
		m_lookAt = m_destOrigin + pev->view_ofs;
		m_pauseTime = engine->GetTime() + 1.0f;
		return;
	}
	else if (m_isZombieBot)
	{
		if (m_navNode.HasNext())
		{
			const Path* next = g_waypoint->GetPath(m_navNode.Next());
			if (next)
				m_lookAt = next->origin + pev->view_ofs;
		}
		else
		{
			m_lookAt.x = m_destOrigin.x + pev->view_ofs.x;
			m_lookAt.y = m_destOrigin.y + pev->view_ofs.y;
			m_lookAt.z = pev->origin.z + pev->view_ofs.z;
		}
		
		m_pauseTime = engine->GetTime() + 1.0f;
		return;
	}
	else if (m_hasFriendsNear && !FNullEnt(m_nearestFriend) && IsAttacking(m_nearestFriend) && cstrncmp(STRING(m_nearestFriend->v.viewmodel), "models/v_knife", 14))
	{
		const Bot* bot = g_botManager->GetBot(m_nearestFriend);
		if (bot)
		{
			m_lookAt = bot->m_lookAt;
			if (bot->m_hasEnemiesNear)
			{
				edict_t* enemy = bot->m_nearestEnemy;
				if (!FNullEnt(enemy) && CheckGrenadeThrow(enemy))
					return;
			}
		}
		else
		{
			TraceResult tr;
			const Vector eyePosition = m_nearestFriend->v.origin + m_nearestFriend->v.view_ofs;
			MakeVectors(m_nearestFriend->v.angles);
			TraceLine(eyePosition, eyePosition + g_pGlobals->v_forward * 2000.0f, TraceIgnore::Nothing, m_nearestFriend, &tr);

			if (!FNullEnt(tr.pHit) && tr.pHit != m_myself)
				m_lookAt = GetEntityOrigin(tr.pHit);
			else
				m_lookAt = tr.vecEndPos;
		}

		m_pauseTime = engine->GetTime() + crandomfloat(2.0f, 7.0f);
		if (m_currentWeapon == Weapon::Knife)
			SelectBestWeapon();

		return;
	}

	const float time2 = engine->GetTime();
	if (m_pauseTime > time2)
		return;

	if (m_isSlowThink)
	{
		if (chanceof(m_senseChance)) // who's footsteps is this or fire sounds?
		{
			float maxDist;
			for (const auto& client : g_clients)
			{
				if (FNullEnt(client.ent))
					continue;

				// we can see our friends in radar... let bots act like that.
				if (client.team == m_team)
					continue;

				if (!(client.flags & CFLAG_USED))
					continue;

				if (!(client.flags & CFLAG_ALIVE))
					continue;

				if (client.ent->v.flags & FL_NOTARGET)
					continue;

				maxDist = 1280.0f;
				if (!IsAttacking(client.ent))
				{
					if ((client.ent->v.flags & FL_DUCKING))
						continue;

					if (client.ent->v.speed < client.ent->v.maxspeed * 0.66f)
						continue;

					maxDist = 768.0f;
				}

				if (((pev->origin + pev->velocity) - (client.ent->v.origin + client.ent->v.velocity)).GetLengthSquared() > squaredf(maxDist))
					continue;

				if (m_skill > (m_personality == Personality::Rusher ? 30 : 10))
					SelectBestWeapon();

				m_pauseTime = time2 + crandomfloat(2.0f, 7.0f);
				m_lookAt = client.ent->v.origin + client.ent->v.view_ofs;
				CheckGrenadeThrow(client.ent);
				return;
			}
		}
	}

	if (m_searchTime > time2)
	{
		if (m_navNode.HasNext())
		{
			const Path* next = g_waypoint->GetPath(m_navNode.Next());
			if (next)
				m_lookAt = next->origin + pev->view_ofs;
		}

		return;
	}

	const Vector nextFrame = EyePosition() + pev->velocity;
	Vector selectRandom;
	Vector bestLookPos = nextFrame;
	bestLookPos.x += crandomfloat(-1024.0f, 1024.0f);
	bestLookPos.y += crandomfloat(-1024.0f, 1024.0f);
	bestLookPos.z += crandomfloat(-256.0f, 256.0f);

	const int16_t index = g_waypoint->FindNearest(bestLookPos, 999999.0f);
	if (IsValidWaypoint(index) && m_knownWaypointIndex[0] != index && m_knownWaypointIndex[1] != index && m_knownWaypointIndex[2] != index && m_knownWaypointIndex[3] != index && m_knownWaypointIndex[4] != index && m_knownWaypointIndex[5] != index)
	{
		const Path* pointer = g_waypoint->GetPath(index);
		if (pointer)
		{
			const Vector waypointOrigin = pointer->origin;
			const float waypointRadius = static_cast<float>(pointer->radius);
			selectRandom.x = waypointOrigin.x + crandomfloat(-waypointRadius, waypointRadius);
			selectRandom.y = waypointOrigin.y + crandomfloat(-waypointRadius, waypointRadius);
			selectRandom.z = waypointOrigin.z + pev->view_ofs.z;
			m_knownWaypointIndex[0] = index;
			m_knownWaypointIndex[1] = m_knownWaypointIndex[0];
			m_knownWaypointIndex[2] = m_knownWaypointIndex[1];
			m_knownWaypointIndex[3] = m_knownWaypointIndex[2];
			m_knownWaypointIndex[4] = m_knownWaypointIndex[3];
			m_knownWaypointIndex[5] = m_knownWaypointIndex[4];
		}
	}

	if ((pev->origin - selectRandom).GetLengthSquared() > squaredf(512.0f) && IsVisible(selectRandom, m_myself))
	{
		m_lookAt = selectRandom;
		m_pauseTime = time2 + crandomfloat(1.05f, 1.45f);
	}

	m_searchTime = time2 + crandomfloat(0.2f, 0.3f);
}

void Bot::CalculatePing(void)
{
	extern ConVar ebot_ping;
	if (!ebot_ping.GetBool())
		return;

	// save cpu power if no one is lookin' at scoreboard...
	if (g_fakePingUpdate < engine->GetTime())
		return;

	int averagePing = 0;
	int numHumans = 0;
	int ping, loss;

	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (IsValidBot(client.index))
			continue;

		numHumans++;
		PLAYER_CNX_STATS(client.ent, &ping, &loss);

		if (!ping || ping > 150)
			ping = crandomint(5, 50);

		averagePing += ping;
	}

	if (numHumans)
		averagePing /= numHumans;
	else
		averagePing = crandomint(30, 60);

	int botPing = m_basePingLevel + crandomint(averagePing - averagePing / 5, averagePing + averagePing / 5) + crandomint(m_difficulty + 3, m_difficulty + 6);
	if (botPing < 9)
		botPing = crandomint(9, 19);
	else if (botPing > 133)
		botPing = crandomint(99, 119);

	int j;
	for (j = 0; j < 2; j++)
	{
		for (m_pingOffset[j] = 0; m_pingOffset[j] < 4; m_pingOffset[j]++)
		{
			if (!((botPing - m_pingOffset[j]) % 4))
			{
				m_ping[j] = (botPing - m_pingOffset[j]) / 4;
				break;
			}
		}
	}

	m_ping[2] = botPing;
}

void Bot::MoveAction(void)
{
	if (m_buttons & IN_JUMP)
		m_jumpTime = engine->GetTime();

	if (m_jumpTime + 2.0f > engine->GetTime())
	{
		if (!IsOnFloor() && !IsInWater() && !IsOnLadder())
		{
			pev->flags |= FL_DUCKING;
			m_buttons |= IN_DUCK;
		}
	}
	else
	{
		if (IsInWater())
		{
			if (InFieldOfView(m_destOrigin - EyePosition()) > 90.0f)
				m_buttons |= IN_BACK;
			else
				m_buttons |= IN_FORWARD;

			if (m_moveAngles.x > 60.0f)
				m_buttons |= IN_DUCK;
			else if (m_moveAngles.x < -60.0f)
				m_buttons |= IN_JUMP;
		}
		else if (IsOnLadder())
		{
			if (InFieldOfView(m_destOrigin - EyePosition()) < 180.0f)
				m_buttons |= IN_FORWARD;
			else
			 	m_buttons |= IN_BACK;
		}
		else
		{
			if (m_moveSpeed > 0.0f)
				m_buttons |= IN_FORWARD;
			else if (m_moveSpeed < 0.0f)
				m_buttons |= IN_BACK;

			if (m_strafeSpeed > 0.0f)
				m_buttons |= IN_MOVERIGHT;
			else if (m_strafeSpeed < 0.0f)
				m_buttons |= IN_MOVELEFT;
		}

		if (m_duckTime > engine->GetTime())
			m_buttons |= IN_DUCK;
	}
}

void Bot::DebugModeMsg(void)
{
	if (!ebot_debug.GetBool())
		return;

	static float timeDebugUpdate = 0.0f;
	static int16_t index{}, goal;
	static Process processID{}, rememberedProcessID;
	static edict_t* mi;
	for (const auto& player : g_clients)
	{
		mi = player.ent;
		if (!IsValidPlayer(mi))
			continue;

		if (IsValidBot(mi))
			continue;

		if (mi->v.iuser2 != m_index + 1)
			continue;

		if (timeDebugUpdate < engine->GetTime())
		{
			if (!FNullEnt(g_helicopter))
			{
				const Vector center = GetBoxOrigin(g_helicopter);
				extern ConVar ebot_helicopter_width;
				MakeVectors(g_helicopter->v.angles);
				const Vector right = center + g_pGlobals->v_right * ebot_helicopter_width.GetFloat();
				const Vector left = center - g_pGlobals->v_right * ebot_helicopter_width.GetFloat();
				const Color col = Color(255, 0, 255, 255);
				engine->DrawLine(mi, center, right, col, 5, 0, 0, 10);
				engine->DrawLine(mi, center, left, col, 5, 0, 0, 10);
			}

			processID = m_currentProcess;
			index = m_currentWaypointIndex;
			goal = m_currentGoalIndex;

			char processName[80];
			sprintf(processName, "%s", GetProcessName(m_currentProcess));

			char rememberedProcessName[80];
			sprintf(rememberedProcessName, "%s", GetProcessName(m_rememberedProcess));

			char weaponName[80], botType[32];
			char enemyName[80], friendName[80];

			if (IsAlive(m_nearestEntity) && m_entityDistance < m_enemyDistance)
				sprintf(enemyName, "%s", GetEntityName(m_nearestEntity));
			else if (IsAlive(m_nearestEnemy))
				sprintf(enemyName, "%s", GetEntityName(m_nearestEnemy));
			else
				sprintf(enemyName, "%s", GetEntityName(nullptr));

			if (IsAlive(m_nearestFriend))
				sprintf(friendName, "%s", GetEntityName(m_nearestFriend));
			else
				sprintf(friendName, "%s", GetEntityName(nullptr));

			char weaponCount = 0;
			WeaponSelect* selectTab = &g_weaponSelect[0];
			while (m_currentWeapon != selectTab[weaponCount].id && weaponCount < Const_NumWeapons)
				weaponCount++;

			// set the bot type
			sprintf(botType, "%s%s%s", m_personality == Personality::Rusher ? "Rusher" : "",
				m_personality == Personality::Careful ? "Careful" : "",
				m_personality == Personality::Normal ? "Normal" : "");

			if (weaponCount >= Const_NumWeapons)
			{
				// prevent printing unknown message from known weapons
				switch (m_currentWeapon)
				{
					case Weapon::HeGrenade:
					{
						sprintf(weaponName, "weapon_hegrenade");
						break;
					}
					case Weapon::FbGrenade:
					{
						sprintf(weaponName, "weapon_flashbang");
						break;
					}
					case Weapon::SmGrenade:
					{
						sprintf(weaponName, "weapon_smokegrenade");
						break;
					}
					case Weapon::C4:
					{
						sprintf(weaponName, "weapon_c4");
						break;
					}
					default:
						sprintf(weaponName, "Unknown! (%d)", m_currentWeapon);
				}
			}
			else
				sprintf(weaponName, "%s", selectTab[weaponCount].weaponName);

			char outputBuffer[512];
			sprintf(outputBuffer, "\n\n\n\n\n\n\n"
				"\n %s \n Process: %s (%i) RE-Process: %s (%i) \n"
				"Weapon: %s  Clip: %d  Ammo: %d \n"
				"Type: %s  Total Path: %d \n"
				"Enemy: %s  Friend: %s \n\n"

				"Current Index: %d  Goal Index: %d  Enemy Reachable: %s \n"
				"Nav: %d  Next Nav: %d  Enemy Distance: %2.f \n"
				"Move Speed: %2.f  Strafe Speed: %2.f  Velocity: %2.f \n"
				"Stuck Time: %2.f  Stuck: %s \n",
				GetEntityName(m_myself), processName, static_cast<int>(m_currentProcess), rememberedProcessName, static_cast<int>(m_rememberedProcess),
				&weaponName[7], GetAmmoInClip(), m_ammo[g_weaponDefs[m_currentWeapon].ammo1],
				botType, m_navNode.Length(),
				enemyName, friendName,

				m_currentWaypointIndex, goal, m_isEnemyReachable ? "Yes" : "No",
				m_navNode.IsEmpty() ? -1 : m_navNode.First(), m_navNode.HasNext() ? m_navNode.Next() : -1, m_enemyDistance,
				m_moveSpeed, m_strafeSpeed, pev->velocity.GetLength2D(),
				m_stuckTime, m_isStuck ? "Yes" : "No");

			MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, nullptr, mi);
			WRITE_BYTE(TE_TEXTMESSAGE);
			WRITE_BYTE(1);
			WRITE_SHORT(FixedSigned16(-1.0f, (1 << 13)));
			WRITE_SHORT(FixedSigned16(0.0f, (1 << 13)));
			WRITE_BYTE(0);
			WRITE_BYTE(m_team == Team::Counter ? 0 : 255);
			WRITE_BYTE(100);
			WRITE_BYTE(m_team != Team::Counter ? 0 : 255);
			WRITE_BYTE(0);
			WRITE_BYTE(255);
			WRITE_BYTE(255);
			WRITE_BYTE(255);
			WRITE_BYTE(0);
			WRITE_SHORT(FixedUnsigned16(0.0f, (1 << 8)));
			WRITE_SHORT(FixedUnsigned16(0.0f, (1 << 8)));
			WRITE_SHORT(FixedUnsigned16(1.0f, (1 << 8)));
			WRITE_STRING(const_cast<const char*>(&outputBuffer[0]));
			MESSAGE_END();

			timeDebugUpdate = engine->GetTime() + 0.6f;
		}

		if (m_hasEnemiesNear && !m_enemyOrigin.IsNull())
			engine->DrawLine(mi, EyePosition(), m_enemyOrigin, Color(255, 0, 0, 255), 10, 0, 5, 1, LINE_SIMPLE);
		else if (m_isZombieBot && !FNullEnt(m_nearestEnemy))
			engine->DrawLine(mi, EyePosition(), m_nearestEnemy->v.origin, Color(255, 0, 0, 255), 10, 0, 5, 1, LINE_SIMPLE);

		if (m_navNode.HasNext())
		{
			int16_t i;
			for (i = 0; i < m_navNode.Length() - 1; i++)
				engine->DrawLine(mi, g_waypoint->GetPath(m_navNode.Get(i))->origin, g_waypoint->GetPath(m_navNode.Get(i + 1))->origin, Color(255, 100, 55, 255), 15, 0, 8, 1, LINE_SIMPLE);
		}

		if (IsValidWaypoint(m_currentWaypointIndex))
		{
			if (m_currentTravelFlags & PATHFLAG_JUMP)
				engine->DrawLine(mi, pev->origin, m_destOrigin, Color(255, 0, 0, 255), 15, 0, 8, 1, LINE_SIMPLE);
			else if (m_currentTravelFlags & PATHFLAG_DOUBLE)
				engine->DrawLine(mi, pev->origin, m_destOrigin, Color(0, 0, 255, 255), 15, 0, 8, 1, LINE_SIMPLE);
			else
				engine->DrawLine(mi, pev->origin, m_destOrigin, Color(0, 255, 0, 255), 15, 0, 8, 1, LINE_SIMPLE);

			if (m_waypoint.radius > 4)
			{
				const float root = static_cast<float>(m_waypoint.radius);
				const Color& def = Color(0, 0, 255, 255);
				engine->DrawLine(mi, m_waypoint.origin + Vector(root, root, 0.0f), m_waypoint.origin + Vector(-root, root, 0.0f), def, 5, 0, 0, 10);
				engine->DrawLine(mi, m_waypoint.origin + Vector(root, root, 0.0f), m_waypoint.origin + Vector(root, -root, 0.0f), def, 5, 0, 0, 10);
				engine->DrawLine(mi, m_waypoint.origin + Vector(-root, -root, 0.0f), m_waypoint.origin + Vector(root, -root, 0.0f), def, 5, 0, 0, 10);
				engine->DrawLine(mi, m_waypoint.origin + Vector(-root, -root, 0.0f), m_waypoint.origin + Vector(-root, root, 0.0f), def, 5, 0, 0, 10);
			}
		}
	}
}

// this function gets called by network message handler, when screenfade message get's send
// it's used to make bot blind froumd the grenade
void Bot::TakeBlinded(const Vector& fade, const int alpha)
{
	if (fade.x != 255 || fade.y != 255 || fade.z != 255 || alpha <= 170)
		return;

	SetProcess(Process::Blind, "i'm blind", false, engine->GetTime() + crandomfloat(3.2f, 6.4f));
}

Vector Bot::CheckToss(const Vector& start, const Vector& stop)
{
	Vector end = stop - pev->velocity;
	end.z -= 15.0f;

	if (cabsf(end.z - start.z) > 500.0f)
		return nullvec;

	TraceResult tr;
	Vector midPoint = start + (end - start) * 0.5f;
	TraceHull(midPoint, midPoint + Vector(0.0f, 0.0f, 500.0f), TraceIgnore::Nothing, head_hull, m_myself, &tr);

	if (tr.flFraction < 1.0f && !FNullEnt(tr.pHit))
	{
		midPoint = tr.vecEndPos;
		midPoint.z = tr.pHit->v.absmin.z - 1.0f;
	}

	if (midPoint.z < start.z || midPoint.z < end.z)
		return nullvec;

	const float gravity = engine->GetGravity() * 0.55f;
	const float timeOne = csqrtf((midPoint.z - start.z) / (0.5f * gravity));
	if (timeOne < 0.1f)
		return nullvec;

	Vector velocity = (end - start) / (timeOne + csqrtf((midPoint.z - end.z) / (0.5f * gravity)));
	velocity.z = gravity * timeOne;

	Vector apex = start + velocity * timeOne;
	apex.z = midPoint.z;

	TraceHull(start, apex, TraceIgnore::Nothing, head_hull, m_myself, &tr);
	if (tr.flFraction < 1.0f || tr.fAllSolid)
		return nullvec;

	TraceHull(end, apex, TraceIgnore::Monsters, head_hull, m_myself, &tr);
	if (tr.flFraction < 1.0f)
	{
		if (-(tr.vecPlaneNormal | (apex - end).Normalize()) > 0.75f || tr.flFraction < 0.8f)
			return nullvec;
	}

	return velocity * 0.777f;
}

Vector Bot::CheckThrow(const Vector& start, const Vector& end)
{
	Vector velocity = end - start;
	float time = velocity.GetLength() * 0.00512820512f;
	if (time < 0.01f)
		return nullvec;

	if (time > 2.0f)
		time = 1.2f;

	const float gravity = engine->GetGravity() * 0.55f;
	const float half = time * 0.5f;

	velocity = velocity * (1.0f / time);
	velocity.z += gravity * half * half;

	Vector apex = start + (end - start) * 0.5f;
	apex.z += 0.5f * gravity * half;

	TraceResult tr;
	TraceHull(start, apex, TraceIgnore::Nothing, head_hull, m_myself, &tr);
	if (tr.flFraction < 1.0f)
		return nullvec;

	TraceHull(end, apex, TraceIgnore::Monsters, head_hull, m_myself, &tr);
	if (tr.flFraction < 1.0f || tr.fAllSolid)
	{
		if (-(tr.vecPlaneNormal | (apex - end).Normalize()) > 0.75f || tr.flFraction < 0.8f)
			return nullvec;
	}

	return velocity * 0.7793f;
}