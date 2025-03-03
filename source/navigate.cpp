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

#include "../include/core.h"
#include <cstring>

ConVar ebot_zombies_as_path_cost("ebot_zombie_count_as_path_cost", "1");
ConVar ebot_has_semiclip("ebot_has_semiclip", "0");
ConVar ebot_breakable_health_limit("ebot_breakable_health_limit", "3000.0");
ConVar ebot_force_shortest_path("ebot_force_shortest_path", "0");
ConVar ebot_pathfinder_seed_min("ebot_pathfinder_seed_min", "0.9");
ConVar ebot_pathfinder_seed_max("ebot_pathfinder_seed_max", "1.1");
ConVar ebot_helicopter_width("ebot_helicopter_width", "54.0");
ConVar ebot_use_pathfinding_for_avoid("ebot_use_pathfinding_for_avoid", "0");

int Bot::FindGoalZombie(void)
{
	if (g_waypoint->m_terrorPoints.IsEmpty())
	{
		m_currentGoalIndex = crandomint(0, g_numWaypoints - 1);
		return m_currentGoalIndex;
	}

	if (crandomint(1, 3) == 1)
	{
		m_currentGoalIndex = crandomint(0, g_numWaypoints - 1);
		return m_currentGoalIndex;
	}

	m_currentGoalIndex = g_waypoint->m_terrorPoints.Random();
	return m_currentGoalIndex;
}

float GetWaypointDistance(const int start, const int goal)
{
	if (g_isMatrixReady)
		return static_cast<float>(*(g_waypoint->m_distMatrix + (start * g_numWaypoints) + goal));

	return (g_waypoint->m_paths[start].origin - g_waypoint->m_paths[goal].origin).GetLength();
}

int Bot::FindGoalHuman(void)
{
	if (!IsValidWaypoint(m_currentWaypointIndex))
		FindWaypoint();

	if (IsValidWaypoint(m_myMeshWaypoint))
	{
		m_currentGoalIndex = m_myMeshWaypoint;
		return m_currentGoalIndex;
	}
	else if (IsValidWaypoint(m_zhCampPointIndex))
	{
		m_currentGoalIndex = m_zhCampPointIndex;
		return m_currentGoalIndex;
	}
	else
	{
		if (!g_waypoint->m_zmHmPoints.IsEmpty())
		{
			if (g_DelayTimer < engine->GetTime())
			{
				float dist, maxDist = 999999999999.0f;
				int16_t i, index = -1, temp;

				if (m_numFriendsLeft)
				{
					for (i = 0; i < g_waypoint->m_zmHmPoints.Size(); i++)
					{
						temp = g_waypoint->m_zmHmPoints.Get(i);
						dist = GetWaypointDistance(m_currentWaypointIndex, temp);
						if (dist > maxDist)
							continue;

						// bots won't left you alone like other persons in your life...
						if (!IsFriendReachableToPosition(g_waypoint->m_paths[temp].origin))
							continue;

						index = temp;
						maxDist = dist;
					}

					if (!IsValidWaypoint(index))
					{
						maxDist = 999999999999.0f;
						int16_t i, index = -1, temp;
						for (i = 0; i < g_waypoint->m_zmHmPoints.Size(); i++)
						{
							temp = g_waypoint->m_zmHmPoints.Get(i);
							dist = GetWaypointDistance(m_currentWaypointIndex, temp);
							if (dist > maxDist)
								continue;

							// no friends nearby? go to safe camp spot
							if (IsEnemyReachableToPosition(g_waypoint->m_paths[temp].origin))
								continue;

							index = temp;
							maxDist = dist;
						}
					}
				}

				// if we are alone just stay to nearest
				// theres nothing we can do...
				if (!IsValidWaypoint(index))
				{
					maxDist = 999999999999.0f;
					for (i = 0; i < g_waypoint->m_zmHmPoints.Size(); i++)
					{
						temp = g_waypoint->m_zmHmPoints.Get(i);
						dist = GetWaypointDistance(m_currentWaypointIndex, temp);

						// at least get nearest camp spot
						if (dist > maxDist)
							continue;

						index = temp;
						maxDist = dist;
					}
				}

				if (IsValidWaypoint(index))
				{
					m_currentGoalIndex = index;
					return m_currentGoalIndex;
				}
				else
				{
					m_currentGoalIndex = g_waypoint->m_zmHmPoints.Random();
					return m_currentGoalIndex;
				}
			}
			else
			{
				m_currentGoalIndex = g_waypoint->m_zmHmPoints.Random();
				return m_currentGoalIndex;
			}
		}
	}

	m_currentGoalIndex = crandomint(0, g_numWaypoints - 1);
	return m_currentGoalIndex;
}

void Bot::MoveTo(const Vector& targetPosition, const bool checkStuck)
{
	const Vector directionOld = (targetPosition + pev->velocity * -m_frameInterval) - (pev->origin + pev->velocity * m_frameInterval);
	m_moveAngles = directionOld.ToAngles();
	m_moveAngles.x = -m_moveAngles.x; // invert for engine
	m_moveSpeed = pev->maxspeed;

	if (checkStuck)
		CheckStuck(directionOld.Normalize2D(), m_frameInterval);
}

void Bot::MoveOut(const Vector& targetPosition, const bool checkStuck)
{
	const Vector directionOld = targetPosition - pev->origin;
	SetStrafeSpeed(directionOld.Normalize2D(), pev->maxspeed);
	m_moveAngles = directionOld.ToAngles();
	m_moveAngles.x = -m_moveAngles.x; // invert for engine
	m_moveSpeed = -pev->maxspeed;

	if (checkStuck)
		CheckStuck((pev->origin- targetPosition).Normalize2D(), m_frameInterval);
}

void Bot::FollowPath(void)
{
	m_navNode.Start();
}

// this function is a main path navigation
void Bot::DoWaypointNav(void)
{
	m_destOrigin = m_waypointOrigin;
	if (m_jumpReady && !m_waitForLanding)
	{
		if (IsOnFloor() || IsOnLadder())
		{
			const Vector myOrigin = GetBottomOrigin(m_myself);
			Vector waypointOrigin = m_waypoint.origin; // directly jump to waypoint, ignore risk of fall

			if (m_waypoint.flags & WAYPOINT_CROUCH)
				waypointOrigin.z -= 18.0f;
			else
				waypointOrigin.z -= 36.0f;

			if (pev->maxspeed > MATH_EQEPSILON)
			{
				const float timeToReachWaypoint = (((waypointOrigin - myOrigin).GetLength() / pev->maxspeed) + MATH_EQEPSILON);
				Vector temp;
				temp.x = (waypointOrigin.x - myOrigin.x) / timeToReachWaypoint;
				temp.y = (waypointOrigin.y - myOrigin.y) / timeToReachWaypoint;
				temp.z = ((waypointOrigin.z - myOrigin.z) * pev->gravity * squaredf(timeToReachWaypoint)) / timeToReachWaypoint;
				if (!temp.IsNull())
				{
					m_moveAngles.x = pev->velocity.x = temp.x;
					m_moveAngles.y = pev->velocity.y = temp.y;
					m_moveAngles.z = pev->velocity.z = temp.z;
				}
			}

			m_duckTime = engine->GetTime() + 1.25f;
			m_buttons |= (IN_DUCK | IN_JUMP);
			m_jumpReady = false;
			m_waitForLanding = true;
		}

		return;
	}

	if (m_waitForLanding)
	{
		if (IsOnFloor() || IsOnLadder() || IsInWater())
			SetProcess(Process::Pause, "waiting a bit for next jump", true, engine->GetTime() + crandomfloat(0.1f, 0.35f));

		return;
	}

	if (!IsValidWaypoint(m_currentWaypointIndex) && IsValidWaypoint(m_navNode.First()))
		ChangeWptIndex(m_navNode.First());

	if (m_waypoint.flags & WAYPOINT_FALLCHECK)
	{
		TraceResult tr;
		const Vector origin = g_waypoint->m_paths[m_currentWaypointIndex].origin;
		TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), TraceIgnore::Nothing, m_myself, &tr);
		if (tr.flFraction >= 1.0f)
		{
			m_navNode.Clear();
			ResetStuck();
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;
			return;
		}
	}
	else if (m_waypoint.flags & WAYPOINT_WAITUNTIL)
	{
		TraceResult tr;
		const Vector origin = g_waypoint->m_paths[m_currentWaypointIndex].origin;
		TraceLine(origin, m_waypoint.origin - Vector(0.0f, 0.0f, 60.0f), TraceIgnore::Nothing, m_myself, &tr);
		if (tr.flFraction >= 1.0f)
		{
			ResetStuck();
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;
			return;
		}
	}
	else if (m_waypoint.flags & WAYPOINT_HELICOPTER)
	{
		TraceResult tr;
		if (!FNullEnt(g_helicopter))
		{
			const Vector center = GetBoxOrigin(g_helicopter);
			if (IsVisible(center, m_myself))
			{
				if ((pev->origin - center).GetLengthSquared2D() < squaredf(70))
				{
					ResetStuck();
					m_moveSpeed = 0.0f;
					m_strafeSpeed = 0.0f;
					m_isEnemyReachable = false;
				}
				else
				{
					MoveTo(center, false);
					if (IsOnFloor() && pev->velocity.GetLength2D() < (pev->maxspeed * 0.5f))
						m_buttons |= IN_JUMP;
				}
			}
			else
			{
				MakeVectors(g_helicopter->v.angles);
				const Vector right = center + g_pGlobals->v_right * ebot_helicopter_width.GetFloat();
				const Vector left = center - g_pGlobals->v_right * ebot_helicopter_width.GetFloat();
				if ((pev->origin - right).GetLengthSquared2D() < (pev->origin - left).GetLengthSquared2D())
				{
					TraceLine(center, right, TraceIgnore::Nothing, m_myself, &tr);
					if (tr.flFraction >= 1.0f)
					{
						MoveTo(right, false);
						return;
					}
				}

				TraceLine(center, left, TraceIgnore::Nothing, m_myself, &tr);
				if (tr.flFraction >= 1.0f)
				{
					MoveTo(left, false);
					return;
				}

				MoveTo(center, false);
			}
		}
		else
		{
			const Vector origin = g_waypoint->m_paths[m_currentWaypointIndex].origin;
			TraceLine(origin + Vector(0.0f, 0.0f, 36.0f), origin - Vector(0.0f, 0.0f, 36.0f), TraceIgnore::Nothing, m_myself, &tr);
			if (!FNullEnt(tr.pHit))
				g_helicopter = tr.pHit;
			else
			{
				ResetStuck();
				m_moveSpeed = 0.0f;
				m_strafeSpeed = 0.0f;
			}
		}

		return;
	}

	if (m_waypoint.flags & WAYPOINT_LIFT)
	{
		if (!UpdateLiftHandling())
			return;

		if (!UpdateLiftStates())
			return;
	}

	if (m_waypoint.flags & WAYPOINT_CROUCH)
	{
		if (pev->flags & FL_DUCKING)
		{
			if (IsOnFloor())
			{
				// bots sometimes slow down when they are crouching... related to the engine bug?
				const float speed = pev->velocity.GetLength2D();
				if (speed < (pev->maxspeed * 0.5f))
				{
					const float speedNextFrame = speed * 1.25f;
					const Vector vel = (m_waypoint.origin - pev->origin).Normalize2D();
					if (!vel.IsNull())
					{
						pev->velocity.x = vel.x * speedNextFrame;
						pev->velocity.y = vel.y * speedNextFrame;
					}
				}
			}
		}
		else
			m_duckTime = engine->GetTime() + 1.25f;
	}

	// check if we are going through a door...
	if (g_hasDoors && FNullEnt(m_liftEntity))
	{
		TraceResult tr;
		TraceLine(pev->origin, m_waypoint.origin, TraceIgnore::Monsters, m_myself, &tr);
		if (!FNullEnt(tr.pHit) && cstrncmp(STRING(tr.pHit->v.classname), "func_door", 9) == 0)
		{
			// if the door is near enough...
			const Vector origin = GetEntityOrigin(tr.pHit);
			if ((pev->origin - origin).GetLengthSquared() < squaredf(54.0f))
			{
				ResetStuck(); // don't consider being stuck

				if (!(m_oldButtons & IN_USE) && !(m_buttons & IN_USE))
				{
					LookAt(origin);

					// do not use door directrly under xash, or we will get failed assert in gamedll code
					if (g_gameVersion & Game::Xash)
						m_buttons |= IN_USE;
					else
						MDLL_Use(tr.pHit, m_myself); // also 'use' the door randomly
				}
			}

			const float time = engine->GetTime();
			edict_t* button = FindNearestButton(STRING(tr.pHit->v.targetname));
			if (!FNullEnt(button))
			{
				m_buttonEntity = button;
				if (SetProcess(Process::UseButton, "trying to use a button", false, time + 10.0f))
					return;
			}
			
			// if bot hits the door, then it opens, so wait a bit to let it open safely
			if (m_timeDoorOpen < time && pev->velocity.GetLengthSquared2D() < squaredf(pev->maxspeed * 0.20f))
			{
				m_timeDoorOpen = time + 3.0f;

				if (m_tryOpenDoor++ > 2 && !FNullEnt(m_nearestEnemy) && IsWalkableLineClear(EyePosition(), m_nearestEnemy->v.origin))
				{
					m_hasEnemiesNear = true;
					m_enemyOrigin = m_nearestEnemy->v.origin;
					m_tryOpenDoor = 0;
				}
				else
				{
					if (m_isSlowThink)
					{
						SetProcess(Process::Pause, "waiting for door open", true, time + 1.25f);
						m_moveSpeed = 0.0f;
						m_strafeSpeed = 0.0f;
						return;
					}

					m_tryOpenDoor = 0;
				}
			}
		}
	}

	bool next = false;

	if (IsOnLadder() || m_waypoint.flags & WAYPOINT_LADDER)
	{
		// special detection if someone is using the ladder (to prevent to have bots-towers on ladders)
		TraceResult tr;
		bool foundGround = false;
		int previousNode = 0;
		extern ConVar ebot_ignore_enemies;
		for (const auto& client : g_clients)
		{
			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || FNullEnt(client.ent) || (client.ent->v.movetype != MOVETYPE_FLY) || client.index == m_index)
				continue;

			foundGround = false;
			previousNode = 0;

			// more than likely someone is already using our ladder...
			if ((client.ent->v.origin - m_destOrigin).GetLengthSquared() < squaredf(48.0f))
			{
				if (client.team != m_team && !ebot_ignore_enemies.GetBool())
				{
					TraceLine(EyePosition(), client.ent->v.origin, TraceIgnore::Monsters, m_myself, &tr);

					// bot found an enemy on his ladder - he should see him...
					if (!FNullEnt(tr.pHit) && tr.pHit == client.ent)
					{
						m_hasEnemiesNear = true;
						m_nearestEnemy = client.ent;
						m_enemyOrigin = m_nearestEnemy->v.origin;
						break;
					}
				}
				else
				{
					TraceHull(EyePosition(), m_destOrigin, TraceIgnore::Monsters, (pev->flags & FL_DUCKING) ? head_hull : human_hull, m_myself, &tr);

					// someone is above or below us and is using the ladder already
					if (client.ent->v.movetype == MOVETYPE_FLY && cabsf(pev->origin.z - client.ent->v.origin.z) > 15.0f && !FNullEnt(tr.pHit) && tr.pHit == client.ent)
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

		if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(24.0f))
			next = true;
	}
	else
	{
		if (m_waypoint.radius < 5 || m_currentTravelFlags & PATHFLAG_JUMP)
		{
			if (pev->flags & FL_DUCKING)
			{
				if ((pev->origin - m_destOrigin).GetLengthSquared2D() < squaredf(24.0f))
					next = true;
			}
			else
			{
				if (((pev->origin + pev->velocity * m_pathInterval) - (m_destOrigin + pev->velocity * -m_pathInterval)).GetLengthSquared2D() < squaredf(4.0f))
					next = true;
			}
		}
		else
		{
			if ((pev->origin - m_destOrigin).GetLengthSquared2D() < squaredf(static_cast<float>(m_waypoint.radius)))
				next = true;
		}
	}

	if (next)
	{
		m_currentTravelFlags = 0;
		m_navNode.Shift();

		if (!m_navNode.IsEmpty())
		{
			int16_t i;
			const int16_t destIndex = m_navNode.First();
			for (i = 0; i < Const_MaxPathIndex; i++)
			{
				if (m_waypoint.index[i] == destIndex)
				{
					m_prevTravelFlags = m_currentTravelFlags;
					m_currentTravelFlags = m_waypoint.connectionFlags[i];
					break;
				}
			}

			ChangeWptIndex(destIndex);
		}
	}
}

bool Bot::UpdateLiftHandling(void)
{
	const float time = engine->GetTime();
	bool liftClosedDoorExists = false;

	TraceResult tr;

	// wait for something about for lift
	auto wait = [&]()
	{
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;
		m_buttons &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
		ResetStuck();
	};

	// trace line to door
	TraceLine(pev->origin, m_waypoint.origin, TraceIgnore::Everything, m_myself, &tr);
	if (tr.flFraction < 1.0f && (m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor || m_liftState == LiftState::LookingButtonOutside) && !FNullEnt(tr.pHit) && cstrcmp(STRING(tr.pHit->v.classname), "func_door") == 0 && pev->groundentity != tr.pHit)
	{
		if (m_liftState == LiftState::None)
		{
			m_liftState = LiftState::LookingButtonOutside;
			m_liftUsageTime = time + 7.0f;
		}

		liftClosedDoorExists = true;
	}

	if (!m_navNode.IsEmpty())
	{
		// trace line down
		TraceLine(m_waypoint.origin, m_waypoint.origin + Vector(0.0f, 0.0f, -54.0f), TraceIgnore::Everything, m_myself, &tr);

		// if trace result shows us that it is a lift
		if (!liftClosedDoorExists && !FNullEnt(tr.pHit) && (cstrcmp(STRING(tr.pHit->v.classname), "func_door") == 0 || cstrcmp(STRING(tr.pHit->v.classname), "func_plat") == 0 || cstrcmp(STRING(tr.pHit->v.classname), "func_train") == 0))
		{
			if ((m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor || m_liftState == LiftState::LookingButtonOutside) && tr.pHit->v.velocity.z == 0.0f)
			{
				if (cabsf(pev->origin.z - tr.vecEndPos.z) < 70.0f)
				{
					m_liftEntity = tr.pHit;
					m_liftState = LiftState::EnteringIn;
					m_liftTravelPos = m_waypoint.origin;
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
						TraceLine(m_waypoint.origin, pointer->origin, TraceIgnore::Everything, m_myself, &tr);
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
			for (Bot* const &bot : g_botManager->m_bots)
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

		for (Bot* const &bot : g_botManager->m_bots)
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
			m_buttonEntity = button;
			SetProcess(Process::UseButton, "trying to use a button", false, time + 10.0f);
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
					if (FNullEnt(client.ent) || !(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == m_myself || FNullEnt(client.ent->v.groundentity))
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
					m_liftState = LiftState::WaitingFor;
					m_liftUsageTime = time + 20.0f;
					m_buttonEntity = button;
					SetProcess(Process::UseButton, "trying to use a button", false, time + 10.0f);
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
			if (g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LIFT && (m_waypoint.origin.z - pev->origin.z) > 50.0f && (g_waypoint->GetPath(m_prevWptIndex[0])->origin.z - pev->origin.z) > 50.0f)
			{
				m_liftState = LiftState::None;
				m_liftEntity = nullptr;
				m_liftUsageTime = 0.0f;

				if (IsValidWaypoint(m_prevWptIndex[2]))
					FindPath(m_currentWaypointIndex, m_prevWptIndex[2]);
				else
				{
					m_navNode.Clear();
					FindWaypoint();
				}

				return false;
			}
		}
	}

	return true;
}

bool Bot::UpdateLiftStates(void)
{
	if (!FNullEnt(m_liftEntity) && !(m_waypoint.flags & WAYPOINT_LIFT))
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

		m_navNode.Clear();

		if (IsValidWaypoint(m_prevWptIndex[0]))
		{
			if (!(g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LIFT))
			{
				ChangeWptIndex(m_prevWptIndex[0]);
				if (!m_navNode.IsEmpty())
					m_navNode.Set(0, m_prevWptIndex[0]);
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

struct HeapNode
{
	int16_t id{0};
	float priority{0.0f};
} m_heap[Const_MaxWaypoints];

class PriorityQueue
{
public:
	PriorityQueue(void);
	~PriorityQueue(void);
	inline bool IsEmpty(void) const { return !m_size; }
	inline int16_t Size(void) const { return m_size; }
	inline void InsertLowest(const int16_t value, const float priority);
	inline int16_t RemoveLowest(void);
	inline void Setup(void) { m_size = 0; }
private:
	int16_t m_size{0};
};

PriorityQueue::PriorityQueue(void)
{
	m_size = 0;
}

PriorityQueue::~PriorityQueue(void)
{
	m_size = 0;
}

// inserts a value into the priority queue
void PriorityQueue::InsertLowest(const int16_t value, const float priority)
{
	m_heap[m_size].priority = priority;
	m_heap[m_size].id = value;

	static int16_t child{};
	static int16_t parent{};
	static HeapNode temp{};

	child = ++m_size - 1;
	while (child > 0)
	{
		parent = (child - 1) / 2;
		if (m_heap[parent].priority < m_heap[child].priority)
			break;

		temp = m_heap[child];
		m_heap[child] = m_heap[parent];
		m_heap[parent] = temp;
		child = parent;
	}
}

// removes the smallest item from the priority queue
int16_t PriorityQueue::RemoveLowest(void)
{
	static int16_t retID{};
	retID = m_heap[0].id;

	m_size--;
	m_heap[0] = m_heap[m_size];

	static int16_t parent{};
	static int16_t child{};
	static int16_t rightChild{};
	static HeapNode ref{};

	parent = 0;
	child = (2 * parent) + 1;
	ref = m_heap[parent];
	while (child < m_size)
	{
		rightChild = (2 * parent) + 2;
		if (rightChild < m_size)
		{
			if (m_heap[rightChild].priority < m_heap[child].priority)
				child = rightChild;
		}

		if (ref.priority < m_heap[child].priority)
			break;

		m_heap[parent] = m_heap[child];
		parent = child;
		child = (2 * parent) + 1;
	}

	m_heap[parent] = ref;
	return retID;
}

inline const float HF_DistanceSquared(const int16_t& start, const int16_t& goal)
{
	return (g_waypoint->m_paths[start].origin - g_waypoint->m_paths[goal].origin).GetLengthSquared();
}

inline const float HF_Distance(const int16_t& start, const int16_t& goal)
{
	return (g_waypoint->m_paths[start].origin - g_waypoint->m_paths[goal].origin).GetLength();
}

inline const float HF_Matrix(const int16_t& start, const int16_t& goal)
{
	return static_cast<float>(*(g_waypoint->m_distMatrix + (start * g_numWaypoints) + goal));
}

inline const float HF_Auto(const int16_t& start, const int16_t& goal)
{
	if (g_isMatrixReady)
		return HF_Matrix(start, goal);

	return HF_Distance(start, goal);
}

static Path pathCache;
static int8_t countCache;
inline const float GF_CostHuman(const int16_t& index, const int16_t& parent, const uint32_t& parentFlags, const int8_t& team, const float& gravity, const bool& isZombie)
{
	if (!parentFlags)
		return HF_Auto(index, parent);

	if (parentFlags & WAYPOINT_AVOID)
		return 65355.0f;

	if (isZombie)
	{
		if (parentFlags & WAYPOINT_HUMANONLY)
			return 65355.0f;
	}
	else
	{
		if (parentFlags & WAYPOINT_ZOMBIEONLY)
			return 65355.0f;

		if (parentFlags & WAYPOINT_DJUMP)
			return 65355.0f;
	}

	pathCache = g_waypoint->m_paths[parent];
	if (parentFlags & WAYPOINT_ONLYONE)
	{
		for (const auto& client : g_clients)
		{
			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			if ((client.origin - pathCache.origin).GetLengthSquared() < squaredi(static_cast<int>(pathCache.radius) + 64))
				return 65355.0f;
		}
	}

	static float distance;
	static float totalDistance;
	totalDistance = 0.0f;
	countCache = 0;
	for (const auto& client : g_clients)
	{
		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team == client.team || !IsZombieEntity(client.ent))
			continue;

		distance = ((client.ent->v.origin + client.ent->v.velocity * g_pGlobals->frametime) - pathCache.origin).GetLengthSquared();
		if (distance < squaredi(static_cast<int>(pathCache.radius) + 128))
			countCache++;

		totalDistance += distance;
	}

	if (countCache && totalDistance > 0.0f)
		return (HF_Auto(index, parent) * static_cast<float>(countCache)) + totalDistance;

	return HF_Auto(index, parent);
}

inline const float GF_CostCareful(const int16_t& index, const int16_t& parent, const uint32_t& parentFlags, const int8_t& team, const float& gravity, const bool& isZombie)
{
	if (!parentFlags)
		return HF_Auto(index, parent);

	if (parentFlags & WAYPOINT_AVOID)
		return 65355.0f;

	if (isZombie)
	{
		if (parentFlags & WAYPOINT_HUMANONLY)
			return 65355.0f;
	}
	else
	{
		if (parentFlags & WAYPOINT_ZOMBIEONLY)
			return 65355.0f;

		if (parentFlags & WAYPOINT_DJUMP)
			return 65355.0f;
	}

	if (parentFlags & WAYPOINT_ONLYONE)
	{
		pathCache = g_waypoint->m_paths[parent];
		for (const auto& client : g_clients)
		{
			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			if ((client.origin - pathCache.origin).GetLengthSquared() < squaredi(static_cast<int>(pathCache.radius) + 64))
				return 65355.0f;
		}
	}

	if (isZombie)
	{
		if (parentFlags & WAYPOINT_DJUMP)
		{
			pathCache = g_waypoint->m_paths[parent];
			countCache = 0;
			for (const auto& client : g_clients)
			{
				if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != team)
					continue;

				if ((client.origin - pathCache.origin).GetLengthSquared() < squaredi(static_cast<int>(pathCache.radius) + 512))
					countCache++;
				else if (IsVisible(pathCache.origin, client.ent))
					countCache++;
			}

			// don't count me
			if (countCache < static_cast<int8_t>(2))
				return 65355.0f;

			return HF_Auto(index, parent) / static_cast<float>(countCache);
		}
	}

	return HF_Auto(index, parent);
}

inline const float GF_CostNormal(const int16_t& index, const int16_t& parent, const uint32_t& parentFlags, const int8_t& team, const float& gravity, const bool& isZombie)
{
	if (!parentFlags)
		return HF_Distance(index, parent);

	if (parentFlags & WAYPOINT_AVOID)
		return 65355.0f;

	if (isZombie)
	{
		if (parentFlags & WAYPOINT_HUMANONLY)
			return 65355.0f;
	}
	else
	{
		if (parentFlags & WAYPOINT_ZOMBIEONLY)
			return 65355.0f;

		if (parentFlags & WAYPOINT_DJUMP)
			return 65355.0f;
	}

	if (parentFlags & WAYPOINT_ONLYONE)
	{
		pathCache = g_waypoint->m_paths[parent];
		for (const auto& client : g_clients)
		{
			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			if ((client.origin - pathCache.origin).GetLengthSquared() < squaredi(static_cast<int>(pathCache.radius) + 64))
				return 65355.0f;
		}
	}

	if (isZombie)
	{
		if (parentFlags & WAYPOINT_DJUMP)
		{
			pathCache = g_waypoint->m_paths[parent];
			countCache = 0;
			for (const auto& client : g_clients)
			{
				if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != team)
					continue;

				if ((client.origin - pathCache.origin).GetLengthSquared() < squaredi(static_cast<int>(pathCache.radius) + 512))
					countCache++;
				else if (IsVisible(pathCache.origin, client.ent))
					countCache++;
			}

			// don't count me
			if (countCache < static_cast<int8_t>(2))
				return 65355.0f;

			return HF_Auto(index, parent) / static_cast<float>(countCache);
		}
	}

	if (parentFlags & WAYPOINT_LADDER)
		return HF_Auto(index, parent) * 2.0f;

	return HF_Auto(index, parent);
}

inline const float GF_CostRusher(const int16_t& index, const int16_t& parent, const uint32_t& parentFlags, const int8_t& team, const float& gravity, const bool& isZombie)
{
	if (!parentFlags)
		return HF_Auto(index, parent);

	if (parentFlags & WAYPOINT_AVOID)
		return 65355.0f;

	if (isZombie)
	{
		if (parentFlags & WAYPOINT_HUMANONLY)
			return 65355.0f;
	}
	else
	{
		if (parentFlags & WAYPOINT_ZOMBIEONLY)
			return 65355.0f;

		if (parentFlags & WAYPOINT_DJUMP)
			return 65355.0f;
	}

	if (parentFlags & WAYPOINT_ONLYONE)
	{
		pathCache = g_waypoint->m_paths[parent];
		for (const auto& client : g_clients)
		{
			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			if ((client.origin - pathCache.origin).GetLengthSquared() < squaredi(static_cast<int>(pathCache.radius) + 64))
				return 65355.0f;
		}
	}

	// rusher bots never wait for boosting
	if (parentFlags & WAYPOINT_DJUMP)
		return 65355.0f;

	if (parentFlags & WAYPOINT_CROUCH)
		return HF_Auto(index, parent) * 2.0f;

	return HF_Auto(index, parent);
}

enum class RouteState : int8_t
{
	New = 0,
	Open = 1,
	Closed = 2,
};

struct AStar
{
	float g{0.0f};
	float f{0.0f};
	int16_t parent{-1};
	RouteState state{RouteState::New};
};

AStar waypoints[Const_MaxWaypoints];

// this function finds a path from srcIndex to destIndex
void Bot::FindPath(int& srcIndex, int& destIndex)
{
	if (g_pathTimer > engine->GetTime())
		return;

	if (!IsValidWaypoint(srcIndex))
	{
		srcIndex = FindWaypoint();
		if (!IsValidWaypoint(srcIndex))
			return;
	}

	if (ebot_force_shortest_path.GetBool() || g_numWaypoints > 2048)
	{
		FindShortestPath(srcIndex, destIndex);
		return;
	}

	if (srcIndex == destIndex)
		return;

	const float (*hcalc) (const int16_t&, const int16_t&) = nullptr;
	const float (*gcalc) (const int16_t&, const int16_t&, const uint32_t&, const int8_t&, const float&, const bool&) = nullptr;

	if (ebot_zombies_as_path_cost.GetBool() && !m_isZombieBot)
		gcalc = GF_CostHuman;
	else if (m_personality == Personality::Careful)
		gcalc = GF_CostCareful;
	else if (m_personality == Personality::Rusher)
		gcalc = GF_CostRusher;
	else
		gcalc = GF_CostNormal;

	if (!gcalc)
		return;

	if (g_isMatrixReady)
		hcalc = HF_Matrix;
	else
		hcalc = HF_Distance;

	if (!hcalc)
		return;

	int seed = m_index + m_numSpawns + m_currentWeapon;
	float min = ebot_pathfinder_seed_min.GetFloat();
	float max = ebot_pathfinder_seed_max.GetFloat();

	int16_t i;
	Path currPath;

	if (m_isStuck)
		seed += static_cast<int>(engine->GetTime() * 0.1f);

	for (i = 0; i < Const_MaxWaypoints; i++)
	{
		waypoints[i].g = 0.0f;
		waypoints[i].f = 0.0f;
		waypoints[i].parent = -1;
		waypoints[i].state = RouteState::New;
	}

	// put start waypoint into open list
	AStar& srcWaypoint = waypoints[srcIndex];
	srcWaypoint.g = ((gcalc(srcIndex, destIndex, 0, m_team, pev->gravity, m_isZombieBot) * crandomfloatfast(seed, min, max)));
	srcWaypoint.f = srcWaypoint.g + hcalc(srcIndex, destIndex);
	srcWaypoint.state = RouteState::Open;

	// loop cache
	AStar* currWaypoint;
	AStar* childWaypoint;
	int16_t currentIndex, self;
	uint32_t flags;
	float g, f;

	PriorityQueue openList;
	openList.Setup();
	openList.InsertLowest(srcIndex, srcWaypoint.g);
	while (!openList.IsEmpty())
	{
		currentIndex = openList.RemoveLowest();
		if (openList.Size() >= g_numWaypoints - 1)
		{
			Kick();
			return;
		}

		if (currentIndex == destIndex)
		{
			m_navNode.Clear();

			do
			{
				m_navNode.Add(currentIndex);
				currentIndex = waypoints[currentIndex].parent;
			} while (IsValidWaypoint(currentIndex));

			m_navNode.Reverse();
			if (m_navNode.HasNext() && (g_waypoint->GetPath(m_navNode.Next())->origin - pev->origin).GetLengthSquared() < (g_waypoint->GetPath(m_navNode.Next())->origin - g_waypoint->GetPath(m_navNode.First())->origin).GetLengthSquared())
				m_navNode.Shift();

			ChangeWptIndex(m_navNode.First());
			g_pathTimer = engine->GetTime() + 0.05f;
			return;
		}

		currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->state != RouteState::Open)
			continue;

		currWaypoint->state = RouteState::Closed;

		currPath = g_waypoint->m_paths[currentIndex];
		for (i = 0; i < Const_MaxPathIndex; i++)
		{
			self = currPath.index[i];
			if (!IsValidWaypoint(self))
				continue;
			
			flags = g_waypoint->m_paths[self].flags;
			if (flags)
			{
				if (flags & WAYPOINT_FALLCHECK)
				{
					TraceResult tr;
					const Vector origin = g_waypoint->m_paths[self].origin;
					TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), TraceIgnore::Nothing, m_myself, &tr);
					if (tr.flFraction >= 1.0f)
						continue;
				}
				
				if (flags & WAYPOINT_SPECIFICGRAVITY)
				{
					if ((pev->gravity * engine->GetGravity()) > g_waypoint->m_paths[self].gravity)
						continue;
				}

				if (flags & WAYPOINT_ONLYONE)
				{
					bool skip = false;
					for (Bot* const& bot : g_botManager->m_bots)
					{
						if (skip)
							break;

						if (bot && bot->m_isAlive && bot->m_myself != m_myself)
						{
							int16_t j;
							for (j = 0; j < bot->m_navNode.Length(); j++)
							{
								if (bot->m_navNode.Get(j) == self)
								{
									skip = true;
									break;
								}
							}
						}
					}

					if (skip)
						continue;
				}
			}

			g = currWaypoint->g + ((gcalc(currentIndex, self, flags, m_team, pev->gravity, m_isZombieBot) * crandomfloatfast(seed, min, max)));
			f = g + hcalc(self, destIndex);

			childWaypoint = &waypoints[self];
			if (childWaypoint->state == RouteState::New || childWaypoint->f > f)
			{
				childWaypoint->parent = currentIndex;
				childWaypoint->state = RouteState::Open;
				childWaypoint->g = g;
				childWaypoint->f = f;
				openList.InsertLowest(self, g);
			}
		}
	}

	// roam around poorly :(
	if (m_navNode.IsEmpty())
	{
		MiniArray <int16_t> PossiblePath;
		for (i = 0; i < g_numWaypoints; i++)
		{
			if (waypoints[i].state == RouteState::Closed)
				PossiblePath.Push(i);
		}

		if (!PossiblePath.IsEmpty())
		{
			int index = PossiblePath.Random();
			FindShortestPath(srcIndex, index);
			return;
		}

		FindShortestPath(srcIndex, destIndex);
	}
}

void Bot::FindShortestPath(int& srcIndex, int& destIndex)
{
	if (srcIndex == destIndex)
		return;

	const float (*hcalc) (const int16_t&, const int16_t&) = nullptr;

	if (g_isMatrixReady)
		hcalc = HF_Matrix;
	else
		hcalc = HF_DistanceSquared;

	if (!hcalc)
		return;

	int16_t i;
	for (i = 0; i < Const_MaxWaypoints; i++)
	{
		waypoints[i].g = 0.0f;
		waypoints[i].f = 0.0f;
		waypoints[i].parent = -1;
		waypoints[i].state = RouteState::New;
	}

	AStar& srcWaypoint = waypoints[srcIndex];
	srcWaypoint.f = hcalc(srcIndex, destIndex);
	srcWaypoint.state = RouteState::Open;

	// loop cache
	AStar* currWaypoint;
	AStar* childWaypoint;
	int16_t currentIndex, self;
	uint32_t flags;
	Path currPath;
	float g, f;

	PriorityQueue openList;
	openList.Setup();
	openList.InsertLowest(srcIndex, srcWaypoint.g);
	while (!openList.IsEmpty())
	{
		currentIndex = openList.RemoveLowest();
		if (openList.Size() >= g_numWaypoints - 1)
		{
			Kick();
			return;
		}

		if (currentIndex == destIndex)
		{
			m_navNode.Clear();

			do
			{
				m_navNode.Add(currentIndex);
				currentIndex = waypoints[currentIndex].parent;
			} while (IsValidWaypoint(currentIndex));

			m_navNode.Reverse();
			if (m_navNode.HasNext() && (g_waypoint->GetPath(m_navNode.Next())->origin - pev->origin).GetLengthSquared() < (g_waypoint->GetPath(m_navNode.Next())->origin - g_waypoint->GetPath(m_navNode.First())->origin).GetLengthSquared())
				m_navNode.Shift();

			ChangeWptIndex(m_navNode.First());
			g_pathTimer = engine->GetTime() + 0.05f;
			return;
		}

		currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->state != RouteState::Open)
			continue;

		currWaypoint->state = RouteState::Closed;
		currPath = g_waypoint->m_paths[currentIndex];
		for (i = 0; i < Const_MaxPathIndex; i++)
		{
			self = currPath.index[i];
			if (!IsValidWaypoint(self))
				continue;

			g = currWaypoint->g + hcalc(self, currentIndex);
			f = g + hcalc(self, destIndex);
			childWaypoint = &waypoints[self];
			if (childWaypoint->state == RouteState::New || childWaypoint->f > f)
			{
				childWaypoint->parent = currentIndex;
				childWaypoint->state = RouteState::Open;
				childWaypoint->g = g;
				childWaypoint->f = f;
				openList.InsertLowest(self, g);
			}
		}
	}

	g_pathTimer = engine->GetTime() + 0.05f;
}

void Bot::FindEscapePath(int& srcIndex, const Vector& dangerOrigin)
{
	// if we can't find new path we will go backwards
	m_navNode.Clear();

	// TODO: FIXME: ...
	if (g_pathTimer > engine->GetTime())
		return;

	if (!ebot_use_pathfinding_for_avoid.GetBool())
		return;

	int16_t i;
	if (!IsValidWaypoint(srcIndex))
	{
		i = FindWaypoint();
		srcIndex = i;
		if (!IsValidWaypoint(srcIndex))
			return;
	}

	for (i = 0; i < Const_MaxWaypoints; i++)
	{
		waypoints[i].g = 0.0f;
		waypoints[i].f = 0.0f;
		waypoints[i].parent = -1;
		waypoints[i].state = RouteState::New;
	}

	AStar& srcWaypoint = waypoints[srcIndex];
	srcWaypoint.g = -(g_waypoint->m_paths[srcIndex].origin - pev->origin).GetLengthSquared();
	srcWaypoint.f = srcWaypoint.g + -(g_waypoint->m_paths[srcIndex].origin - dangerOrigin).GetLengthSquared();
	srcWaypoint.state = RouteState::Open;

	// loop cache
	AStar* currWaypoint;
	AStar* childWaypoint;
	int16_t currentIndex, self;
	uint32_t flags;
	Path currPath;
	float f, g;
	bool found = false;

	PriorityQueue openList;
	openList.Setup();
	openList.InsertLowest(srcIndex, srcWaypoint.g);
	while (!openList.IsEmpty())
	{
		currentIndex = openList.RemoveLowest();
		if (openList.Size() >= g_numWaypoints - 1)
		{
			Kick();
			return;
		}

		currPath = g_waypoint->m_paths[currentIndex];
		if (!IsEnemyReachableToPosition(currPath.origin))
		{
			g_pathTimer = engine->GetTime() + 0.05f;
			break;
		}

		currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->state != RouteState::Open)
			continue;

		currWaypoint->state = RouteState::Closed;
		for (i = 0; i < Const_MaxPathIndex; i++)
		{
			self = currPath.index[i];
			if (!IsValidWaypoint(self))
				continue;
			
			flags = g_waypoint->m_paths[self].flags;
			if (flags)
			{
				if (flags & WAYPOINT_FALLCHECK)
				{
					TraceResult tr;
					const Vector origin = g_waypoint->m_paths[self].origin;
					TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), TraceIgnore::Nothing, m_myself, &tr);
					if (tr.flFraction >= 1.0f)
						continue;
				}

				if (flags & WAYPOINT_SPECIFICGRAVITY)
				{
					if ((pev->gravity * engine->GetGravity()) > g_waypoint->m_paths[self].gravity)
						continue;
				}
			}

			g = currWaypoint->g + -(g_waypoint->m_paths[self].origin - pev->origin).GetLengthSquared();
			f = g + -(g_waypoint->m_paths[self].origin - dangerOrigin).GetLengthSquared();
			childWaypoint = &waypoints[self];
			if (childWaypoint->state == RouteState::New || childWaypoint->f < f)
			{
				// put the current child into open list
				childWaypoint->parent = currentIndex;
				childWaypoint->state = RouteState::Open;
				childWaypoint->g = g;
				childWaypoint->f = f;
				openList.InsertLowest(self, g);
			}
		}
	}

	if (found)
	{
		do
		{
			m_navNode.Add(currentIndex);
			currentIndex = waypoints[currentIndex].parent;
		} while (IsValidWaypoint(currentIndex));

		// at least get 2
		if (m_navNode.HasNext())
		{
			m_navNode.Reverse();
			if ((g_waypoint->GetPath(m_navNode.Next())->origin - pev->origin).GetLengthSquared() < (g_waypoint->GetPath(m_navNode.Next())->origin - g_waypoint->GetPath(m_navNode.First())->origin).GetLengthSquared())
				m_navNode.Shift();

			ChangeWptIndex(m_navNode.First());
		}
	}
	else
		g_pathTimer = engine->GetTime() + 0.05f;
}

void Bot::CheckTouchEntity(edict_t* entity)
{
	if (FNullEnt(entity))
		return;

	// if we won't be able to break it, don't try
	if (entity->v.takedamage == DAMAGE_NO)
		return;

	// see if it's breakable
	if ((FClassnameIs(entity, "func_breakable") || (FClassnameIs(entity, "func_pushable") && (entity->v.spawnflags & SF_PUSH_BREAKABLE)) || FClassnameIs(entity, "func_wall")) && entity->v.health > 0.0f && entity->v.health < ebot_breakable_health_limit.GetFloat())
	{
		TraceResult tr;
		TraceHull(EyePosition(), m_waypoint.origin, TraceIgnore::Nothing, head_hull, m_myself, &tr);

		TraceResult tr2;
		TraceLine(EyePosition(), m_destOrigin, TraceIgnore::Nothing, m_myself, &tr2);

		// double check
		if ((!FNullEnt(tr.pHit) && tr.pHit == entity) || (!FNullEnt(tr2.pHit) && tr2.pHit == entity))
		{
			m_breakableEntity = entity;
			m_breakableOrigin = GetBoxOrigin(m_breakableEntity);

			const float time2 = engine->GetTime();
			SetProcess(Process::DestroyBreakable, "trying to destroy a breakable", false, time2 + 60.0f);

			if (pev->origin.z > m_breakableOrigin.z) // make bots smarter
			{
				// tell my enemies to destroy it, so i will fall
				edict_t* ent;
				for (const auto& enemy : g_botManager->m_bots)
				{
					if (!enemy)
						continue;

					if (m_team == enemy->m_team)
						continue;

					if (!enemy->m_isAlive)
						continue;

					if (enemy->m_isZombieBot)
						continue;

					if (enemy->m_currentWeapon == Weapon::Knife)
						continue;

					ent = enemy->m_myself;
					if (!ent)
						continue;

					TraceHull(enemy->EyePosition(), m_breakableOrigin, TraceIgnore::Nothing, point_hull, ent, &tr);
					TraceHull(ent->v.origin, m_breakableOrigin, TraceIgnore::Nothing, head_hull, ent, &tr2);
					if ((!FNullEnt(tr.pHit) && tr.pHit == entity) || (!FNullEnt(tr2.pHit) && tr2.pHit == entity))
					{
						enemy->m_breakableEntity = entity;
						enemy->m_breakableOrigin = m_breakableOrigin;
						enemy->SetProcess(Process::DestroyBreakable, "trying to destroy a breakable for my enemy fall", false, time2 + 60.0f);
					}
				}
			}
			else if (!m_isZombieBot) // tell my friends to destroy it
			{
				edict_t* ent;
				for (Bot* const& bot : g_botManager->m_bots)
				{
					if (!bot)
						continue;

					if (m_team != bot->m_team)
						continue;

					if (!bot->m_isAlive)
						continue;

					if (bot->m_isZombieBot)
						continue;

					ent = bot->m_myself;
					if (!ent)
						continue;

					if (m_myself == ent)
						continue;

					TraceHull(bot->EyePosition(), m_breakableOrigin, TraceIgnore::Nothing, point_hull, ent, &tr);
					TraceHull(ent->v.origin, m_breakableOrigin, TraceIgnore::Nothing, head_hull, ent, &tr2);

					if ((!FNullEnt(tr.pHit) && tr.pHit == entity) || (!FNullEnt(tr2.pHit) && tr2.pHit == entity))
					{
						bot->m_breakableEntity = entity;
						bot->m_breakableOrigin = m_breakableOrigin;
						bot->SetProcess(Process::DestroyBreakable, "trying to help my friend for destroy a breakable", false, time2 + 60.0f);
					}
				}
			}
		}
	}
}

int Bot::FindWaypoint(void)
{
	if (m_isAlive)
	{
		int index;
		if (!m_isStuck && m_navNode.HasNext() && g_waypoint->Reachable(m_myself, m_navNode.First()))
			index = m_navNode.First();
		else if (!m_isStuck && g_waypoint->Reachable(m_myself, m_navNode.First()))
			index = g_clients[m_index].wp;
		else
		{
			index = g_waypoint->FindNearestToEnt(pev->origin, 2048.0f, m_myself);
			if (!IsValidWaypoint(index))
				index = g_waypoint->FindNearest(pev->origin);
		}
	
		ChangeWptIndex(index);
		return index;
	}
	
	return m_currentWaypointIndex;
}

// return priority of player (0 = max pri)
int GetPlayerPriority(edict_t* player)
{
	// human players have highest priority
	const Bot* bot = g_botManager->GetBot(player);
	if (bot)
		return bot->m_index + 3; // everyone else is ranked by their unique ID (which cannot be zero)

	return 1;
}

void Bot::ResetStuck(void)
{
	m_stuckTime = 0.0f;
	m_isStuck = false;
	m_checkFall = false;
	for (Vector& fall : m_checkFallPoint)
		fall = nullvec;

	ResetCollideState();
	IgnoreCollisionShortly();
}

void Bot::IgnoreCollisionShortly(void)
{
	const float time2 = engine->GetTime();
	m_prevTime = time2 + 0.25f;
	m_lastCollTime = time2 + 2.0f;
	if (m_waypointTime < time2 + 7.0f)
		m_waypointTime = time2 + 7.0f;

	m_isStuck = false;
	m_stuckTime = 0.0f;
}

bool Bot::CheckWaypoint(void)
{
	if (m_isSlowThink)
	{
		if ((m_isStuck && m_stuckTime > 7.0f) || (pev->origin - m_waypoint.origin).GetLengthSquared() > squaredf(512.0f) || !g_waypoint->Reachable(m_myself, m_currentWaypointIndex))
		{
			FindWaypoint();
			FindPath(m_currentWaypointIndex, m_currentGoalIndex);
			return false;
		}

		if (m_checkFall)
		{
			if (IsOnFloor())
			{
				m_checkFall = false;
				bool fixFall = false;

				const float baseDistance = (m_checkFallPoint[0] - m_checkFallPoint[1]).GetLengthSquared();
				const float nowDistance = (pev->origin - m_checkFallPoint[1]).GetLengthSquared();

				if (nowDistance > baseDistance && (nowDistance > (baseDistance * 1.25f) || nowDistance > (baseDistance + 200.0f)) && baseDistance > squaredf(80.0f) && nowDistance > squaredf(100.0f))
					fixFall = true;
				else if (pev->origin.z + 128.0f < m_checkFallPoint[1].z && pev->origin.z + 128.0f < m_checkFallPoint[0].z)
					fixFall = true;
				else if (IsValidWaypoint(m_currentWaypointIndex) && nowDistance < squaredf(32.0f) && pev->origin.z + 64.0f < m_checkFallPoint[1].z)
					fixFall = true;

				if (fixFall)
				{
					FindWaypoint();
					FindPath(m_currentWaypointIndex, m_currentGoalIndex);
				}
			}
			
		}
		else if (IsOnFloor())
		{
			m_checkFallPoint[0] = pev->origin;
			if (m_hasEnemiesNear && !FNullEnt(m_nearestEnemy))
				m_checkFallPoint[1] = m_nearestEnemy->v.origin;
			else if (IsValidWaypoint(m_currentWaypointIndex))
				m_checkFallPoint[1] = m_destOrigin;
			else
				m_checkFallPoint[1] = nullvec;
		}
		else if (!IsOnLadder() && !IsInWater())
		{
			if (!m_checkFallPoint[0].IsNull() && !m_checkFallPoint[1].IsNull())
				m_checkFall = true;
		}
	}

	return true;
}

void Bot::CheckStuck(const Vector& directionNormal, const float finterval)
{
	if (m_waypoint.flags & WAYPOINT_FALLCHECK)
	{
		ResetStuck();
		return;
	}

	if (m_prevTime < engine->GetTime())
	{
		m_isStuck = false;

		// see how far bot has moved since the previous position...
		m_movedDistance = (m_prevOrigin - pev->origin).GetLengthSquared();

		// save current position as previous
		m_prevOrigin = pev->origin;

		if (pev->flags & FL_DUCKING)
			m_prevTime = engine->GetTime() + 0.55f;
		else
			m_prevTime = engine->GetTime() + 0.25f;
	}
	else
	{
		// reset path if current waypoint is too far
		if (m_hasFriendsNear && !FNullEnt(m_nearestFriend))
		{
			m_avoid = nullptr;
			const int prio = GetPlayerPriority(m_myself);
			int friendPrio = GetPlayerPriority(m_nearestFriend);
			if (prio > friendPrio)
				m_avoid = m_nearestFriend;
			else
			{
				const Bot* bot = g_botManager->GetBot(m_nearestFriend);
				if (bot && !FNullEnt(bot->m_avoid) && m_myself != bot->m_avoid && friendPrio > GetPlayerPriority(bot->m_avoid))
					m_avoid = bot->m_avoid;
			}
			
			if (!FNullEnt(m_avoid))
			{
				const float interval = finterval * (!(pev->flags & FL_DUCKING) && pev->velocity.GetLengthSquared2D() > 200.0f ? 10.0f : 5.0f);

				// use our movement angles, try to predict where we should be next frame
				Vector right, forward;
				m_moveAngles.BuildVectors(&forward, &right, nullptr);

				Vector predict = pev->origin + forward * m_moveSpeed * interval;

				predict += right * m_strafeSpeed * interval;
				predict += pev->velocity * interval;

				const float distance = (pev->origin - m_avoid->v.origin).GetLengthSquared();
				const float movedDistance = (predict - m_avoid->v.origin).GetLengthSquared();
				const float nextFrameDistance = (pev->origin - (m_avoid->v.origin + m_avoid->v.velocity * interval)).GetLengthSquared();

				// is player that near now or in future that we need to steer away?
				if (movedDistance < squaredf(64.0f) || (distance < squaredf(72.0f) && nextFrameDistance < distance))
				{
					const Vector dir = (pev->origin - m_avoid->v.origin).Normalize2D();

					// to start strafing, we have to first figure out if the target is on the left side or right side
					if ((dir | right.Normalize2D()) > 0.0f)
						SetStrafeSpeed(directionNormal, pev->maxspeed);
					else
						SetStrafeSpeed(directionNormal, -pev->maxspeed);

					if (distance < squaredf(80.0f))
					{
						if ((dir | forward.Normalize2D()) < 0.0f)
							m_moveSpeed = -pev->maxspeed;
						else
							m_moveSpeed = pev->maxspeed;
					}

					if (m_isStuck && m_navNode.HasNext())
					{
						const Bot* bot = g_botManager->GetBot(m_avoid);
						if (bot && bot != this && bot->m_navNode.HasNext())
						{
							if (m_currentWaypointIndex == bot->m_currentWaypointIndex)
								m_navNode.Shift();
							/*else // TODO: find better solution
							{
								m_currentGoalIndex = bot->m_currentGoalIndex;
								m_currentWaypointIndex = bot->m_currentWaypointIndex;
								FindShortestPath(m_currentWaypointIndex, m_currentGoalIndex);
							}*/
						}
					}
				}
			}
		}
	}

	if (m_lastCollTime < engine->GetTime())
	{
		if (m_movedDistance < 2.0f && (m_prevSpeed > 20.0f || m_prevVelocity < squaredf(m_moveSpeed * 0.5f)))
		{
			m_prevTime = engine->GetTime();
			m_isStuck = true;

			if (m_firstCollideTime == 0.0f)
				m_firstCollideTime = engine->GetTime() + 0.2f;
		}
		else
		{
			// test if there's something ahead blocking the way
			if (!(m_waypoint.flags & WAYPOINT_LADDER) && !IsOnLadder() && CantMoveForward(directionNormal))
			{
				if (m_firstCollideTime == 0.0f)
					m_firstCollideTime = engine->GetTime() + 0.2f;
				else if (m_firstCollideTime < engine->GetTime())
					m_isStuck = true;
			}
			else
				m_firstCollideTime = 0.0f;
		}
	}

	if (m_isStuck)
	{
		m_stuckTime += finterval;
		if (m_stuckTime > 60.0f)
			Kill();

		// not yet decided what to do?
		if (m_collisionState == COSTATE_UNDECIDED)
		{
			TraceResult tr;
			uint32_t bits = 0;

			if (IsOnLadder())
				bits |= COPROBE_STRAFE;
			else if (IsInWater())
				bits |= (COPROBE_JUMP | COPROBE_STRAFE);
			else
			{
				bits |= COPROBE_STRAFE;
				if (g_numWaypoints > 1024)
				{
					if (chanceof(70))
						bits |= COPROBE_DUCK;
				}
				else if (chanceof(10))
					bits |= COPROBE_DUCK;

				if (chanceof(35))
					bits |= COPROBE_JUMP;
			}

			// collision check allowed if not flying through the air
			if (IsOnFloor() || IsOnLadder() || IsInWater())
			{
				Vector src, dest;

				int state[8];
				int i = 0;

				// first 4 entries hold the possible collision states
				state[i++] = COSTATE_STRAFELEFT;
				state[i++] = COSTATE_STRAFERIGHT;
				state[i++] = COSTATE_JUMP;
				state[i++] = COSTATE_DUCK;

				if (bits & COPROBE_STRAFE)
				{
					state[i] = 0;
					state[i + 1] = 0;

					// to start strafing, we have to first figure out if the target is on the left side or right side
					MakeVectors(m_moveAngles);

					bool dirRight = false;
					bool dirLeft = false;
					bool blockedLeft = false;
					bool blockedRight = false;

					if (((pev->origin - m_destOrigin).Normalize2D() | g_pGlobals->v_right.Normalize2D()) > 0.0f)
						dirRight = true;
					else
						dirLeft = true;

					const Vector testDir = m_moveSpeed > 0.0f ? g_pGlobals->v_forward : -g_pGlobals->v_forward;

					// now check which side is blocked
					src = pev->origin + g_pGlobals->v_right * 52.0f;
					dest = src + testDir * 52.0f;

					TraceHull(src, dest, TraceIgnore::Nothing, head_hull, m_myself, &tr);
					if (tr.flFraction < 1.0f)
						blockedRight = true;

					src = pev->origin - g_pGlobals->v_right * 52.0f;
					dest = src + testDir * 52.0f;

					TraceHull(src, dest, TraceIgnore::Nothing, head_hull, m_myself, &tr);
					if (tr.flFraction < 1.0f)
						blockedLeft = true;

					if (dirLeft)
						state[i] += 5;
					else
						state[i] -= 5;
		
					if (blockedLeft)
						state[i] -= 5;

					i++;

					if (dirRight)
						state[i] += 5;
					else
						state[i] -= 5;

					if (blockedRight)
						state[i] -= 5;
				}

				// now weight all possible states
				if (bits & COPROBE_JUMP)
				{
					state[i] = 0;
					if (CanJumpUp(directionNormal))
						state[i] += 10;

					if (m_destOrigin.z >= pev->origin.z + 18.0f)
						state[i] += 5;

					if (IsVisible(m_destOrigin, m_myself))
					{
						MakeVectors(m_moveAngles);

						src = EyePosition();
						src = src + g_pGlobals->v_right * 15.0f;

						TraceHull(src, m_destOrigin, TraceIgnore::Nothing, point_hull, m_myself, &tr);
						if (tr.flFraction >= 1.0f)
						{
							src = EyePosition();
							src = src - g_pGlobals->v_right * 15.0f;

							TraceHull(src, m_destOrigin, TraceIgnore::Nothing, point_hull, m_myself, &tr);
							if (tr.flFraction >= 1.0f)
								state[i] += 5;
						}
					}

					if (pev->flags & FL_DUCKING)
						src = pev->origin;
					else
						src = pev->origin + Vector(0.0f, 0.0f, -17.0f);

					dest = src + directionNormal * 30.0f;
					TraceHull(src, dest, TraceIgnore::Nothing, point_hull, m_myself, &tr);

					if (tr.flFraction < 1.0f)
						state[i] += 10;
				}
				else
					state[i] = 0;

				i++;

				if (bits & COPROBE_DUCK)
				{
					state[i] = 0;
					if (CanDuckUnder(directionNormal))
						state[i] += 10;

					if ((m_destOrigin.z + 36.0f <= pev->origin.z) && IsVisible(m_destOrigin, m_myself))
						state[i] += 5;
				}
				else
					state[i] = 0;

				i++;

				// weighted all possible moves, now sort them to start with most probable
				bool isSorting = false;
				int temp;

				do
				{
					isSorting = false;
					for (i = 0; i < 3; i++)
					{
						if (state[i + 3] < state[i + 3 + 1])
						{
							temp = state[i];
							state[i] = state[i + 1];
							state[i + 1] = temp;
							temp = state[i + 3];
							state[i + 3] = state[i + 4];
							state[i + 4] = temp;
							isSorting = true;
						}
					}
				} while (isSorting);

				for (i = 0; i < 3; i++)
					m_collideMoves[i] = state[i];

				m_collideTime = engine->GetTime();
				m_probeTime = engine->GetTime() + crandomfloat(0.5f, 1.5f);
				m_collisionProbeBits = bits;
				m_collisionState = COSTATE_PROBING;
				m_collStateIndex = 0;
			}
		}
		else if (m_collisionState == COSTATE_PROBING)
		{
			if (m_probeTime < engine->GetTime())
			{
				m_collStateIndex++;
				m_probeTime = engine->GetTime() + crandomfloat(0.5f, 1.5f);
				if (m_collStateIndex > 3)
					ResetCollideState();
			}
			else if (m_collStateIndex < 3)
			{
				switch (m_collideMoves[m_collStateIndex])
				{
					case COSTATE_JUMP:
					{
						if (m_isZombieBot && pev->flags & FL_DUCKING)
						{
							m_moveSpeed = pev->maxspeed;
							m_buttons = IN_FORWARD;
						}
						else
							m_buttons |= (IN_DUCK | IN_JUMP);
						
						break;
					}
					case COSTATE_DUCK:
					{
						m_buttons |= IN_DUCK;
						break;
					}
					case COSTATE_STRAFELEFT:
					{
						m_buttons |= IN_MOVELEFT;
						SetStrafeSpeedNoCost(directionNormal, -pev->maxspeed);
						break;
					}
					case COSTATE_STRAFERIGHT:
					{
						m_buttons |= IN_MOVERIGHT;
						SetStrafeSpeedNoCost(directionNormal, pev->maxspeed);
						break;
					}
				}
			}
		}
	}
	else
	{
		m_stuckTime -= finterval;
		if (m_stuckTime < 0.0f)
			m_stuckTime = 0.0f;

		// boosting improve
		if (m_isZombieBot && m_waypoint.flags & WAYPOINT_DJUMP && IsOnFloor() && ((pev->origin - m_waypoint.origin).GetLengthSquared() < squaredf(54.0f)))
			m_buttons |= IN_DUCK;
		else
		{
			if (m_probeTime + 1.0f < engine->GetTime())
				ResetCollideState(); // reset collision memory if not being stuck for 1 sec
			else
			{
				// remember to keep pressing duck if it was necessary ago
				if (m_collideMoves[m_collStateIndex] == COSTATE_DUCK && IsOnFloor() || IsInWater())
					m_buttons |= IN_DUCK;
				else if (m_collideMoves[m_collStateIndex] == COSTATE_STRAFELEFT)
					SetStrafeSpeedNoCost(directionNormal, -pev->maxspeed);
				else if (m_collideMoves[m_collStateIndex] == COSTATE_STRAFERIGHT)
					SetStrafeSpeedNoCost(directionNormal, pev->maxspeed);
			}
		}
	}

	m_prevSpeed = cabsf(m_moveSpeed);
	m_prevVelocity = pev->velocity.GetLengthSquared2D();
}

void Bot::ResetCollideState(void)
{
	m_probeTime = 0.0f;
	m_collideTime = 0.0f;

	m_collisionState = COSTATE_UNDECIDED;
	m_collStateIndex = 0;

	for (int& collide : m_collideMoves)
		collide = 0;
}

void Bot::SetWaypointOrigin(void)
{
	if (m_currentTravelFlags & PATHFLAG_JUMP || m_waypoint.flags & WAYPOINT_FALLRISK)
	{
		m_waypointOrigin = m_waypoint.origin;
		if (!IsOnLadder())
		{
			// hack, let bot turn instantly to next waypoint
			const float speed = pev->velocity.GetLength2D();
			if (speed > 10.0f)
			{
				const Vector vel = (m_waypoint.origin - pev->origin).Normalize2D();
				if (!vel.IsNull())
				{
					pev->velocity.x = vel.x * speed;
					pev->velocity.y = vel.y * speed;
				}
			}
		}

		if (m_currentTravelFlags & PATHFLAG_JUMP)
			m_jumpReady = true;
	}
	else if (m_waypoint.radius)
	{
		const float radius = static_cast<float>(m_waypoint.radius);
		if (m_navNode.HasNext())
		{
			if (m_isStuck)
			 	MakeVectors((m_waypoint.origin - pev->origin).ToAngles());
			else
			 	MakeVectors((g_waypoint->m_paths[m_navNode.Next()].origin - m_waypoint.origin).ToAngles());

			m_waypointOrigin.x = m_waypoint.origin.x + g_pGlobals->v_forward.x * radius;
			m_waypointOrigin.y = m_waypoint.origin.y + g_pGlobals->v_forward.y * radius;
			m_waypointOrigin.z = m_waypoint.origin.z;
		}
		else
		{
			m_waypointOrigin.x = m_waypoint.origin.x + crandomfloat(-radius, radius);
			m_waypointOrigin.y = m_waypoint.origin.y + crandomfloat(-radius, radius);
			m_waypointOrigin.z = m_waypoint.origin.z;
		}
	}
	else
		m_waypointOrigin = m_waypoint.origin;
}

void Bot::ChangeWptIndex(const int waypointIndex)
{
	m_currentWaypointIndex = waypointIndex;
	if (!IsValidWaypoint(waypointIndex))
		return;

	if (m_prevWptIndex[0] != m_currentWaypointIndex)
	{
		m_prevWptIndex[0] = m_currentWaypointIndex;
		m_prevWptIndex[1] = m_prevWptIndex[0];
		m_prevWptIndex[2] = m_prevWptIndex[1];
		m_prevWptIndex[3] = m_prevWptIndex[2];
	}

	m_waypoint = g_waypoint->m_paths[waypointIndex];
	SetWaypointOrigin();
	const float speed = pev->velocity.GetLength();
	if (speed > 10.0f)
		m_waypointTime = engine->GetTime() + cclampf((pev->origin - m_destOrigin).GetLength() / speed, 3.0f, 12.0f);
	else
		m_waypointTime = engine->GetTime() + 12.0f;
}

// checks if bot is blocked in his movement direction (excluding doors)
bool Bot::CantMoveForward(const Vector& normal)
{
	// first do a trace from the bot's eyes forward...
	const Vector eyePosition = EyePosition();
	Vector src = eyePosition;
	Vector forward = src + normal * 24.0f;

	MakeVectors(Vector(0.0f, pev->angles.y, 0.0f));
	TraceResult tr;

	// trace from the bot's eyes straight forward...
	TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f)
	{
		if (cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) == 0)
			return false;

		return true; // bot's head will hit something
	}

	// bot's head is clear, check at shoulder level...
	// trace from the bot's shoulder left diagonal forward to the right shoulder...
	src = eyePosition + Vector(0.0f, 0.0f, -16.0f) - g_pGlobals->v_right * 16.0f;
	forward = eyePosition + Vector(0.0f, 0.0f, -16.0f) + g_pGlobals->v_right * 16.0f + normal * 24.0f;

	TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) != 0)
		return true; // bot's body will hit something

	// bot's head is clear, check at shoulder level...
	// trace from the bot's shoulder right diagonal forward to the left shoulder...
	src = eyePosition + Vector(0.0f, 0.0f, -16.0f) + g_pGlobals->v_right * 16.0f;
	forward = eyePosition + Vector(0.0f, 0.0f, -16.0f) - g_pGlobals->v_right * 16.0f + normal * 24.0f;

	TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) != 0)
		return true; // bot's body will hit something

	// now check below waist
	if ((pev->flags & FL_DUCKING))
	{
		src = pev->origin + Vector(0.0f, 0.0f, -19.0f + 19.0f);
		forward = src + Vector(0.0f, 0.0f, 10.0f) + normal * 24.0f;

		TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

		// check if the trace hit something...
		if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) != 0)
			return true; // bot's body will hit something

		src = pev->origin;
		forward = src + normal * 24.0f;

		TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

		// check if the trace hit something...
		if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) != 0)
			return true; // bot's body will hit something
	}
	else
	{
		// trace from the left waist to the right forward waist pos
		src = pev->origin + Vector(0.0f, 0.0f, -17.0f) - g_pGlobals->v_right * 16.0f;
		forward = pev->origin + Vector(0.0f, 0.0f, -17.0f) + g_pGlobals->v_right * 16.0f + normal * 24.0f;

		// trace from the bot's waist straight forward...
		TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

		// check if the trace hit something...
		if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) != 0)
			return true; // bot's body will hit something

		// trace from the left waist to the right forward waist pos
		src = pev->origin + Vector(0.0f, 0.0f, -17.0f) + g_pGlobals->v_right * 16.0f;
		forward = pev->origin + Vector(0.0f, 0.0f, -17.0f) - g_pGlobals->v_right * 16.0f + normal * 24.0f;

		TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

		// check if the trace hit something...
		if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) != 0)
			return true; // bot's body will hit something
	}

	return false; // bot can move forward, return false
}

bool Bot::CanJumpUp(const Vector& normal)
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
	TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

	if (tr.flFraction < 1.0f)
		goto CheckDuckJump;
	else
	{
		// now trace from jump height upward to check for obstructions...
		src = dest;
		dest.z = dest.z + 37.0f;

		TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);
		if (tr.flFraction < 1.0f)
			return false;
	}

	// now check same height to one side of the bot...
	src = pev->origin + g_pGlobals->v_right * 16.0f + Vector(0.0f, 0.0f, -36.0f + 45.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		goto CheckDuckJump;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now check same height on the other side of the bot...
	src = pev->origin + (-g_pGlobals->v_right * 16.0f) + Vector(0.0f, 0.0f, -36.0f + 45.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		goto CheckDuckJump;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

	// if trace hit something, return false
	return tr.flFraction > 1.0f;

	// here we check if a duck jump would work...
CheckDuckJump:
	// use center of the body first... maximum duck jump height is 62, so check one unit above that (63)
	src = pev->origin + Vector(0.0f, 0.0f, -36.0f + 63.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

	if (tr.flFraction < 1.0f)
		return false;
	else
	{
		// now trace from jump height upward to check for obstructions...
		src = dest;
		dest.z = dest.z + 37.0f;

		TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

		// if trace hit something, check duckjump
		if (tr.flFraction < 1.0f)
			return false;
	}

	// now check same height to one side of the bot...
	src = pev->origin + g_pGlobals->v_right * 16.0f + Vector(0.0f, 0.0f, -36.0f + 63.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now check same height on the other side of the bot...
	src = pev->origin + (-g_pGlobals->v_right * 16.0f) + Vector(0.0f, 0.0f, -36.0f + 63.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

	// if trace hit something, return false
	return tr.flFraction > 1.0f;
}

// this function check if bot can duck under obstacle
bool Bot::CanDuckUnder(const Vector& normal)
{
	TraceResult tr{};
	Vector baseHeight;

	// use center of the body first...
	if (pev->flags & FL_DUCKING)
		baseHeight = pev->origin + Vector(0.0f, 0.0f, -17.0f);
	else
		baseHeight = pev->origin;

	Vector src = baseHeight;
	Vector dest = src + normal * 32.0f;

	// trace a line forward at duck height...
	TraceLine(src, dest, TraceIgnore::Nothing, pev->pContainingEntity, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	MakeVectors(Vector(0.0f, pev->angles.y, 0.0f));

	// now check same height to one side of the bot...
	src = baseHeight + g_pGlobals->v_right * 16.0f;
	dest = src + normal * 32.0f;

	// trace a line forward at duck height...
	TraceLine(src, dest, TraceIgnore::Nothing, pev->pContainingEntity, &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now check same height on the other side of the bot...
	src = baseHeight + (-g_pGlobals->v_right * 16.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at duck height...
	TraceLine(src, dest, TraceIgnore::Nothing, pev->pContainingEntity, &tr);

	// if trace hit something, return false
	return tr.flFraction > 1.0f;
}

bool Bot::CheckWallOnForward(void)
{
	TraceResult tr;
	MakeVectors(pev->angles);

	// do a trace to the forward...
	TraceLine(pev->origin, pev->origin + g_pGlobals->v_forward * 54.0f, TraceIgnore::Nothing, m_myself, &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f)
		return true;
	else
	{
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, TraceIgnore::Nothing, m_myself, &tr);

		// we don't want to fall
		if (tr.flFraction >= 1.0f)
			return true;
	}

	return false;
}

bool Bot::CheckWallOnBehind(void)
{
	TraceResult tr;
	MakeVectors(pev->angles);

	// do a trace to the behind...
	TraceLine(pev->origin, pev->origin - g_pGlobals->v_forward * 54.0f, TraceIgnore::Nothing, m_myself, &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f)
		return true;
	else
	{
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, TraceIgnore::Nothing, m_myself, &tr);

		// we don't want to fall
		if (tr.flFraction >= 1.0f)
			return true;
	}

	return false;
}

bool Bot::CheckWallOnLeft(void)
{
	TraceResult tr;
	MakeVectors(pev->angles);

	// do a trace to the left...
	TraceLine(pev->origin, pev->origin - g_pGlobals->v_right * 54.0f, TraceIgnore::Nothing, m_myself, &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f)
		return true;
	else
	{
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, TraceIgnore::Nothing, m_myself, &tr);

		// we don't want to fall
		if (tr.flFraction >= 1.0f)
			return true;
	}

	return false;
}

bool Bot::CheckWallOnRight(void)
{
	TraceResult tr;
	MakeVectors(pev->angles);

	// do a trace to the right...
	TraceLine(pev->origin, pev->origin + g_pGlobals->v_right * 54.0f, TraceIgnore::Nothing, m_myself, &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f)
		return true;
	else
	{
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, TraceIgnore::Nothing, m_myself, &tr);

		// we don't want to fall
		if (tr.flFraction >= 1.0f)
			return true;
	}

	return false;
}

// this function eturns if given location would hurt Bot with falling damage
bool Bot::IsDeadlyDrop(const Vector& targetOriginPos)
{
	int tries = 0;
	const Vector botPos = pev->origin;
	TraceResult tr;

	const Vector move((targetOriginPos - botPos).ToYaw(), 0.0f, 0.0f);
	MakeVectors(move);

	const Vector direction = (targetOriginPos - botPos).Normalize(); // 1 unit long
	Vector check = botPos;
	Vector down = botPos;

	down.z = down.z - 1000.0f; // straight down 1000 units

	TraceHull(check, down, TraceIgnore::Monsters, head_hull, m_myself, &tr);
	if (tr.flFraction > 0.036f) // We're not on ground anymore?
		tr.flFraction = 0.036f;

	float height;
	float lastHeight = tr.flFraction * 1000.0f; // height from ground

	float distance = (targetOriginPos - check).GetLengthSquared();  // distance from goal
	while (distance > squaredf(16.0f) && tries < 1000)
	{
		check = check + direction * 16.0f; // move 16 units closer to the goal...

		down = check;
		down.z = down.z - 1000.0f; // straight down 1000 units

		TraceHull(check, down, TraceIgnore::Monsters, head_hull, m_myself, &tr);
		if (tr.fStartSolid) // wall blocking?
			return false;

		height = tr.flFraction * 1000.0f; // height from ground
		if (lastHeight < height - 100.0f) // drops more than 100 units?
			return true;

		lastHeight = height;
		distance = (targetOriginPos - check).GetLengthSquared(); // distance from goal
		tries++;
	}

	return false;
}

void Bot::LookAt(const Vector& origin, const Vector& velocity)
{
	m_updateLooking = true;
	m_lookAt = origin;
	m_lookVelocity = velocity;
}

void Bot::SetStrafeSpeed(const Vector& moveDir, const float strafeSpeed)
{
	MakeVectors(pev->angles);
	if (CheckWallOnBehind())
	{
		if (CheckWallOnRight())
			m_tempstrafeSpeed = -strafeSpeed;
		else if (CheckWallOnLeft())
			m_tempstrafeSpeed = strafeSpeed;
		
		m_strafeSpeed = m_tempstrafeSpeed;
		if ((m_isStuck || pev->speed >= pev->maxspeed) && IsOnFloor() && !IsOnLadder() && m_jumpTime + 5.0f < engine->GetTime() && CanJumpUp(moveDir))
			m_buttons |= IN_JUMP;
	}
	else if (((moveDir - pev->origin).Normalize2D() | g_pGlobals->v_forward.SkipZ()) > 0.0f && !CheckWallOnRight())
		m_strafeSpeed = strafeSpeed;
	else if (!CheckWallOnLeft())
		m_strafeSpeed = -strafeSpeed;
}

void Bot::SetStrafeSpeedNoCost(const Vector& moveDir, const float strafeSpeed)
{
	MakeVectors(pev->angles);
	if (((moveDir - pev->origin).Normalize2D() | g_pGlobals->v_forward.SkipZ()) > 0.0f)
		m_strafeSpeed = strafeSpeed;
	else
		m_strafeSpeed = -strafeSpeed;
}

bool Bot::IsWaypointOccupied(const int index)
{
	if (ebot_has_semiclip.GetBool())
		return false;

	if (pev->solid == SOLID_NOT)
		return false;

	Path* pointer;
	Bot* bot;
	for (const Clients& client : g_clients)
	{
		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == m_myself)
			continue;

		bot = g_botManager->GetBot(client.index);
		if (bot && bot->m_isAlive)
		{
			if (bot->m_currentWaypointIndex == index)
				return true;

			if (bot->m_prevWptIndex[0] == index)
				return true;

			if (!bot->m_navNode.IsEmpty() && bot->m_navNode.Last() == index)
				return true;
		}
		else
		{
			pointer = g_waypoint->GetPath(index);
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
	float distance;

	// find the nearest button which can open our target
	while (!FNullEnt(searchEntity = FIND_ENTITY_BY_TARGET(searchEntity, className)))
	{
		distance = (pev->origin - GetEntityOrigin(searchEntity)).GetLengthSquared();
		if (distance < nearestDistance)
		{
			nearestDistance = distance;
			foundEntity = searchEntity;
		}
	}

	return foundEntity;
}