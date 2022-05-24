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

ConVar ebot_dangerfactor("ebot_dangerfactor", "400");
ConVar ebot_aimbot("ebot_aimbot", "0");
ConVar ebot_aiming_type("ebot_aiming_type", "1");

extern ConVar ebot_anti_block;

int Bot::FindGoal(void)
{
	// SyPB Pro P.42 - AMXX API
	if (m_waypointGoalAPI != -1)
		return m_chosenGoalIndex = m_waypointGoalAPI;

	// force bot move to bomb.
	if (GetGameMod() == MODE_BASE && (g_mapType & MAP_DE) && g_bombPlanted && m_team == TEAM_COUNTER)
	{
		if ((pev->origin - g_waypoint->GetBombPosition()).GetLength() <= 1024.0f || m_inBombZone || m_isLeader || g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_GOAL)
		{
			int index = g_waypoint->FindNearest(g_waypoint->GetBombPosition());
			if (IsValidWaypoint(index))
				return m_chosenGoalIndex = index;
		}
	}

	// we will hunt enemy or walk randomly
	if (IsDeathmatchMode() || (g_mapType & MAP_AWP) || (g_mapType & MAP_KA) || (g_mapType & MAP_FY) || (GetGameMod() == MODE_BASE && m_personality == PERSONALITY_RUSHER && ChanceOf(50) && !m_isBomber && !m_isReloading && !m_inBombZone && !g_bombPlanted && !m_isVIP && !HasHostage()))
	{
		if (!IsValidWaypoint(m_currentWaypointIndex))
			GetValidWaypoint();

		if (!FNullEnt(m_lastEnemy) && IsAlive(m_lastEnemy))
			return m_chosenGoalIndex = g_waypoint->FindNearest(GetEntityOrigin(m_lastEnemy));

		if (IsDeathmatchMode() || ChanceOf(50))
			return m_chosenGoalIndex = engine->RandomInt(0, g_numWaypoints - 1);
	}
	else if (!g_bombPlanted && (g_mapType & MAP_DE) && m_team == TEAM_COUNTER && !m_isReloading && !m_inBombZone) // ct bots must kill t bots.
	{
		if (!IsValidWaypoint(m_currentWaypointIndex))
			GetValidWaypoint();

		if (!FNullEnt(m_lastEnemy) && IsAlive(m_lastEnemy))
			return m_chosenGoalIndex = g_waypoint->FindNearest(GetEntityOrigin(m_lastEnemy));
	}

	// path finding behavior depending on map type
	int tactic;
	int offensive;
	int defensive;
	int goalDesire;

	int forwardDesire;
	int campDesire;
	int backoffDesire;
	int tacticChoice;

	Array <int> offensiveWpts;
	Array <int> defensiveWpts;

	if (GetGameMod() == MODE_BASE)
	{
		switch (m_team)
		{
		case TEAM_TERRORIST:
			offensiveWpts = g_waypoint->m_ctPoints;
			defensiveWpts = g_waypoint->m_terrorPoints;
			break;

		case TEAM_COUNTER:
			offensiveWpts = g_waypoint->m_terrorPoints;
			defensiveWpts = g_waypoint->m_ctPoints;
			break;
		}
	}
	// SyPB Pro P.30 - Zombie Mode Human Camp
	else if (GetGameMod() == MODE_ZP)
	{
		// SyPB Pro P.42 - Zombie Mode Camp improve
		if (!m_isZombieBot && !g_waypoint->m_zmHmPoints.IsEmpty())
			offensiveWpts = g_waypoint->m_zmHmPoints;
		else if (m_isZombieBot && FNullEnt(m_moveTargetEntity) && FNullEnt(m_enemy))
		{
			// SyPB Pro P.43 - Zombie improve
			int checkPoint[3];
			for (int i = 0; i < 3; i++)
				checkPoint[i] = engine->RandomInt(0, g_numWaypoints - 1);

			int movePoint = checkPoint[0];
			if (engine->RandomInt(1, 5) <= 2)
			{
				int maxEnemyNum = 0;
				for (int i = 0; i < 3; i++)
				{
					int enemyNum = GetNearbyEnemiesNearPosition(g_waypoint->GetPath(checkPoint[i])->origin, 300);
					if (enemyNum > maxEnemyNum)
					{
						maxEnemyNum = enemyNum;
						movePoint = checkPoint[i];
					}
				}
			}
			else
			{
				int playerWpIndex = GetEntityWaypoint(GetEntity());
				float maxDistance = 0.0f;
				for (int i = 0; i < 3; i++)
				{
					float distance = g_waypoint->GetPathDistanceFloat(playerWpIndex, checkPoint[i]);
					if (distance >= maxDistance)
					{
						maxDistance = distance;
						movePoint = checkPoint[i];
					}
				}
			}

			return m_chosenGoalIndex = movePoint;
		}
		else
			return m_chosenGoalIndex = engine->RandomInt(0, g_numWaypoints - 1);
	}
	// SyPB Pro P.40 - Goal Point change
	else
		return m_chosenGoalIndex = engine->RandomInt(0, g_numWaypoints - 1);

	// terrorist carrying the C4?
	if (m_isBomber || m_isVIP)
	{
		tactic = 3;
		goto TacticChoosen;
	}
	else if (HasHostage() && m_team == TEAM_COUNTER)
	{
		tactic = 2;
		offensiveWpts = g_waypoint->m_rescuePoints;

		goto TacticChoosen;
	}

	offensive = static_cast <int> (m_agressionLevel * 100);
	defensive = static_cast <int> (m_fearLevel * 100);

	// SyPB Pro P.28 - Game Mode
	if (GetGameMod() == MODE_BASE)
	{
		if (g_mapType & (MAP_AS | MAP_CS))
		{
			if (m_team == TEAM_TERRORIST)
			{
				defensive += 30;
				offensive -= 30;
			}
			else if (m_team == TEAM_COUNTER)
			{
				defensive -= 30;
				offensive += 30;
			}
		}
		else if ((g_mapType & MAP_DE))
		{
			if (m_team == TEAM_COUNTER)
			{
				if (g_bombPlanted && GetCurrentTask()->taskID != TASK_ESCAPEFROMBOMB && g_waypoint->GetBombPosition() != nullvec)
				{
					const Vector& bombPos = g_waypoint->GetBombPosition();

					if (GetBombTimeleft() >= 10.0f && IsBombDefusing(bombPos))
						return m_chosenGoalIndex = FindDefendWaypoint(bombPos);

					if (g_bombSayString)
					{
						ChatMessage(CHAT_PLANTBOMB);
						g_bombSayString = false;
					}

					return m_chosenGoalIndex = ChooseBombWaypoint();
				}

				defensive += 30;
				offensive -= 30;
			}
			else if (m_team == TEAM_TERRORIST)
			{
				defensive -= 30;
				offensive += 30;

				// send some terrorists to guard planter bomb
				if (g_bombPlanted && GetCurrentTask()->taskID != TASK_ESCAPEFROMBOMB && GetBombTimeleft() >= 15.0f)
					return m_chosenGoalIndex = FindDefendWaypoint(g_waypoint->GetBombPosition());

				float leastPathDistance = 0.0f;
				int goalIndex = -1;

				ITERATE_ARRAY(g_waypoint->m_goalPoints, i)
				{
					float realPathDistance = g_waypoint->GetPathDistanceFloat(m_currentWaypointIndex, g_waypoint->m_goalPoints[i]) + engine->RandomFloat(0.0, 128.0f);

					if (leastPathDistance > realPathDistance)
					{
						goalIndex = g_waypoint->m_goalPoints[i];
						leastPathDistance = realPathDistance;
					}
				}

				if (IsValidWaypoint(goalIndex) && !g_bombPlanted && m_isBomber)
					return m_chosenGoalIndex = goalIndex;
			}
		}
	}
	else if (IsZombieMode())
	{
		if (m_isZombieBot)
		{
			defensive += 30;
			offensive -= 30;
		}
		else
		{
			defensive -= 30;
			offensive += 30;

			// SyPB Pro P.30 - Zombie Mode Human Camp
			if (!g_waypoint->m_zmHmPoints.IsEmpty())
			{
				tactic = 4;
				goto TacticChoosen;
			}
		}
	}

	goalDesire = engine->RandomInt(0, 70) + offensive;
	forwardDesire = engine->RandomInt(0, 50) + offensive;
	campDesire = engine->RandomInt(0, 70) + defensive;
	backoffDesire = engine->RandomInt(0, 50) + defensive;

	if (UsesSniper() || ((g_mapType & MAP_DE) && m_team == TEAM_COUNTER && !g_bombPlanted) && (GetGameMod() == MODE_BASE || IsDeathmatchMode()))
		campDesire = static_cast <int> (engine->RandomFloat(1.5f, 2.5f) * static_cast <float> (campDesire));

	tacticChoice = backoffDesire;
	tactic = 0;

	if (UsesSubmachineGun())
		campDesire = static_cast <int> (campDesire * 1.5f);

	if (campDesire > tacticChoice)
	{
		tacticChoice = campDesire;
		tactic = 1;
	}
	if (forwardDesire > tacticChoice)
	{
		tacticChoice = forwardDesire;
		tactic = 2;
	}
	if (goalDesire > tacticChoice)
		tactic = 3;

TacticChoosen:
	int goalChoices[4] = {-1, -1, -1, -1};

	if (tactic == 0 && !defensiveWpts.IsEmpty()) // careful goal
	{
		for (int i = 0; i < 4; i++)
		{
			goalChoices[i] = defensiveWpts.GetRandomElement();
			InternalAssert(goalChoices[i] >= 0 && goalChoices[i] < g_numWaypoints);
		}
	}
	else if (tactic == 1 && !g_waypoint->m_campPoints.IsEmpty()) // camp waypoint goal
	{
		// pickup sniper points if possible for sniping bots
		if (!g_waypoint->m_sniperPoints.IsEmpty() && UsesSniper())
		{
			for (int i = 0; i < 4; i++)
			{
				goalChoices[i] = g_waypoint->m_sniperPoints.GetRandomElement();
				InternalAssert(goalChoices[i] >= 0 && goalChoices[i] < g_numWaypoints);
			}
		}
		else
		{
			for (int i = 0; i < 4; i++)
			{
				goalChoices[i] = g_waypoint->m_campPoints.GetRandomElement();
				InternalAssert(goalChoices[i] >= 0 && goalChoices[i] < g_numWaypoints);
			}
		}
	}
	else if (tactic == 2 && !offensiveWpts.IsEmpty()) // offensive goal
	{
		for (int i = 0; i < 4; i++)
		{
			goalChoices[i] = offensiveWpts.GetRandomElement();
			InternalAssert(goalChoices[i] >= 0 && goalChoices[i] < g_numWaypoints);
		}
	}
	else if (tactic == 3 && !g_waypoint->m_goalPoints.IsEmpty()) // map goal waypoint
	{
		bool closer = false;
		int closerIndex = 0;

		ITERATE_ARRAY(g_waypoint->m_goalPoints, i)
		{
			int closest = g_waypoint->m_goalPoints[i];

			if ((pev->origin - g_waypoint->GetPath(closest)->origin).GetLength() <= 1024.0f && !IsGroupOfEnemies(g_waypoint->GetPath(closest)->origin))
			{
				closer = true;
				goalChoices[closerIndex] = closest;

				InternalAssert(goalChoices[closerIndex] >= 0 && goalChoices[closerIndex] < g_numWaypoints);

				if (++closerIndex > 3)
					break;
			}
		}

		if (!closer)
		{
			for (int i = 0; i < 4; i++)
			{
				goalChoices[i] = g_waypoint->m_goalPoints.GetRandomElement();
				InternalAssert(goalChoices[i] >= 0 && goalChoices[i] < g_numWaypoints);
			}
		}
		else if (m_isBomber)
			return m_chosenGoalIndex = goalChoices[engine->RandomInt(0, closerIndex - 1)];
	}
	else if (tactic == 4 && !offensiveWpts.IsEmpty()) // offensive goal
	{
		int wpIndex = offensiveWpts.GetRandomElement();

		if (IsValidWaypoint(wpIndex))
			return m_chosenGoalIndex = wpIndex;
	}

	if (!IsValidWaypoint(m_currentWaypointIndex < 0))
		// SyPB Pro P.40 - Small Change
		GetValidWaypoint();

	if (!IsValidWaypoint(goalChoices[0]))
		return m_chosenGoalIndex = engine->RandomInt(0, g_numWaypoints - 1);

	// rusher bots does not care any danger (idea from pbmm)
	if (m_personality == PERSONALITY_RUSHER)
	{
		int randomGoal = goalChoices[engine->RandomInt(0, 3)];

		if (IsValidWaypoint(randomGoal >= 0))
			return m_chosenGoalIndex = randomGoal;
	}

	bool isSorting = false;

	do
	{
		isSorting = false;

		for (int i = 0; i < 3; i++)
		{
			if (goalChoices[i + 1] < 0)
				break;

			if (g_exp.GetValue(m_currentWaypointIndex, goalChoices[i], m_team) < g_exp.GetValue(m_currentWaypointIndex, goalChoices[i + 1], m_team))
			{
				goalChoices[i + 1] = goalChoices[i];
				goalChoices[i] = goalChoices[i + 1];

				isSorting = true;
			}

		}
	} while (isSorting);

	// return and store goal
	return m_chosenGoalIndex = goalChoices[0];
}

void Bot::FilterGoals(const Array <int>& goals, int* result)
{
	// this function filters the goals, so new goal is not bot's old goal, and array of goals doesn't contains duplicate goals

	int searchCount = 0;

	for (int index = 0; index < 4; index++)
	{
		int rand = goals.GetRandomElement();

		if (searchCount <= 8 && (m_prevGoalIndex == rand || ((result[0] == rand || result[1] == rand || result[2] == rand || result[3] == rand) && goals.GetElementNumber() > 4)) && !IsWaypointOccupied(rand) && IsValidWaypoint(rand))
		{
			if (index > 0)
				index--;

			searchCount++;
			continue;
		}

		result[index] = rand;
	}
}

bool Bot::GoalIsValid(void)
{
	int goal = GetCurrentTask()->data;

	if (!IsValidWaypoint(goal)) // not decided about a goal
		return false;
	else if (goal == m_currentWaypointIndex) // no nodes needed
		return true;
	else if (m_navNode == null) // no path calculated
		return false;

	// got path - check if still valid
	PathNode* node = m_navNode;

	while (node->next != null)
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

		m_waypointOrigin = g_waypoint->GetPath(m_currentWaypointIndex)->origin;

		// if node radius non zero vary origin a bit depending on the body angles
		if (g_waypoint->GetPath(m_currentWaypointIndex)->radius > 0.0f)
			m_waypointOrigin += Vector(pev->angles.x, AngleNormalize(pev->angles.y + engine->RandomFloat(-90.0f, 90.0f)), 0.0f) + (g_pGlobals->v_forward * engine->RandomFloat(0.0f, g_waypoint->GetPath(m_currentWaypointIndex)->radius));

		m_navTimeset = engine->GetTime();
	}

	extern ConVar ebot_think_fps;

	m_destOrigin = m_waypointOrigin + pev->view_ofs + g_pGlobals->v_forward + (g_pGlobals->v_forward * (1 / ebot_think_fps.GetFloat()));

	// this waypoint has additional travel flags - care about them
	if (m_currentTravelFlags & PATHFLAG_JUMP)
	{
		// bot is not jumped yet?
		if (!m_jumpFinished)
		{
			// cheating for jump, bots cannot do some hard jumps and double jumps too
			// who cares about double jump for bots? :)
			if (m_desiredVelocity == nullvec || m_desiredVelocity == -1)
			{
				pev->velocity.x = ((m_waypointOrigin.x - pev->origin.x) * 2.25f);
				pev->velocity.y = ((m_waypointOrigin.y - pev->origin.y) * 2.25f);
				pev->velocity.z = ((m_waypointOrigin.z - pev->origin.z) * 2.25f);
			}
			else
				pev->velocity = m_desiredVelocity;

			// do not use z buffer here
			if (pev->velocity.x > pev->maxspeed)
				pev->velocity.x = pev->maxspeed;

			if (pev->velocity.y > pev->maxspeed)
				pev->velocity.y = pev->maxspeed;

			if (pev->velocity.x < -pev->maxspeed)
				pev->velocity.x = -pev->maxspeed;

			if (pev->velocity.y < -pev->maxspeed)
				pev->velocity.y = -pev->maxspeed;

			pev->button |= IN_JUMP;
			
			// if bot's on the ground or on the ladder we're free to jump. actually setting the correct velocity is cheating
			// pressing the jump button gives the illusion of the bot actual jumping
			if (IsOnFloor() || IsOnLadder())
			{
				m_jumpFinished = true;
				m_checkTerrain = false;
				m_desiredVelocity = nullvec;
			}
		}
		else if (!ebot_knifemode.GetBool() && m_currentWeapon == WEAPON_KNIFE && IsOnFloor())
			SelectBestWeapon();
	}

	float waypointDistance = (pev->origin - m_waypointOrigin).GetLengthSquared() + ebot_think_fps.GetFloat();
	float fixedWaypointDistance = Q_rsqrt(waypointDistance);

	if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_LADDER)
	{
		if (m_waypointOrigin.z >= (pev->origin.z + 16.0f))
			m_waypointOrigin = g_waypoint->GetPath(m_currentWaypointIndex)->origin + Vector(0, 0, 16);
		else if (m_waypointOrigin.z < pev->origin.z + 16.0f && !IsOnLadder() && IsOnFloor())
		{
			m_moveSpeed = fixedWaypointDistance;
			if (m_moveSpeed < 150.0f)
				m_moveSpeed = 150.0f;
			else if (m_moveSpeed > pev->maxspeed)
				m_moveSpeed = pev->maxspeed;
		}
	}

	bool haveDoorEntity = false;
	edict_t* door_entity = null;
	while (!FNullEnt(door_entity = FIND_ENTITY_IN_SPHERE(door_entity, pev->origin, fixedWaypointDistance)))
	{
		if (strncmp(STRING(door_entity->v.classname), "func_door", 9) == 0)
		{
			haveDoorEntity = true;
			break;
		}
	}

	if (!haveDoorEntity)
		m_doorOpenAttempt = 0;
	else
	{
		TraceResult tr;
		TraceLine(pev->origin, m_waypointOrigin, true, GetEntity(), &tr);

		m_aimFlags &= ~(AIM_LASTENEMY | AIM_PREDICTENEMY);

		if (!FNullEnt(tr.pHit) && strncmp(STRING(tr.pHit->v.classname), "func_door", 9) == 0)
		{
			if (FNullEnt(m_pickupItem) && m_pickupType != PICKTYPE_BUTTON)
			{
				edict_t* button = FindNearestButton(STRING(tr.pHit->v.classname));
				if (!FNullEnt(button))
				{
					m_pickupItem = button;
					m_pickupType = PICKTYPE_BUTTON;

					m_navTimeset = engine->GetTime();
				}
			}

			// if bot hits the door, then it opens, so wait a bit to let it open safely
			if (pev->velocity.GetLength2D() <= 1 && m_timeDoorOpen < engine->GetTime())
			{
				PushTask(TASK_PAUSE, TASKPRI_PAUSE, -1, engine->GetTime() + 1.25f, false);

				m_doorOpenAttempt++;
				m_timeDoorOpen = engine->GetTime() + 1.25f; // retry in 1.25 sec until door is open

				edict_t* ent = nullptr;

				if (m_doorOpenAttempt > 2 && !FNullEnt(ent = FIND_ENTITY_IN_SPHERE(ent, pev->origin, 512.0f)))
				{
					if (IsValidPlayer(ent) && IsAlive(ent))
					{
						if (m_team != GetTeam(ent))
						{
							if (IsShootableThruObstacle(ent))
							{
								m_seeEnemyTime = engine->GetTime() - 0.5f;

								m_states |= STATE_SEEINGENEMY;
								m_aimFlags |= AIM_ENEMY;

								SetEnemy(ent);
								SetLastEnemy(ent);
							}
						}
						else
						{
							DeleteSearchNodes();
							ResetTasks();
						}
					}
					else if (!IsAlive(ent))
						m_doorOpenAttempt = 0;
				}
			}
		}
	}

	float desiredDistance = 8.0f;

	// initialize the radius for a special waypoint type, where the wpt is considered to be reached
	if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_LIFT)
		desiredDistance = 64.0f;
	else if ((pev->flags & FL_DUCKING) || (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_GOAL))
		desiredDistance = 32.0f;
	else if (g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_LADDER)
		desiredDistance = 16.0f;
	else
		desiredDistance = g_waypoint->GetPath(m_currentWaypointIndex)->radius;

	// check if waypoint has a special travelflag, so they need to be reached more precisely
	for (int i = 0; i < Const_MaxPathIndex; i++)
	{
		if (g_waypoint->GetPath(m_currentWaypointIndex)->connectionFlags[i] != 0)
		{
			desiredDistance = 8.0f;
			break;
		}
	}

	if (desiredDistance < 24.0f && waypointDistance < (32.0f * 32.0f) && (pev->origin + (pev->velocity * m_frameInterval) - m_waypointOrigin).GetLengthSquared2D() >= waypointDistance)
		desiredDistance = fixedWaypointDistance + 2.0f;
	else if (!(m_currentTravelFlags & PATHFLAG_JUMP) && (m_waypointOrigin - pev->origin).GetLengthSquared2D() <= (16.0f * 16.0f) && m_waypointOrigin.z <= pev->origin.z + 32.0f)
	{
		if (m_navNode == null || (m_navNode->next != null && g_waypoint->Reachable(GetEntity(), m_navNode->next->index)))
			desiredDistance = fixedWaypointDistance + 2.0f;
	}

	if (m_waypointGoalAPI != -1 && m_currentWaypointIndex == m_waypointGoalAPI)
		m_waypointGoalAPI = -1;

	if (waypointDistance <= (desiredDistance * desiredDistance) || ((pev->origin - m_waypointOrigin).GetLengthSquared2D() + (ebot_think_fps.GetFloat() * ebot_think_fps.GetFloat())) <= (desiredDistance * desiredDistance))
	{
		// Did we reach a destination Waypoint?
		if (GetCurrentTask()->data == m_currentWaypointIndex)
		{
			// add goal values
			if (GetGameMod() == MODE_BASE && m_chosenGoalIndex != -1)
				g_exp.CollectValue(m_chosenGoalIndex, m_currentWaypointIndex, static_cast <int> (pev->health), m_goalValue);

			return true;
		}
		else if (m_navNode == null)
			return false;

		if ((g_mapType & MAP_DE) && g_bombPlanted && m_team == TEAM_COUNTER && GetCurrentTask()->taskID != TASK_ESCAPEFROMBOMB && GetCurrentTask()->data != -1)
		{
			Vector bombOrigin = CheckBombAudible();

			// bot within 'hearable' bomb tick noises?
			if (bombOrigin != nullvec)
			{
				float distance = (bombOrigin - g_waypoint->GetPath(GetCurrentTask()->data)->origin).GetLength();

				if (distance > 512.0f)
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

// this function finds the shortest path from source index to destination index
void Bot::FindShortestPath(int srcIndex, int destIndex)
{
	DeleteSearchNodes();

	if (!IsValidWaypoint(srcIndex) || m_isStuck) // if we're stuck, find nearest waypoint
	{
		if (IsValidWaypoint(m_currentWaypointIndex) && !m_isStuck) // did we have a current waypoint?
			srcIndex = m_currentWaypointIndex;
		else // we can try find start waypoint for avoid pathfinding errors
		{
			int secondindex = g_waypoint->FindNearest(pev->origin, 9999.0f, -1, GetEntity());

			if (IsValidWaypoint(secondindex))
				srcIndex = secondindex;
			else
			{
				AddLogEntry(LOG_ERROR, "Pathfinder source path index not valid (%d)", srcIndex);
				return;
			}
		}
	}

	m_goalValue = 0.0f;
	m_chosenGoalIndex = srcIndex;

	PathNode* node = new PathNode;
	if (node == null)
		return;

	node->index = srcIndex;
	node->next = null;

	m_navNodeStart = node;
	m_navNode = m_navNodeStart;

	while (srcIndex != destIndex)
	{
		srcIndex = *(g_waypoint->m_pathMatrix + (srcIndex * g_numWaypoints) + destIndex);

		node->next = new PathNode;
		if (node == null)
			return;

		node = node->next;
		node->index = srcIndex;
		node->next = null;
	}
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
	m_heapSize = Const_MaxWaypoints * 4;
	m_heap = new HeapNode_t[m_heapSize];
}

PriorityQueue::~PriorityQueue(void)
{
	if (m_heap != null)
		delete[] m_heap;

	m_heap = null;
}

// inserts a value into the priority queue
void PriorityQueue::Insert(int value, float priority)
{
	if (m_size >= m_heapSize)
	{
		m_heapSize += 100;
		m_heap = (HeapNode_t*)realloc(m_heap, sizeof(HeapNode_t) * m_heapSize);

		if (m_heap == null)
			return;
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

inline const float GF_Cost(int index, int parent, int team, float offset)
{
	float baseCost = g_exp.GetAStarValue(index, team, false);

	for (int i = 0; i < Const_MaxPathIndex; i++)
	{
		int neighbour = g_waypoint->GetPath(index)->index[i];

		if (neighbour != -1)
			baseCost += g_exp.GetDamage(neighbour, neighbour, team);
	}
	float pathDist = g_waypoint->GetPathDistanceFloat(parent, index);

	if (g_waypoint->GetPath(index)->flags & WAYPOINT_LADDER)
		baseCost += pathDist * 2.0f;

	return pathDist + (baseCost * (ebot_dangerfactor.GetFloat() * 2.0f / offset));
}

inline const float GF_CostDist(int index, int parent, int team, float offset)
{
	float baseCost = g_exp.GetAStarValue(index, team, true);

	for (int i = 0; i < Const_MaxPathIndex; i++)
	{
		int neighbour = g_waypoint->GetPath(index)->index[i];

		if (neighbour != -1)
			baseCost += g_exp.GetDamage(neighbour, neighbour, team);
	}
	float pathDist = g_waypoint->GetPathDistanceFloat(parent, index);

	if (g_waypoint->GetPath(index)->flags & WAYPOINT_CROUCH)
		baseCost += pathDist * 2.0f;

	return pathDist + (baseCost * (ebot_dangerfactor.GetFloat() * 2.0f / offset));
}

inline const float GF_CostNoHostage(int index, int parent, int team, float offset)
{
	Path* path = g_waypoint->GetPath(index);

	if (path->flags & WAYPOINT_CROUCH)
		return 65355.0f;

	if (path->flags & WAYPOINT_LADDER)
		return 65355.0f;

	if (path->flags & WAYPOINT_AVOID)
		return 65355.0f;

	if (path->flags & WAYPOINT_FALLCHECK)
		return 65355.0f;

	if (path->flags & WAYPOINT_JUMP)
		return 65355.0f;

	if (path->flags & WAYPOINT_DJUMP)
		return 65355.0f;

	for (int i = 0; i < Const_MaxPathIndex; i++)
	{
		int neighbour = g_waypoint->GetPath(index)->index[i];

		if (IsValidWaypoint(neighbour) && (path->connectionFlags[neighbour] & PATHFLAG_JUMP || path->connectionFlags[neighbour] & PATHFLAG_DOUBLE))
			return 65355.0f;
	}

	float baseCost = g_exp.GetDamage(index, index, team);
	float pathDist = g_waypoint->GetPathDistanceFloat(parent, index);

	if (g_waypoint->GetPath(index)->flags & WAYPOINT_CROUCH && g_waypoint->GetPath(index)->flags & WAYPOINT_CAMP)
		baseCost += pathDist * 2.0f;

	return pathDist + (baseCost * (ebot_dangerfactor.GetFloat() * 2.0f / offset));
}

inline const float GF_CostNoHostageDist(int index, int parent, int team, float offset)
{
	Path* path = g_waypoint->GetPath(index);

	if (path->flags & WAYPOINT_CROUCH)
		return 65355.0f;

	if (path->flags & WAYPOINT_LADDER)
		return 65355.0f;

	if (path->flags & WAYPOINT_AVOID)
		return 65355.0f;

	if (path->flags & WAYPOINT_FALLCHECK)
		return 65355.0f;

	if (path->flags & WAYPOINT_JUMP)
		return 65355.0f;

	if (path->flags & WAYPOINT_DJUMP)
		return 65355.0f;

	for (int i = 0; i < Const_MaxPathIndex; i++)
	{
		int neighbour = g_waypoint->GetPath(index)->index[i];

		if (IsValidWaypoint(neighbour) && (path->connectionFlags[neighbour] & PATHFLAG_JUMP || path->connectionFlags[neighbour] & PATHFLAG_DOUBLE))
			return 65355.0f;
	}

	float baseCost = g_exp.GetAStarValue(index, team, true);
	float pathDist = g_waypoint->GetPathDistanceFloat(parent, index);

	if (g_waypoint->GetPath(index)->flags & WAYPOINT_CROUCH && g_waypoint->GetPath(index)->flags & WAYPOINT_CAMP)
		baseCost += pathDist * 2.0f;

	return pathDist + (baseCost * (ebot_dangerfactor.GetFloat() * 2.0f / offset));
}

inline const float HF_PathDist(int start, int goal)
{
	Path* pathStart = g_waypoint->GetPath(start);
	Path* pathGoal = g_waypoint->GetPath(goal);

	return fabsf(pathGoal->origin.x - pathStart->origin.x) + fabsf(pathGoal->origin.y - pathStart->origin.y) + fabsf(pathGoal->origin.z - pathStart->origin.z);
}

inline const float HF_NumberNodes(int start, int goal)
{
	return HF_PathDist(start, goal) / 128.0f * g_exp.GetKillHistory();
}

inline const float HF_None(int start, int goal)
{
	return HF_PathDist(start, goal) / (128.0f * 100.0f);
}

inline const float GF_CostZB(int index, int parent, int team, float offset)
{
	float baseCost = g_exp.GetAStarValue(index, team, false);
	float pathDist = g_waypoint->GetPathDistanceFloat(parent, index);

	if (g_waypoint->GetPath(index)->flags & WAYPOINT_CROUCH)
		baseCost += pathDist * 1.5f;

	return pathDist + (baseCost * (ebot_dangerfactor.GetFloat() * 2.3f / offset));
}

inline const float HF_ZB(int start, int goal)
{
	Path* pathStart = g_waypoint->GetPath(start);
	Path* pathGoal = g_waypoint->GetPath(goal);

	float xDist = fabsf(pathStart->origin.x - pathGoal->origin.x);
	float yDist = fabsf(pathStart->origin.y - pathGoal->origin.y);
	float zDist = fabsf(pathStart->origin.z - pathGoal->origin.z);

	if (xDist > yDist)
		return 1.4f * yDist + (xDist - yDist) + zDist;

	return 1.4f * xDist + (yDist - xDist) + zDist;
}

// this function finds a path from srcIndex to destIndex
void Bot::FindPath(int srcIndex, int destIndex, uint8_t pathType)
{
	if (m_pathtimer + 1.0 > engine->GetTime())
	{
		if (!HasNextPath()) // take care about that
			return;
	}

	if (srcIndex > g_numWaypoints - 1 || srcIndex < 0 || m_isStuck) // if we're stuck, find nearest waypoint
	{
		if (IsValidWaypoint(m_currentWaypointIndex) && !m_isStuck) // did we have a current waypoint?
			srcIndex = m_currentWaypointIndex;
		else // we can try find start waypoint for avoid pathfinding errors
		{
			int secondindex = g_waypoint->FindNearest(pev->origin, 9999.0f, -1, GetEntity());
			if (IsValidWaypoint(secondindex))
				srcIndex = secondindex;
			else
			{
				AddLogEntry(LOG_ERROR, "Pathfinder source path index not valid (%d)", srcIndex);
				return;
			}
		}
	}

	if (destIndex > g_numWaypoints - 1 || destIndex < 0)
	{
		AddLogEntry(LOG_ERROR, "Pathfinder destination path index not valid (%d)", destIndex);
		return;
	}

	m_pathtimer = engine->GetTime();

	DeleteSearchNodes();

	m_chosenGoalIndex = srcIndex;
	m_goalValue = 0.0f;

	PriorityQueue openList;

	for (int i = 0; i < g_numWaypoints; i++)
	{
		waypoints[i].g = 0;
		waypoints[i].f = 0;
		waypoints[i].parent = -1;
		waypoints[i].state = State::New;
	}

	const float (*gcalc) (int, int, int, float) = null;
	const float (*hcalc) (int, int) = null;

	float offset = 1.0f;

	if (m_isZombieBot && m_personality != PERSONALITY_CAREFUL)
	{
		gcalc = GF_CostZB;
		hcalc = HF_ZB;
		offset = static_cast <float> (m_skill / 20);
	}
	else if (pathType == 1)
	{
		gcalc = HasHostage() ? GF_CostNoHostageDist : GF_CostDist;
		hcalc = HF_NumberNodes;
		offset = static_cast <float> (m_skill / 25);
	}
	else
	{
		gcalc = HasHostage() ? GF_CostNoHostage : GF_Cost;
		hcalc = HF_PathDist;
		offset = static_cast <float> (m_skill / 20);
	}

	// put start node into open list
	auto srcWaypoint = &waypoints[srcIndex];
	srcWaypoint->g = gcalc(srcIndex, -1, m_team, offset);
	srcWaypoint->f = srcWaypoint->g + hcalc(srcIndex, destIndex);
	srcWaypoint->state = State::Open;

	openList.Insert(srcIndex, srcWaypoint->f);
	while (!openList.Empty())
	{
		// remove the first node from the open list
		int currentIndex = openList.Remove();

		// is the current node the goal node?
		if (currentIndex == destIndex)
		{
			// build the complete path
			m_navNode = null;

			do
			{
				PathNode* path = new PathNode;

				if (path == null)
					return;

				path->index = currentIndex;
				path->next = m_navNode;

				m_navNode = path;
				currentIndex = waypoints[currentIndex].parent;

			} while (currentIndex != -1);

			m_navNodeStart = m_navNode;
			return;
		}

		auto currWaypoint = &waypoints[currentIndex];
		if (currWaypoint->state != State::Open)
			continue;

		// put current node into Closed list
		currWaypoint->state = State::Closed;

		// now expand the current node
		for (int i = 0; i < Const_MaxPathIndex; i++)
		{
			int self = g_waypoint->GetPath(currentIndex)->index[i];
			if (self == -1)
				continue;

			// calculate the F value as F = G + H
			float g = currWaypoint->g + gcalc(self, currentIndex, m_team, offset);
			float h = hcalc(srcIndex, destIndex);
			float f = g + h;

			auto childWaypoint = &waypoints[self];
			if (childWaypoint->state == State::New || childWaypoint->f > f)
			{
				// put the current child into open list
				childWaypoint->parent = currentIndex;
				childWaypoint->state = State::Open;

				childWaypoint->g = g;
				childWaypoint->f = f;

				openList.Insert(self, f);
			}
		}
	}

	// sadly this causing the CRASH
	//	FindShortestPath(srcIndex, destIndex); // A* found no path, try floyd pathfinder instead 
}

void Bot::DeleteSearchNodes(void)
{
	if (m_pathtimer + 1.0f > engine->GetTime())
		return;
	
	PathNode* deletingNode = null;
	PathNode* node = m_navNodeStart;
	
	while (node != null)
	{
		deletingNode = node->next;
		delete node;

		node = deletingNode;
	}
	
	m_navNodeStart = null;
	m_navNode = null;
	m_chosenGoalIndex = -1;
}

void Bot::CheckTouchEntity(edict_t* entity)
{
	if (m_checktouch + 1.0f > engine->GetTime())
		return;

	if (m_currentWeapon == WEAPON_KNIFE &&
		(!FNullEnt(m_enemy) || !FNullEnt(m_breakableEntity)) &&
		(m_enemy == entity || m_breakableEntity == entity))
		KnifeAttack((pev->origin - GetEntityOrigin(entity)).GetLength() + 50.0f);
	
	if (IsShootableBreakable(entity))
	{
		bool attackBreakable = false;
		if (m_isStuck || &m_navNode[0] == null)
			attackBreakable = true;
		else if (IsValidWaypoint(m_currentWaypointIndex))
		{
			TraceResult tr;
			TraceLine(pev->origin, g_waypoint->GetPath(m_currentWaypointIndex)->origin, false, false, GetEntity(), &tr);
			if (tr.pHit == entity)// && tr.flFraction < 1.0f)
				attackBreakable = true;
		}

		if (attackBreakable)
		{
			m_breakableEntity = entity;
			m_breakable = GetEntityOrigin(entity);
			m_destOrigin = m_breakable;

			if (pev->origin.z > m_breakable.z)
				m_campButtons = IN_DUCK;
			else
				m_campButtons = pev->button & IN_DUCK;

			PushTask(TASK_DESTROYBREAKABLE, TASKPRI_SHOOTBREAKABLE, -1, 1.0f, false);
		}
	}

	m_checktouch = engine->GetTime();
}

void Bot::SetEnemy(edict_t* entity)
{
	if (FNullEnt(entity) || !IsAlive(entity))
	{
		if (!FNullEnt(m_enemy) && &m_navNode[0] == null)
		{
			SetEntityWaypoint(GetEntity(), -2);
			m_currentWaypointIndex = -1;
			GetValidWaypoint();
		}

		m_enemy = null;
		m_enemyOrigin = nullvec;
		return;
	}

	if (m_enemy != entity)
		m_enemyReachableTimer = 0.0f;

	m_enemy = entity;
}

void Bot::SetLastEnemy(edict_t* entity)
{
	if (FNullEnt(entity) || !IsValidPlayer(entity) || !IsAlive(entity))
	{
		m_lastEnemy = null;
		m_lastEnemyOrigin = nullvec;
		return;
	}

	m_lastEnemy = entity;
	m_lastEnemyOrigin = GetEntityOrigin(entity);
}

void Bot::SetMoveTarget(edict_t* entity)
{
	m_moveTargetOrigin = GetEntityOrigin(entity);
	if (FNullEnt(entity) || m_moveTargetOrigin == nullvec)
	{
		m_moveTargetEntity = null;
		m_moveTargetOrigin = nullvec;

		if (GetCurrentTask()->taskID == TASK_MOVETOTARGET)
		{
			RemoveCertainTask(TASK_MOVETOTARGET);
			m_prevGoalIndex = -1;
			GetCurrentTask()->data = -1;
		}

		return;
	}

	m_states &= ~STATE_SEEINGENEMY;
	SetEnemy(null);
	SetLastEnemy(null);
	m_enemyUpdateTime = 0.0f;
	m_aimFlags &= ~AIM_ENEMY;

	if (m_moveTargetEntity == entity)
		return;

	SetEntityWaypoint(entity);
	SetEntityWaypoint(GetEntity(), GetEntityWaypoint(entity));

	m_currentWaypointIndex = -1;
	GetValidWaypoint();

	m_moveTargetEntity = entity;

	PushTask(TASK_MOVETOTARGET, TASKPRI_MOVETOTARGET, -1, 0.0, true);
}

int Bot::GetAimingWaypoint(Vector targetOriginPos)
{
	// return the most distant waypoint which is seen from the Bot to the Target and is within count

	if (!IsValidWaypoint(m_currentWaypointIndex))
		GetValidWaypoint();

	int srcIndex = m_currentWaypointIndex;
	int destIndex = g_waypoint->FindNearest(targetOriginPos);
	int bestIndex = srcIndex;

	PathNode* node = new PathNode;

	if (node == null)
		return -1;

	node->index = destIndex;
	node->next = null;

	PathNode* startNode = node;

	while (destIndex != srcIndex)
	{
		destIndex = *(g_waypoint->m_pathMatrix + (destIndex * g_numWaypoints) + srcIndex);

		if (destIndex < 0)
			break;

		node->next = new PathNode();
		node = node->next;

		if (node == null)
			return -1;

		node->index = destIndex;
		node->next = null;

		if (g_waypoint->IsVisible(m_currentWaypointIndex, destIndex))
		{
			bestIndex = destIndex;
			break;
		}
	}

	while (startNode != null)
	{
		node = startNode->next;
		delete startNode;

		startNode = node;
	}

	return bestIndex;
}

int Bot::FindWaypoint(void)
{
	// this function find a node in the near of the bot if bot had lost his path of pathfinder needs
	// to be restarted over again.

	int busy = -1;

	float lessDist[3] = {9999.0f, 9999.0f, 9999.0f};
	int lessIndex[3] = {-1, -1, -1};

	for (int at = 0; at < g_numWaypoints; at++)
	{
		if (!IsValidWaypoint(at))
			continue;

		int numToSkip = engine->RandomInt(0, 2);

		bool skip = !!(int(g_waypoint->GetPath(at)->index) == m_currentWaypointIndex);

		// skip the current node, if any
		if (skip && numToSkip > 0)
			continue;

		// skip if isn't visible
		if (!IsVisible(g_waypoint->GetPath(at)->origin, GetEntity()))
			continue;

		// skip current and recent previous nodes
		for (int j = 0; j < numToSkip; ++j)
		{
			if (at == m_prevWptIndex[j])
			{
				skip = true;
				break;
			}
		}

		// skip node from recent list
		if (skip)
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
		float distance = (pev->origin - g_waypoint->GetPath(at)->origin).GetLength();

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
		selected = g_waypoint->FindNearest(pev->origin, 9999.0f, -1, GetEntity());

	ChangeWptIndex(selected);

	return true;
}

void Bot::IgnoreCollisionShortly(void)
{
	ResetCollideState();

	m_lastCollTime = engine->GetTime() + 1.0f;
	m_isStuck = false;
	m_checkTerrain = false;
}

void Bot::SetWaypointOrigin(void)
{
	extern ConVar ebot_think_fps;

	m_waypointOrigin = g_waypoint->GetPath(m_currentWaypointIndex)->origin + g_pGlobals->v_forward * (1 / ebot_think_fps.GetFloat());

	float radius = g_waypoint->GetPath(m_currentWaypointIndex)->radius;
	if (radius > 0)
	{
		MakeVectors(Vector(pev->angles.x, AngleNormalize(pev->angles.y + engine->RandomFloat(-90.0f, 90.0f)), 0.0f));
		int sPoint = -1;

		if (&m_navNode[0] != null && m_navNode->next != null)
		{
			Vector waypointOrigin[5];
			for (int i = 0; i < 5; i++)
			{
				waypointOrigin[i] = m_waypointOrigin;
				waypointOrigin[i] += Vector(engine->RandomFloat(-radius, radius), engine->RandomFloat(-radius, radius), 0.0f);
			}

			int destIndex = m_navNode->next->index;

			float sDistance = 9999.0f;
			for (int i = 0; i < 5; i++)
			{
				// SyPB Pro P.42 - Small Waypoint OS improve
				float distance = ((pev->origin - waypointOrigin[i]).GetLength2D() + (waypointOrigin[i] - g_waypoint->GetPath(destIndex)->origin).GetLength2D() * (1 / ebot_think_fps.GetFloat()));

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
			m_waypointOrigin = m_waypointOrigin + g_pGlobals->v_forward * engine->RandomFloat((radius / 2), radius) * (1 / ebot_think_fps.GetFloat());
	}

	if (IsOnLadder())
	{
		TraceResult tr;
		TraceLine(Vector(pev->origin.x, pev->origin.y, pev->absmin.z), m_waypointOrigin, true, true, GetEntity(), &tr);

		if (tr.flFraction < 1.0f)
			m_waypointOrigin = m_waypointOrigin + (pev->origin - m_waypointOrigin) * 0.5f + Vector(0.0f, 0.0f, 32.0f);
	}
}

void Bot::GetValidWaypoint(void)
{
	// checks if the last waypoint the bot was heading for is still valid

	bool needFindWaypont = false;
	if (!IsValidWaypoint(m_currentWaypointIndex))
		needFindWaypont = true;
	else if ((m_navTimeset + GetEstimatedReachTime() + 5.0f) < engine->GetTime())
		needFindWaypont = true;
	else
	{
		const int client = GetIndex() - 1;
		if (m_isStuck)
			g_clients[client].wpIndex = g_clients[client].wpIndex2 = -1;

		if (!IsValidWaypoint(g_clients[client].wpIndex) && !IsValidWaypoint(g_clients[client].wpIndex2))
			needFindWaypont = true;
	}

	if (needFindWaypont)
	{
		if (IsValidWaypoint(m_currentWaypointIndex) && !IsZombieMode())
			g_exp.CollectValidDamage(m_currentWaypointIndex, m_team);

		FindWaypoint();
		SetWaypointOrigin();
	}
}

void Bot::ChangeWptIndex(int waypointIndex)
{
	bool badPrevWpt = true;
	for (int i = 0; (IsValidWaypoint(m_currentWaypointIndex) && i < Const_MaxPathIndex); i++)
	{
		if (g_waypoint->GetPath(m_currentWaypointIndex)->index[i] == waypointIndex)
			badPrevWpt = false;
	}

	if (badPrevWpt == true)
	{
		m_prevWptIndex[1] = -1;
		m_prevWptIndex[0] = -1;
	}
	else
	{
		m_prevWptIndex[1] = m_prevWptIndex[0];
		m_prevWptIndex[0] = m_currentWaypointIndex;
	}

	m_currentWaypointIndex = waypointIndex;
	m_navTimeset = engine->GetTime();

	// get the current waypoint flags
	if (IsValidWaypoint(m_currentWaypointIndex))
		m_waypointFlags = g_waypoint->GetPath(m_currentWaypointIndex)->flags;
	else
		m_waypointFlags = 0;
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
		float distance = (g_waypoint->GetPath(g_waypoint->m_goalPoints[i])->origin - bombOrigin).GetLength2D();

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
	// no camp waypoints
	if (g_waypoint->m_campPoints.IsEmpty())
		return  engine->RandomInt(0, g_numWaypoints - 1);

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

		TraceResult tr{};

		TraceLine(g_waypoint->GetPath(index)->origin, origin, true, true, GetEntity(), &tr);

		if (tr.flFraction == 1.0f && !IsWaypointOccupied(index)) // distance isn't matter
			BestSpots.Push(index);
		else if ((g_waypoint->GetPath(index)->origin - origin).GetLengthSquared() <= (1024.0f * 1024.0f) && !IsWaypointOccupied(index))
			OkSpots.Push(index);
		else if (!IsWaypointOccupied(index))
			WorstSpots.Push(index);
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

	return g_waypoint->m_campPoints.GetRandomElement();
}

int Bot::FindCoverWaypoint(float maxDistance)
{
	// really?
	if (maxDistance < 512.0f)
		maxDistance = 512.0f;

	// do not move to a position near to the enemy
	float enemydist = (m_lastEnemyOrigin - pev->origin).GetLength2D();
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

		TraceResult tr{};

		Vector origin = !FNullEnt(m_enemy) ? m_enemy->v.origin : m_lastEnemy->v.origin;

		TraceLine(g_waypoint->GetPath(i)->origin, origin, true, true, GetEntity(), &tr);

		if (tr.flFraction != 1.0f && !IsWaypointOccupied(i) && (g_waypoint->GetPath(i)->origin - origin).GetLength2D() <= maxDistance)
			BestSpots.Push(i);
		else if (tr.flFraction != 1.0f && !IsWaypointOccupied(i)) // distance isn't matter now
			OkSpots.Push(i);
	}

	if (!BestSpots.IsEmpty() && !IsValidWaypoint(ChoosenIndex))
	{
		for (int i = 0; i < BestSpots.GetElementNumber(); i++)
		{
			if (!IsValidWaypoint(i))
				continue;

			float maxdist = maxDistance;
			float experience = float(g_exp.GetDamage(i, i, m_team) * 100 / MAX_EXPERIENCE_VALUE) + (g_exp.GetDangerIndex(i, i, m_team) * 100 / MAX_EXPERIENCE_VALUE) + g_exp.GetKillHistory();
			float distance = ((pev->origin - g_waypoint->GetPath(i)->origin).GetLength2D() + experience);

			if (distance < maxdist)
			{
				ChoosenIndex = i;
				maxdist = distance;
			}
		}
	}
	else if (!OkSpots.IsEmpty() && !IsValidWaypoint(ChoosenIndex))
	{
		for (int i = 0; i < OkSpots.GetElementNumber(); i++)
		{
			if (!IsValidWaypoint(i))
				continue;

			float maxdist = 999999.0f;
			float experience = float(g_exp.GetDamage(i, i, m_team) * 100 / MAX_EXPERIENCE_VALUE) + (g_exp.GetDangerIndex(i, i, m_team) * 100 / MAX_EXPERIENCE_VALUE) + g_exp.GetKillHistory();
			float distance = ((pev->origin - g_waypoint->GetPath(i)->origin).GetLength2D() + experience);

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

bool Bot::GetBestNextWaypoint(void)
{
	// this function does a realtime postprocessing of waypoints return from the
	// pathfinder, to vary paths and find the best waypoint on our way

	InternalAssert(m_navNode != null);
	InternalAssert(m_navNode->next != null);

	if (!IsWaypointOccupied(m_navNode->index))
		return false;

	for (int i = 0; i < Const_MaxPathIndex; i++)
	{
		int id = g_waypoint->GetPath(m_currentWaypointIndex)->index[i];

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
	return (m_navNode != null && m_navNode->next != null);
}

bool Bot::HeadTowardWaypoint(void)
{
	// advances in our pathfinding list and sets the appropiate destination origins for this bot

	GetValidWaypoint(); // check if old waypoints is still reliable

	// no waypoints from pathfinding?
	if (m_navNode == null)
		return false;

	TraceResult tr;

	m_navNode = m_navNode->next; // advance in list
	m_currentTravelFlags = 0; // reset travel flags (jumping etc)

	// we're not at the end of the list?
	if (m_navNode != null)
	{
		if (m_navNode->next != null && !(g_waypoint->GetPath(m_navNode->next->index)->flags & WAYPOINT_LADDER))
		{
			if (m_navNodeStart != m_navNode)
			{
				GetBestNextWaypoint();
				int taskID = GetCurrentTask()->taskID;

				m_minSpeed = pev->maxspeed;

				// only if we in normal task and bomb is not planted
				if (GetGameMod() == MODE_BASE && taskID == TASK_NORMAL && !g_bombPlanted && m_personality != PERSONALITY_RUSHER && !m_isBomber && !m_isVIP && (!IsValidWaypoint(m_loosedBombWptIndex) && m_team == TEAM_TERRORIST))
				{
					m_campButtons = 0;

					int waypoint = m_navNode->next->index;
					int kills = g_exp.GetDamage(waypoint, waypoint, m_team);

					// if damage done higher than one
					if (kills > 1 && g_timeRoundMid > engine->GetTime() && g_killHistory > 0)
					{
						kills = (kills * 100) / g_killHistory;
						kills /= 100;

						switch (m_personality)
						{
						case PERSONALITY_NORMAL:
							kills /= 3;
							break;

						default:
							kills /= 2;
							break;
						}

						if (m_baseAgressionLevel < static_cast <float> (kills))
						{
							PushTask(TASK_GOINGFORCAMP, TASKPRI_MOVETOPOSITION, FindDefendWaypoint(g_waypoint->GetPath(waypoint)->origin), engine->GetTime() + (m_fearLevel * (g_timeRoundMid - engine->GetTime()) * 0.5f), true);

							m_campButtons |= IN_DUCK;
						}
					}
					else if (g_botsCanPause && !IsOnLadder() && !IsInWater() && !m_currentTravelFlags && IsOnFloor())
					{
						if (static_cast <float> (kills) == m_baseAgressionLevel)
							m_campButtons |= IN_DUCK;
						else if (engine->RandomInt(1, 100) > (m_skill + engine->RandomInt(1, 20)))
							m_minSpeed = GetWalkSpeed();
					}
				}
			}
		}

		if (m_navNode != null)
		{
			int destIndex = m_navNode->index;

			// find out about connection flags
			if (IsValidWaypoint(m_currentWaypointIndex))
			{
				Path* path = g_waypoint->GetPath(m_currentWaypointIndex);

				for (int i = 0; i < Const_MaxPathIndex; i++)
				{
					if (path->index[i] == m_navNode->index)
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
				if (HasNextPath())
				{
					for (int i = 0; i < Const_MaxPathIndex; i++)
					{
						if (g_waypoint->GetPath(m_navNode->index)->index[i] == m_navNode->next->index && (g_waypoint->GetPath(m_navNode->index)->connectionFlags[i] & PATHFLAG_JUMP))
						{
							src = g_waypoint->GetPath(m_navNode->index)->origin;
							destination = g_waypoint->GetPath(m_navNode->next->index)->origin;

							jumpDistance = (g_waypoint->GetPath(m_navNode->index)->origin - g_waypoint->GetPath(m_navNode->next->index)->origin).GetLength();
							willJump = true;

							break;
						}
					}
				}

				if (willJump && !(m_states & STATE_SEEINGENEMY) && FNullEnt(m_lastEnemy) && m_currentWeapon != WEAPON_KNIFE && !m_isReloading && (jumpDistance > 210 || (destination.z + 32.0f > src.z && jumpDistance > 150) || ((destination - src).GetLength2D() < 50 && jumpDistance > 60) || pev->maxspeed <= 210))
				{
					if (GetGameMod() != MODE_DM && !IsAntiBlock(GetEntity()) && !IsOnLadder() && g_waypoint->GetPath(destIndex)->flags & WAYPOINT_LADDER)
					{
						float waitTime = -1.0f;
					    for (const auto& client : g_clients)
						{
							Bot* otherBot = g_botManager->GetBot(client.ent);

							if (!IsValidBot(otherBot->GetEntity()) || otherBot == this || !IsAlive(otherBot->GetEntity()))
								continue;

							if (!otherBot->IsOnLadder())
							{
								if (otherBot->m_currentWaypointIndex == destIndex)
								{
									Vector wpOrigin = g_waypoint->GetPath(destIndex)->origin;

									if ((otherBot->pev->origin - wpOrigin).GetLength() <= (pev->origin - wpOrigin).GetLength())
									{
										waitTime = 2.0f;
										break;
									}
								}

								continue;
							}

							if (&otherBot->m_navNode[0] == null)
							{
								int otherBotLadderWpIndex = otherBot->m_currentWaypointIndex;
								if (destIndex == otherBotLadderWpIndex || (m_navNode->next != null && m_navNode->next->index == otherBotLadderWpIndex))
								{
									waitTime = 0.5f;
									break;
								}

								continue;
							}

							if (otherBot->m_currentWaypointIndex == destIndex && otherBot->m_navNode != null && otherBot->m_navNode->index == m_currentWaypointIndex)
							{
								waitTime = 0.5f;
								break;
							}
						}

						if (waitTime != -1.0f)
						{
							PushTask(TASK_PAUSE, TASKPRI_PAUSE, -1, engine->GetTime() + waitTime, false);
							return false;
						}
					}
				}
			}

			ChangeWptIndex(destIndex);
		}
	}

	SetWaypointOrigin();

	if (IsOnLadder())
	{
		TraceLine(Vector(pev->origin.x, pev->origin.y, pev->absmin.z), m_waypointOrigin, false, false, GetEntity(), &tr);
		if (tr.flFraction < 1.0f)
		{
			if (m_waypointOrigin.z >= pev->origin.z)
				m_waypointOrigin += tr.vecPlaneNormal;
			else
				m_waypointOrigin -= tr.vecPlaneNormal;
		}
	}

	m_navTimeset = engine->GetTime();

	return true;
}

bool Bot::CantMoveForward(Vector normal, TraceResult* tr)
{
	// Checks if bot is blocked in his movement direction (excluding doors)

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

	// do a trace to the left...
	TraceLine(pev->origin, pev->origin - g_pGlobals->v_forward * 50.0f, true, false, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction != 1.0f)
		return true;
	else
	{
		TraceResult tr2;
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 50.0f, true, GetEntity(), &tr2);

		// we don't want fall
		if (tr2.flFraction == 1.0f)
			return true;
	}

	return false;
}

bool Bot::CheckWallOnLeft(void)
{
	TraceResult tr;
	MakeVectors(pev->angles);

	// do a trace to the left...
	TraceLine(pev->origin, pev->origin - g_pGlobals->v_right * 50.0f, true, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction != 1.0f)
		return true;
	else
	{
		TraceResult tr2;
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 50.0f, true, GetEntity(), &tr2);

		// we don't want fall
		if (tr2.flFraction == 1.0f)
			return true;
	}

	return false;
}

bool Bot::CheckWallOnRight(void)
{
	TraceResult tr;
	MakeVectors(pev->angles);

	// do a trace to the right...
	TraceLine(pev->origin, pev->origin + g_pGlobals->v_right * 50.0f, true, GetEntity(), &tr);

	// check if the trace hit something...
	if (tr.flFraction != 1.0f)
		return true;
	else
	{
		TraceResult tr2;
		TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 50.0f, true, GetEntity(), &tr2);

		// we don't want fall
		if (tr2.flFraction == 1.0f)
			return true;
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

	float distance = (targetOriginPos - check).GetLength();  // distance from goal

	while (distance > 16.0f)
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
		distance = (targetOriginPos - check).GetLength();  // distance from goal
	}
	return false;
}

bool Bot::CheckCloseAvoidance(const Vector& dirNormal)
{
	if (IsAntiBlock(GetEntity()))
		return false;

	if (m_seeEnemyTime + 3.0f < engine->GetTime())
		return false;

	edict_t* hindrance = nullptr;
	float distance = 300.0f * 300.0f;

	// find nearest player to bot
	for (const auto& client : g_clients)
	{
		// need only good meat
		if (!(client.flags & CFLAG_USED))
			continue;

		// and still alive meet
		if (!(client.flags & CFLAG_ALIVE))
			continue;

		// our team, alive and not myself?
		if (client.team != m_team || client.ent == GetEntity())
			continue;

		float nearest = (pev->origin - client.ent->v.origin).GetLengthSquared();
		if (nearest < (pev->maxspeed * pev->maxspeed) && nearest < distance)
		{
			hindrance = client.ent;
			distance = nearest;
		}
	}

	// found somebody?
	if (!hindrance)
		return false;

	// don't get stuck in small areas, follow the same goal
	if (m_isStuck)
	{
		Bot* otherBot = g_botManager->GetBot(hindrance);
		if (otherBot != null)
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
				FindPath(m_currentWaypointIndex, index, 0);
				otherBot->FindPath(m_currentWaypointIndex, index, 0);
			}
		}
	}

	const float interval = m_frameInterval * 4.0f;

	// use our movement angles, try to predict where we should be next frame
	Vector right, forward;
	m_moveAngles.BuildVectors(&forward, &right, nullptr);

	Vector predict = pev->origin + forward * m_moveSpeed * interval;

	predict += right * m_strafeSpeed * interval;
	predict += pev->velocity * interval;

	float movedDistance = (predict - hindrance->v.origin).GetLengthSquared();
	float nextFrameDistance = (pev->origin - (hindrance->v.origin + hindrance->v.velocity * interval)).GetLengthSquared();

	// is player that near now or in future that we need to steer away?
	if (movedDistance <= (48.0f * 48.0f) || (distance <= (56.0f * 56.0f) && nextFrameDistance < distance))
	{
		auto dir = (pev->origin - hindrance->v.origin).Normalize2D();

		// to start strafing, we have to first figure out if the target is on the left side or right side
		if ((dir | right.GetLength2D()) > 0.0f)
			SetStrafeSpeed(dirNormal, pev->maxspeed);
		else
			SetStrafeSpeed(dirNormal, -pev->maxspeed);

		if (distance < (56.0f * 56.0f))
		{
			if ((dir | forward.GetLength2D()) < 0.0f)
				m_moveSpeed = -pev->maxspeed;
		}

		return true;
	}

	return false;
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

		int index = -1;
		float maxdamage = 99999.0f;
		float damage = g_exp.GetDamage(i, i, m_team);
		

		if (damage > maxdamage)
		{
			index = i;
			maxdamage = damage;
		}

		DangerWaypoint = index;

		if (!IsVisible(g_waypoint->GetPath(i)->origin, GetEntity()))
			continue;

		if ((g_waypoint->GetPath(i)->origin - pev->origin).GetLength() > 512.0f)
			BestWaypoints.Push(i);
		else
			OkWaypoints.Push(i);
	}

	int dangerIndex = g_exp.GetDangerIndex(currentWay, currentWay, TEAM_COUNTER);
	if (ChanceOf(20) && IsValidWaypoint(dangerIndex) && IsVisible(g_waypoint->GetPath(dangerIndex)->origin, GetEntity()))
		return dangerIndex;
	else if (ChanceOf(20) && IsValidWaypoint(DangerWaypoint) && IsVisible(g_waypoint->GetPath(DangerWaypoint)->origin, GetEntity()))
		return DangerWaypoint;
	else if (!BestWaypoints.IsEmpty())
		return BestWaypoints.GetRandomElement();
	else if (!OkWaypoints.IsEmpty())
		return OkWaypoints.GetRandomElement();

	return g_waypoint->m_otherPoints.GetRandomElement();
}

// more human like aiming
void Bot::FacePosition(void)
{
	if (!FNullEnt(m_enemy) || !FNullEnt(m_enemyAPI))
	{
		if (m_aimstoptime + 0.3f > engine->GetTime())
			m_aimstoptime = engine->GetTime();

		m_aimstopdelay = IsZombieMode() ? 0.05f : engine->RandomFloat(0.1f, 0.3f);
	}
	else
		m_aimstopdelay = engine->RandomFloat(0.5f, 1.25f);

	// humanize aim, also saving cpu power
	if (m_aimstoptime > engine->GetTime())
		return;

	// SyPB Pro P.30 - AMXX
	if (m_lookAtAPI != nullvec)
		m_lookAt = m_lookAtAPI;

	if ((GetCurrentTask()->taskID == TASK_DEFUSEBOMB && m_hasProgressBar) || (GetCurrentTask()->taskID == TASK_PLANTBOMB && m_hasProgressBar))
		return;

	if (m_aimFlags & AIM_ENEMY && m_aimFlags & AIM_ENTITY && m_aimFlags & AIM_GRENADE && m_aimFlags & AIM_LASTENEMY || GetCurrentTask()->taskID == TASK_DESTROYBREAKABLE)
	{
		m_playerTargetTime = engine->GetTime();

		// force press attack button for human bots in zombie mode
		if (!m_isReloading && IsZombieMode() && !(pev->button & IN_ATTACK) && GetCurrentTask()->taskID != TASK_THROWFBGRENADE && GetCurrentTask()->taskID != TASK_THROWHEGRENADE && GetCurrentTask()->taskID != TASK_THROWSMGRENADE && GetCurrentTask()->taskID != TASK_THROWFLARE)
			pev->button |= IN_ATTACK;
	}

	if (pev->button & IN_ATTACK && ebot_aimbot.GetInt() == 1)
	{
		pev->v_angle = (m_lookAt - EyePosition()).ToAngles() + pev->punchangle;
		pev->angles.x = -pev->v_angle.x * (1.0f / 3.0f);
		pev->angles.y = pev->v_angle.y;

		return;
	}

	const float delta = engine->Clamp(engine->GetTime() - m_trackingtime, MATH_EQEPSILON, 0.03333333333f);
	m_trackingtime = engine->GetTime();

	// adjust all body and view angles to face an absolute vector
	Vector direction = (m_lookAt - EyePosition()).ToAngles() + pev->punchangle;
	direction.x *= -1.0f; // invert for engine

	direction.z = 0;

	float accelerate = float(m_skill * 36);
	float stiffness = float(m_skill * 3.3);
	float damping = float(m_skill / 3);

	m_idealAngles = pev->v_angle;

	float angleDiffPitch = engine->AngleDiff(direction.x, m_idealAngles.x);
	float angleDiffYaw = engine->AngleDiff(direction.y, m_idealAngles.y);

	if (angleDiffYaw < 1.0f && angleDiffYaw > -1.0f)
	{
		m_lookYawVel = 0.0f;

		// help bot for 100% weapon accurate.
		if ((m_isSlowThink && ChanceOf(int((m_skill / 5) + 1))) || IsZombieMode() || m_numFriendsLeft == 0 || m_isLeader || m_isVIP || GetCurrentTask()->taskID == TASK_CAMP)
			m_idealAngles.y = direction.y;

		m_aimstoptime = engine->GetTime() + m_aimstopdelay;
	}
	else
	{
		float accel = engine->Clamp(stiffness * angleDiffYaw - damping * m_lookYawVel, -accelerate, accelerate);

		m_lookYawVel += delta * accel;
		m_idealAngles.y += delta * m_lookYawVel;
	}

	float accel = engine->Clamp(2.0f * stiffness * angleDiffPitch - damping * m_lookPitchVel, -accelerate, accelerate);

	m_lookPitchVel += delta * accel;
	m_idealAngles.x += engine->Clamp(delta * m_lookPitchVel, -89.0f, 89.0f);

	if (m_idealAngles.x < -89.0f)
		m_idealAngles.x = -89.0f;
	else if (m_idealAngles.x > 89.0f)
		m_idealAngles.x = 89.0f;

	pev->v_angle = m_idealAngles;
	pev->v_angle.z = 0;

	// set the body angles to point the gun correctly
	pev->angles.x = -pev->v_angle.x * (1.0f / 3.0f);
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

		if (m_jumpTime + 5.0f < engine->GetTime() && IsOnFloor())
			pev->button |= IN_JUMP;
	}
	else if (dot > 0 && !CheckWallOnRight())
		m_strafeSpeed = strafeSpeed;
	else if (!CheckWallOnLeft())
		m_strafeSpeed = -strafeSpeed;
	else if (GetCurrentTask()->taskID == TASK_CAMP)
		m_lastEnemy = null;
}

// find hostage improve
int Bot::FindHostage(void)
{
	if (m_team != TEAM_COUNTER || !(g_mapType & MAP_CS) || m_isZombieBot)
		return -1;

	edict_t* ent = null;

	while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "hostage_entity")))
	{
		bool canF = true;

		for (const auto& client : g_clients)
		{
			Bot* bot = g_botManager->GetBot(client.ent);

			if (bot != null && bot->m_notKilled)
			{
				for (int j = 0; j < Const_MaxHostages; j++)
				{
					if (bot->m_hostages[j] == ent)
						canF = false;
				}
			}
		}

		int nearestIndex = g_waypoint->FindNearest(GetEntityOrigin(ent), 9999.0f, -1, ent);

		if (IsValidWaypoint(nearestIndex) && canF)
			return nearestIndex;
		else
		{
			// do we need second try?
			int nearestIndex2 = g_waypoint->FindNearest(GetEntityOrigin(ent));

			if (IsValidWaypoint(nearestIndex2) && canF)
				return nearestIndex2;
		}
	}

	return -1;
}

int Bot::FindLoosedBomb(void)
{
	if (m_team != TEAM_TERRORIST || !(g_mapType & MAP_DE) || m_isZombieBot)
		return -1; // don't search for bomb if the player is CT, or it's not defusing bomb

	edict_t* bombEntity = null; // temporaly pointer to bomb

	// search the bomb on the map
	while (!FNullEnt(bombEntity = FIND_ENTITY_BY_CLASSNAME(bombEntity, "weaponbox")))
	{
		if (strcmp(STRING(bombEntity->v.model) + 9, "backpack.mdl") == 0)
		{
			// ebot 1.55 - find bomb improve

			int nearestIndex = g_waypoint->FindNearest(GetEntityOrigin(bombEntity), 9999.0f, -1, bombEntity);

			if (IsValidWaypoint(nearestIndex))
				return nearestIndex;
			else
			{
				// do we need second try?
				int nearestIndex2 = g_waypoint->FindNearest(GetEntityOrigin(bombEntity));

				if (IsValidWaypoint(nearestIndex2))
					return nearestIndex2;
			}

			break;
		}
	}

	return -1;
}

bool Bot::IsWaypointUsed(int index)
{
	if (!IsValidWaypoint(index))
		return true;

	for (const auto& client : g_clients)
	{
		if (!IsAlive(client.ent))
			continue;

		if (IsAntiBlock(client.ent))
			continue;

		if (client.ent == GetEntity())
			continue;

		if ((g_waypoint->GetPath(index)->origin - GetEntityOrigin(client.ent)).GetLength() <= 64.0f)
			return true;

		Bot* bot = g_botManager->GetBot(client.ent);
		if (bot != null && bot->m_currentWaypointIndex == index)
			return true;
	}

	return false;
}

bool Bot::IsWaypointOccupied(int index)
{
	if (!IsValidWaypoint(index))
		return true;

	if (IsAntiBlock(GetEntity()))
		return false;

	// first check if current waypoint of one of the bots is index waypoint
	for (const auto& client : g_clients)
	{
		if (client.ent == NULL || client.ent == GetEntity() || !IsAlive(client.ent) || IsAntiBlock(client.ent))
			continue;

		if (m_notKilled && IsValidWaypoint(m_currentWaypointIndex) && IsValidWaypoint(m_prevWptIndex[0]))
		{
			int occupyId = GetShootingConeDeviation(GetEntity(), &pev->origin) >= 0.7f ? m_prevWptIndex[0] : m_currentWaypointIndex;

			// length check
			float length = (g_waypoint->GetPath(occupyId)->origin - g_waypoint->GetPath(index)->origin).GetLength();
			if (occupyId == index || GetCurrentTask()->data == index || length < 64.0f)
				return true;
		}
	}

	return false;
}

edict_t* Bot::FindNearestButton(const char* className)
{
	// this function tries to find nearest to current bot button, and returns pointer to
	// it's entity, also here must be specified the target, that button must open.

	if (IsNullString(className))
		return null;

	float nearestDistance = FLT_MAX;
	edict_t* searchEntity = null, * foundEntity = null;

	// find the nearest button which can open our target
	while (!FNullEnt(searchEntity = FIND_ENTITY_BY_TARGET(searchEntity, className)))
	{
		Vector entityOrign = GetEntityOrigin(searchEntity);

		float distance = (pev->origin - entityOrign).GetLength();

		if (distance <= nearestDistance)
		{
			nearestDistance = distance;
			foundEntity = searchEntity;
		}
	}

	return foundEntity;
}

// SyPB Pro P.38 - AMXX API
int Bot::CheckBotPointAPI(int mod)
{
	if (mod == 0)
		return GetEntityWaypoint(GetEntity());
	else if (mod == 1)
		return m_currentWaypointIndex;
	else if (mod == 2)
	{
		if (&m_navNode[0] != null)
			return m_navNode->index;
	}

	return -1;
}

// SyPB Pro P.40 - AMXX API
int Bot::GetNavData(int data)
{
	PathNode* navid = &m_navNode[0];
	int pointNum = 0;

	while (navid != null)
	{
		pointNum++;
		if (pointNum == data)
			return navid->index;

		navid = navid->next;
	}

	if (!IsValidWaypoint(data))
		return pointNum;

	return -1;
}