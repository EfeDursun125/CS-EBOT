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

ConVar ebot_aimbot("ebot_aimbot", "0");
ConVar ebot_aim_boost_in_zm("ebot_zm_aim_boost", "1");
ConVar ebot_zombies_as_path_cost("ebot_zombie_count_as_path_cost", "1");
ConVar ebot_ping_affects_aim("ebot_ping_affects_aim", "0");
ConVar ebot_aim_type("ebot_aim_type", "2");
ConVar ebot_path_smoothing("ebot_path_smoothing", "0");

int Bot::FindGoal(void)
{
	if (IsZombieMode())
	{
		if (m_isZombieBot)
		{
			if (g_waypoint->m_terrorPoints.IsEmpty())
				return m_chosenGoalIndex = engine->RandomInt(0, g_numWaypoints - 1);

			Array <int> Important;
			for (int i = 0; i < g_waypoint->m_terrorPoints.GetElementNumber(); i++)
			{
				int index;
				g_waypoint->m_terrorPoints.GetAt(i, index);
				Important.Push(index);
			}

			if (!Important.IsEmpty())
				return m_chosenGoalIndex = Important.GetRandomElement();
		}
		else if (IsValidWaypoint(m_myMeshWaypoint))
			return m_chosenGoalIndex = m_myMeshWaypoint;

		if (!g_waypoint->m_zmHmPoints.IsEmpty())
		{
			// if round starts always go to nearest (zombies appeared)
			if (g_DelayTimer <= engine->GetTime())
			{
				int targetWpIndex = -1;
				float distance = FLT_MAX;

				for (int i = 0; i < g_waypoint->m_zmHmPoints.GetElementNumber(); i++)
				{
					int wpIndex;
					g_waypoint->m_zmHmPoints.GetAt(i, wpIndex);
					if (IsValidWaypoint(wpIndex))
					{
						float theDistance = (pev->origin - g_waypoint->GetPath(wpIndex)->origin).GetLengthSquared2D();
						if (theDistance < distance)
						{
							distance = theDistance;
							targetWpIndex = wpIndex;
						}
					}
				}

				if (IsValidWaypoint(targetWpIndex))
					return m_chosenGoalIndex = targetWpIndex;
			}

			Array <int> ZombieWaypoints;
			for (int i = 0; i < g_waypoint->m_zmHmPoints.GetElementNumber(); i++)
			{
				int index;
				g_waypoint->m_zmHmPoints.GetAt(i, index);

				if (m_numEnemiesLeft > 0 && m_numFriendsLeft > 0 && GetNearbyEnemiesNearPosition(g_waypoint->GetPath(index)->origin, 600.0f) > GetNearbyFriendsNearPosition(g_waypoint->GetPath(index)->origin, 600.0f))
					continue;

				ZombieWaypoints.Push(index);
			}

			if (!ZombieWaypoints.IsEmpty())
				return m_chosenGoalIndex = ZombieWaypoints.GetRandomElement();

			if (!IsValidWaypoint(m_chosenGoalIndex))
			{
				int targetWpIndex = -1;
				float distance = FLT_MAX;

				for (int i = 0; i < g_waypoint->m_zmHmPoints.GetElementNumber(); i++)
				{
					int wpIndex;
					g_waypoint->m_zmHmPoints.GetAt(i, wpIndex);
					if (IsValidWaypoint(wpIndex))
					{
						if (m_numEnemiesLeft > 0 && m_numFriendsLeft > 0 && GetNearbyEnemiesNearPosition(g_waypoint->GetPath(wpIndex)->origin, 600.0f) > GetNearbyFriendsNearPosition(g_waypoint->GetPath(wpIndex)->origin, 600.0f))
							continue;

						float theDistance = (pev->origin - g_waypoint->GetPath(wpIndex)->origin).GetLengthSquared2D();

						if (theDistance < distance)
						{
							distance = theDistance;
							targetWpIndex = wpIndex;
						}
					}
				}

				if (IsValidWaypoint(targetWpIndex))
					return m_chosenGoalIndex = targetWpIndex;
			}

			if (!IsValidWaypoint(m_chosenGoalIndex))
				m_chosenGoalIndex = g_waypoint->m_zmHmPoints.GetRandomElement();

			return m_chosenGoalIndex;
		}
		else
			return m_chosenGoalIndex = engine->RandomInt(0, g_numWaypoints - 1);
	}
	else if (GetGameMode() == MODE_BASE)
	{
		if (g_mapType & MAP_DE)
		{
			if (g_bombPlanted)
			{
				const bool noTimeLeft = OutOfBombTimer();

				if (GetCurrentTask()->taskID != TASK_ESCAPEFROMBOMB)
				{
					if (noTimeLeft)
					{
						TaskComplete();
						PushTask(TASK_ESCAPEFROMBOMB, TASKPRI_ESCAPEFROMBOMB, -1, 2.0f, true);
					}
					else if (m_team == TEAM_COUNTER)
					{
						const Vector bombOrigin = g_waypoint->GetBombPosition();
						if (bombOrigin != nullvec)
						{
							if (IsBombDefusing(bombOrigin))
							{
								if (GetCurrentTask()->taskID != TASK_CAMP && GetCurrentTask()->taskID != TASK_GOINGFORCAMP)
								{
									m_chosenGoalIndex = FindDefendWaypoint(bombOrigin);
									if (IsValidWaypoint(m_chosenGoalIndex))
									{
										m_campposition = g_waypoint->GetPath(m_chosenGoalIndex)->origin;
										PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, m_chosenGoalIndex, GetBombTimeleft(), true);
										m_campButtons |= IN_DUCK;
										return m_chosenGoalIndex;
									}
								}
								else
									return m_chosenGoalIndex;
							}
							else
							{
								if (GetCurrentTask()->taskID == TASK_CAMP || GetCurrentTask()->taskID == TASK_GOINGFORCAMP)
									TaskComplete();
								else if (g_bombSayString)
								{
									ChatMessage(CHAT_PLANTBOMB);
									g_bombSayString = false;
								}

								m_chosenGoalIndex = g_waypoint->FindNearest(bombOrigin, 999999.0f);
								if (IsValidWaypoint(m_chosenGoalIndex))
									return m_chosenGoalIndex;
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
								if (GetCurrentTask()->taskID == TASK_CAMP || GetCurrentTask()->taskID == TASK_GOINGFORCAMP)
									TaskComplete();

								m_chosenGoalIndex = g_waypoint->FindNearest(bombOrigin, 999999.0f);
								if (IsValidWaypoint(m_chosenGoalIndex))
									return m_chosenGoalIndex;
							}
							else
							{
								if (GetCurrentTask()->taskID != TASK_CAMP && GetCurrentTask()->taskID != TASK_GOINGFORCAMP)
								{
									m_chosenGoalIndex = FindDefendWaypoint(bombOrigin);
									if (IsValidWaypoint(m_chosenGoalIndex))
									{
										m_campposition = g_waypoint->GetPath(m_chosenGoalIndex)->origin;
										PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, m_chosenGoalIndex, GetBombTimeleft(), true);
										m_campButtons |= IN_DUCK;
										return m_chosenGoalIndex;
									}
								}
								else
									return m_chosenGoalIndex;
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
						if (GetCurrentTask()->taskID != TASK_CAMP && GetCurrentTask()->taskID != TASK_GOINGFORCAMP)
						{
							m_chosenGoalIndex = FindDefendWaypoint(g_waypoint->GetPath(m_loosedBombWptIndex)->origin);
							if (IsValidWaypoint(m_chosenGoalIndex))
							{
								m_campposition = g_waypoint->GetPath(m_chosenGoalIndex)->origin;
								PushTask(TASK_GOINGFORCAMP, TASKPRI_GOINGFORCAMP, m_chosenGoalIndex, GetBombTimeleft(), true);
								m_campButtons |= IN_DUCK;
								return m_chosenGoalIndex;
							}
						}
						else
							return m_chosenGoalIndex;
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
							m_chosenGoalIndex = g_waypoint->m_ctPoints.GetRandomElement();
							if (IsValidWaypoint(m_chosenGoalIndex))
								return m_chosenGoalIndex;
						}
					}
					else
					{
						if (m_isBomber)
						{
							m_loosedBombWptIndex = -1;
							if (!IsValidWaypoint(m_chosenGoalIndex) || !(g_waypoint->GetPath(m_chosenGoalIndex)->flags & WAYPOINT_GOAL))
							{
								if (!g_waypoint->m_goalPoints.IsEmpty())
									return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
							}
						}
						else if (!g_waypoint->m_terrorPoints.IsEmpty())
						{
							m_chosenGoalIndex = g_waypoint->m_terrorPoints.GetRandomElement();
							if (IsValidWaypoint(m_chosenGoalIndex))
								return m_chosenGoalIndex;
						}
					}
				}
			}
		}
		else if (g_mapType & MAP_CS)
		{
			static bool ohShit;
			if (m_team == TEAM_COUNTER)
			{
				ohShit = false;
				if (!g_waypoint->m_rescuePoints.IsEmpty() && HasHostage())
				{
					ohShit = true;
					if (!IsValidWaypoint(m_chosenGoalIndex) || !(g_waypoint->GetPath(m_chosenGoalIndex)->flags & WAYPOINT_RESCUE))
						return m_chosenGoalIndex = g_waypoint->m_rescuePoints.GetRandomElement();
				}
				else
				{
					if (!g_waypoint->m_ctPoints.IsEmpty() && engine->RandomInt(1, 2) == 1)
						return m_chosenGoalIndex = g_waypoint->m_ctPoints.GetRandomElement();
					else if (!g_waypoint->m_goalPoints.IsEmpty())
						return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
				}
			}
			else
			{
				if (!g_waypoint->m_rescuePoints.IsEmpty() && (ohShit || engine->RandomInt(1, 11) == 1))
					return m_chosenGoalIndex = g_waypoint->m_rescuePoints.GetRandomElement();
				else if (!g_waypoint->m_goalPoints.IsEmpty())
					return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
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
					if (!g_waypoint->m_goalPoints.IsEmpty() && engine->RandomInt(1, 2) == 1)
						return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
					else if (!g_waypoint->m_ctPoints.IsEmpty())
						return m_chosenGoalIndex = g_waypoint->m_ctPoints.GetRandomElement();
				}
			}
			else
			{
				if (!g_waypoint->m_goalPoints.IsEmpty() && engine->RandomInt(1, 11) == 1)
					return m_chosenGoalIndex = g_waypoint->m_goalPoints.GetRandomElement();
				else if (!g_waypoint->m_terrorPoints.IsEmpty())
					return m_chosenGoalIndex = g_waypoint->m_terrorPoints.GetRandomElement();
			}
		}
	}

	if (!FNullEnt(m_lastEnemy) && IsAlive(m_lastEnemy))
	{
		const Vector origin = GetEntityOrigin(m_lastEnemy);
		if (origin != nullvec)
		{
			m_chosenGoalIndex = g_waypoint->FindNearest(origin, 9999999.0f, -1, m_lastEnemy);
			if (IsValidWaypoint(m_chosenGoalIndex))
				return m_chosenGoalIndex;
		}
	}

	if (!g_waypoint->m_otherPoints.IsEmpty())
		return m_chosenGoalIndex = g_waypoint->m_otherPoints.GetRandomElement();

	return m_chosenGoalIndex = engine->RandomInt(0, g_numWaypoints - 1);
}

bool Bot::GoalIsValid(void)
{
	int goal = GetCurrentTask()->data;

	if (!IsValidWaypoint(goal)) // not decided about a goal
		return false;
	else if (goal == m_currentWaypointIndex) // no nodes needed
		return true;
	else if (m_navNode == nullptr) // no path calculated
		return false;

	// got path - check if still valid
	PathNode* node = m_navNode;

	while (node->next != nullptr)
		node = node->next;

	if (node->index == goal)
		return true;

	return false;
}

// this function is a main path navigation
bool Bot::DoWaypointNav(void)
{
	// check if we need to find a waypoint...
	if (!IsValidWaypoint(m_currentWaypointIndex))
	{
		GetValidWaypoint();
		SetWaypointOrigin();
		m_navTimeset = engine->GetTime();
	}
	else if (m_waypointOrigin != nullvec)
	{
		m_cachedWaypointIndex = m_currentWaypointIndex;
		m_destOrigin = m_waypointOrigin;
	}

	const auto currentWaypoint = g_waypoint->GetPath(m_currentWaypointIndex);

	m_destOrigin = m_waypointOrigin;

	// this waypoint has additional travel flags - care about them
	if (m_currentTravelFlags & PATHFLAG_JUMP)
	{
		// bot is not jumped yet?
		if (!m_jumpFinished)
		{
			// cheating for jump, bots cannot do some hard jumps and double jumps too
			// who cares about double jump for bots? :)
			pev->button |= IN_JUMP;

			if (m_desiredVelocity == nullvec || m_desiredVelocity == -1)
			{
				const float timeToReachWaypoint = Q_sqrt(powf(m_waypointOrigin.x - pev->origin.x, 2.0f) + powf(m_waypointOrigin.y - pev->origin.y, 2.0f)) / pev->maxspeed;
				pev->velocity.x = (m_waypointOrigin.x - pev->origin.x) / timeToReachWaypoint;
				pev->velocity.y = (m_waypointOrigin.y - pev->origin.y) / timeToReachWaypoint;
				pev->velocity.z = 2.0f * (m_waypointOrigin.z - pev->origin.z - 0.5f * pev->gravity * powf(timeToReachWaypoint, 2.0f)) / timeToReachWaypoint;

				const float limit = (pev->maxspeed * 1.25f);
				if (pev->velocity.z > limit)
					pev->velocity.z = limit;
			}
			else
				pev->velocity = m_desiredVelocity + m_desiredVelocity * 0.05f;

			m_jumpFinished = true;
			m_checkTerrain = false;
			m_desiredVelocity = nullvec;

			PushTask(TASK_PAUSE, TASKPRI_PAUSE, -1, engine->GetTime() + engine->RandomFloat(0.48f, 0.64f), false);
		}
	}

	if (currentWaypoint->flags & WAYPOINT_LADDER || IsOnLadder())
	{
		if (IsValidWaypoint(m_prevWptIndex[0]) && g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LADDER)
		{
			if (fabsf(m_waypointOrigin.z - pev->origin.z) > 5.0f)
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
			if (client.ent == nullptr || !(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || (client.ent->v.movetype != MOVETYPE_FLY) || client.index == m_index)
				continue;

			TraceResult tr;
			bool foundGround = false;
			int previousNode = 0;

			// more than likely someone is already using our ladder...
			if ((client.ent->v.origin - currentWaypoint->origin).GetLengthSquared() <= SquaredF(40.0f))
			{
				extern ConVar ebot_ignore_enemies;
				if ((client.team != m_team || GetGameMode() == MODE_DM || GetGameMode() == MODE_NOTEAM) && !ebot_ignore_enemies.GetBool())
				{
					TraceLine(EyePosition(), client.ent->v.origin, ignore_monsters, GetEntity(), &tr);

					// bot found an enemy on his ladder - he should see him...
					if (tr.pHit == client.ent)
					{
						m_enemy = client.ent;
						m_lastEnemy = client.ent;
						m_lastEnemyOrigin = client.ent->v.origin;

						m_visibility = (VISIBILITY_HEAD | VISIBILITY_BODY);

						m_states |= STATE_SEEINGENEMY;
						m_seeEnemyTime = engine->GetTime();
						break;
					}
				}
				else
				{
					TraceHull(EyePosition(), m_destOrigin, ignore_monsters, (pev->flags & FL_DUCKING) ? head_hull : human_hull, GetEntity(), &tr);

					// someone is above or below us and is using the ladder already
					if (tr.pHit == client.ent && fabsf(pev->origin.z - client.ent->v.origin.z) > 15.0f && (client.ent->v.movetype == MOVETYPE_FLY))
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

	if (m_waypointFlags & WAYPOINT_LIFT)
	{
		if (UpdateLiftHandling())
		{
			if (!UpdateLiftStates())
				return false;
		}
		else
			return false;
	}

	TraceResult tr;

	// check if we are going through a door...
	{
		TraceLine(pev->origin, m_waypointOrigin, ignore_monsters, GetEntity(), &tr);

		if (!FNullEnt(tr.pHit) && FNullEnt(m_liftEntity) && strncmp(STRING(tr.pHit->v.classname), "func_door", 9) == 0)
		{
			// if the door is near enough...
			if ((pev->origin - GetEntityOrigin(tr.pHit)).GetLengthSquared() <= SquaredF(54.0f))
			{
				IgnoreCollisionShortly(); // don't consider being stuck

				if (!(pev->oldbuttons & IN_USE && pev->button & IN_USE))
				{
					// do not use door directrly under xash, or we will get failed assert in gamedll code
					if (g_gameVersion == CSVER_XASH)
						pev->button |= IN_USE;
					else
						MDLL_Use(tr.pHit, GetEntity()); // also 'use' the door randomly
				}
			}

			// make sure we are always facing the door when going through it
			m_aimFlags &= ~(AIM_LASTENEMY | AIM_PREDICTENEMY);
			m_canChooseAimDirection = false;

			edict_t* button = FindNearestButton(STRING(tr.pHit->v.targetname));

			// check if we got valid button
			if (!FNullEnt(button))
			{
				m_pickupItem = button;
				m_pickupType = PICKTYPE_BUTTON;

				m_navTimeset = engine->GetTime();
			}

			// if bot hits the door, then it opens, so wait a bit to let it open safely
			if (pev->velocity.GetLengthSquared2D() < SquaredF(pev->maxspeed * 0.20f) && m_timeDoorOpen < engine->GetTime())
			{
				PushTask(TASK_PAUSE, TASKPRI_PAUSE, -1, engine->GetTime() + 1.25f, false);
				m_timeDoorOpen = engine->GetTime() + 1.0f; // retry in 1 sec until door is open

				if (++m_tryOpenDoor > 2 && !FNullEnt(m_lastEnemy) && IsWalkableTraceLineClear(pev->origin, GetEntityOrigin(m_lastEnemy)))
				{
					m_seeEnemyTime = engine->GetTime() - 0.5f;

					m_states |= STATE_SEEINGENEMY;
					m_aimFlags |= AIM_ENEMY;

					m_enemy = m_lastEnemy;
					m_lastEnemyOrigin = GetEntityOrigin(m_lastEnemy);

					m_tryOpenDoor = 0;
				}
				else
					m_tryOpenDoor = 0;
			}
		}
	}

	float desiredDistance = SquaredF(8.0f) + pev->maxspeed;

	// initialize the radius for a special node type, where the node is considered to be reached
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

	const float nodeDistance = (pev->origin - m_waypointOrigin).GetLengthSquared();

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
	if (desiredDistance <= SquaredF(22.0f) && nodeDistance <= SquaredF(30.0f) && (m_waypointOrigin - (pev->origin + pev->velocity * m_frameInterval)).GetLengthSquared() >= nodeDistance)
		desiredDistance = nodeDistance + pev->maxspeed;

	// this allows us to prevent stupid bot behaviour when he reaches almost end point of this route, but some one  (other bot eg)
	// is sitting there, so the bot is unable to reach the node because of other player on it, and he starts to jumping and so on
	// here we're clearing task memory data (important!), since task executor may restart goal with one from memory, so this process
	// will go in cycle, and forcing bot to re-create new route.
	if (HasNextPath() && m_navNode->next->index == m_chosenGoalIndex && IsWaypointOccupied(m_navNode->next->index, true))
	{
		GetCurrentTask()->data = -1;
		m_currentWaypointIndex = -1;
		DeleteSearchNodes();
		return false;
	}

	if (nodeDistance <= desiredDistance)
	{
		// did we reach a destination waypoint?
		if (GetCurrentTask()->data == m_currentWaypointIndex)
			return true;
		else if (m_navNode == nullptr)
			return false;

		if (GetGameMode() == MODE_BASE && (g_mapType & MAP_DE) && g_bombPlanted && m_team == TEAM_COUNTER && GetCurrentTask()->taskID != TASK_ESCAPEFROMBOMB && GetCurrentTask()->data != -1)
		{
			const Vector bombOrigin = CheckBombAudible();

			// bot within 'hearable' bomb tick noises?
			if (bombOrigin != nullvec)
			{
				if ((bombOrigin - g_waypoint->GetPath(GetCurrentTask()->data)->origin).GetLengthSquared() > SquaredF(512.0f))
					g_waypoint->SetGoalVisited(GetCurrentTask()->data); // doesn't hear so not a good goal
			}
			else
				g_waypoint->SetGoalVisited(GetCurrentTask()->data); // doesn't hear so not a good goal
		}

		HeadTowardWaypoint(); // do the actual movement checking

		return false;
	}

	return false;
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

		IgnoreCollisionShortly();
	};

	// trace line to door
	TraceLine(pev->origin, m_waypointOrigin, true, true, GetEntity(), &tr);

	if (tr.flFraction < 1.0f && tr.pHit && strcmp(STRING(tr.pHit->v.classname), "func_door") == 0 && (m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor || m_liftState == LiftState::LookingButtonOutside) && pev->groundentity != tr.pHit)
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
		if (!FNullEnt(tr.pHit) && (strcmp(STRING(tr.pHit->v.classname), "func_door") == 0 || strcmp(STRING(tr.pHit->v.classname), "func_plat") == 0 || strcmp(STRING(tr.pHit->v.classname), "func_train") == 0) && !liftClosedDoorExists)
		{
			if ((m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor || m_liftState == LiftState::LookingButtonOutside) && tr.pHit->v.velocity.z == 0.0f)
			{
				if (fabsf(pev->origin.z - tr.vecEndPos.z) < 70.0f)
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
				int nextNode = m_navNode->next->index;

				if (IsValidWaypoint(nextNode) && (g_waypoint->GetPath(nextNode)->flags & WAYPOINT_LIFT))
				{
					TraceLine(path->origin, g_waypoint->GetPath(nextNode)->origin, true, true, GetEntity(), &tr);

					if (!FNullEnt(tr.pHit) && (strcmp(STRING(tr.pHit->v.classname), "func_door") == 0 || strcmp(STRING(tr.pHit->v.classname), "func_plat") == 0 || strcmp(STRING(tr.pHit->v.classname), "func_train") == 0))
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
					if (client.index < 0 || client.ent == nullptr || !(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == GetEntity() || FNullEnt(client.ent->v.groundentity))
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
				SetWaypointOrigin();

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
			{
				FindWaypoint();
				SetWaypointOrigin();
			}
		}
		else
		{
			FindWaypoint();
			SetWaypointOrigin();
		}

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
	int retID = m_heap[0].id;

	m_size--;
	m_heap[0] = m_heap[m_size];

	HeapSiftDown(0);
	return retID;
}

void PriorityQueue::HeapSiftDown(int subRoot)
{
	int parent = subRoot;
	int child = (2 * parent) + 1;

	HeapNode_t ref = m_heap[parent];

	while (child < m_size)
	{
		int rightChild = (2 * parent) + 2;

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
		int parent = (child - 1) / 2;

		if (m_heap[parent].priority <= m_heap[child].priority)
			break;

		HeapNode_t temp = m_heap[child];

		m_heap[child] = m_heap[parent];
		m_heap[parent] = temp;

		child = parent;
	}
}

inline const float GF_CostHuman(const int index, const int parent, const int team, const float gravity, const bool isZombie)
{
	const Path* path = g_waypoint->GetPath(index);
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
			if (client.index < 0)
				continue;

			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			float distance = (client.origin - path->origin).GetLengthSquared();
			if (distance <= SquaredF(path->radius + 64.0f))
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

		float distance = ((client.origin + client.ent->v.velocity * g_pGlobals->frametime) - waypointOrigin).GetLengthSquared();
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
			if (client.index < 0)
				continue;

			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			float distance = (client.origin - path->origin).GetLengthSquared();
			if (distance <= SquaredF(path->radius + 64.0f))
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
				if (client.index < 0)
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
			if (client.index < 0)
				continue;

			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			float distance = (client.origin - path->origin).GetLengthSquared();
			if (distance <= SquaredF(path->radius + 64.0f))
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
				if (client.index < 0)
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
			if (client.index < 0)
				continue;

			if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || team != client.team)
				continue;

			float distance = (client.origin - path->origin).GetLengthSquared();
			if (distance <= SquaredF(path->radius + 64.0f))
				return 65355.0f;
		}
	}

	// rusher bots never wait for boosting
	if (path->flags & WAYPOINT_DJUMP)
		return 65355.0f;

	const float baseCost = g_waypoint->GetPathDistance(index, team);
	if (path->flags & WAYPOINT_CROUCH)
		return baseCost;

	return engine->RandomFloat(1.0f, baseCost);
}

inline const float GF_CostNoHostage(const int index, const int parent, const int team, const float gravity, const bool isZombie)
{
	const Path* path = g_waypoint->GetPath(index);

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

	for (int i = 0; i < Const_MaxPathIndex; i++)
	{
		const int neighbour = g_waypoint->GetPath(index)->index[i];
		if (IsValidWaypoint(neighbour) && (path->connectionFlags[neighbour] & PATHFLAG_JUMP || path->connectionFlags[neighbour] & PATHFLAG_DOUBLE))
			return 65355.0f;
	}

	if (path->flags & WAYPOINT_AVOID)
		return 65355.0f;

	float baseCost = g_waypoint->GetPathDistance(index, team);
	if (g_waypoint->GetPath(index)->flags & WAYPOINT_CROUCH && g_waypoint->GetPath(index)->flags & WAYPOINT_CAMP)
		baseCost *= 2.0f;

	return baseCost;
}

inline const float HF_Distance(int start, int goal)
{
	auto sPath = g_waypoint->GetPath(start)->origin;
	auto gPath = g_waypoint->GetPath(goal)->origin;
	return (sPath - gPath).GetLengthSquared();
}

inline const float GF_CostNav(NavArea* start, NavArea* goal)
{
	return (g_navmesh->GetCenter(start) - g_navmesh->GetCenter(goal)).GetLengthSquared();
}

inline const float HF_Nav(NavArea* start, NavArea* goal)
{
	return (g_navmesh->GetCenter(start) - g_navmesh->GetCenter(goal)).GetLengthSquared();
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
	}

	if (dest == nullptr)
		dest = g_navmesh->GetNavArea(engine->RandomInt(1, g_numNavAreas - 1));

	if (start == dest)
		return;

	AStar_t waypoints[Const_MaxWaypoints];
	for (int i = 0; i < g_numNavAreas; i++)
	{
		waypoints[i].g = 0;
		waypoints[i].f = 0;
		waypoints[i].parent = -1;
		waypoints[i].state = State::New;
	}

	const float (*gcalc) (NavArea*, NavArea*) = nullptr;
	const float (*hcalc) (NavArea*, NavArea*) = nullptr;

	hcalc = HF_Nav;
	if (hcalc == nullptr)
		return;

	gcalc = GF_CostNav;
	if (gcalc == nullptr)
		return;

	// put start node into open list
	const auto srcWaypoint = &waypoints[start->index];
	srcWaypoint->g = gcalc(start, dest);
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
				currentIndex = waypoints[currentIndex].parent;
			} while (IsValidWaypoint(currentIndex));

			m_navNodeStart = m_navNode;
			m_currentWaypointIndex = m_navNodeStart->index;
			m_cachedWaypointIndex = m_currentWaypointIndex;

			NavArea* area = g_navmesh->GetNavArea(m_currentWaypointIndex);
			if (area != nullptr)
			{
				m_waypointOrigin = g_navmesh->GetClosestPosition(area, pev->origin);
				m_destOrigin = m_waypointOrigin;
			}

			return;
		}

		const auto currWaypoint = &waypoints[currentIndex];
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
			NavArea* self = pointer->connections[i];
			if (self == nullptr)
				continue;

			// calculate the F value as F = G + H
			const float g = currWaypoint->g + gcalc(pointer, self);
			const float h = hcalc(self, dest);
			const float f = g + h;

			const auto childWaypoint = &waypoints[self->index];
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
		const int index = FindWaypoint(false);
		if (IsValidWaypoint(index))
			srcIndex = index;
		else
		{
			const int secondIndex = g_waypoint->FindNearest(pev->origin + pev->velocity * m_frameInterval, 1024.0f, -1, GetEntity());
			if (IsValidWaypoint(secondIndex))
				srcIndex = secondIndex;
			else
				return;
		}
	}

	if (!IsValidWaypoint(destIndex))
		destIndex = g_waypoint->m_otherPoints.GetRandomElement();

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

	const float (*gcalc) (int, int, int, float, bool) = nullptr;
	const float (*hcalc) (int, int) = nullptr;

	hcalc = HF_Distance;
	if (hcalc == nullptr)
		return;

	if (IsZombieMode() && ebot_zombies_as_path_cost.GetBool() && !m_isZombieBot)
		gcalc = GF_CostHuman;
	else if (m_isBomber || m_isVIP || (g_bombPlanted && m_inBombZone))
	{
		// move faster...
		if (g_timeRoundMid < engine->GetTime())
			gcalc = GF_CostRusher;
		else
			gcalc = GF_CostCareful;
	}
	else if (g_bombPlanted && m_team == TEAM_COUNTER)
		gcalc = GF_CostRusher;
	else if (HasHostage())
		gcalc = GF_CostNoHostage;
	else if (m_personality == PERSONALITY_CAREFUL)
		gcalc = GF_CostCareful;
	else if (m_personality == PERSONALITY_RUSHER)
		gcalc = GF_CostRusher;
	else
		gcalc = GF_CostNormal;

	if (hcalc == nullptr)
		return;

	// put start node into open list
	const auto srcWaypoint = &waypoints[srcIndex];
	srcWaypoint->g = gcalc(srcIndex, -1, m_team, pev->gravity, m_isZombieBot);
	srcWaypoint->f = srcWaypoint->g + hcalc(srcIndex, destIndex);
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
			m_cachedWaypointIndex = m_currentWaypointIndex;
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

			int32 flags = g_waypoint->GetPath(self)->flags;
			if (flags & WAYPOINT_FALLCHECK)
			{
				TraceResult tr;
				TraceLine(g_waypoint->GetPath(self)->origin, g_waypoint->GetPath(self)->origin - Vector(0.0f, 0.0f, 60.0f), false, false, GetEntity(), &tr);
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
			const float h = hcalc(self, destIndex);
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
		FindShortestPath(srcIndex, PossiblePath.GetRandomElement());
		return;
	}
	
	FindShortestPath(srcIndex, destIndex);
}

void Bot::FindShortestPath(int srcIndex, int destIndex)
{
	// if we're stuck, find nearest waypoint
	if (!IsValidWaypoint(srcIndex))
	{
		const int index = FindWaypoint(false);
		if (IsValidWaypoint(index))
			srcIndex = index;
		else
		{
			const int secondIndex = g_waypoint->FindNearest(pev->origin + pev->velocity * m_frameInterval, 1024.0f, -1, GetEntity());
			if (IsValidWaypoint(secondIndex))
				srcIndex = secondIndex;
			else
				return;
		}
	}

	if (!IsValidWaypoint(destIndex))
		destIndex = g_waypoint->m_otherPoints.GetRandomElement();

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
	srcWaypoint->f = HF_Distance(srcIndex, destIndex);
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
			m_cachedWaypointIndex = m_currentWaypointIndex;
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

			int32 flags = g_waypoint->GetPath(self)->flags;
			if (flags & WAYPOINT_FALLCHECK)
			{
				TraceResult tr;
				TraceLine(g_waypoint->GetPath(self)->origin, g_waypoint->GetPath(self)->origin - Vector(0.0f, 0.0f, 60.0f), false, false, GetEntity(), &tr);
				if (tr.flFraction == 1.0f)
					continue;
			}
			else if (flags & WAYPOINT_SPECIFICGRAVITY)
			{
				if (pev->gravity * (1600.0f - 800.0f) < g_waypoint->GetPath(self)->campStartY)
					continue;
			}

			const float f = HF_Distance(self, destIndex);
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
		FindShortestPath(srcIndex, PossiblePath.GetRandomElement());
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
	m_chosenGoalIndex = -1;
}

void Bot::CheckTouchEntity(edict_t* entity)
{
	// If we won't be able to break it, don't try
	if (entity->v.takedamage != DAMAGE_YES)
	{
		// defuse bomb
		if (m_team == TEAM_COUNTER && strcmp(STRING(entity->v.model) + 9, "c4.mdl") == 0)
		{
			if (GetCurrentTask()->taskID != TASK_DEFUSEBOMB)
			{
				// notify team of defusing
				if (m_numFriendsLeft >= 1)
					RadioMessage(Radio_CoverMe);

				m_moveToGoal = false;
				m_checkTerrain = false;

				m_moveSpeed = 0.0f;
				m_strafeSpeed = 0.0f;

				PushTask(TASK_DEFUSEBOMB, TASKPRI_DEFUSEBOMB, -1, 0.0, false);
			}
		}

		return;
	}

	// See if it's breakable
	if (IsShootableBreakable(entity))
	{
		bool breakIt = false;

		TraceResult tr;
		TraceLine(pev->origin, m_destOrigin, false, false, GetEntity(), &tr);

		TraceResult tr2;
		TraceHull(pev->origin, m_destOrigin, false, head_hull, GetEntity(), &tr2);

		// double check
		if (tr.pHit == entity || tr2.pHit == entity)
		{
			m_breakableEntity = entity;
			m_breakable = GetBoxOrigin(entity);
			m_destOrigin = m_breakable;

			if (pev->origin.z > m_breakable.z)
				m_campButtons = IN_DUCK;
			else
				m_campButtons = pev->button & IN_DUCK;

			PushTask(TASK_DESTROYBREAKABLE, TASKPRI_SHOOTBREAKABLE, -1, 1.0f, false);

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

					Vector breakableOrigin = GetBoxOrigin(m_breakableEntity);
					TraceResult tr;
					TraceLine(bot->pev->origin, breakableOrigin, true, true, bot->GetEntity(), &tr);
					if (tr.pHit == entity)
					{
						bot->m_breakableEntity = entity;
						bot->m_breakable = breakableOrigin;

						if (bot->m_currentWeapon == WEAPON_KNIFE)
							bot->m_destOrigin = bot->m_breakable;

						if (bot->pev->origin.z > bot->m_breakable.z)
							bot->m_campButtons = IN_DUCK;
						else
							bot->m_campButtons = bot->pev->button & IN_DUCK;

						bot->PushTask(TASK_DESTROYBREAKABLE, TASKPRI_SHOOTBREAKABLE, -1, 1.0f, false);
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

				TraceResult tr;
				TraceLine(enemy->pev->origin, m_breakable, true, true, enemy->GetEntity(), &tr);
				if (tr.pHit == entity)
				{
					enemy->m_breakableEntity = entity;
					enemy->m_breakable = GetBoxOrigin(entity);

					if (enemy->pev->origin.z > enemy->m_breakable.z)
						enemy->m_campButtons = IN_DUCK;
					else
						enemy->m_campButtons = enemy->pev->button & IN_DUCK;

					enemy->PushTask(TASK_DESTROYBREAKABLE, TASKPRI_SHOOTBREAKABLE, -1, 1.0f, false);
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
		m_enemyOrigin = nullvec;
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
int Bot::FindWaypoint(bool skipLag)
{
	if (skipLag && !m_isSlowThink && IsValidWaypoint(m_cachedWaypointIndex))
		return m_cachedWaypointIndex;

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
		float distance = (pev->origin - g_waypoint->GetPath(at)->origin).GetLengthSquared();

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
		index = engine->RandomInt(0, 2);
	else if (IsValidWaypoint(lessIndex[1]))
		index = engine->RandomInt(0, 1);
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
	return selected;
}

void Bot::IgnoreCollisionShortly(void)
{
	ResetCollideState();

	m_lastCollTime = engine->GetTime() + 1.0f;
	m_checkTerrain = false;
}

Vector Smooth(const Vector& one, const Vector& second)
{
	return (one + second) * 0.5f;
}

Vector SmoothMax(const Vector& one, const Vector& second, const Vector& third, const Vector& last)
{
	return (one + second + third + last) * 0.25f;
}

Vector SmoothMaxMax(const Vector& one, const Vector& second, const Vector& third, const Vector& last, const Vector& onemore)
{
	return (one + second + third + last + onemore) * 0.20f;
}

void Bot::SetWaypointOrigin(void)
{
	const int myIndex = m_currentWaypointIndex;
	const int32 myFlags = g_waypoint->GetPath(myIndex)->flags;
	m_waypointOrigin = g_waypoint->GetPath(myIndex)->origin;
	
	if (ebot_path_smoothing.GetBool())
	{
		bool isSmooth = false;
		if (IsOnLadder())
		{
			m_aimStopTime = 0.0f;
			TraceResult tr;
			TraceLine(Vector(pev->origin.x, pev->origin.y, pev->absmin.z), m_waypointOrigin, true, true, GetEntity(), &tr);

			if (tr.flFraction < 1.0f)
				m_waypointOrigin = m_waypointOrigin + (pev->origin - m_waypointOrigin) * 0.5f + Vector(0.0f, 0.0f, 32.0f);
		}
		else if (!(myFlags & WAYPOINT_JUMP) && !(myFlags & WAYPOINT_LADDER) && !(myFlags & WAYPOINT_FALLCHECK) && !(myFlags & WAYPOINT_FALLRISK) && &m_navNode[0] != nullptr)
		{
			if (m_navNode->next != nullptr)
			{
				if (m_navNode->next->next != nullptr)
				{
					const Vector& origin = g_waypoint->GetPath(m_navNode->next->index)->origin;
					if (IsVisible(origin, GetEntity()))
					{
						const Vector& origin2 = g_waypoint->GetPath(m_navNode->next->next->index)->origin;
						if (IsVisible(origin2, GetEntity()))
						{
							Vector origin3;
							if (m_navNode->next->next->next != nullptr && IsVisible((origin3 = g_waypoint->GetPath(m_navNode->next->next->next->index)->origin), GetEntity()))
							{
								isSmooth = true;
								m_waypointOrigin = SmoothMaxMax(m_prevOrigin, m_waypointOrigin, origin, origin2, origin3);
							}
							else
							{
								isSmooth = true;
								m_waypointOrigin = SmoothMax(m_prevOrigin, m_waypointOrigin, origin, origin2);
							}
						}
						else
						{
							isSmooth = true;
							m_waypointOrigin = Smooth(m_waypointOrigin, origin);
						}
					}
				}
				else
				{
					const Vector& origin = g_waypoint->GetPath(m_navNode->next->index)->origin;
					if (IsVisible(origin, GetEntity()))
					{
						isSmooth = true;
						m_waypointOrigin = Smooth(m_waypointOrigin, origin);
					}
				}
			}
		}

		if (isSmooth)
			return;
	}

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
				float distance = (pev->origin - waypointOrigin[i]).GetLengthSquared();

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
			m_waypointOrigin = m_waypointOrigin + pev->view_ofs + g_pGlobals->v_forward;
	}
}

// checks if the last waypoint the bot was heading for is still valid
void Bot::GetValidWaypoint(void)
{
	bool needFindWaypont = false;
	if (!IsValidWaypoint(m_currentWaypointIndex))
		needFindWaypont = true;
	else if ((m_navTimeset + GetEstimatedReachTime() < engine->GetTime()))
		needFindWaypont = true;
	else
	{
		int waypointIndex1, waypointIndex2;
		int client = ENTINDEX(GetEntity()) - 1;
		waypointIndex1 = g_clients[client].wpIndex;
		waypointIndex2 = g_clients[client].wpIndex2;

		if (m_currentWaypointIndex != waypointIndex1 && m_currentWaypointIndex != waypointIndex2 && (g_waypoint->GetPath(m_currentWaypointIndex)->origin - pev->origin).GetLengthSquared() > SquaredF(600.0f) && !g_waypoint->Reachable(GetEntity(), m_currentWaypointIndex))
			needFindWaypont = true;
	}

	if (needFindWaypont)
	{
		DeleteSearchNodes();
		FindWaypoint();
	}
}

void Bot::ChangeWptIndex(int waypointIndex)
{
	if (!IsValidWaypoint(waypointIndex))
		return;

	if (m_currentWaypointIndex == waypointIndex)
		return;

	bool noCheck = false;

	// no current waypoint = no check
	if (!IsValidWaypoint(m_currentWaypointIndex))
		noCheck = true;

	m_currentWaypointIndex = waypointIndex;

	if (m_prevWptIndex[0] != m_currentWaypointIndex)
	{
		m_prevWptIndex[0] = m_currentWaypointIndex;
		m_prevWptIndex[1] = m_prevWptIndex[0];
		m_prevWptIndex[2] = m_prevWptIndex[1];
	}
	
	if (noCheck)
		return;

	m_navTimeset = engine->GetTime();

	// get the current waypoint flags
	m_waypointFlags = g_waypoint->GetPath(m_currentWaypointIndex)->flags;
}

int Bot::ChooseBombWaypoint(void)
{
	// this function finds the best goal (bomb) waypoint for CTs when searching for a planted bomb.

	if (g_waypoint->m_goalPoints.IsEmpty())
		return engine->RandomInt(0, g_numWaypoints - 1); // reliability check

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

int Bot::FindDefendWaypoint(Vector origin)
{
	// where to defend?
	if (origin == nullvec)
		return -1;

	// no camp waypoints
	if (g_waypoint->m_campPoints.IsEmpty())
		return engine->RandomInt(0, g_numWaypoints - 1);

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

// advances in our pathfinding list and sets the appropiate destination origins for this bot
bool Bot::HeadTowardWaypoint(void)
{
	GetValidWaypoint(); // check if old nodes is still reliable

	// no nodes from pathfinding?
	if (m_navNode == nullptr)
		return false;

	TraceResult tr{};

	m_navNode = m_navNode->next; // advance in list
	m_currentTravelFlags = 0; // reset travel flags (jumping etc)

	// helper to change bot's goal
	auto changeNextGoal = [&]
	{
		int newGoal = FindGoal();

		m_prevGoalIndex = newGoal;
		m_chosenGoalIndex = newGoal;

		// remember index
		GetCurrentTask()->data = newGoal;

		// do path finding if it's not the current node
		if (newGoal != m_currentWaypointIndex)
			FindPath(m_currentWaypointIndex, newGoal);
	};

	// we're not at the end of the list?
	if (m_navNode != nullptr)
	{
		if (HasNextPath() && m_navNodeStart != m_navNode)
		{
			GetBestNextWaypoint();
			m_minSpeed = pev->maxspeed;

			const auto taskID = GetCurrentTask()->taskID;

			// only if we in normal task and bomb is not planted
			if (GetGameMode() == MODE_BASE && taskID == TASK_NORMAL && !g_bombPlanted && m_personality != PERSONALITY_RUSHER && !m_isBomber && !m_isVIP && (!IsValidWaypoint(m_loosedBombWptIndex) && m_team == TEAM_TERRORIST))
			{
				m_campButtons = 0;

				const int waypoint = m_navNode->next->index;

				// if damage done higher than one
				if (g_timeRoundMid > engine->GetTime())
				{
					int cost = m_numEnemiesLeft - m_numEnemiesLeft;
					if (cost < 0)
						cost = 0;

					if (m_baseAgressionLevel < cost)
					{
						PushTask(TASK_GOINGFORCAMP, TASKPRI_MOVETOPOSITION, FindDefendWaypoint(g_waypoint->GetPath(waypoint)->origin), engine->GetTime() + (m_fearLevel * (g_timeRoundMid - engine->GetTime()) * 0.5f), true);
						m_campButtons |= IN_DUCK;
					}
				}
				else if (g_botsCanPause && !IsOnLadder() && !IsInWater() && !m_currentTravelFlags && IsOnFloor())
				{
					if (engine->RandomInt(1, 100) > (m_skill + engine->RandomInt(1, 20)))
					{
						m_minSpeed = GetWalkSpeed();
						if (m_currentWeapon == WEAPON_KNIFE)
							SelectBestWeapon();
					}
				}
			}
		}

		if (m_navNode != nullptr)
		{
			const int destIndex = m_navNode->index;

			// find out about connection flags
			if (m_currentWaypointIndex != -1)
			{
				const Path* path = g_waypoint->GetPath(m_currentWaypointIndex);

				for (int i = 0; i < Const_MaxPathIndex; i++)
				{
					if (path->index[i] == destIndex)
					{
						m_currentTravelFlags = path->connectionFlags[i];
						m_desiredVelocity = path->connectionVelocity[i];
						m_jumpFinished = false;

						break;
					}
				}

				// check if bot is going to jump
				bool willJump = false;
				float jumpDistance = 0.0f;

				Vector src = nullvec;
				Vector destination = nullvec;

				// try to find out about future connection flags
				if (m_navNode->next != nullptr)
				{
					for (int i = 0; i < Const_MaxPathIndex; i++)
					{
						if (g_waypoint->GetPath(destIndex)->index[i] == m_navNode->next->index && (g_waypoint->GetPath(destIndex)->connectionFlags[i] & PATHFLAG_JUMP))
						{
							src = g_waypoint->GetPath(destIndex)->origin;
							destination = g_waypoint->GetPath(m_navNode->next->index)->origin;

							jumpDistance = (g_waypoint->GetPath(destIndex)->origin - g_waypoint->GetPath(m_navNode->next->index)->origin).GetLength();
							willJump = true;

							break;
						}
					}
				}

				// bot not already on ladder but will be soon?
				if (g_waypoint->GetPath(destIndex)->flags & WAYPOINT_LADDER && !IsOnLadder())
				{
					// get ladder nodes used by other (first moving) bots
					for (const auto& other : g_botManager->m_bots)
					{
						// if another bot uses this ladder, wait 3 secs
						if (other != nullptr && other != this && other->m_isAlive && other->m_currentWaypointIndex == destIndex)
						{
							PushTask(TASK_PAUSE, TASKPRI_PAUSE, -1, AddTime(3.0f), false);
							return true;
						}
					}
				}
			}

			ChangeWptIndex(destIndex);
		}
	}

	// if wayzone radius non zero vary origin a bit depending on the body angles
	SetWaypointOrigin();

	m_navTimeset = engine->GetTime();

	return true;
}

// checks if bot is blocked in his movement direction (excluding doors)
bool Bot::CantMoveForward(Vector normal, TraceResult* tr)
{
	// first do a trace from the bot's eyes forward...
	Vector src = EyePosition();
	Vector forward = src + normal * 24.0f;

	MakeVectors(Vector(0.0f, pev->angles.y, 0.0f));

	// trace from the bot's eyes straight forward...
	TraceLine(src, forward, true, GetEntity(), tr);

	// check if the trace hit something...
	if (tr->flFraction < 1.0f)
	{
		if (strncmp("func_door", STRING(tr->pHit->v.classname), 9) == 0)
			return false;

		return true;  // bot's head will hit something
	}

	// bot's head is clear, check at shoulder level...
	// trace from the bot's shoulder left diagonal forward to the right shoulder...
	src = EyePosition() + Vector(0.0f, 0.0f, -16.0f) - g_pGlobals->v_right * 16.0f;
	forward = EyePosition() + Vector(0.0f, 0.0f, -16.0f) + g_pGlobals->v_right * 16.0f + normal * 24.0f;

	TraceLine(src, forward, true, GetEntity(), tr);

	// check if the trace hit something...
	if (tr->flFraction < 1.0f && strncmp("func_door", STRING(tr->pHit->v.classname), 9) != 0)
		return true;  // bot's body will hit something

	 // bot's head is clear, check at shoulder level...
	 // trace from the bot's shoulder right diagonal forward to the left shoulder...
	src = EyePosition() + Vector(0.0f, 0.0f, -16.0f) + g_pGlobals->v_right * 16.0f;
	forward = EyePosition() + Vector(0.0f, 0.0f, -16.0f) - g_pGlobals->v_right * 16.0f + normal * 24.0f;

	TraceLine(src, forward, true, GetEntity(), tr);

	// check if the trace hit something...
	if (tr->flFraction < 1.0f && strncmp("func_door", STRING(tr->pHit->v.classname), 9) != 0)
		return true;  // bot's body will hit something

	 // now check below waist
	if (pev->flags & FL_DUCKING)
	{
		src = pev->origin + Vector(0.0f, 0.0f, -19.0f + 19.0f);
		forward = src + Vector(0.0f, 0.0f, 10.0f) + normal * 24.0f;

		TraceLine(src, forward, true, GetEntity(), tr);

		// check if the trace hit something...
		if (tr->flFraction < 1.0f && strncmp("func_door", STRING(tr->pHit->v.classname), 9) != 0)
			return true;  // bot's body will hit something

		src = pev->origin;
		forward = src + normal * 24.0f;

		TraceLine(src, forward, true, GetEntity(), tr);

		// check if the trace hit something...
		if (tr->flFraction < 1.0f && strncmp("func_door", STRING(tr->pHit->v.classname), 9) != 0)
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
		if (tr->flFraction < 1.0f && strncmp("func_door", STRING(tr->pHit->v.classname), 9) != 0)
			return true;  // bot's body will hit something

		 // trace from the left waist to the right forward waist pos
		src = pev->origin + Vector(0.0f, 0.0f, -17.0f) + g_pGlobals->v_right * 16.0f;
		forward = pev->origin + Vector(0.0f, 0.0f, -17.0f) - g_pGlobals->v_right * 16.0f + normal * 24.0f;

		TraceLine(src, forward, true, GetEntity(), tr);

		// check if the trace hit something...
		if (tr->flFraction < 1.0f && strncmp("func_door", STRING(tr->pHit->v.classname), 9) != 0)
			return true;  // bot's body will hit something
	}

	return false;  // bot can move forward, return false
}

bool Bot::CanJumpUp(Vector normal)
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

bool Bot::CheckWallOnBehind(void)
{
	TraceResult tr;
	MakeVectors(pev->angles);

	// do a trace to the behind...
	TraceLine(pev->origin, pev->origin - g_pGlobals->v_forward * 54.0f, false, false, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction != 1.0f)
	{
		m_lastWallOrigin = tr.flFraction;
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
		m_lastWallOrigin = tr.flFraction;
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
		m_lastWallOrigin = tr.flFraction;
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
bool Bot::IsDeadlyDrop(Vector targetOriginPos)
{
	Vector botPos = pev->origin;
	TraceResult tr;

	Vector move((targetOriginPos - botPos).ToYaw(), 0.0f, 0.0f);
	MakeVectors(move);

	Vector direction = (targetOriginPos - botPos).Normalize();  // 1 unit long
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
		check = check + direction * 16.0f; // move 10 units closer to the goal...

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
		if (client.index < 0)
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
	// predict enemy
	if (m_aimFlags & AIM_ENEMY && !FNullEnt(m_enemy))
	{
		extern ConVar ebot_ping;
		if (ebot_ping.GetBool() && ebot_ping_affects_aim.GetBool())
		{
			if (m_trackTime < engine->GetTime())
			{
				m_tempAim = m_lookAt;
				m_tempVel = m_enemy->v.velocity;
				m_trackTime = AddTime(float(m_ping[2]) * 0.0025f);
			}
			else
			{
				m_tempAim.x += m_tempVel.x * g_pGlobals->frametime;
				m_tempAim.y += m_tempVel.y * g_pGlobals->frametime;

				// careful only
				if (m_personality == PERSONALITY_CAREFUL)
					m_tempAim.z += m_tempVel.z * g_pGlobals->frametime;
			}

			// set position
			m_lookAt = m_tempAim;
		}
		else
		{
			Vector enemyVel = m_enemy->v.velocity;
			m_lookAt.x += enemyVel.x * g_pGlobals->frametime;
			m_lookAt.y += enemyVel.y * g_pGlobals->frametime;
			m_lookAt.z += enemyVel.z * g_pGlobals->frametime;
		}

		m_playerTargetTime = engine->GetTime();

		// force press attack button for human bots in zombie mode
		if (IsZombieMode() && !m_isReloading && !m_isSlowThink && !(pev->button & IN_ATTACK) && !(pev->oldbuttons & IN_ATTACK))
			pev->button |= IN_ATTACK;

		if (ebot_aimbot.GetInt() == 1)
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
	}

	const float delta = engine->GetTime() - m_aimInterval;
	m_aimInterval = engine->GetTime();

	if (ebot_aim_type.GetInt() == 2)
	{
		m_idealAngles = pev->v_angle;
		Vector direction = (m_lookAt - EyePosition()).ToAngles() + pev->punchangle;
		direction.x = -direction.x; // invert for engine

		float aimSpeed = ((m_skill * 0.054f) + 11.0f) * delta;

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

	if (FNullEnt(m_enemy) && FNullEnt(m_breakableEntity))
	{
		if (IsOnLadder() || g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_LADDER || (HasNextPath() && g_waypoint->GetPath(m_navNode->next->index)->flags & WAYPOINT_LADDER))
			m_aimStopTime = 0.0f;

		if (m_aimStopTime >= engine->GetTime())
			return;
	}
	else
		m_aimStopTime = 0.0f;

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

	if (IsZombieMode() && !m_isZombieBot && (m_isEnemyReachable || ebot_aim_boost_in_zm.GetBool()))
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
		const float accel = Clamp((stiffness * angleDiffYaw) - (damping * m_lookYawVel), -accelerate, accelerate);

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
		const float accel = Clamp(stiffness * angleDiffPitch - (damping * m_lookPitchVel), -accelerate, accelerate);

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
	if (GetGameMode() != MODE_BASE || m_team != TEAM_TERRORIST || !(g_mapType & MAP_DE))
		return -1; // don't search for bomb if the player is CT, or it's not defusing bomb

	edict_t* bombEntity = nullptr; // temporaly pointer to bomb

	// search the bomb on the map
	while (!FNullEnt(bombEntity = FIND_ENTITY_BY_CLASSNAME(bombEntity, "weaponbox")))
	{
		if (strcmp(STRING(bombEntity->v.model) + 9, "backpack.mdl") == 0)
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
		if (client.ent == nullptr || !(client.flags & (CFLAG_USED | CFLAG_ALIVE)) || client.team != m_team || client.ent == GetEntity())
			continue;

		// do not check clients far away from us
		if ((pev->origin -client.origin).GetLengthSquared() > SquaredF(320.0f))
			continue;

		if (needZeroVelocity && client.ent->v.velocity != nullvec)
			continue;

		const auto path = g_waypoint->GetPath(index);
		const float length = (client.origin - path->origin).GetLengthSquared();
		if (length < Clamp(SquaredF(path->radius) * 2.0f, SquaredF(40.0f), SquaredF(90.0f)))
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