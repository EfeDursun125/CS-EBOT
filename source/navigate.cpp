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
constexpr int16_t pMax = static_cast<int16_t>(Const_MaxPathIndex);

ConVar ebot_zombies_as_path_cost("ebot_zombie_count_as_path_cost", "1");
ConVar ebot_has_semiclip("ebot_has_semiclip", "0");
ConVar ebot_breakable_health_limit("ebot_breakable_health_limit", "3000.0");
ConVar ebot_force_shortest_path("ebot_force_shortest_path", "0");
ConVar ebot_pathfinder_seed_min("ebot_pathfinder_seed_min", "0.9");
ConVar ebot_pathfinder_seed_max("ebot_pathfinder_seed_max", "1.1");
ConVar ebot_helicopter_width("ebot_helicopter_width", "54.0");
ConVar ebot_use_pathfinding_for_avoid("ebot_use_pathfinding_for_avoid", "1");

int16_t Bot::FindGoalZombie(void)
{
	if (g_waypoint->m_terrorPoints.IsEmpty())
	{
		m_currentGoalIndex =static_cast<int16_t>(crandomint(0, g_numWaypoints - 1));
		return m_currentGoalIndex;
	}

	if (crandomint(1, 3) == 1)
	{
		m_currentGoalIndex = static_cast<int16_t>(crandomint(0, g_numWaypoints - 1));
		return m_currentGoalIndex;
	}

	m_currentGoalIndex = g_waypoint->m_terrorPoints.Random();
	return m_currentGoalIndex;
}

inline float GetWaypointDistance(const int16_t &start, const int16_t &goal)
{
	if (!IsValidWaypoint(start) || !IsValidWaypoint(goal))
		return 999999999.0f;

	if (g_isMatrixReady)
		return static_cast<float>(*(g_waypoint->m_distMatrix.Get() + (start * g_numWaypoints) + goal));

	return GetVectorDistanceSSE(g_waypoint->m_paths[start].origin, g_waypoint->m_paths[goal].origin);
}

int16_t Bot::FindGoalHuman(void)
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
						int16_t i, temp;
						index = -1;
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

	m_currentGoalIndex = static_cast<int16_t>(crandomint(0, g_numWaypoints - 1));
	return m_currentGoalIndex;
}

void Bot::MoveTo(const Vector &targetPosition, const bool checkStuck)
{
	const Vector directionOld = (targetPosition + pev->velocity * -m_frameInterval) - (pev->origin + pev->velocity * m_frameInterval);
	m_moveAngles = directionOld.ToAngles();
	m_moveAngles.x = -m_moveAngles.x; // invert for engine
	m_moveAngles.ClampAngles();
	m_moveSpeed = pev->maxspeed;

	if (checkStuck)
		CheckStuck(directionOld.Normalize2D(), m_frameInterval);
}

void Bot::MoveOut(const Vector &targetPosition, const bool checkStuck)
{
	const Vector directionOld = targetPosition - pev->origin;
	SetStrafeSpeed(directionOld.Normalize2D(), pev->maxspeed);
	m_moveAngles = directionOld.ToAngles();
	m_moveAngles.x = -m_moveAngles.x; // invert for engine
	m_moveAngles.ClampAngles();
	m_moveSpeed = -pev->maxspeed;

	if (checkStuck)
		CheckStuck((pev->origin - targetPosition).Normalize2D(), m_frameInterval);
}

void Bot::FollowPath(void)
{
	m_navNode.Start();
}

void Bot::DoWaypointNav(void)
{
	m_destOrigin = m_waypointOrigin;

	// check if there is a breakable entity blocking our jump path
	if (m_currentTravelFlags & PATHFLAG_JUMP || m_waypoint.flags & WAYPOINT_JUMP)
	{
		TraceResult tr;
		TraceLine(EyePosition(), m_destOrigin, TraceIgnore::Nothing, GetEntity(), &tr);
		if (tr.flFraction < 1.0f && !FNullEnt(tr.pHit))
		{
			edict_t *ent = tr.pHit;
			if (ent->v.takedamage != DAMAGE_NO && ent->v.health > 0.0f && ent->v.health < ebot_breakable_health_limit.GetFloat())
			{
				m_breakableEntity = ent;
				m_breakableOrigin = GetBoxOrigin(m_breakableEntity);
				SetProcess(Process::DestroyBreakable, "trying to destroy a blocking breakable on jump path", false, engine->GetTime() + 60.0f);
				m_moveSpeed = 0.0f;
				m_strafeSpeed = 0.0f;
				return;
			}
		}
	}

	// check if this is a consecutive/mid-air jump (double-jump / air-dash)
	const bool isConsecutiveJump = m_jumpReady && m_waitForLanding && !IsOnFloor() && !IsOnLadder() && !IsInWater();

	if ((m_jumpReady && !m_waitForLanding && (IsOnFloor() || IsOnLadder())) || isConsecutiveJump)
	{
			const Vector myOrigin = GetBottomOrigin(GetEntity());
			Vector waypointOrigin = m_waypoint.origin; // directly jump to waypoint, ignore risk of fall

			if (m_waypoint.flags & WAYPOINT_CROUCH)
				waypointOrigin.z -= 18.0f;
			else
				waypointOrigin.z -= 36.0f;

			// calculate the displacement vector
			const Vector displacement = waypointOrigin - myOrigin;
			const float distance2D = displacement.GetLength2D();

			// determine actual gravity acting on the bot (GoldSrc default is 800)
			const float actualGravity = engine->GetGravity() * (pev->gravity != 0.0f ? pev->gravity : 1.0f);

			// dynamic ceiling detection using TraceHull
			const float headOffset = (pev->flags & FL_DUCKING) ? 18.0f : 36.0f;
			TraceResult trCeiling;
			const Vector headTop = pev->origin + Vector(0.0f, 0.0f, headOffset);
			TraceHull(headTop, headTop + Vector(0.0f, 0.0f, 120.0f), TraceIgnore::Nothing, head_hull, GetEntity(), &trCeiling);

			float maxRiseHeight = 120.0f;
			if (trCeiling.flFraction < 1.0f)
			{
				const float ceilingZ = headTop.z + 120.0f * trCeiling.flFraction;
				maxRiseHeight = ceilingZ - headTop.z - 5.0f; // 5 units safety buffer below ceiling
			}

			if (maxRiseHeight < 10.0f)
				maxRiseHeight = 10.0f;

			// dynamic obstacle clearance detection using TraceLine
			float requiredObstacleVz = 0.0f;
			TraceResult trObstacle;
			TraceLine(pev->origin, waypointOrigin, TraceIgnore::Nothing, GetEntity(), &trObstacle);

			if (trObstacle.flFraction < 1.0f && !FNullEnt(trObstacle.pHit))
			{
				TraceResult trObstacleTop;
				const Vector hitPos = trObstacle.vecEndPos;
				TraceLine(hitPos + Vector(0.0f, 0.0f, 150.0f), hitPos, TraceIgnore::Nothing, GetEntity(), &trObstacleTop);
				if (trObstacleTop.flFraction < 1.0f)
				{
					const float requiredPeakHeight = (trObstacleTop.vecEndPos.z - myOrigin.z) + 10.0f;
					if (requiredPeakHeight > 0.0f)
						requiredObstacleVz = csqrtf(2.0f * actualGravity * requiredPeakHeight);
				}
			}

			// calculate the minimum vertical velocity required to reach target height
			float minVz = 0.0f;
			if (displacement.z > 0.0f)
				minVz = csqrtf(2.0f * actualGravity * displacement.z);

			// add clearance buffer and clamp to minimum standard jump velocity
			float Vz = minVz + 50.0f;
			if (Vz < 268.0f)
				Vz = 268.0f;

			// clamp to minimum required vertical velocity to clear obstacle
			if (Vz < requiredObstacleVz)
				Vz = requiredObstacleVz;

			// clamp vertical velocity based on ceiling height to prevent head collisions
			if (actualGravity > 10.0f)
			{
				const float maxSafeVz = csqrtf(2.0f * actualGravity * maxRiseHeight);
				if (Vz > maxSafeVz)
				{
					Vz = maxSafeVz;

					// prioritize obstacle clearance if conflict exists, but at least reach target height
					if (Vz < minVz)
						Vz = minVz;
				}
			}

			// solve quadratic formula for exact time of flight: T = (Vz + sqrt(Vz^2 - 2 * g * dz)) / g
			// the term inside sqrt is guaranteed to be >= 0 because Vz >= sqrt(2 * g * dz)
			float timeOfFlight = 0.5f;
			const float termInsideSqrt = (Vz * Vz) - (2.0f * actualGravity * displacement.z);
			if (actualGravity > 10.0f)
				timeOfFlight = (Vz + csqrtf(termInsideSqrt >= 0.0f ? termInsideSqrt : 0.0f)) / actualGravity;

			if (timeOfFlight < 0.1f)
				timeOfFlight = 0.1f;

			// apply horizontal and vertical velocities
			if (distance2D > 0.0f)
			{
				const Vector dir2D = displacement.Normalize2D();
				pev->velocity.x = dir2D.x * (distance2D / timeOfFlight);
				pev->velocity.y = dir2D.y * (distance2D / timeOfFlight);
			}
			else
			{
				pev->velocity.x = 0.0f;
				pev->velocity.y = 0.0f;
			}

			pev->velocity.z = Vz;

			m_duckTime = engine->GetTime() + 1.25f;
			m_buttons |= (IN_DUCK | IN_JUMP);
			m_jumpReady = false;
			m_waitForLanding = true;
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
		TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), TraceIgnore::Nothing, GetEntity(), &tr);
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
		TraceLine(origin, m_waypoint.origin - Vector(0.0f, 0.0f, 60.0f), TraceIgnore::Nothing, GetEntity(), &tr);
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
			if (IsVisible(center, GetEntity()))
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
					TraceLine(center, right, TraceIgnore::Nothing, GetEntity(), &tr);
					if (tr.flFraction > 0.99f)
					{
						MoveTo(right, false);
						return;
					}
				}

				TraceLine(center, left, TraceIgnore::Nothing, GetEntity(), &tr);
				if (tr.flFraction > 0.99f)
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
			TraceLine(origin + Vector(0.0f, 0.0f, 36.0f), origin - Vector(0.0f, 0.0f, 36.0f), TraceIgnore::Nothing, GetEntity(), &tr);
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
					const float speedNextFrame = speed * 1.5f;
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
		TraceLine(pev->origin, m_waypoint.origin, TraceIgnore::Monsters, GetEntity(), &tr);
		if (!FNullEnt(tr.pHit) && !cstrncmp(STRING(tr.pHit->v.classname), "func_door", 9))
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
						MDLL_Use(tr.pHit, GetEntity()); // also 'use' the door randomly
				}
			}

			const float time = engine->GetTime();
			edict_t *button = FindNearestButton(STRING(tr.pHit->v.targetname));
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
		// special detection if someone is using the ladder (to prevent to have
		// bots-towers on ladders)
		TraceResult tr;
		bool foundGround = false;
		int16_t previousNode = 0;
		extern ConVar ebot_ignore_enemies;
		for (const auto &client : g_clients)
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
					TraceLine(EyePosition(), client.ent->v.origin, TraceIgnore::Monsters, GetEntity(), &tr);

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
					TraceHull(EyePosition(), m_destOrigin, TraceIgnore::Monsters, (pev->flags & FL_DUCKING) ? head_hull : human_hull, GetEntity(), &tr);

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

		const float dist = (pev->origin - m_waypointOrigin).GetLengthSquared();
		if (m_waypointOrigin.z >= (pev->origin.z + 16.0f))
			m_waypointOrigin = g_waypoint->GetPath(m_currentWaypointIndex)->origin + Vector(0, 0, 16);
		else if (m_waypointOrigin.z < pev->origin.z + 16.0f && !IsOnLadder() && IsOnFloor() && !(pev->flags & FL_DUCKING))
		{
			m_moveSpeed = csqrtf(dist);
			if (m_moveSpeed < 150.0f)
				m_moveSpeed = 150.0f;
			else if (m_moveSpeed > pev->maxspeed)
				m_moveSpeed = pev->maxspeed;
		}

		if (dist < squaredf(24.0f))
			next = true;
	}
	else
	{
		if (m_waypoint.radius < 50 || (m_currentTravelFlags & PATHFLAG_JUMP))
		{
			if (pev->flags & FL_DUCKING)
			{
				if ((pev->origin - m_destOrigin).GetLengthSquared2D() < squaredf(24.0f))
					next = true;
			}
			else
			{
				constexpr float MIN_RADIUS = 4.0f;
				const float checkRadiusSqr = squaredf(cmaxf(static_cast<float>(m_waypoint.radius), MIN_RADIUS));
				const Vector vel = pev->velocity.SkipZ();
				const float speed2 = vel.GetLengthSquared2D();
				if (speed2 < 1e-4f)
				{
					if ((pev->origin - m_destOrigin).GetLengthSquared2D() < checkRadiusSqr)
						next = true;
				}
				else
				{
					const Vector origin = pev->origin;
					const Vector toTarget = m_destOrigin - origin;
					float t = (toTarget.x * vel.x + toTarget.y * vel.y) / speed2;
					if (t < m_pathInterval)
						t = m_pathInterval;
					else
					{
						const float lookahead = cmaxf(m_pathInterval * 2.0f, 0.12f) * ((m_waypoint.radius < 5 || (m_currentTravelFlags & PATHFLAG_JUMP)) ? 1.5f : 1.0f);
						if (t > lookahead)
							t = lookahead;
					}

					const Vector closest = origin + vel * t;
					if ((closest - m_destOrigin).GetLengthSquared2D() < checkRadiusSqr)
						next = true;
					else if ((origin - m_destOrigin).GetLengthSquared2D() < squaredf(cmaxf(static_cast<float>(m_waypoint.radius), 4.0f)))
						next = true;
				}
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
			for (i = 0; i < pMax; i++)
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
	TraceLine(pev->origin, m_waypoint.origin, TraceIgnore::Everything, GetEntity(), &tr);
	if (tr.flFraction < 1.0f && (m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor || m_liftState == LiftState::LookingButtonOutside) && !FNullEnt(tr.pHit) && !cstrcmp(STRING(tr.pHit->v.classname), "func_door") && pev->groundentity != tr.pHit)
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
		TraceLine(m_waypoint.origin, m_waypoint.origin + Vector(0.0f, 0.0f, -54.0f), TraceIgnore::Everything, GetEntity(), &tr);

		// if trace result shows us that it is a lift
		if (!liftClosedDoorExists && !FNullEnt(tr.pHit) && (!cstrcmp(STRING(tr.pHit->v.classname), "func_door") || !cstrcmp(STRING(tr.pHit->v.classname), "func_plat") || !cstrcmp(STRING(tr.pHit->v.classname), "func_train")))
		{
			if ((m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor || m_liftState == LiftState::LookingButtonOutside) && Math::FltZero(tr.pHit->v.velocity.z))
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
				const int16_t nextWaypoint = m_navNode.Next();
				if (IsValidWaypoint(nextWaypoint))
				{
					const Path *pointer = g_waypoint->GetPath(nextWaypoint);
					if (pointer->flags & WAYPOINT_LIFT)
					{
						TraceLine(m_waypoint.origin, pointer->origin, TraceIgnore::Everything, GetEntity(), &tr);
						if (!FNullEnt(tr.pHit) && (!cstrcmp(STRING(tr.pHit->v.classname), "func_door") || !cstrcmp(STRING(tr.pHit->v.classname), "func_plat") || !cstrcmp(STRING(tr.pHit->v.classname), "func_train")))
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

			// need to wait our following teammate?
			bool needWaitForTeammate = false;

			// if some bot is following a bot going into lift - he should take the
			// same lift to go
			for (Bot *const &bot : g_botManager->m_bots)
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
		// need to wait our following teammate?
		bool needWaitForTeammate = false;

		for (Bot *const &bot : g_botManager->m_bots)
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
		edict_t *button = FindNearestButton(STRING(m_liftEntity->v.targetname));

		// got a valid button entity?
		if (!FNullEnt(button) && pev->groundentity == m_liftEntity && m_buttonPushTime + 1.0f < time && Math::FltZero(m_liftEntity->v.velocity.z) && IsOnFloor())
		{
			m_buttonEntity = button;
			SetProcess(Process::UseButton, "trying to use a button", false, time + 10.0f);
		}
	}

	// is lift activated and bot is standing on it and lift is moving?
	if (m_liftState == LiftState::LookingButtonInside || m_liftState == LiftState::EnteringIn || m_liftState == LiftState::WaitingForTeammates || m_liftState == LiftState::WaitingFor)
	{
		if (pev->groundentity == m_liftEntity && !Math::FltZero(m_liftEntity->v.velocity.z) && IsOnFloor() && (g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LIFT))
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
			edict_t *button = FindNearestButton(STRING(m_liftEntity->v.targetname));

			// if we got a valid button entity
			if (!FNullEnt(button))
			{
				// lift is already used?
				bool liftUsed = false;

				// iterate though clients, and find if lift already used
				for (const auto &client : g_clients)
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

	if (m_liftUsageTime < engine->GetTime() && !Math::FltZero(m_liftUsageTime))
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
};
CArray<HeapNode> heapNode{2};

class PriorityQueue
{
public:
	PriorityQueue(void);
	~PriorityQueue(void);
	inline bool IsEmpty(void) const { return m_size < 1; }
	inline int16_t Size(void) const { return m_size; }
	inline void InsertLowest(const int16_t value, const float priority);
	inline int16_t RemoveLowest(void);
	inline bool Setup(void)
	{
		if (heapNode.NotAllocated())
		{
			heapNode.Resize(g_numWaypoints, true);
			return false;
		}

		if (heapNode.Capacity() != g_numWaypoints)
		{
			heapNode.Resize(g_numWaypoints, true);
			return false;
		}

		m_size = 0;
		return true;
	}

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
	heapNode[m_size].priority = priority;
	heapNode[m_size].id = value;

	int16_t child;
	int16_t parent;
	HeapNode temp;

	child = ++m_size - 1;
	while (child > 0)
	{
		parent = (child - 1) / 2;
		if (heapNode[parent].priority < heapNode[child].priority)
			break;

		temp = heapNode[child];
		heapNode[child] = heapNode[parent];
		heapNode[parent] = temp;
		child = parent;
	}
}

// removes the smallest item from the priority queue
int16_t PriorityQueue::RemoveLowest(void)
{
	const int16_t retID = heapNode[0].id;

	m_size--;
	heapNode[0] = heapNode[m_size];

	int16_t rightChild;
	int16_t parent = 0;
	int16_t child = (2 * parent) + 1;
	HeapNode ref = heapNode[parent];
	while (child < m_size)
	{
		rightChild = (2 * parent) + 2;
		if (rightChild < m_size)
		{
			if (heapNode[rightChild].priority < heapNode[child].priority)
				child = rightChild;
		}

		if (ref.priority < heapNode[child].priority)
			break;

		heapNode[parent] = heapNode[child];
		parent = child;
		child = (2 * parent) + 1;
	}

	heapNode[parent] = ref;
	return retID;
}

inline const float HF_DistanceSquared(const int16_t &start, const int16_t &goal)
{
	return (g_waypoint->m_paths[start].origin - g_waypoint->m_paths[goal].origin).GetLengthSquared();
}

inline const float HF_Distance(const int16_t &start, const int16_t &goal)
{
	return GetVectorDistanceSSE(g_waypoint->m_paths[start].origin, g_waypoint->m_paths[goal].origin);
}

inline const float HF_Matrix(const int16_t &start, const int16_t &goal)
{
	return static_cast<float>(*(g_waypoint->m_distMatrix.Get() + (start * g_numWaypoints) + goal));
}

inline const float HF_Auto(const int16_t &start, const int16_t &goal)
{
	if (g_isMatrixReady)
		return HF_Matrix(start, goal);

	return HF_Distance(start, goal);
}

inline const float GF_CostHuman(const int16_t &index, const int16_t &parent, const uint32_t &parentFlags, const int8_t &team, const float &gravity, const bool &isZombie)
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

	Path pathCache = g_waypoint->m_paths[parent];
	if (parentFlags & WAYPOINT_ONLYONE)
	{
		for (const auto &client : g_clients)
		{
			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			if ((client.origin - pathCache.origin).GetLengthSquared() < squaredi(static_cast<int>(pathCache.radius) + 64))
				return 65355.0f;
		}
	}

	float distance;
	float distanceSq;
	float dangerScore = 0.0f;
	int zombieCountInArea = 0;
	const float scanRadius = 1200.0f;

	for (const auto &client : g_clients)
	{
		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team == client.team)
			continue;

		distanceSq = (client.ent->v.origin - pathCache.origin).GetLengthSquared();
		if (distanceSq < (scanRadius * scanRadius))
		{
			zombieCountInArea++;

			distance = csqrtf(distanceSq);
			if (distance < 1.0f)
				distance = 1.0f;

			dangerScore += (scanRadius - distance) * 1.5f;
		}
	}

	if (zombieCountInArea)
	{
		float finalCost = HF_Auto(index, parent) + (dangerScore * (1.0f + (static_cast<float>(zombieCountInArea * zombieCountInArea) * 0.1f)));
		if (finalCost > 65000.0f)
			return 65000.0f;

		return finalCost;
	}

	return HF_Auto(index, parent);
}

inline const float GF_CostCareful(const int16_t &index, const int16_t &parent, const uint32_t &parentFlags, const int8_t &team, const float &gravity, const bool &isZombie)
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
		Path pathCache = g_waypoint->m_paths[parent];
		for (const auto &client : g_clients)
		{
			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			if ((client.origin - pathCache.origin).GetLengthSquared() < squaredi(static_cast<int>(pathCache.radius) + 64))
				return 65355.0f;
		}
	}

	if (isZombie)
	{
		Path pathCache = g_waypoint->m_paths[parent];
		float zombieCostPenalty = 0.0f;

		for (Bot *const &bot : g_botManager->m_bots)
		{
			if (bot == nullptr || !bot->m_isAlive || bot->m_team != team)
				continue;

			if (bot->m_isZombieBot && bot->m_personality != Personality::Careful)
			{
				if ((bot->pev->origin - pathCache.origin).GetLengthSquared() < squaredf(400.0f))
					zombieCostPenalty += 600.0f;
			}
		}

		if (parentFlags & WAYPOINT_DJUMP)
		{
			int8_t countCache = 0;
			for (const auto &client : g_clients)
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

			return (HF_Auto(index, parent) / static_cast<float>(countCache)) + zombieCostPenalty;
		}

		if (zombieCostPenalty > 0.0f)
			return HF_Auto(index, parent) + zombieCostPenalty;
	}

	return HF_Auto(index, parent);
}

inline const float GF_CostNormal(const int16_t &index, const int16_t &parent, const uint32_t &parentFlags, const int8_t &team, const float &gravity, const bool &isZombie)
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
		Path pathCache = g_waypoint->m_paths[parent];
		for (const auto &client : g_clients)
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
			Path pathCache = g_waypoint->m_paths[parent];
			int8_t countCache = 0;
			for (const auto &client : g_clients)
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

inline const float GF_CostRusher(const int16_t &index, const int16_t &parent, const uint32_t &parentFlags, const int8_t &team, const float &gravity, const bool &isZombie)
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
		Path pathCache = g_waypoint->m_paths[parent];
		for (const auto &client : g_clients)
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

struct AStar
{
	float g{0.0f};
	float f{0.0f};
	int16_t parent{-1};
	bool is_closed{false};
};
CArray<AStar> waypoints{2};

// this function finds a path from srcIndex to destIndex
void Bot::FindPath(int16_t &srcIndex, int16_t &destIndex)
{
	if (!IsValidWaypoint(srcIndex))
		srcIndex = FindWaypoint();

	if (!IsValidWaypoint(srcIndex) || !IsValidWaypoint(destIndex))
	{
		m_navNode.Clear();
		return;
	}

	if (ebot_force_shortest_path.GetBool() || g_numWaypoints > 2048)
	{
		FindShortestPath(srcIndex, destIndex);
		return;
	}

	if (srcIndex == destIndex)
		return;

	if (waypoints.NotAllocated())
	{
		waypoints.Resize(g_numWaypoints, true);
		return;
	}

	if (waypoints.Capacity() != g_numWaypoints)
	{
		waypoints.Resize(g_numWaypoints, true);
		return;
	}

	PriorityQueue openList;
	if (!openList.Setup())
		return;

	const float (*hcalc)(const int16_t &, const int16_t &) = nullptr;
	const float (*gcalc)(const int16_t &, const int16_t &, const uint32_t &, const int8_t &, const float &, const bool &) = nullptr;

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

	Waypoint *gP = g_waypoint;
	if (!gP)
		return;

	int seed = m_index + m_numSpawns + m_currentWeapon;
	float min = ebot_pathfinder_seed_min.GetFloat();
	float max = ebot_pathfinder_seed_max.GetFloat();

	int16_t i;
	Path currPath;

	if (m_isStuck)
		seed += static_cast<int>(engine->GetTime() * 0.1f);

	for (i = 0; i < g_numWaypoints; i++)
	{
		waypoints[i].g = 0.0f;
		waypoints[i].f = 0.0f;
		waypoints[i].parent = -1;
		waypoints[i].is_closed = false;
	}

	// put start waypoint into open list
	AStar &srcWaypoint = waypoints[srcIndex];
	srcWaypoint.g = 0.0f;
	srcWaypoint.f = srcWaypoint.g + hcalc(srcIndex, destIndex);

	// loop cache
	AStar *currWaypoint;
	AStar *childWaypoint;
	int16_t currentIndex, self;
	uint32_t flags;
	float g, f;

	openList.InsertLowest(srcIndex, srcWaypoint.f);
	while (!openList.IsEmpty())
	{
		currentIndex = openList.RemoveLowest();
		if (currentIndex == destIndex)
		{
			int16_t pathLength = 0;
			int16_t tempIndex = currentIndex;
			while (IsValidWaypoint(tempIndex))
			{
				pathLength++;
				tempIndex = waypoints[tempIndex].parent;
			}

			if (!m_navNode.Init(pathLength))
				m_navNode.Clear();

			do
			{
				if (!m_navNode.Add(currentIndex))
					break;

				currentIndex = waypoints[currentIndex].parent;
			} while (IsValidWaypoint(currentIndex));

			m_navNode.Reverse();
			if (m_navNode.HasNext() && (gP->GetPath(m_navNode.Next())->origin - pev->origin) .GetLengthSquared() < (gP->GetPath(m_navNode.Next())->origin - gP->GetPath(m_navNode.First())->origin).GetLengthSquared())
				m_navNode.Shift();

			if (!m_navNode.IsEmpty())
				 ChangeWptIndex(m_navNode.First());

			return;
		}

		currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->is_closed)
			continue;

		currWaypoint->is_closed = true;

		currPath = gP->m_paths[currentIndex];
		for (i = 0; i < pMax; i++)
		{
			self = currPath.index[i];
			if (!IsValidWaypoint(self))
				break;

			if (self == m_lastDeclineWaypoint)
				continue;

			if (currPath.connectionFlags[i] & PATHFLAG_VISIBLE)
			{
				TraceResult tr;
				TraceLine(currPath.origin, gP->m_paths[self].origin, TraceIgnore::Nothing, GetEntity(), &tr);
				if (tr.flFraction < 1.0f)
					continue;
			}

			flags = gP->m_paths[self].flags;
			if (flags)
			{
				if (flags & WAYPOINT_FALLCHECK)
				{
					TraceResult tr;
					const Vector origin = gP->m_paths[self].origin;
					TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), TraceIgnore::Nothing, GetEntity(), &tr);
					if (tr.flFraction >= 1.0f)
						continue;
				}

				if (flags & WAYPOINT_SPECIFICGRAVITY)
				{
					if ((pev->gravity * engine->GetGravity()) > gP->m_paths[self].gravity)
						continue;
				}

				if (flags & WAYPOINT_ONLYONE)
				{
					bool skip = false;
					for (Bot *const &bot : g_botManager->m_bots)
					{
						if (skip)
							break;

						if (bot && bot->m_isAlive && bot->GetEntity() != GetEntity())
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
			if (!childWaypoint->is_closed && (childWaypoint->f == 0.0f || childWaypoint->f > f))
			{
				childWaypoint->parent = currentIndex;
				childWaypoint->g = g;
				childWaypoint->f = f;
				openList.InsertLowest(self, f);
			}
		}
	}

	// roam around poorly :(
	if (m_navNode.IsEmpty())
	{
		CArray<int16_t> PossiblePath;
		for (i = 0; i < g_numWaypoints; i++)
		{
			if (waypoints[i].is_closed)
				PossiblePath.Push(i);
		}

		if (!PossiblePath.IsEmpty())
		{
			int16_t index = PossiblePath.Random();
			FindShortestPath(srcIndex, index);
			return;
		}

		FindShortestPath(srcIndex, destIndex);
	}
}

void Bot::FindShortestPath(int16_t &srcIndex, int16_t &destIndex)
{
	if (!IsValidWaypoint(srcIndex) || !IsValidWaypoint(destIndex))
	{
		m_navNode.Clear();
		return;
	}

	if (srcIndex == destIndex)
		return;

	if (waypoints.NotAllocated())
	{
		waypoints.Resize(g_numWaypoints, true);
		return;
	}

	if (waypoints.Capacity() != g_numWaypoints)
	{
		waypoints.Resize(g_numWaypoints, true);
		return;
	}

	PriorityQueue openList;
	if (!openList.Setup())
		return;

	const float (*hcalc)(const int16_t &, const int16_t &) = nullptr;

	if (g_isMatrixReady)
		hcalc = HF_Matrix;
	else
		hcalc = HF_DistanceSquared;

	if (!hcalc)
		return;

	Waypoint *gP = g_waypoint;
	if (!gP)
		return;

	int16_t i;
	for (i = 0; i < g_numWaypoints; i++)
	{
		waypoints[i].g = 0.0f;
		waypoints[i].f = 0.0f;
		waypoints[i].parent = -1;
		waypoints[i].is_closed = false;
	}

	AStar &srcWaypoint = waypoints[srcIndex];
	srcWaypoint.f = hcalc(srcIndex, destIndex);

	// loop cache
	AStar *currWaypoint;
	AStar *childWaypoint;
	int16_t currentIndex, self;
	uint32_t flags;
	Path currPath;
	float g, f;

	openList.InsertLowest(srcIndex, srcWaypoint.f);
	while (!openList.IsEmpty())
	{
		currentIndex = openList.RemoveLowest();
		if (currentIndex == destIndex)
		{
			int16_t pathLength = 0;
			int16_t tempIndex = currentIndex;
			while (IsValidWaypoint(tempIndex))
			{
				pathLength++;
				tempIndex = waypoints[tempIndex].parent;
			}

			if (!m_navNode.Init(pathLength))
				m_navNode.Clear();

			do
			{
				if (!m_navNode.Add(currentIndex))
					break;

				currentIndex = waypoints[currentIndex].parent;
			} while (IsValidWaypoint(currentIndex));

			m_navNode.Reverse();
			if (m_navNode.HasNext() && (gP->GetPath(m_navNode.Next())->origin - pev->origin) .GetLengthSquared() < (gP->GetPath(m_navNode.Next())->origin - gP->GetPath(m_navNode.First())->origin).GetLengthSquared())
				m_navNode.Shift();

			if (!m_navNode.IsEmpty())
				 ChangeWptIndex(m_navNode.First());

			return;
		}

		currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->is_closed)
			continue;

		currWaypoint->is_closed = true;
		currPath = gP->m_paths[currentIndex];
		for (i = 0; i < pMax; i++)
		{
			self = currPath.index[i];
			if (!IsValidWaypoint(self))
				break;

			if (self == m_lastDeclineWaypoint)
				continue;

			if (currPath.connectionFlags[i] & PATHFLAG_VISIBLE)
			{
				TraceResult tr;
				TraceLine(currPath.origin, gP->m_paths[self].origin, TraceIgnore::Nothing, GetEntity(), &tr);
				if (tr.flFraction < 1.0f)
					continue;
			}

			flags = gP->m_paths[self].flags;
			if (flags)
			{
				if (flags & WAYPOINT_FALLCHECK)
				{
					TraceResult tr;
					const Vector origin = gP->m_paths[self].origin;
					TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), TraceIgnore::Nothing, GetEntity(), &tr);
					if (tr.flFraction >= 1.0f)
						continue;
				}

				if (flags & WAYPOINT_SPECIFICGRAVITY)
				{
					if ((pev->gravity * engine->GetGravity()) > gP->m_paths[self].gravity)
						continue;
				}

				if (flags & WAYPOINT_ONLYONE)
				{
					bool skip = false;
					for (Bot *const &bot : g_botManager->m_bots)
					{
						if (skip)
							break;

						if (bot && bot->m_isAlive && bot->GetEntity() != GetEntity())
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

			g = currWaypoint->g + hcalc(self, currentIndex);
			f = g + hcalc(self, destIndex);
			childWaypoint = &waypoints[self];
			if (!childWaypoint->is_closed && (childWaypoint->f == 0.0f || childWaypoint->f > f))
			{
				childWaypoint->parent = currentIndex;
				childWaypoint->g = g;
				childWaypoint->f = f;
				openList.InsertLowest(self, f);
			}
		}
	}
}

void Bot::FindEscapePath(int16_t &srcIndex, const Vector &dangerOrigin)
{
	if (!ebot_use_pathfinding_for_avoid.GetBool())
	{
		m_navNode.Clear();
		return;
	}

	if (waypoints.NotAllocated())
	{
		waypoints.Resize(g_numWaypoints, true);
		return;
	}

	if (waypoints.Capacity() != g_numWaypoints)
	{
		waypoints.Resize(g_numWaypoints, true);
		return;
	}

	PriorityQueue openList;
	if (!openList.Setup())
		return;

	Waypoint *gP = g_waypoint;
	if (!gP)
		return;

	if (!IsValidWaypoint(srcIndex))
	{
		srcIndex = FindWaypoint();
		return;
	}

	int16_t i;
	for (i = 0; i < g_numWaypoints; i++)
	{
		waypoints[i].g = FLT_MAX;
		waypoints[i].f = FLT_MAX;
		waypoints[i].parent = -1;
		waypoints[i].is_closed = false;
	}

	int16_t currentIndex, neighborIndex;
	AStar *currWaypoint, *childWaypoint;
	Path currPath;
	uint32_t flags;

	AStar &srcWaypoint = waypoints[srcIndex];
	const Vector &srcPos = gP->m_paths[srcIndex].origin;
	srcWaypoint.g = (srcPos - pev->origin).GetLengthSquared();
	float dangerDistance = (srcPos - dangerOrigin).GetLengthSquared();
	srcWaypoint.f = srcWaypoint.g - dangerDistance;

	openList.InsertLowest(srcIndex, srcWaypoint.f);

	bool found = false;
	int16_t foundIndex = -1;
	int16_t iterations = 0;
	const int16_t maxIterations = 150;
	while (!openList.IsEmpty() && iterations < maxIterations)
	{
		iterations++;
		currentIndex = openList.RemoveLowest();
		currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->is_closed)
			continue;

		currWaypoint->is_closed = true;
		currPath = gP->m_paths[currentIndex];
		if (!IsEnemyReachableToPosition(currPath.origin))
		{
			found = true;
			foundIndex = currentIndex;
			break;
		}

		for (i = 0; i < pMax; i++)
		{
			neighborIndex = currPath.index[i];
			if (!IsValidWaypoint(neighborIndex))
				break;

			childWaypoint = &waypoints[neighborIndex];
			if (childWaypoint->is_closed)
				continue;

			if (currPath.connectionFlags[i] & PATHFLAG_VISIBLE)
			{
				TraceResult tr;
				TraceLine(currPath.origin, gP->m_paths[neighborIndex].origin, TraceIgnore::Nothing, GetEntity(), &tr);
				if (tr.flFraction < 1.0f)
					continue;
			}

			flags = gP->m_paths[neighborIndex].flags;
			if (flags)
			{
				if (flags & WAYPOINT_FALLCHECK)
				{
					TraceResult tr;
					const Vector origin = gP->m_paths[neighborIndex].origin;
					TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), TraceIgnore::Nothing, GetEntity(), &tr);
					if (tr.flFraction >= 1.0f)
						continue;
				}

				if (flags & WAYPOINT_SPECIFICGRAVITY)
				{
					if ((pev->gravity * engine->GetGravity()) > gP->m_paths[neighborIndex].gravity)
						continue;
				}

				if (flags & WAYPOINT_ONLYONE)
				{
					bool skip = false;
					for (Bot *const &bot : g_botManager->m_bots)
					{
						if (skip)
							break;

						if (bot && bot->m_isAlive && bot->GetEntity() != GetEntity())
						{
							for (int16_t j = 0; j < bot->m_navNode.Length(); j++)
							{
								if (bot->m_navNode.Get(j) == neighborIndex)
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

			const Vector &neighborPos = gP->m_paths[neighborIndex].origin;
			float dangerDist = (neighborPos - dangerOrigin).GetLengthSquared();
			float dangerPenalty = 0.0f;
			constexpr float safeDist = 512.0f * 512.0f;
			if (dangerDist < safeDist)
				dangerPenalty = (safeDist - dangerDist) * 2.0f;

			float g = currWaypoint->g + (currPath.origin - neighborPos).GetLengthSquared() + dangerPenalty;
			float escapeScore = g + -dangerDist;

			// Ally bonus: prefer moving towards allies on the safe side (away from danger)
			float botToDangerSq = (pev->origin - dangerOrigin).GetLengthSquared();
			float allyBonus = 0.0f;
			bool allyOnSafeSide = false;

			for (const Clients &client : g_clients)
			{
				if (client.team != m_team || !IsAlive(client.ent))
					continue;

				Vector allyPos = client.ent->v.origin;
				float allyToDangerSq = (allyPos - dangerOrigin).GetLengthSquared();

				// Ally safe side'da mı? (danger'a bot'tan daha uzak)
				if (allyToDangerSq > botToDangerSq)
				{
					// Ally, escape yönünde mi? (Karesel mesafe teoremi eşdeğeri kontrol)
					float botToAllySq = (allyPos - pev->origin).GetLengthSquared();
					if (allyToDangerSq > (botToDangerSq + botToAllySq))
					{
						allyBonus += (safeDist - allyToDangerSq) * 0.3f;
						allyOnSafeSide = true;
					}
				}
			}

			if (allyOnSafeSide)
				escapeScore -= allyBonus;
			else if (allyBonus == 0.0f)
				escapeScore *= 1.5f;

			float f = escapeScore;
			if (!childWaypoint->is_closed && (childWaypoint->f == 0.0f || childWaypoint->f > f))
			{
				childWaypoint->parent = currentIndex;
				childWaypoint->g = g;
				childWaypoint->f = f;
				openList.InsertLowest(neighborIndex, f);
			}
		}
	}

	if (found && IsValidWaypoint(foundIndex))
	{
		int16_t pathLength = 0;
		int16_t tempIndex = foundIndex;
		while (IsValidWaypoint(tempIndex) && pathLength < maxIterations)
		{
			pathLength++;
			tempIndex = waypoints[tempIndex].parent;
		}

		if (pathLength > m_navNode.GetCapacity())
		{
			if (!m_navNode.Init(pathLength + 5))
			{
				m_navNode.Clear();
				return;
			}
		}
		else
			m_navNode.Clear();

		int16_t pathIndex = foundIndex;
		int16_t safetyCounter = 0;

		do
		{
			safetyCounter++;
			if (safetyCounter > pathLength + 10)
				break;

			if (!m_navNode.Add(pathIndex))
				break;

			pathIndex = waypoints[pathIndex].parent;
		} while (IsValidWaypoint(pathIndex));

		if (m_navNode.Length() > 1)
		{
			m_navNode.Reverse();
			if ((gP->GetPath(m_navNode.Next())->origin - pev->origin).GetLengthSquared() < (gP->GetPath(m_navNode.First())->origin - pev->origin).GetLengthSquared())
				m_navNode.Shift();

			if (!m_navNode.IsEmpty())
				 ChangeWptIndex(m_navNode.First());
		}
		else if (!m_navNode.IsEmpty())
			 ChangeWptIndex(m_navNode.First());
	}
}

void Bot::CheckTouchEntity(edict_t *entity)
{
	if (FNullEnt(entity))
		return;

	if (entity->v.flags & (FL_CLIENT | FL_FAKECLIENT))
		return;

	// if we won't be able to break it, don't try
	if (entity->v.takedamage == DAMAGE_NO)
		return;

	// see if it's breakable
	if (entity->v.health && entity->v.health > 0.0f && entity->v.health < ebot_breakable_health_limit.GetFloat())
	{
		// sandbags...
		if (!m_isZombieBot && FClassnameIs(entity, "amxx_pallets"))
			return;

		// check lasermine team...
		if (entity->v.iuser2)
		{
			int ownerIndex = entity->v.iuser2; 
			if (ownerIndex > 1 && ownerIndex <= engine->GetMaxClients())
			{
				edict_t *owner = INDEXENT(ownerIndex - 1);
				if (!FNullEnt(owner))
				{
					if (GetTeam(owner) == m_team)
						return; 
				}
			}
		}

		TraceResult tr;
		TraceHull(EyePosition(), m_waypoint.origin, TraceIgnore::Nothing, head_hull, GetEntity(), &tr);

		TraceResult tr2;
		TraceLine(EyePosition(), m_destOrigin, TraceIgnore::Nothing, GetEntity(), &tr2);

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
				if (IsBreakable(entity))
				{
					edict_t *ent;
					for (const auto &enemy : g_botManager->m_bots)
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

						ent = enemy->GetEntity();
						if (FNullEnt(ent))
							continue;

						TraceHull(enemy->EyePosition(), m_breakableOrigin, TraceIgnore::Nothing, point_hull, ent, &tr);
						TraceHull(ent->v.origin, m_breakableOrigin, TraceIgnore::Nothing, head_hull, ent, &tr2);
						if ((!FNullEnt(tr.pHit) && tr.pHit == entity) || (!FNullEnt(tr2.pHit) && tr2.pHit == entity))
						{
							enemy->m_breakableEntity = entity;
							enemy->m_breakableOrigin = m_breakableOrigin;
							enemy->SetProcess(Process::DestroyBreakable, "trying to destroy a breakable to force enemy fall", false, time2 + 60.0f);
						}
					}
				}
			}
			else if (!m_isZombieBot) // tell my friends to destroy it
			{
				edict_t *ent;
				for (Bot *const &bot : g_botManager->m_bots)
				{
					if (!bot)
						continue;

					if (m_team != bot->m_team)
						continue;

					if (!bot->m_isAlive)
						continue;

					if (bot->m_isZombieBot)
						continue;

					ent = bot->GetEntity();
					if (FNullEnt(ent))
						continue;

					if (GetEntity() == ent)
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

int16_t Bot::FindWaypoint(void)
{
	if (m_isAlive)
	{
		int16_t index = -1;
		bool foundOnPath = false;

		if (m_navNode.HasNext())
		{
			for (int16_t i = 0; i < cmin(m_navNode.Length(), static_cast<int16_t>(4)); i++)
			{
				int16_t wpOnPath = m_navNode.Get(i);
				if (IsValidWaypoint(wpOnPath) && g_waypoint->Reachable(GetEntity(), wpOnPath))
				{
					if (wpOnPath == m_lastDeclineWaypoint)
						continue;

					index = wpOnPath;
					foundOnPath = true;
					for (int16_t j = 0; j < i; j++)
						m_navNode.Shift();
					break;
				}
			}

			// Path-retention fallback: If none of the next waypoints are strictly reachable
			// (common in vents or stairs due to tight geometry blocking traces),
			// but we have a planned path and are not stuck, keep targeting the next node.
			if (!foundOnPath && (!m_isStuck || m_stuckTime < 2.0f))
			{
				int16_t nextWp = m_navNode.First();
				if (IsValidWaypoint(nextWp) && nextWp != m_lastDeclineWaypoint)
				{
					index = nextWp;
					foundOnPath = true;
				}
			}
		}

		if (!foundOnPath)
		{
			if ((!m_isStuck || m_stuckTime < 1.0f) 
				&& g_clients[m_index].wp != m_lastDeclineWaypoint 
				&& g_waypoint->Reachable(GetEntity(), g_clients[m_index].wp))
			{
				index = g_clients[m_index].wp;
			}
			else
			{
				index = g_waypoint->FindNearestToEnt(pev->origin, 2048.0f, GetEntity());

				// If the found index is the blacklisted one, look for an alternative nearest reachable one
				if (index == m_lastDeclineWaypoint || !IsValidWaypoint(index))
				{
					float minDistance = squaredf(2048.0f);
					int16_t bestIdx = -1;
					for (int16_t i = 0; i < g_numWaypoints; i++)
					{
						if (i == m_lastDeclineWaypoint)
							continue;

						float distance = (g_waypoint->GetPath(i)->origin - pev->origin).GetLengthSquared();
						if (distance < minDistance)
						{
							if (!g_waypoint->Reachable(GetEntity(), i))
								continue;

							bestIdx = i;
							minDistance = distance;
						}
					}
					index = bestIdx;
				}

				// Extreme fallback: absolute nearest excluding blacklisted
				if (!IsValidWaypoint(index))
				{
					float minDistance = 99999999.0f;
					int16_t bestIdx = -1;
					for (int16_t i = 0; i < g_numWaypoints; i++)
					{
						if (i == m_lastDeclineWaypoint)
							continue;

						float distance = (g_waypoint->GetPath(i)->origin - pev->origin).GetLengthSquared();
						if (distance < minDistance)
						{
							bestIdx = i;
							minDistance = distance;
						}
					}
					index = bestIdx;
				}
			}
		}

		ChangeWptIndex(index);
		return index;
	}

	return m_currentWaypointIndex;
}

// return priority of player (0 = max pri)
inline int GetPlayerPriority(edict_t *player)
{
	// human players have highest priority
	const Bot *bot = g_botManager->GetBot(player);
	if (bot)
		return bot->m_index + 3; // everyone else is ranked by their unique ID (which cannot be zero)

	return 1;
}

void Bot::ResetStuck(void)
{
	m_stuckTime = 0.0f;
	m_isStuck = false;
	m_wasStuck = false;
	m_checkFall = false;
	for (Vector &fall : m_checkFallPoint)
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
		if ((m_isStuck && m_stuckTime > 7.0f)  || (pev->origin - m_waypoint.origin).GetLengthSquared() > squaredf(512.0f)  || (!g_waypoint->Reachable(GetEntity(), m_currentWaypointIndex) && (!m_navNode.HasNext() || (m_isStuck && m_stuckTime > 2.0f))))
		{
			FindWaypoint();
			m_navNode.Clear();
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
					m_navNode.Clear();
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

void Bot::CheckStuck(const Vector &directionNormal, const float finterval)
{
	if (m_waypoint.flags & WAYPOINT_FALLCHECK)
	{
		ResetStuck();
		return;
	}

	const float time2 = engine->GetTime();
	const bool isDucking = !!(pev->flags & FL_DUCKING);
	const bool onFloor = IsOnFloor();
	const bool onLadder = IsOnLadder();
	const bool inWater = IsInWater();

	const bool inCooldown = m_stuckCooldownUntil > time2 && (pev->origin - m_lastStuckOrigin).GetLengthSquared() < squaredf(72.0f);

	// determine goal position
	const Vector goalPos = !m_waypointOrigin.IsNull() ? m_waypointOrigin : m_destOrigin;

	// velocity-direction analysis (every frame, independent of sampling)
	const Vector velNorm = pev->velocity.Normalize2D();
	const Vector goalDir = (goalPos - pev->origin).Normalize2D();
	const float velocityDot = velNorm | goalDir;

	float sampleInterval;
	if (isDucking || onLadder)
		sampleInterval = inCooldown ? 0.25f : 0.50f;
	else
		sampleInterval = inCooldown ? 0.125f : 0.25f;

	if (m_prevTime < time2)
	{
		m_isStuck = false;

		m_movedDistance = (m_prevOrigin - pev->origin).GetLengthSquared();
		m_prevOrigin = pev->origin;

		m_prevTime = time2 + sampleInterval;

		m_posHistory[m_posHistoryIdx] = pev->origin;
		m_posHistoryIdx = (m_posHistoryIdx + 1) & 7; // mod 8
		if (m_posHistoryCount < 8)
			m_posHistoryCount++;

		// check oscillation when buffer is full
		if (m_posHistoryCount >= 8)
		{
			Vector bboxMin = m_posHistory[0];
			Vector bboxMax = m_posHistory[0];

			for (int h = 1; h < 8; h++)
			{
				if (m_posHistory[h].x < bboxMin.x) bboxMin.x = m_posHistory[h].x;
				if (m_posHistory[h].y < bboxMin.y) bboxMin.y = m_posHistory[h].y;
				if (m_posHistory[h].x > bboxMax.x) bboxMax.x = m_posHistory[h].x;
				if (m_posHistory[h].y > bboxMax.y) bboxMax.y = m_posHistory[h].y;
			}

			const float bboxSizeX = bboxMax.x - bboxMin.x;
			const float bboxSizeY = bboxMax.y - bboxMin.y;

			// check goal progress over the entire buffer window
			const int oldestIdx = m_posHistoryIdx; // oldest entry (circular)
			const float oldDistToGoal = (m_posHistory[oldestIdx] - goalPos).GetLength2D();
			const float curDistToGoal = (pev->origin - goalPos).GetLength2D();
			const float progressOverBuffer = oldDistToGoal - curDistToGoal;

			if (bboxSizeX < 80.0f && bboxSizeY < 80.0f && progressOverBuffer < 20.0f)
				m_isOscillating = true;
			else
				m_isOscillating = false;
		}
	}
	else
	{
		if (m_hasFriendsNear && !ebot_has_semiclip.GetBool() && !FNullEnt(m_nearestFriend))
		{
			m_avoid = nullptr;
			const int prio = GetPlayerPriority(GetEntity());
			const int friendPrio = GetPlayerPriority(m_nearestFriend);

			if (prio > friendPrio)
				m_avoid = m_nearestFriend;
			else
			{
				const Bot *friendBot = g_botManager->GetBot(m_nearestFriend);
				if (friendBot && !FNullEnt(friendBot->m_avoid) && GetEntity() != friendBot->m_avoid && friendPrio > GetPlayerPriority(friendBot->m_avoid))
					m_avoid = friendBot->m_avoid;
			}

			if (!FNullEnt(m_avoid))
			{
				const Bot *avoidBot = g_botManager->GetBot(m_avoid);
				Vector smoothedAvoidVel;
				if (avoidBot && avoidBot != this)
					smoothedAvoidVel = avoidBot->pev->velocity; // avoid target is a bot — use its raw velocity directly (more reliable)
				else
					smoothedAvoidVel = m_avoid->v.velocity * 0.65f + m_lastAvoidVel * 0.35f; // avoid target is a player — smooth the velocity

				m_lastAvoidVel = smoothedAvoidVel;

				// scale prediction interval based on movement state
				const float interval = finterval * (!isDucking && pev->velocity.GetLengthSquared2D() > squaredf(14.0f) ? 10.0f : 5.0f);

				Vector right, forward;
				m_moveAngles.BuildVectors(&forward, &right, nullptr);

				// predict our future position
				Vector predict = pev->origin + forward * m_moveSpeed * interval;
				predict += right * m_strafeSpeed * interval;
				predict += pev->velocity * interval;

				// predict the avoid target's future position (using smoothed velocity)
				const Vector avoidFuturePos = m_avoid->v.origin + smoothedAvoidVel * interval;

				const float distance = (pev->origin - m_avoid->v.origin).GetLengthSquared();
				const float movedDistance = (predict - m_avoid->v.origin).GetLengthSquared();
				const float nextFrameDistance = (pev->origin - avoidFuturePos).GetLengthSquared();
				const float futureConverge = (predict - avoidFuturePos).GetLengthSquared();

				if (movedDistance < squaredf(64.0f) || futureConverge < squaredf(56.0f) || (distance < squaredf(72.0f) && nextFrameDistance < distance))
				{
					const Vector dir = (pev->origin - m_avoid->v.origin).Normalize2D();
					const Vector rightNorm = right.Normalize2D();
					const Vector forwardNorm = forward.Normalize2D();

					MakeVectors(m_moveAngles);
					TraceResult trLeft, trRight;

					TraceHull(pev->origin, pev->origin - g_pGlobals->v_right * 48.0f, TraceIgnore::Nothing, head_hull, GetEntity(), &trLeft);
					TraceHull(pev->origin, pev->origin + g_pGlobals->v_right * 48.0f, TraceIgnore::Nothing, head_hull, GetEntity(), &trRight);

					const bool narrowLeft = trLeft.flFraction < 0.6f;
					const bool narrowRight = trRight.flFraction < 0.6f;
					const bool isNarrowCorridor = narrowLeft && narrowRight;

					bool isHeadOn = false;
					Vector otherIntendedDir = nullvec;

					if (avoidBot && avoidBot != this)
					{
						if (!avoidBot->m_waypointOrigin.IsNull())
							otherIntendedDir = (avoidBot->m_waypointOrigin - m_avoid->v.origin).Normalize2D();
						else if (m_avoid->v.velocity.GetLengthSquared2D() > squaredf(5.0f))
							otherIntendedDir = m_avoid->v.velocity.Normalize2D();

						if (!otherIntendedDir.IsNull())
						{
							const Vector myIntendedDir = !m_waypointOrigin.IsNull() ? (m_waypointOrigin - pev->origin).Normalize2D() : forwardNorm;
							const float headOnDot = myIntendedDir | otherIntendedDir;
							isHeadOn = headOnDot < -0.5f;
						}
					}

					// if oscillating, force strafe to least-used direction to break the loop
					bool oscillationStrafeOverride = false;
					float oscillationStrafeDir = 0.0f;
					if (m_isOscillating)
					{
						// compare left/right blocked states to pick least-blocked direction
						if (narrowLeft && !narrowRight)
							oscillationStrafeDir = 1.0f; // go right
						else if (narrowRight && !narrowLeft)
							oscillationStrafeDir = -1.0f; // go left
						else
							oscillationStrafeDir = (m_currentWaypointIndex & 1) ? 1.0f : -1.0f; // neither blocked or both: alternate based on current waypoint index

						oscillationStrafeOverride = true;
					}

					if (isHeadOn && isNarrowCorridor)
					{
						const bool iAmLowerPriority = prio > friendPrio;

						if (iAmLowerPriority)
						{
							m_moveSpeed = -pev->maxspeed;

							if (m_isStuck && m_stuckTime > 3.0f)
							{
								m_moveSpeed = 0.0f;
								m_strafeSpeed = 0.0f;
								SetProcess(Process::Pause, "yielding in narrow corridor", true, time2 + crandomfloat(1.0f, 2.5f));
							}
							else if (!narrowLeft)
							{
								SetStrafeSpeed(directionNormal, -pev->maxspeed);
								m_moveSpeed = -pev->maxspeed * 0.5f;
							}
							else if (!narrowRight)
							{
								SetStrafeSpeed(directionNormal, pev->maxspeed);
								m_moveSpeed = -pev->maxspeed * 0.5f;
							}
						}
						else
						{
							m_moveSpeed = pev->maxspeed;

							if (!narrowLeft && narrowRight)
								SetStrafeSpeed(directionNormal, -pev->maxspeed * 0.5f);
							else if (!narrowRight && narrowLeft)
								SetStrafeSpeed(directionNormal, pev->maxspeed * 0.5f);
						}
					}
					else if (isHeadOn && !isNarrowCorridor)
					{
						if (oscillationStrafeOverride)
						{
							SetStrafeSpeed(directionNormal, pev->maxspeed * oscillationStrafeDir);
							m_moveSpeed = pev->maxspeed * 0.7f;
						}
						else if (!narrowRight)
						{
							SetStrafeSpeed(directionNormal, pev->maxspeed);
							m_moveSpeed = pev->maxspeed * 0.7f;
						}
						else if (!narrowLeft)
						{
							SetStrafeSpeed(directionNormal, -pev->maxspeed);
							m_moveSpeed = pev->maxspeed * 0.7f;
						}
						else
						{
							if ((dir | rightNorm) > 0.0f)
								SetStrafeSpeed(directionNormal, pev->maxspeed);
							else
								SetStrafeSpeed(directionNormal, -pev->maxspeed);
						}
					}
					else
					{
						// NOT HEAD-ON: standard avoidance
						if (oscillationStrafeOverride)
						{
							SetStrafeSpeed(directionNormal, pev->maxspeed * oscillationStrafeDir);
						}
						else
						{
							if ((dir | rightNorm) > 0.0f)
								SetStrafeSpeed(directionNormal, pev->maxspeed);
							else
								SetStrafeSpeed(directionNormal, -pev->maxspeed);
						}

						if (distance < squaredf(80.0f))
						{
							if ((dir | forwardNorm) < 0.0f)
								m_moveSpeed = -pev->maxspeed;
							else
								m_moveSpeed = pev->maxspeed;
						}
					}

					if (avoidBot && avoidBot != this)
					{
						if (m_navNode.HasNext() && avoidBot->m_navNode.HasNext() && m_navNode.First() == avoidBot->m_navNode.First())
						{
							if (prio > friendPrio)
								m_navNode.Shift();
						}

						if (avoidBot->m_isStuck && !FNullEnt(avoidBot->m_avoid) && avoidBot->m_avoid == GetEntity())
						{
							if (prio > friendPrio)
							{
								// same destination: lower priority skips
								if (m_navNode.HasNext() && avoidBot->m_navNode.HasNext() && m_navNode.First() == avoidBot->m_navNode.First())
									m_navNode.Shift();
								else if (m_navNode.HasNext() && avoidBot->m_navNode.HasNext()) // different destinations but paths intersect: yield briefly
								{
									if (m_stuckTime < 5.0f)
										SetProcess(Process::Pause, "mutual deadlock yield", true, time2 + crandomfloat(1.0f, 3.0f)); // pause to let higher priority pass
									else if (m_isSlowThink)
									{
										FindWaypoint(); // deadlock exceeded 5 seconds: full repath
										FindPath(m_currentWaypointIndex, m_currentGoalIndex);
									}
								}
							}
						}

						if (avoidBot->m_collisionState == COSTATE_PROBING)
						{
							// read what the other bot is trying and do the opposite
							if (avoidBot->m_collStateIndex < 3)
							{
								const int otherMove = avoidBot->m_collideMoves[avoidBot->m_collStateIndex];
								if (otherMove == COSTATE_STRAFELEFT)
									SetStrafeSpeed(directionNormal, pev->maxspeed); // we go right
								else if (otherMove == COSTATE_STRAFERIGHT)
									SetStrafeSpeed(directionNormal, -pev->maxspeed); // we go left
							}
						}

						// other bot is stuck on something else but blocking us
						if (avoidBot->m_isStuck && !FNullEnt(avoidBot->m_avoid) && avoidBot->m_avoid != GetEntity())
							m_stuckTime += finterval * 0.5f; // more aggressive stuck escalation
					}

					if (m_isStuck && m_navNode.HasNext())
					{
						if (avoidBot && avoidBot != this && avoidBot->m_navNode.HasNext())
						{
							if (m_currentWaypointIndex == avoidBot->m_currentWaypointIndex)
							{
								if (prio > friendPrio)
									m_navNode.Shift();
							}
							else if (avoidBot->m_isStuck && !FNullEnt(avoidBot->m_avoid) && avoidBot->m_avoid == GetEntity())
							{
								if (prio > friendPrio)
								{
									m_navNode.Shift();

									if (m_stuckTime > 5.0f && m_isSlowThink)
									{
										FindWaypoint();
										FindPath(m_currentWaypointIndex, m_currentGoalIndex);
									}
								}
							}
							else if (m_isStuck && m_stuckTime > 2.0f && prio > friendPrio)
							{
								if (m_navNode.HasNext() && m_navNode.Next() == avoidBot->m_currentWaypointIndex)
									m_navNode.Shift();

								if (avoidBot->m_navNode.HasNext() && avoidBot->m_navNode.First() == m_currentWaypointIndex)
								{
									if (m_isSlowThink)
									{
										FindWaypoint();
										FindPath(m_currentWaypointIndex, m_currentGoalIndex);
									}
								}
							}
						}
					}
				}

				if (m_isZombieBot && !FNullEnt(m_avoid))
				{
					float crowdPressure = 0.0f;
					Vector pressureGradient = nullvec;
					int nearbyCount = 0;

					for (int bi = 0; bi < 32; bi++)
					{
						const Bot *nearBot = g_botManager->m_bots[bi];
						if (!nearBot || nearBot == this || !nearBot->m_isAlive)
							continue;

						if (nearBot->m_team != m_team)
							continue;

						const float dist = (pev->origin - nearBot->pev->origin).GetLength2D();
						if (dist > 192.0f || dist < 0.1f)
							continue;

						crowdPressure += 1.0f / dist;
						pressureGradient += (pev->origin - nearBot->pev->origin).Normalize2D() * (1.0f / dist);
						nearbyCount++;
					}

					if (crowdPressure > 0.08f && nearbyCount > 0)
					{
						// strafe toward lowest crowd density
						const Vector escapeDir = pressureGradient.Normalize2D();
						Vector right2;
						m_moveAngles.BuildVectors(nullptr, &right2, nullptr);
						const Vector rightNorm2 = right2.Normalize2D();

						if ((escapeDir | rightNorm2) > 0.0f)
							SetStrafeSpeed(directionNormal, pev->maxspeed);
						else
							SetStrafeSpeed(directionNormal, -pev->maxspeed);
					}
				}
			}
		}
	}

	if (m_lastCollTime < time2)
	{
		const float currentSpeed = pev->velocity.GetLength2D();
		const float toleranceSpeed = pev->maxspeed / 3.0f;

		if (currentSpeed >= toleranceSpeed)
		{
			if (velocityDot > 0.5f)
				m_firstCollideTime = 0.0f; // heading toward goal with good speed → definitely not stuck
			else if (velocityDot < -0.3f)
				m_firstCollideTime = 0.0f;
			else
				m_firstCollideTime = 0.0f;
		}
		else
		{
			const float prevDistToGoal = (m_prevOrigin - goalPos).GetLength2D();
			const float currDistToGoal = (pev->origin - goalPos).GetLength2D();

			float progressThreshold;
			if (onLadder)
				progressThreshold = 1.0f;
			else if (isDucking)
				progressThreshold = 1.2f;
			else
				progressThreshold = 2.0f;

			const bool makingProgress = (currDistToGoal < prevDistToGoal - progressThreshold);

			float stuckThresholdMul = inCooldown ? 0.7f : 1.0f;
			const float stuckThreshold = (isDucking ? squaredf(0.8f) : squaredf(1.5f)) * stuckThresholdMul;

			if (FNullEnt(m_avoid) && !makingProgress && m_movedDistance < stuckThreshold && (m_prevSpeed > 20.0f || m_prevVelocity < squaredf(m_moveSpeed * 0.5f)))
			{
				bool knockbackSafe = false;
				if (velocityDot < -0.3f && !m_waypointOrigin.IsNull())
				{
					// bot is being pushed backward — check if waypoint is still reachable
					if (IsVisible(m_waypointOrigin, GetEntity()))
						knockbackSafe = true;
				}

				if (!knockbackSafe)
				{
					m_prevTime = time2;
					m_isStuck = true;

					if (Math::FltZero(m_firstCollideTime))
						m_firstCollideTime = time2 + 0.2f;
				}
			}
			else if (makingProgress)
			{
				// making goal progress — not stuck even if moved distance is low
				// (handles stairs, ducking, slow ladder movement)
				m_firstCollideTime = 0.0f;
			}
			else
			{
				// test if there's something ahead blocking the way
				if (!(m_waypoint.flags & WAYPOINT_LADDER) && !onLadder && CantMoveForward(directionNormal))
				{
					if (Math::FltZero(m_firstCollideTime))
						m_firstCollideTime = time2 + 0.2f;
					else if (m_firstCollideTime < time2)
						m_isStuck = true;
				}
				else
					m_firstCollideTime = 0.0f;
			}
		}
	}

	if (m_isStuck)
	{
		if (m_collisionState == COSTATE_UNDECIDED)
		{
			StuckCause stuckCause = STUCK_CAUSE_UNKNOWN;
			if (m_isOscillating)
				stuckCause = STUCK_CAUSE_OSCILLATION;
			else if (m_hasFriendsNear && !FNullEnt(m_avoid))
				stuckCause = STUCK_CAUSE_FRIEND;
			else if (m_hasEnemiesNear && !FNullEnt(m_nearestEnemy) && (pev->origin - m_nearestEnemy->v.origin).GetLengthSquared() < squaredf(128.0f))
				stuckCause = STUCK_CAUSE_ENEMY;
			else
				stuckCause = STUCK_CAUSE_GEOMETRY;

			m_stuckCause = stuckCause;
		}

		if (m_isEnemyReachable)
			CheckReachable();
		else
		{
			float stuckAccum;
			if (m_movedDistance < squaredf(0.5f))
				stuckAccum = finterval * 2.0f;
			else
				stuckAccum = finterval;

			// oscillation: 1.5x faster escalation
			if (m_isOscillating)
				stuckAccum *= 1.5f;

			m_stuckTime += stuckAccum;

			if (IsValidWaypoint(m_currentWaypointIndex))
			{
				if (m_lastStuckWaypointIndex == m_currentWaypointIndex)
				{
					if (m_isStuck && !m_wasStuck)
						m_waypointStuckCount++;
				}
				else
				{
					m_lastStuckWaypointIndex = m_currentWaypointIndex;
					m_waypointStuckCount = 1;
				}

				// blacklist trigger: 3+ stucks at same waypoint with significant stuck time
				if (m_waypointStuckCount >= 3 && m_stuckTime > 7.0f && m_isSlowThink)
				{
					m_lastDeclineWaypoint = m_currentWaypointIndex;
					// NOTE: actual blacklist record (WaypointStuckRecord) should be written
					// to a BotManager-level or global container here.
					// The penalty duration would be: m_waypointStuckCount * 15.0f (max 60.0f)
				}
			}

			if (m_stuckTime > 45.0f)
				Kill();
			else if (m_stuckTime > 7.0f && m_isSlowThink)
			{
				if (IsValidWaypoint(m_currentWaypointIndex))
					m_lastDeclineWaypoint = m_currentWaypointIndex;

				FindWaypoint();
				FindPath(m_currentWaypointIndex, m_currentGoalIndex);
			}
		}

		// not yet decided what to do?
		if (m_collisionState == COSTATE_UNDECIDED)
		{
			TraceResult tr;
			uint32_t bits = 0;

			if (m_stuckCause == STUCK_CAUSE_FRIEND)
			{
				// friend blocking: skip heavy geometry probes, favor strafe
				bits |= COPROBE_STRAFE;
				if (m_stuckTime > 3.0f)
					bits |= (COPROBE_JUMP | COPROBE_DUCK);
			}
			else if (m_stuckCause == STUCK_CAUSE_ENEMY)
			{
				if (m_isZombieBot)
				{
					// zombie vs human: duck + forward push
					bits |= (COPROBE_DUCK | COPROBE_STRAFE);
				}
				else
				{
					// human: strafe + direction change
					bits |= COPROBE_STRAFE;
					if (m_stuckTime > 1.0f)
						bits |= COPROBE_JUMP;
				}
			}
			else if (onLadder)
			{
				bits |= COPROBE_STRAFE;
				if (m_stuckTime > 2.0f)
					bits |= COPROBE_JUMP;
			}
			else if (inWater)
			{
				bits |= (COPROBE_JUMP | COPROBE_STRAFE);
			}
			else
			{
				bits |= COPROBE_STRAFE;
				if (pev->maxspeed > 20.0f)
				{
					const bool destIsAbove = m_destOrigin.z >= pev->origin.z + 18.0f;
					const bool destIsBelow = m_destOrigin.z + 36.0f <= pev->origin.z;
					const bool waypointCrouch = !!(m_waypoint.flags & WAYPOINT_CROUCH);

					if (destIsAbove || CanJumpUp(directionNormal))
						bits |= COPROBE_JUMP;
					else if (!isDucking && m_stuckTime > 1.0f)
						bits |= COPROBE_JUMP;

					if (destIsBelow || waypointCrouch || CanDuckUnder(directionNormal))
						bits |= COPROBE_DUCK;
					else if (m_stuckTime > 2.0f)
						bits |= COPROBE_DUCK;

					if (m_isZombieBot)
						bits |= COPROBE_DUCK;
				}
			}

			// collision check allowed if not flying through the air
			if (onFloor || onLadder || inWater)
			{
				Vector src, dest;

				struct ProbeState
				{
					int move;
					int weight;
				};

				ProbeState states[4] =
				{
					{ COSTATE_STRAFELEFT, 0 },
					{ COSTATE_STRAFERIGHT, 0 },
					{ COSTATE_JUMP, 0 },
					{ COSTATE_DUCK, 0 }
				};

				if (bits & COPROBE_STRAFE)
				{
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
					const float strafeTestDist = isDucking ? 36.0f : 52.0f;

					src = pev->origin + g_pGlobals->v_right * strafeTestDist;
					dest = src + testDir * strafeTestDist;

					TraceHull(src, dest, TraceIgnore::Nothing, head_hull, GetEntity(), &tr);
					if (tr.flFraction < 1.0f)
						blockedRight = true;

					src = pev->origin - g_pGlobals->v_right * strafeTestDist;
					dest = src + testDir * strafeTestDist;

					TraceHull(src, dest, TraceIgnore::Nothing, head_hull, GetEntity(), &tr);
					if (tr.flFraction < 1.0f)
						blockedLeft = true;

					int leftWeight = 0;
					int rightWeight = 0;

					if (dirLeft)
						leftWeight += 5;
					else
						leftWeight -= 5;

					if (blockedLeft)
						leftWeight -= 5;

					if (blockedLeft && blockedRight)
					{
						if (dirLeft)
							leftWeight += 3;
					}

					if (dirRight)
						rightWeight += 5;
					else
						rightWeight -= 5;

					if (blockedRight)
						rightWeight -= 5;

					if (blockedLeft && blockedRight)
					{
						if (dirRight)
							rightWeight += 3;
					}

					if (m_isOscillating)
					{
						// force least-used direction: flip the weights
						if (m_currentWaypointIndex & 1)
						{
							// prefer right
							rightWeight += 4;
							leftWeight -= 4;
						}
						else
						{
							// prefer left
							leftWeight += 4;
							rightWeight -= 4;
						}
					}

					states[0].weight = leftWeight;
					states[1].weight = rightWeight;
				}
				else
				{
					states[0].weight = -99;
					states[1].weight = -99;
				}

				// now weight all possible states
				if (bits & COPROBE_JUMP)
				{
					int jumpWeight = 0;
					if (CanJumpUp(directionNormal))
						jumpWeight += 10;

					if (m_destOrigin.z >= pev->origin.z + 18.0f)
						jumpWeight += 5;

					if (IsVisible(m_destOrigin, GetEntity()))
					{
						MakeVectors(m_moveAngles);

						src = EyePosition();
						src = src + g_pGlobals->v_right * 15.0f;

						TraceHull(src, m_destOrigin, TraceIgnore::Nothing, point_hull, GetEntity(), &tr);
						if (tr.flFraction >= 1.0f)
						{
							src = EyePosition();
							src = src - g_pGlobals->v_right * 15.0f;

							TraceHull(src, m_destOrigin, TraceIgnore::Nothing, point_hull, GetEntity(), &tr);
							if (tr.flFraction >= 1.0f)
								jumpWeight += 5;
						}
					}

					if (isDucking)
						src = pev->origin;
					else
						src = pev->origin + Vector(0.0f, 0.0f, -17.0f);

					dest = src + directionNormal * 30.0f;
					TraceHull(src, dest, TraceIgnore::Nothing, point_hull, GetEntity(), &tr);

					if (tr.flFraction < 1.0f)
						jumpWeight += 10;

					if (onLadder)
						jumpWeight += 3;

					states[2].weight = jumpWeight;
				}
				else
					states[2].weight = -99;

				if (bits & COPROBE_DUCK)
				{
					int duckWeight = 0;
					if (CanDuckUnder(directionNormal))
						duckWeight += 10;

					if ((m_destOrigin.z + 36.0f <= pev->origin.z) && IsVisible(m_destOrigin, GetEntity()))
						duckWeight += 5;

					if (m_waypoint.flags & WAYPOINT_CROUCH)
						duckWeight += 8;

					if (m_isZombieBot && isDucking)
						duckWeight += 3;

					if (m_stuckCause == STUCK_CAUSE_ENEMY && m_isZombieBot)
						duckWeight += 6;

					states[3].weight = duckWeight;
				}
				else
					states[3].weight = -99;

				int k;
				for (k = 0; k < 4; k++)
				{
					if (m_failedMovesAtWaypoint & (1 << k))
						states[k].weight -= 8;
				}

				if (m_stuckCause == STUCK_CAUSE_OSCILLATION)
				{
					// boost strafe weight for oscillation resolution
					states[0].weight += 3;
					states[1].weight += 3;
				}

				bool isSorting = false;
				do
				{
					isSorting = false;
					for (k = 0; k < 3; k++)
					{
						if (states[k].weight < states[k + 1].weight)
						{
							ProbeState temp = states[k];
							states[k] = states[k + 1];
							states[k + 1] = temp;
							isSorting = true;
						}
					}
				} while (isSorting);

				for (k = 0; k < 4; k++)
					m_collideMoves[k] = states[k].move;

				m_collideTime = time2;

				const int bestMove = m_collideMoves[0];
				if (bestMove == COSTATE_JUMP)
					m_probeTime = time2 + crandomfloat(0.4f, 0.8f);
				else if (bestMove == COSTATE_DUCK)
					m_probeTime = time2 + crandomfloat(0.6f, 1.2f);
				else
					m_probeTime = time2 + crandomfloat(0.5f, 1.0f);

				m_collisionProbeBits = bits;
				m_collisionState = COSTATE_PROBING;
				m_collStateIndex = 0;
			}
		}
		else if (m_collisionState == COSTATE_PROBING)
		{
			if (m_probeTime < time2)
			{
				if (m_collStateIndex < 3)
				{
					int moveIdx = -1;
					const int failedMove = m_collideMoves[m_collStateIndex];

					if (failedMove == COSTATE_STRAFELEFT)
						moveIdx = 0;
					else if (failedMove == COSTATE_STRAFERIGHT)
						moveIdx = 1;
					else if (failedMove == COSTATE_JUMP)
						moveIdx = 2;
					else if (failedMove == COSTATE_DUCK)
						moveIdx = 3;

					if (moveIdx >= 0)
						m_failedMovesAtWaypoint |= (1 << moveIdx);
				}

				m_collStateIndex++;

				if (m_collStateIndex < 3)
				{
					const int nextMove = m_collideMoves[m_collStateIndex];
					if (nextMove == COSTATE_JUMP)
						m_probeTime = time2 + crandomfloat(0.4f, 0.8f);
					else if (nextMove == COSTATE_DUCK)
						m_probeTime = time2 + crandomfloat(0.6f, 1.2f);
					else
						m_probeTime = time2 + crandomfloat(0.5f, 1.0f);
				}

				if (m_collStateIndex >= 3)
					ResetCollideState();
			}
			else if (m_collStateIndex < 3)
			{
				switch (m_collideMoves[m_collStateIndex])
				{
					case COSTATE_JUMP:
					{
						if (pev->maxspeed < 20.0f)
							break;

						if (m_isZombieBot && isDucking)
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
						if (pev->maxspeed < 20.0f)
							break;

						m_buttons |= IN_DUCK;

						// --- SECTION 3: enemy cause — zombie duck + forward push ---
						if (m_isZombieBot && (m_stuckTime > 1.5f || m_stuckCause == STUCK_CAUSE_ENEMY))
							m_moveSpeed = pev->maxspeed;

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
		if (m_stuckTime > 1.0f)
		{
			m_lastStuckOrigin = pev->origin;
			m_stuckCooldownUntil = time2 + 4.0f;
		}

		if (m_currentWaypointIndex != m_lastStuckWaypointIndex)
			m_failedMovesAtWaypoint = 0;

		m_isOscillating = false;
		m_posHistoryCount = 0;
		m_posHistoryIdx = 0;

		m_wasStuck = m_isStuck;

		if (m_lastStuckWaypointIndex != m_currentWaypointIndex)
		{
			m_waypointStuckCount = 0;
			m_lastStuckWaypointIndex = -1;
		}

		m_stuckTime -= finterval;
		if (m_stuckTime < 0.0f)
			m_stuckTime = 0.0f;

		if (m_isZombieBot && m_waypoint.flags & WAYPOINT_DJUMP && onFloor)
		{
			const float boostRadius = m_waypoint.radius ? static_cast<float>(m_waypoint.radius) + 20.0f : 54.0f;
			if ((pev->origin - m_waypoint.origin).GetLengthSquared() < squaredf(boostRadius))
				m_buttons |= IN_DUCK;
		}
		else
		{
			if (m_probeTime + 1.0f < time2)
				ResetCollideState();
			else if (m_collStateIndex < 3)
			{
				if (m_collideMoves[m_collStateIndex] == COSTATE_DUCK && (onFloor || inWater))
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

	for (int &collide : m_collideMoves)
		collide = 0;
}

void Bot::SetWaypointOrigin(void)
{
	if (m_currentTravelFlags & PATHFLAG_JUMP || m_waypoint.flags & WAYPOINT_FALLRISK || m_isStuck)
	{
		m_waypointOrigin = m_waypoint.origin;
		if (!IsOnLadder() && !IsInWater())
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
			// until we find better alternative just use this m_avoid
			if (m_hasFriendsNear)
			{
				if (!ebot_has_semiclip.GetBool() && !FNullEnt(m_avoid))
				{
					if (m_isStuck)
						MakeVectors((m_waypoint.origin - pev->origin).ToAngles());
					else
						MakeVectors((g_waypoint->m_paths[m_navNode.Next()].origin - m_waypoint.origin).ToAngles());

					const Vector forward = g_pGlobals->v_forward;
					const Vector right = Vector(-forward.y, forward.x, 0.0f);

					// if we need to avoid someone, move to the side instead of blocking
					if (!FNullEnt(m_avoid))
					{
						const Bot *avoidBot = g_botManager->GetBot(m_avoid);
						Vector toAvoid;

						if (avoidBot) // we know exactly where they're going, use their target position to predict their path
							toAvoid = (avoidBot->m_waypointOrigin - m_waypoint.origin).Normalize2D();
						else // guess based on current position
							toAvoid = (m_avoid->v.origin - m_waypoint.origin).Normalize2D();

						// check which side is better to avoid (hitbox + margin)
						const float sideOffset = ((toAvoid | right) > 0.0f) ? -48.0f : 48.0f;
						m_waypointOrigin.x = m_waypoint.origin.x + forward.x * radius + right.x * sideOffset;
						m_waypointOrigin.y = m_waypoint.origin.y + forward.y * radius + right.y * sideOffset;
						m_waypointOrigin.z = m_waypoint.origin.z;
					}
					else
					{
						const Vector direction = (forward * (1.0f - cabsf(m_sidePreference) * 0.5f) + Vector(-forward.y, forward.x, 0.0f) * m_sidePreference).Normalize2D();
						m_waypointOrigin.x = m_waypoint.origin.x + direction.x * radius;
						m_waypointOrigin.y = m_waypoint.origin.y + direction.y * radius;
						m_waypointOrigin.z = m_waypoint.origin.z;
					}
				}
				else
				{
					if (m_isStuck)
						MakeVectors((m_waypoint.origin - pev->origin).ToAngles());
					else
						MakeVectors((g_waypoint->m_paths[m_navNode.Next()].origin - m_waypoint.origin).ToAngles());

					const Vector forward = g_pGlobals->v_forward;
					const Vector direction = (forward * (1.0f - cabsf(m_sidePreference) * 0.5f) + Vector(-forward.y, forward.x, 0.0f) * m_sidePreference).Normalize2D();
					m_waypointOrigin.x = m_waypoint.origin.x + direction.x * radius;
					m_waypointOrigin.y = m_waypoint.origin.y + direction.y * radius;
					m_waypointOrigin.z = m_waypoint.origin.z;
				}
			}
			else
			{
				if (m_isStuck)
					MakeVectors((m_waypoint.origin - pev->origin).ToAngles());
				else
					MakeVectors((g_waypoint->m_paths[m_navNode.Next()].origin - m_waypoint.origin).ToAngles());

				const Vector forward = g_pGlobals->v_forward;
				m_waypointOrigin.x = m_waypoint.origin.x + forward.x * radius;
				m_waypointOrigin.y = m_waypoint.origin.y + forward.y * radius;
				m_waypointOrigin.z = m_waypoint.origin.z;
			}
		}
		else
		{
			m_waypointOrigin.x = m_waypoint.origin.x + crandomfloat(-radius, radius);
			m_waypointOrigin.y = m_waypoint.origin.y + crandomfloat(-radius, radius);
			m_waypointOrigin.z = m_waypoint.origin.z;

			if (m_hasFriendsNear)
			{
				const float range = static_cast<float>(m_numFriendsNear * m_numFriendsNear);
				m_waypointOrigin.x += crandomfloat(-range, range);
				m_waypointOrigin.y += crandomfloat(-range, range);
			}
		}
	}
	else
		m_waypointOrigin = m_waypoint.origin;

	m_waypointDistance = (pev->origin - m_waypointOrigin).GetLengthSquared();
}

void Bot::ChangeWptIndex(const int16_t waypointIndex)
{
	if (!IsValidWaypoint(waypointIndex))
		return;

	m_currentWaypointIndex = waypointIndex;
	if (m_prevWptIndex[0] != m_currentWaypointIndex)
	{
		m_prevWptIndex[3] = m_prevWptIndex[2];
		m_prevWptIndex[2] = m_prevWptIndex[1];
		m_prevWptIndex[1] = m_prevWptIndex[0];
		m_prevWptIndex[0] = m_currentWaypointIndex;
	}

	m_waypoint = g_waypoint->m_paths[waypointIndex];
	SetWaypointOrigin();
	const float speed = pev->velocity.GetLength();
	if (speed > 10.0f)
		m_waypointTime = engine->GetTime() + cclampf(GetVectorDistanceSSE(pev->origin, m_destOrigin) / speed, 3.0f, 12.0f);
	else
		m_waypointTime = engine->GetTime() + 12.0f;
}

// checks if bot is blocked in his movement direction (excluding doors)
bool Bot::CantMoveForward(const Vector &normal)
{
	// first do a trace from the bot's eyes forward...
	const Vector eyePosition = EyePosition();
	Vector src = eyePosition;
	Vector forward = src + normal * 24.0f;

	MakeVectors(Vector(0.0f, pev->angles.y, 0.0f));
	TraceResult tr;

	// trace from the bot's eyes straight forward...
	TraceLine(src, forward, TraceIgnore::Nothing, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f)
	{
		if (!cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
			return false;

		return true; // bot's head will hit something
	}

	// bot's head is clear, check at shoulder level... trace from the bot's shoulder left diagonal forward to the right shoulder...
	src = eyePosition + Vector(0.0f, 0.0f, -16.0f) - g_pGlobals->v_right * 16.0f;
	forward = eyePosition + Vector(0.0f, 0.0f, -16.0f) + g_pGlobals->v_right * 16.0f + normal * 24.0f;

	TraceLine(src, forward, TraceIgnore::Nothing, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
		return true; // bot's body will hit something

	// bot's head is clear, check at shoulder level... trace from the bot's shoulder right diagonal forward to the left shoulder...
	src = eyePosition + Vector(0.0f, 0.0f, -16.0f) + g_pGlobals->v_right * 16.0f;
	forward = eyePosition + Vector(0.0f, 0.0f, -16.0f) - g_pGlobals->v_right * 16.0f + normal * 24.0f;

	TraceLine(src, forward, TraceIgnore::Nothing, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
		return true; // bot's body will hit something

	// now check below waist
	if ((pev->flags & FL_DUCKING))
	{
		src = pev->origin + Vector(0.0f, 0.0f, -19.0f + 19.0f);
		forward = src + Vector(0.0f, 0.0f, 10.0f) + normal * 24.0f;

		TraceLine(src, forward, TraceIgnore::Nothing, GetEntity(), &tr);

		// check if the trace hit something...
		if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
			return true; // bot's body will hit something

		src = pev->origin;
		forward = src + normal * 24.0f;

		TraceLine(src, forward, TraceIgnore::Nothing, GetEntity(), &tr);

		// check if the trace hit something...
		if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
			return true; // bot's body will hit something
	}
	else
	{
		// trace from the left waist to the right forward waist pos
		src = pev->origin + Vector(0.0f, 0.0f, -17.0f) - g_pGlobals->v_right * 16.0f;
		forward = pev->origin + Vector(0.0f, 0.0f, -17.0f) + g_pGlobals->v_right * 16.0f + normal * 24.0f;

		// trace from the bot's waist straight forward...
		TraceLine(src, forward, TraceIgnore::Nothing, GetEntity(), &tr);

		// check if the trace hit something...
		if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
			return true; // bot's body will hit something

		// trace from the left waist to the right forward waist pos
		src = pev->origin + Vector(0.0f, 0.0f, -17.0f) + g_pGlobals->v_right * 16.0f;
		forward = pev->origin + Vector(0.0f, 0.0f, -17.0f) - g_pGlobals->v_right * 16.0f + normal * 24.0f;

		TraceLine(src, forward, TraceIgnore::Nothing, GetEntity(), &tr);

		// check if the trace hit something...
		if (tr.flFraction < 1.0f && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
			return true; // bot's body will hit something
	}

	return false; // bot can move forward, return false
}

bool Bot::CanJumpUp(const Vector &normal)
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
	TraceLine(src, dest, TraceIgnore::Nothing, GetEntity(), &tr);

	if (tr.flFraction < 1.0f)
		goto CheckDuckJump;
	else
	{
		// now trace from jump height upward to check for obstructions...
		src = dest;
		dest.z = dest.z + 37.0f;

		TraceLine(src, dest, TraceIgnore::Nothing, GetEntity(), &tr);
		if (tr.flFraction < 1.0f)
			return false;
	}

	// now check same height to one side of the bot...
	src = pev->origin + g_pGlobals->v_right * 16.0f + Vector(0.0f, 0.0f, -36.0f + 45.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, TraceIgnore::Nothing, GetEntity(), &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		goto CheckDuckJump;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, TraceIgnore::Nothing, GetEntity(), &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now check same height on the other side of the bot...
	src = pev->origin + (-g_pGlobals->v_right * 16.0f) + Vector(0.0f, 0.0f, -36.0f + 45.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, TraceIgnore::Nothing, GetEntity(), &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		goto CheckDuckJump;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, TraceIgnore::Nothing, GetEntity(), &tr);

	// if trace hit something, return false
	return tr.flFraction >= 1.0f;

	// here we check if a duck jump would work...
CheckDuckJump:
	// use center of the body first... maximum duck jump height is 62, so check one unit above that (63)
	src = pev->origin + Vector(0.0f, 0.0f, -36.0f + 63.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, TraceIgnore::Nothing, GetEntity(), &tr);

	if (tr.flFraction < 1.0f)
		return false;
	else
	{
		// now trace from jump height upward to check for obstructions...
		src = dest;
		dest.z = dest.z + 37.0f;

		TraceLine(src, dest, TraceIgnore::Nothing, GetEntity(), &tr);

		// if trace hit something, check duckjump
		if (tr.flFraction < 1.0f)
			return false;
	}

	// now check same height to one side of the bot...
	src = pev->origin + g_pGlobals->v_right * 16.0f + Vector(0.0f, 0.0f, -36.0f + 63.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, TraceIgnore::Nothing, GetEntity(), &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, TraceIgnore::Nothing, GetEntity(), &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now check same height on the other side of the bot...
	src = pev->origin + (-g_pGlobals->v_right * 16.0f) + Vector(0.0f, 0.0f, -36.0f + 63.0f);
	dest = src + normal * 32.0f;

	// trace a line forward at maximum jump height...
	TraceLine(src, dest, TraceIgnore::Nothing, GetEntity(), &tr);

	// if trace hit something, return false
	if (tr.flFraction < 1.0f)
		return false;

	// now trace from jump height upward to check for obstructions...
	src = dest;
	dest.z = dest.z + 37.0f;

	TraceLine(src, dest, TraceIgnore::Nothing, GetEntity(), &tr);

	// if trace hit something, return false
	return tr.flFraction >= 1.0f;
}

// this function check if bot can duck under obstacle
bool Bot::CanDuckUnder(const Vector &normal)
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
	return tr.flFraction >= 1.0f;
}

bool Bot::CheckWallOnForward(void)
{
	TraceResult tr;
	MakeVectors(pev->angles);

	// do a trace to the forward...
	TraceLine(pev->origin, pev->origin + g_pGlobals->v_forward * 54.0f, TraceIgnore::Nothing, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f)
		return true;
	else
	{
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, TraceIgnore::Nothing, GetEntity(), &tr);

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
	TraceLine(pev->origin, pev->origin - g_pGlobals->v_forward * 54.0f, TraceIgnore::Nothing, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f)
		return true;
	else
	{
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, TraceIgnore::Nothing, GetEntity(), &tr);

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
	TraceLine(pev->origin, pev->origin - g_pGlobals->v_right * 54.0f, TraceIgnore::Nothing, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f)
		return true;
	else
	{
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, TraceIgnore::Nothing, GetEntity(), &tr);

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
	TraceLine(pev->origin, pev->origin + g_pGlobals->v_right * 54.0f, TraceIgnore::Nothing, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction < 1.0f)
		return true;
	else
	{
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f, TraceIgnore::Nothing, GetEntity(), &tr);

		// we don't want to fall
		if (tr.flFraction >= 1.0f)
			return true;
	}

	return false;
}

// this function eturns if given location would hurt Bot with falling damage
bool Bot::IsDeadlyDrop(const Vector &targetOriginPos)
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

	TraceHull(check, down, TraceIgnore::Monsters, head_hull, GetEntity(), &tr);
	if (tr.flFraction > 0.036f) // We're not on ground anymore?
		tr.flFraction = 0.036f;

	float height;
	float lastHeight = tr.flFraction * 1000.0f; // height from ground

	float distance = (targetOriginPos - check).GetLengthSquared(); // distance from goal
	while (distance > squaredf(16.0f) && tries < 1000)
	{
		check = check + direction * 16.0f; // move 16 units closer to the goal...

		down = check;
		down.z = down.z - 1000.0f; // straight down 1000 units

		TraceHull(check, down, TraceIgnore::Monsters, head_hull, GetEntity(), &tr);
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

void Bot::LookAt(const Vector &origin, const Vector &velocity)
{
	m_updateLooking = true;
	m_lookAt = origin;
	m_lookVelocity = velocity;
}

void Bot::SetStrafeSpeed(const Vector &moveDir, const float strafeSpeed)
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

void Bot::SetStrafeSpeedNoCost(const Vector &moveDir, const float strafeSpeed)
{
	MakeVectors(pev->angles);
	if (((moveDir - pev->origin).Normalize2D() | g_pGlobals->v_forward.SkipZ()) > 0.0f)
		m_strafeSpeed = strafeSpeed;
	else
		m_strafeSpeed = -strafeSpeed;
}

bool Bot::IsWaypointOccupied(const int16_t index)
{
	if (ebot_has_semiclip.GetBool())
		return false;

	if (pev->solid == SOLID_NOT)
		return false;

	Path *pointer;
	Bot *bot;
	for (const Clients &client : g_clients)
	{
		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == GetEntity())
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

// this function tries to find nearest to current bot button, and returns
// pointer to it's entity, also here must be specified the target, that button must open
edict_t *Bot::FindNearestButton(const char *className)
{
	if (IsNullString(className))
		return nullptr;

	float nearestDistance = 65355.0f;
	edict_t *searchEntity = nullptr;
	edict_t *foundEntity = nullptr;
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
