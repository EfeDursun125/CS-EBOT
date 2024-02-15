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

ConVar ebot_zombies_as_path_cost("ebot_zombie_count_as_path_cost", "1");
ConVar ebot_has_semiclip("ebot_has_semiclip", "0");
ConVar ebot_breakable_health_limit("ebot_breakable_health_limit", "3000.0");
ConVar ebot_stuck_detect_height("ebot_stuck_detect_height", "72.0");
ConVar ebot_force_shortest_path("ebot_force_shortest_path", "0");
ConVar ebot_pathfinder_seed_min("ebot_pathfinder_seed_min", "0.5");
ConVar ebot_pathfinder_seed_max("ebot_pathfinder_seed_max", "5.0");

int Bot::FindGoal(void)
{
	if (IsZombieMode())
	{
		if (m_isZombieBot)
		{
			if (g_waypoint->m_terrorPoints.IsEmpty())
				return crandomint(0, g_numWaypoints - 1);
			else
				return g_waypoint->m_terrorPoints.Random();
		}
		else if (IsValidWaypoint(m_myMeshWaypoint))
			return m_myMeshWaypoint;
		else if (IsValidWaypoint(m_zhCampPointIndex))
			return m_zhCampPointIndex;
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
							dist = (pev->origin - g_waypoint->m_paths[temp].origin).GetLengthSquared();
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
								dist = (pev->origin - g_waypoint->m_paths[temp].origin).GetLengthSquared();
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
							dist = (pev->origin - g_waypoint->m_paths[temp].origin).GetLengthSquared();

							// at least get nearest camp spot
							if (dist > maxDist)
								continue;

							index = temp;
							maxDist = dist;
						}
					}

					if (IsValidWaypoint(index))
						return index;
					else
						return g_waypoint->m_zmHmPoints.Random();
				}
				else
					return g_waypoint->m_zmHmPoints.Random();
			}
		}

		return crandomint(0, g_numWaypoints - 1);
	}
	else if (!IsDeathmatchMode())
	{
		if (g_mapType == MAP_DE)
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
								return index;
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
								return index;
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
								return index;
						}
						else
						{
							const int index = FindDefendWaypoint(bombOrigin);
							if (IsValidWaypoint(index))
							{
								m_campIndex = index;
								SetProcess(Process::Camp, "i will defend the bomb", true, engine->GetTime() + GetBombTimeleft());
								return index;
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
						const int index = FindDefendWaypoint(g_waypoint->m_paths[m_loosedBombWptIndex].origin);
						if (IsValidWaypoint(index))
						{
							extern ConVar ebot_camp_max;
							m_campIndex = index;
							SetProcess(Process::Camp, "i will defend the dropped bomb", true, engine->GetTime() + ebot_camp_max.GetFloat());
							return index;
						}
					}
					else
						return m_loosedBombWptIndex;
				}
				else
				{
					if (m_team == Team::Counter)
					{
						if (!g_waypoint->m_ctPoints.IsEmpty())
						{
							const int index = g_waypoint->m_ctPoints.Random();
							if (IsValidWaypoint(index))
								return index;
						}
					}
					else
					{
						if (m_isBomber)
						{
							m_loosedBombWptIndex = -1;
							if (IsValidWaypoint(m_navNode.Last()) && g_waypoint->GetPath(m_navNode.Last())->flags & WAYPOINT_GOAL)
								return m_navNode.Last();
							else if (!g_waypoint->m_goalPoints.IsEmpty())
								return g_waypoint->m_goalPoints.Random();
						}
						else if (!g_waypoint->m_terrorPoints.IsEmpty())
						{
							const int index = g_waypoint->m_terrorPoints.Random();
							if (IsValidWaypoint(index))
								return index;
						}
					}
				}
			}
		}
		else if (g_mapType == MAP_CS)
		{
			if (m_team == Team::Counter)
			{
				if (!g_waypoint->m_rescuePoints.IsEmpty() && HasHostage())
				{
					if (IsValidWaypoint(m_navNode.Last()) && g_waypoint->GetPath(m_navNode.Last())->flags & WAYPOINT_RESCUE)
						return m_navNode.Last();
					else
						return g_waypoint->m_rescuePoints.Random();
				}
				else
				{
					if (!g_waypoint->m_goalPoints.IsEmpty() && (g_timeRoundMid < engine->GetTime() || crandomint(1, 3) <= 2))
						return g_waypoint->m_goalPoints.Random();
					else if (!g_waypoint->m_ctPoints.IsEmpty())
						return g_waypoint->m_ctPoints.Random();
				}
			}
			else
			{
				if (!g_waypoint->m_rescuePoints.IsEmpty() && crandomint(1, 11) == 1)
					return g_waypoint->m_rescuePoints.Random();
				else if (!g_waypoint->m_goalPoints.IsEmpty() && crandomint(1, 3) == 1)
					return g_waypoint->m_goalPoints.Random();
				else if (!g_waypoint->m_terrorPoints.IsEmpty())
					return g_waypoint->m_terrorPoints.Random();
			}
		}
		else if (g_mapType == MAP_AS)
		{
			if (m_team == Team::Counter)
			{
				if (m_isVIP && !g_waypoint->m_goalPoints.IsEmpty())
					return g_waypoint->m_goalPoints.Random();
				else
				{
					if (!g_waypoint->m_goalPoints.IsEmpty() && crandomint(1, 2) == 1)
						return g_waypoint->m_goalPoints.Random();
					else if (!g_waypoint->m_ctPoints.IsEmpty())
						return g_waypoint->m_ctPoints.Random();
				}
			}
			else
			{
				if (!g_waypoint->m_goalPoints.IsEmpty() && crandomint(1, 11) == 1)
					return g_waypoint->m_goalPoints.Random();
				else if (!g_waypoint->m_terrorPoints.IsEmpty())
					return g_waypoint->m_terrorPoints.Random();
			}
		}
	}

	if (IsAlive(m_nearestEnemy))
	{
		const Vector origin = GetEntityOrigin(m_nearestEnemy);
		if (origin != nullvec)
		{
			const int index = g_waypoint->FindNearest(origin, 9999999.0f, -1, m_nearestEnemy);
			if (IsValidWaypoint(index))
				return index;
		}
	}

	return crandomint(0, g_numWaypoints - 1);
}

void Bot::MoveTo(const Vector& targetPosition)
{
	const Vector directionOld = (targetPosition + pev->velocity * -m_frameInterval) - (pev->origin + pev->velocity * m_frameInterval);
	const Vector directionNormal = directionOld.Normalize2D();
	SetStrafeSpeed(directionNormal, pev->maxspeed);
	m_moveAngles = directionOld.ToAngles();
	m_moveAngles.ClampAngles();
	m_moveAngles.x = -m_moveAngles.x; // invert for engine
	m_moveSpeed = pev->maxspeed;
}

void Bot::MoveOut(const Vector& targetPosition)
{
	const Vector directionOld = (targetPosition + pev->velocity * -m_frameInterval) - (pev->origin + pev->velocity * m_frameInterval);
	const Vector directionNormal = directionOld.Normalize2D();
	SetStrafeSpeed(directionNormal, pev->maxspeed);
	m_moveAngles = directionOld.ToAngles();
	m_moveAngles.ClampAngles();
	m_moveAngles.x = -m_moveAngles.x; // invert for engine
	m_moveSpeed = -pev->maxspeed;
}

void Bot::FollowPath(void)
{
	m_strafeSpeed = 0.0f;
	m_moveSpeed = GetMaxSpeed();
	DoWaypointNav();
	CheckStuck(m_moveSpeed + cabsf(m_strafeSpeed));
	m_moveAngles = (m_destOrigin - (pev->origin + pev->velocity * m_frameInterval)).ToAngles();
	m_moveAngles.ClampAngles();
	m_moveAngles.x = -m_moveAngles.x; // invert for engine
}

// this function used after jump action to track real path
bool Bot::GetNextBestNode(void)
{
	int16_t i, link;
	const int16_t nextWaypointIndex = m_navNode.Next();
	const int16_t currentWaypointIndex = m_navNode.First();

	// check the links
	for (i = 0; i < Const_MaxPathIndex; i++)
	{
		link = m_waypoint.index[i];

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
			ChangeWptIndex(link);
			return true;
		}
	}

	return false;
}

void Bot::AutoJump(void)
{
	// cheating for jump, bots cannot do some hard jumps and double jumps too
	// who cares about double jump for bots? :)

	auto jump = [&](void)
		{
			const Vector myOrigin = GetBottomOrigin(GetEntity());
			Vector waypointOrigin = m_destOrigin;
			Vector walkableOrigin = GetWalkablePosition(waypointOrigin + pev->velocity * m_frameInterval, GetEntity(), true);

			if (m_waypoint.flags & WAYPOINT_CROUCH)
				waypointOrigin.z -= 18.0f;
			else
				waypointOrigin.z -= 36.0f;

			if (walkableOrigin != nullvec && (waypointOrigin - walkableOrigin).GetLengthSquared() < squaredf(8.0f))
				waypointOrigin = walkableOrigin;
			else
			{
				walkableOrigin = GetWalkablePosition(waypointOrigin + pev->velocity * -m_frameInterval, GetEntity(), true);
				if (walkableOrigin != nullvec && (waypointOrigin - walkableOrigin).GetLengthSquared() < squaredf(8.0f))
					waypointOrigin = walkableOrigin;
			}

			const float timeToReachWaypoint = csqrtf(squaredf(waypointOrigin.x - myOrigin.x) + squaredf(waypointOrigin.y - myOrigin.y) + squaredf(waypointOrigin.z - myOrigin.z)) / pev->maxspeed;
			pev->velocity.x = (waypointOrigin.x - myOrigin.x) / timeToReachWaypoint;
			pev->velocity.y = (waypointOrigin.y - myOrigin.y) / timeToReachWaypoint;
			pev->velocity.z = ((waypointOrigin.z - myOrigin.z) * pev->gravity * squaredf(timeToReachWaypoint)) / timeToReachWaypoint;
		};

	jump();
	pev->buttons |= (IN_DUCK | IN_JUMP);
	jump();
}

// this function is a main path navigation
void Bot::DoWaypointNav(void)
{
	if (!IsValidWaypoint(m_currentWaypointIndex))
	{
		if (!m_stuckWarn && IsValidWaypoint(m_navNode.First()))
			ChangeWptIndex(m_navNode.First());
		else
			FindWaypoint();
	}

	m_destOrigin = m_waypointOrigin;
	if ((m_currentTravelFlags & PATHFLAG_JUMP || m_prevTravelFlags & WAYPOINT_JUMP) && pev->origin.z < m_destOrigin.z && cabsf(m_destOrigin.z - pev->origin.z) > ebot_stuck_detect_height.GetFloat())
	{
		if (m_jumpTime + 2.0f > engine->GetTime())
		{
			if (!IsOnFloor() && !IsOnLadder())
				return;
		}
		else
		{
			if (m_isStuck)
			{
				m_currentTravelFlags = 0;
				m_prevTravelFlags = 0;
				m_navNode.Clear();
				m_currentWaypointIndex = -1;
				FindWaypoint();
			}
			else
				m_isStuck = true;
		}
	}

	if (m_waypoint.flags & WAYPOINT_FALLCHECK)
	{
		TraceResult tr{};
		const Vector origin = g_waypoint->m_paths[m_currentWaypointIndex].origin;
		TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), false, false, GetEntity(), &tr);
		if (tr.flFraction == 1.0f)
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
		TraceResult tr{};
		const Vector origin = g_waypoint->m_paths[m_currentWaypointIndex].origin;
		TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), false, false, GetEntity(), &tr);
		if (tr.flFraction == 1.0f)
		{
			ResetStuck();
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;
			return;
		}
	}

	if (m_waypoint.flags & WAYPOINT_CROUCH && (!(m_waypoint.flags & WAYPOINT_CAMP) || m_stuckWarn > 1))
	{
		// we need to cheat here...
		if (m_moveSpeed != 0.0f)
			pev->speed = m_moveSpeed;

		m_duckTime = engine->GetTime() + 1.0f;
	}

	if (m_waypoint.flags & WAYPOINT_LIFT)
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

				if (!(pev->oldbuttons & IN_USE) && !(pev->buttons & IN_USE))
				{
					m_lookAt = origin;
					// do not use door directrly under xash, or we will get failed assert in gamedll code
					if (g_gameVersion & Game::Xash)
						pev->buttons |= IN_USE;
					else if (me)
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

				if (m_tryOpenDoor++ > 2 && !FNullEnt(m_nearestEnemy) && IsWalkableLineClear(EyePosition(), m_nearestEnemy->v.origin))
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
						m_moveSpeed = 0.0f;
						m_strafeSpeed = 0.0f;
						pev->speed = 0.0f;
						return;
					}

					m_tryOpenDoor = 0;
				}
			}
		}
	}

	float distance;
	float desiredDistance;

	if (IsOnLadder() || m_waypoint.flags & WAYPOINT_LADDER)
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
		TraceResult tr{};
		bool foundGround = false;
		int previousNode = 0;
		extern ConVar ebot_ignore_enemies;
		for (const auto& client : g_clients)
		{
			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || FNullEnt(client.ent) || (client.ent->v.movetype != MOVETYPE_FLY) || client.index == (m_index - 1))
				continue;

			foundGround = false;
			previousNode = 0;

			// more than likely someone is already using our ladder...
			if ((client.ent->v.origin - m_waypoint.origin).GetLengthSquared() < squaredf(48.0f))
			{
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

		distance = (pev->origin - m_destOrigin).GetLengthSquared();
		desiredDistance = 24.0f + static_cast<float>(m_stuckWarn) + m_frameInterval;
	}
	else
	{
		distance = ((pev->origin + pev->velocity * m_frameInterval) - m_destOrigin).GetLengthSquared2D();

		if (m_currentTravelFlags & PATHFLAG_JUMP)
			desiredDistance = 0.0f;
		else
		{
			if (m_waypoint.flags & WAYPOINT_LIFT)
				desiredDistance = 54.0f + m_frameInterval;
			else if ((pev->flags & FL_DUCKING) || m_waypoint.flags & WAYPOINT_GOAL)
				desiredDistance = 24.0f + m_frameInterval;
			else if (m_waypoint.radius > 5)
				desiredDistance = static_cast<float>(m_waypoint.radius + static_cast<int>(m_stuckWarn)) + m_frameInterval;
			else
				desiredDistance = 5.0f + static_cast<float>(m_stuckWarn) + m_frameInterval;
		}
	}

	if (desiredDistance < 22.0f && distance < squaredf(30.0f) && (m_destOrigin - (pev->origin + pev->velocity * m_frameInterval)).GetLengthSquared() > distance)
		desiredDistance += distance;

	if (distance < squaredf(desiredDistance))
	{
		if (!m_currentTravelFlags)
			m_prevTravelFlags = 0;

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
					if (m_currentTravelFlags & PATHFLAG_JUMP)
					{
						// jump directly to the waypoint, otherwise we will fall...
						m_waypointOrigin = g_waypoint->m_paths[destIndex].origin;
						m_destOrigin = m_waypointOrigin;
						AutoJump();
					}

					break;
				}
			}

			ChangeWptIndex(destIndex);
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
		pev->buttons &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
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

	if (!m_navNode.IsEmpty())
	{
		// trace line down
		TraceLine(m_waypoint.origin, m_waypointOrigin + Vector(0.0f, 0.0f, -54.0f), true, true, me, &tr);

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
						TraceLine(m_waypoint.origin, pointer->origin, true, true, me, &tr);
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
					m_navNode.First() = m_prevWptIndex[0];
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
public:
	PriorityQueue(void);
	~PriorityQueue(void);
	inline bool IsEmpty(void) const { return !m_size; }
	inline int16_t Size(void) const { return m_size; }
	inline void InsertLowest(const int16_t value, const float priority);
	inline void InsertHighest(const int16_t value, const float priority);
	inline int16_t RemoveLowest(void);
	inline int16_t RemoveHighest(void);
private:
	struct HeapNode
	{
		int16_t id{};
		float priority{};
	} *m_heap{};
	int16_t m_size{};
	int16_t m_heapSize{};
};

PriorityQueue::PriorityQueue(void)
{
	m_size = 0;
	m_heapSize = static_cast<int16_t>((g_numWaypoints / 2) + 32);
	m_heap = static_cast<HeapNode*>(malloc(sizeof(HeapNode) * m_heapSize));
}

PriorityQueue::~PriorityQueue(void)
{
	if (m_heap)
		free(m_heap);

	m_size = 0;
	m_heapSize = 0;
}

// inserts a value into the priority queue
void PriorityQueue::InsertLowest(const int16_t value, const float priority)
{
	if (m_size >= m_heapSize)
	{
		m_heapSize += static_cast<int16_t>(50);
		if (!m_heap)
			m_heap = static_cast<HeapNode*>(malloc(sizeof(HeapNode) * m_heapSize));
		else
		{
			HeapNode* hp = static_cast<HeapNode*>(realloc(m_heap, sizeof(HeapNode) * m_size));
			m_heap = hp;
			hp = nullptr;
		}
	}

	if (!m_heap)
		return;

	m_heap[m_size].priority = priority;
	m_heap[m_size].id = value;

	static int16_t child{};
	static int16_t parent{};
	static HeapNode temp{};

	child = ++m_size - 1;
	while (child)
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

// inserts a value into the priority queue
void PriorityQueue::InsertHighest(const int16_t value, const float priority)
{
	if (m_size >= m_heapSize)
	{
		m_heapSize += static_cast<int16_t>(50);
		if (!m_heap)
			m_heap = static_cast<HeapNode*>(malloc(sizeof(HeapNode) * m_heapSize));
		else
		{
			HeapNode* hp = static_cast<HeapNode*>(realloc(m_heap, m_size));
			m_heap = hp;
			hp = nullptr;
		}
	}

	if (!m_heap)
		return;

	m_heap[m_size].priority = priority;
	m_heap[m_size].id = value;

	static int16_t child{};
	static int16_t parent{};
	static HeapNode temp{};

	child = ++m_size - 1;
	while (child)
	{
		parent = (child - 1) / 2;
		if (m_heap[parent].priority > m_heap[child].priority)
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
	if (!m_heap)
		return -1;

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

// removes the largest item from the priority queue
int16_t PriorityQueue::RemoveHighest(void)
{
	if (!m_heap)
		return -1;

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

		if (ref.priority > m_heap[child].priority)
			break;

		m_heap[parent] = m_heap[child];
		parent = child;
		child = (2 * parent) + 1;
	}

	m_heap[parent] = ref;
	return retID;
}

inline const float HF_Distance(const int16_t& start, const int16_t& goal)
{
	return (g_waypoint->m_paths[start].origin - g_waypoint->m_paths[goal].origin).GetLength();
}

inline const float HF_Distance2D(const int16_t& start, const int16_t& goal)
{
	return (g_waypoint->m_paths[start].origin - g_waypoint->m_paths[goal].origin).GetLength2D();
}

inline const float HF_DistanceSquared(const int16_t& start, const int16_t& goal)
{
	return (g_waypoint->m_paths[start].origin - g_waypoint->m_paths[goal].origin).GetLengthSquared();
}

static Path pathCache{};
static int8_t countCache{};
inline const float GF_CostHuman(const int16_t& index, const int16_t& parent, const uint32_t& parentFlags, const int8_t& team, const float& gravity, const bool& isZombie)
{
	if (!parentFlags)
		return HF_Distance2D(index, parent);

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

	static float distance{};
	static float totalDistance{};
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
		return (HF_Distance(index, parent) * static_cast<float>(countCache)) + totalDistance;

	return HF_Distance(index, parent);
}

inline const float GF_CostCareful(const int16_t& index, const int16_t& parent, const uint32_t& parentFlags, const int8_t& team, const float& gravity, const bool& isZombie)
{
	if (!parentFlags)
		return HF_Distance2D(index, parent);

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

			return HF_Distance2D(index, parent) / static_cast<float>(countCache);
		}
	}

	return HF_Distance2D(index, parent);
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

			return HF_Distance(index, parent) / static_cast<float>(countCache);
		}
	}

	if (parentFlags & WAYPOINT_LADDER)
		return HF_Distance(index, parent) * 2.0f;

	return HF_Distance(index, parent);
}

inline const float GF_CostRusher(const int16_t& index, const int16_t& parent, const uint32_t& parentFlags, const int8_t& team, const float& gravity, const bool& isZombie)
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

	// rusher bots never wait for boosting
	if (parentFlags & WAYPOINT_DJUMP)
		return 65355.0f;

	if (parentFlags & WAYPOINT_CROUCH)
		return HF_Distance(index, parent) * 2.0f;

	return HF_Distance(index, parent);
}

inline const float GF_CostNoHostage(const int16_t& index, const int16_t& parent, const uint32_t& parentFlags, const int8_t& team, const float& gravity, const bool& isZombie)
{
	if (parentFlags & WAYPOINT_CROUCH)
		return 65355.0f;

	if (parentFlags & WAYPOINT_LADDER)
		return 65355.0f;

	int16_t neighbour;
	pathCache = g_waypoint->m_paths[index];
	for (countCache = 0; countCache < Const_MaxPathIndex; countCache++)
	{
		neighbour = pathCache.index[countCache];
		if (neighbour == parent)
		{
			if (pathCache.connectionFlags[countCache] & PATHFLAG_JUMP || pathCache.connectionFlags[countCache] & PATHFLAG_DOUBLE)
				return 65355.0f;
		}
	}

	return HF_Distance2D(index, parent);
}

// this function finds a path from srcIndex to destIndex
void Bot::FindPath(int& srcIndex, int& destIndex, edict_t* enemy)
{
	if (g_pathTimer > engine->GetTime() && !m_navNode.IsEmpty())
		return;

	if (g_gameVersion & Game::HalfLife || ebot_force_shortest_path.GetBool())
	{
		FindShortestPath(srcIndex, destIndex);
		return;
	}
	else
	{
		if (m_isBomber && !m_navNode.IsEmpty())
			return;

		if (!enemy && !FNullEnt(m_nearestEnemy))
			enemy = m_nearestEnemy;
	}

	int16_t index;
	if (!IsValidWaypoint(srcIndex))
	{
		index = FindWaypoint();
		srcIndex = index;
		ChangeWptIndex(index);
	}

	if (!IsValidWaypoint(destIndex))
		destIndex = FindGoal();

	if (srcIndex == destIndex)
		return;

	const float (*gcalc) (const int16_t&, const int16_t&, const uint32_t&, const int8_t&, const float&, const bool&) = nullptr;

	if (IsZombieMode() && ebot_zombies_as_path_cost.GetBool() && !m_isZombieBot)
		gcalc = GF_CostHuman;
	else if (g_bombPlanted && m_team == Team::Counter)
		gcalc = GF_CostRusher;
	else if (m_team == Team::Counter && (HasHostage() || m_pickupType == PickupType::Hostage))
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

	if (!gcalc)
		return;

	int16_t i;
	int16_t stuckIndex;
	Path currWay;

	if (m_isStuck)
	{
		stuckIndex = g_waypoint->FindNearestInCircle(m_stuckArea, 256.0f);
		if (srcIndex == stuckIndex)
		{
			currWay = g_waypoint->m_paths[srcIndex];
			for (i = 0; i < Const_MaxPathIndex; i++)
			{
				index = currWay.index[i];
				if (!IsValidWaypoint(index))
					continue;

				if (!IsWaypointOccupied(index))
				{
					srcIndex = index;
					break;
				}
			}
		}
	}
	else
		stuckIndex = -1;

	struct AStar
	{
		float g{};
		float f{};
		int16_t parent{};
		bool is_closed{};
	} waypoints[g_numWaypoints];

	for (i = 0; i < g_numWaypoints; i++)
	{
		waypoints[i].g = 0.0f;
		waypoints[i].f = 0.0f;
		waypoints[i].parent = -1;
		waypoints[i].is_closed = false;
	}

	// put start waypoint into open list
	AStar& srcWaypoint = waypoints[srcIndex];
	srcWaypoint.g = gcalc(srcIndex, destIndex, 0, m_team, pev->gravity, m_isZombieBot);
	srcWaypoint.f = srcWaypoint.g + HF_DistanceSquared(srcIndex, destIndex);

	// loop cache
	AStar* currWaypoint;
	AStar* childWaypoint;
	int16_t currentIndex, self;
	uint32_t flags;
	float g, f;
	bool enemyIsNull;

	if (!FNullEnt(enemy))
		enemyIsNull = false;
	else
		enemyIsNull = true;

	// do not allow to search whole map
	const int16_t limit = static_cast<int16_t>((g_numWaypoints / 2) + 24);

	PriorityQueue openList;
	openList.InsertLowest(srcIndex, srcWaypoint.f);
	while (!openList.IsEmpty())
	{
		// remove the first waypoint from the open list
		currentIndex = openList.RemoveLowest();

		// is the current waypoint the goal waypoint?
		if (currentIndex == destIndex || openList.Size() > limit)
		{
			// delete path for new one
			m_navNode.Clear();

			do
			{
				m_navNode.Add(currentIndex);
				currentIndex = waypoints[currentIndex].parent;
			} while (IsValidWaypoint(currentIndex));

			m_navNode.Reverse();
			ChangeWptIndex(m_navNode.First());
			g_pathTimer = engine->GetTime() + 0.2f;
			return;
		}

		currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->is_closed)
			continue;

		// set current waypoint as closed
		currWaypoint->is_closed = true;

		// now expand the current waypoint
		currWay = g_waypoint->m_paths[currentIndex];
		for (i = 0; i < Const_MaxPathIndex; i++)
		{
			self = currWay.index[i];
			if (!IsValidWaypoint(self))
				continue;

			if (self != destIndex)
			{
				if (stuckIndex == self)
					continue;

				if (flags = g_waypoint->m_paths[self].flags)
				{
					if (flags & WAYPOINT_FALLCHECK)
					{
						TraceResult tr{};
						const Vector origin = g_waypoint->m_paths[self].origin;
						TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), false, false, GetEntity(), &tr);
						if (tr.flFraction == 1.0f)
							continue;
					}
					else if (flags & WAYPOINT_SPECIFICGRAVITY)
					{
						if ((pev->gravity * engine->GetGravity()) > g_waypoint->m_paths[self].gravity)
							continue;
					}
					else if (!enemyIsNull)
					{
						const Vector origin = g_waypoint->GetPath(self)->origin;
						if (::IsInViewCone(origin, enemy) && IsVisible(origin, enemy))
						{
							if ((GetEntityOrigin(enemy) - origin).GetLengthSquared() - (pev->origin - origin).GetLengthSquared() < 0.0f)
								continue;
						}
					}
					/*else if (hasHostage)
					{
						if (flags & WAYPOINT_CROUCH)
							continue;

						if (flags & WAYPOINT_LADDER)
							continue;

						int8_t i;
						int16_t neighbour;
						Path path;
						for (i = 0; i < Const_MaxPathIndex; i++)
						{
							neighbour = g_waypoint->m_paths[index].index[i];
							if (neighbour == self)
							{
								path = g_waypoint->m_paths[index];
								if (path.connectionFlags[i] & PATHFLAG_JUMP || path.connectionFlags[i] & PATHFLAG_DOUBLE)
									continue;
							}
						}
					}*/
				}
			}

			g = currWaypoint->g + gcalc(currentIndex, self, flags, m_team, pev->gravity, m_isZombieBot);
			f = g + HF_Distance(self, destIndex);

			childWaypoint = &waypoints[self];
			if (!childWaypoint->is_closed || childWaypoint->f > f)
			{
				// put the current child into open list
				childWaypoint->parent = currentIndex;
				childWaypoint->is_closed = true;
				childWaypoint->g = g;
				childWaypoint->f = f;
				openList.InsertLowest(self, f);
			}
		}
	}

	// roam around poorly :(
	MiniArray <int16_t> PossiblePath;
	for (i = 0; i < g_numWaypoints; i++)
	{
		if (waypoints[i].is_closed)
			PossiblePath.Push(i);
	}

	if (!PossiblePath.IsEmpty())
	{
		index = PossiblePath.Random();
		FindShortestPath(srcIndex, destIndex);
		return;
	}

	FindShortestPath(srcIndex, destIndex);
}

void Bot::FindShortestPath(int& srcIndex, int& destIndex)
{
	int16_t i;
	if (!IsValidWaypoint(srcIndex))
	{
		i = FindWaypoint();
		srcIndex = i;
		ChangeWptIndex(i);
	}

	if (!IsValidWaypoint(destIndex))
		destIndex = FindGoal();

	if (srcIndex == destIndex)
		return;

	struct AStar
	{
		float g{};
		float f{};
		int16_t parent{};
		bool is_closed{};
	} waypoints[g_numWaypoints];

	for (i = 0; i < g_numWaypoints; i++)
	{
		waypoints[i].g = 0.0f;
		waypoints[i].f = 0.0f;
		waypoints[i].parent = -1;
		waypoints[i].is_closed = false;
	}

	AStar& srcWaypoint = waypoints[srcIndex];
	srcWaypoint.f = HF_DistanceSquared(srcIndex, destIndex);

	// loop cache
	AStar* currWaypoint;
	AStar* childWaypoint;
	int16_t currentIndex, self;
	uint32_t flags;
	Path currPath;
	float f;

	// do not allow to search whole map
	const int16_t limit = static_cast<int16_t>((g_numWaypoints / 2) + 24);

	PriorityQueue openList;
	openList.InsertLowest(srcIndex, srcWaypoint.f);
	while (!openList.IsEmpty())
	{
		// remove the first waypoint from the open list
		currentIndex = openList.RemoveLowest();
		if (!IsValidWaypoint(currentIndex))
			continue;

		// is the current waypoint the goal waypoint?
		if (currentIndex == destIndex || openList.Size() > limit)
		{
			// delete path for new one
			m_navNode.Clear();

			do
			{
				m_navNode.Add(currentIndex);
				currentIndex = waypoints[currentIndex].parent;
			} while (IsValidWaypoint(currentIndex));

			m_navNode.Reverse();
			ChangeWptIndex(m_navNode.First());
			g_pathTimer = engine->GetTime() + 0.2f;
			return;
		}

		currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->is_closed)
			continue;

		// set current waypoint as closed
		currWaypoint->is_closed = true;

		// now expand the current waypoint
		currPath = g_waypoint->m_paths[currentIndex];
		for (i = 0; i < Const_MaxPathIndex; i++)
		{
			self = currPath.index[i];
			if (!IsValidWaypoint(self))
				continue;

			if (flags = g_waypoint->m_paths[self].flags)
			{
				if (flags & WAYPOINT_FALLCHECK)
				{
					TraceResult tr{};
					const Vector origin = g_waypoint->m_paths[self].origin;
					TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), false, false, GetEntity(), &tr);
					if (tr.flFraction == 1.0f)
						continue;
				}
				else if (flags & WAYPOINT_SPECIFICGRAVITY)
				{
					if ((pev->gravity * engine->GetGravity()) > g_waypoint->m_paths[self].gravity)
						continue;
				}
			}

			f = HF_DistanceSquared(self, destIndex);
			childWaypoint = &waypoints[self];
			if (!childWaypoint->is_closed || childWaypoint->f > f)
			{
				// put the current child into open list
				childWaypoint->parent = currentIndex;
				childWaypoint->f = f;
				openList.InsertLowest(self, f);
			}
		}
	}
}

void Bot::FindEscapePath(int& srcIndex, const Vector& dangerOrigin)
{
	int16_t i;
	if (!IsValidWaypoint(srcIndex))
	{
		i = FindWaypoint();
		srcIndex = i;
		ChangeWptIndex(i);
	}

	struct AStar
	{
		float g{};
		float f{};
		int16_t parent{};
		bool is_closed{};
	} waypoints[g_numWaypoints];

	for (i = 0; i < g_numWaypoints; i++)
	{
		waypoints[i].g = 0.0f;
		waypoints[i].f = 0.0f;
		waypoints[i].parent = -1;
		waypoints[i].is_closed = false;
	}

	AStar& srcWaypoint = waypoints[srcIndex];
	srcWaypoint.f = (g_waypoint->m_paths[srcIndex].origin - dangerOrigin).GetLengthSquared();

	// loop cache
	AStar* currWaypoint;
	AStar* childWaypoint;
	int16_t currentIndex, self;
	uint32_t flags;
	Path currPath;
	float f;

	// do not allow to search whole map
	const int16_t limit = static_cast<int16_t>((g_numWaypoints / 2) + 24);

	PriorityQueue openList;
	openList.InsertLowest(srcIndex, srcWaypoint.f);
	while (!openList.IsEmpty())
	{
		// remove the first waypoint from the open list
		currentIndex = openList.RemoveHighest();
		if (!IsValidWaypoint(currentIndex))
			break;

		// now expand the current waypoint
		currPath = g_waypoint->m_paths[currentIndex];

		// is the current waypoint the goal waypoint?
		if (!IsEnemyReachableToPosition(currPath.origin) || openList.Size() > limit)
		{
			// delete path for new one
			m_navNode.Clear();

			do
			{
				m_navNode.Add(currentIndex);
				currentIndex = waypoints[currentIndex].parent;
			} while (IsValidWaypoint(currentIndex));

			m_navNode.Reverse();
			ChangeWptIndex(m_navNode.First());
			return;
		}

		currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->is_closed)
			continue;

		// set current waypoint as closed
		currWaypoint->is_closed = true;
		for (i = 0; i < Const_MaxPathIndex; i++)
		{
			self = currPath.index[i];
			if (!IsValidWaypoint(self))
				continue;

			if (flags = g_waypoint->m_paths[self].flags)
			{
				if (flags & WAYPOINT_FALLCHECK)
				{
					TraceResult tr{};
					const Vector origin = g_waypoint->m_paths[self].origin;
					TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), false, false, GetEntity(), &tr);
					if (tr.flFraction == 1.0f)
						continue;
				}
				else if (flags & WAYPOINT_SPECIFICGRAVITY)
				{
					if ((pev->gravity * engine->GetGravity()) > g_waypoint->m_paths[self].gravity)
						continue;
				}
			}

			f = (g_waypoint->m_paths[self].origin - dangerOrigin).GetLengthSquared();
			childWaypoint = &waypoints[self];
			if (!childWaypoint->is_closed || childWaypoint->f > f)
			{
				// put the current child into open list
				childWaypoint->parent = currentIndex;
				childWaypoint->f = f;
				openList.InsertHighest(self, f);
			}
		}
	}
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
		if (!me)
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

					ent = enemy->GetEntity();
					if (!ent)
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
				edict_t* ent;
				for (const auto& bot : g_botManager->m_bots)
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
					if (!ent)
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

int Bot::FindWaypoint(void)
{
	int index = FindNearestReachableWaypoint();

	if (!index)
		index = g_waypoint->FindNearest(pev->origin, 512.0f, -1, GetEntity());

	if (!index)
		index = g_waypoint->FindNearestInCircle(pev->origin);

	ChangeWptIndex(index);
	if (!m_navNode.IsEmpty())
		m_navNode.First() = index;

	return index;
}

void Bot::ResetStuck(void)
{
	m_isStuck = false;
	m_stuckWarn = 0;
	m_stuckTimer = engine->GetTime() + 1.28f;
}

void Bot::CheckStuck(const float maxSpeed)
{
	if (!ebot_has_semiclip.GetBool() && m_hasFriendsNear && !(m_waypoint.flags & WAYPOINT_FALLRISK) && !FNullEnt(m_nearestFriend))
	{
		const Vector myOrigin = pev->origin + pev->velocity * m_frameInterval;
		const Vector friendOrigin = m_friendOrigin + m_nearestFriend->v.velocity * m_frameInterval;
		if ((myOrigin - friendOrigin).GetLengthSquared() < squaredf((m_nearestFriend->v.maxspeed + pev->maxspeed) * 0.17f))
		{
			if (!m_isSlowThink && !IsOnLadder() && m_navNode.HasNext() && IsWaypointOccupied(m_navNode.First()))
			{
				Bot* bot;
				int8_t C;
				int16_t index;
				for (C = 0; C < Const_MaxPathIndex; C++)
				{
					index = g_waypoint->GetPath(m_navNode.First())->index[C];
					if (IsValidWaypoint(index) && index != m_navNode.Next() && g_waypoint->IsConnected(index, m_navNode.Next()) && !IsWaypointOccupied(index))
					{
						if (g_waypoint->Reachable(GetEntity(), index) && !IsDeadlyDrop(g_waypoint->GetPath(index)->origin))
						{
							ChangeWptIndex(index);
							m_navNode.First() = index;
						}
						else if (GetCurrentState() == Process::Default && m_hasFriendsNear && !FNullEnt(m_nearestFriend))
						{
							if ((pev->origin - m_waypoint.origin).GetLengthSquared() > (m_nearestFriend->v.origin - m_waypoint.origin).GetLengthSquared())
							{
								bot = g_botManager->GetBot(m_nearestFriend);
								if (bot)
								{
									if (bot->GetCurrentState() == Process::Default && !bot->m_navNode.IsEmpty())
									{
										ChangeWptIndex(bot->m_navNode.First());
										m_navNode.First() = bot->m_navNode.First();
									}
								}
								else if (m_nearestFriend->v.speed > (m_nearestFriend->v.maxspeed * 0.25))
								{
									index = g_waypoint->FindNearest(m_nearestFriend->v.origin + m_nearestFriend->v.velocity, 99999999.0f, -1, GetEntity());
									if (IsValidWaypoint(index))
									{
										ChangeWptIndex(index);
										m_navNode.First() = index;
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
						pev->speed = maxSpeed;
						doorStuck = true;
					}
					else
					{
						m_moveSpeed = -maxSpeed;
						pev->speed = -maxSpeed;
					}
				}
				else
				{
					if (CheckWallOnForward())
					{
						m_moveSpeed = -maxSpeed;
						pev->speed = -maxSpeed;
						doorStuck = true;
					}
					else
					{
						m_moveSpeed = maxSpeed;
						pev->speed = maxSpeed;
					}
				}
			}

			if (IsOnFloor() && m_stuckWarn > 7)
			{
				if (!(m_nearestFriend->v.buttons & IN_JUMP) && !(m_nearestFriend->v.oldbuttons & IN_JUMP) && ((m_nearestFriend->v.buttons & IN_DUCK && m_nearestFriend->v.oldbuttons & IN_DUCK) || CanJumpUp(pev->velocity.SkipZ()) || CanJumpUp(dir)))
				{
					if (m_jumpTime + 1.0f < engine->GetTime())
						pev->buttons |= IN_JUMP;
				}
				else if (!(m_nearestFriend->v.buttons & IN_DUCK) && !(m_nearestFriend->v.oldbuttons & IN_DUCK))
				{
					if (m_duckTime < engine->GetTime())
						m_duckTime = engine->GetTime() + crandomfloat(1.0f, 2.0f);
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
						if (!(pev->buttons & IN_ATTACK) && !(pev->oldbuttons & IN_ATTACK))
							pev->buttons |= IN_ATTACK;

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
	else if (m_stuckWarn && !(m_waypoint.flags & WAYPOINT_FALLRISK) && (pev->origin - m_waypoint.origin).GetLengthSquared2D() > squaredf(pev->maxspeed))
	{
		const bool left = CheckWallOnLeft();
		const bool right = CheckWallOnRight();

		if (left && right)
		{
			if (CheckWallOnForward())
			{
				m_moveSpeed = -maxSpeed;
				pev->speed = -maxSpeed;
			}
		}
		else if (left)
			m_strafeSpeed = maxSpeed;
		else if (right)
			m_strafeSpeed = -maxSpeed;
	}

	if (!m_isSlowThink)
	{
		const float time = engine->GetTime();
		if (m_stuckTimer < time)
		{
			m_stuckArea = pev->origin;
			m_stuckTimer = time + 1.28f;
		}

		return;
	}
	else if (m_stuckWarn > 2)
	{
		if (!g_waypoint->IsNodeReachable(pev->origin, m_destOrigin))
		{
			m_currentWaypointIndex = -1;
			m_navNode.Clear();
			FindWaypoint();
		}
	}

	const float distance = (pev->origin - m_stuckArea).GetLengthSquared2D();
	const float range = maxSpeed * 2.0f;
	if (distance < range)
	{
		m_stuckWarn++;

		if (m_stuckWarn > 20)
		{
			m_stuckWarn = 10;
			m_currentWaypointIndex = -1;
			m_navNode.Clear();
			FindWaypoint();
		}

		if (m_stuckWarn > 10)
		{
			// reset the connection flags
			m_currentTravelFlags = 0;
			m_prevTravelFlags = 0;
			m_isStuck = true;
		}
		else if (m_stuckWarn == 10)
			pev->buttons |= IN_JUMP;
	}
	else
	{
		const float rn = range * 4.0f;
		if (distance > rn)
		{
			if (m_stuckWarn)
			{
				if (m_stuckWarn < 5)
					m_isStuck = false;

				m_stuckWarn--;
			}

			// are we teleported? O_O
			if (!IsOnLadder() && distance > ((rn * 4.0f) - m_friendsNearCount) && !g_waypoint->IsNodeReachable(pev->origin, m_destOrigin))
			{
				// try to get new waypoint
				if (!IsValidWaypoint(m_currentWaypointIndex) || !GetNextBestNode())
				{
					m_currentWaypointIndex = -1;
					FindWaypoint();
				}
			}
		}
	}
}

void Bot::SetWaypointOrigin(void)
{
	m_waypointOrigin = m_waypoint.origin;
	if (m_waypoint.radius > 5)
	{
		const float radius = static_cast<float>(m_waypoint.radius);
		MakeVectors(Vector(pev->angles.x, AngleNormalize(pev->angles.y + crandomfloat(-90.0f, 90.0f)), 0.0f));
		int sPoint = -1;

		if (m_navNode.HasNext())
		{
			int i;
			Vector waypointOrigin[5];
			for (i = 0; i < 5; i++)
			{
				waypointOrigin[i] = m_waypointOrigin;
				waypointOrigin[i] += Vector(crandomfloat(-radius, radius), crandomfloat(-radius, radius), 0.0f);
			}

			float sDistance = 65355.0f;
			float distance;
			for (i = 0; i < 5; i++)
			{
				distance = (pev->origin - waypointOrigin[i]).GetLengthSquared();
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
}

int Bot::FindDefendWaypoint(const Vector& origin)
{
	// no camp waypoints
	if (g_waypoint->m_campPoints.IsEmpty())
		return -1;

	MiniArray <int16_t> BestSpots;
	MiniArray <int16_t> OkSpots;
	MiniArray <int16_t> WorstSpots;

	int i, index;
	TraceResult tr{};
	Path* pointer;
	for (i = 0; i < g_waypoint->m_campPoints.Size(); i++)
	{
		index = g_waypoint->m_campPoints.Get(i);
		pointer = g_waypoint->GetPath(index);
		if (!pointer)
			continue;

		if (pointer->flags & WAYPOINT_LADDER)
			continue;

		if (GetGameMode() == GameMode::Original)
		{
			if (m_team == Team::Counter && pointer->flags & WAYPOINT_TERRORIST)
				continue;
			else if (pointer->flags & WAYPOINT_COUNTER)
				continue;
		}

		if (pointer->flags & WAYPOINT_SNIPER && !IsSniper())
			continue;

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
		BestIndex = BestSpots.Random();
	else if (!OkSpots.IsEmpty())
		BestIndex = OkSpots.Random();
	else if (!WorstSpots.IsEmpty())
		BestIndex = WorstSpots.Random();
	
	if (IsValidWaypoint(BestIndex))
		return BestIndex;

	return -1;
}

// checks if bot is blocked in his movement direction (excluding doors)
bool Bot::CantMoveForward(const Vector& normal)
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

bool Bot::CanJumpUp(const Vector& normal)
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
bool Bot::IsDeadlyDrop(const Vector& targetOriginPos)
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
	float delta = engine->GetTime() - m_aimInterval;
	m_aimInterval += delta;

	if (delta > 0.05f)
		delta = 0.05f;

	// adjust all body and view angles to face an absolute vector
	Vector direction = (m_lookAt - EyePosition()).ToAngles() + pev->punchangle;
	direction.x = -direction.x; // invert for engine

	const float angleDiffPitch = AngleNormalize(direction.x - pev->v_angle.x);
	const float angleDiffYaw = AngleNormalize(direction.y - pev->v_angle.y);
	const float lockn = 0.125f / delta;

	if (angleDiffYaw < lockn && angleDiffYaw > -lockn)
	{
		m_lookYawVel = 0.0f;
		pev->v_angle.y = direction.y;
	}
	else
	{
		const float fskill = static_cast<float>(m_skill);
		const float accelerate = fskill * 40.0f;
		m_lookYawVel += delta * cclampf((fskill * 4.0f * angleDiffYaw) - (fskill * 0.4f * m_lookYawVel), -accelerate, accelerate);
		pev->v_angle.y += delta * m_lookYawVel;
	}

	if (angleDiffPitch < lockn && angleDiffPitch > -lockn)
	{
		m_lookPitchVel = 0.0f;
		pev->v_angle.x = direction.x;
	}
	else
	{
		const float fskill = static_cast<float>(m_skill);
		const float accelerate = fskill * 40.0f;
		m_lookPitchVel += delta * cclampf(2.0f * fskill * 4.0f * angleDiffPitch - (fskill * 0.4f * m_lookPitchVel), -accelerate, accelerate);
		pev->v_angle.x += delta * m_lookPitchVel;
	}

	if (pev->v_angle.x < -89.0f)
		pev->v_angle.x = -89.0f;
	else if (pev->v_angle.x > 89.0f)
		pev->v_angle.x = 89.0f;

	// set the body angles to point the gun correctly
	pev->angles.x = -pev->v_angle.x * 0.33333333333f;
	pev->angles.y = pev->v_angle.y;
}

void Bot::LookAt(const Vector& origin)
{
	m_lookAt = origin;
	FacePosition();
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
			pev->buttons |= IN_JUMP;
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

int Bot::FindHostage(void)
{
	if (m_team != Team::Counter || g_mapType != MAP_CS)
		return -1;

	edict_t* ent = nullptr;

	while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "hostage_entity")))
	{
		int16_t j;
		bool canF = true;
		for (const auto& bot : g_botManager->m_bots)
		{
			if (bot && bot->m_isAlive)
			{
				for (j = 0; j < Const_MaxHostages; j++)
				{
					if (bot->m_hostages[j] == ent)
						canF = false;
				}
			}
		}

		j = g_waypoint->FindNearest(ent->v.origin, 512.0f, -1, ent);
		if (IsValidWaypoint(j) && canF)
			return j;
		else
		{
			// do we need second try?
			j = g_waypoint->FindNearestInCircle(ent->v.origin);
			if (IsValidWaypoint(j) && canF)
				return j;
		}
	}

	return -1;
}

int Bot::FindLoosedBomb(void)
{
	if (GetGameMode() != GameMode::Original || g_mapType != MAP_DE)
		return -1;

	edict_t* bombEntity = nullptr; // temporaly pointer to bomb

	// search the bomb on the map
	while (!FNullEnt(bombEntity = FIND_ENTITY_BY_CLASSNAME(bombEntity, "weaponbox")))
	{
		if (cstrcmp(STRING(bombEntity->v.model) + 9, "backpack.mdl") == 0)
		{
			int nearestIndex = g_waypoint->FindNearest(bombEntity->v.origin, 512.0f, -1, bombEntity);
			if (IsValidWaypoint(nearestIndex))
				return nearestIndex;
			else
			{
				// do we need second try?
				nearestIndex = g_waypoint->FindNearestInCircle(bombEntity->v.origin);
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
	if (ebot_has_semiclip.GetBool())
		return false;

	if (pev->solid == SOLID_NOT)
		return false;

	Path* pointer;
	Bot* bot;
	for (const auto& client : g_clients)
	{
		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == GetEntity())
			continue;

		bot = g_botManager->GetBot(client.index);
		if (bot)
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