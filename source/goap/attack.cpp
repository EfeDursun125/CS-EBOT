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

	const float time = engine->GetTime();

	FindFriendsAndEnemiens();
	FindEnemyEntities();
	LookAtEnemies();
	FacePosition();
	CheckReload();

	if (!m_hasEnemiesNear && !m_hasEntitiesNear)
	{
		if (g_bombPlanted && (m_team == Team::Counter || IsBombDefusing(g_waypoint->GetBombPosition())))
		{
			if (!FNullEnt(m_nearestEnemy) && IsAlive(m_nearestEnemy))
				CheckGrenadeThrow(m_nearestEnemy);

			FinishCurrentProcess("no target exist");
			return;
		}

		// health based wait time
		float wait = 2.0f - (pev->health / pev->max_health);

		if (m_personality == Personality::Careful)
			wait = 3.0f - (pev->health / pev->max_health);
		else if (m_personality == Personality::Rusher && (m_currentWeapon == Weapon::M3 || m_currentWeapon == Weapon::Xm1014 || m_currentWeapon == Weapon::M249))
			wait = 0.75f;

		// i'm alone
		if (!m_hasFriendsNear && m_numEnemiesLeft > 1)
			wait += 1.25f;

		// be careful
		if (m_isBomber || m_isVIP)
		{
			// let my team go first to protect me
			if (m_hasFriendsNear)
				wait *= 2.0f;
		}
		else
		{
			if (IsSniper())
			{
				const bool usesSniper = UsesSniper();
				if (!usesSniper)
					SelectBestWeapon();

				const float minRange = SquaredF(384.0f);
				const float distance = GetTargetDistance();
				if (distance > minRange)
				{
					if (!CheckWallOnBehind() && !CheckWallOnForward() && !CheckWallOnLeft() && !CheckWallOnRight())
					{
						if (usesSniper && pev->fov == 90.0f && !(pev->button & IN_ATTACK2) && !(pev->oldbuttons & IN_ATTACK2))
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

		if (m_enemySeeTime + wait < time && m_entitySeeTime + wait < time)
		{
			SetWalkTime(7.0f);
			FinishCurrentProcess("no target exist");
		}
		else if (!FNullEnt(m_nearestEnemy) && IsAlive(m_nearestEnemy))
			CheckGrenadeThrow(m_nearestEnemy);

		return;
	}
	else
		FireWeapon();

	const float distance = GetTargetDistance();
	const int melee = g_gameVersion & Game::HalfLife ? WeaponHL::Crowbar : Weapon::Knife;
	if (m_currentWeapon == melee)
	{
		if (IsEnemyReachable())
		{
			DeleteSearchNodes();
			MoveTo(m_enemyOrigin);
		}
		else
			FollowPath(m_enemyOrigin);

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

	if (g_gameVersion & Game::HalfLife)
	{
		if (m_currentWeapon == WeaponHL::Mp5_HL && distance > SquaredF(300.0f) && distance <= SquaredF(800.0f))
		{
			if (!(pev->oldbuttons & IN_ATTACK2) && !m_isSlowThink && CRandomInt(1, 3) == 1)
				pev->button |= IN_ATTACK2;
		}
		else if (m_currentWeapon == WeaponHL::Crowbar && m_personality != Personality::Careful)
			pev->button |= IN_ATTACK;
	}

	int approach;
	if (!m_hasEnemiesNear && !m_hasEntitiesNear) // if suspecting enemy stand still
		approach = 49;
	else if (!(g_gameVersion & Game::HalfLife) && (m_isReloading || m_isVIP)) // if reloading or vip back off
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
		m_lastFightStyleCheck = time;
	}
	else if (UsesRifle() || UsesSubmachineGun())
	{
		if (m_lastFightStyleCheck + 0.5f < time)
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

			m_lastFightStyleCheck = time;
		}
	}
	else
	{
		if (m_lastFightStyleCheck + 0.5f < time)
		{
			if (ChanceOf(75))
			{
				if (ChanceOf(50))
					m_fightStyle = 0;
				else
					m_fightStyle = 1;
			}

			m_lastFightStyleCheck = time;
		}
	}

	if (m_fightStyle == 0 || ((pev->button & IN_RELOAD) || m_isReloading) || (UsesPistol() && distance < SquaredF(768.0f)) || m_currentWeapon == melee)
	{
		if (m_strafeSetTime < time)
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

			m_strafeSetTime = time + CRandomFloat(0.5f, 3.0f);
		}

		if (m_combatStrafeDir == 0)
		{
			if (!CheckWallOnLeft())
				m_strafeSpeed = -pev->maxspeed;
			else
			{
				m_combatStrafeDir = 1;
				m_strafeSetTime = time + CRandomFloat(0.75, 1.5f);
			}
		}
		else
		{
			if (!CheckWallOnRight())
				m_strafeSpeed = pev->maxspeed;
			else
			{
				m_combatStrafeDir = 0;
				m_strafeSetTime = time + CRandomFloat(0.75, 1.5f);
			}
		}

		if (m_jumpTime + 2.0f < time && !IsOnLadder() && ChanceOf(m_isReloading ? 5 : 2) && !UsesSniper() && pev->velocity.GetLength2D() > float(m_skill + 50))
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
			m_duckTime = time + 1.0f;

		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;
	}

	if (m_duckTime > time)
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

	const Vector directionOld = m_enemyOrigin - pev->origin;
	m_moveAngles = directionOld.ToAngles();
	m_moveAngles.ClampAngles();
}

void Bot::AttackEnd(void)
{
	FindWaypoint();
}

bool Bot::AttackReq(void)
{
	return true;
}