#include <core.h>

void Bot::AttackStart(void)
{
	DeleteSearchNodes();
}

void Bot::AttackUpdate(void)
{
	if (m_isZombieBot)
	{
		FinishCurrentProcess("i am a zombie now");
		return;
	}

	FindFriendsAndEnemiens();
	FindEnemyEntities();
	LookAtEnemies();
	CheckReload();

	if (!m_hasEnemiesNear && !m_hasEntitiesNear)
	{
		// health based wait time
		float wait = 2.0f - (pev->health / pev->max_health);

		if (m_personality == PERSONALITY_CAREFUL)
			wait = 3.0f - (pev->health / pev->max_health);
		else if (m_personality == PERSONALITY_RUSHER && (m_currentWeapon == WEAPON_M3 || m_currentWeapon == WEAPON_XM1014 || m_currentWeapon == WEAPON_M249))
			wait = 0.5f;

		// be careful
		if (m_isBomber || m_isVIP)
		{
			// let my team go first
			if (m_hasFriendsNear > 0)
				wait *= 2.0f;
		}
		else
		{
			if (IsSniper())
			{
				if (!UsesSniper())
					SelectBestWeapon();

				const float minRange = SquaredF(384.0f);
				const float distance = GetTargetDistance();
				if (distance > minRange)
				{
					if (!CheckWallOnBehind() && !CheckWallOnForward() && !CheckWallOnLeft() && !CheckWallOnRight())
					{
						if (pev->fov == 90.0f && !(pev->button & IN_ATTACK2) && !(pev->oldbuttons & IN_ATTACK2))
							pev->button |= IN_ATTACK2;

						wait = cclampf(csqrtf(distance) * 0.01f, 5.0f, 15.0f);
					}
					else if (m_hasFriendsNear)
						wait = 5.0f;
				}
				else if (m_hasFriendsNear)
					wait = 5.0f;
			}
		}

		if (m_enemySeeTime + wait < engine->GetTime() && m_entitySeeTime + wait < engine->GetTime())
		{
			SetWalkTime(7.0f);
			FinishCurrentProcess("no target exist");
		}
		else
			CheckGrenadeThrow();

		return;
	}
	else
		FireWeapon();

	const float distance = m_enemyDistance;
	const int melee = g_gameVersion == HALFLIFE ? WEAPON_CROWBAR : WEAPON_KNIFE;
	if (m_currentWeapon == melee)
	{
		m_destOrigin = m_enemyOrigin;
		m_moveSpeed = pev->maxspeed;
		m_strafeSpeed = 0.0f;
		return;
	}
	else
	{
		if (distance <= SquaredF(256.0f)) // get back!
		{
			m_moveSpeed = -pev->maxspeed;
			return;
		}
	}

	if (g_gameVersion == HALFLIFE)
	{
		if (m_currentWeapon == WEAPON_MP5_HL && distance > SquaredF(300.0f) && distance <= SquaredF(800.0f))
		{
			if (!(pev->oldbuttons & IN_ATTACK2) && !m_isSlowThink && CRandomInt(1, 3) == 1)
				pev->button |= IN_ATTACK2;
		}
		else if (m_currentWeapon == WEAPON_CROWBAR && m_personality != PERSONALITY_CAREFUL)
			pev->button |= IN_ATTACK;
	}

	int approach;
	if (!m_hasEnemiesNear && !m_hasEntitiesNear) // if suspecting enemy stand still
		approach = 49;
	else if (g_gameVersion != HALFLIFE && (m_isReloading || m_isVIP)) // if reloading or vip back off
		approach = 29;
	else
	{
		approach = pev->health;
		if (UsesSniper() && approach > 49)
			approach = 49;
	}

	// only take cover when bomb is not planted and enemy can see the bot or the bot is VIP
	if (approach < 30 && !g_bombPlanted && (::IsInViewCone(m_enemyOrigin, GetEntity()) || m_isVIP))
	{
		m_moveSpeed = -pev->maxspeed;
		//PushTask(TASK_SEEKCOVER, TASKPRI_SEEKCOVER, -1, 0.0f, true);
	}
	else if (m_currentWeapon != melee) // if enemy cant see us, we never move
		m_moveSpeed = 0.0f;
	else if (approach >= 50 || UsesBadPrimary()) // we lost him?
		m_moveSpeed = pev->maxspeed;
	else
		m_moveSpeed = pev->maxspeed;

	if (UsesSniper())
	{
		m_fightStyle = 1;
		m_lastFightStyleCheck = engine->GetTime();
	}
	else if (UsesRifle() || UsesSubmachineGun())
	{
		if (m_lastFightStyleCheck + 0.5f < engine->GetTime())
		{
			if (ChanceOf(75))
			{
				if (distance < SquaredF(768.0f))
					m_fightStyle = 0;
				else if (distance < SquaredF(1024.0f))
				{
					if (ChanceOf(UsesSubmachineGun() ? 50 : 30))
						m_fightStyle = 0;
					else
						m_fightStyle = 1;
				}
				else
				{
					if (ChanceOf(UsesSubmachineGun() ? 80 : 93))
						m_fightStyle = 1;
					else
						m_fightStyle = 0;
				}
			}

			m_lastFightStyleCheck = engine->GetTime();
		}
	}
	else
	{
		if (m_lastFightStyleCheck + 0.5f < engine->GetTime())
		{
			if (ChanceOf(75))
			{
				if (ChanceOf(50))
					m_fightStyle = 0;
				else
					m_fightStyle = 1;
			}

			m_lastFightStyleCheck = engine->GetTime();
		}
	}

	if (m_fightStyle == 0 || ((pev->button & IN_RELOAD) || m_isReloading) || (UsesPistol() && distance < SquaredF(768.0f)) || m_currentWeapon == melee)
	{
		if (m_strafeSetTime < engine->GetTime())
		{
			// to start strafing, we have to first figure out if the target is on the left side or right side
			MakeVectors(m_nearestEnemy->v.v_angle);

			const Vector& dirToPoint = (pev->origin - m_enemyOrigin).Normalize2D();
			const Vector& rightSide = g_pGlobals->v_right.Normalize2D();

			if ((dirToPoint | rightSide) < 0)
				m_combatStrafeDir = 1;
			else
				m_combatStrafeDir = 0;

			if (ChanceOf(30))
				m_combatStrafeDir = (m_combatStrafeDir == 1 ? 0 : 1);

			m_strafeSetTime = engine->GetTime() + CRandomFloat(0.5f, 3.0f);
		}

		if (m_combatStrafeDir == 0)
		{
			if (!CheckWallOnLeft())
				m_strafeSpeed = -pev->maxspeed;
			else
			{
				m_combatStrafeDir = 1;
				m_strafeSetTime = engine->GetTime() + 1.5f;
			}
		}
		else
		{
			if (!CheckWallOnRight())
				m_strafeSpeed = pev->maxspeed;
			else
			{
				m_combatStrafeDir = 0;
				m_strafeSetTime = engine->GetTime() + 1.5f;
			}
		}

		if (m_jumpTime + 2.0f < engine->GetTime() && !IsOnLadder() && ChanceOf(m_isReloading ? 5 : 2) && !UsesSniper() && pev->velocity.GetLength2D() > float(m_skill + 50))
			pev->button |= IN_JUMP;

		if (m_moveSpeed > 0.0f && distance > SquaredF(512.0f) && m_currentWeapon != melee)
			m_moveSpeed = 0.0f;

		if (m_currentWeapon == melee)
			m_strafeSpeed = 0.0f;
	}
	else if (m_fightStyle == 1)
	{
		const Vector& src = pev->origin - Vector(0, 0, 18.0f);
		if (!(m_visibility & (Visibility::Head | Visibility::Body)) && IsVisible(src, m_nearestEnemy))
			m_duckTime = AddTime(1.0f);

		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;
	}

	if (m_duckTime > engine->GetTime())
	{
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;
	}

	if (m_isReloading)
	{
		m_moveSpeed = -pev->maxspeed;
		m_duckTime = 0.0f;
	}

	if (!IsInWater() && !IsOnLadder() && (m_moveSpeed > 0.0f || m_strafeSpeed >= 0.0f))
	{
		MakeVectors(pev->v_angle);

		if (IsDeadlyDrop(pev->origin + (g_pGlobals->v_forward * m_moveSpeed * 0.2f) + (g_pGlobals->v_right * m_strafeSpeed * 0.2f) + (pev->velocity * m_frameInterval)))
		{
			m_strafeSpeed = -m_strafeSpeed;
			m_moveSpeed = -m_moveSpeed;
			pev->button &= ~IN_JUMP;
		}
	}
}

void Bot::AttackEnd(void)
{
	ResetStuck();
	FindWaypoint();
}

bool Bot::AttackReq(void)
{
	return true;
}