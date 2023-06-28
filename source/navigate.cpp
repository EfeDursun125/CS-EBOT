#include <core.h>

ConVar ebot_zombies_as_path_cost("ebot_zombie_count_as_path_cost", "1");
ConVar ebot_aim_type("ebot_aim_type", "1");
ConVar ebot_has_semiclip("ebot_has_semiclip", "0");

int Bot::FindGoal(void)
{
	m_prevGoalIndex = m_chosenGoalIndex;

	if (IsZombieMode())
	{
		if (m_isZombieBot)
		{
			if (g_waypoint->m_terrorPoints.IsEmpty())
				return m_chosenGoalIndex = CRandomInt(0, g_numWaypoints - 1);
			else
				return m_chosenGoalIndex = g_waypoint->m_terrorPoints.GetRandomElement();
		}
		else if (IsValidWaypoint(m_myMeshWaypoint))
			return m_chosenGoalIndex = m_myMeshWaypoint;
		else if (IsValidWaypoint(m_zhCampPointIndex))
			return m_chosenGoalIndex = m_zhCampPointIndex;
		else
			return m_chosenGoalIndex = CRandomInt(0, g_numWaypoints - 1);
	}
	else if (GetGameMode() == MODE_BASE)
	{
		if (g_mapType & MAP_CS)
		{
			if (m_team == TEAM_COUNTER)
			{
				if (!g_waypoint->m_rescuePoints.IsEmpty() && HasHostage())
				{
					if (IsValidWaypoint(m_chosenGoalIndex) && g_waypoint->GetPath(m_chosenGoalIndex)->flags & WAYPOINT_RESCUE)
						return m_chosenGoalIndex;
					else
						return m_chosenGoalIndex = g_waypoint->m_rescuePoints.GetRandomElement();
				}
				else
				{
					if (!g_waypoint->m_goalPoints.IsEmpty() && (g_timeRoundMid <= engine->GetTime() || CRandomInt(1, 3) <= 2))
						return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
					else if (!g_waypoint->m_ctPoints.IsEmpty())
						return m_chosenGoalIndex = g_waypoint->m_ctPoints.GetRandomElement();
				}
			}
			else
			{
				if (!g_waypoint->m_rescuePoints.IsEmpty() && CRandomInt(1, 11) == 1)
					return m_chosenGoalIndex = g_waypoint->m_rescuePoints.GetRandomElement();
				else if (!g_waypoint->m_goalPoints.IsEmpty() && CRandomInt(1, 3) == 1)
					return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
				else if (!g_waypoint->m_terrorPoints.IsEmpty())
					return m_chosenGoalIndex = g_waypoint->m_terrorPoints.GetRandomElement();
			}
		}
		else if (g_mapType & MAP_AS)
		{
			if (m_team == TEAM_COUNTER)
			{
				if (m_isVIP && !g_waypoint->m_goalPoints.IsEmpty())
					return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
				else
				{
					if (!g_waypoint->m_goalPoints.IsEmpty() && CRandomInt(1, 2) == 1)
						return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
					else if (!g_waypoint->m_ctPoints.IsEmpty())
						return m_chosenGoalIndex = g_waypoint->m_ctPoints.GetRandomElement();
				}
			}
			else
			{
				if (!g_waypoint->m_goalPoints.IsEmpty() && CRandomInt(1, 11) == 1)
					return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
				else if (!g_waypoint->m_terrorPoints.IsEmpty())
					return m_chosenGoalIndex = g_waypoint->m_terrorPoints.GetRandomElement();
			}
		}
		else if (g_mapType & MAP_DE)
		{
			if (g_bombPlanted)
			{
				const bool noTimeLeft = OutOfBombTimer();

				if (noTimeLeft)
					SetProcess(Process::Escape, "escaping from the bomb", true, 60.0f);
				else if (m_team == TEAM_COUNTER)
				{
					const Vector bombOrigin = g_waypoint->GetBombPosition();
					if (bombOrigin != nullvec)
					{
						if (IsBombDefusing(bombOrigin))
						{
							const int index = FindDefendWaypoint(bombOrigin);
							if (IsValidWaypoint(index))
							{
								m_campIndex = index;
								SetProcess(Process::Camp, "i will protect my teammate", true, AddTime(GetBombTimeleft()));
								return m_chosenGoalIndex = index;;
							}
						}
						else
						{
							if (g_bombSayString)
							{
								ChatMessage(CHAT_PLANTBOMB);
								g_bombSayString = false;
							}

							const int index = g_waypoint->FindNearest(bombOrigin, 999999.0f);
							if (IsValidWaypoint(index))
								return m_chosenGoalIndex = index;
						}
					}
				}
				else
				{
					const Vector bombOrigin = g_waypoint->GetBombPosition();
					if (bombOrigin != nullvec)
					{
						if (IsBombDefusing(bombOrigin))
						{
							const int index = g_waypoint->FindNearest(bombOrigin, 999999.0f);
							if (IsValidWaypoint(index))
								return m_chosenGoalIndex = index;
						}
						else
						{
							const int index = FindDefendWaypoint(bombOrigin);
							if (IsValidWaypoint(index))
							{
								m_campIndex = index;
								SetProcess(Process::Camp, "i will defend the bomb", true, AddTime(GetBombTimeleft()));
								return m_chosenGoalIndex = index;
							}
						}
					}
				}
			}
			else
			{
				if (m_isSlowThink)
					m_loosedBombWptIndex = FindLoosedBomb();

				if (IsValidWaypoint(m_loosedBombWptIndex))
				{
					if (m_team == TEAM_COUNTER)
					{
						const int index = FindDefendWaypoint(g_waypoint->GetPath(m_loosedBombWptIndex)->origin);
						if (IsValidWaypoint(index))
						{
							extern ConVar ebot_camp_max;
							m_campIndex = index;
							SetProcess(Process::Camp, "i will defend the dropped bomb", true, AddTime(ebot_camp_max.GetFloat()));
							return m_chosenGoalIndex = index;
						}
					}
					else
						return m_chosenGoalIndex = m_loosedBombWptIndex;
				}
				else
				{
					if (m_team == TEAM_COUNTER)
					{
						if (!g_waypoint->m_ctPoints.IsEmpty())
						{
							const int index = g_waypoint->m_ctPoints.GetRandomElement();
							if (IsValidWaypoint(index))
								return m_chosenGoalIndex = index;
						}
					}
					else
					{
						if (m_isBomber)
						{
							m_loosedBombWptIndex = -1;
							if (IsValidWaypoint(m_chosenGoalIndex) && g_waypoint->GetPath(m_chosenGoalIndex)->flags & WAYPOINT_GOAL)
								return m_chosenGoalIndex;
							else if (!g_waypoint->m_goalPoints.IsEmpty())
								return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
						}
						else if (!g_waypoint->m_terrorPoints.IsEmpty())
						{
							const int index = g_waypoint->m_terrorPoints.GetRandomElement();
							if (IsValidWaypoint(index))
								return m_chosenGoalIndex = index;
						}
					}
				}
			}
		}
	}

	if (!FNullEnt(m_nearestEnemy) && IsAlive(m_nearestEnemy))
	{
		const Vector origin = GetEntityOrigin(m_nearestEnemy);
		if (origin != nullvec)
		{
			const int index = g_waypoint->FindNearest(origin, 9999999.0f, -1, m_nearestEnemy);
			if (IsValidWaypoint(index))
				return m_chosenGoalIndex = index;
		}
	}

	if (!g_waypoint->m_otherPoints.IsEmpty())
		return m_chosenGoalIndex = g_waypoint->m_otherPoints.GetRandomElement();

	return m_chosenGoalIndex = CRandomInt(0, g_numWaypoints - 1);
}

bool Bot::GoalIsValid(void)
{
	if (!IsValidWaypoint(m_chosenGoalIndex))
		return false;

	if (m_chosenGoalIndex == m_currentWaypointIndex)
		return false;

	if (m_navNode == nullptr)
		return false;

	return true;
}

void Bot::MoveTo(const Vector targetPosition)
{
	const Vector directionOld = (targetPosition + pev->velocity * -m_frameInterval) - (pev->origin + pev->velocity * m_frameInterval);
	const Vector directionNormal = directionOld.Normalize2D();
	m_moveAngles = directionOld.ToAngles();
	m_moveAngles.ClampAngles();
	m_moveAngles.x = -m_moveAngles.x; // invert for engine
	m_moveSpeed = pev->maxspeed;
	m_strafeSpeed = 0.0f;
}

void Bot::MoveOut(const Vector targetPosition)
{
	const Vector directionOld = (targetPosition + pev->velocity * -m_frameInterval) - (pev->origin + pev->velocity * m_frameInterval);
	const Vector directionNormal = directionOld.Normalize2D();
	SetStrafeSpeed(directionNormal, pev->maxspeed);
	m_moveAngles = directionOld.ToAngles();
	m_moveAngles.ClampAngles();
	m_moveAngles.x = -m_moveAngles.x; // invert for engine
	m_moveSpeed = -pev->maxspeed;
}

void Bot::FollowPath(const Vector targetPosition)
{
	FollowPath(g_waypoint->FindNearestInCircle(targetPosition));
}

void Bot::FollowPath(const int targetIndex)
{
	m_strafeSpeed = 0.0f;

	if (m_navNode != nullptr)
	{
		DoWaypointNav();

		const Vector directionOld = m_waypointFlags & WAYPOINT_FALLRISK ? (m_destOrigin + pev->velocity * -m_frameInterval) - (pev->origin + pev->velocity * m_frameInterval) : m_destOrigin - (pev->origin + pev->velocity * m_frameInterval);
		const Vector directionNormal = directionOld.Normalize2D();
		m_moveAngles = directionOld.ToAngles();
		m_moveAngles.ClampAngles();
		m_moveAngles.x = -m_moveAngles.x; // invert for engine

		if (pev->flags & FL_DUCKING)
		{
			m_moveSpeed = pev->maxspeed;
			CheckStuck(pev->maxspeed * 0.502f);
		}
		else if (IsOnLadder() || m_waypointFlags & WAYPOINT_LADDER)
		{
			const float minSpeed = pev->maxspeed * 0.502f;
			m_moveSpeed = (pev->origin - m_destOrigin).GetLength();

			if (m_moveSpeed > pev->maxspeed)
				m_moveSpeed = pev->maxspeed;
			else if (!IsOnLadder() && m_moveSpeed < minSpeed)
				m_moveSpeed = minSpeed;

			CheckStuck(m_moveSpeed);
		}
		else
		{
			const float maxSpeed = GetMaxSpeed();
			m_moveSpeed = maxSpeed;
			CheckStuck(maxSpeed);
		}
	}
	else if (!m_isSlowThink)
	{
		m_moveSpeed = 0.0f;
		FindPath(m_currentWaypointIndex, targetIndex);
	}
}

// this function is a main path navigation
void Bot::DoWaypointNav(void)
{
	if (!IsValidWaypoint(m_currentWaypointIndex))
	{
		if (m_navNode != nullptr)
			m_currentWaypointIndex = m_navNode->index;
		else
			FindWaypoint();

		return;
	}

	const Path* currentWaypoint = g_waypoint->GetPath(m_currentWaypointIndex);
	m_destOrigin = m_waypointOrigin;

	auto autoJump = [&](void)
	{
		// this waypoint has additional travel flags - care about them
		if (m_currentTravelFlags & PATHFLAG_JUMP)
		{
			// bot is not jumped yet?
			if (!m_jumpFinished)
			{
				// cheating for jump, bots cannot do some hard jumps and double jumps too
				// who cares about double jump for bots? :)
				pev->button |= IN_JUMP;

				if (m_desiredVelocity == nullvec)
				{
					const Vector myOrigin = GetBottomOrigin(GetEntity());
					const Vector waypointOrigin = currentWaypoint->origin;

					// we need gap
					/*if (currentWaypoint->flags & WAYPOINT_CROUCH)
						waypointOrigin.z -= 18.0f;
					else
						waypointOrigin.z -= 36.0f;*/

					const float limit = (pev->maxspeed * 1.28f);
					const float timeToReachWaypoint = csqrtf(powf(waypointOrigin.x - myOrigin.x, 2.0f) + powf(waypointOrigin.y - myOrigin.y, 2.0f)) / (pev->maxspeed - pev->gravity);
					pev->velocity.x = (waypointOrigin.x - myOrigin.x) / timeToReachWaypoint;
					pev->velocity.y = (waypointOrigin.y - myOrigin.y) / timeToReachWaypoint;
					pev->velocity.z = cclampf(2.0f * (waypointOrigin.z - myOrigin.z - 0.5f * pev->gravity * powf(timeToReachWaypoint, 2.0f)) / timeToReachWaypoint, -limit, limit);

					pev->basevelocity = pev->velocity;
					pev->clbasevelocity = pev->velocity;
					pev->avelocity = pev->velocity;
					pev->flFallVelocity = pev->velocity.z;
				}
				else
				{
					pev->velocity = m_desiredVelocity;
					pev->basevelocity = m_desiredVelocity;
					pev->clbasevelocity = m_desiredVelocity;
					pev->avelocity = m_desiredVelocity;
					pev->flFallVelocity = m_desiredVelocity.z;
				}

				m_jumpFinished = true;
			}
		}
	};

	// NOO!
	if (m_stuckWarn >= 2)
		autoJump();

	if (currentWaypoint->flags & WAYPOINT_LADDER || IsOnLadder())
	{
		if (IsValidWaypoint(m_prevWptIndex[0]) && g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LADDER)
		{
			if (cabsf(m_waypointOrigin.z - pev->origin.z) > 5.0f)
				m_waypointOrigin.z += pev->origin.z - m_waypointOrigin.z;

			if (m_waypointOrigin.z > (pev->origin.z + 16.0f))
				m_waypointOrigin = m_waypointOrigin - Vector(0.0f, 0.0f, 16.0f);

			if (m_waypointOrigin.z < (pev->origin.z - 16.0f))
				m_waypointOrigin = m_waypointOrigin + Vector(0.0f, 0.0f, 16.0f);
		}

		m_destOrigin = m_waypointOrigin;

		// special detection if someone is using the ladder (to prevent to have bots-towers on ladders)
		for (const auto& client : g_clients)
		{
			if (FNullEnt(client.ent) || !(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || (client.ent->v.movetype != MOVETYPE_FLY) || client.index == m_index)
				continue;

			TraceResult tr;
			bool foundGround = false;
			int previousNode = 0;

			// more than likely someone is already using our ladder...
			if ((client.ent->v.origin - currentWaypoint->origin).GetLengthSquared() <= SquaredF(48.0f))
			{
				extern ConVar ebot_ignore_enemies;
				if ((client.team != m_team || GetGameMode() == MODE_DM || GetGameMode() == MODE_NOTEAM) && !ebot_ignore_enemies.GetBool())
				{
					TraceLine(EyePosition(), client.ent->v.origin, ignore_monsters, GetEntity(), &tr);

					// bot found an enemy on his ladder - he should see him...
					if (!FNullEnt(tr.pHit) && tr.pHit == client.ent)
					{
						m_hasEnemiesNear = true;
						m_nearestEnemy = client.ent;
						m_visibility = (Visibility::Head | Visibility::Body);
						m_enemyOrigin = GetEnemyPosition();
						SetWalkTime(7.0f);
						break;
					}
				}
				else
				{
					TraceHull(EyePosition(), m_destOrigin, ignore_monsters, (pev->flags & FL_DUCKING) ? head_hull : human_hull, GetEntity(), &tr);

					// someone is above or below us and is using the ladder already
					if (client.ent->v.movetype == MOVETYPE_FLY && cabsf(pev->origin.z - client.ent->v.origin.z) > 15.0f && tr.pHit == client.ent)
					{
						if (IsValidWaypoint(m_prevWptIndex[0]))
						{
							if (!(g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LADDER))
							{
								foundGround = true;
								previousNode = m_prevWptIndex[0];
							}
							else if (IsValidWaypoint(m_prevWptIndex[1]))
							{
								if (!(g_waypoint->GetPath(m_prevWptIndex[1])->flags & WAYPOINT_LADDER))
								{
									foundGround = true;
									previousNode = m_prevWptIndex[1];
								}
								else if (IsValidWaypoint(m_prevWptIndex[2]))
								{
									if (!(g_waypoint->GetPath(m_prevWptIndex[2])->flags & WAYPOINT_LADDER))
									{
										foundGround = true;
										previousNode = m_prevWptIndex[2];
									}
									else if (IsValidWaypoint(m_prevWptIndex[3]))
									{
										if (!(g_waypoint->GetPath(m_prevWptIndex[3])->flags & WAYPOINT_LADDER))
										{
											foundGround = true;
											previousNode = m_prevWptIndex[3];
										}
									}
								}
							}
						}

						if (foundGround)
						{
							if (GetCurrentTask()->taskID != TASK_MOVETOPOSITION || GetCurrentTask()->desire == TASKPRI_PLANTBOMB)
							{
								ChangeWptIndex(m_prevWptIndex[0]);
								PushTask(TASK_MOVETOPOSITION, TASKPRI_PLANTBOMB, previousNode, 0.0f, true);
							}

							break;
						}
					}
				}
			}
		}
	}

	if (currentWaypoint->flags & WAYPOINT_CROUCH && (!(currentWaypoint->flags & WAYPOINT_CAMP) || m_stuckWarn >= 4))
		m_duckTime = AddTime(1.0f);

	if (currentWaypoint->flags & WAYPOINT_LIFT)
	{
		if (!UpdateLiftHandling())
			return;

		if (!UpdateLiftStates())
			return;
	}

	// check if we are going through a door...
	if (g_hasDoors)
	{
		TraceResult tr;
		TraceLine(pev->origin, m_waypointOrigin, ignore_monsters, GetEntity(), &tr);

		if (!FNullEnt(tr.pHit) && FNullEnt(m_liftEntity) && cstrncmp(STRING(tr.pHit->v.classname), "func_door", 9) == 0)
		{
			// if the door is near enough...
			if ((pev->origin - GetEntityOrigin(tr.pHit)).GetLengthSquared() <= SquaredF(54.0f))
			{
				ResetStuck(); // don't consider being stuck

				if (!(pev->oldbuttons & IN_USE && pev->button & IN_USE))
				{
					// do not use door directrly under xash, or we will get failed assert in gamedll code
					if (g_gameVersion == CSVER_XASH)
						pev->button |= IN_USE;
					else
						MDLL_Use(tr.pHit, GetEntity()); // also 'use' the door randomly
				}
			}

			edict_t* button = FindNearestButton(STRING(tr.pHit->v.targetname));

			// check if we got valid button
			if (!FNullEnt(button))
			{
				m_pickupItem = button;
				m_pickupType = PICKTYPE_BUTTON;
				m_navTimeset = engine->GetTime();
			}

			// if bot hits the door, then it opens, so wait a bit to let it open safely
			if (m_timeDoorOpen < engine->GetTime() && pev->velocity.GetLengthSquared2D() < SquaredF(pev->maxspeed * 0.20f))
			{
				m_timeDoorOpen = engine->GetTime() + 3.0f;

				if (m_tryOpenDoor++ > 2 && !FNullEnt(m_nearestEnemy) && IsWalkableTraceLineClear(EyePosition(), m_nearestEnemy->v.origin))
				{
					m_hasEnemiesNear = true;
					m_visibility = Visibility::Body;
					m_enemyOrigin = m_nearestEnemy->v.origin;
					SetWalkTime(7.0f);
					m_tryOpenDoor = 0;
				}
				else
				{
					if (m_isSlowThink)
					{
						SetProcess(Process::Pause, "waiting for door open", false, 1.25f);
						DeleteSearchNodes();
						return;
					}

					m_tryOpenDoor = 0;
				}
			}
		}
	}

	float desiredDistance = SquaredF(8.0f) + pev->maxspeed;

	if (IsOnLadder() || currentWaypoint->flags & WAYPOINT_LADDER)
		desiredDistance = SquaredF(24.0f);
	else if (currentWaypoint->flags & WAYPOINT_LIFT)
		desiredDistance = SquaredF(50.0f);
	else if (m_currentTravelFlags & PATHFLAG_JUMP)
		desiredDistance = 0.0f;
	else if (pev->flags & FL_DUCKING || currentWaypoint->flags & WAYPOINT_GOAL)
		desiredDistance = SquaredF(24.0f);
	else if (IsWaypointOccupied(m_currentWaypointIndex))
		desiredDistance = SquaredF(96.0f);
	else if (currentWaypoint->radius > 8.0f)
		desiredDistance = SquaredF(currentWaypoint->radius) + pev->maxspeed;

	const float distance = (pev->origin - m_destOrigin).GetLengthSquared();

	// check if waypoint has a special travelflag, so they need to be reached more precisely
	for (int i = 0; i < Const_MaxPathIndex; i++)
	{
		if (currentWaypoint->connectionFlags[i] != 0)
		{
			desiredDistance = 0.0f;
			break;
		}
	}

	// needs precise placement - check if we get past the point
	if (desiredDistance <= SquaredF(24.0f) && distance <= SquaredF(32.0f) && (m_destOrigin - (pev->origin + pev->velocity * m_frameInterval)).GetLengthSquared() >= distance)
		desiredDistance = distance + pev->maxspeed;

	if (distance <= desiredDistance)
	{
		m_navNode = m_navNode->next;
		m_currentTravelFlags = 0;
		m_desiredVelocity = nullvec;

		if (m_navNode != nullptr)
		{
			const int destIndex = m_navNode->index;

			ChangeWptIndex(destIndex);
			SetWaypointOrigin();

			for (int i = 0; i < Const_MaxPathIndex; i++)
			{
				if (currentWaypoint->index[i] == destIndex)
				{
					m_jumpFinished = false;
					m_currentTravelFlags = currentWaypoint->connectionFlags[i];
					m_desiredVelocity = currentWaypoint->connectionVelocity[i];
					autoJump();
					break;
				}
			}
		}
	}
}

bool Bot::UpdateLiftHandling()
{
	bool liftClosedDoorExists = false;

	// update node time set
	m_navTimeset = engine->GetTime();

	TraceResult tr{};

	// wait for something about for lift
	auto wait = [&]()
	{
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;

		m_navTimeset = engine->GetTime();
		m_aimFlags |= AIM_NAVPOINT;

		pev->button &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);

		ResetStuck();
	};

	// trace line to door
	TraceLine(pev->origin, m_waypointOrigin, true, true, GetEntity(), &tr);

	if (tr.flFraction < 1.0f && (m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor || m_liftState == LiftState::LookingButtonOutside) && !FNullEnt(tr.pHit) && cstrcmp(STRING(tr.pHit->v.classname), "func_door") == 0 && pev->groundentity != tr.pHit)
	{
		if (m_liftState == LiftState::None)
		{
			m_liftState = LiftState::LookingButtonOutside;
			m_liftUsageTime = engine->GetTime() + 7.0f;
		}

		liftClosedDoorExists = true;
	}

	const auto path = g_waypoint->GetPath(m_currentWaypointIndex);

	if (m_navNode != nullptr)
	{
		// trace line down
		TraceLine(path->origin, m_waypointOrigin + Vector(0.0f, 0.0f, -50.0f), true, true, GetEntity(), &tr);

		// if trace result shows us that it is a lift
		if (!liftClosedDoorExists && !FNullEnt(tr.pHit) && (cstrcmp(STRING(tr.pHit->v.classname), "func_door") == 0 || cstrcmp(STRING(tr.pHit->v.classname), "func_plat") == 0 || cstrcmp(STRING(tr.pHit->v.classname), "func_train") == 0))
		{
			if ((m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor || m_liftState == LiftState::LookingButtonOutside) && tr.pHit->v.velocity.z == 0.0f)
			{
				if (cabsf(pev->origin.z - tr.vecEndPos.z) < 70.0f)
				{
					m_liftEntity = tr.pHit;
					m_liftState = LiftState::EnteringIn;
					m_liftTravelPos = m_waypointOrigin;
					m_liftUsageTime = engine->GetTime() + 5.0f;
				}
			}
			else if (m_liftState == LiftState::TravelingBy)
			{
				m_liftState = LiftState::Leaving;
				m_liftUsageTime = engine->GetTime() + 7.0f;
			}
		}
		else
		{
			if ((m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor) && HasNextPath())
			{
				const int nextNode = m_navNode->next->index;

				if (IsValidWaypoint(nextNode) && (g_waypoint->GetPath(nextNode)->flags & WAYPOINT_LIFT))
				{
					TraceLine(path->origin, g_waypoint->GetPath(nextNode)->origin, true, true, GetEntity(), &tr);

					if (!FNullEnt(tr.pHit) && (cstrcmp(STRING(tr.pHit->v.classname), "func_door") == 0 || cstrcmp(STRING(tr.pHit->v.classname), "func_plat") == 0 || cstrcmp(STRING(tr.pHit->v.classname), "func_train") == 0))
						m_liftEntity = tr.pHit;
				}

				m_liftState = LiftState::LookingButtonOutside;
				m_liftUsageTime = engine->GetTime() + 15.0f;
			}
		}
	}

	// bot is going to enter the lift
	if (m_liftState == LiftState::EnteringIn)
	{
		m_destOrigin = m_liftTravelPos;

		// check if we enough to destination
		if ((pev->origin - m_destOrigin).GetLengthSquared() <= SquaredF(20.0f))
		{
			wait();

			// need to wait our following teammate ?
			bool needWaitForTeammate = false;

			// if some bot is following a bot going into lift - he should take the same lift to go
			for (const auto& bot : g_botManager->m_bots)
			{
				if (bot == nullptr || !bot->m_isAlive || bot->m_team != m_team || bot->m_targetEntity != GetEntity() || bot->GetCurrentTask()->taskID != TASK_FOLLOWUSER)
					continue;

				if (bot->pev->groundentity == m_liftEntity && bot->IsOnFloor())
					break;

				bot->m_liftEntity = m_liftEntity;
				bot->m_liftState = LiftState::EnteringIn;
				bot->m_liftTravelPos = m_liftTravelPos;

				needWaitForTeammate = true;
			}

			if (needWaitForTeammate)
			{
				m_liftState = LiftState::WaitingForTeammates;
				m_liftUsageTime = engine->GetTime() + 8.0f;
			}
			else
			{
				m_liftState = LiftState::LookingButtonInside;
				m_liftUsageTime = engine->GetTime() + 10.0f;
			}
		}
	}

	// bot is waiting for his teammates
	if (m_liftState == LiftState::WaitingForTeammates)
	{
		// need to wait our following teammate ?
		bool needWaitForTeammate = false;

		for (const auto& bot : g_botManager->m_bots)
		{
			if (bot == nullptr || !bot->m_isAlive || bot->m_team != m_team || bot->m_targetEntity != GetEntity() || bot->GetCurrentTask()->taskID != TASK_FOLLOWUSER || bot->m_liftEntity != m_liftEntity)
				continue;

			if (bot->pev->groundentity == m_liftEntity || !bot->IsOnFloor())
			{
				needWaitForTeammate = true;
				break;
			}
		}

		// need to wait for teammate
		if (needWaitForTeammate)
		{
			m_destOrigin = m_liftTravelPos;

			if ((pev->origin - m_destOrigin).GetLengthSquared() <= SquaredF(20.0f))
				wait();
		}

		// else we need to look for button
		if (!needWaitForTeammate || m_liftUsageTime < engine->GetTime())
		{
			m_liftState = LiftState::LookingButtonInside;
			m_liftUsageTime = engine->GetTime() + 10.0f;
		}
	}

	// bot is trying to find button inside a lift
	if (m_liftState == LiftState::LookingButtonInside)
	{
		edict_t* button = FindNearestButton(STRING(m_liftEntity->v.targetname));

		// got a valid button entity ?
		if (!FNullEnt(button) && pev->groundentity == m_liftEntity && m_buttonPushTime + 1.0f < engine->GetTime() && m_liftEntity->v.velocity.z == 0.0f && IsOnFloor())
		{
			m_pickupItem = button;
			m_pickupType = PICKTYPE_BUTTON;

			m_navTimeset = engine->GetTime();
		}
	}

	// is lift activated and bot is standing on it and lift is moving ?
	if (m_liftState == LiftState::LookingButtonInside || m_liftState == LiftState::EnteringIn || m_liftState == LiftState::WaitingForTeammates || m_liftState == LiftState::WaitingFor)
	{
		if (pev->groundentity == m_liftEntity && m_liftEntity->v.velocity.z != 0.0f && IsOnFloor() && (g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LIFT || !FNullEnt(m_targetEntity)))
		{
			m_liftState = LiftState::TravelingBy;
			m_liftUsageTime = engine->GetTime() + 14.0f;

			if ((pev->origin - m_destOrigin).GetLengthSquared() <= SquaredF(20.0f))
				wait();
		}
	}

	// bots is currently moving on lift
	if (m_liftState == LiftState::TravelingBy)
	{
		m_destOrigin = Vector(m_liftTravelPos.x, m_liftTravelPos.y, pev->origin.z);

		if ((pev->origin - m_destOrigin).GetLengthSquared() <= SquaredF(20.0f))
			wait();
	}

	// need to find a button outside the lift
	if (m_liftState == LiftState::LookingButtonOutside)
	{
		// button has been pressed, lift should come
		if (m_buttonPushTime + 8.0f >= engine->GetTime())
		{
			if (IsValidWaypoint(m_prevWptIndex[0]))
				m_destOrigin = g_waypoint->GetPath(m_prevWptIndex[0])->origin;
			else
				m_destOrigin = pev->origin;

			if ((pev->origin - m_destOrigin).GetLengthSquared() <= SquaredF(20.0f))
				wait();
		}
		else if (!FNullEnt(m_liftEntity))
		{
			edict_t* button = FindNearestButton(STRING(m_liftEntity->v.targetname));

			// if we got a valid button entity
			if (!FNullEnt(button))
			{
				// lift is already used ?
				bool liftUsed = false;

				// iterate though clients, and find if lift already used
				for (const auto& client : g_clients)
				{
					if (FNullEnt(client.ent) || !(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == GetEntity() || FNullEnt(client.ent->v.groundentity))
						continue;

					if (client.ent->v.groundentity == m_liftEntity)
					{
						liftUsed = true;
						break;
					}
				}

				// lift is currently used
				if (liftUsed)
				{
					if (IsValidWaypoint(m_prevWptIndex[0]))
						m_destOrigin = g_waypoint->GetPath(m_prevWptIndex[0])->origin;
					else
						m_destOrigin = button->v.origin;

					if ((pev->origin - m_destOrigin).GetLengthSquared() <= SquaredF(20.0f))
						wait();
				}
				else
				{
					m_pickupItem = button;
					m_pickupType = PICKTYPE_BUTTON;
					m_liftState = LiftState::WaitingFor;

					m_navTimeset = engine->GetTime();
					m_liftUsageTime = engine->GetTime() + 20.0f;
				}
			}
			else
			{
				m_liftState = LiftState::WaitingFor;
				m_liftUsageTime = engine->GetTime() + 15.0f;
			}
		}
	}

	// bot is waiting for lift
	if (m_liftState == LiftState::WaitingFor)
	{
		if (IsValidWaypoint(m_prevWptIndex[0]))
		{
			if (!(g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LIFT))
				m_destOrigin = g_waypoint->GetPath(m_prevWptIndex[0])->origin;
			else if (IsValidWaypoint(m_prevWptIndex[1]))
				m_destOrigin = g_waypoint->GetPath(m_prevWptIndex[1])->origin;
		}

		if ((pev->origin - m_destOrigin).GetLengthSquared() <= SquaredF(20.0f))
			wait();
	}

	// if bot is waiting for lift, or going to it
	if (m_liftState == LiftState::WaitingFor || m_liftState == LiftState::EnteringIn)
	{
		// bot fall down somewhere inside the lift's groove :)
		if (pev->groundentity != m_liftEntity && IsValidWaypoint(m_prevWptIndex[0]))
		{
			if (g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LIFT && (path->origin.z - pev->origin.z) > 50.0f && (g_waypoint->GetPath(m_prevWptIndex[0])->origin.z - pev->origin.z) > 50.0f)
			{
				m_liftState = LiftState::None;
				m_liftEntity = nullptr;
				m_liftUsageTime = 0.0f;

				DeleteSearchNodes();
				FindWaypoint();

				if (IsValidWaypoint(m_prevWptIndex[2]))
					FindShortestPath(m_currentWaypointIndex, m_prevWptIndex[2]);

				return false;
			}
		}
	}

	return true;
}

bool Bot::UpdateLiftStates()
{
	if (!FNullEnt(m_liftEntity) && !(m_waypointFlags & WAYPOINT_LIFT))
	{
		if (m_liftState == LiftState::TravelingBy)
		{
			m_liftState = LiftState::Leaving;
			m_liftUsageTime = engine->GetTime() + 10.0f;
		}
		else if (m_liftState == LiftState::Leaving && m_liftUsageTime < engine->GetTime() && pev->groundentity != m_liftEntity)
		{
			m_liftState = LiftState::None;
			m_liftUsageTime = 0.0f;

			m_liftEntity = nullptr;
		}
	}

	if (m_liftUsageTime < engine->GetTime() && m_liftUsageTime != 0.0f)
	{
		m_liftEntity = nullptr;
		m_liftState = LiftState::None;
		m_liftUsageTime = 0.0f;

		DeleteSearchNodes();

		if (IsValidWaypoint(m_prevWptIndex[0]))
		{
			if (!(g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LIFT))
			{
				ChangeWptIndex(m_prevWptIndex[0]);
				SetWaypointOrigin();
			}
			else
				FindWaypoint();
		}
		else
			FindWaypoint();

		return false;
	}

	return true;
}

// Priority queue class (smallest item out first)
class PriorityQueue
{
public:
	PriorityQueue(void);
	~PriorityQueue(void);

	inline int  Empty(void) { return m_size == 0; }
	inline int  Size(void) { return m_size; }
	void        Insert(int, float);
	int         Remove(void);

private:
	struct HeapNode_t
	{
		int   id;
		float priority;
	} *m_heap;

	int         m_size;
	int         m_heapSize;

	void        HeapSiftDown(int);
	void        HeapSiftUp(void);
};

PriorityQueue::PriorityQueue(void)
{
	m_size = 0;
	m_heapSize = g_numWaypoints * 8;
	m_heap = new HeapNode_t[m_heapSize];
}

PriorityQueue::~PriorityQueue(void)
{
	if (m_heap != nullptr)
		delete[] m_heap;

	m_heap = nullptr;
}

// inserts a value into the priority queue
void PriorityQueue::Insert(int value, float priority)
{
	if (m_size >= m_heapSize)
	{
		m_heapSize += 100;
		m_heap = (HeapNode_t*)realloc(m_heap, sizeof(HeapNode_t) * m_heapSize);
		if (m_heap == nullptr)
		{
			AddLogEntry(LOG_MEMORY, "unexpected memory error");
			return;
		}
	}

	m_heap[m_size].priority = priority;
	m_heap[m_size].id = value;

	m_size++;
	HeapSiftUp();
}

// removes the smallest item from the priority queue
int PriorityQueue::Remove(void)
{
	const int retID = m_heap[0].id;

	m_size--;
	m_heap[0] = m_heap[m_size];

	HeapSiftDown(0);
	return retID;
}

void PriorityQueue::HeapSiftDown(int subRoot)
{
	int parent = subRoot;
	int child = (2 * parent) + 1;

	const HeapNode_t ref = m_heap[parent];

	while (child < m_size)
	{
		const int rightChild = (2 * parent) + 2;

		if (rightChild < m_size)
		{
			if (m_heap[rightChild].priority < m_heap[child].priority)
				child = rightChild;
		}

		if (ref.priority <= m_heap[child].priority)
			break;

		m_heap[parent] = m_heap[child];

		parent = child;
		child = (2 * parent) + 1;
	}

	m_heap[parent] = ref;
}

void PriorityQueue::HeapSiftUp(void)
{
	int child = m_size - 1;

	while (child)
	{
		const int parent = (child - 1) / 2;

		if (m_heap[parent].priority <= m_heap[child].priority)
			break;

		const HeapNode_t temp = m_heap[child];

		m_heap[child] = m_heap[parent];
		m_heap[parent] = temp;

		child = parent;
	}
}

inline const float GF_CostHuman(const int index, const int parent, const int team, const float gravity, const bool isZombie)
{
	const Path* path = g_waypoint->GetPath(index);
	if (path->flags & WAYPOINT_AVOID)
		return FLT_MAX;

	if (isZombie)
	{
		if (path->flags & WAYPOINT_HUMANONLY)
			return FLT_MAX;
	}
	else
	{
		if (path->flags & WAYPOINT_ZOMBIEONLY)
			return FLT_MAX;

		if (path->flags & WAYPOINT_DJUMP)
			return FLT_MAX;
	}

	if (path->flags & WAYPOINT_ONLYONE)
	{
		for (const auto& client : g_clients)
		{
			if (FNullEnt(client.ent))
				continue;

			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			const float distance = (client.origin - path->origin).GetLengthSquared();
			if (distance <= SquaredF(path->radius + 64.0f))
				return FLT_MAX;
		}
	}

	int count = 0;
	float totalDistance = 0.0f;
	const Vector waypointOrigin = path->origin;
	for (const auto& client : g_clients)
	{
		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team == client.team || !IsZombieEntity(client.ent))
			continue;

		const float distance = ((client.ent->v.origin + client.ent->v.velocity * g_pGlobals->frametime) - waypointOrigin).GetLengthSquared();
		if (distance <= SquaredF(path->radius + 128.0f))
			count++;

		totalDistance += distance;
	}

	if (count > 0 && totalDistance > 0.0f)
	{
		float baseCost = g_waypoint->GetPathDistance(index, parent);
		baseCost *= count;
		baseCost += totalDistance;
		return baseCost;
	}

	return 1.0f;
}

inline const float GF_CostCareful(const int index, const int parent, const int team, const float gravity, const bool isZombie)
{
	const Path* path = g_waypoint->GetPath(index);
	if (path->flags & WAYPOINT_AVOID)
		return FLT_MAX;

	if (isZombie)
	{
		if (path->flags & WAYPOINT_HUMANONLY)
			return FLT_MAX;
	}
	else
	{
		if (path->flags & WAYPOINT_ZOMBIEONLY)
			return FLT_MAX;

		if (path->flags & WAYPOINT_DJUMP)
			return FLT_MAX;
	}

	if (path->flags & WAYPOINT_ONLYONE)
	{
		for (const auto& client : g_clients)
		{
			if (FNullEnt(client.ent))
				continue;

			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			const float distance = (client.origin - path->origin).GetLengthSquared();
			if (distance <= SquaredF(path->radius + 64.0f))
				return FLT_MAX;
		}
	}

	if (isZombie)
	{
		if (path->flags & WAYPOINT_DJUMP)
		{
			int count = 0;
			for (const auto& client : g_clients)
			{
				if (FNullEnt(client.ent))
					continue;

				if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != team)
					continue;

				if ((client.origin - path->origin).GetLengthSquared() <= SquaredF(512.0f + path->radius))
					count++;
				else if (IsVisible(path->origin, client.ent))
					count++;
			}

			// don't count me
			if (count <= 1)
				return FLT_MAX;
			
			float baseCost = g_waypoint->GetPathDistance(index, parent);
			baseCost /= count;
			return baseCost;
		}
	}

	return 1.0f;
}

inline const float GF_CostNormal(const int index, const int parent, const int team, const float gravity, const bool isZombie)
{
	const Path* path = g_waypoint->GetPath(index);
	if (path->flags & WAYPOINT_AVOID)
		return FLT_MAX;

	if (isZombie)
	{
		if (path->flags & WAYPOINT_HUMANONLY)
			return FLT_MAX;
	}
	else
	{
		if (path->flags & WAYPOINT_ZOMBIEONLY)
			return FLT_MAX;

		if (path->flags & WAYPOINT_DJUMP)
			return FLT_MAX;
	}

	if (path->flags & WAYPOINT_ONLYONE)
	{
		for (const auto& client : g_clients)
		{
			if (FNullEnt(client.ent))
				continue;

			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			const float distance = (client.origin - path->origin).GetLengthSquared();
			if (distance <= SquaredF(path->radius + 64.0f))
				return FLT_MAX;
		}
	}

	if (isZombie)
	{
		if (path->flags & WAYPOINT_DJUMP)
		{
			int count = 0;
			for (const auto& client : g_clients)
			{
				if (FNullEnt(client.ent))
					continue;

				if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != team)
					continue;

				if ((client.origin - path->origin).GetLengthSquared() <= SquaredF(512.0f + path->radius))
					count++;
				else if (IsVisible(path->origin, client.ent))
					count++;
			}

			// don't count me
			if (count <= 1)
				return FLT_MAX;
			
			float baseCost = g_waypoint->GetPathDistance(index, parent);
			baseCost /= count;
			return baseCost;
		}
	}
	
	if (path->flags & WAYPOINT_LADDER)
		return g_waypoint->GetPathDistance(index, parent);
	
	return 1.0f;
}

inline const float GF_CostRusher(const int index, const int parent, const int team, const float gravity, const bool isZombie)
{
	const Path* path = g_waypoint->GetPath(index);
	if (path->flags & WAYPOINT_AVOID)
		return FLT_MAX;

	if (isZombie)
	{
		if (path->flags & WAYPOINT_HUMANONLY)
			return FLT_MAX;
	}
	else
	{
		if (path->flags & WAYPOINT_ZOMBIEONLY)
			return FLT_MAX;

		if (path->flags & WAYPOINT_DJUMP)
			return FLT_MAX;
	}

	if (path->flags & WAYPOINT_ONLYONE)
	{
		for (const auto& client : g_clients)
		{
			if (FNullEnt(client.ent))
				continue;

			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			const float distance = (client.origin - path->origin).GetLengthSquared();
			if (distance <= SquaredF(path->radius + 64.0f))
				return FLT_MAX;
		}
	}

	// rusher bots never wait for boosting
	if (path->flags & WAYPOINT_DJUMP)
		return FLT_MAX;

	const float baseCost = g_waypoint->GetPathDistance(index, parent);
	if (path->flags & WAYPOINT_CROUCH)
		return baseCost;

	return 1.0f;
}

inline const float GF_CostNoHostage(const int index, const int parent, const int team, const float gravity, const bool isZombie)
{
	const Path* path = g_waypoint->GetPath(parent);

	if (path->flags & WAYPOINT_SPECIFICGRAVITY)
		return FLT_MAX;

	if (path->flags & WAYPOINT_CROUCH)
		return FLT_MAX;

	if (path->flags & WAYPOINT_LADDER)
		return FLT_MAX;

	if (path->flags & WAYPOINT_AVOID)
		return FLT_MAX;

	if (path->flags & WAYPOINT_WAITUNTIL)
		return FLT_MAX;

	if (path->flags & WAYPOINT_JUMP)
		return FLT_MAX;

	if (path->flags & WAYPOINT_DJUMP)
		return FLT_MAX;

	for (int i = 0; i < Const_MaxPathIndex; i++)
	{
		const int neighbour = g_waypoint->GetPath(index)->index[i];
		if (IsValidWaypoint(neighbour) && (path->connectionFlags[neighbour] & PATHFLAG_JUMP || path->connectionFlags[neighbour] & PATHFLAG_DOUBLE))
			return FLT_MAX;
	}

	return g_waypoint->GetPathDistance(index, parent);
}

inline const float HF_Distance(const int start, const int goal)
{
	const Vector startOrigin = g_waypoint->GetPath(start)->origin;
	const Vector goalOrigin = g_waypoint->GetPath(goal)->origin;
	return (startOrigin - goalOrigin).GetLengthSquared();
}

inline const float HF_Distance2D(const int start, const int goal)
{
	const Vector startOrigin = g_waypoint->GetPath(start)->origin;
	const Vector goalOrigin = g_waypoint->GetPath(goal)->origin;
	return (startOrigin - goalOrigin).GetLengthSquared2D();
}

inline const float HF_Chebyshev(const int start, const int goal)
{
	const Vector startOrigin = g_waypoint->GetPath(start)->origin;
	const Vector goalOrigin = g_waypoint->GetPath(goal)->origin;
	return cmaxf(cmaxf(cabsf(startOrigin.x - goalOrigin.x), cabsf(startOrigin.y - goalOrigin.y)), cabsf(startOrigin.z - goalOrigin.z));
}

inline const float HF_Chebyshev2D(const int start, const int goal)
{
	const Vector startOrigin = g_waypoint->GetPath(start)->origin;
	const Vector goalOrigin = g_waypoint->GetPath(goal)->origin;
	return cmaxf(cabsf(startOrigin.x - goalOrigin.x), cabsf(startOrigin.y - goalOrigin.y));
}

inline const float HF_Manhattan(const int start, const int goal)
{
	const Vector startOrigin = g_waypoint->GetPath(start)->origin;
	const Vector goalOrigin = g_waypoint->GetPath(goal)->origin;
	return cabsf(startOrigin.x - goalOrigin.x) + cabsf(startOrigin.y - goalOrigin.y) + cabsf(startOrigin.z - goalOrigin.z);
}

inline const float HF_Manhattan2D(const int start, const int goal)
{
	const Vector startOrigin = g_waypoint->GetPath(start)->origin;
	const Vector goalOrigin = g_waypoint->GetPath(goal)->origin;
	return cabsf(startOrigin.x - goalOrigin.x) + cabsf(startOrigin.y - goalOrigin.y);
}

inline const float HF_Euclidean(const int start, const int goal)
{
	const Vector startOrigin = g_waypoint->GetPath(start)->origin;
	const Vector goalOrigin = g_waypoint->GetPath(goal)->origin;

	const float x = cabsf(startOrigin.x - goalOrigin.x);
	const float y = cabsf(startOrigin.y - goalOrigin.y);
	const float z = cabsf(startOrigin.z - goalOrigin.z);

	const float euclidean = csqrtf(powf(x, 2.0f) + powf(y, 2.0f) + powf(z, 2.0f));
	return 1000.0f * (cceilf(euclidean) - euclidean);
}

inline const float HF_Euclidean2D(const int start, const int goal)
{
	const Vector startOrigin = g_waypoint->GetPath(start)->origin;
	const Vector goalOrigin = g_waypoint->GetPath(goal)->origin;

	const float x = cabsf(startOrigin.x - goalOrigin.x);
	const float y = cabsf(startOrigin.y - goalOrigin.y);

	const float euclidean = csqrtf(powf(x, 2.0f) + powf(y, 2.0f));
	return 1000.0f * (cceilf(euclidean) - euclidean);
}

inline const float GF_CostNav(const NavArea* start, const NavArea* goal, const int index)
{
	float multiplier = 1.0f;
	int seed = int(engine->GetTime() * 0.1f) + 1;
	seed *= goal->index;
	seed *= index;
	multiplier += (cosf(float(seed)) + 1.0f) * 10.0f;
	return (g_navmesh->GetCenter(start) - g_navmesh->GetCenter(goal)).GetLengthSquared() * multiplier;
}

inline const float HF_Nav(const NavArea* start, const NavArea* goal)
{
	return (g_navmesh->GetCenter(start) - g_navmesh->GetCenter(goal)).GetLengthSquared();
}

inline const float RandomSeed(const int botindex, const int waypoint, const int personality)
{
	int seed = int(engine->GetTime() * 0.1f) + 1;
	seed *= waypoint;
	seed *= botindex;
	seed *= personality + 1;
	const float multiplier = 1.0f + ((cosf(float(seed)) + 1.0f) * 10.0f);
	return multiplier;
}

char* Bot::GetHeuristicName(void)
{
	if (m_2dH)
	{
		switch (m_heuristic)
		{
		case 1:
			return "Squared Length 2D";
		case 2:
			return "Manhattan 2D";
		case 3:
			return "Chebyshev 2D";
		case 4:
			return "Euclidean 2D";
		}
	}
	else
	{
		switch (m_heuristic)
		{
		case 1:
			return "Squared Length";
		case 2:
			return "Manhattan";
		case 3:
			return "Chebyshev";
		case 4:
			return "Euclidean";
		}
	}

	return "UNKNOWN";
}


// this function finds a path from srcIndex to destIndex
void Bot::FindPathNavMesh(NavArea* start, NavArea* dest)
{
	if (m_isBomber && HasNextPath())
		return;

	if (start == nullptr)
	{
		NavArea* index = g_navmesh->GetNearestNavArea(pev->origin);
		if (index == nullptr)
			return;

		start = index;
		m_jumpFinished = false;
	}

	if (dest == nullptr)
		dest = g_navmesh->GetNavArea(CRandomInt(1, g_numNavAreas - 1));

	if (start == dest)
		return;

	AStar_t navmesh[Const_MaxWaypoints];
	for (int i = 0; i < g_numNavAreas; i++)
	{
		navmesh[i].g = 0;
		navmesh[i].f = 0;
		navmesh[i].parent = -1;
		navmesh[i].state = State::New;
	}

	const float (*gcalc) (const NavArea*, const NavArea*, const int index) = nullptr;
	const float (*hcalc) (const NavArea*, const NavArea*) = nullptr;

	hcalc = HF_Nav;
	if (hcalc == nullptr)
		return;

	gcalc = GF_CostNav;
	if (gcalc == nullptr)
		return;

	// put start node into open list
	const auto srcWaypoint = &navmesh[start->index];
	srcWaypoint->g = gcalc(start, dest, m_index);
	srcWaypoint->f = srcWaypoint->g + hcalc(start, dest);
	srcWaypoint->state = State::Open;

	PriorityQueue openList;
	openList.Insert(start->index, srcWaypoint->f);
	while (!openList.Empty())
	{
		// remove the first node from the open list
		int currentIndex = openList.Remove();

		// is the current node the goal node?
		if (currentIndex == dest->index)
		{
			// delete path for new one
			DeleteSearchNodes();

			// set the chosen goal value
			m_chosenGoalIndex = dest->index;

			// build the complete path
			m_navNode = nullptr;

			do
			{
				PathNode* path = new PathNode;
				if (path == nullptr)
				{
					AddLogEntry(LOG_MEMORY, "unexpected memory error");
					break;
				}

				path->index = currentIndex;
				path->next = m_navNode;

				m_navNode = path;
				currentIndex = navmesh[currentIndex].parent;
			} while (IsValidWaypoint(currentIndex));

			m_navNodeStart = m_navNode;
			m_currentWaypointIndex = m_navNodeStart->index;
			m_jumpFinished = false;

			NavArea* area = g_navmesh->GetNavArea(m_currentWaypointIndex);
			if (area != nullptr)
			{
				m_waypointOrigin = g_navmesh->GetClosestPosition(area, pev->origin);
				m_destOrigin = m_waypointOrigin;
			}

			return;
		}

		const auto currWaypoint = &navmesh[currentIndex];
		if (currWaypoint->state != State::Open)
			continue;

		// put current node into Closed list
		currWaypoint->state = State::Closed;

		NavArea* pointer = g_navmesh->GetNavArea(currentIndex);
		if (pointer == nullptr)
			continue;

		// now expand the current node
		for (int i = 0; i < pointer->connections.GetElementNumber(); i++)
		{
			const NavArea* self = pointer->connections[i];
			if (self == nullptr)
				continue;

			// calculate the F value as F = G + H
			const float g = currWaypoint->g + gcalc(pointer, self, m_index);
			const float h = hcalc(self, dest);
			const float f = g + h;

			const auto childWaypoint = &navmesh[self->index];
			if (childWaypoint->state == State::New || childWaypoint->f > f)
			{
				// put the current child into open list
				childWaypoint->parent = currentIndex;
				childWaypoint->state = State::Open;

				childWaypoint->g = g;
				childWaypoint->f = f;

				openList.Insert(self->index, childWaypoint->f);
			}
		}
	}
}

// this function finds a path from srcIndex to destIndex
void Bot::FindPath(int srcIndex, int destIndex)
{
	if (srcIndex == destIndex)
		return;

	if (g_isUsingNAV)
	{
		FindPathNavMesh(nullptr, g_navmesh->GetNearestNavArea(g_waypoint->GetPath(destIndex)->origin));
		return;
	}

	if (g_gameVersion == HALFLIFE)
	{
		FindShortestPath(srcIndex, destIndex);
		return;
	}

	if (m_isBomber && HasNextPath())
		return;

	// if we're stuck, find nearest waypoint
	if (!IsValidWaypoint(srcIndex))
	{
		const int index = FindWaypoint();
		if (IsValidWaypoint(index))
			srcIndex = index;
	}

	if (!IsValidWaypoint(destIndex))
		destIndex = g_waypoint->m_otherPoints.GetRandomElement();

	// again...
	if (srcIndex == destIndex)
		return;

	AStar_t waypoints[Const_MaxWaypoints];
	for (int i = 0; i < g_numWaypoints; i++)
	{
		waypoints[i].g = 0;
		waypoints[i].f = 0;
		waypoints[i].parent = -1;
		waypoints[i].state = State::New;
	}

	const float (*gcalc) (const int, const int, const int, const float, const bool) = nullptr;
	const float (*hcalc) (const int, const int) = nullptr;
	bool useSeed = true;

	if (IsZombieMode() && ebot_zombies_as_path_cost.GetBool() && !m_isZombieBot)
		gcalc = GF_CostHuman;
	else if (HasHostage())
	{
		useSeed = false;
		gcalc = GF_CostNoHostage;
	}
	else if (m_isBomber || m_isVIP || (g_bombPlanted && m_inBombZone))
	{
		// move faster...
		if (g_timeRoundMid < engine->GetTime())
			gcalc = GF_CostRusher;
		else
			gcalc = GF_CostCareful;
	}
	else if (m_personality == PERSONALITY_CAREFUL)
		gcalc = GF_CostCareful;
	else if (m_personality == PERSONALITY_RUSHER)
		gcalc = GF_CostRusher;
	else
		gcalc = GF_CostNormal;

	if (gcalc == nullptr)
		return;

	if (m_2dH)
	{
		switch (m_heuristic)
		{
		case 1:
			hcalc = HF_Distance2D;
			break;
		case 2:
			hcalc = HF_Manhattan2D;
			break;
		case 3:
			hcalc = HF_Chebyshev2D;
			break;
		case 4:
			hcalc = HF_Euclidean2D;
			break;
		default:
			hcalc = HF_Distance2D;
			break;
		}
	}
	else
	{
		switch (m_heuristic)
		{
		case 1:
			hcalc = HF_Distance;
			break;
		case 2:
			hcalc = HF_Manhattan;
			break;
		case 3:
			hcalc = HF_Chebyshev;
			break;
		case 4:
			hcalc = HF_Euclidean;
			break;
		default:
			hcalc = HF_Distance;
			break;
		}
	}

	if (hcalc == nullptr)
		return;

	// put start node into open list
	const auto srcWaypoint = &waypoints[srcIndex];
	srcWaypoint->g = gcalc(srcIndex, -1, m_team, pev->gravity, m_isZombieBot);
	srcWaypoint->f = srcWaypoint->g;
	srcWaypoint->state = State::Open;

	PriorityQueue openList;
	openList.Insert(srcIndex, srcWaypoint->f);
	while (!openList.Empty())
	{
		// remove the first node from the open list
		int currentIndex = openList.Remove();

		// is the current node the goal node?
		if (currentIndex == destIndex)
		{
			// delete path for new one
			DeleteSearchNodes();

			// set the chosen goal value
			m_chosenGoalIndex = destIndex;

			// build the complete path
			m_navNode = nullptr;

			do
			{
				PathNode* path = new PathNode;
				if (path == nullptr)
				{
					AddLogEntry(LOG_MEMORY, "unexpected memory error");
					break;
				}

				path->index = currentIndex;
				path->next = m_navNode;

				m_navNode = path;
				currentIndex = waypoints[currentIndex].parent;
			} while (IsValidWaypoint(currentIndex));

			m_navNodeStart = m_navNode;
			m_currentWaypointIndex = m_navNodeStart->index;
			m_waypointOrigin = g_waypoint->GetPath(m_currentWaypointIndex)->origin;
			m_destOrigin = m_waypointOrigin;

			return;
		}

		const auto currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->state != State::Open)
			continue;

		// put current node into Closed list
		currWaypoint->state = State::Closed;

		// now expand the current node
		for (int i = 0; i < Const_MaxPathIndex; i++)
		{
			const int self = g_waypoint->GetPath(currentIndex)->index[i];
			if (self == -1)
				continue;

			const int32 flags = g_waypoint->GetPath(self)->flags;
			if (flags & WAYPOINT_FALLCHECK)
			{
				TraceResult tr;
				const Vector origin = g_waypoint->GetPath(self)->origin;
				TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), false, false, GetEntity(), &tr);
				if (tr.flFraction == 1.0f)
					continue;
			}
			else if (flags & WAYPOINT_SPECIFICGRAVITY)
			{
				if (pev->gravity * (1600.0f - 800.0f) < g_waypoint->GetPath(self)->campStartY)
					continue;
			}

			// calculate the F value as F = G + H
			const float g = currWaypoint->g + gcalc(currentIndex, self, m_team, pev->gravity, m_isZombieBot);
			const float h = hcalc(self, destIndex) * useSeed ? RandomSeed(m_index, self, m_personality) : 0.1f;
			const float f = g + h;

			const auto childWaypoint = &waypoints[self];
			if (childWaypoint->state == State::New || childWaypoint->f > f)
			{
				// put the current child into open list
				childWaypoint->parent = currentIndex;
				childWaypoint->state = State::Open;

				childWaypoint->g = g;
				childWaypoint->f = f;

				openList.Insert(self, childWaypoint->f);
			}
		}
	}

	// roam around poorly :(
	Array <int> PossiblePath;
	for (int i = 0; i < g_numWaypoints; i++)
	{
		if (waypoints[i].state == State::Closed)
			PossiblePath.Push(i);
	}
	
	if (!PossiblePath.IsEmpty())
	{
		const int index = PossiblePath.GetRandomElement();
		FindShortestPath(srcIndex, index);
		m_tasks->data = index;
		return;
	}
	
	FindShortestPath(srcIndex, destIndex);
}

void Bot::FindShortestPath(int srcIndex, int destIndex)
{
	if (srcIndex == destIndex)
		return;

	// if we're stuck, find nearest waypoint
	if (!IsValidWaypoint(srcIndex))
	{
		const int index = FindWaypoint();
		if (IsValidWaypoint(index))
			srcIndex = index;
	}

	if (!IsValidWaypoint(destIndex))
		destIndex = g_waypoint->m_otherPoints.GetRandomElement();

	// again...
	if (srcIndex == destIndex)
		return;

	AStar_t waypoints[Const_MaxWaypoints];
	for (int i = 0; i < g_numWaypoints; i++)
	{
		waypoints[i].f = 0;
		waypoints[i].parent = -1;
		waypoints[i].state = State::New;
	}

	// put start node into open list
	const auto srcWaypoint = &waypoints[srcIndex];
	srcWaypoint->f = 1.0f;
	srcWaypoint->state = State::Open;

	PriorityQueue openList;
	openList.Insert(srcIndex, srcWaypoint->f);
	while (!openList.Empty())
	{
		// remove the first node from the open list
		int currentIndex = openList.Remove();

		// is the current node the goal node?
		if (currentIndex == destIndex)
		{
			// delete path for new one
			DeleteSearchNodes();

			// set chosen goal
			m_chosenGoalIndex = destIndex;

			// build the complete path
			m_navNode = nullptr;

			do
			{
				PathNode* path = new PathNode;
				if (path == nullptr)
				{
					AddLogEntry(LOG_MEMORY, "unexpected memory error");
					break;
				}

				path->index = currentIndex;
				path->next = m_navNode;

				m_navNode = path;
				currentIndex = waypoints[currentIndex].parent;
			} while (IsValidWaypoint(currentIndex));

			m_navNodeStart = m_navNode;
			m_currentWaypointIndex = m_navNodeStart->index;
			m_waypointOrigin = g_waypoint->GetPath(m_currentWaypointIndex)->origin;
			m_destOrigin = m_waypointOrigin;
			m_jumpFinished = false;

			return;
		}

		const auto currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->state != State::Open)
			continue;

		// put current node into Closed list
		currWaypoint->state = State::Closed;

		// now expand the current node
		for (int i = 0; i < Const_MaxPathIndex; i++)
		{
			const int self = g_waypoint->GetPath(currentIndex)->index[i];
			if (self == -1)
				continue;

			const int32 flags = g_waypoint->GetPath(self)->flags;
			if (flags & WAYPOINT_FALLCHECK)
			{
				TraceResult tr;
				const Vector origin = g_waypoint->GetPath(self)->origin;
				TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), false, false, GetEntity(), &tr);
				if (tr.flFraction == 1.0f)
					continue;
			}
			else if (flags & WAYPOINT_SPECIFICGRAVITY)
			{
				if (pev->gravity * (1600.0f - 800.0f) < g_waypoint->GetPath(self)->campStartY)
					continue;
			}

			const float f = HF_Chebyshev(self, destIndex);
			const auto childWaypoint = &waypoints[self];
			if (childWaypoint->state == State::New || childWaypoint->f > f)
			{
				// put the current child into open list
				childWaypoint->parent = currentIndex;
				childWaypoint->state = State::Open;
				childWaypoint->f = f;

				openList.Insert(self, childWaypoint->f);
			}
		}
	}

	Array <int> PossiblePath;
	for (int i = 0; i < g_numWaypoints; i++)
	{
		if (waypoints[i].state == State::Closed)
			PossiblePath.Push(i);
	}

	if (!PossiblePath.IsEmpty())
	{
		const int index = PossiblePath.GetRandomElement();
		FindShortestPath(srcIndex, index);
		m_tasks->data = index;
		return;
	}
}

void Bot::DeleteSearchNodes(void)
{
	PathNode* deletingNode = nullptr;
	PathNode* node = m_navNodeStart;
	
	while (node != nullptr)
	{
		deletingNode = node->next;
		delete node;
		node = deletingNode;
	}
	
	m_navNodeStart = nullptr;
	m_navNode = nullptr;
	m_jumpFinished = false;
}

void Bot::CheckTouchEntity(edict_t* entity)
{
	if (FNullEnt(entity))
		return;

	// If we won't be able to break it, don't try
	if (entity->v.takedamage != DAMAGE_YES)
	{
		// defuse bomb
		if (g_bombPlanted && cstrcmp(STRING(entity->v.model) + 9, "c4.mdl") == 0)
			SetProcess(Process::Defuse, "trying to defusing the bomb", false, m_hasDefuser ? 6.0f : 12.0f);

		return;
	}

	// See if it's breakable
	if (IsShootableBreakable(entity))
	{
		bool breakIt = false;

		TraceResult tr;
		TraceLine(EyePosition(), m_destOrigin, false, false, GetEntity(), &tr);

		TraceResult tr2;
		TraceHull(pev->origin, m_destOrigin, false, head_hull, GetEntity(), &tr2);

		// double check
		if (tr.pHit == entity || tr2.pHit == entity)
		{
			m_breakableEntity = entity;
			m_breakable = tr.pHit == entity ? tr.vecEndPos : tr2.vecEndPos;
			m_destOrigin = m_breakable;

			if (pev->origin.z > m_breakable.z)
				m_campButtons = IN_DUCK;
			else
				m_campButtons = pev->button & IN_DUCK;

			SetProcess(Process::DestroyBreakable, "trying to destroy a breakable", false, 20.0f);

			// tell my friends to destroy it
			if (!m_isZombieBot)
			{
				for (const auto& bot : g_botManager->m_bots)
				{
					if (bot == nullptr)
						continue;

					if (m_team != bot->m_team)
						continue;

					if (!bot->m_isAlive)
						continue;

					if (bot->m_isZombieBot)
						continue;

					if (GetEntity() == bot->GetEntity())
						continue;

					TraceResult tr3;
					TraceLine(bot->EyePosition(), m_breakable, true, true, bot->GetEntity(), &tr3);
					if (tr3.pHit == entity)
					{
						bot->m_breakableEntity = entity;
						bot->m_breakable = tr3.vecEndPos;

						if (bot->m_currentWeapon == WEAPON_KNIFE)
							bot->m_destOrigin = bot->m_breakable;

						if (bot->pev->origin.z > bot->m_breakable.z)
							bot->m_campButtons = IN_DUCK;
						else
							bot->m_campButtons = bot->pev->button & IN_DUCK;

						bot->SetProcess(Process::DestroyBreakable, "trying to help my friend for destroy a breakable", false, 20.0f);
					}
				}
			}
		}
		else if (pev->origin.z > m_breakable.z) // make bots smarter
		{
			// tell my enemies to destroy it, so i will fall
			for (const auto& enemy : g_botManager->m_bots)
			{
				if (enemy == nullptr)
					continue;

				if (m_team == enemy->m_team)
					continue;

				if (!enemy->m_isAlive)
					continue;

				if (enemy->m_isZombieBot)
					continue;

				TraceResult tr3;
				TraceLine(enemy->EyePosition(), m_breakable, true, true, enemy->GetEntity(), &tr3);
				if (tr3.pHit == entity)
				{
					enemy->m_breakableEntity = entity;
					enemy->m_breakable = tr3.vecEndPos;

					if (enemy->pev->origin.z > enemy->m_breakable.z)
						enemy->m_campButtons = IN_DUCK;
					else
						enemy->m_campButtons = enemy->pev->button & IN_DUCK;

					enemy->SetProcess(Process::DestroyBreakable, "trying to destroy a breakable for my enemy fall", false, 20.0f);
				}
			}
		}
	}
}

void Bot::SetEnemy(edict_t* entity)
{
	if (FNullEnt(entity))
	{
		m_enemy = nullptr;
		return;
	}

	if (!IsAlive(entity))
		return;

	if (m_team == GetTeam(entity))
		return;

	m_enemy = entity;
}

void Bot::SetLastEnemy(edict_t* entity)
{
	if (FNullEnt(entity))
	{
		m_lastEnemy = nullptr;
		m_lastEnemyOrigin = nullvec;
		return;
	}

	if (!IsAlive(entity))
		return;

	if (m_team == GetTeam(entity))
		return;

	m_lastEnemy = entity;
	m_lastEnemyOrigin = GetEntityOrigin(entity);
}

void Bot::SetMoveTarget(edict_t* entity)
{
	if (FNullEnt(entity) || !IsAlive(entity) || m_team == GetTeam(entity))
	{
		m_moveTargetEntity = nullptr;
		m_moveTargetOrigin = nullvec;

		if (GetCurrentTask()->taskID == TASK_MOVETOTARGET)
		{
			RemoveCertainTask(TASK_MOVETOTARGET);
			m_prevGoalIndex = -1;
			GetCurrentTask()->data = -1;
		}

		return;
	}

	m_moveTargetOrigin = GetEntityOrigin(entity);
	m_states &= ~STATE_SEEINGENEMY;
	SetEnemy(nullptr);
	SetLastEnemy(nullptr);
	m_enemyUpdateTime = 0.0f;
	m_aimFlags &= ~AIM_ENEMY;

	if (m_moveTargetEntity == entity)
		return;

	SetEntityWaypoint(entity);
	SetEntityWaypoint(GetEntity(), GetEntityWaypoint(entity));

	m_moveTargetEntity = entity;
	PushTask(TASK_MOVETOTARGET, TASKPRI_MOVETOTARGET, -1, 0.0, true);
}

// this function find a node in the near of the bot if bot had lost his path of pathfinder needs
// to be restarted over again
int Bot::FindWaypoint(void)
{
	int busy = -1;
	float lessDist[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
	int lessIndex[3] = {-1, -1, -1};

	for (int at = 0; at < g_numWaypoints; at++)
	{
		if (!IsValidWaypoint(at))
			continue;
		
		if (m_team == TEAM_COUNTER && g_waypoint->GetPath(at)->flags & WAYPOINT_ZOMBIEONLY)
			continue;
		else if (m_team == TEAM_TERRORIST && g_waypoint->GetPath(at)->flags & WAYPOINT_HUMANONLY)
			continue;

		bool skip = !!(int(g_waypoint->GetPath(at)->index) == m_currentWaypointIndex);

		// skip current and recent previous nodes
		if (at == m_prevWptIndex[0] || at == m_prevWptIndex[1] || at == m_prevWptIndex[2])
			skip = true;

		// skip the current node, if any
		if (skip)
			continue;

		// skip if isn't visible
		if (!IsVisible(g_waypoint->GetPath(at)->origin, GetEntity()))
			continue;

		// cts with hostages should not pick
		if (m_team == TEAM_COUNTER && HasHostage())
			continue;

		// check we're have link to it
		if (IsValidWaypoint(m_currentWaypointIndex) && !g_waypoint->IsConnected(m_currentWaypointIndex, at))
			continue;

		// ignore non-reacheable nodes...
		if (!g_waypoint->IsNodeReachable(g_waypoint->GetPath(m_currentWaypointIndex)->origin, g_waypoint->GetPath(at)->origin))
			continue;

		// check if node is already used by another bot...
		if (IsWaypointOccupied(at))
		{
			busy = at;
			continue;
		}

		// if we're still here, find some close nodes
		const float distance = (pev->origin - g_waypoint->GetPath(at)->origin).GetLengthSquared();

		if (distance < lessDist[0])
		{
			lessDist[2] = lessDist[1];
			lessIndex[2] = lessIndex[1];

			lessDist[1] = lessDist[0];
			lessIndex[1] = lessIndex[0];

			lessDist[0] = distance;
			lessIndex[0] = at;
		}
		else if (distance < lessDist[1])
		{
			lessDist[2] = lessDist[1];
			lessIndex[2] = lessIndex[1];

			lessDist[1] = distance;
			lessIndex[1] = at;
		}
		else if (distance < lessDist[2])
		{
			lessDist[2] = distance;
			lessIndex[2] = at;
		}
	}

	int selected = -1;

	// now pick random one from choosen
	int index = 0;

	// choice from found
	if (IsValidWaypoint(lessIndex[2]))
		index = CRandomInt(0, 2);
	else if (IsValidWaypoint(lessIndex[1]))
		index = CRandomInt(0, 1);
	else if (IsValidWaypoint(lessIndex[0])) 
		index = 0;

	selected = lessIndex[index];

	// if we're still have no node and have busy one (by other bot) pick it up
	if (!IsValidWaypoint(selected) && IsValidWaypoint(busy)) 
		selected = busy;

	// worst case... find atleast something
	if (!IsValidWaypoint(selected))
		selected = g_waypoint->FindNearest(pev->origin + pev->velocity, 9999.0f, -1, GetEntity());

	ChangeWptIndex(selected);
	SetWaypointOrigin();
	return selected;
}

void Bot::IgnoreCollisionShortly(void)
{
	ResetCollideState();
	m_lastCollTime = engine->GetTime() + 1.0f;
}

void Bot::ResetStuck(void)
{
	m_isStuck = false;
	m_stuckWarn = 0;
	m_stuckTimer = AddTime(2.0f);
	m_jumpFinished = false;
}

void Bot::CheckStuck(const float maxSpeed)
{
	if (!ebot_has_semiclip.GetBool() && m_hasFriendsNear && !(m_waypointFlags & WAYPOINT_FALLRISK))
	{
		const Vector myOrigin = pev->origin + pev->velocity * m_frameInterval;
		const Vector friendOrigin = m_friendOrigin + m_nearestFriend->v.velocity * m_frameInterval;
		if ((myOrigin - friendOrigin).GetLengthSquared() <= SquaredF(m_nearestFriend->v.maxspeed * 0.33f))
		{
			// use our movement angles, try to predict where we should be next frame
			Vector right, forward;
			m_moveAngles.BuildVectors(&forward, &right, nullptr);

			const Vector dir = (myOrigin - friendOrigin).Normalize2D();
			bool moveBack = false;

			// to start strafing, we have to first figure out if the target is on the left side or right side
			if ((dir | right.Normalize2D()) > 0.0f)
			{
				if (!CheckWallOnRight())
					m_strafeSpeed = maxSpeed;
				else
				{
					moveBack = true;
					if (!CheckWallOnLeft())
						m_strafeSpeed = -maxSpeed;
				}
			}
			else
			{
				if (!CheckWallOnLeft())
					m_strafeSpeed = -maxSpeed;
				else
				{
					moveBack = true;
					if (!CheckWallOnRight())
						m_strafeSpeed = maxSpeed;
				}
			}

			bool doorStuck = false;
			if ((moveBack && m_stuckWarn >= 3) || m_stuckWarn >= 10)
			{
				if ((dir | forward.Normalize2D()) < 0.0f)
				{
					if (CheckWallOnBehind())
					{
						m_moveSpeed = maxSpeed;
						doorStuck = true;
					}
					else
						m_moveSpeed = -maxSpeed;
				}
				else
				{
					if (CheckWallOnForward())
					{
						m_moveSpeed = -maxSpeed;
						doorStuck = true;
					}
					else
						m_moveSpeed = maxSpeed;
				}
			}

			if (IsOnFloor() && m_stuckWarn >= 6)
			{
				if (!(m_nearestFriend->v.button & IN_JUMP) && !(m_nearestFriend->v.oldbuttons & IN_JUMP) && ((m_nearestFriend->v.button & IN_DUCK && m_nearestFriend->v.oldbuttons & IN_DUCK) || CanJumpUp(pev->velocity.SkipZ()) || CanJumpUp(dir)))
				{
					if (m_jumpTime + 1.0f < engine->GetTime())
						pev->button |= IN_JUMP;
				}
				else if (!(m_nearestFriend->v.button & IN_DUCK) && !(m_nearestFriend->v.oldbuttons & IN_DUCK))
				{
					if (m_duckTime < engine->GetTime())
						m_duckTime = engine->GetTime() + engine->RandomFloat(1.0f, 2.0f);
				}
			}
			
			if (doorStuck && m_stuckWarn >= 10) // ENOUGH!
			{
				extern ConVar ebot_ignore_enemies;
				if (!ebot_ignore_enemies.GetBool())
				{
					const bool friendlyFire = engine->IsFriendlyFireOn();
					if ((!friendlyFire && m_currentWeapon == WEAPON_KNIFE) || (friendlyFire && !IsValidBot(m_nearestFriend))) // DOOR STUCK! || DIE HUMAN!
					{
						m_lookAt = friendOrigin + (m_nearestFriend->v.view_ofs * 0.9f);
						m_pauseTime = 0.0f;
						if (!(pev->button & IN_ATTACK) && !(pev->oldbuttons & IN_ATTACK))
							pev->button |= IN_ATTACK;

						// WHO SAID BOT'S DON'T HAVE A FEELINGS?
						// then show it, or act as...
						if (m_personality == PERSONALITY_CAREFUL)
						{
							if (friendlyFire)
								ChatSay(false, "YOU'RE NOT ONE OF US!");
							else
								ChatSay(false, "I'M STUCK!");
						}
						else if (m_personality == PERSONALITY_RUSHER)
						{
							if (friendlyFire)
								ChatSay(false, "DIE HUMAN!");
							else
								ChatSay(false, "GET OUT OF MY WAY!");
						}
						else
						{
							if (friendlyFire)
								ChatSay(false, "YOU GAVE ME NO CHOICE!");
							else
								ChatSay(false, "DOOR STUCK!");
						}
					}
					else
						SelectKnife();
				}
			}
		}
	}
	else if (m_stuckWarn >= 2)
	{
		bool moveBack = false;

		if (!CheckWallOnRight())
			m_strafeSpeed = maxSpeed;
		else
		{
			moveBack = true;
			if (!CheckWallOnLeft())
				m_strafeSpeed = -maxSpeed;
		}

		if (!CheckWallOnLeft())
			m_strafeSpeed = -maxSpeed;
		else
		{
			moveBack = true;
			if (!CheckWallOnRight())
				m_strafeSpeed = maxSpeed;
		}

		if (moveBack && CheckWallOnBehind())
			m_moveSpeed = maxSpeed;
	}

	if (!m_isSlowThink)
		return;

	const float distance = ((pev->origin + pev->velocity * m_frameInterval) - m_stuckArea).GetLengthSquared2D();
	float range = maxSpeed * 2.2f;
	if (distance < range)
	{
		m_stuckWarn++;

		if (m_stuckWarn >= 20)
		{
			m_stuckWarn = 10;
			DeleteSearchNodes();
			m_currentWaypointIndex = -1;
			FindWaypoint();
		}

		if (m_stuckWarn > 10)
			m_isStuck = true;
		else if (m_stuckWarn == 10)
			pev->button |= IN_JUMP;
	}
	else
	{
		// are we teleported? O_O
		if (distance > SquaredF(range))
		{
			DeleteSearchNodes();
			m_currentWaypointIndex = -1;
			FindWaypoint();
		}

		if (m_stuckWarn > 0)
			m_stuckWarn -= 1;

		if (m_stuckWarn < 5)
			m_isStuck = false;
	}

	if (m_stuckTimer < engine->GetTime())
	{
		m_stuckArea = (pev->origin + pev->velocity * -m_frameInterval);
		m_stuckTimer = AddTime(2.0f);
	}
}

void Bot::SetWaypointOrigin(void)
{
	const int myIndex = m_currentWaypointIndex;
	const int32 myFlags = g_waypoint->GetPath(myIndex)->flags;
	m_waypointOrigin = g_waypoint->GetPath(myIndex)->origin;

	const float radius = g_waypoint->GetPath(myIndex)->radius;
	if (radius > 0)
	{
		MakeVectors(Vector(pev->angles.x, AngleNormalize(pev->angles.y + engine->RandomFloat(-90.0f, 90.0f)), 0.0f));
		int sPoint = -1;

		if (&m_navNode[0] != nullptr && m_navNode->next != nullptr)
		{
			Vector waypointOrigin[5];
			for (int i = 0; i < 5; i++)
			{
				waypointOrigin[i] = m_waypointOrigin;
				waypointOrigin[i] += Vector(engine->RandomFloat(-radius, radius), engine->RandomFloat(-radius, radius), 0.0f);
			}

			float sDistance = FLT_MAX;
			for (int i = 0; i < 5; i++)
			{
				const float distance = (pev->origin - waypointOrigin[i]).GetLengthSquared();

				if (distance < sDistance)
				{
					sPoint = i;
					sDistance = distance;
				}
			}

			if (sPoint != -1)
				m_waypointOrigin = waypointOrigin[sPoint];
		}

		if (sPoint == -1)
			m_waypointOrigin += g_pGlobals->v_forward;
	}
}

// checks if the last waypoint the bot was heading for is still valid
void Bot::GetValidWaypoint(void)
{
	DeleteSearchNodes();
	FindWaypoint();
}

void Bot::ChangeWptIndex(const int waypointIndex)
{
	m_currentWaypointIndex = waypointIndex;

	if (m_prevWptIndex[0] != m_currentWaypointIndex)
	{
		m_prevWptIndex[0] = m_currentWaypointIndex;
		m_prevWptIndex[1] = m_prevWptIndex[0];
		m_prevWptIndex[2] = m_prevWptIndex[1];
	}

	m_waypointFlags = g_waypoint->GetPath(m_currentWaypointIndex)->flags;
	m_jumpFinished = false;
}

int Bot::ChooseBombWaypoint(void)
{
	// this function finds the best goal (bomb) waypoint for CTs when searching for a planted bomb.

	if (g_waypoint->m_goalPoints.IsEmpty())
		return CRandomInt(0, g_numWaypoints - 1); // reliability check

	Vector bombOrigin = CheckBombAudible();

	// if bomb returns no valid vector, return the current bot pos
	if (bombOrigin == nullvec)
		bombOrigin = pev->origin;

	Array <int> goals;

	int goal = 0, count = 0;
	float lastDistance = FLT_MAX;

	// find nearest goal waypoint either to bomb (if "heard" or player)
	ITERATE_ARRAY(g_waypoint->m_goalPoints, i)
	{
		float distance = (g_waypoint->GetPath(g_waypoint->m_goalPoints[i])->origin - bombOrigin).GetLengthSquared2D();

		// check if we got more close distance
		if (distance < lastDistance)
		{
			goal = g_waypoint->m_goalPoints[i];
			lastDistance = distance;

			goals.Push(goal);
		}
	}

	while (g_waypoint->IsGoalVisited(goal))
	{
		if (g_waypoint->m_goalPoints.GetElementNumber() == 1)
			goal = g_waypoint->m_goalPoints[0];
		else
			goal = goals.GetRandomElement();

		if (count++ >= goals.GetElementNumber())
			break;
	}

	return goal;
}

int Bot::FindDefendWaypoint(const Vector& origin)
{
	// where to defend?
	if (origin == nullvec)
		return -1;

	// no camp waypoints
	if (g_waypoint->m_campPoints.IsEmpty())
		return CRandomInt(0, g_numWaypoints - 1);

	// invalid index
	if (!IsValidWaypoint(m_currentWaypointIndex))
		return g_waypoint->m_campPoints.GetRandomElement();

	Array <int> BestSpots;
	Array <int> OkSpots;
	Array <int> WorstSpots;

	for (int i = 0; i < g_waypoint->m_campPoints.GetElementNumber(); i++)
	{
		int index = -1;
		g_waypoint->m_campPoints.GetAt(i, index);

		if (!IsValidWaypoint(index))
			continue;

		if (g_waypoint->GetPath(index)->flags & WAYPOINT_LADDER)
			continue;

		if (GetGameMode() == MODE_BASE)
		{
			if (m_team == TEAM_COUNTER && g_waypoint->GetPath(index)->flags & WAYPOINT_TERRORIST)
				continue;
			else if (g_waypoint->GetPath(index)->flags & WAYPOINT_COUNTER)
				continue;
		}

		if (!IsWaypointOccupied(index))
		{
			TraceResult tr{};
			TraceLine(g_waypoint->GetPath(index)->origin, origin, true, true, GetEntity(), &tr);

			if (tr.flFraction == 1.0f) // distance isn't matter
				BestSpots.Push(index);
			else if ((g_waypoint->GetPath(index)->origin - origin).GetLengthSquared() <= SquaredF(1024.0f))
				OkSpots.Push(index);
			else
				WorstSpots.Push(index);
		}
	}

	int BestIndex = -1;

	if (!BestSpots.IsEmpty() && !IsValidWaypoint(BestIndex))
		BestIndex = BestSpots.GetRandomElement();
	else if (!OkSpots.IsEmpty() && !IsValidWaypoint(BestIndex))
		BestIndex = OkSpots.GetRandomElement();
	else if (!WorstSpots.IsEmpty() && !IsValidWaypoint(BestIndex))
		BestIndex = WorstSpots.GetRandomElement();
	
	if (IsValidWaypoint(BestIndex))
		return BestIndex;

	return -1;
}

int Bot::FindCoverWaypoint(float maxDistance)
{
	// really?
	if (maxDistance < SquaredF(512.0f))
		maxDistance = SquaredF(512.0f);

	// do not move to a position near to the enemy
	float enemydist = (m_lastEnemyOrigin - pev->origin).GetLengthSquared2D();
	if (maxDistance > enemydist)
		maxDistance = enemydist;

	Array <int> BestSpots;
	Array <int> OkSpots;

	int ChoosenIndex = -1;

	for (int i = 0; i < g_numWaypoints; i++)
	{
		if (g_waypoint->GetPath(i)->flags & WAYPOINT_LADDER)
			continue;

		if (g_waypoint->GetPath(i)->flags & WAYPOINT_AVOID)
			continue;

		if (g_waypoint->GetPath(i)->flags & WAYPOINT_FALLCHECK)
			continue;

		if (!IsWaypointOccupied(i))
		{
			TraceResult tr{};
			Vector origin = !FNullEnt(m_enemy) ? GetPlayerHeadOrigin(m_enemy) : m_lastEnemyOrigin;
			TraceLine(g_waypoint->GetPath(i)->origin, origin, true, true, GetEntity(), &tr);

			if (tr.flFraction != 1.0f)
			{
				if ((g_waypoint->GetPath(i)->origin - origin).GetLengthSquared2D() <= SquaredF(maxDistance))
					BestSpots.Push(i);
				else
					OkSpots.Push(i);
			}
		}
	}

	if (!BestSpots.IsEmpty() && !IsValidWaypoint(ChoosenIndex))
	{
		float maxdist = maxDistance;

		for (int i = 0; i < BestSpots.GetElementNumber(); i++)
		{
			if (!IsValidWaypoint(i))
				continue;

			float distance = (pev->origin - g_waypoint->GetPath(i)->origin).GetLengthSquared2D();

			if (distance < maxdist)
			{
				ChoosenIndex = i;
				maxdist = distance;
			}
		}
	}
	else if (!OkSpots.IsEmpty() && !IsValidWaypoint(ChoosenIndex))
	{
		float maxdist = FLT_MAX;
		for (int i = 0; i < OkSpots.GetElementNumber(); i++)
		{
			if (!IsValidWaypoint(i))
				continue;

			float distance = (pev->origin - g_waypoint->GetPath(i)->origin).GetLengthSquared2D();

			if (distance < maxdist)
			{
				ChoosenIndex = i;
				maxdist = distance;
			}
		}
	}

	if (IsValidWaypoint(ChoosenIndex))
		return ChoosenIndex;

	return -1; // do not use random points
}

// this function does a realtime postprocessing of waypoints return from the
// pathfinder, to vary paths and find the best waypoint on our way
bool Bot::GetBestNextWaypoint(void)
{
	InternalAssert(m_navNode != nullptr);
	InternalAssert(m_navNode->next != nullptr);

	if (!IsWaypointOccupied(m_navNode->index))
		return false;

	for (int i = 0; i < Const_MaxPathIndex; i++)
	{
		const int id = g_waypoint->GetPath(m_currentWaypointIndex)->index[i];

		if (IsValidWaypoint(id) && g_waypoint->IsConnected(id, m_navNode->next->index) && g_waypoint->IsConnected(m_currentWaypointIndex, id))
		{
			if (g_waypoint->GetPath(id)->flags & WAYPOINT_LADDER || g_waypoint->GetPath(id)->flags & WAYPOINT_CAMP || g_waypoint->GetPath(id)->flags & WAYPOINT_JUMP || g_waypoint->GetPath(id)->flags & WAYPOINT_DJUMP) // don't use these waypoints as alternative
				continue;

			if (!IsWaypointOccupied(id))
			{
				m_navNode->index = id;
				return true;
			}
		}
	}

	return false;
}

bool Bot::HasNextPath(void)
{
	return (m_navNode != nullptr && m_navNode->next != nullptr);
}

// checks if bot is blocked in his movement direction (excluding doors)
bool Bot::CantMoveForward(const Vector normal)
{
	// first do a trace from the bot's eyes forward...
	Vector src = EyePosition();
	Vector forward = src + normal * 24.0f;

	MakeVectors(Vector(0.0f, pev->angles.y, 0.0f));

	TraceResult* tr;

	// trace from the bot's eyes straight forward...
	TraceLine(src, forward, true, GetEntity(), tr);

	// check if the trace hit something...
	if (tr->flFraction < 1.0f)
	{
		if (cstrncmp("func_door", STRING(tr->pHit->v.classname), 9) == 0)
			return false;

		return true;  // bot's head will hit something
	}

	// bot's head is clear, check at shoulder level...
	// trace from the bot's shoulder left diagonal forward to the right shoulder...
	src = EyePosition() + Vector(0.0f, 0.0f, -16.0f) - g_pGlobals->v_right * 16.0f;
	forward = EyePosition() + Vector(0.0f, 0.0f, -16.0f) + g_pGlobals->v_right * 16.0f + normal * 24.0f;

	TraceLine(src, forward, true, GetEntity(), tr);

	// check if the trace hit something...
	if (tr->flFraction < 1.0f && cstrncmp("func_door", STRING(tr->pHit->v.classname), 9) != 0)
		return true;  // bot's body will hit something

	 // bot's head is clear, check at shoulder level...
	 // trace from the bot's shoulder right diagonal forward to the left shoulder...
	src = EyePosition() + Vector(0.0f, 0.0f, -16.0f) + g_pGlobals->v_right * 16.0f;
	forward = EyePosition() + Vector(0.0f, 0.0f, -16.0f) - g_pGlobals->v_right * 16.0f + normal * 24.0f;

	TraceLine(src, forward, true, GetEntity(), tr);

	// check if the trace hit something...
	if (tr->flFraction < 1.0f && cstrncmp("func_door", STRING(tr->pHit->v.classname), 9) != 0)
		return true;  // bot's body will hit something

	 // now check below waist
	if (pev->flags & FL_DUCKING)
	{
		src = pev->origin + Vector(0.0f, 0.0f, -19.0f + 19.0f);
		forward = src + Vector(0.0f, 0.0f, 10.0f) + normal * 24.0f;

		TraceLine(src, forward, true, GetEntity(), tr);

		// check if the trace hit something...
		if (tr->flFraction < 1.0f && cstrncmp("func_door", STRING(tr->pHit->v.classname), 9) != 0)
			return true;  // bot's body will hit something

		src = pev->origin;
		forward = src + normal * 24.0f;

		TraceLine(src, forward, true, GetEntity(), tr);

		// check if the trace hit something...
		if (tr->flFraction < 1.0f && cstrncmp("func_door", STRING(tr->pHit->v.classname), 9) != 0)
			return true;  // bot's body will hit something
	}
	else
	{
		// trace from the left waist to the right forward waist pos
		src = pev->origin + Vector(0.0f, 0.0f, -17.0f) - g_pGlobals->v_right * 16.0f;
		forward = pev->origin + Vector(0.0f, 0.0f, -17.0f) + g_pGlobals->v_right * 16.0f + normal * 24.0f;

		// trace from the bot's waist straight forward...
		TraceLine(src, forward, true, GetEntity(), tr);

		// check if the trace hit something...
		if (tr->flFraction < 1.0f && cstrncmp("func_door", STRING(tr->pHit->v.classname), 9) != 0)
			return true;  // bot's body will hit something

		 // trace from the left waist to the right forward waist pos
		src = pev->origin + Vector(0.0f, 0.0f, -17.0f) + g_pGlobals->v_right * 16.0f;
		forward = pev->origin + Vector(0.0f, 0.0f, -17.0f) - g_pGlobals->v_right * 16.0f + normal * 24.0f;

		TraceLine(src, forward, true, GetEntity(), tr);

		// check if the trace hit something...
		if (tr->flFraction < 1.0f && cstrncmp("func_door", STRING(tr->pHit->v.classname), 9) != 0)
			return true;  // bot's body will hit something
	}

	return false;  // bot can move forward, return false
}

bool Bot::CanJumpUp(const Vector normal)
{
	// this function check if bot can jump over some obstacle

	TraceResult tr;

	// can't jump if not on ground and not on ladder/swimming
	if (!IsOnFloor() && (IsOnLadder() || !IsInWater()))
		return false;

	MakeVectors(Vector(0.0f, pev->angles.y, 0.0f));

	// check for normal jump height first...
	Vector src = pev->origin + Vector(0.0f, 0.0f, -36.0f + 45.0f);
	Vector dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, true, GetEntity(), &tr);

	if (tr.flFraction < 1.0f)
		goto CheckDuckJump;
	else
	{
		// now trace from jump height upward to check for obstructions...
		src = dest;
		dest.z = dest.z + 37.0f;

		TraceLine(src, dest, true, GetEntity(), &tr);

		if (tr.flFraction < 1.0f)
			return false;
	}

	// now check same height to one side of the bot...
	src = pev->origin + g_pGlobals->v_right * 16.0f + Vector(0.0f, 0.0f, -36.0f + 45.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, true, GetEntity(), &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		goto CheckDuckJump;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, true, GetEntity(), &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now check same height on the other side of the bot...
	src = pev->origin + (-g_pGlobals->v_right * 16.0f) + Vector(0.0f, 0.0f, -36.0f + 45.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, true, GetEntity(), &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		goto CheckDuckJump;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, true, GetEntity(), &tr);

	// if trace hit something, return false
	return tr.flFraction > 1.0f;

	// here we check if a duck jump would work...
CheckDuckJump:
	// use center of the body first... maximum duck jump height is 62, so check one unit above that (63)
	src = pev->origin + Vector(0.0f, 0.0f, -36.0f + 63.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, true, GetEntity(), &tr);

	if (tr.flFraction < 1.0f)
		return false;
	else
	{
		// now trace from jump height upward to check for obstructions...
		src = dest;
		dest.z = dest.z + 37.0f;

		TraceLine(src, dest, true, GetEntity(), &tr);

		// if trace hit something, check duckjump
		if (tr.flFraction < 1.0f)
			return false;
	}

	// now check same height to one side of the bot...
	src = pev->origin + g_pGlobals->v_right * 16.0f + Vector(0.0f, 0.0f, -36.0f + 63.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, true, GetEntity(), &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, true, GetEntity(), &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now check same height on the other side of the bot...
	src = pev->origin + (-g_pGlobals->v_right * 16.0f) + Vector(0.0f, 0.0f, -36.0f + 63.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, true, GetEntity(), &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, true, GetEntity(), &tr);

	// if trace hit something, return false
	return tr.flFraction > 1.0f;
}

bool Bot::CheckWallOnForward(void)
{
	TraceResult tr;
	MakeVectors(pev->angles);

	// do a trace to the forward...
	TraceLine(pev->origin, pev->origin + g_pGlobals->v_forward * 54.0f, false, false, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction != 1.0f)
	{
		m_lastWallOrigin = tr.vecEndPos;
		return true;
	}
	else
	{
		TraceResult tr2;
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, false, false, GetEntity(), &tr2);

		// we don't want fall
		if (tr2.flFraction == 1.0f)
		{
			m_lastWallOrigin = pev->origin;
			return true;
		}
	}

	return false;
}

bool Bot::CheckWallOnBehind(void)
{
	TraceResult tr;
	MakeVectors(pev->angles);

	// do a trace to the behind...
	TraceLine(pev->origin, pev->origin - g_pGlobals->v_forward * 54.0f, false, false, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction != 1.0f)
	{
		m_lastWallOrigin = tr.vecEndPos;
		return true;
	}
	else
	{
		TraceResult tr2;
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, false, false, GetEntity(), &tr2);

		// we don't want fall
		if (tr2.flFraction == 1.0f)
		{
			m_lastWallOrigin = pev->origin;
			return true;
		}
	}

	return false;
}

bool Bot::CheckWallOnLeft(void)
{
	TraceResult tr;
	MakeVectors(pev->angles);

	// do a trace to the left...
	TraceLine(pev->origin, pev->origin - g_pGlobals->v_right * 54.0f, false, false, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction != 1.0f)
	{
		m_lastWallOrigin = tr.vecEndPos;
		return true;
	}
	else
	{
		TraceResult tr2;
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, false, false, GetEntity(), &tr2);

		// we don't want fall
		if (tr2.flFraction == 1.0f)
		{
			m_lastWallOrigin = pev->origin;
			return true;
		}
	}

	return false;
}

bool Bot::CheckWallOnRight(void)
{
	TraceResult tr;
	MakeVectors(pev->angles);

	// do a trace to the right...
	TraceLine(pev->origin, pev->origin + g_pGlobals->v_right * 54.0f, false, false, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction != 1.0f)
	{
		m_lastWallOrigin = tr.vecEndPos;
		return true;
	}
	else
	{
		TraceResult tr2;
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, false, false, GetEntity(), &tr2);

		// we don't want fall
		if (tr2.flFraction == 1.0f)
		{
			m_lastWallOrigin = pev->origin;
			return true;
		}
	}

	return false;
}

// this function eturns if given location would hurt Bot with falling damage
bool Bot::IsDeadlyDrop(const Vector targetOriginPos)
{
	const Vector botPos = pev->origin;
	TraceResult tr;

	const Vector move((targetOriginPos - botPos).ToYaw(), 0.0f, 0.0f);
	MakeVectors(move);

	const Vector direction = (targetOriginPos - botPos).Normalize();  // 1 unit long
	Vector check = botPos;
	Vector down = botPos;

	down.z = down.z - 1000.0f;  // straight down 1000 units

	TraceHull(check, down, true, head_hull, GetEntity(), &tr);

	if (tr.flFraction > 0.036f) // We're not on ground anymore?
		tr.flFraction = 0.036f;

	float height;
	float lastHeight = tr.flFraction * 1000.0f;  // height from ground

	float distance = (targetOriginPos - check).GetLengthSquared();  // distance from goal

	while (distance > SquaredF(16.0f))
	{
		check = check + direction * 16.0f; // move 16 units closer to the goal...

		down = check;
		down.z = down.z - 1000.0f;  // straight down 1000 units

		TraceHull(check, down, true, head_hull, GetEntity(), &tr);

		if (tr.fStartSolid) // Wall blocking?
			return false;

		height = tr.flFraction * 1000.0f;  // height from ground

		if (lastHeight < height - 100.0f) // Drops more than 100 Units?
			return true;

		lastHeight = height;
		distance = (targetOriginPos - check).GetLengthSquared();  // distance from goal
	}

	return false;
}

void Bot::CheckCloseAvoidance(const Vector& dirNormal)
{
	if (pev->solid == SOLID_NOT)
		return;

	float distance = SquaredF(384.0f);
	const float maxSpeed = SquaredF(pev->maxspeed);
	m_avoid = nullptr;

	// get our priority
	const unsigned int myPri = GetPlayerPriority(GetEntity());

	// find nearest player to bot
	for (const auto& client : g_clients)
	{
		// only valid meat
		if (FNullEnt(client.ent))
			continue;

		// need only good meat
		if (!(client.flags & CFLAG_USED))
			continue;

		// and still alive meat
		if (!(client.flags & CFLAG_ALIVE))
			continue;

		// our team, alive and not myself?
		if (client.team != m_team || client.ent == GetEntity())
			continue;

		// get priority of other player
		const unsigned int otherPri = GetPlayerPriority(client.ent);

		// if our priority is better, don't budge
		if (myPri < otherPri)
			continue;

		// they are higher priority - make way, unless we're already making way for someone more important
		if (!FNullEnt(m_avoid) && m_avoid != client.ent)
		{
			const unsigned int avoidPri = GetPlayerPriority(m_avoid);
			if (avoidPri < otherPri) // ignore because we're already avoiding someone better
				continue;
		}

		float nearest = (pev->origin - client.ent->v.origin).GetLengthSquared();
		if (nearest <= maxSpeed && nearest < distance)
		{
			m_avoid = client.ent;
			distance = nearest;
		}
	}

	// found somebody?
	if (FNullEnt(m_avoid))
		return;

	// don't get stuck in small areas, follow the same goal
	if (m_isStuck)
	{
		Bot* otherBot = g_botManager->GetBot(m_avoid);
		if (otherBot != nullptr)
		{
			m_tasks->data = otherBot->m_tasks->data;
			m_prevGoalIndex = otherBot->m_prevGoalIndex;
			m_chosenGoalIndex = otherBot->m_chosenGoalIndex;
			int index = m_tasks->data;
			if (index == -1)
				index = m_chosenGoalIndex;

			if (index == -1)
				index = m_prevGoalIndex;

			if (index != -1)
			{
				FindPath(m_currentWaypointIndex, index);
				otherBot->FindPath(m_currentWaypointIndex, index);
			}
		}
	}

	const float interval = m_frameInterval * 8.0f;

	// use our movement angles, try to predict where we should be next frame
	Vector right, forward;
	m_moveAngles.BuildVectors(&forward, &right, nullptr);

	Vector predict = pev->origin + forward * m_moveSpeed * interval;

	predict += right * m_strafeSpeed * interval;
	predict += pev->velocity * interval;

	const float movedDistance = (predict - m_avoid->v.origin).GetLengthSquared();
	const float nextFrameDistance = (pev->origin - (m_avoid->v.origin + m_avoid->v.velocity * interval)).GetLengthSquared();

	// is player that near now or in future that we need to steer away?
	if (movedDistance <= SquaredF(64.0f) || (distance <= SquaredF(72.0f) && nextFrameDistance < distance))
	{
		const auto dir = (pev->origin - m_avoid->v.origin).Normalize2D();

		// to start strafing, we have to first figure out if the target is on the left side or right side
		if ((dir | right.Normalize2D()) > 0.0f)
			SetStrafeSpeed(dirNormal, pev->maxspeed);
		else
			SetStrafeSpeed(dirNormal, -pev->maxspeed);

		if (distance <= SquaredF(80.0f))
		{
			if ((dir | forward.Normalize2D()) < 0.0f)
				m_moveSpeed = -pev->maxspeed;
		}

		return;
	}
}

int Bot::GetCampAimingWaypoint(void)
{
	Array <int> BestWaypoints;
	Array <int> OkWaypoints;

	int currentWay = m_currentWaypointIndex;

	if (!IsValidWaypoint(currentWay))
		currentWay = g_waypoint->FindNearest(pev->origin);

	int DangerWaypoint = -1;
	for (int i = 0; i < g_numWaypoints; i++)
	{
		if (!IsValidWaypoint(i))
			continue;

		if (currentWay == i)
			continue;

		if (!IsVisible(g_waypoint->GetPath(i)->origin, GetEntity()))
			continue;

		if ((g_waypoint->GetPath(i)->origin - pev->origin).GetLengthSquared() > SquaredF(512.0f))
			BestWaypoints.Push(i);
		else
			OkWaypoints.Push(i);
	}

	if (!BestWaypoints.IsEmpty())
		return BestWaypoints.GetRandomElement();
	else if (!OkWaypoints.IsEmpty())
		return OkWaypoints.GetRandomElement();

	return g_waypoint->m_otherPoints.GetRandomElement();
}

void Bot::FacePosition(void)
{
	const int aimType = ebot_aim_type.GetInt();
	if (aimType == 0)
	{
		Vector direction = (m_lookAt - EyePosition()).ToAngles() + pev->punchangle;
		direction.x = -direction.x; // invert for engine

		if (direction.x < -89.0f)
			direction.x = -89.0f;
		else if (direction.x > 89.0f)
			direction.x = 89.0f;

		pev->v_angle = direction;
		pev->angles.x = -pev->v_angle.x * 0.33333333333f;
		pev->angles.y = pev->v_angle.y;
		return;
	}

	const float delta = engine->GetTime() - m_aimInterval;
	m_aimInterval = engine->GetTime();

	if (aimType == 2)
	{
		m_idealAngles = pev->v_angle;
		Vector direction = (m_lookAt - EyePosition()).ToAngles() + pev->punchangle;
		direction.x = -direction.x; // invert for engine

		const float aimSpeed = ((m_skill * 0.033f) + 9.0f) * delta;

		m_idealAngles.x += AngleNormalize(direction.x - m_idealAngles.x) * aimSpeed;
		m_idealAngles.y += AngleNormalize(direction.y - m_idealAngles.y) * aimSpeed;

		if (m_idealAngles.x < -89.0f)
			m_idealAngles.x = -89.0f;
		else if (m_idealAngles.x > 89.0f)
			m_idealAngles.x = 89.0f;

		pev->v_angle = m_idealAngles;
		pev->angles.x = -pev->v_angle.x * 0.33333333333f;
		pev->angles.y = pev->v_angle.y;
		return;
	}

	// adjust all body and view angles to face an absolute vector
	Vector direction = (m_lookAt - EyePosition()).ToAngles() + pev->punchangle;
	direction.x = -direction.x; // invert for engine

	float accelerate = float(m_skill) * 40.0f;
	float stiffness = float(m_skill) * 4.0f;
	float damping = float(m_skill) * 0.25f;

	m_idealAngles = pev->v_angle;

	float angleDiffPitch = AngleNormalize(direction.x - m_idealAngles.x);
	float angleDiffYaw = AngleNormalize(direction.y - m_idealAngles.y);

	float lockn = (m_skill + 50.0f) * delta;

	if (IsZombieMode() && !m_isZombieBot && m_isEnemyReachable)
	{
		accelerate *= 2.0f;
		stiffness *= 2.0f;
		damping *= 2.0f;
		lockn *= 2.0f;
	}

	if (angleDiffYaw <= lockn && angleDiffYaw >= -lockn)
	{
		m_lookYawVel = 0.0f;
		m_idealAngles.y = direction.y;
		m_aimStopTime = AddTime(engine->RandomFloat(0.25f, 1.25f));
	}
	else
	{
		const float accel = cclampf((stiffness * angleDiffYaw) - (damping * m_lookYawVel), -accelerate, accelerate);
		m_lookYawVel += delta * accel;
		m_idealAngles.y += delta * m_lookYawVel;
	}

	if (angleDiffPitch <= lockn && angleDiffPitch >= -lockn)
	{
		m_lookPitchVel = 0.0f;
		m_idealAngles.x = direction.x;
	}
	else
	{
		const float accel = cclampf(stiffness * angleDiffPitch - (damping * m_lookPitchVel), -accelerate, accelerate);
		m_lookPitchVel += delta * accel;
		m_idealAngles.x += delta * m_lookPitchVel;
	}

	if (m_idealAngles.x < -89.0f)
		m_idealAngles.x = -89.0f;
	else if (m_idealAngles.x > 89.0f)
		m_idealAngles.x = 89.0f;

	pev->v_angle = m_idealAngles;

	// set the body angles to point the gun correctly
	pev->angles.x = -pev->v_angle.x * 0.33333333333f;
	pev->angles.y = pev->v_angle.y;
}

void Bot::SetStrafeSpeed(Vector moveDir, float strafeSpeed)
{
	MakeVectors(pev->angles);

	Vector los = (moveDir - pev->origin).Normalize2D();
	float dot = los | g_pGlobals->v_forward.SkipZ();

	if (CheckWallOnBehind())
	{
		if (CheckWallOnRight())
			m_tempstrafeSpeed = -strafeSpeed;
		else if (CheckWallOnLeft())
			m_tempstrafeSpeed = strafeSpeed;
		
		m_strafeSpeed = m_tempstrafeSpeed;

		if ((m_isStuck || pev->speed >= pev->maxspeed) && !IsOnLadder() && m_jumpTime + 5.0f < engine->GetTime() && IsOnFloor())
			pev->button |= IN_JUMP;
	}
	else if (dot > 0 && !CheckWallOnRight())
		m_strafeSpeed = strafeSpeed;
	else if (!CheckWallOnLeft())
		m_strafeSpeed = -strafeSpeed;
	else if (GetCurrentTask()->taskID == TASK_CAMP)
		m_lastEnemy = nullptr;
}

// find hostage improve
int Bot::FindHostage(void)
{
	if (m_team != TEAM_COUNTER || !(g_mapType & MAP_CS) || m_isZombieBot)
		return -1;

	edict_t* ent = nullptr;

	while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "hostage_entity")))
	{
		bool canF = true;

		for (const auto& bot : g_botManager->m_bots)
		{
			if (bot != nullptr && bot->m_isAlive)
			{
				for (int j = 0; j < Const_MaxHostages; j++)
				{
					if (bot->m_hostages[j] == ent)
						canF = false;
				}
			}
		}

		const Vector entOrigin = GetEntityOrigin(ent);
		const int nearestIndex = g_waypoint->FindNearest(entOrigin, 512.0f, -1, ent);

		if (IsValidWaypoint(nearestIndex) && canF)
			return nearestIndex;
		else
		{
			// do we need second try?
			const int nearestIndex2 = g_waypoint->FindNearest(entOrigin);

			if (IsValidWaypoint(nearestIndex2) && canF)
				return nearestIndex2;
		}
	}

	return -1;
}

int Bot::FindLoosedBomb(void)
{
	if (GetGameMode() != MODE_BASE || !(g_mapType & MAP_DE))
		return -1;

	edict_t* bombEntity = nullptr; // temporaly pointer to bomb

	// search the bomb on the map
	while (!FNullEnt(bombEntity = FIND_ENTITY_BY_CLASSNAME(bombEntity, "weaponbox")))
	{
		if (cstrcmp(STRING(bombEntity->v.model) + 9, "backpack.mdl") == 0)
		{
			const Vector bombOrigin = GetEntityOrigin(bombEntity);
			const int nearestIndex = g_waypoint->FindNearest(bombOrigin, 512.0f, -1, bombEntity);
			if (IsValidWaypoint(nearestIndex))
				return nearestIndex;
			else
			{
				// do we need second try?
				const int nearestIndex2 = g_waypoint->FindNearest(bombOrigin);

				if (IsValidWaypoint(nearestIndex2))
					return nearestIndex2;
			}

			break;
		}
	}

	return -1;
}

bool Bot::IsWaypointOccupied(int index, bool needZeroVelocity)
{
	if (!IsValidWaypoint(index))
		return false;

	if (pev->solid == SOLID_NOT)
		return false;

	for (const auto& client : g_clients)
	{
		if (FNullEnt(client.ent) || !(client.flags & (CFLAG_USED | CFLAG_ALIVE)) || client.team != m_team || client.ent == GetEntity())
			continue;

		// do not check clients far away from us
		if ((pev->origin - client.origin).GetLengthSquared() > SquaredF(320.0f))
			continue;

		if (needZeroVelocity && client.ent->v.velocity != nullvec)
			continue;

		const auto path = g_waypoint->GetPath(index);
		const float length = (client.origin - path->origin).GetLengthSquared();
		if (length < cclampf(SquaredF(path->radius) * 2.0f, SquaredF(40.0f), SquaredF(90.0f)))
			return true;

		auto bot = g_botManager->GetBot(client.index);
		if (bot == nullptr || bot == this || !bot->m_isAlive)
			continue;

		return bot->m_currentWaypointIndex == index;
	}

	return false;
}

// this function tries to find nearest to current bot button, and returns pointer to
// it's entity, also here must be specified the target, that button must open
edict_t* Bot::FindNearestButton(const char* className)
{
	if (IsNullString(className))
		return nullptr;

	float nearestDistance = FLT_MAX;
	edict_t* searchEntity = nullptr;
	edict_t* foundEntity = nullptr;

	// find the nearest button which can open our target
	while (!FNullEnt(searchEntity = FIND_ENTITY_BY_TARGET(searchEntity, className)))
	{
		float distance = (pev->origin - GetEntityOrigin(searchEntity)).GetLengthSquared();
		if (distance < nearestDistance)
		{
			nearestDistance = distance;
			foundEntity = searchEntity;
		}
	}

	return foundEntity;
}