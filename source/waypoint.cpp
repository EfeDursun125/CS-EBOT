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

ConVar ebot_analyze_distance("ebot_analyze_distance", "75");
ConVar ebot_analyze_disable_fall_connections("ebot_analyze_disable_fall_connections", "0");
ConVar ebot_analyze_wall_check_distance("ebot_analyze_wall_check_distance", "24");
ConVar ebot_analyze_max_jump_height("ebot_analyze_max_jump_height", "44");
ConVar ebot_analyze_goal_check_distance("ebot_analyze_goal_check_distance", "200");
ConVar ebot_analyze_create_camp_waypoints("ebot_analyze_create_camp_waypoints", "1");

// this function initialize the waypoint structures..
void Waypoint::Initialize(void)
{
    // have any waypoint path nodes been allocated yet?
    if (m_waypointPaths)
    {
        for (int i = 0; i < g_numWaypoints; i++)
        {
            delete m_paths[i];
            m_paths[i] = null;
        }
    }

    g_numWaypoints = 0;
    m_lastWaypoint = nullvec;
}

void Waypoint::Analyze(void)
{
    if (g_analyzewaypoints == false)
        return;

    if (g_numWaypoints > 0)
    {
        for (int i = 0; i < g_numWaypoints; i++)
        {
            if (IsValidWaypoint(i))
            {
                Vector WayVec = g_waypoint->GetPath(i)->origin;

                Vector Start;

                float ran = ebot_analyze_distance.GetFloat();

                Start.x = (WayVec.x + engine->RandomFloat(((-ran) - 5.0f), (ran + 5.0f)));
                Start.y = (WayVec.y + engine->RandomFloat(((-ran) - 5.0f), (ran + 5.0f)));
                Start.z = (WayVec.z + engine->RandomFloat(1, ran));

                TraceResult tr;
                TraceResult tr2;

                TraceHull(WayVec, Start, true, human_hull, g_hostEntity, &tr);
                TraceLine(tr.vecEndPos, Vector(tr.vecEndPos.x, tr.vecEndPos.y, -9999.0f), true, false, g_hostEntity, &tr2);

                float lastwaypointaddtime = 0.0f;

                if (tr.flFraction == 1.0f)
                {
                    int startindex = g_waypoint->FindNearest(tr.vecEndPos, ran);

                    if (!IsValidWaypoint(startindex))
                    {
                        if (tr2.flFraction != 1.0f)
                        {
                            Vector TargetPosition = tr2.vecEndPos;
                            TargetPosition.z += 36.0f;
                            int endindex = g_waypoint->FindNearest(TargetPosition, ran);

                            if (!IsValidWaypoint(endindex))
                            {
                                edict_t* ent = null;

                                while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "hostage_entity")))
                                {
                                    // if already saved || moving skip it
                                    if ((ent->v.effects & EF_NODRAW) && (ent->v.speed > 0))
                                        continue;

                                    TraceResult vis;
                                    TraceLine(TargetPosition, GetEntityOrigin(ent), true, false, g_hostEntity, &vis);

                                    if (IsNodeReachable(g_waypoint->GetPath(FindNearest(TargetPosition, 250.0f))->origin, TargetPosition) && IsNodeReachable(TargetPosition, g_waypoint->GetPath(FindNearest(TargetPosition, 250.0f))->origin) && vis.flFraction == 1.0f && (TargetPosition - GetEntityOrigin(ent)).GetLength() <= ebot_analyze_goal_check_distance.GetFloat())
                                    {
                                        lastwaypointaddtime = engine->GetTime();
                                        Add(100, TargetPosition);
                                    }
                                }

                                while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_bomb_target")))
                                {
                                    TraceResult vis;
                                    TraceLine(TargetPosition, GetEntityOrigin(ent), true, false, g_hostEntity, &vis);

                                    if (IsNodeReachable(g_waypoint->GetPath(FindNearest(TargetPosition, 250.0f))->origin, TargetPosition) && IsNodeReachable(TargetPosition, g_waypoint->GetPath(FindNearest(TargetPosition, 250.0f))->origin) && vis.flFraction == 1.0f && (TargetPosition - GetEntityOrigin(ent)).GetLength() <= ebot_analyze_goal_check_distance.GetFloat())
                                    {
                                        lastwaypointaddtime = engine->GetTime();
                                        Add(100, TargetPosition);
                                    }
                                }

                                while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_bomb_target")))
                                {
                                    TraceResult vis;
                                    TraceLine(TargetPosition, GetEntityOrigin(ent), true, false, g_hostEntity, &vis);

                                    if (IsNodeReachable(g_waypoint->GetPath(FindNearest(TargetPosition, 250.0f))->origin, TargetPosition) && IsNodeReachable(TargetPosition, g_waypoint->GetPath(FindNearest(TargetPosition, 250.0f))->origin) && vis.flFraction == 1.0f && (TargetPosition - GetEntityOrigin(ent)).GetLength() <= ebot_analyze_goal_check_distance.GetFloat())
                                    {
                                        lastwaypointaddtime = engine->GetTime();
                                        Add(100, TargetPosition);
                                    }
                                }

                                int doublecheckindex = g_waypoint->FindNearest(TargetPosition, ran);

                                if (!IsValidWaypoint(doublecheckindex))
                                {
                                    g_analyzeputrequirescrouch = false;

                                    TraceResult upcheck;
                                    Vector TargetPosition2 = Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z + 33.0f));
                                    TraceLine(TargetPosition, TargetPosition2, true, false, g_hostEntity, &upcheck);

                                    if (upcheck.flFraction != 1.0f)
                                        g_analyzeputrequirescrouch = true;

                                    lastwaypointaddtime = engine->GetTime();

                                    if ((IsNodeReachable(g_waypoint->GetPath(FindNearest(TargetPosition, 250.0f))->origin, g_analyzeputrequirescrouch ? Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z - 18.0f)) : TargetPosition) &&
                                        IsNodeReachable(g_analyzeputrequirescrouch ? Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z - 18.0f)) : TargetPosition, g_waypoint->GetPath(FindNearest(TargetPosition, 250.0f))->origin))
                                        || (IsNodeReachableWithJump(g_waypoint->GetPath(FindNearest(TargetPosition, 250.0f))->origin, g_analyzeputrequirescrouch ? Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z - 18.0f)) : TargetPosition, -1) && IsNodeReachableWithJump(g_waypoint->GetPath(FindNearest(TargetPosition, 250.0f))->origin, g_analyzeputrequirescrouch ? Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z - 18.0f)) : TargetPosition, -1)))
                                        Add(-1, g_analyzeputrequirescrouch ? Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z - 18.0f)) : TargetPosition);
                                }
                            }
                        }
                    }
                }

                if (tr2.flFraction != 1.0f)
                {
                    if (ebot_analyze_create_camp_waypoints.GetInt() == 1 && lastwaypointaddtime + 0.1f < engine->GetTime()) // delay it for save performance
                    {
                        int campindex = g_waypoint->FindNearest(tr2.vecEndPos, ran);

                        if (!IsValidWaypoint(campindex) || !(g_waypoint->GetPath(campindex)->flags & WAYPOINT_CAMP))
                        {
                            TraceResult tr12;
                            TraceResult tr13;
                            TraceResult tr14;
                            TraceResult tr15;

                            Vector TargetPositionCamp = Vector(tr2.vecEndPos.x, tr2.vecEndPos.y, (tr2.vecEndPos.z + 72.0f));

                            TraceLine(TargetPositionCamp, (TargetPositionCamp + (g_pGlobals->v_forward * ebot_analyze_wall_check_distance.GetFloat())), true, false, g_hostEntity, &tr12); // forward
                            TraceLine(TargetPositionCamp, (TargetPositionCamp - (g_pGlobals->v_forward * ebot_analyze_wall_check_distance.GetFloat())), true, false, g_hostEntity, &tr13); // backward
                            TraceLine(TargetPositionCamp, (TargetPositionCamp + (g_pGlobals->v_right * ebot_analyze_wall_check_distance.GetFloat())), true, false, g_hostEntity, &tr14); // right
                            TraceLine(TargetPositionCamp, (TargetPositionCamp - (g_pGlobals->v_right * ebot_analyze_wall_check_distance.GetFloat())), true, false, g_hostEntity, &tr15); // left

                            int hit1 = 0;
                            int hit2 = 0;
                            int hit3 = 0;
                            int hit4 = 0;

                            if (tr12.flFraction != 1.0f)
                                hit1 = 1;

                            if (tr13.flFraction != 1.0f)
                                hit2 = 1;

                            if (tr14.flFraction != 1.0f)
                                hit3 = 1;

                            if (tr15.flFraction != 1.0f)
                                hit4 = 1;

                            int hitcount = hit1 + hit2 + hit3 + hit4;

                            // its a corner?
                            if (hitcount >= 3)
                            {
                                TraceResult tr8;
                                TraceResult tr9;
                                TraceResult tr10;
                                TraceResult tr11;

                                Vector TargetPosition = Vector(tr2.vecEndPos.x, tr2.vecEndPos.y, (tr2.vecEndPos.z + 36.0f));

                                TraceLine(TargetPosition, (TargetPosition + (g_pGlobals->v_forward * (ebot_analyze_wall_check_distance.GetFloat() / 1.25f))), true, false, g_hostEntity, &tr8); // forward
                                TraceLine(TargetPosition, (TargetPosition - (g_pGlobals->v_forward * (ebot_analyze_wall_check_distance.GetFloat() / 1.25f))), true, false, g_hostEntity, &tr9); // backward
                                TraceLine(TargetPosition, (TargetPosition + (g_pGlobals->v_right * (ebot_analyze_wall_check_distance.GetFloat() / 1.25f))), true, false, g_hostEntity, &tr10); // right
                                TraceLine(TargetPosition, (TargetPosition - (g_pGlobals->v_right * (ebot_analyze_wall_check_distance.GetFloat() / 1.25f))), true, false, g_hostEntity, &tr11); // left

                                if (tr8.flFraction == 1.0f && tr9.flFraction == 1.0f && tr10.flFraction == 1.0f && tr11.flFraction == 1.0f)
                                {
                                    g_analyzeputrequirescrouch = true;
                                    Add(5, Vector(tr2.vecEndPos.x, tr2.vecEndPos.y, (tr2.vecEndPos.z + 18.0f)));
                                    g_analyzeputrequirescrouch = false;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void Waypoint::AnalyzeDeleteUselessWaypoints(void)
{
    int connections;
    int i, j;

    for (i = 0; i < g_numWaypoints; i++)
    {
        connections = 0;

        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (m_paths[i]->index[j] != -1)
            {
                if (m_paths[i]->index[j] > g_numWaypoints)
                    DeleteByIndex(i);

                connections++;
                break;
            }
        }

        if (connections == 0)
        {
            if (!IsConnected(i))
                DeleteByIndex(i);
        }

        if (m_paths[i]->pathNumber != i)
            DeleteByIndex(i);

        for (int k = 0; k < Const_MaxPathIndex; k++)
        {
            if (m_paths[i]->index[k] != -1)
            {
                if (m_paths[i]->index[k] >= g_numWaypoints || m_paths[i]->index[k] < -1)
                    DeleteByIndex(i);
                else if (m_paths[i]->index[k] == i)
                    DeleteByIndex(i);
            }
        }
    }

    CenterPrint("Waypoints are saved!");
    Save();
}

void Waypoint::AddPath(int addIndex, int pathIndex, float distance, int type)
{
    if (!IsValidWaypoint(addIndex) || !IsValidWaypoint(pathIndex) || addIndex == pathIndex)
        return;

    Path* path = m_paths[addIndex];

    // don't allow paths get connected twice
    for (int i = 0; i < Const_MaxPathIndex; i++)
    {
        if (path->index[i] == pathIndex)
        {
            AddLogEntry(LOG_WARNING, "Denied path creation from %d to %d (path already exists)", addIndex, pathIndex);
            return;
        }
    }

    // check for free space in the connection indices
    for (int i = 0; i < Const_MaxPathIndex; i++)
    {
        if (path->index[i] == -1)
        {
            path->index[i] = static_cast <int16> (pathIndex);
            path->distances[i] = abs(static_cast <int> (distance));
            if (type == 1)
            {
                path->connectionFlags[i] |= PATHFLAG_JUMP;
                path->flags |= WAYPOINT_JUMP;
                path->radius = 0.0f;
            }
            else if (type == 2)
            {
                path->connectionFlags[i] |= PATHFLAG_DOUBLE;
                path->flags |= WAYPOINT_DJUMP;
            }
            AddLogEntry(LOG_DEFAULT, "Path added from %d to %d", addIndex, pathIndex);
            return;
        }
    }

    // there wasn't any free space. try exchanging it with a long-distance path
    int maxDistance = -9999;
    int slotID = -1;

    for (int i = 0; i < Const_MaxPathIndex; i++)
    {
        if (path->distances[i] > maxDistance)
        {
            maxDistance = path->distances[i];
            slotID = i;
        }
    }

    if (slotID != -1)
    {
        AddLogEntry(LOG_DEFAULT, "Path added from %d to %d", addIndex, pathIndex);
        path->index[slotID] = static_cast <int16> (pathIndex);
        path->distances[slotID] = abs(static_cast <int> (distance));
        if (type == 1)
        {
            path->connectionFlags[slotID] |= PATHFLAG_JUMP;
            path->flags |= WAYPOINT_JUMP;
            path->radius = 0.0f;
        }
        else if (type == 2)
        {
            path->connectionFlags[slotID] |= PATHFLAG_DOUBLE;
            path->flags |= WAYPOINT_DJUMP;
        }
    }
}

// find the farest node to that origin, and return the index to this node
int Waypoint::FindFarest(const Vector& origin, float maxDistance)
{
    int index = -1;
    for (int i = 0; i < g_numWaypoints; i++)
    {
        float distance = (m_paths[i]->origin - origin).GetLength();
        if (distance > maxDistance)
        {
            index = i;
            maxDistance = distance;
        }
    }
    return index;
}

int Waypoint::FindSafestForHuman(edict_t* enemy, edict_t* self, float maxDistance) // trash
{
    int index = -1;

    for (int i = 0; i < g_numWaypoints; i++)
    {
        float distance = (m_paths[i]->origin - GetEntityOrigin(enemy)).GetLength();

        if (!(::IsVisible(m_paths[i]->origin, self)))
            continue;

        if (!IsInViewCone(m_paths[i]->origin, self))
            continue;

        if (distance > maxDistance)
        {
            index = i;
            maxDistance = distance;
        }
    }

    return index;
}

void Waypoint::ChangeZBCampPoint(Vector origin)
{
    int point[2] = { -1, -1 };
    if (!m_zmHmPoints.IsEmpty())
    {
        for (int i = m_zmHmPoints.GetElementNumber(); i >= 0; i--)
        {
            int wpIndex;
            m_zmHmPoints.GetAt(i, wpIndex);

            if (IsValidWaypoint(wpIndex))
            {
                if (point[0] == -1)
                    point[0] = wpIndex;
                else if (point[1] == -1 && wpIndex != point[0])
                    point[1] = wpIndex;
            }

            if (point[0] != -1 && point[1] != -1)
                break;
        }
    }

    m_zmHmPoints.RemoveAll();

    if (point[1] != -1)
        m_zmHmPoints.Push(point[1]);
    if (point[0] != -1)
        m_zmHmPoints.Push(point[0]);

    int newPoint = FindNearest(origin);
    if (newPoint != -1 && newPoint != point[0] && newPoint != point[1])
        m_zmHmPoints.Push(newPoint);
}

bool Waypoint::IsZBCampPoint(int pointID)
{
    if (g_waypoint->m_zmHmPoints.IsEmpty())
        return false;

    for (int i = 0; i <= m_zmHmPoints.GetElementNumber(); i++)
    {
        int wpIndex;
        m_zmHmPoints.GetAt(i, wpIndex);

        if (pointID == wpIndex)
            return true;
    }

    return false;
}

int Waypoint::FindNearest(Vector origin, float minDistance, int flags, edict_t* entity, int* findWaypointPoint, int mode)
{
    const int checkPoint = 20;

    float wpDistance[checkPoint];
    int wpIndex[checkPoint];

    for (int i = 0; i < checkPoint; i++)
    {
        wpIndex[i] = -1;
        wpDistance[i] = 9999.9f;
    }

    for (int i = 0; i < g_numWaypoints; i++)
    {
        if (flags != -1 && !(m_paths[i]->flags & flags))
            continue;

        float distance = (m_paths[i]->origin - origin).GetLength();
        if (distance > minDistance)
            continue;

        Vector dest = m_paths[i]->origin;
        float distance2D = (dest - origin).GetLength2D();
        if (((dest.z > origin.z + 62.0f || dest.z < origin.z - 100.0f) && !(m_paths[i]->flags & WAYPOINT_LADDER)) && distance2D <= 130.0f)
            continue;

        for (int y = 0; y < checkPoint; y++)
        {
            if (distance >= wpDistance[y])
                continue;

            for (int z = checkPoint - 1; z >= y; z--)
            {
                if (z == checkPoint - 1 || !IsValidWaypoint(wpIndex[z]))
                    continue;

                wpIndex[z + 1] = wpIndex[z];
                wpDistance[z + 1] = wpDistance[z];
            }

            wpIndex[y] = i;
            wpDistance[y] = distance;
            y = checkPoint + 5;
        }
    }

    if (mode != -1)
    {
        int cdWPIndex[checkPoint];
        float cdWPDistance[checkPoint];
        for (int i = 0; i < checkPoint; i++)
        {
            cdWPIndex[i] = -1;
            cdWPDistance[i] = 9999.9f;
        }

        for (int i = 0; i < checkPoint; i++)
        {
            if (!IsValidWaypoint(wpIndex[i]))
                continue;

            float distance = g_waypoint->GetPathDistanceFloat(wpIndex[i], mode);
            for (int y = 0; y < checkPoint; y++)
            {
                if (distance >= cdWPDistance[y])
                    continue;

                for (int z = checkPoint - 1; z >= y; z--)
                {
                    if (z == checkPoint - 1 || !IsValidWaypoint(cdWPIndex[z]))
                        continue;

                    cdWPIndex[z + 1] = cdWPIndex[z];
                    cdWPDistance[z + 1] = cdWPDistance[z];
                }

                cdWPIndex[y] = wpIndex[i];
                cdWPDistance[y] = distance;
                y = checkPoint + 5;
            }
        }

        for (int i = 0; i < checkPoint; i++)
        {
            wpIndex[i] = cdWPIndex[i];
            wpDistance[i] = cdWPDistance[i];
        }
    }

    int firsIndex = -1;
    if (!FNullEnt(entity))
    {
        for (int i = 0; i < checkPoint; i++)
        {
            if (IsValidWaypoint(wpIndex[i]))
                continue;

            if (!Reachable(entity, wpIndex[i]))
                continue;

            if (findWaypointPoint == (int*)-2)
                return wpIndex[i];

            if (!IsValidWaypoint(firsIndex))
            {
                firsIndex = wpIndex[i];
                continue;
            }

            *findWaypointPoint = wpIndex[i];
            return firsIndex;
        }
    }

    if (!IsValidWaypoint(firsIndex))
        firsIndex = wpIndex[0];

    return firsIndex;
}

void Waypoint::FindInRadius(Vector origin, float radius, int* holdTab, int* count)
{
    // returns all waypoints within radius from position

    int maxCount = *count;
    *count = 0;

    for (int i = 0; i < g_numWaypoints; i++)
    {
        if ((m_paths[i]->origin - origin).GetLength() < radius)
        {
            *holdTab++ = i;
            *count += 1;

            if (*count >= maxCount)
                break;
        }
    }

    *count -= 1;
}

void Waypoint::FindInRadius(Array <int>& queueID, float radius, Vector origin)
{
    for (int i = 0; i < g_numWaypoints; i++)
    {
        if ((m_paths[i]->origin - origin).GetLength() < radius)
            queueID.Push(i);
    }
}

void Waypoint::SgdWp_Set(const char* modset)
{
    if (stricmp(modset, "on") == 0)
    {
        if (m_badMapName)
        {
            Initialize();
            Load(1);
            ChartPrint("[SgdWP] I find the bad waypoint data ***");
            ChartPrint("[SgdWP] And I will load your bad waypoint data now ***");
            ChartPrint("[SgdWP] If this is bad waypoint, you need delete this ***");
        }

        ServerCommand("mp_roundtime 9");
        ServerCommand("sv_restart 1");
        ServerCommand("mp_timelimit 0");
        ServerCommand("mp_freezetime 0");

        g_waypointOn = true;
        g_autoWaypoint = false;
        g_sgdWaypoint = true;
        g_sautoWaypoint = false;

        if (g_numWaypoints < 1)
            CreateBasic();

        ChartPrint("[SgdWP] Hold 'E' Call [SgdWP] Menu *******");
    }
    else if (stricmp(modset, "off") == 0)
    {
        g_sautoWaypoint = false;
        g_sgdWaypoint = false;
        g_waypointOn = false;
    }
    else if ((stricmp(modset, "save") == 0 || stricmp(modset, "save_non-check") == 0) && g_sgdWaypoint)
    {
        // SyPB Pro P.45 - SgdWP 
        if (stricmp(modset, "save_non-check") == 0 || g_waypoint->NodesValid())
        {
            Save();
            g_sautoWaypoint = false;
            g_sgdWaypoint = false;
            g_waypointOn = false;

            // SyPB Pro P.38 - SgdWP Msg
            ChartPrint("[SgdWP] Save your waypoint - Finsh *******");
            ChartPrint("[SgdWP] Pls restart the game and re-load the waypoint *******");
            ChartPrint("[SgdWP] Pls restart the game and re-load the waypoint *******");
        }
        else
        {
            g_editNoclip = false;
            ChartPrint("[SgdWP] Cannot Save your waypoint, Your waypoint has the problems!");
        }
    }

    edict_t* spawnEntity = null;
    while (!FNullEnt(spawnEntity = FIND_ENTITY_BY_CLASSNAME(spawnEntity, "info_player_start")))
    {
        if (g_sgdWaypoint)
            spawnEntity->v.effects &= ~EF_NODRAW;
        else
            spawnEntity->v.effects |= EF_NODRAW;
    }

    while (!FNullEnt(spawnEntity = FIND_ENTITY_BY_CLASSNAME(spawnEntity, "info_player_deathmatch")))
    {
        if (g_sgdWaypoint)
            spawnEntity->v.effects &= ~EF_NODRAW;
        else
            spawnEntity->v.effects |= EF_NODRAW;
    }

    while (!FNullEnt(spawnEntity = FIND_ENTITY_BY_CLASSNAME(spawnEntity, "info_vip_start")))
    {
        if (g_sgdWaypoint)
            spawnEntity->v.effects &= ~EF_NODRAW;
        else
            spawnEntity->v.effects |= EF_NODRAW;
    }
}

void Waypoint::Add(int flags, Vector waypointOrigin)
{
    if (FNullEnt(g_hostEntity))
        return;

    int index = -1, i;
    float distance;

    Vector forward = nullvec;
    Path* path = null;

    bool placeNew = true;
    Vector newOrigin = waypointOrigin;

    if (waypointOrigin == nullvec)
        newOrigin = GetEntityOrigin(g_hostEntity);

    if (g_botManager->GetBotsNum() > 0)
        g_botManager->RemoveAll();

    g_waypointsChanged = true;

    switch (flags)
    {
    case 6:
        index = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);

        if (g_waypoint->GetPath(index)->flags & WAYPOINT_SNIPER)
        {
            MakeVectors(g_hostEntity->v.v_angle);
            Vector forceforward = GetEntityOrigin(g_hostEntity) + g_hostEntity->v.view_ofs + g_pGlobals->v_forward * 640;

            m_paths[index]->campEndX = forceforward.x;
            m_paths[index]->campEndY = forceforward.y;
        }
        else
            CenterPrint("This is not a sniper waypoint!");

        return;

    case 9:
        index = FindNearest(GetEntityOrigin(g_hostEntity), 25.0f);

        if (index != -1)
        {
            distance = (m_paths[index]->origin - GetEntityOrigin(g_hostEntity)).GetLength();

            if (distance < 25)
            {
                placeNew = false;
                path = m_paths[index];

                if (flags == 9)
                    path->origin = (path->origin + m_learnPosition) / 2;
            }
        }
        else
            newOrigin = m_learnPosition;
        break;

    case 10:
        index = FindNearest(GetEntityOrigin(g_hostEntity), 25.0f);

        if (index != -1)
        {
            distance = (m_paths[index]->origin - GetEntityOrigin(g_hostEntity)).GetLength();

            if (distance < 25)
            {
                placeNew = false;
                path = m_paths[index];

                int accumFlags = 0;

                for (i = 0; i < Const_MaxPathIndex; i++)
                    accumFlags += path->connectionFlags[i];

                if (accumFlags == 0)
                    path->origin = (path->origin + GetEntityOrigin(g_hostEntity)) / 2;
            }
        }
        break;
    }

    m_lastDeclineWaypoint = index;

    if (placeNew)
    {
        if (g_numWaypoints >= Const_MaxWaypoints)
            return;

        index = g_numWaypoints;

        m_paths[index] = new Path;

        if (m_paths[index] == null)
            return;

        path = m_paths[index];

        // increment total number of waypoints
        g_numWaypoints++;
        path->pathNumber = index;
        path->flags = 0;

        // store the origin (location) of this waypoint
        path->origin = newOrigin;

        path->campEndX = 0;
        path->campEndY = 0;
        path->campStartX = 0;
        path->campStartY = 0;

        for (i = 0; i < Const_MaxPathIndex; i++)
        {
            path->index[i] = -1;
            path->distances[i] = 0;

            path->connectionFlags[i] = 0;
            path->connectionVelocity[i] = nullvec;
        }

        // store the last used waypoint for the auto waypoint code...
        m_lastWaypoint = GetEntityOrigin(g_hostEntity);
    }

    // set the time that this waypoint was originally displayed...
    m_waypointDisplayTime[index] = 0;

    if (flags == 9)
        m_lastJumpWaypoint = index;
    else if (flags == 10)
    {
        distance = (m_paths[m_lastJumpWaypoint]->origin - GetEntityOrigin(g_hostEntity)).GetLength();
        AddPath(m_lastJumpWaypoint, index, distance);

        for (i = 0; i < Const_MaxPathIndex; i++)
        {
            if (m_paths[m_lastJumpWaypoint]->index[i] == index)
            {
                m_paths[m_lastJumpWaypoint]->connectionFlags[i] |= PATHFLAG_JUMP;
                m_paths[m_lastJumpWaypoint]->connectionVelocity[i] = m_learnVelocity;

                break;
            }
        }

        CalculateWayzone(index);

        return;
    }

    // disable autocheck if we're analyzing
    if (g_hostEntity->v.flags & FL_DUCKING && g_analyzewaypoints == false || g_analyzeputrequirescrouch == true)
        path->flags |= WAYPOINT_CROUCH;  // set a crouch waypoint

    if (g_hostEntity->v.movetype == MOVETYPE_FLY && g_analyzewaypoints == false)
    {
        path->flags |= WAYPOINT_LADDER;
        MakeVectors(g_hostEntity->v.v_angle);

        forward = GetEntityOrigin(g_hostEntity) + g_hostEntity->v.view_ofs + g_pGlobals->v_forward * 640;
        path->campStartY = forward.y;
    }
    else if (m_isOnLadder)
        path->flags |= WAYPOINT_LADDER;

    switch (flags)
    {
    case 1:
        path->flags |= WAYPOINT_CROSSING;
        path->flags |= WAYPOINT_TERRORIST;
        break;

    case 2:
        path->flags |= WAYPOINT_CROSSING;
        path->flags |= WAYPOINT_COUNTER;
        break;

    case 3:
        path->flags |= WAYPOINT_AVOID;
        break;

    case 4:
        path->flags |= WAYPOINT_RESCUE;
        break;

    case 5:
        path->flags |= WAYPOINT_CAMP;

        MakeVectors(g_hostEntity->v.v_angle);
        forward = GetEntityOrigin(g_hostEntity) + g_hostEntity->v.view_ofs + g_pGlobals->v_forward * 640;

        path->campStartX = forward.x;
        path->campStartY = forward.y;

        // ebot 1.51 - set end by default for aviod camp waypoint errors, ebot 1.53 - keep camp directions for older versions.
        path->campEndX = forward.x;
        path->campEndY = forward.y;

        break;

    case 6: // ebot 1.51 use button waypoints
        path->flags |= WAYPOINT_USEBUTTON;
        break;

    case 100:
        path->flags |= WAYPOINT_GOAL;
        break;
    }

    // SyPB Pro P.20 - SgdWP
    if (flags == 102)
        m_lastFallWaypoint = index;
    else if (flags == 103 && m_lastFallWaypoint != -1)
    {
        distance = (m_paths[m_lastFallWaypoint]->origin - GetEntityOrigin(g_hostEntity)).GetLength();
        AddPath(m_lastFallWaypoint, index, distance);
        m_lastFallWaypoint = -1;
    }

    if (flags == 104)
        // SyPB Pro P.24 - Zombie Mod Human Camp
        path->flags |= WAYPOINT_ZMHMCAMP;

    // Ladder waypoints need careful connections
    if (path->flags & WAYPOINT_LADDER)
    {
        float minDistance = 9999.0f;
        int destIndex = -1;

        TraceResult tr;

        // calculate all the paths to this new waypoint
        for (i = 0; i < g_numWaypoints; i++)
        {
            if (i == index)
                continue; // skip the waypoint that was just added

             // Other ladder waypoints should connect to this
            if (m_paths[i]->flags & WAYPOINT_LADDER)
            {
                // check if the waypoint is reachable from the new one
                TraceLine(newOrigin, m_paths[i]->origin, true, g_hostEntity, &tr);

                if (tr.flFraction == 1.0f && fabs(newOrigin.x - m_paths[i]->origin.x) < 64 && fabs(newOrigin.y - m_paths[i]->origin.y) < 64 && fabs(newOrigin.z - m_paths[i]->origin.z) < g_autoPathDistance)
                {
                    AddPath(index, i, (m_paths[i]->origin - newOrigin).GetLength());
                    AddPath(i, index, (m_paths[i]->origin - newOrigin).GetLength());
                }
            }
            else
            {
                distance = (m_paths[i]->origin - newOrigin).GetLength();

                if (distance < minDistance)
                {
                    destIndex = i;
                    minDistance = distance;
                }

                if (IsNodeReachable(newOrigin, m_paths[destIndex]->origin))
                    AddPath(index, destIndex, distance);
            }
        }

        if (IsValidWaypoint(destIndex))
        {
            if (g_analyzewaypoints == true)
            {
                AddPath(index, destIndex, (m_paths[destIndex]->origin - newOrigin).GetLength());
                AddPath(destIndex, index, (m_paths[destIndex]->origin - newOrigin).GetLength());
            }
            else
            {
                // check if the waypoint is reachable from the new one (one-way)
                if (IsNodeReachable(newOrigin, m_paths[destIndex]->origin))
                    AddPath(index, destIndex, (m_paths[destIndex]->origin - newOrigin).GetLength());

                // check if the new one is reachable from the waypoint (other way)
                if (IsNodeReachable(m_paths[destIndex]->origin, newOrigin))
                    AddPath(destIndex, index, (m_paths[destIndex]->origin - newOrigin).GetLength());
            }
        }
    }
    else
    {
        // calculate all the paths to this new waypoint
        for (i = 0; i < g_numWaypoints; i++)
        {
            if (i == index)
                continue; // skip the waypoint that was just added

            if (g_analyzewaypoints == true) // if we're analyzing, be careful (we dont want path errors)
            {
                if (g_waypoint->GetPath(i)->flags & WAYPOINT_LADDER && (IsNodeReachable(newOrigin, m_paths[i]->origin) || IsNodeReachableWithJump(newOrigin, m_paths[i]->origin, 0)) && (newOrigin - m_paths[i]->origin).GetLength() <= (ebot_analyze_distance.GetFloat() * 2.25f))
                {
                    AddPath(index, i, (m_paths[i]->origin - newOrigin).GetLength());
                    AddPath(i, index, (m_paths[i]->origin - newOrigin).GetLength());
                }

                if (!IsNodeReachable(newOrigin, m_paths[i]->origin) && IsNodeReachableWithJump(newOrigin, m_paths[i]->origin, m_paths[i]->flags) && (m_paths[i]->origin - newOrigin).GetLength() <= (ebot_analyze_distance.GetFloat() * 2.25f))
                    AddPath(index, i, (m_paths[i]->origin - newOrigin).GetLength(), 1);

                if (!IsNodeReachable(m_paths[i]->origin, newOrigin) && IsNodeReachableWithJump(m_paths[i]->origin, newOrigin, m_paths[index]->flags) && (m_paths[i]->origin - newOrigin).GetLength() <= (ebot_analyze_distance.GetFloat() * 2.25f))
                    AddPath(i, index, (m_paths[i]->origin - newOrigin).GetLength(), 1);

                if (IsNodeReachable(newOrigin, m_paths[i]->origin) && IsNodeReachable(m_paths[i]->origin, newOrigin) && (m_paths[i]->origin - newOrigin).GetLength() <= (ebot_analyze_distance.GetFloat() * 2.25f))
                {
                    AddPath(index, i, (m_paths[i]->origin - newOrigin).GetLength());
                    AddPath(i, index, (m_paths[i]->origin - newOrigin).GetLength());
                }
                else if (ebot_analyze_disable_fall_connections.GetInt() == 0)
                {
                    if (IsNodeReachable(newOrigin, m_paths[i]->origin) && newOrigin.z > m_paths[i]->origin.z && (m_paths[i]->origin - newOrigin).GetLength() <= (ebot_analyze_distance.GetFloat() * 2.25f))
                        AddPath(index, i, (m_paths[i]->origin - newOrigin).GetLength());

                    if (IsNodeReachable(m_paths[i]->origin, newOrigin) && newOrigin.z < m_paths[i]->origin.z && (m_paths[i]->origin - newOrigin).GetLength() <= (ebot_analyze_distance.GetFloat() * 2.25f))
                        AddPath(i, index, (m_paths[i]->origin - newOrigin).GetLength());
                }
            }
            else
            {
                // check if the waypoint is reachable from the new one (one-way)
                if (IsNodeReachable(newOrigin, m_paths[i]->origin))
                    AddPath(index, i, (m_paths[i]->origin - newOrigin).GetLength());

                // check if the new one is reachable from the waypoint (other way)
                if (IsNodeReachable(m_paths[i]->origin, newOrigin))
                    AddPath(i, index, (m_paths[i]->origin - newOrigin).GetLength());
            }
        }
    }

    PlaySound(g_hostEntity, "weapons/xbow_hit1.wav");
    CalculateWayzone(index); // calculate the wayzone of this waypoint
}

void Waypoint::Delete(void)
{
    g_waypointsChanged = true;

    if (g_numWaypoints < 1)
        return;

    if (g_botManager->GetBotsNum() > 0)
        g_botManager->RemoveAll();

    int index = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);

    if (!IsValidWaypoint(index))
        return;

    Path* path = null;
    InternalAssert(m_paths[index] != null);

    int i, j;

    for (i = 0; i < g_numWaypoints; i++) // delete all references to Node
    {
        path = m_paths[i];

        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (path->index[j] == index)
            {
                path->index[j] = -1;  // unassign this path
                path->connectionFlags[j] = 0;
                path->distances[j] = 0;
                path->connectionVelocity[j] = nullvec;
            }
        }
    }

    for (i = 0; i < g_numWaypoints; i++)
    {
        path = m_paths[i];

        if (path->pathNumber > index) // if pathnumber bigger than deleted node...
            path->pathNumber--;

        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (path->index[j] > index)
                path->index[j]--;
        }
    }

    // free deleted node
    delete m_paths[index];
    m_paths[index] = null;

    // Rotate Path Array down
    for (i = index; i < g_numWaypoints - 1; i++)
        m_paths[i] = m_paths[i + 1];

    g_numWaypoints--;
    m_waypointDisplayTime[index] = 0;

    PlaySound(g_hostEntity, "weapons/mine_activate.wav");
}

void Waypoint::DeleteByIndex(int index)
{
    g_waypointsChanged = true;

    if (g_numWaypoints < 1)
        return;

    if (g_botManager->GetBotsNum() > 0)
        g_botManager->RemoveAll();

    if (!IsValidWaypoint(index))
        return;

    Path* path = null;
    InternalAssert(m_paths[index] != null);

    int i, j;

    for (i = 0; i < g_numWaypoints; i++) // delete all references to Node
    {
        path = m_paths[i];

        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (path->index[j] == index)
            {
                path->index[j] = -1;  // unassign this path
                path->connectionFlags[j] = 0;
                path->distances[j] = 0;
                path->connectionVelocity[j] = nullvec;
            }
        }
    }

    for (i = 0; i < g_numWaypoints; i++)
    {
        path = m_paths[i];

        if (path->pathNumber > index) // if pathnumber bigger than deleted node...
            path->pathNumber--;

        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (path->index[j] > index)
                path->index[j]--;
        }
    }

    // free deleted node
    delete m_paths[index];
    m_paths[index] = null;

    // Rotate Path Array down
    for (i = index; i < g_numWaypoints - 1; i++)
        m_paths[i] = m_paths[i + 1];

    g_numWaypoints--;
    m_waypointDisplayTime[index] = 0;

    PlaySound(g_hostEntity, "weapons/mine_activate.wav");
}

void Waypoint::DeleteFlags(void)
{
    int index = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);

    if (index != -1)
    {
        m_paths[index]->flags = 0;
        PlaySound(g_hostEntity, "common/wpn_hudon.wav");
    }
}

void Waypoint::ToggleFlags(int toggleFlag)
{
    // this function allow manually changing flags

    int index = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);

    if (index != -1)
    {
        if (m_paths[index]->flags & toggleFlag)
            m_paths[index]->flags &= ~toggleFlag;

        else if (!(m_paths[index]->flags & toggleFlag))
        {
            if (toggleFlag == WAYPOINT_SNIPER && !(m_paths[index]->flags & WAYPOINT_CAMP))
            {
                AddLogEntry(LOG_ERROR, "Cannot assign sniper flag to waypoint #%d. This is not camp waypoint", index);
                return;
            }
            m_paths[index]->flags |= toggleFlag;
        }

        // play "done" sound...
        PlaySound(g_hostEntity, "common/wpn_hudon.wav");
    }
}

// this function allow manually setting the zone radius
void Waypoint::SetRadius(int radius)
{
    int index = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);

    if (g_sautoWaypoint)
    {
        m_sautoRadius = radius;
        ChartPrint("[SgdWP Auto] Waypoint Radius is: %d ", m_sautoRadius);
    }

    if (index != -1)
    {
        if (g_sautoWaypoint)
        {
            if (m_paths[index]->radius > 0)
                return;
        }

        m_paths[index]->radius = static_cast <float> (radius);
        PlaySound(g_hostEntity, "common/wpn_hudon.wav");
    }
}

// this function checks if waypoint A has a connection to waypoint B
bool Waypoint::IsConnected(int pointA, int pointB)
{
    for (int i = 0; i < Const_MaxPathIndex; i++)
    {
        if (m_paths[pointA]->index[i] == pointB)
            return true;
    }
    return false;
}

// this function finds waypoint the user is pointing at.
int Waypoint::GetFacingIndex(void)
{
    int pointedIndex = -1;
    float viewCone[3] = { 0.0, 0.0, 0.0 };

    // find the waypoint the user is pointing at
    for (int i = 0; i < g_numWaypoints; i++)
    {
        if ((m_paths[i]->origin - GetEntityOrigin(g_hostEntity)).GetLengthSquared() > 250000)
            continue;

        // get the current view cone
        viewCone[0] = GetShootingConeDeviation(g_hostEntity, &m_paths[i]->origin);
        Vector bound = m_paths[i]->origin - Vector(0.0f, 0.0f, m_paths[i]->flags & WAYPOINT_CROUCH ? 8.0f : 15.0f);

        // get the current view cone
        viewCone[1] = GetShootingConeDeviation(g_hostEntity, &bound);
        bound = m_paths[i]->origin + Vector(0.0f, 0.0f, m_paths[i]->flags & WAYPOINT_CROUCH ? 8.0f : 15.0f);

        // get the current view cone
        viewCone[2] = GetShootingConeDeviation(g_hostEntity, &bound);

        // check if we can see it
        if (viewCone[0] < 0.998f && viewCone[1] < 0.997f && viewCone[2] < 0.997f)
            continue;

        pointedIndex = i;
    }

    return pointedIndex;
}

void Waypoint::CreatePath(char dir)
{
    // this function allow player to manually create a path from one waypoint to another

    int nodeFrom = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);

    if (nodeFrom == -1)
    {
        CenterPrint("Unable to find nearest waypoint in 75 units");
        return;
    }

    int nodeTo = m_facingAtIndex;

    if (!IsValidWaypoint(nodeTo))
    {
        if (IsValidWaypoint(m_cacheWaypointIndex))
            nodeTo = m_cacheWaypointIndex;
        else
        {
            CenterPrint("Unable to find destination waypoint");
            return;
        }
    }

    if (nodeTo == nodeFrom)
    {
        CenterPrint("Unable to connect waypoint with itself");
        return;
    }

    float distance = (m_paths[nodeTo]->origin - m_paths[nodeFrom]->origin).GetLength();

    if (dir == PATHCON_OUTGOING)
        AddPath(nodeFrom, nodeTo, distance);
    else if (dir == PATHCON_INCOMING)
        AddPath(nodeTo, nodeFrom, distance);
    else if (dir == PATHCON_JUMPING)
        AddPath(nodeFrom, nodeTo, distance, 1);
    else if (dir == PATHCON_BOOSTING)
        AddPath(nodeFrom, nodeTo, distance, 2);
    else if (dir == PATHCON_VISIBLE)
        AddPath(nodeFrom, nodeTo, distance, 3);
    else
    {
        AddPath(nodeFrom, nodeTo, distance);
        AddPath(nodeTo, nodeFrom, distance);
    }

    PlaySound(g_hostEntity, "common/wpn_hudon.wav");
    g_waypointsChanged = true;
}


// SyPB Pro P.12
void Waypoint::TeleportWaypoint(void)
{
    m_facingAtIndex = GetFacingIndex();

    if (m_facingAtIndex != -1)
        (*g_engfuncs.pfnSetOrigin) (g_hostEntity, g_waypoint->m_paths[m_facingAtIndex]->origin);
}

void Waypoint::DeletePath(void)
{
    // this function allow player to manually remove a path from one waypoint to another

    int nodeFrom = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);
    int index = 0;

    if (!IsValidWaypoint(nodeFrom))
    {
        CenterPrint("Unable to find nearest waypoint in 75 units");
        return;
    }

    int nodeTo = m_facingAtIndex;

    if (!IsValidWaypoint(nodeTo))
    {
        if (IsValidWaypoint(m_cacheWaypointIndex))
            nodeTo = m_cacheWaypointIndex;
        else
        {
            CenterPrint("Unable to find destination waypoint");
            return;
        }
    }

    for (index = 0; index < Const_MaxPathIndex; index++)
    {
        if (m_paths[nodeFrom]->index[index] == nodeTo)
        {
            g_waypointsChanged = true;

            m_paths[nodeFrom]->index[index] = -1; // unassign this path
            m_paths[nodeFrom]->connectionFlags[index] = 0;
            m_paths[nodeFrom]->connectionVelocity[index] = nullvec;
            m_paths[nodeFrom]->distances[index] = 0;

            PlaySound(g_hostEntity, "weapons/mine_activate.wav");
            return;
        }
    }

    // not found this way ? check for incoming connections then
    index = nodeFrom;
    nodeFrom = nodeTo;
    nodeTo = index;

    for (index = 0; index < Const_MaxPathIndex; index++)
    {
        if (m_paths[nodeFrom]->index[index] == nodeTo)
        {
            g_waypointsChanged = true;

            m_paths[nodeFrom]->index[index] = -1; // unassign this path
            m_paths[nodeFrom]->connectionFlags[index] = 0;
            m_paths[nodeFrom]->connectionVelocity[index] = nullvec;
            m_paths[nodeFrom]->distances[index] = 0;

            PlaySound(g_hostEntity, "weapons/mine_activate.wav");
            return;
        }
    }

    CenterPrint("There is already no path on this waypoint");
}

void Waypoint::CacheWaypoint(void)
{
    int node = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);

    if (!IsValidWaypoint(node))
    {
        m_cacheWaypointIndex = -1;
        CenterPrint("Cached waypoint cleared (nearby point not found in 75 units range)");

        return;
    }

    m_cacheWaypointIndex = node;
    CenterPrint("Waypoint #%d has been put into memory", m_cacheWaypointIndex);
}

// calculate "wayzones" for the nearest waypoint to pentedict (meaning a dynamic distance area to vary waypoint origin)
void Waypoint::CalculateWayzone(int index)
{
    Path* path = m_paths[index];
    Vector start, direction;

    TraceResult tr;
    bool wayBlocked = false;

    if ((path->flags & (WAYPOINT_LADDER | WAYPOINT_GOAL | WAYPOINT_CAMP | WAYPOINT_RESCUE | WAYPOINT_CROUCH)) || m_learnJumpWaypoint)
    {
        path->radius = 0.0f;
        return;
    }

    for (int i = 0; i < Const_MaxPathIndex; i++)
    {
        if (path->index[i] != -1 && (m_paths[path->index[i]]->flags & WAYPOINT_LADDER))
        {
            path->radius = 0.0f;
            return;
        }
    }

    for (float scanDistance = 16.0f; scanDistance < 144.0f; scanDistance += 16.0f)
    {
        start = path->origin;
        MakeVectors(nullvec);

        direction = g_pGlobals->v_forward * scanDistance;
        direction = direction.ToAngles();

        path->radius = scanDistance;

        for (float circleRadius = 0.0f; circleRadius < 180.0f; circleRadius += 5.0f)
        {
            MakeVectors(direction);

            Vector radiusStart = start - g_pGlobals->v_forward * scanDistance;
            Vector radiusEnd = start + g_pGlobals->v_forward * scanDistance;

            TraceHull(radiusStart, radiusEnd, true, head_hull, null, &tr);

            if (tr.flFraction < 1.0f)
            {
                TraceLine(radiusStart, radiusEnd, true, null, &tr);

                if (FClassnameIs(tr.pHit, "func_door") || FClassnameIs(tr.pHit, "func_door_rotating"))
                {
                    path->radius = 0.0f;
                    wayBlocked = true;

                    break;
                }

                wayBlocked = true;
                path->radius -= 16.0f;

                break;
            }

            Vector dropStart = start + (g_pGlobals->v_forward * scanDistance);
            Vector dropEnd = dropStart - Vector(0.0f, 0.0f, scanDistance + 60.0f);

            TraceHull(dropStart, dropEnd, true, head_hull, null, &tr);

            if (tr.flFraction >= 1.0f)
            {
                wayBlocked = true;
                path->radius -= 16.0f;

                break;
            }
            dropStart = start - (g_pGlobals->v_forward * scanDistance);
            dropEnd = dropStart - Vector(0.0f, 0.0f, scanDistance + 60.0f);

            TraceHull(dropStart, dropEnd, true, head_hull, null, &tr);

            if (tr.flFraction >= 1.0f)
            {
                wayBlocked = true;
                path->radius -= 16.0f;
                break;
            }

            radiusEnd.z += 34.0f;
            TraceHull(radiusStart, radiusEnd, true, head_hull, null, &tr);

            if (tr.flFraction < 1.0f)
            {
                wayBlocked = true;
                path->radius -= 16.0f;
                break;
            }

            direction.y = AngleNormalize(direction.y + circleRadius);
        }
        if (wayBlocked)
            break;
    }
    path->radius -= 16.0f;

    if (path->radius < 0.0f)
        path->radius = 0.0f;
}

void Waypoint::InitTypes(int mode)
{
    if (mode == 0)
    {
        m_terrorPoints.RemoveAll();
        m_ctPoints.RemoveAll();
        m_goalPoints.RemoveAll();
        m_campPoints.RemoveAll();
        m_rescuePoints.RemoveAll();
        m_sniperPoints.RemoveAll();
        m_visitedGoals.RemoveAll();
        m_zmHmPoints.RemoveAll();
        m_otherPoints.RemoveAll();
    }

    for (int i = 0; i < g_numWaypoints; i++)
    {
        if (mode == 0)
        {
            if (m_paths[i]->flags & WAYPOINT_TERRORIST)
                m_terrorPoints.Push(i);
            else if (m_paths[i]->flags & WAYPOINT_COUNTER)
                m_ctPoints.Push(i);
            else if (m_paths[i]->flags & WAYPOINT_GOAL)
                m_goalPoints.Push(i);
            else if (m_paths[i]->flags & WAYPOINT_CAMP)
                m_campPoints.Push(i);
            else if (m_paths[i]->flags & WAYPOINT_SNIPER)
                m_sniperPoints.Push(i);
            else if (m_paths[i]->flags & WAYPOINT_RESCUE)
                m_rescuePoints.Push(i);
            else if (m_paths[i]->flags & WAYPOINT_ZMHMCAMP)
                m_zmHmPoints.Push(i);

            if (!(m_paths[i]->flags & WAYPOINT_AVOID) && !(m_paths[i]->flags & WAYPOINT_CROUCH) && !(m_paths[i]->flags & WAYPOINT_FALLCHECK) && !(m_paths[i]->flags & WAYPOINT_LADDER))
                m_otherPoints.Push(i);
        }
    }
}

bool Waypoint::Load(int mode)
{
    WaypointHeader header;
    File fp(CheckSubfolderFile(), "rb");

    m_badMapName = false;

    if (fp.IsValid())
    {
        fp.Read(&header, sizeof(header));

        if (strncmp(header.header, FH_WAYPOINT, strlen(FH_WAYPOINT)) == 0)
        {
            if (stricmp(header.mapName, GetMapName()) && mode == 0)
            {
                m_badMapName = true;

                sprintf(m_infoBuffer, "%s.pwf - hacked waypoint file, fileName doesn't match waypoint header information (mapname: '%s', header: '%s')", GetMapName(), GetMapName(), header.mapName);
                AddLogEntry(LOG_ERROR, m_infoBuffer);

                fp.Close();
                return false;
            }
            else
            {
                Initialize();
                g_numWaypoints = header.pointNumber;

                for (int i = 0; i < g_numWaypoints; i++)
                {
                    m_paths[i] = new Path;

                    if (m_paths[i] == null)
                        return false;

                    fp.Read(m_paths[i], sizeof(Path));
                }

                m_waypointPaths = true;
            }
        }
        else
        {
           
            sprintf(m_infoBuffer, "%s.pwf is not a ebot waypoint file (header found '%s' needed '%s'", GetMapName(), header.header, FH_WAYPOINT);
            AddLogEntry(LOG_ERROR, m_infoBuffer);
            fp.Close();
            return false;
        }

        fp.Close();
    }
    else
    {
        sprintf(m_infoBuffer, "%s.pwf does not exist, pleasue use 'ebot wp analyze' for create waypoints! (dont forget using 'ebot wp analyzeoff' when finished)", GetMapName());
        AddLogEntry(LOG_ERROR, m_infoBuffer);

        return false;
    }

    if (strncmp(header.author, "EfeDursun125", 12) == 0)
        sprintf(m_infoBuffer, "Using Official Waypoint File By: %s", header.author);
    else
        sprintf(m_infoBuffer, "Using Waypoint File By: %s", header.author);

    for (int i = 0; i < g_numWaypoints; i++)
        m_waypointDisplayTime[i] = 0.0f;

    InitPathMatrix();
    InitTypes(0);

    g_waypointsChanged = false;
    g_killHistory = 0;

    m_pathDisplayTime = 0.0f;
    m_arrowDisplayTime = 0.0f;

    g_exp.Load();

    g_botManager->InitQuota();

    extern ConVar ebot_debuggoal;
    ebot_debuggoal.SetInt(-1);

    return true;
}

void Waypoint::Save(void)
{
    WaypointHeader header;

    memset(header.mapName, 0, sizeof(header.mapName));
    memset(header.author, 0, sizeof(header.author));
    memset(header.header, 0, sizeof(header.header));

    char waypointAuthor[32];
    sprintf(waypointAuthor, "%s", GetEntityName(g_hostEntity));

    strcpy(header.header, FH_WAYPOINT);
    strcpy(header.author, waypointAuthor);
    strncpy(header.mapName, GetMapName(), 31);

    header.mapName[31] = 0;
    header.fileVersion = FV_WAYPOINT;
    header.pointNumber = g_numWaypoints;

    File fp(CheckSubfolderFile(), "wb");

    // file was opened
    if (fp.IsValid())
    {
        // write the waypoint header to the file...
        fp.Write(&header, sizeof(header), 1);

        // save the waypoint paths...
        for (int i = 0; i < g_numWaypoints; i++)
            fp.Write(m_paths[i], sizeof(Path));

        fp.Close();
    }
    else
        AddLogEntry(LOG_ERROR, "Error writing '%s.pwf' waypoint file", GetMapName());
}

String Waypoint::CheckSubfolderFile(void)
{
    String returnFile = "";
    returnFile = FormatBuffer("%s/%s.pwf", GetWaypointDir(), GetMapName());

    if (TryFileOpen(returnFile))
        return returnFile;

    return FormatBuffer("%s%s.pwf", GetWaypointDir(), GetMapName());
}

void Waypoint::SaveXML(void)
{
    File fp(FormatBuffer("%sdata/%s.xml", GetWaypointDir(), GetMapName()), "w");

    if (fp.IsValid())
    {
        int j;

        fp.Print("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
        fp.Print("<!-- Generated by %s v%s. You may not edit this file! -->\n", PRODUCT_NAME, PRODUCT_VERSION);
        fp.Print("<WaypointData>\n");
        fp.Print("\t<header>\n");
        fp.Print("\t\t<author>%s</author>\n", GetEntityName(g_hostEntity));
        fp.Print("\t\t<map>%s</map>\n", GetMapName());
        fp.Print("\t\t<version>%d</version>\n", FV_WAYPOINT);
        fp.Print("\t\t<number>%d</number>\n", g_numWaypoints);
        fp.Print("\t</header>\n");
        fp.Print("\t<map>\n");

        // save the waypoint paths...
        for (int i = 0; i < g_numWaypoints; i++)
        {
            Path* path = m_paths[i];

            fp.Print("\t\t<waypoint id=\"%d\">\n", i + 1);
            fp.Print("\t\t\t<campend x=\"%.2f\" y=\"%.2f\"/>\n", path->campEndX, path->campEndY);
            fp.Print("\t\t\t<campstart x=\"%.2f\" y=\"%.2f\"/>\n", path->campStartX, path->campStartY);
            fp.Print("\t\t\t<flags>%d</flags>\n", path->flags);
            fp.Print("\t\t\t<pathnum>%d</pathnum>\n", path->pathNumber);
            fp.Print("\t\t\t<radius>%.2f</radius>\n", path->radius);
            fp.Print("\t\t\t<origin x=\"%.2f\" y=\"%.2f\" z=\"%.2f\"/>\n", path->origin.x, path->origin.y, path->origin.z);
            fp.Print("\t\t\t<connections size=\"%d\">\n", Const_MaxPathIndex);

            for (j = 0; j < Const_MaxPathIndex; j++)
            {
                fp.Print("\t\t\t\t<connection id=\"%d\">\n", j + 1);
                fp.Print("\t\t\t\t\t<velocity x=\"%.2f\" y=\"%.2f\" z=\"%.2f\"/>\n", path->connectionVelocity[j].x, path->connectionVelocity[j].y, path->connectionVelocity[j].z);
                fp.Print("\t\t\t\t\t<distance>%d</distance>\n", path->distances[j]);
                fp.Print("\t\t\t\t\t<index>%d</index>\n", path->index[j]);
                fp.Print("\t\t\t\t\t<flags>%u</flags>\n", path->connectionFlags[j]);
                fp.Print("\t\t\t\t</connection>\n");
            }
            fp.Print("\t\t\t</connections>\n");

            fp.Print("\t\t</waypoint>\n");

        }
        fp.Print("\t</map>\n");
        fp.Print("</WaypointData>");
        fp.Close();
    }
    else
        AddLogEntry(LOG_ERROR, "Error writing '%s.xml' waypoint file, no waypoints/data folder found!", GetMapName());
}

// this function returns 2D traveltime to a position
float Waypoint::GetTravelTime(float maxSpeed, Vector src, Vector origin)
{
    return (origin - src).GetLength2D() / fabsf(maxSpeed);
}

bool Waypoint::Reachable(edict_t* entity, int index)
{
    if (!IsValidWaypoint(index))
        return false;

    Vector src = GetEntityOrigin(entity);
    Vector dest = m_paths[index]->origin;

    if ((dest - src).GetLength() >= 1200.0f)
        return false;

    if (entity->v.waterlevel != 2 && entity->v.waterlevel != 3)
    {
        if ((dest.z > src.z + 62.0f || dest.z < src.z - 100.0f) &&
            (!(GetPath(index)->flags & WAYPOINT_LADDER) || (dest - src).GetLength2D() >= 120.0f))
            return false;
    }

    TraceResult tr;
    TraceHull(src, dest, true, human_hull, entity, &tr);
    if (tr.flFraction == 1.0f)
        return true;

    return false;
}

bool Waypoint::IsNodeReachable(Vector src, Vector destination)
{
    // cant connect to itself...
    if (src == destination)
        return false;

    // unable to reach with walking
    if ((destination.z - src.z) > 64.0f)
        return false;

    TraceResult tr{};

    float distance = (destination - src).GetLength();

    // is the destination not close enough?
    if (distance > g_autoPathDistance)
        return false;

    // check if this waypoint is "visible"...
    TraceHull(src, destination, true, human_hull, g_hostEntity, &tr);

    // check if we go through a func_illusionary, in which case return false
    if (tr.pHit && strcmp("func_illusionary", STRING(tr.pHit->v.classname)) == 0)
        return false; // don't add pathnodes through func_illusionaries

    TraceLine(src, destination, true, true, g_hostEntity, &tr);

    // if waypoint is visible from current position (even behind head)...
    if (tr.pHit && (tr.flFraction >= 1.0f || strncmp("func_door", STRING(tr.pHit->v.classname), 9) == 0))
    {
        // if it's a door check if nothing blocks behind
        if (strncmp("func_door", STRING(tr.pHit->v.classname), 9) == 0)
        {
            TraceLine(tr.vecEndPos, destination, true, true, tr.pHit, &tr);

            if (tr.flFraction < 1.0f)
                return false;
        }

        // check for special case of both nodes being in water...
        if (POINT_CONTENTS(src) == CONTENTS_WATER && POINT_CONTENTS(destination) == CONTENTS_WATER)
            return true; // then they're reachable each other

        // is dest node higher than src? (45 is max jump height)
        if (destination.z > src.z + 44.0f)
        {
            Vector sourceNew = destination;
            Vector destinationNew = destination;
            destinationNew.z = destinationNew.z - 50.0f; // straight down 50 units

            TraceHull(sourceNew, destinationNew, true, human_hull, g_hostEntity, &tr);

            // check if we didn't hit anything, if not then it's in mid-air
            if (tr.flFraction >= 1.0f)
                return false; // can't reach this one
        }

        // check if distance to ground drops more than step height at points between source and destination...
        Vector direction = (destination - src).Normalize(); // 1 unit long
        Vector check = src, down = src;

        down.z = down.z - 1000.0f; // straight down 1000 units

        TraceLine(check, down, true, true, g_hostEntity, &tr);

        float lastHeight = tr.flFraction * 1000.0f; // height from ground
        distance = (destination - check).GetLength(); // distance from goal

        while (distance > 10.0f)
        {
            // move 10 units closer to the goal...
            check = check + (direction * 10.0f);

            down = check;
            down.z = down.z - 1000.0f; // straight down 1000 units

            TraceLine(check, down, true, true, g_hostEntity, &tr);

            float height = tr.flFraction * 1000.0f; // height from ground

            // is the current height greater than the step height?
            if (height < lastHeight - 18.0f)
                return false; // can't get there without jumping...

            lastHeight = height;
            distance = (destination - check).GetLength(); // distance from goal
        }

        return true;
    }

    return false;
}

bool Waypoint::IsNodeReachableWithJump(Vector src, Vector destination, int flags)
{
    // cant connect to itself...
    if (src == destination)
        return false;

    if (flags > 0)
        return false;

    if ((destination.z - src.z) > ebot_analyze_max_jump_height.GetFloat())
        return false;

    TraceResult tr{};

    float distance = (destination - src).GetLength();

    // is the destination not close enough?
    if (distance > g_autoPathDistance)
        return false;

    // check if this waypoint is "visible"...
    TraceHull(src, destination, true, human_hull, g_hostEntity, &tr);

    // check if we go through a func_illusionary, in which case return false
    if (tr.pHit && strcmp("func_illusionary", STRING(tr.pHit->v.classname)) == 0)
        return false; // don't add pathnodes through func_illusionaries

    TraceLine(src, destination, true, true, g_hostEntity, &tr);

    // if waypoint is visible from current position (even behind head)...
    if (tr.pHit && (tr.flFraction >= 1.0f || strncmp("func_door", STRING(tr.pHit->v.classname), 9) == 0))
    {
        // if it's a door check if nothing blocks behind
        if (strncmp("func_door", STRING(tr.pHit->v.classname), 9) == 0)
        {
            TraceLine(tr.vecEndPos, destination, true, true, tr.pHit, &tr);

            if (tr.flFraction < 1.0f)
                return false;
        }

        // check for special case of both nodes being in water...
        if (POINT_CONTENTS(src) == CONTENTS_WATER && POINT_CONTENTS(destination) == CONTENTS_WATER)
            return true; // then they're reachable each other

        // check if distance to ground drops more than step height at points between source and destination...
        Vector direction = (destination - src).Normalize(); // 1 unit long
        Vector check = src, down = src;

        down.z = down.z - 1000.0f; // straight down 1000 units

        TraceLine(check, down, true, true, g_hostEntity, &tr);

        float lastHeight = tr.flFraction * 1000.0f; // height from ground
        distance = (destination - check).GetLength(); // distance from goal

        while (distance > 10.0f)
        {
            // move 10 units closer to the goal...
            check = check + (direction * 10.0f);

            down = check;
            down.z = down.z - 1000.0f; // straight down 1000 units

            TraceLine(check, down, true, true, g_hostEntity, &tr);

            float height = tr.flFraction * 1000.0f; // height from ground

            // is the current height greater than the step height?
            if (height < lastHeight - ebot_analyze_max_jump_height.GetFloat())
                return false; // can't get there without jumping...

            lastHeight = height;
            distance = (destination - check).GetLength(); // distance from goal
        }

        return true;
    }

    return false;
}

void Waypoint::InitializeVisibility(void)
{
    if (m_redoneVisibility == false)
        return;

    TraceResult tr;
    uint8_t res, shift;

    for (m_visibilityIndex = 0; m_visibilityIndex < g_numWaypoints; m_visibilityIndex++)
    {
        Vector sourceDuck = m_paths[m_visibilityIndex]->origin;
        Vector sourceStand = m_paths[m_visibilityIndex]->origin;

        if (m_paths[m_visibilityIndex]->flags & WAYPOINT_CROUCH)
        {
            sourceDuck.z += 12.0f;
            sourceStand.z += 18.0f + 28.0f;
        }
        else
        {
            sourceDuck.z += -18.0f + 12.0f;
            sourceStand.z += 28.0f;
        }

        uint16 standCount = 0, crouchCount = 0;

        for (int i = 0; i < g_numWaypoints; i++)
        {
            // first check ducked visibility
            Vector dest = m_paths[i]->origin;

            TraceLine(sourceDuck, dest, true, null, &tr);

            // check if line of sight to object is not blocked (i.e. visible)
            if ((tr.flFraction != 1.0f) || tr.fStartSolid)
                res = 1;
            else
                res = 0;

            res <<= 1;

            TraceLine(sourceStand, dest, true, null, &tr);

            // check if line of sight to object is not blocked (i.e. visible)
            if ((tr.flFraction != 1.0f) || tr.fStartSolid)
                res |= 1;

            shift = (i % 4) << 1;
            m_visLUT[m_visibilityIndex][i >> 2] &= ~(3 << shift);
            m_visLUT[m_visibilityIndex][i >> 2] |= res << shift;

            if (!(res & 2))
                crouchCount++;

            if (!(res & 1))
                standCount++;
        }
        m_paths[m_visibilityIndex]->vis.crouch = crouchCount;
        m_paths[m_visibilityIndex]->vis.stand = standCount;
    }

    m_redoneVisibility = false;
}

bool Waypoint::IsVisible(int srcIndex, int destIndex)
{
    if (!IsValidWaypoint(srcIndex) || !IsValidWaypoint(destIndex))
        return false;

    uint8_t res = m_visLUT[srcIndex][destIndex >> 2];
    res >>= (destIndex % 4) << 1;

    return !((res & 3) == 3);
}

bool Waypoint::IsDuckVisible(int srcIndex, int destIndex)
{
    if (!IsValidWaypoint(srcIndex) || !IsValidWaypoint(destIndex))
        return false;

    uint8_t res = m_visLUT[srcIndex][destIndex >> 2];
    res >>= (destIndex % 4) << 1;

    return !((res & 2) == 2);
}

bool Waypoint::IsStandVisible(int srcIndex, int destIndex)
{
    // SyPB Pro P.42 - Base improve
    if (!IsValidWaypoint(srcIndex) || !IsValidWaypoint(destIndex))
        return false;

    uint8_t res = m_visLUT[srcIndex][destIndex >> 2];
    res >>= (destIndex % 4) << 1;

    return !((res & 1) == 1);
}

char* Waypoint::GetWaypointInfo(int id)
{
    // this function returns path information for waypoint pointed by id.

    Path* path = GetPath(id);

    // if this path is null, return
    if (path == null)
        return "\0";

    bool jumpPoint = false;

    // iterate through connections and find, if it's a jump path
    for (int i = 0; i < Const_MaxPathIndex; i++)
    {
        // check if we got a valid connection
        if (path->index[i] != -1 && (path->connectionFlags[i] & PATHFLAG_JUMP))
        {
            jumpPoint = true;

            if (!(path->flags & WAYPOINT_JUMP))
                path->flags = WAYPOINT_JUMP;
        }
    }

    static char messageBuffer[1024];
    sprintf(messageBuffer, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", (path->flags == 0 && !jumpPoint) ? " (none)" : "", path->flags & WAYPOINT_LIFT ? " LIFT" : "", path->flags & WAYPOINT_CROUCH ? " CROUCH" : "", path->flags & WAYPOINT_CROSSING ? " CROSSING" : "", path->flags & WAYPOINT_CAMP ? " CAMP" : "", path->flags & WAYPOINT_TERRORIST ? " TERRORIST" : "", path->flags & WAYPOINT_COUNTER ? " CT" : "", path->flags & WAYPOINT_SNIPER ? " SNIPER" : "", path->flags & WAYPOINT_GOAL ? " GOAL" : "", path->flags & WAYPOINT_LADDER ? " LADDER" : "", path->flags & WAYPOINT_RESCUE ? " RESCUE" : "", path->flags & WAYPOINT_DJUMP ? " JUMPHELP" : "", path->flags & WAYPOINT_AVOID ? " AVOID" : "", path->flags & WAYPOINT_USEBUTTON ? " USE BUTTON" : "", path->flags & WAYPOINT_FALLCHECK ? " FALL CHECK" : "", jumpPoint ? " JUMP" : "");

    // SyPB Pro P.29 - Zombie Mode Hm Camp Waypoints
    if (path->flags & WAYPOINT_ZMHMCAMP)
        sprintf(messageBuffer, "ZOMBIE MODE HUMAN CAMP");

    // return the message buffer
    return messageBuffer;
}

void Waypoint::Think(void)
{
    // this function executes frame of waypoint operation code.

    if (FNullEnt(g_hostEntity))
        return; // this function is only valid on listenserver, and in waypoint enabled mode.

    float nearestDistance = FLT_MAX;

    if (m_learnJumpWaypoint)
    {
        if (!m_endJumpPoint)
        {
            if (g_hostEntity->v.button & IN_JUMP)
            {
                Add(9);
                m_timeJumpStarted = engine->GetTime();
                m_endJumpPoint = true;
            }
            else
            {
                Vector origin = GetWalkablePosition(GetEntityOrigin(g_hostEntity));
                m_learnVelocity = g_hostEntity->v.velocity;
                m_learnPosition = Vector(origin.x, origin.y, (origin.z + 36.0f));
            }
        }
        else if (((g_hostEntity->v.flags & FL_ONGROUND) || g_hostEntity->v.movetype == MOVETYPE_FLY) && m_timeJumpStarted + 0.1 < engine->GetTime())
        {
            Add(10);

            m_learnJumpWaypoint = false;
            m_endJumpPoint = false;
        }
    }

    // SyPB Pro P.20 - SgdWP
    if (g_sgdWaypoint)
    {
        if (g_autoWaypoint)
            g_autoWaypoint = false;

        g_hostEntity->v.health = fabsf(static_cast <float> (255.0));

        if (g_hostEntity->v.button & IN_USE && (g_hostEntity->v.flags & FL_ONGROUND))
        {
            if (m_timeGetProTarGet == 0.0f)
                m_timeGetProTarGet = engine->GetTime();
            else if (m_timeGetProTarGet + 1.0 < engine->GetTime())
            {
                DisplayMenuToClient(g_hostEntity, &g_menus[21]);
                m_timeGetProTarGet = 0.0f;
            }
        }
        else
            m_timeGetProTarGet = 0.0f;

        if (g_sautoWaypoint)
        {
            if (!m_ladderPoint)
            {
                if ((g_hostEntity->v.movetype == MOVETYPE_FLY) && !(g_hostEntity->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)))
                {
                    if (FindNearest(GetEntityOrigin(g_hostEntity), 75.0f, WAYPOINT_LADDER) == -1)
                    {
                        Add(3);
                        SetRadius(0);
                    }

                    m_ladderPoint = true;
                }
            }
            else
            {
                if ((g_hostEntity->v.movetype == MOVETYPE_FLY) && !(g_hostEntity->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)))
                {
                    if (FindNearest(GetEntityOrigin(g_hostEntity), 75.0f, WAYPOINT_LADDER) == -1)
                    {
                        Add(3);
                        SetRadius(0);
                    }
                }
            }

            if (g_hostEntity->v.flags & (FL_ONGROUND | FL_PARTIALGROUND))
            {
                if (m_ladderPoint && !(g_hostEntity->v.movetype == MOVETYPE_FLY))
                {
                    Add(0);
                    SetRadius(m_sautoRadius);
                    m_ladderPoint = false;
                }

                if (m_fallPosition != nullvec && m_fallPoint)
                {
                    // SyPB Pro P.23 - SgdWP
                    if (m_fallPosition.z > (GetEntityOrigin(g_hostEntity).z + 150.0f))
                    {
                        Add(102, m_fallPosition);
                        SetRadius(m_sautoRadius);
                        Add(103);
                        SetRadius(m_sautoRadius);
                    }

                    m_fallPoint = false;
                    m_fallPosition = nullvec;
                }

                if (g_hostEntity->v.button & IN_DUCK)
                {
                    if (m_timeCampWaypoint == 0.0f)
                        m_timeCampWaypoint = engine->GetTime();
                    else if (m_timeCampWaypoint + 2.5 < engine->GetTime())
                    {
                        // SyPB Pro P.37 - SgdWP
                        m_timeCampWaypoint = 0.0f;
                        DisplayMenuToClient(g_hostEntity, &g_menus[22]);
                    }
                }
                else
                    m_timeCampWaypoint = 0.0f;

                float distance = (m_lastWaypoint - GetEntityOrigin(g_hostEntity)).GetLength();
                int newWaypointDistance = (g_numWaypoints >= 800) ? 16384 : 12000;

                // SyPB Pro P.37 - SgdWP
                if (g_waypoint->GetPath(g_waypoint->FindNearest(m_lastWaypoint, 10.0f))->radius == 0.0f)
                    newWaypointDistance = 10000;

                if (distance > newWaypointDistance)
                {
                    for (int i = 0; i < g_numWaypoints; i++)
                    {
                        if (IsNodeReachable(GetEntityOrigin(g_hostEntity), m_paths[i]->origin))
                        {
                            distance = (m_paths[i]->origin - GetEntityOrigin(g_hostEntity)).GetLength();

                            if (distance < nearestDistance)
                                nearestDistance = distance;
                        }
                    }

                    if (nearestDistance >= newWaypointDistance)
                    {
                        Add(0);
                        SetRadius(m_sautoRadius);
                    }
                }

                m_fallPosition = GetEntityOrigin(g_hostEntity);
                m_learnJumpWaypoint = true;
            }
            else if (m_timeGetProTarGet != 0.0f)
                m_learnJumpWaypoint = false;
            else
                m_fallPoint = true;
        }
    }

    // check if it's a autowaypoint mode enabled
    if (g_autoWaypoint && (g_hostEntity->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)))
    {
        // find the distance from the last used waypoint
        float distance = (m_lastWaypoint - GetEntityOrigin(g_hostEntity)).GetLength();

        if (distance > 128)
        {
            // check that no other reachable waypoints are nearby...
            for (int i = 0; i < g_numWaypoints; i++)
            {
                if (IsNodeReachable(GetEntityOrigin(g_hostEntity), m_paths[i]->origin))
                {
                    distance = (m_paths[i]->origin - GetEntityOrigin(g_hostEntity)).GetLength();

                    if (distance < nearestDistance)
                        nearestDistance = distance;
                }
            }

            // make sure nearest waypoint is far enough away...
            if (nearestDistance >= 128)
                Add(0);  // place a waypoint here
        }
    }

    ShowWaypointMsg();
}

void Waypoint::ShowWaypointMsg(void)
{
    if (FNullEnt(g_hostEntity))
        return;

    m_facingAtIndex = GetFacingIndex();

    // reset the minimal distance changed before
    float nearestDistance = FLT_MAX;
    int nearestIndex = -1;

    // now iterate through all waypoints in a map, and draw required ones
    for (int i = 0; i < g_numWaypoints; i++)
    {
        float distance = (m_paths[i]->origin - GetEntityOrigin(g_hostEntity)).GetLength();

        // check if waypoint is whitin a distance, and is visible
        if (distance <= 768 && ((::IsVisible(m_paths[i]->origin, g_hostEntity) && IsInViewCone(m_paths[i]->origin, g_hostEntity)) || !IsAlive(g_hostEntity) || distance <= 512))
        {
            // check the distance
            if (distance < nearestDistance)
            {
                nearestIndex = i;
                nearestDistance = distance;
            }

            if (m_waypointDisplayTime[i] + 1.0f < engine->GetTime())
            {
                float nodeHeight = (m_paths[i]->flags & WAYPOINT_CROUCH) ? 36.0f : 72.0f; // check the node height
                float nodeHalfHeight = nodeHeight * 0.5f;

                // all waypoints are by default are green
                Color nodeColor = Color(0, 255, 0, 255);

                // colorize all other waypoints
                if (m_paths[i]->flags & WAYPOINT_CAMP)
                    nodeColor = Color(0, 255, 255, 255);
                else if (m_paths[i]->flags & WAYPOINT_GOAL)
                    nodeColor = Color(128, 0, 255, 255);
                else if (m_paths[i]->flags & WAYPOINT_LADDER)
                    nodeColor = Color(128, 64, 0, 255);
                else if (m_paths[i]->flags & WAYPOINT_RESCUE)
                    nodeColor = Color(255, 255, 255, 255);
                else if (m_paths[i]->flags & WAYPOINT_AVOID)
                    nodeColor = Color(255, 0, 0, 255);
                else if (m_paths[i]->flags & WAYPOINT_FALLCHECK)
                    nodeColor = Color(128, 128, 128, 255);
                else if (m_paths[i]->flags & WAYPOINT_USEBUTTON)
                    nodeColor = Color(0, 0, 255, 255);
                else if (m_paths[i]->flags & WAYPOINT_ZMHMCAMP)
                    nodeColor = Color(199, 69, 209, 255);

                // colorize additional flags
                Color nodeFlagColor = Color(-1, -1, -1, 0);

                // check the colors
                if (m_paths[i]->flags & WAYPOINT_SNIPER)
                    nodeFlagColor = Color(130, 87, 0, 255);
                else if (m_paths[i]->flags & WAYPOINT_TERRORIST)
                    nodeFlagColor = Color(255, 0, 0, 255);
                else if (m_paths[i]->flags & WAYPOINT_COUNTER)
                    nodeFlagColor = Color(0, 0, 255, 255);
                else if (m_paths[i]->flags & WAYPOINT_ZMHMCAMP)
                    nodeFlagColor = Color(0, 0, 255, 255);

                nodeColor.alpha = 255;
                nodeFlagColor.alpha = 255;

                // draw node without additional flags
                if (nodeFlagColor.red == -1)
                    engine->DrawLine(g_hostEntity, m_paths[i]->origin - Vector(0.0f, 0.0f, nodeHalfHeight), m_paths[i]->origin + Vector(0.0f, 0.0f, nodeHalfHeight), nodeColor, 7, 0, 0, 10);
                else // draw node with flags
                {
                    engine->DrawLine(g_hostEntity, m_paths[i]->origin - Vector(0.0f, 0.0f, nodeHalfHeight), m_paths[i]->origin - Vector(0.0f, 0.0f, nodeHalfHeight - nodeHeight * 0.75f), nodeColor, 6, 0, 0, 10); // draw basic path
                    engine->DrawLine(g_hostEntity, m_paths[i]->origin - Vector(0.0f, 0.0f, nodeHalfHeight - nodeHeight * 0.75f), m_paths[i]->origin + Vector(0.0f, 0.0f, nodeHalfHeight), nodeFlagColor, 6, 0, 0, 10); // draw additional path
                }

                m_waypointDisplayTime[i] = engine->GetTime();
            }
        }
    }

    if (nearestIndex == -1)
        return;

    // draw arrow to a some importaint waypoints
    if (IsValidWaypoint(m_findWPIndex) || IsValidWaypoint(m_cacheWaypointIndex) || IsValidWaypoint(m_facingAtIndex))
    {
        // check for drawing code
        if (m_arrowDisplayTime + 0.5 < engine->GetTime())
        {
            // finding waypoint - pink arrow
            if (IsValidWaypoint(m_findWPIndex))
                engine->DrawLine(g_hostEntity, m_paths[m_findWPIndex]->origin, GetEntityOrigin(g_hostEntity), Color(128, 0, 128, 255), 10, 0, 0, 5, LINE_ARROW);

            // cached waypoint - yellow arrow
            if (IsValidWaypoint(m_cacheWaypointIndex))
                engine->DrawLine(g_hostEntity, m_paths[m_cacheWaypointIndex]->origin, GetEntityOrigin(g_hostEntity), Color(255, 255, 0, 255), 10, 0, 0, 5, LINE_ARROW);

            // waypoint user facing at - white arrow
            if (IsValidWaypoint(m_facingAtIndex))
                engine->DrawLine(g_hostEntity, m_paths[m_facingAtIndex]->origin, GetEntityOrigin(g_hostEntity), Color(255, 255, 255, 255), 10, 0, 0, 5, LINE_ARROW);

            m_arrowDisplayTime = engine->GetTime();
        }
    }

    // create path pointer for faster access
    Path* path = m_paths[nearestIndex];

    // draw a paths, camplines and danger directions for nearest waypoint
    if (nearestDistance <= 2048 && m_pathDisplayTime <= engine->GetTime())
    {
        m_pathDisplayTime = engine->GetTime() + 1.0f;

        // draw the camplines
        if (path->flags & WAYPOINT_SNIPER)
        {
            const Vector& src = path->origin + Vector(0, 0, (path->flags & WAYPOINT_CROUCH) ? 18.0f : 36.0f); // check if it's a source

            // draw it now
            engine->DrawLine(g_hostEntity, src, Vector(path->campStartX, path->campStartY, src.z), Color(255, 0, 0, 255), 10, 0, 0, 10);
            engine->DrawLine(g_hostEntity, src, Vector(path->campEndX, path->campEndY, src.z), Color(255, 0, 0, 255), 10, 0, 0, 10);
        }

        // draw the connections
        for (int i = 0; i < Const_MaxPathIndex; i++)
        {
            if (path->index[i] == -1)
                continue;

            // jump connection
            if (path->connectionFlags[i] & PATHFLAG_JUMP)
                engine->DrawLine(g_hostEntity, path->origin, m_paths[path->index[i]]->origin, Color(255, 0, 0, 255), 5, 0, 0, 10);
            // boosting friend connection
            else if (path->connectionFlags[i] & PATHFLAG_DOUBLE)
                engine->DrawLine(g_hostEntity, path->origin, m_paths[path->index[i]]->origin, Color(0, 0, 255, 255), 5, 0, 0, 10);
            else if (IsConnected(path->index[i], nearestIndex)) // twoway connection
                engine->DrawLine(g_hostEntity, path->origin, m_paths[path->index[i]]->origin, Color(255, 255, 0, 255), 5, 0, 0, 10);
            else // oneway connection
                engine->DrawLine(g_hostEntity, path->origin, m_paths[path->index[i]]->origin, Color(250, 250, 250, 255), 5, 0, 0, 10);
        }

        // now look for oneway incoming connections
        for (int i = 0; i < g_numWaypoints; i++)
        {
            if (IsConnected(m_paths[i]->pathNumber, path->pathNumber) && !IsConnected(path->pathNumber, m_paths[i]->pathNumber))
                engine->DrawLine(g_hostEntity, path->origin, m_paths[i]->origin, Color(0, 192, 96, 255), 5, 0, 0, 10);
        }

        // draw the radius circle
        Vector origin = (path->flags & WAYPOINT_CROUCH) ? path->origin : path->origin - Vector(0, 0, 18);

        // if radius is nonzero, draw a square
        if (path->radius > 0.0f)
        {
            const float root = path->radius;
            const Color& def = Color(0, 0, 255, 255);

            engine->DrawLine(g_hostEntity, origin + Vector(root, root, 0), origin + Vector(-root, root, 0), def, 5, 0, 0, 10);
            engine->DrawLine(g_hostEntity, origin + Vector(root, root, 0), origin + Vector(root, -root, 0), def, 5, 0, 0, 10);
            engine->DrawLine(g_hostEntity, origin + Vector(-root, -root, 0), origin + Vector(root, -root, 0), def, 5, 0, 0, 10);
            engine->DrawLine(g_hostEntity, origin + Vector(-root, -root, 0), origin + Vector(-root, root, 0), def, 5, 0, 0, 10);
        }
        else
        {
            const float root = 5.0f;
            const Color& def = Color(0, 0, 255, 255);

            engine->DrawLine(g_hostEntity, origin + Vector(root, -root, 0), origin + Vector(-root, root, 0), def, 5, 0, 0, 10);
            engine->DrawLine(g_hostEntity, origin + Vector(-root, -root, 0), origin + Vector(root, root, 0), def, 5, 0, 0, 10);
        }

        g_exp.DrawLines(nearestIndex, path);

        // display some information
        char tempMessage[4096];

        // show the information about that point
        int length = sprintf(tempMessage, "\n\n\n\n    Waypoint Information:\n\n"
            "      Waypoint %d of %d, Radius: %.1f\n"
            "      Flags: %s\n\n", nearestIndex, g_numWaypoints, path->radius, GetWaypointInfo(nearestIndex));


        g_exp.DrawText(nearestIndex, tempMessage, length);

        // check if we need to show the cached point index
        if (m_cacheWaypointIndex != -1)
        {
            length += sprintf(&tempMessage[length], "\n    Cached Waypoint Information:\n\n"
                "      Waypoint %d of %d, Radius: %.1f\n"
                "      Flags: %s\n", m_cacheWaypointIndex, g_numWaypoints, m_paths[m_cacheWaypointIndex]->radius, GetWaypointInfo(m_cacheWaypointIndex));
        }

        // check if we need to show the facing point index
        if (m_facingAtIndex != -1)
        {
            length += sprintf(&tempMessage[length], "\n    Facing Waypoint Information:\n\n"
                "      Waypoint %d of %d, Radius: %.1f\n"
                "      Flags: %s\n", m_facingAtIndex, g_numWaypoints, m_paths[m_facingAtIndex]->radius, GetWaypointInfo(m_facingAtIndex));
        }

        if (g_sgdWaypoint)
        {
            length += sprintf(&tempMessage[length], "    Hold 'E' Call [SgdWP] Menu \n"
                "    [Auto Put Waypoint]:%s \n", g_sautoWaypoint ? "on" : "off");

            if (!g_sautoWaypoint)
                length += sprintf(&tempMessage[length], "    You Can true on [Auto put Waypoint] (menu>7) \n");
            else
            {
                length += sprintf(&tempMessage[length], "    System will auto save Waypoint, you can move in the map now \n"
                    "    Complete, you will save Waypoint (menu>9) \n\n"
                    "    Hold 'IN_DUCK' Can make camp Waypoint \n"
                    "    System Can auto save 'Fall' and 'Jump' Waypoint \n\n");
            }
        }

        // draw entire message
        MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, null, g_hostEntity);
        WRITE_BYTE(TE_TEXTMESSAGE);
        WRITE_BYTE(4); // channel
        WRITE_SHORT(FixedSigned16(0, 1 << 13)); // x
        WRITE_SHORT(FixedSigned16(0, 1 << 13)); // y
        WRITE_BYTE(0); // effect
        WRITE_BYTE(255); // r1
        WRITE_BYTE(255); // g1
        WRITE_BYTE(255); // b1
        WRITE_BYTE(1); // a1
        WRITE_BYTE(255); // r2
        WRITE_BYTE(255); // g2
        WRITE_BYTE(255); // b2
        WRITE_BYTE(255); // a2
        WRITE_SHORT(0); // fadeintime
        WRITE_SHORT(0); // fadeouttime
        WRITE_SHORT(FixedUnsigned16(1.1f, 1 << 8)); // holdtime
        WRITE_STRING(tempMessage);
        MESSAGE_END();
    }
}

bool Waypoint::IsConnected(int index)
{
    for (int i = 0; i < g_numWaypoints; i++)
    {
        if (i != index)
        {
            for (int j = 0; j < Const_MaxPathIndex; j++)
            {
                if (m_paths[i]->index[j] == index)
                    return true;
            }
        }
    }

    return false;
}

bool Waypoint::NodesValid(void)
{
    int terrPoints = 0;
    int ctPoints = 0;
    int goalPoints = 0;
    int rescuePoints = 0;
    int connections;
    int i, j;

    // SyPB Pro P.41 - SgdWP fixed and msg improve
    bool haveError = false;

    for (i = 0; i < g_numWaypoints; i++)
    {
        connections = 0;

        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (m_paths[i]->index[j] != -1)
            {
                if (m_paths[i]->index[j] > g_numWaypoints)
                {
                    AddLogEntry(LOG_WARNING, "Waypoint %d connected with invalid Waypoint #%d!", i, m_paths[i]->index[j]);
                    (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i]->origin);
                    haveError = true;
                    if (g_sgdWaypoint)
                        ChartPrint("[SgdWP] Waypoint %d connected with invalid Waypoint #%d!", i, m_paths[i]->index[j]);
                }

                connections++;
                break;
            }
        }

        if (connections == 0)
        {
            if (!IsConnected(i))
            {
                AddLogEntry(LOG_WARNING, "Waypoint %d isn't connected with any other Waypoint!", i);
                (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i]->origin);
                haveError = true;
                if (g_sgdWaypoint)
                    ChartPrint("[SgdWP] Waypoint %d isn't connected with any other Waypoint!", i);
            }
        }

        if (m_paths[i]->pathNumber != i)
        {
            AddLogEntry(LOG_WARNING, "Waypoint %d pathnumber differs from index!", i);
            (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i]->origin);
            haveError = true;
            if (g_sgdWaypoint)
                ChartPrint("[SgdWP] Waypoint %d pathnumber differs from index!", i);
        }

        if (m_paths[i]->flags & WAYPOINT_CAMP)
        {
            if (m_paths[i]->campEndX == 0 && m_paths[i]->campEndY == 0)
            { // try fix camp waypoints
                m_paths[i]->campEndX = m_paths[i]->campStartX;
                m_paths[i]->campEndY = m_paths[i]->campStartY;
            }
        }
        else if (m_paths[i]->flags & WAYPOINT_TERRORIST)
            terrPoints++;
        else if (m_paths[i]->flags & WAYPOINT_COUNTER)
            ctPoints++;
        else if (m_paths[i]->flags & WAYPOINT_GOAL)
            goalPoints++;
        else if (m_paths[i]->flags & WAYPOINT_RESCUE)
            rescuePoints++;

        for (int k = 0; k < Const_MaxPathIndex; k++)
        {
            if (m_paths[i]->index[k] != -1)
            {
                if (m_paths[i]->index[k] >= g_numWaypoints || m_paths[i]->index[k] < -1)
                {
                    AddLogEntry(LOG_WARNING, "Waypoint %d - Pathindex %d out of Range!", i, k);
                    (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i]->origin);

                    g_waypointOn = true;
                    g_editNoclip = true;

                    haveError = true;
                    if (g_sgdWaypoint)
                        ChartPrint("[SgdWP] Waypoint %d - Pathindex %d out of Range!", i, k);
                }
                else if (m_paths[i]->index[k] == i)
                {
                    AddLogEntry(LOG_WARNING, "Waypoint %d - Pathindex %d points to itself!", i, k);
                    (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i]->origin);

                    g_waypointOn = true;
                    g_editNoclip = true;

                    haveError = true;
                    if (g_sgdWaypoint)
                        ChartPrint("[SgdWP] Waypoint %d - Pathindex %d points to itself!", i, k);
                }
            }
        }
    }

    if (g_mapType & MAP_CS && GetGameMod() == MODE_BASE)
    {
        if (rescuePoints == 0)
        {
            AddLogEntry(LOG_WARNING, "You didn't set a Rescue Point!");
            haveError = true;
            if (g_sgdWaypoint)
                ChartPrint("[SgdWP] You didn't set a Rescue Point!");
        }
    }

    if (terrPoints == 0 && GetGameMod() == MODE_BASE)
    {
        AddLogEntry(LOG_WARNING, "You didn't set any Terrorist Important Point!");
        haveError = true;
        if (g_sgdWaypoint)
            ChartPrint("[SgdWP] You didn't set any Terrorist Important Point!");
    }
    else if (ctPoints == 0 && GetGameMod() == MODE_BASE)
    {
        AddLogEntry(LOG_WARNING, "You didn't set any CT Important Point!");
        haveError = true;
        if (g_sgdWaypoint)
            ChartPrint("[SgdWP] You didn't set any CT Important Point!");
    }
    else if (goalPoints == 0 && GetGameMod() == MODE_BASE)
    {
        AddLogEntry(LOG_WARNING, "You didn't set any Goal Point!");
        haveError = true;
        if (g_sgdWaypoint)
            ChartPrint("[SgdWP] You didn't set any Goal Point!");
    }

    CenterPrint("Waypoints are saved!");

    return haveError ? false : true;
}

void Waypoint::InitPathMatrix(void)
{
    int i, j, k;

    if (m_distMatrix != null)
        delete[](m_distMatrix);

    if (m_pathMatrix != null)
        delete[] m_pathMatrix;

    m_distMatrix = null;
    m_pathMatrix = null;

    m_distMatrix = new int[g_numWaypoints * g_numWaypoints];
    m_pathMatrix = new int[g_numWaypoints * g_numWaypoints];

    if (m_distMatrix == null || m_pathMatrix == null)
        return;

    if (LoadPathMatrix())
        return; // matrix loaded from file

    for (i = 0; i < g_numWaypoints; i++)
    {
        for (j = 0; j < g_numWaypoints; j++)
        {
            *(m_distMatrix + i * g_numWaypoints + j) = 999999;
            *(m_pathMatrix + i * g_numWaypoints + j) = -1;
        }
    }

    for (i = 0; i < g_numWaypoints; i++)
    {
        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (m_paths[i]->index[j] >= 0 && m_paths[i]->index[j] < g_numWaypoints)
            {
                *(m_distMatrix + (i * g_numWaypoints) + m_paths[i]->index[j]) = m_paths[i]->distances[j];
                *(m_pathMatrix + (i * g_numWaypoints) + m_paths[i]->index[j]) = m_paths[i]->index[j];
            }
        }
    }

    for (i = 0; i < g_numWaypoints; i++)
        *(m_distMatrix + (i * g_numWaypoints) + i) = 0;

    for (k = 0; k < g_numWaypoints; k++)
    {
        for (i = 0; i < g_numWaypoints; i++)
        {
            for (j = 0; j < g_numWaypoints; j++)
            {
                if (*(m_distMatrix + (i * g_numWaypoints) + k) + *(m_distMatrix + (k * g_numWaypoints) + j) < (*(m_distMatrix + (i * g_numWaypoints) + j)))
                {
                    *(m_distMatrix + (i * g_numWaypoints) + j) = *(m_distMatrix + (i * g_numWaypoints) + k) + *(m_distMatrix + (k * g_numWaypoints) + j);
                    *(m_pathMatrix + (i * g_numWaypoints) + j) = *(m_pathMatrix + (i * g_numWaypoints) + k);
                }
            }
        }
    }

    // save path matrix to file for faster access
    SavePathMatrix();
}

void Waypoint::SavePathMatrix(void)
{
    File fp(FormatBuffer("%sdata/%s.pmt", GetWaypointDir(), GetMapName()), "wb");

    // unable to open file
    if (!fp.IsValid())
    {
        AddLogEntry(LOG_FATAL, "Failed to open file for writing, waypoint/data folder isn't exits?");
        return;
    }

    // write number of waypoints
    fp.Write(&g_numWaypoints, sizeof(int));

    // write path & distance matrix
    fp.Write(m_pathMatrix, sizeof(int), g_numWaypoints * g_numWaypoints);
    fp.Write(m_distMatrix, sizeof(int), g_numWaypoints * g_numWaypoints);

    // and close the file
    fp.Close();
}

bool Waypoint::LoadPathMatrix(void)
{
    File fp(FormatBuffer("%sdata/%s.pmt", GetWaypointDir(), GetMapName()), "rb");

    // file doesn't exists return false
    if (!fp.IsValid())
        return false;

    int num = 0;

    // read number of waypoints
    fp.Read(&num, sizeof(int));

    if (num != g_numWaypoints)
    {
        AddLogEntry(LOG_DEFAULT, "Wrong number of points (pmt:%d/cur:%d). Matrix will be rebuilded/n",
            "********* If you use new waypoint, you need get the new .pmt file"
            , num, g_numWaypoints);
        fp.Close();

        // SyPB Pro P.37 - del pmt file (this will auto make new file)
        unlink(FormatBuffer("%sdata/%s.pmt", GetWaypointDir(), GetMapName()));

        return false;
    }

    // read path & distance matrixes
    fp.Read(m_pathMatrix, sizeof(int), g_numWaypoints * g_numWaypoints);
    fp.Read(m_distMatrix, sizeof(int), g_numWaypoints * g_numWaypoints);

    // and close the file
    fp.Close();

    return true;
}

int Waypoint::GetPathDistance(int srcIndex, int destIndex)
{
    if (!IsValidWaypoint(srcIndex) || !IsValidWaypoint(destIndex))
        return 1;

    return *(m_distMatrix + (srcIndex * g_numWaypoints) + destIndex);
}

float Waypoint::GetPathDistanceFloat(int srcIndex, int destIndex)
{
    return static_cast <float> (GetPathDistance(srcIndex, destIndex));
}

void Waypoint::SetGoalVisited(int index)
{
    if (!IsValidWaypoint(index))
        return;

    if (!IsGoalVisited(index) && (m_paths[index]->flags & WAYPOINT_GOAL))
    {
        int bombPoint = FindNearest(GetBombPosition());

        if (IsValidWaypoint(bombPoint) && bombPoint != index)
            m_visitedGoals.Push(index);
    }
}

bool Waypoint::IsGoalVisited(int index)
{
    ITERATE_ARRAY(m_visitedGoals, i)
    {
        if (m_visitedGoals[i] == index)
            return true;
    }

    return false;
}

void Waypoint::CreateBasic(void)
{
    // this function creates basic waypoint types on map

    edict_t* ent = null;

    // first of all, if map contains ladder points, create it
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_ladder")))
    {
        Vector ladderLeft = ent->v.absmin;
        Vector ladderRight = ent->v.absmax;
        ladderLeft.z = ladderRight.z;

        TraceResult tr;
        Vector up, down, front, back;

        Vector diff = ((ladderLeft - ladderRight) ^ Vector(0.0f, 0.0f, 0.0f)).Normalize() * 15.0f;
        front = back = GetEntityOrigin(ent);

        front = front + diff; // front
        back = back - diff; // back

        up = down = front;
        down.z = ent->v.absmax.z;

        TraceHull(down, up, true, point_hull, null, &tr);

        if (POINT_CONTENTS(up) == CONTENTS_SOLID || tr.flFraction != 1.0f)
        {
            up = down = back;
            down.z = ent->v.absmax.z;
        }

        TraceHull(down, up - Vector(0.0f, 0.0f, 1000.0f), true, point_hull, null, &tr);
        up = tr.vecEndPos;

        Vector pointOrigin = up + Vector(0.0f, 0.0f, 39.0f);
        m_isOnLadder = true;

        do
        {
            if (FindNearest(pointOrigin, 50.0f) == -1)
                Add(-1, pointOrigin);

            pointOrigin.z += 160.0f;
        } while (pointOrigin.z < down.z - 40.0f);

        pointOrigin = down + Vector(0.0f, 0.0f, 38.0f);

        if (FindNearest(pointOrigin, 50.0f) == -1)
            Add(-1, pointOrigin);

        m_isOnLadder = false;
    }

    // then terrortist spawnpoints
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_player_deathmatch")))
    {
        Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);

        if (FindNearest(origin, 50.0f) == -1)
            Add(0, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // then add ct spawnpoints
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_player_start")))
    {
        Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);

        if (FindNearest(origin, 50.0f) == -1)
            Add(0, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // then vip spawnpoint
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_vip_start")))
    {
        Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);

        if (FindNearest(origin, 50.0f) == -1)
            Add(0, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // hostage rescue zone
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_hostage_rescue")))
    {
        Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);

        if (FindNearest(origin, 50.0f) == -1)
            Add(4, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // hostage rescue zone (same as above)
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_hostage_rescue")))
    {
        Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);

        if (FindNearest(origin, 50.0f) == -1)
            Add(4, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // bombspot zone
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_bomb_target")))
    {
        Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);

        if (FindNearest(origin, 50.0f) == -1)
            Add(100, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // bombspot zone (same as above)
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_bomb_target")))
    {
        Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);

        if (FindNearest(origin, 50.0f) == -1)
            Add(100, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // hostage entities
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "hostage_entity")))
    {
        // if already saved || moving skip it
        if ((ent->v.effects & EF_NODRAW) && (ent->v.speed > 0))
            continue;

        Vector origin = GetEntityOrigin(ent);

        if (FindNearest(origin, 250.0f) == -1 && g_analyzewaypoints == true)
            Add(2, Vector(origin.x, origin.y, (origin.z + 36.0f))); // goal waypoints will be added by analyzer
        else if (FindNearest(origin, 50.0f) == -1)
            Add(100, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // vip rescue (safety) zone
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_vip_safetyzone")))
    {
        Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);

        if (FindNearest(origin, 50.0f) == -1)
            Add(100, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // terrorist escape zone
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_escapezone")))
    {
        Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);

        if (FindNearest(origin, 50.0f) == -1)
            Add(100, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // weapons on the map?
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "armoury_entity")))
    {
        Vector origin = GetEntityOrigin(ent);

        if (FindNearest(origin, 50.0f) == -1)
            Add(0, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }
}

Path* Waypoint::GetPath(int id)
{
    Path* path = m_paths[id];

    if (path == null)
        return null;

    return path;
}

void Waypoint::EraseFromHardDisk(void)
{
    // this function removes waypoint file from the hard disk

    String deleteList[5];

    // if we're delete waypoint, delete all corresponding to it files
    deleteList[0] = FormatBuffer("%s%s.pwf", GetWaypointDir(), GetMapName()); // waypoint itself
    deleteList[1] = FormatBuffer("%sdata/%s.exp", GetWaypointDir(), GetMapName()); // corresponding to waypoint experience
    deleteList[2] = FormatBuffer("%sdata/%s.vis", GetWaypointDir(), GetMapName()); // corresponding to waypoint vistable
    deleteList[3] = FormatBuffer("%sdata/%s.pmt", GetWaypointDir(), GetMapName()); // corresponding to waypoint path matrix
    deleteList[4] = FormatBuffer("%sdata/%s.xml", GetWaypointDir(), GetMapName()); // corresponding to waypoint xml database

    for (int i = 0; i < 4; i++)
    {
        if (TryFileOpen(deleteList[i]))
        {
            unlink(deleteList[i]);
            AddLogEntry(LOG_DEFAULT, "File %s, has been deleted from the disk", &deleteList[i][0]);
        }
        else
            AddLogEntry(LOG_ERROR, "Unable to open %s", &deleteList[i][0]);
    }

    Initialize(); // reintialize points
}

void Waypoint::SetBombPosition(bool shouldReset)
{
    // this function stores the bomb position as a vector

    if (shouldReset)
    {
        m_foundBombOrigin = nullvec;
        g_bombPlanted = false;

        return;
    }

    edict_t* ent = null;

    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "grenade")))
    {
        if (strcmp(STRING(ent->v.model) + 9, "c4.mdl") == 0)
        {
            m_foundBombOrigin = GetEntityOrigin(ent);
            break;
        }
    }
}

// SyPB Pro P.30 - SgdWP
void Waypoint::SetLearnJumpWaypoint(int mod)
{
    // SyPB Pro P.32 - SgdWP
    if (mod == -1)
        m_learnJumpWaypoint = (m_learnJumpWaypoint ? false : true);
    else
        m_learnJumpWaypoint = (mod == 1 ? true : false);
}

void Waypoint::SetFindIndex(int index)
{
    if (IsValidWaypoint(index))
    {
        m_findWPIndex = index;
        ServerPrint("Showing Direction to Waypoint #%d", m_findWPIndex);
    }
}

int Waypoint::AddGoalScore(int index, int other[4])
{
    Array <int> left;

    if (m_goalsScore[index] < 1024.0f)
        left.Push(index);

    for (int i = 0; i < 3; i++)
    {
        if (m_goalsScore[other[i]] < 1024.0f)
            left.Push(other[i]);
    }

    if (left.IsEmpty())
        index = other[engine->RandomInt(0, 3)];
    else
        index = left.GetRandomElement();

    if (m_paths[index]->flags & WAYPOINT_GOAL)
        m_goalsScore[index] += 384.0f;
    else if (m_paths[index]->flags & (WAYPOINT_COUNTER | WAYPOINT_TERRORIST))
        m_goalsScore[index] += 768.0f;
    else if (m_paths[index]->flags & WAYPOINT_CAMP)
        m_goalsScore[index] += 1024.0f;

    return index;
}

void Waypoint::ClearGoalScore(void)
{
    // iterate though all waypoints
    for (int i = 0; i < Const_MaxWaypoints; i++)
        m_goalsScore[i] = 0.0f;
}

Waypoint::Waypoint(void)
{
    m_waypointPaths = false;
    m_endJumpPoint = false;
    m_redoneVisibility = false;
    m_learnJumpWaypoint = false;
    m_timeJumpStarted = 0.0f;

    m_learnVelocity = nullvec;
    m_learnPosition = nullvec;
    m_lastJumpWaypoint = -1;
    m_cacheWaypointIndex = -1;
    m_findWPIndex = -1;
    m_visibilityIndex = 0;

    m_lastDeclineWaypoint = -1;

    m_lastWaypoint = nullvec;
    m_isOnLadder = false;

    m_pathDisplayTime = 0.0f;
    m_arrowDisplayTime = 0.0f;

    m_terrorPoints.RemoveAll();
    m_ctPoints.RemoveAll();
    m_goalPoints.RemoveAll();
    m_campPoints.RemoveAll();
    m_rescuePoints.RemoveAll();
    m_sniperPoints.RemoveAll();
    m_otherPoints.RemoveAll();

    m_distMatrix = null;
    m_pathMatrix = null;
}

Waypoint::~Waypoint(void)
{
    if (m_distMatrix != null)
        delete[] m_distMatrix;

    if (m_pathMatrix != null)
        delete[] m_pathMatrix;

    m_distMatrix = null;
    m_pathMatrix = null;
}
