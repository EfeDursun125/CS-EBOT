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
// all copies or substantial portions of the Software.ebot_aim_spring_stiffness_y
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

ConVar ebot_zombies_as_path_cost("ebot_zombie_count_as_path_cost", "1");
ConVar ebot_has_semiclip("ebot_has_semiclip", "0");
ConVar ebot_use_old_jump_method("ebot_use_old_jump_method", "0");
ConVar ebot_breakable_health_limit("ebot_breakable_health_limit", "3000.0");

int Bot::FindGoal(void)
{
	m_prevGoalIndex = m_chosenGoalIndex;

	if (IsZombieMode())
	{
		if (m_isZombieBot)
		{
			if (g_waypoint->m_terrorPoints.IsEmpty())
				return m_chosenGoalIndex = crandomint(0, g_numWaypoints - 1);
			else
				return m_chosenGoalIndex = g_waypoint->m_terrorPoints.GetRandomElement();
		}
		else if (IsValidWaypoint(m_myMeshWaypoint))
			return m_chosenGoalIndex = m_myMeshWaypoint;
		else if (IsValidWaypoint(m_zhCampPointIndex))
			return m_chosenGoalIndex = m_zhCampPointIndex;
		else
			return m_chosenGoalIndex = crandomint(0, g_numWaypoints - 1);
	}
	else if (GetGameMode() == GameMode::Original)
	{
		if (g_mapType & MAP_CS)
		{
			if (m_team == Team::Counter)
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
					if (!g_waypoint->m_goalPoints.IsEmpty() && (g_timeRoundMid < engine->GetTime() || crandomint(1, 3) <= 2))
						return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
					else if (!g_waypoint->m_ctPoints.IsEmpty())
						return m_chosenGoalIndex = g_waypoint->m_ctPoints.GetRandomElement();
				}
			}
			else
			{
				if (!g_waypoint->m_rescuePoints.IsEmpty() && crandomint(1, 11) == 1)
					return m_chosenGoalIndex = g_waypoint->m_rescuePoints.GetRandomElement();
				else if (!g_waypoint->m_goalPoints.IsEmpty() && crandomint(1, 3) == 1)
					return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
				else if (!g_waypoint->m_terrorPoints.IsEmpty())
					return m_chosenGoalIndex = g_waypoint->m_terrorPoints.GetRandomElement();
			}
		}
		else if (g_mapType & MAP_AS)
		{
			if (m_team == Team::Counter)
			{
				if (m_isVIP && !g_waypoint->m_goalPoints.IsEmpty())
					return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
				else
				{
					if (!g_waypoint->m_goalPoints.IsEmpty() && crandomint(1, 2) == 1)
						return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
					else if (!g_waypoint->m_ctPoints.IsEmpty())
						return m_chosenGoalIndex = g_waypoint->m_ctPoints.GetRandomElement();
				}
			}
			else
			{
				if (!g_waypoint->m_goalPoints.IsEmpty() && crandomint(1, 11) == 1)
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
					SetProcess(Process::Escape, "escaping from the bomb", true, engine->GetTime() + GetBombTimeleft());
				else if (m_team == Team::Counter)
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
								SetProcess(Process::Camp, "i will protect my teammate", true, engine->GetTime() + GetBombTimeleft());
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
								SetProcess(Process::Camp, "i will defend the bomb", true, engine->GetTime() + GetBombTimeleft());
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
					if (m_team == Team::Counter)
					{
						const int index = FindDefendWaypoint(g_waypoint->GetPath(m_loosedBombWptIndex)->origin);
						if (IsValidWaypoint(index))
						{
							extern ConVar ebot_camp_max;
							m_campIndex = index;
							SetProcess(Process::Camp, "i will defend the dropped bomb", true, engine->GetTime() + ebot_camp_max.GetFloat());
							return m_chosenGoalIndex = index;
						}
					}
					else
						return m_chosenGoalIndex = m_loosedBombWptIndex;
				}
				else
				{
					if (m_team == Team::Counter)
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

	return m_chosenGoalIndex = crandomint(0, g_numWaypoints - 1);
}

bool Bot::GoalIsValid(void)
{
	if (!IsValidWaypoint(m_chosenGoalIndex))
		return false;

	if (m_chosenGoalIndex == m_currentWaypointIndex)
		return false;

	if (m_navNode.IsEmpty())
		return false;

	return true;
}

void Bot::MoveTo(const Vector targetPosition)
{
	const float inter = (m_frameInterval + g_pGlobals->frametime) * 0.5f;
	const Vector directionOld = (targetPosition + pev->velocity * -inter) - (pev->origin + pev->velocity * inter);
	const Vector directionNormal = directionOld.Normalize2D();
	SetStrafeSpeed(directionNormal, pev->maxspeed);
	m_moveAngles = directionOld.ToAngles();
	m_moveAngles.ClampAngles();
	m_moveAngles.x = -m_moveAngles.x; // invert for engine
	m_moveSpeed = pev->maxspeed;
}

void Bot::MoveOut(const Vector targetPosition)
{
	const float inter = (m_frameInterval + g_pGlobals->frametime) * 0.5f;
	const Vector directionOld = (targetPosition + pev->velocity * -inter) - (pev->origin + pev->velocity * inter);
	const Vector directionNormal = directionOld.Normalize2D();
	SetStrafeSpeed(directionNormal, pev->maxspeed);
	m_moveAngles = directionOld.ToAngles();
	m_moveAngles.ClampAngles();
	m_moveAngles.x = -m_moveAngles.x; // invert for engine
	m_moveSpeed = -pev->maxspeed;
}

void Bot::FollowPath(const Vector targetPosition)
{
	const int index = g_waypoint->FindNearestInCircle(targetPosition);
	if (IsValidWaypoint(index))
		FollowPath(index);
}

void Bot::FollowPath(const int targetIndex)
{
	m_strafeSpeed = 0.0f;

	if (!m_navNode.IsEmpty())
	{
		DoWaypointNav();

		const float inter = (m_frameInterval + g_pGlobals->frametime) * 0.5f;
		const Vector directionOld = (m_destOrigin + pev->velocity * -inter) - (pev->origin + pev->velocity * inter);
		m_moveAngles = directionOld.ToAngles();
		m_moveAngles.ClampAngles();
		m_moveAngles.x = -m_moveAngles.x; // invert for engine

		if ((pev->flags & FL_DUCKING))
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

// this function used after jump action to track real path
bool Bot::GetNextBestNode(void)
{
	const Path* pointer = g_waypoint->GetPath(m_currentWaypointIndex);
	if (pointer == nullptr)
		return false;

	const int nextWaypointIndex = m_navNode.Next();
	const int currentWaypointIndex = m_navNode.First();

	// check the links
	for (const auto& link : pointer->index)
	{
		// skip invalid links, or links that points to itself
		if (!IsValidWaypoint(link) || currentWaypointIndex == link)
			continue;

		// skip isn't connected links
		if (!g_waypoint->IsConnected(link, nextWaypointIndex) || !g_waypoint->IsConnected(link, m_currentWaypointIndex))
			continue;

		// don't use ladder waypoints as alternative
		if (g_waypoint->GetPath(link)->flags & WAYPOINT_LADDER)
			continue;

		// if not occupied, just set advance
		if (!IsWaypointOccupied(link))
		{
			m_navNode.First() = link;
			return true;
		}
	}

	return false;
}

// this function is a main path navigation
void Bot::DoWaypointNav(void)
{
	if (!IsValidWaypoint(m_currentWaypointIndex))
	{
		FindWaypoint();
		return;
	}

	const Path* currentWaypoint = g_waypoint->GetPath(m_currentWaypointIndex);
	if (currentWaypoint == nullptr)
		return;

	m_destOrigin = m_waypointOrigin;

	auto autoJump = [&](void)
	{
		// bot is not jumped yet?
		if (!m_jumpFinished)
		{
			// cheating for jump, bots cannot do some hard jumps and double jumps too
			// who cares about double jump for bots? :)
			pev->button |= (IN_DUCK | IN_JUMP);

			if (ebot_use_old_jump_method.GetBool())
			{
				pev->velocity.x = (m_destOrigin.x - (pev->origin.x + pev->velocity.x * (m_frameInterval * 2.0f))) * (2.0f + cabsf((pev->maxspeed * 0.004) - pev->gravity));
				pev->velocity.y = (m_destOrigin.y - (pev->origin.y + pev->velocity.y * (m_frameInterval * 2.0f))) * (2.0f + cabsf((pev->maxspeed * 0.004) - pev->gravity));

				// poor bots can't jump :(
				if (pev->gravity >= 0.88f)
					pev->velocity.z *= (cabsf(pev->gravity - 0.88f) + pev->gravity + m_frameInterval);
			}
			else
			{
				const Vector myOrigin = GetBottomOrigin(GetEntity());
				Vector waypointOrigin = m_destOrigin;

				if (currentWaypoint->flags & WAYPOINT_CROUCH)
					waypointOrigin.z -= 18.0f;
				else
					waypointOrigin.z -= 36.0f;

				const float timeToReachWaypoint = csqrtf(squaredf(waypointOrigin.x - myOrigin.x) + squaredf(waypointOrigin.y - myOrigin.y)) / pev->maxspeed;
				pev->velocity.x = (waypointOrigin.x - myOrigin.x) / timeToReachWaypoint;
				pev->velocity.y = (waypointOrigin.y - myOrigin.y) / timeToReachWaypoint;
			}
			
			// pause
			if (GetNextBestNode())
				SetProcess(Process::Jump, "entering the jump state", false, engine->GetTime() + 2.0f);

			m_jumpFinished = true;
		}
	};

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
		for (const auto &client : g_clients)
		{
			if (FNullEnt(client.ent) || !(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || (client.ent->v.movetype != MOVETYPE_FLY) || client.index == (GetIndex() - 1))
				continue;

			TraceResult tr{};
			bool foundGround = false;
			int previousNode = 0;

			// more than likely someone is already using our ladder...
			if ((client.ent->v.origin - currentWaypoint->origin).GetLengthSquared() < squaredf(48.0f))
			{
				extern ConVar ebot_ignore_enemies;
				if ((client.team != m_team || GetGameMode() == GameMode::Deathmatch || GetGameMode() == GameMode::NoTeam) && !ebot_ignore_enemies.GetBool())
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
							FindPath(m_prevWptIndex[0], previousNode);
							break;
						}
					}
				}
			}
		}
	}

	if (currentWaypoint->flags & WAYPOINT_CROUCH && (!(currentWaypoint->flags & WAYPOINT_CAMP) || m_stuckWarn > 1))
		m_duckTime = engine->GetTime() + 1.0f;

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
		edict_t* me = GetEntity();
		TraceResult tr{};
		TraceLine(pev->origin, m_waypointOrigin, ignore_monsters, me, &tr);

		if (!FNullEnt(tr.pHit) && FNullEnt(m_liftEntity) && cstrncmp(STRING(tr.pHit->v.classname), "func_door", 9) == 0)
		{
			// if the door is near enough...
			const Vector origin = GetEntityOrigin(tr.pHit);
			if ((pev->origin - origin).GetLengthSquared() < squaredf(54.0f))
			{
				ResetStuck(); // don't consider being stuck

				if (!(pev->oldbuttons & IN_USE && pev->button & IN_USE))
				{
					// do not use door directrly under xash, or we will get failed assert in gamedll code
					if (g_gameVersion & Game::Xash)
					{
						m_lookAt = origin;
						pev->button |= IN_USE;
					}
					else if (me != nullptr)
						MDLL_Use(tr.pHit, me); // also 'use' the door randomly
				}
			}

			edict_t* button = FindNearestButton(STRING(tr.pHit->v.targetname));

			// check if we got valid button
			if (!FNullEnt(button))
			{
				m_pickupItem = button;
				m_pickupType = PickupType::Button;
			}

			const float time = engine->GetTime();

			// if bot hits the door, then it opens, so wait a bit to let it open safely
			if (m_timeDoorOpen < time && pev->velocity.GetLengthSquared2D() < squaredf(pev->maxspeed * 0.20f))
			{
				m_timeDoorOpen = time + 3.0f;

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
						SetProcess(Process::Pause, "waiting for door open", false, time + 1.25f);
						return;
					}

					m_tryOpenDoor = 0;
				}
			}
		}
	}

	float distance;
	float desiredDistance;

	if (IsOnLadder() || currentWaypoint->flags & WAYPOINT_LADDER)
	{
		distance = (pev->origin - m_destOrigin).GetLengthSquared();
		desiredDistance = 24.0f;
	}
	else
	{
		distance = ((pev->origin + pev->velocity * m_frameInterval) - m_destOrigin).GetLengthSquared2D();

		if (m_currentTravelFlags & PATHFLAG_JUMP)
			desiredDistance = 0.0f;
		else
		{
			if (currentWaypoint->flags & WAYPOINT_LIFT)
				desiredDistance = 54.0f;
			else if ((pev->flags & FL_DUCKING) || currentWaypoint->flags & WAYPOINT_GOAL)
				desiredDistance = 24.0f;
			else if (currentWaypoint->radius > 4)
				desiredDistance = static_cast<float>(currentWaypoint->radius + m_stuckWarn);
			else
				desiredDistance = 4.0f;
		}
	}

	if (desiredDistance < 22.0f && distance < squaredf(30.0f))
	{
		const float inter = (m_frameInterval + g_pGlobals->frametime) * 0.5f;
		if (((m_destOrigin + pev->velocity * -inter) - (pev->origin + pev->velocity * inter)).GetLengthSquared() > distance)
			desiredDistance += distance;
	}

	if (distance < squaredf(desiredDistance))
	{
		m_currentTravelFlags = 0;
		m_navNode.Shift();

		if (!m_navNode.IsEmpty())
		{
			const int destIndex = m_navNode.First();

			ChangeWptIndex(destIndex);
			SetWaypointOrigin();

			int i;
			for (i = 0; i < Const_MaxPathIndex; i++)
			{
				if (currentWaypoint->index[i] == destIndex)
				{
					m_jumpFinished = false;
					m_currentTravelFlags = currentWaypoint->connectionFlags[i];

					if (m_currentTravelFlags & PATHFLAG_JUMP)
					{
						// jump directly to the waypoint, otherwise we will fall...
						m_waypointOrigin = g_waypoint->GetPath(destIndex)->origin;
						m_destOrigin = m_waypointOrigin;
						autoJump();
					}

					break;
				}
			}
		}
	}
}

bool Bot::UpdateLiftHandling(void)
{
	edict_t* me = GetEntity();
	const float time = engine->GetTime();
	bool liftClosedDoorExists = false;

	TraceResult tr{};

	// wait for something about for lift
	auto wait = [&]()
	{
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;
		pev->button &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
		ResetStuck();
	};

	// trace line to door
	TraceLine(pev->origin, m_waypointOrigin, true, true, me, &tr);

	if (tr.flFraction < 1.0f && (m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor || m_liftState == LiftState::LookingButtonOutside) && !FNullEnt(tr.pHit) && cstrcmp(STRING(tr.pHit->v.classname), "func_door") == 0 && pev->groundentity != tr.pHit)
	{
		if (m_liftState == LiftState::None)
		{
			m_liftState = LiftState::LookingButtonOutside;
			m_liftUsageTime = time + 7.0f;
		}

		liftClosedDoorExists = true;
	}

	const auto path = g_waypoint->GetPath(m_currentWaypointIndex);

	if (!m_navNode.IsEmpty())
	{
		// trace line down
		TraceLine(path->origin, m_waypointOrigin + Vector(0.0f, 0.0f, -54.0f), true, true, me, &tr);

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
					m_liftUsageTime = time + 5.0f;
				}
			}
			else if (m_liftState == LiftState::TravelingBy)
			{
				m_liftState = LiftState::Leaving;
				m_liftUsageTime = time + 7.0f;
			}
		}
		else
		{
			if ((m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor) && m_navNode.HasNext())
			{
				const int nextWaypoint = m_navNode.Next();
				if (IsValidWaypoint(nextWaypoint))
				{
					const Path* pointer = g_waypoint->GetPath(nextWaypoint);
					if (pointer->flags & WAYPOINT_LIFT)
					{
						TraceLine(path->origin, pointer->origin, true, true, me, &tr);
						if (!FNullEnt(tr.pHit) && (cstrcmp(STRING(tr.pHit->v.classname), "func_door") == 0 || cstrcmp(STRING(tr.pHit->v.classname), "func_plat") == 0 || cstrcmp(STRING(tr.pHit->v.classname), "func_train") == 0))
							m_liftEntity = tr.pHit;
					}
				}

				m_liftState = LiftState::LookingButtonOutside;
				m_liftUsageTime = time + 15.0f;
			}
		}
	}

	// bot is going to enter the lift
	if (m_liftState == LiftState::EnteringIn)
	{
		m_destOrigin = m_liftTravelPos;

		// check if we enough to destination
		if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f))
		{
			wait();

			// need to wait our following teammate ?
			bool needWaitForTeammate = false;

			// if some bot is following a bot going into lift - he should take the same lift to go
			for (const auto &bot : g_botManager->m_bots)
			{
				if (!bot || !bot->m_isAlive || bot->m_team != m_team)
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
				m_liftUsageTime = time + 8.0f;
			}
			else
			{
				m_liftState = LiftState::LookingButtonInside;
				m_liftUsageTime = time + 10.0f;
			}
		}
	}

	// bot is waiting for his teammates
	if (m_liftState == LiftState::WaitingForTeammates)
	{
		// need to wait our following teammate ?
		bool needWaitForTeammate = false;

		for (const auto &bot : g_botManager->m_bots)
		{
			if (!bot || !bot->m_isAlive || bot->m_team != m_team || bot->m_liftEntity != m_liftEntity)
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

			if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f))
				wait();
		}

		// else we need to look for button
		if (!needWaitForTeammate || m_liftUsageTime < time)
		{
			m_liftState = LiftState::LookingButtonInside;
			m_liftUsageTime = time + 10.0f;
		}
	}

	// bot is trying to find button inside a lift
	if (m_liftState == LiftState::LookingButtonInside)
	{
		edict_t* button = FindNearestButton(STRING(m_liftEntity->v.targetname));

		// got a valid button entity ?
		if (!FNullEnt(button) && pev->groundentity == m_liftEntity && m_buttonPushTime + 1.0f < time && m_liftEntity->v.velocity.z == 0.0f && IsOnFloor())
		{
			m_pickupItem = button;
			m_pickupType = PickupType::Button;

		}
	}

	// is lift activated and bot is standing on it and lift is moving ?
	if (m_liftState == LiftState::LookingButtonInside || m_liftState == LiftState::EnteringIn || m_liftState == LiftState::WaitingForTeammates || m_liftState == LiftState::WaitingFor)
	{
		if (pev->groundentity == m_liftEntity && m_liftEntity->v.velocity.z != 0.0f && IsOnFloor() && (g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LIFT))
		{
			m_liftState = LiftState::TravelingBy;
			m_liftUsageTime = time + 14.0f;

			if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f))
				wait();
		}
	}

	// bots is currently moving on lift
	if (m_liftState == LiftState::TravelingBy)
	{
		m_destOrigin = Vector(m_liftTravelPos.x, m_liftTravelPos.y, pev->origin.z);
		if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f))
			wait();
	}

	// need to find a button outside the lift
	if (m_liftState == LiftState::LookingButtonOutside)
	{
		// button has been pressed, lift should come
		if (m_buttonPushTime + 8.0f > time)
		{
			if (IsValidWaypoint(m_prevWptIndex[0]))
				m_destOrigin = g_waypoint->GetPath(m_prevWptIndex[0])->origin;
			else
				m_destOrigin = pev->origin;

			if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f))
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
				for (const auto &client : g_clients)
				{
					if (FNullEnt(client.ent) || !(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == me || FNullEnt(client.ent->v.groundentity))
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

					if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f))
						wait();
				}
				else
				{
					m_pickupItem = button;
					m_pickupType = PickupType::Button;
					m_liftState = LiftState::WaitingFor;
					m_liftUsageTime = time + 20.0f;
				}
			}
			else
			{
				m_liftState = LiftState::WaitingFor;
				m_liftUsageTime = time + 15.0f;
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

		if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f))
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
					FindPath(m_currentWaypointIndex, m_prevWptIndex[2]);

				return false;
			}
		}
	}

	return true;
}

bool Bot::UpdateLiftStates(void)
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

class PriorityQueue
{
private:
	struct Node
	{
		int id;
		float pri;
	};

	int m_allocCount;
	int m_size;
	int m_heapSize;
	Node* m_heap;

public:

	inline bool IsEmpty(void)
	{
		return !m_size;
	}

	inline int Size(void)
	{
		return m_size;
	}

	inline PriorityQueue(const int initialSize = g_numWaypoints + 32)
	{
		m_size = 0;
		m_heapSize = initialSize;
		m_allocCount = 0;
		m_heap = static_cast<Node*>(malloc(sizeof(Node) * m_heapSize));
	}

	inline ~PriorityQueue(void)
	{
		free(m_heap);
		m_heap = nullptr;
	}

	// inserts a value into the priority queue
	inline void Insert(const int value, const float pri)
	{
		if (m_allocCount > 20)
		{
			AddLogEntry(Log::Fatal, "Tried to re-allocate heap too many times in pathfinder. This usually indicates corrupted waypoint file. Please obtain new copy of waypoint.");
			return;
		}

		if (m_heap == nullptr)
			return;

		if (m_size >= m_heapSize)
		{
			m_allocCount++;
			m_heapSize += 100;

			Node* newHeap = static_cast<Node*>(realloc(m_heap, sizeof(Node) * m_heapSize));
			if (newHeap != nullptr)
				m_heap = newHeap;
		}

		m_heap[m_size].pri = pri;
		m_heap[m_size].id = value;

		int child = ++m_size - 1;
		while (child)
		{
			const int parent = (child - 1) * 0.5f;
			if (m_heap[parent].pri <= m_heap[child].pri)
				break;

			const Node ref = m_heap[child];

			m_heap[child] = m_heap[parent];
			m_heap[parent] = ref;

			child = parent;
		}
	}

	// removes the smallest item from the priority queue
	inline int Remove(void)
	{
		if (m_heap == nullptr)
			return -1;

		const int result = m_heap[0].id;

		m_size--;
		m_heap[0] = m_heap[m_size];

		int parent = 0;
		int child = (2 * parent) + 1;

		const Node ref = m_heap[parent];

		while (child < m_size)
		{
			const int right = (2 * parent) + 2;
			if (right < m_size && m_heap[right].pri < m_heap[child].pri)
				child = right;

			if (ref.pri <= m_heap[child].pri)
				break;

			m_heap[parent] = m_heap[child];

			parent = child;
			child = (2 * parent) + 1;
		}

		m_heap[parent] = ref;
		return result;
	}
};

inline const float GF_CostHuman(const int index, const int parent, const int team, const float gravity, const bool isZombie)
{
	const Path* path = g_waypoint->GetPath(index);
	if (path == nullptr)
		return 65355.0f;

	if (path->flags & WAYPOINT_AVOID)
		return 65355.0f;

	if (isZombie)
	{
		if (path->flags & WAYPOINT_HUMANONLY)
			return 65355.0f;
	}
	else
	{
		if (path->flags & WAYPOINT_ZOMBIEONLY)
			return 65355.0f;

		if (path->flags & WAYPOINT_DJUMP)
			return 65355.0f;
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
			if (distance < squaredi(path->radius + 64))
				return 65355.0f;
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
		if (distance < squaredi(path->radius + 128))
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
	if (path == nullptr)
		return 65355.0f;

	if (path->flags & WAYPOINT_AVOID)
		return 65355.0f;

	if (isZombie)
	{
		if (path->flags & WAYPOINT_HUMANONLY)
			return 65355.0f;
	}
	else
	{
		if (path->flags & WAYPOINT_ZOMBIEONLY)
			return 65355.0f;

		if (path->flags & WAYPOINT_DJUMP)
			return 65355.0f;
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
			if (distance < squaredi(path->radius + 64))
				return 65355.0f;
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

				if ((client.origin - path->origin).GetLengthSquared() < squaredi(512 + path->radius))
					count++;
				else if (IsVisible(path->origin, client.ent))
					count++;
			}

			// don't count me
			if (count <= 1)
				return 65355.0f;
			
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
	if (path == nullptr)
		return 65355.0f;

	if (path->flags & WAYPOINT_AVOID)
		return 65355.0f;

	if (isZombie)
	{
		if (path->flags & WAYPOINT_HUMANONLY)
			return 65355.0f;
	}
	else
	{
		if (path->flags & WAYPOINT_ZOMBIEONLY)
			return 65355.0f;

		if (path->flags & WAYPOINT_DJUMP)
			return 65355.0f;
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
			if (distance < squaredi(path->radius + 64))
				return 65355.0f;
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

				if ((client.origin - path->origin).GetLengthSquared() < squaredi(512 + path->radius))
					count++;
				else if (IsVisible(path->origin, client.ent))
					count++;
			}

			// don't count me
			if (count <= 1)
				return 65355.0f;
			
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
	if (path == nullptr)
		return 65355.0f;

	if (path->flags & WAYPOINT_AVOID)
		return 65355.0f;

	if (isZombie)
	{
		if (path->flags & WAYPOINT_HUMANONLY)
			return 65355.0f;
	}
	else
	{
		if (path->flags & WAYPOINT_ZOMBIEONLY)
			return 65355.0f;

		if (path->flags & WAYPOINT_DJUMP)
			return 65355.0f;
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
			if (distance < squaredi(path->radius + 64))
				return 65355.0f;
		}
	}

	// rusher bots never wait for boosting
	if (path->flags & WAYPOINT_DJUMP)
		return 65355.0f;

	const float baseCost = g_waypoint->GetPathDistance(index, parent);
	if (path->flags & WAYPOINT_CROUCH)
		return baseCost;

	return 1.0f;
}

inline const float GF_CostNoHostage(const int index, const int parent, const int team, const float gravity, const bool isZombie)
{
	const Path* path = g_waypoint->GetPath(parent);
	if (path == nullptr)
		return 65355.0f;

	if (path->flags & WAYPOINT_SPECIFICGRAVITY)
		return 65355.0f;

	if (path->flags & WAYPOINT_CROUCH)
		return 65355.0f;

	if (path->flags & WAYPOINT_LADDER)
		return 65355.0f;

	if (path->flags & WAYPOINT_AVOID)
		return 65355.0f;

	if (path->flags & WAYPOINT_WAITUNTIL)
		return 65355.0f;

	if (path->flags & WAYPOINT_JUMP)
		return 65355.0f;

	if (path->flags & WAYPOINT_DJUMP)
		return 65355.0f;

	int i;
	for (i = 0; i < Const_MaxPathIndex; i++)
	{
		const int neighbour = g_waypoint->GetPath(index)->index[i];
		if (IsValidWaypoint(neighbour) && (path->connectionFlags[neighbour] & PATHFLAG_JUMP || path->connectionFlags[neighbour] & PATHFLAG_DOUBLE))
			return 65355.0f;
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

	const float euclidean = csqrtf(squaredf(x) + squaredf(y) + squaredf(z));
	return 1000.0f * (cceilf(euclidean) - euclidean);
}

inline const float HF_Euclidean2D(const int start, const int goal)
{
	const Vector startOrigin = g_waypoint->GetPath(start)->origin;
	const Vector goalOrigin = g_waypoint->GetPath(goal)->origin;

	const float x = cabsf(startOrigin.x - goalOrigin.x);
	const float y = cabsf(startOrigin.y - goalOrigin.y);

	const float euclidean = csqrtf(squaredf(x) + squaredf(y));
	return 1000.0f * (cceilf(euclidean) - euclidean);
}

char* Bot::GetHeuristicName(void)
{
	if (m_2dH)
	{
		switch (m_heuristic)
		{
		case 1:
			return "squared Length 2D";
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
			return "squared Length";
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
void Bot::FindPath(int srcIndex, int destIndex, edict_t* enemy)
{
	if (g_pathTimer > engine->GetTime() && !m_navNode.IsEmpty())
		return;

	if (g_gameVersion & Game::HalfLife)
	{
		FindShortestPath(srcIndex, destIndex);
		return;
	}

	if (m_isBomber && !m_navNode.IsEmpty())
		return;

	// if we're stuck, find nearest waypoint
	if (!IsValidWaypoint(srcIndex))
	{
		const int index = FindWaypoint();
		if (!IsValidWaypoint(index))
			return;

		srcIndex = index;
	}

	if (!IsValidWaypoint(destIndex))
		destIndex = g_waypoint->m_otherPoints.GetRandomElement();

	if (srcIndex == destIndex)
	{
		DeleteSearchNodes();
		m_navNode.Add(destIndex);
		return;
	}

	const float (*gcalc) (const int, const int, const int, const float, const bool) = nullptr;
	const float (*hcalc) (const int, const int) = nullptr;

	if (IsZombieMode() && ebot_zombies_as_path_cost.GetBool() && !m_isZombieBot)
		gcalc = GF_CostHuman;
	else if (HasHostage())
		gcalc = GF_CostNoHostage;
	else if (m_isBomber || m_isVIP || (g_bombPlanted && m_inBombZone))
	{
		// move faster...
		if (g_timeRoundMid < engine->GetTime())
			gcalc = GF_CostRusher;
		else
			gcalc = GF_CostCareful;
	}
	else if (m_personality == Personality::Careful)
		gcalc = GF_CostCareful;
	else if (m_personality == Personality::Rusher)
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

	int i;
	int stuckIndex = -1;
	if (m_isStuck)
	{
		stuckIndex = g_waypoint->FindNearestInCircle(m_stuckArea, 256.0f);
		if (srcIndex == stuckIndex)
		{
			const Path* pointer = g_waypoint->GetPath(srcIndex);
			if (pointer)
			{
				for (i = 0; i < Const_MaxPathIndex; i++)
				{
					const int next = pointer->index[i];
					if (next == -1)
						continue;

					if (!IsWaypointOccupied(next))
					{
						srcIndex = next;
						break;
					}
				}
			}
		}
	}

	AStar_t waypoints[g_numWaypoints];
	for (i = 0; i < g_numWaypoints; i++)
	{
		waypoints[i].g = 0.0f;
		waypoints[i].f = 0.0f;
		waypoints[i].parent = -1;
		waypoints[i].state = State::New;
	}

	// put start node into open list
	const auto srcWaypoint = &waypoints[srcIndex];
	srcWaypoint->g = gcalc(srcIndex, -1, m_team, pev->gravity, m_isZombieBot);
	srcWaypoint->f = srcWaypoint->g;
	srcWaypoint->state = State::Open;

	PriorityQueue openList;
	openList.Insert(srcIndex, srcWaypoint->f);
	while (!openList.IsEmpty())
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

			do
			{
				m_navNode.Add(currentIndex);
				currentIndex = waypoints[currentIndex].parent;
			} while (IsValidWaypoint(currentIndex));

			m_navNode.Reverse();

			m_currentWaypointIndex = m_navNode.First();

			const Path* pointer = g_waypoint->GetPath(m_currentWaypointIndex);
			if (pointer != nullptr)
			{
				if (pointer->radius > 8 && ((pev->origin + pev->velocity * m_frameInterval) - pointer->origin).GetLengthSquared2D() < squaredi(pointer->radius))
					m_waypointOrigin = pev->origin + pev->velocity * (m_frameInterval + m_frameInterval);
				else
					m_waypointOrigin = pointer->origin;
			}

			m_destOrigin = m_waypointOrigin;
			m_jumpFinished = false;
			g_pathTimer = engine->GetTime() + 0.25f;

			return;
		}

		const auto currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->state != State::Open)
			continue;

		// put current node into Closed list
		currWaypoint->state = State::Closed;

		// now expand the current node
		const Path* pointer = g_waypoint->GetPath(currentIndex);
		if (!pointer)
			continue;

		for (i = 0; i < Const_MaxPathIndex; i++)
		{
			const int self = pointer->index[i];
			if (!IsValidWaypoint(self))
				continue;

			if (self != destIndex)
			{
				if (stuckIndex == self)
					continue;

				const int32 flags = g_waypoint->GetPath(self)->flags;
				if (flags & WAYPOINT_FALLCHECK)
				{
					TraceResult tr{};
					const Vector origin = g_waypoint->GetPath(self)->origin;
					TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), false, false, GetEntity(), &tr);
					if (tr.flFraction == 1.0f)
						continue;
				}
				else if (flags & WAYPOINT_SPECIFICGRAVITY)
				{
					if (pev->gravity * (1600.0f - engine->GetGravity()) < g_waypoint->GetPath(self)->gravity)
						continue;
				}
				else if (enemy != nullptr)
				{
					const Vector origin = g_waypoint->GetPath(self)->origin;
					if (::IsInViewCone(origin, enemy) && IsVisible(origin, enemy))
					{
						if ((GetEntityOrigin(enemy) - origin).GetLengthSquared() - (pev->origin - origin).GetLengthSquared() < 0.0f)
							continue;
					}
				}
			}

			const float g = currWaypoint->g + gcalc(currentIndex, self, m_team, pev->gravity, m_isZombieBot);
			const float f = g + hcalc(self, destIndex);
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
	for (i = 0; i < g_numWaypoints; i++)
	{
		if (waypoints[i].state == State::Closed)
			PossiblePath.Push(i);
	}
	
	if (!PossiblePath.IsEmpty())
	{
		const int index = PossiblePath.GetRandomElement();
		m_chosenGoalIndex = index;
		FindShortestPath(srcIndex, destIndex);
		return;
	}

	FindShortestPath(srcIndex, destIndex);
}

void Bot::FindShortestPath(int srcIndex, int destIndex)
{
	// if we're stuck, find nearest waypoint
	if (!IsValidWaypoint(srcIndex))
	{
		const int index = FindWaypoint();
		if (IsValidWaypoint(index))
			srcIndex = index;
	}

	if (!IsValidWaypoint(destIndex))
		destIndex = g_waypoint->m_otherPoints.GetRandomElement();

	if (srcIndex == destIndex)
	{
		DeleteSearchNodes();
		m_navNode.Add(destIndex);
		return;
	}

	const float (*hcalc) (const int, const int) = nullptr;

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

	int i;
	AStar_t waypoints[g_numWaypoints];
	for (i = 0; i < g_numWaypoints; i++)
	{
		waypoints[i].g = 0.0f;
		waypoints[i].f = 0.0f;
		waypoints[i].parent = -1;
		waypoints[i].state = State::New;
	}

	// put start node into open list
	const auto srcWaypoint = &waypoints[srcIndex];
	srcWaypoint->g = 1.0f;
	srcWaypoint->f = 1.0f;
	srcWaypoint->state = State::Open;

	PriorityQueue openList;
	openList.Insert(srcIndex, srcWaypoint->f);
	while (!openList.IsEmpty())
	{
		// remove the first node from the open list
		int currentIndex = openList.Remove();

		// is the current node the goal node?
		if (currentIndex == destIndex)
		{
			// delete path for new one
			DeleteSearchNodes();

			// set the chosen goal
			m_chosenGoalIndex = destIndex;

			do
			{
				m_navNode.Add(currentIndex);
				currentIndex = waypoints[currentIndex].parent;
			} while (IsValidWaypoint(currentIndex));

			m_navNode.Reverse();
			m_currentWaypointIndex = m_navNode.First();

			const Path* pointer = g_waypoint->GetPath(m_currentWaypointIndex);
			if (pointer != nullptr)
			{
				if (pointer->radius > 8 && ((pev->origin + pev->velocity * m_frameInterval) - pointer->origin).GetLengthSquared2D() < squaredi(pointer->radius))
					m_waypointOrigin = pev->origin + pev->velocity * (m_frameInterval + m_frameInterval);
				else
					m_waypointOrigin = pointer->origin;
			}

			m_destOrigin = m_waypointOrigin;
			m_jumpFinished = false;
			g_pathTimer = engine->GetTime() + 0.25f;

			return;
		}

		const auto currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->state != State::Open)
			continue;

		// put current node into Closed list
		currWaypoint->state = State::Closed;

		// now expand the current node
		const Path* pointer = g_waypoint->GetPath(currentIndex);
		if (!pointer)
			continue;
		
		for (i = 0; i < Const_MaxPathIndex; i++)
		{
			const int self = pointer->index[i];
			if (!IsValidWaypoint(self))
				continue;

			const float g = hcalc(currentIndex, self);
			const float f = hcalc(self, destIndex);
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
}

void Bot::DeleteSearchNodes(void)
{
	m_navNode.Clear();
}

void Bot::CheckTouchEntity(edict_t* entity)
{
	if (FNullEnt(entity))
		return;

	// if we won't be able to break it, don't try
	if (entity->v.takedamage == DAMAGE_NO)
	{
		// defuse bomb
		if (g_bombPlanted && cstrcmp(STRING(entity->v.model) + 9, "c4.mdl") == 0)
			SetProcess(Process::Defuse, "trying to defusing the bomb", false, engine->GetTime() + m_hasDefuser ? 6.0f : 12.0f);

		return;
	}

	// see if it's breakable
	if ((FClassnameIs(entity, "func_breakable") || (FClassnameIs(entity, "func_pushable") && (entity->v.spawnflags & SF_PUSH_BREAKABLE)) || FClassnameIs(entity, "func_wall")) && entity->v.health > 0.0f && entity->v.health < ebot_breakable_health_limit.GetFloat())
	{
		edict_t* me = GetEntity();
		if (me == nullptr)
			return;

		TraceResult tr{};
		TraceHull(EyePosition(), m_destOrigin, false, point_hull, me, &tr);

		TraceResult tr2{};
		TraceHull(pev->origin, m_destOrigin, false, head_hull, me, &tr2);

		// double check
		if ((!FNullEnt(tr.pHit) && tr.pHit == entity) || (!FNullEnt(tr2.pHit) && tr2.pHit == entity))
		{
			m_breakableEntity = entity;
			m_breakable = (!FNullEnt(tr.pHit) && tr.pHit == entity) ? tr.vecEndPos : ((GetEntityOrigin(entity) * 0.5f) + (tr2.vecEndPos * 0.5f));
			m_destOrigin = m_breakable;

			const float time = engine->GetTime();
			SetProcess(Process::DestroyBreakable, "trying to destroy a breakable", false, time + 20.0f);

			if (pev->origin.z > m_breakable.z) // make bots smarter
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

					if (enemy->m_currentWeapon == Weapon::Knife)
						continue;

					edict_t* ent = enemy->GetEntity();
					if (ent == nullptr)
						continue;

					TraceHull(enemy->EyePosition(), m_breakable, false, point_hull, ent, &tr);
					TraceHull(ent->v.origin, m_breakable, false, head_hull, ent, &tr2);

					if ((!FNullEnt(tr.pHit) && tr.pHit == entity) || (!FNullEnt(tr2.pHit) && tr2.pHit == entity))
					{
						enemy->m_breakableEntity = entity;
						enemy->m_breakable = (!FNullEnt(tr.pHit) && tr.pHit == entity) ? tr.vecEndPos : ((GetEntityOrigin(entity) * 0.5f) + (tr2.vecEndPos * 0.5f));
						enemy->SetProcess(Process::DestroyBreakable, "trying to destroy a breakable for my enemy fall", false, time + 20.0f);
					}
				}
			}
			else if (!m_isZombieBot) // tell my friends to destroy it
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

					edict_t* ent = bot->GetEntity();
					if (ent == nullptr)
						continue;

					if (me == ent)
						continue;

					TraceHull(bot->EyePosition(), m_breakable, false, point_hull, ent, &tr);
					TraceHull(ent->v.origin, m_breakable, false, head_hull, ent, &tr2);

					if ((!FNullEnt(tr.pHit) && tr.pHit == entity) || (!FNullEnt(tr2.pHit) && tr2.pHit == entity))
					{
						bot->m_breakableEntity = entity;
						bot->m_breakable = (!FNullEnt(tr.pHit) && tr.pHit == entity) ? tr.vecEndPos : ((GetEntityOrigin(entity) * 0.5f) + (tr2.vecEndPos * 0.5f));

						if (bot->m_currentWeapon == Weapon::Knife)
							bot->m_destOrigin = bot->m_breakable;

						bot->SetProcess(Process::DestroyBreakable, "trying to help my friend for destroy a breakable", false, time + 20.0f);
					}
				}
			}
		}
	}
}

// this function find a node in the near of the bot if bot had lost his path of pathfinder needs to be restarted over again
int Bot::FindWaypoint(void)
{
	int busy = -1;
	float lessDist[3] = {65355.0f, 65355.0f, 65355.0f};
	int lessIndex[3] = {-1, -1, -1};

	int at;
	for (at = 0; at < g_numWaypoints; at++)
	{
		if (!IsValidWaypoint(at))
			continue;
		
		const Path* pointer = g_waypoint->GetPath(at);
		if (pointer == nullptr)
			continue;

		if (m_team == Team::Counter && pointer->flags & WAYPOINT_ZOMBIEONLY)
			continue;
		else if (m_team == Team::Terrorist && pointer->flags & WAYPOINT_HUMANONLY)
			continue;

		bool skip = !!(int(pointer->index) == m_currentWaypointIndex);

		// skip current and recent previous nodes
		if (at == m_prevWptIndex[0] || at == m_prevWptIndex[1] || at == m_prevWptIndex[2])
			skip = true;

		// skip the current node, if any
		if (skip)
			continue;

		// skip if isn't visible
		if (!IsVisible(pointer->origin, GetEntity()))
			continue;

		// cts with hostages should not pick
		if (m_team == Team::Counter && HasHostage())
			continue;

		// check we're have link to it
		if (IsValidWaypoint(m_currentWaypointIndex) && !g_waypoint->IsConnected(m_currentWaypointIndex, at))
			continue;

		// ignore non-reacheable nodes...
		if (!g_waypoint->IsNodeReachable(g_waypoint->GetPath(m_currentWaypointIndex)->origin, pointer->origin))
			continue;

		// check if node is already used by another bot...
		if (IsWaypointOccupied(at))
		{
			busy = at;
			continue;
		}

		// if we're still here, find some close nodes
		const float distance = (pev->origin - pointer->origin).GetLengthSquared();
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
		index = crandomint(0, 2);
	else if (IsValidWaypoint(lessIndex[1]))
		index = crandomint(0, 1);
	else if (IsValidWaypoint(lessIndex[0])) 
		index = 0;

	selected = lessIndex[index];

	// if we're still have no node and have busy one (by other bot) pick it up
	if (!IsValidWaypoint(selected) && IsValidWaypoint(busy)) 
		selected = busy;

	// worst case... find atleast something
	if (!IsValidWaypoint(selected))
		selected = g_waypoint->FindNearest(pev->origin + pev->velocity, 9999.0f, -1, GetEntity());

	// still not? what a bad waypointer...
	if (!IsValidWaypoint(selected))
		selected = g_waypoint->FindNearestInCircle(pev->origin + pev->velocity);

	ChangeWptIndex(selected);
	SetWaypointOrigin();
	return selected;
}

void Bot::ResetStuck(void)
{
	m_isStuck = false;
	m_stuckWarn = 0;
	m_stuckTimer = engine->GetTime() + 1.28f;
	m_jumpFinished = false;
}

void Bot::CheckStuck(const float maxSpeed)
{
	if (!ebot_has_semiclip.GetBool() && m_hasFriendsNear && !(m_waypointFlags & WAYPOINT_FALLRISK) && !FNullEnt(m_nearestFriend))
	{
		const Vector myOrigin = pev->origin + pev->velocity * m_frameInterval;
		const Vector friendOrigin = m_friendOrigin + m_nearestFriend->v.velocity * m_frameInterval;
		if ((myOrigin - friendOrigin).GetLengthSquared() < squaredf((m_nearestFriend->v.maxspeed + pev->maxspeed) * 0.17f))
		{
			if (!m_isSlowThink && !IsOnLadder() && m_navNode.HasNext() && IsWaypointOccupied(m_navNode.First()))
			{
				int C;
				for (C = 0; C < Const_MaxPathIndex; C++)
				{
					const int index = g_waypoint->GetPath(m_navNode.First())->index[C];
					if (IsValidWaypoint(index) && index != m_navNode.Next() && g_waypoint->IsConnected(index, m_navNode.Next()) && !IsWaypointOccupied(index))
					{
						if (g_waypoint->Reachable(GetEntity(), index) && !IsDeadlyDrop(g_waypoint->GetPath(index)->origin))
						{
							m_navNode.First() = index;
							ChangeWptIndex(index);
							SetWaypointOrigin();
						}
						else if (GetCurrentState() == Process::Default && m_hasFriendsNear && !FNullEnt(m_nearestFriend))
						{
							const Vector origin = g_waypoint->GetPath(m_currentWaypointIndex)->origin;
							if ((pev->origin - origin).GetLengthSquared() > (m_nearestFriend->v.origin - origin).GetLengthSquared())
							{
								Bot* bot = g_botManager->GetBot(m_nearestFriend);
								if (bot != nullptr)
								{
									if (bot->GetCurrentState() == Process::Default && !bot->m_navNode.IsEmpty())
									{
										m_navNode.First() = bot->m_navNode.First();
										ChangeWptIndex(bot->m_navNode.First());
										SetWaypointOrigin();
									}
								}
								else if (m_nearestFriend->v.speed > (m_nearestFriend->v.maxspeed * 0.25))
								{
									const int index = g_waypoint->FindNearest(m_nearestFriend->v.origin + m_nearestFriend->v.velocity, 99999999.0f, -1, GetEntity());
									if (IsValidWaypoint(index))
									{
										m_navNode.First() = index;
										ChangeWptIndex(index);
										SetWaypointOrigin();
									}
								}
								else
									SetProcess(Process::Pause, "waiting for my friend", true, engine->GetTime() + cminf((g_waypoint->GetPath(m_navNode.First())->origin - pev->origin).GetLength() / pev->maxspeed, 6.0f));
							}
						}
					}
				}
			}

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
			if (cabsf(m_nearestFriend->v.speed) > 54.0f && ((moveBack && m_stuckWarn > 5) || m_stuckWarn > 9))
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

			if (IsOnFloor() && m_stuckWarn > 7)
			{
				if (!(m_nearestFriend->v.button & IN_JUMP) && !(m_nearestFriend->v.oldbuttons & IN_JUMP) && ((m_nearestFriend->v.button & IN_DUCK && m_nearestFriend->v.oldbuttons & IN_DUCK) || CanJumpUp(pev->velocity.SkipZ()) || CanJumpUp(dir)))
				{
					if (m_jumpTime + 1.0f < engine->GetTime())
						pev->button |= IN_JUMP;
				}
				else if (!(m_nearestFriend->v.button & IN_DUCK) && !(m_nearestFriend->v.oldbuttons & IN_DUCK))
				{
					const float time = engine->GetTime();
					if (m_duckTime < time)
						m_duckTime = time + crandomfloat(1.0f, 2.0f);
				}
			}

			if (doorStuck && m_stuckWarn > 11) // ENOUGH!
			{
				extern ConVar ebot_ignore_enemies;
				if (!ebot_ignore_enemies.GetBool())
				{
					const bool friendlyFire = engine->IsFriendlyFireOn();
					if ((!friendlyFire && m_currentWeapon == Weapon::Knife) || (friendlyFire && !IsValidBot(m_nearestFriend))) // DOOR STUCK! || DIE HUMAN!
					{
						m_lookAt = friendOrigin + (m_nearestFriend->v.view_ofs * 0.9f);
						m_pauseTime = 0.0f;
						if (!(pev->button & IN_ATTACK) && !(pev->oldbuttons & IN_ATTACK))
							pev->button |= IN_ATTACK;

						if (!IsZombieMode())
						{
							// WHO SAID BOT'S DON'T HAVE A FEELINGS?
							// then show it, or act as...
							if (m_personality == Personality::Careful)
							{
								if (friendlyFire)
									ChatSay(false, "YOU'RE NOT ONE OF US!");
								else
									ChatSay(false, "I'M STUCK!");
							}
							else if (m_personality == Personality::Rusher)
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
					}
					else
						SelectKnife();
				}
			}
		}
	}
	else if (m_stuckWarn > 3 && !(m_waypointFlags & WAYPOINT_FALLRISK))
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
	{
		const float time = engine->GetTime();
		if (m_stuckTimer < time)
		{
			m_stuckArea = (pev->origin + pev->velocity * -m_frameInterval);
			m_stuckTimer = time + 1.28f;
		}

		return;
	}
	else if (m_stuckWarn > 5 && !g_waypoint->Reachable(GetEntity(), m_currentWaypointIndex))
	{
		DeleteSearchNodes();
		FindWaypoint();
	}

	const float distance = ((pev->origin + pev->velocity * m_frameInterval) - (m_stuckArea + pev->velocity * -m_frameInterval)).GetLengthSquared2D();
	const float range = maxSpeed * 2.0f;
	if (distance < range)
	{
		m_stuckWarn++;

		if (m_stuckWarn > 20)
		{
			m_stuckWarn = 10;
			DeleteSearchNodes();
			FindWaypoint();
		}

		if (m_stuckWarn > 10)
			m_isStuck = true;
		else if (m_stuckWarn == 10)
			pev->button |= IN_JUMP;
	}
	else
	{
		const float rn = range * 4.0f;
		if (distance > rn)
		{
			if (m_stuckWarn > 0)
			{
				if (m_stuckWarn < 5)
					m_isStuck = false;

				m_stuckWarn -= 1;
			}

			// are we teleported? O_O
			if (distance > ((rn * 4.0f) - m_friendsNearCount) && !IsVisible(m_destOrigin, GetEntity()))
			{
				// try to get new waypoint
				if (!IsValidWaypoint(m_currentWaypointIndex) || !GetNextBestNode())
				{
					DeleteSearchNodes();
					FindWaypoint();
				}
			}
		}
	}
}

void Bot::SetWaypointOrigin(void)
{
	const Path* pointer = g_waypoint->GetPath(m_currentWaypointIndex);
	const int32 myFlags = pointer->flags;
	m_waypointOrigin = pointer->origin;

	if (pointer->radius > 4)
	{
		const float radius = static_cast<float>(pointer->radius);
		MakeVectors(Vector(pev->angles.x, AngleNormalize(pev->angles.y + crandomfloat(-90.0f, 90.0f)), 0.0f));
		int sPoint = -1;

		if (m_navNode.HasNext())
		{
			Vector waypointOrigin[5];
			for (int i = 0; i < 5; i++)
			{
				waypointOrigin[i] = m_waypointOrigin;
				waypointOrigin[i] += Vector(crandomfloat(-radius, radius), crandomfloat(-radius, radius), 0.0f);
			}

			float sDistance = 65355.0f;
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

void Bot::ChangeWptIndex(const int waypointIndex)
{
	m_currentWaypointIndex = waypointIndex;

	if (m_prevWptIndex[0] != m_currentWaypointIndex)
	{
		m_prevWptIndex[0] = m_currentWaypointIndex;
		m_prevWptIndex[1] = m_prevWptIndex[0];
		m_prevWptIndex[2] = m_prevWptIndex[1];
		m_prevWptIndex[3] = m_prevWptIndex[2];
	}

	m_waypointFlags = g_waypoint->GetPath(m_currentWaypointIndex)->flags;
	m_jumpFinished = false;
}

int Bot::FindDefendWaypoint(const Vector& origin)
{
	// no camp waypoints
	if (g_waypoint->m_campPoints.IsEmpty())
		return crandomint(0, g_numWaypoints - 1);

	Array <int> BestSpots;
	Array <int> OkSpots;
	Array <int> WorstSpots;

	int index = -1;
	TraceResult tr{};
	for (int i = 0; i < g_waypoint->m_campPoints.GetElementNumber(); i++)
	{
		g_waypoint->m_campPoints.GetAt(i, index);

		if (!IsValidWaypoint(index))
			continue;

		const Path* pointer = g_waypoint->GetPath(index);
		if (pointer->flags & WAYPOINT_LADDER)
			continue;

		if (GetGameMode() == GameMode::Original)
		{
			if (m_team == Team::Counter && pointer->flags & WAYPOINT_TERRORIST)
				continue;
			else if (pointer->flags & WAYPOINT_COUNTER)
				continue;
		}

		if (!IsWaypointOccupied(index))
		{
			TraceLine(pointer->origin, origin, true, true, pev->pContainingEntity, &tr);

			if (tr.flFraction == 1.0f) // distance isn't matter
				BestSpots.Push(index);
			else if ((pointer->origin - origin).GetLengthSquared() < squaredf(1024.0f))
				OkSpots.Push(index);
			else
				WorstSpots.Push(index);
		}
	}

	int BestIndex = -1;
	if (!BestSpots.IsEmpty())
		BestIndex = BestSpots.GetRandomElement();
	else if (!OkSpots.IsEmpty())
		BestIndex = OkSpots.GetRandomElement();
	else if (!WorstSpots.IsEmpty())
		BestIndex = WorstSpots.GetRandomElement();
	
	if (IsValidWaypoint(BestIndex))
		return BestIndex;

	return g_waypoint->m_campPoints.GetRandomElement();
}

// checks if bot is blocked in his movement direction (excluding doors)
bool Bot::CantMoveForward(const Vector normal)
{
	// first do a trace from the bot's eyes forward...
	const Vector eyePosition = EyePosition();
	Vector src = eyePosition;
	Vector forward = src + normal * 24.0f;

	MakeVectors(Vector(0.0f, pev->angles.y, 0.0f));

	TraceResult tr{};

	// trace from the bot's eyes straight forward...
	TraceLine(src, forward, true, pev->pContainingEntity, &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f)
	{
		if (cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) == 0)
			return false;

		return true;  // bot's head will hit something
	}

	// bot's head is clear, check at shoulder level...
	// trace from the bot's shoulder left diagonal forward to the right shoulder...
	src = eyePosition + Vector(0.0f, 0.0f, -16.0f) - g_pGlobals->v_right * 16.0f;
	forward = eyePosition + Vector(0.0f, 0.0f, -16.0f) + g_pGlobals->v_right * 16.0f + normal * 24.0f;

	TraceLine(src, forward, true, pev->pContainingEntity, &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) != 0)
		return true;  // bot's body will hit something

	 // bot's head is clear, check at shoulder level...
	 // trace from the bot's shoulder right diagonal forward to the left shoulder...
	src = eyePosition + Vector(0.0f, 0.0f, -16.0f) + g_pGlobals->v_right * 16.0f;
	forward = eyePosition + Vector(0.0f, 0.0f, -16.0f) - g_pGlobals->v_right * 16.0f + normal * 24.0f;

	TraceLine(src, forward, true, pev->pContainingEntity, &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) != 0)
		return true;  // bot's body will hit something

	 // now check below waist
	if ((pev->flags & FL_DUCKING))
	{
		src = pev->origin + Vector(0.0f, 0.0f, -19.0f + 19.0f);
		forward = src + Vector(0.0f, 0.0f, 10.0f) + normal * 24.0f;

		TraceLine(src, forward, true, pev->pContainingEntity, &tr);

		// check if the trace hit something...
		if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) != 0)
			return true;  // bot's body will hit something

		src = pev->origin;
		forward = src + normal * 24.0f;

		TraceLine(src, forward, true, pev->pContainingEntity, &tr);

		// check if the trace hit something...
		if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) != 0)
			return true;  // bot's body will hit something
	}
	else
	{
		// trace from the left waist to the right forward waist pos
		src = pev->origin + Vector(0.0f, 0.0f, -17.0f) - g_pGlobals->v_right * 16.0f;
		forward = pev->origin + Vector(0.0f, 0.0f, -17.0f) + g_pGlobals->v_right * 16.0f + normal * 24.0f;

		// trace from the bot's waist straight forward...
		TraceLine(src, forward, true, pev->pContainingEntity, &tr);

		// check if the trace hit something...
		if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) != 0)
			return true;  // bot's body will hit something

		 // trace from the left waist to the right forward waist pos
		src = pev->origin + Vector(0.0f, 0.0f, -17.0f) + g_pGlobals->v_right * 16.0f;
		forward = pev->origin + Vector(0.0f, 0.0f, -17.0f) - g_pGlobals->v_right * 16.0f + normal * 24.0f;

		TraceLine(src, forward, true, pev->pContainingEntity, &tr);

		// check if the trace hit something...
		if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) != 0)
			return true;  // bot's body will hit something
	}

	return false;  // bot can move forward, return false
}

bool Bot::CanJumpUp(const Vector normal)
{
	// this function check if bot can jump over some obstacle
	TraceResult tr{};

	// can't jump if not on ground and not on ladder/swimming
	if (!IsOnFloor() && (IsOnLadder() || !IsInWater()))
		return false;

	MakeVectors(Vector(0.0f, pev->angles.y, 0.0f));

	// check for normal jump height first...
	Vector src = pev->origin + Vector(0.0f, 0.0f, -36.0f + 45.0f);
	Vector dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, true, pev->pContainingEntity, &tr);

	if (tr.flFraction < 1.0f)
		goto CheckDuckJump;
	else
	{
		// now trace from jump height upward to check for obstructions...
		src = dest;
		dest.z = dest.z + 37.0f;

		TraceLine(src, dest, true, pev->pContainingEntity, &tr);

		if (tr.flFraction < 1.0f)
			return false;
	}

	// now check same height to one side of the bot...
	src = pev->origin + g_pGlobals->v_right * 16.0f + Vector(0.0f, 0.0f, -36.0f + 45.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, true, pev->pContainingEntity, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		goto CheckDuckJump;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, true, pev->pContainingEntity, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now check same height on the other side of the bot...
	src = pev->origin + (-g_pGlobals->v_right * 16.0f) + Vector(0.0f, 0.0f, -36.0f + 45.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, true, pev->pContainingEntity, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		goto CheckDuckJump;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, true, pev->pContainingEntity, &tr);

	// if trace hit something, return false
	return tr.flFraction > 1.0f;

	// here we check if a duck jump would work...
CheckDuckJump:
	// use center of the body first... maximum duck jump height is 62, so check one unit above that (63)
	src = pev->origin + Vector(0.0f, 0.0f, -36.0f + 63.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, true, pev->pContainingEntity, &tr);

	if (tr.flFraction < 1.0f)
		return false;
	else
	{
		// now trace from jump height upward to check for obstructions...
		src = dest;
		dest.z = dest.z + 37.0f;

		TraceLine(src, dest, true, pev->pContainingEntity, &tr);

		// if trace hit something, check duckjump
		if (tr.flFraction < 1.0f)
			return false;
	}

	// now check same height to one side of the bot...
	src = pev->origin + g_pGlobals->v_right * 16.0f + Vector(0.0f, 0.0f, -36.0f + 63.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, true, pev->pContainingEntity, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, true, pev->pContainingEntity, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now check same height on the other side of the bot...
	src = pev->origin + (-g_pGlobals->v_right * 16.0f) + Vector(0.0f, 0.0f, -36.0f + 63.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, true, pev->pContainingEntity, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, true, pev->pContainingEntity, &tr);

	// if trace hit something, return false
	return tr.flFraction > 1.0f;
}

bool Bot::CheckWallOnForward(void)
{
	TraceResult tr{};
	MakeVectors(pev->angles);

	// do a trace to the forward...
	TraceLine(pev->origin, pev->origin + g_pGlobals->v_forward * 54.0f, false, false, pev->pContainingEntity, &tr);

	// check if the trace hit something...
	if (tr.flFraction != 1.0f)
	{
		m_lastWallOrigin = tr.vecEndPos;
		return true;
	}
	else
	{
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, false, false, pev->pContainingEntity, &tr);

		// we don't want fall
		if (tr.flFraction == 1.0f)
		{
			m_lastWallOrigin = pev->origin;
			return true;
		}
	}

	return false;
}

bool Bot::CheckWallOnBehind(void)
{
	TraceResult tr{};
	MakeVectors(pev->angles);

	// do a trace to the behind...
	TraceLine(pev->origin, pev->origin - g_pGlobals->v_forward * 54.0f, false, false, pev->pContainingEntity, &tr);

	// check if the trace hit something...
	if (tr.flFraction != 1.0f)
	{
		m_lastWallOrigin = tr.vecEndPos;
		return true;
	}
	else
	{
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, false, false, pev->pContainingEntity, &tr);

		// we don't want fall
		if (tr.flFraction == 1.0f)
		{
			m_lastWallOrigin = pev->origin;
			return true;
		}
	}

	return false;
}

bool Bot::CheckWallOnLeft(void)
{
	TraceResult tr{};
	MakeVectors(pev->angles);

	// do a trace to the left...
	TraceLine(pev->origin, pev->origin - g_pGlobals->v_right * 54.0f, false, false, pev->pContainingEntity, &tr);

	// check if the trace hit something...
	if (tr.flFraction != 1.0f)
	{
		m_lastWallOrigin = tr.vecEndPos;
		return true;
	}
	else
	{
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, false, false, pev->pContainingEntity, &tr);

		// we don't want fall
		if (tr.flFraction == 1.0f)
		{
			m_lastWallOrigin = pev->origin;
			return true;
		}
	}

	return false;
}

bool Bot::CheckWallOnRight(void)
{
	TraceResult tr{};
	MakeVectors(pev->angles);

	// do a trace to the right...
	TraceLine(pev->origin, pev->origin + g_pGlobals->v_right * 54.0f, false, false, pev->pContainingEntity, &tr);

	// check if the trace hit something...
	if (tr.flFraction != 1.0f)
	{
		m_lastWallOrigin = tr.vecEndPos;
		return true;
	}
	else
	{
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, false, false, pev->pContainingEntity, &tr);

		// we don't want fall
		if (tr.flFraction == 1.0f)
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
	TraceResult tr{};

	const Vector move((targetOriginPos - botPos).ToYaw(), 0.0f, 0.0f);
	MakeVectors(move);

	const Vector direction = (targetOriginPos - botPos).Normalize();  // 1 unit long
	Vector check = botPos;
	Vector down = botPos;

	down.z = down.z - 1000.0f;  // straight down 1000 units

	TraceHull(check, down, true, head_hull, pev->pContainingEntity, &tr);

	if (tr.flFraction > 0.036f) // We're not on ground anymore?
		tr.flFraction = 0.036f;

	float height;
	float lastHeight = tr.flFraction * 1000.0f;  // height from ground

	float distance = (targetOriginPos - check).GetLengthSquared();  // distance from goal

	while (distance > squaredf(16.0f))
	{
		check = check + direction * 16.0f; // move 16 units closer to the goal...

		down = check;
		down.z = down.z - 1000.0f;  // straight down 1000 units

		TraceHull(check, down, true, head_hull, pev->pContainingEntity, &tr);

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

void Bot::FacePosition(void)
{
	// no spinbot please...
	if (m_lastAimTime < engine->GetTime() - 1.0f)
		m_lastAimTime = engine->GetTime();

	m_aimInterval = engine->GetTime() - m_lastAimTime;
	m_lastAimTime = engine->GetTime();

	// adjust all body and view angles to face an absolute vector
	Vector direction = (m_lookAt - EyePosition()).ToAngles() + pev->punchangle;
	direction.x = -direction.x; // invert for engine

	float accelerate = float(m_skill) * 40.0f;
	float stiffness = float(m_skill) * 4.0f;
	float damping = float(m_skill) * 0.4f;

	Vector newAngles = pev->v_angle;

	const float angleDiffPitch = AngleNormalize(direction.x - newAngles.x);
	const float angleDiffYaw = AngleNormalize(direction.y - newAngles.y);
	const float lockn = 0.111f / m_aimInterval;

	if (angleDiffYaw < lockn && angleDiffYaw > -lockn)
	{
		m_lookYawVel = 0.0f;
		newAngles.y = direction.y;
	}
	else
	{
		const float accel = cclampf((stiffness * angleDiffYaw) - (damping * m_lookYawVel), -accelerate, accelerate);
		m_lookYawVel += m_aimInterval * accel;
		newAngles.y += m_aimInterval * m_lookYawVel;
	}

	if (angleDiffPitch < lockn && angleDiffPitch > -lockn)
	{
		m_lookPitchVel = 0.0f;
		newAngles.x = direction.x;
	}
	else
	{
		const float accel = cclampf(2.0f * stiffness * angleDiffPitch - (damping * m_lookPitchVel), -accelerate, accelerate);
		m_lookPitchVel += m_aimInterval * accel;
		newAngles.x += m_aimInterval * m_lookPitchVel;
	}

	if (newAngles.x < -89.0f)
		newAngles.x = -89.0f;
	else if (newAngles.x > 89.0f)
		newAngles.x = 89.0f;

	pev->v_angle = newAngles;

	// set the body angles to point the gun correctly
	pev->angles.x = -pev->v_angle.x * 0.33333333333f;
	pev->angles.y = pev->v_angle.y;
}

void Bot::LookAt(const Vector origin)
{
	m_lookAt = origin;
	FacePosition();
}

void Bot::SetStrafeSpeed(const Vector moveDir, const float strafeSpeed)
{
	MakeVectors(pev->angles);

	const Vector los = (moveDir - pev->origin).Normalize2D();
	const float dot = los | g_pGlobals->v_forward.SkipZ();

	if (CheckWallOnBehind())
	{
		if (CheckWallOnRight())
			m_tempstrafeSpeed = -strafeSpeed;
		else if (CheckWallOnLeft())
			m_tempstrafeSpeed = strafeSpeed;
		
		m_strafeSpeed = m_tempstrafeSpeed;

		if ((m_isStuck || pev->speed >= pev->maxspeed) && IsOnFloor() && !IsOnLadder() && m_jumpTime + 5.0f < engine->GetTime() && CanJumpUp(moveDir))
			pev->button |= IN_JUMP;
	}
	else if (dot > 0.0f && !CheckWallOnRight())
		m_strafeSpeed = strafeSpeed;
	else if (!CheckWallOnLeft())
		m_strafeSpeed = -strafeSpeed;
}

void Bot::SetStrafeSpeedNoCost(const Vector moveDir, const float strafeSpeed)
{
	MakeVectors(pev->angles);

	const Vector los = (moveDir - pev->origin).Normalize2D();
	const float dot = los | g_pGlobals->v_forward.SkipZ();
	
	if (dot > 0.0f)
		m_strafeSpeed = strafeSpeed;
	else
		m_strafeSpeed = -strafeSpeed;
}

// find hostage improve
int Bot::FindHostage(void)
{
	if (m_team != Team::Counter || !(g_mapType & MAP_CS) || m_isZombieBot)
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
		int nearestIndex = g_waypoint->FindNearest(entOrigin, 512.0f, -1, ent);

		if (IsValidWaypoint(nearestIndex) && canF)
			return nearestIndex;
		else
		{
			// do we need second try?
			nearestIndex = g_waypoint->FindNearestInCircle(entOrigin);
			if (IsValidWaypoint(nearestIndex) && canF)
				return nearestIndex;
		}
	}

	return -1;
}

int Bot::FindLoosedBomb(void)
{
	if (GetGameMode() != GameMode::Original || !(g_mapType & MAP_DE))
		return -1;

	edict_t* bombEntity = nullptr; // temporaly pointer to bomb

	// search the bomb on the map
	while (!FNullEnt(bombEntity = FIND_ENTITY_BY_CLASSNAME(bombEntity, "weaponbox")))
	{
		if (cstrcmp(STRING(bombEntity->v.model) + 9, "backpack.mdl") == 0)
		{
			const Vector bombOrigin = GetEntityOrigin(bombEntity);
			int nearestIndex = g_waypoint->FindNearest(bombOrigin, 512.0f, -1, bombEntity);
			if (IsValidWaypoint(nearestIndex))
				return nearestIndex;
			else
			{
				// do we need second try?
				nearestIndex = g_waypoint->FindNearestInCircle(bombOrigin);
				if (IsValidWaypoint(nearestIndex))
					return nearestIndex;
			}

			break;
		}
	}

	return -1;
}

bool Bot::IsWaypointOccupied(const int index)
{
	if (pev->solid == SOLID_NOT)
		return false;

	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (FNullEnt(client.ent))
			continue;

		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == GetEntity())
			continue;

		auto bot = g_botManager->GetBot(client.index);
		if (bot != nullptr)
		{
			if (bot->m_chosenGoalIndex == index)
				return true;

			if (bot->m_currentWaypointIndex == index)
				return true;

			if (bot->m_prevWptIndex[0] == index)
				return true;
		}
		else
		{
			const Path* pointer = g_waypoint->GetPath(index);
			if (pointer && ((client.ent->v.origin + client.ent->v.velocity * m_frameInterval) - pointer->origin).GetLengthSquared() < squaredi(pointer->radius + 54))
				return true;
		}
	}

	return false;
}

// this function tries to find nearest to current bot button, and returns pointer to
// it's entity, also here must be specified the target, that button must open
edict_t* Bot::FindNearestButton(const char* className)
{
	if (IsNullString(className))
		return nullptr;

	float nearestDistance = 65355.0f;
	edict_t* searchEntity = nullptr;
	edict_t* foundEntity = nullptr;

	// find the nearest button which can open our target
	while (!FNullEnt(searchEntity = FIND_ENTITY_BY_TARGET(searchEntity, className)))
	{
		const float distance = (pev->origin - GetEntityOrigin(searchEntity)).GetLengthSquared();
		if (distance < nearestDistance)
		{
			nearestDistance = distance;
			foundEntity = searchEntity;
		}
	}

	return foundEntity;
}