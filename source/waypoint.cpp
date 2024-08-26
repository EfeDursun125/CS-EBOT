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

#ifdef PLATFORM_LINUX
#include <cstdlib>
#endif

ConVar ebot_analyze_distance("ebot_analyze_distance", "40");
ConVar ebot_analyze_disable_fall_connections("ebot_analyze_disable_fall_connections", "0");
ConVar ebot_analyze_wall_check_distance("ebot_analyze_wall_check_distance", "24");
ConVar ebot_analyze_max_jump_height("ebot_analyze_max_jump_height", "62");
ConVar ebot_analyze_goal_check_distance("ebot_analyze_goal_check_distance", "200");
ConVar ebot_analyze_create_camp_waypoints("ebot_analyze_create_camp_waypoints", "1");
ConVar ebot_analyzer_min_fps("ebot_analyzer_min_fps", "30.0");
ConVar ebot_analyze_auto_start("ebot_analyze_auto_start", "1");
ConVar ebot_download_waypoints("ebot_download_waypoints", "0");
ConVar ebot_download_waypoints_from("ebot_download_waypoints_from", "");
ConVar ebot_waypoint_size("ebot_waypoint_size", "7");
ConVar ebot_waypoint_r("ebot_waypoint_r", "0");
ConVar ebot_waypoint_g("ebot_waypoint_g", "255");
ConVar ebot_waypoint_b("ebot_waypoint_b", "0");

// this function initialize the waypoint structures..
void Waypoint::Initialize(void)
{
    m_paths.Destroy();
    g_numWaypoints = 0;
    m_lastWaypoint = nullvec;
}

void SetFlags(const char* className, const int index, const int flag, bool checkEffect = false)
{
    Path* pointer = g_waypoint->GetPath(index);
    edict_t* ent = nullptr;
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, className)))
    {
        if (checkEffect && ((ent->v.effects & EF_NODRAW) || ent->v.speed > 0.0f))
            continue;

        if (!Math::BBoxIntersects(ent->v.absmin, pointer->origin, ent->v.absmax, pointer->origin))
            continue;

        pointer->flags |= flag;
    }
}

void SetGoals(void)
{
    int i;
    for (i = 0; i < g_numWaypoints; i++)
    {
        if (g_mapType == MAP_DE)
        {
            SetFlags("func_bomb_target", i, WAYPOINT_GOAL);
            SetFlags("info_bomb_target", i, WAYPOINT_GOAL);
        }
        else if (g_mapType == MAP_CS)
        {
            SetFlags("monster_scientist", i, WAYPOINT_GOAL, true);
            SetFlags("hostage_entity", i, WAYPOINT_GOAL, true);
            SetFlags("info_hostage_rescue", i, WAYPOINT_RESCUE);
            SetFlags("func_hostage_rescue", i, WAYPOINT_RESCUE);
        }
        else if (g_mapType == MAP_AS)
        {
            SetFlags("func_escapezone", i, WAYPOINT_GOAL);
            SetFlags("func_vip_safetyzone", i, WAYPOINT_GOAL);
        }
    }
}

bool CheckCrouchRequirement(const Vector& TargetPosition)
{
    TraceResult upcheck{};
    const Vector TargetPosition2 = Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z + 36.0f));
    TraceHull(TargetPosition, TargetPosition2, true, point_hull, g_hostEntity, &upcheck);
    return upcheck.flFraction != 1.0f;
}

void CreateWaypoint(Vector& Next, float range)
{
    Next.z += 19.0f;
    TraceResult tr{};
    TraceHull(Next, Next, NO_BOTH, HULL_HEAD, g_hostEntity, &tr);
    Next.z -= 19.0f;

    range *= 0.75f;
    if (tr.flFraction != 1.0f && !IsBreakable(tr.pHit))
        return;

    const int startindex = g_waypoint->FindNearestInCircle(tr.vecEndPos, range);
    if (IsValidWaypoint(startindex))
        return;

    TraceResult tr2{};
    TraceHull(tr.vecEndPos, Vector(tr.vecEndPos.x, tr.vecEndPos.y, (tr.vecEndPos.z - 800.0f)), NO_BOTH, HULL_HEAD, g_hostEntity, &tr2);

    if (tr2.flFraction == 1.0f)
        return;

    const bool isBreakable = IsBreakable(tr.pHit);
    Vector TargetPosition = tr2.vecEndPos;
    TargetPosition.z = TargetPosition.z + 19.0f;

    const int endindex = g_waypoint->FindNearestInCircle(TargetPosition, range);
    if (IsValidWaypoint(endindex))
        return;

    const Vector targetOrigin = g_waypoint->GetPath(g_waypoint->FindNearestInCircle(TargetPosition, 256.0f))->origin;
    g_analyzeputrequirescrouch = CheckCrouchRequirement(TargetPosition);
    if (g_waypoint->IsNodeReachable(targetOrigin, TargetPosition))
        g_waypoint->Add(isBreakable ? 1 : -1, g_analyzeputrequirescrouch ? Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z - 18.0f)) : TargetPosition);
}

void AnalyzeThread(void)
{
    static bool* expanded{};
    static float magicTimer{};
    if (!FNullEnt(g_hostEntity))
    {
        char message[] =
            "+-----------------------------------------------+\n"
            "| Analyzing the map for walkable places |\n"
            "+-----------------------------------------------+\n";

        HudMessage(g_hostEntity, true, Color(255, 255, 255, 255), message);
    }
    else if (!IsDedicatedServer())
        return;

    int16_t i;

    // guarantee to have it
    if (!expanded)
    {
        safeloc(expanded, Const_MaxWaypoints);
        for (i = 0; i < Const_MaxWaypoints; i++)
            expanded[i] = false;
    }

    float range;
    Vector WayVec, Next;
    int8_t dir;
    for (i = 0; i < g_numWaypoints; i++)
    {
        if (expanded[i])
            continue;

        if (magicTimer > engine->GetTime())
            return;

        if ((ebot_analyzer_min_fps.GetFloat() + g_pGlobals->frametime) < 1.0f / g_pGlobals->frametime)
            magicTimer = engine->GetTime() + g_pGlobals->frametime * 0.066f;

        WayVec = g_waypoint->GetPath(i)->origin;
        range = ebot_analyze_distance.GetFloat();
        for (dir = 1; dir < 8; dir++)
        {
            switch (dir)
            {
            case 1:
            {
                Next.x = WayVec.x + range;
                Next.y = WayVec.y;
                Next.z = WayVec.z;
                break;
            }
            case 2:
            {
                Next.x = WayVec.x - range;
                Next.y = WayVec.y;
                Next.z = WayVec.z;
                break;
            }
            case 3:
            {
                Next.x = WayVec.x;
                Next.y = WayVec.y + range;
                Next.z = WayVec.z;
                break;
            }
            case 4:
            {
                Next.x = WayVec.x;
                Next.y = WayVec.y - range;
                Next.z = WayVec.z;
                break;
            }
            case 5:
            {
                Next.x = WayVec.x + range;
                Next.y = WayVec.y;
                Next.z = WayVec.z + 128.0f;
                break;
            }
            case 6:
            {
                Next.x = WayVec.x - range;
                Next.y = WayVec.y;
                Next.z = WayVec.z + 128.0f;
                break;
            }
            case 7:
            {
                Next.x = WayVec.x;
                Next.y = WayVec.y + range;
                Next.z = WayVec.z + 128.0f;
                break;
            }
            case 8:
            {
                Next.x = WayVec.x;
                Next.y = WayVec.y - range;
                Next.z = WayVec.z + 128.0f;
                break;
            }
            }

            CreateWaypoint(Next, range);
        }

        expanded[i] = true;
    }

    if (magicTimer + 2.0f < engine->GetTime())
    {
        g_analyzewaypoints = false;
        g_waypointOn = false;
        g_editNoclip = false;
        g_waypoint->AnalyzeDeleteUselessWaypoints();
        SetGoals();
        g_waypoint->Save();
        g_waypoint->Load();
        ServerCommand("exec addons/ebot/ebot.cfg");
        ServerCommand("ebot wp mdl off");
        safedel(expanded);
    }
}

void Waypoint::Analyze(void)
{
    if (!g_numWaypoints)
        return;

    AnalyzeThread();
}

void Waypoint::AnalyzeDeleteUselessWaypoints(void)
{
    int16_t i;
    int8_t connections, j;
    for (i = 0; i < g_numWaypoints; i++)
    {
        connections = 0;

        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (m_paths[i].index[j] != -1)
            {
                if (m_paths[i].index[j] >= g_numWaypoints)
                    DeleteByIndex(i);
                else if (m_paths[i].index[j] == i)
                    DeleteByIndex(i);
                else
                    connections++;
            }
        }

        if (!connections)
            DeleteByIndex(i);
    }

    CenterPrint("Waypoints are saved!");
}

void Waypoint::AddPath(const int addIndex, const int pathIndex, const int type)
{
    if (!IsValidWaypoint(addIndex) || !IsValidWaypoint(pathIndex) || addIndex == pathIndex)
        return;

    Path* path = &m_paths[addIndex];

    // don't allow paths get connected twice
    int i;
    for (i = 0; i < Const_MaxPathIndex; i++)
    {
        if (path->index[i] == pathIndex)
            return;
    }

    // check for free space in the connection indices
    for (i = 0; i < Const_MaxPathIndex; i++)
    {
        if (path->index[i] == -1)
        {
            path->index[i] = static_cast<int16_t>(pathIndex);

            if (type == 1)
            {
                path->connectionFlags[i] |= PATHFLAG_JUMP;
                path->flags |= WAYPOINT_JUMP;
                path->radius = 4;
            }
            else if (type == 2)
            {
                path->connectionFlags[i] |= PATHFLAG_DOUBLE;
                path->flags |= WAYPOINT_DJUMP;
            }

            return;
        }
    }

    // there wasn't any free space. try exchanging it with a long-distance path
    float distance, maxDistance = FLT_MAX;
    int slotID = -1;
    
    for (i = 0; i < Const_MaxPathIndex; i++)
    {
        distance = (path->origin - m_paths[path->index[i]].origin).GetLengthSquared();
        if (distance > maxDistance)
        {
            maxDistance = distance;
            slotID = i;
        }
    }

    if (slotID != -1)
    {
        path->index[slotID] = static_cast<int16_t>(pathIndex);

        if (type == 1)
        {
            path->connectionFlags[slotID] |= PATHFLAG_JUMP;
            path->flags |= WAYPOINT_JUMP;
            path->radius = 4;
        }
        else if (type == 2)
        {
            path->connectionFlags[slotID] |= PATHFLAG_DOUBLE;
            path->flags |= WAYPOINT_DJUMP;
        }
    }
}

int Waypoint::FindFarest(const Vector& origin, const float maxDistance)
{
    int i, index = -1;
    float distance, maxDist = squaredf(maxDistance);
    for (i = 0; i < g_numWaypoints; i++)
    {
        distance = (m_paths[i].origin - origin).GetLengthSquared();
        if (distance > maxDist)
        {
            index = i;
            maxDist = distance;
        }
    }

    return index;
}

int Waypoint::FindNearestInCircle(const Vector& origin, const float maxDistance)
{
    int i, index = -1;
    float distance, maxDist = squaredf(maxDistance);
    for (i = 0; i < g_numWaypoints; i++)
    {
        distance = (m_paths[i].origin - origin).GetLengthSquared();
        if (distance < maxDist)
        {
            index = i;
            maxDist = distance;
        }
    }

    return index;
}

int Waypoint::FindNearest(const Vector& origin, const float minDistance, const int flags, edict_t* entity, int* findWaypointPoint, const int mode)
{
 
    float squaredMinDistance = squaredf(minDistance);
    const int checkPoint = 20;
    float wpDistance[checkPoint];
    int wpIndex[checkPoint];
    float distance;
    Vector dest;

    int i, y, z;
    for (i = 0; i < checkPoint; i++)
    {
        wpIndex[i] = -1;
        wpDistance[i] = FLT_MAX;
    }

    for (i = 0; i < g_numWaypoints; i++)
    {
        if (flags != -1 && !(m_paths[i].flags & flags))
            continue;

        dest = m_paths[i].origin;
        distance = (dest - origin).GetLengthSquared();
        if (distance > squaredMinDistance)
            continue;

        if (((dest.z > origin.z + 62.0f || dest.z < origin.z - 100.0f) && !(m_paths[i].flags & WAYPOINT_LADDER)) && (dest - origin).GetLengthSquared2D() < squaredf(30.0f))
            continue;

        for (y = 0; y < checkPoint; y++)
        {
            if (distance > wpDistance[y])
                continue;

            for (z = checkPoint - 1; z > y; z--)
            {
                if (z == checkPoint - 1 || wpIndex[z] == -1)
                    continue;

                wpIndex[z + 1] = wpIndex[z];
                wpDistance[z + 1] = wpDistance[z];
            }

            wpIndex[y] = i;
            wpDistance[y] = distance;
            y = checkPoint + 5;
        }
    }

    if (IsValidWaypoint(mode))
    {
        int cdWPIndex[checkPoint];
        float cdWPDistance[checkPoint];

        for (i = 0; i < checkPoint; i++)
        {
            cdWPIndex[i] = -1;
            cdWPDistance[i] = FLT_MAX;
        }

        for (i = 0; i < checkPoint; i++)
        {
            if (!IsValidWaypoint(wpIndex[i]))
                continue;

            distance = g_waypoint->GetPathDistance(wpIndex[i], mode);
            for (y = 0; y < checkPoint; y++)
            {
                if (distance > cdWPDistance[y])
                    continue;

                for (z = checkPoint - 1; z >= y; z--)
                {
                    if (z == checkPoint - 1 || cdWPIndex[z] == -1)
                        continue;

                    cdWPIndex[z + 1] = cdWPIndex[z];
                    cdWPDistance[z + 1] = cdWPDistance[z];
                }

                cdWPIndex[y] = wpIndex[i];
                cdWPDistance[y] = distance;
                y = checkPoint + 5;
            }
        }

        for (i = 0; i < checkPoint; i++)
        {
            wpIndex[i] = cdWPIndex[i];
            wpDistance[i] = cdWPDistance[i];
        }
    }

    int firsIndex = -1;
    if (!FNullEnt(entity))
    {
        for (i = 0; i < checkPoint; i++)
        {
            if (!IsValidWaypoint(wpIndex[i]))
                continue;

            // Use the path variable in the condition     
            if (wpDistance[i] > squaredf(m_paths[wpIndex[i]].radius) && !Reachable(entity, wpIndex[i]))
                continue;

            if (findWaypointPoint == reinterpret_cast<int*>(-2))
                return wpIndex[i];

            if (firsIndex == -1)
            {
                firsIndex = wpIndex[i];
                continue;
            }

            if (findWaypointPoint && findWaypointPoint != reinterpret_cast<int*>(-2))
                *findWaypointPoint = wpIndex[i];

            return firsIndex;
        }
    }

    if (!IsValidWaypoint(firsIndex))
        firsIndex = wpIndex[0];

    return firsIndex;
}

// returns all waypoints within radius from position
void Waypoint::FindInRadius(const Vector& origin, const float radius, int* holdTab, int* count)
{
    const int maxCount = *count;
    const float squared = squaredf(radius);
    *count = 0;

    int i;
    for (i = 0; i < g_numWaypoints; i++)
    {
        if ((m_paths[i].origin - origin).GetLengthSquared() < squared)
        {
            *holdTab++ = i;
            *count += 1;

            if (*count >= maxCount)
                break;
        }
    }

    *count -= 1;
}

void Waypoint::FindInRadius(MiniArray <int16_t>& queueID, const float& radius, const Vector& origin)
{
    int16_t i;
    const float squared = squaredf(radius);
    for (i = 0; i < g_numWaypoints; i++)
    {
        if ((m_paths[i].origin - origin).GetLengthSquared() > squared)
            continue;
        
        queueID.Push(i);
    }
}

void Waypoint::Add(const int flags, const Vector& waypointOrigin)
{
    int index = -1, i;

    Vector forward = nullvec;
    Path* path = nullptr;

    bool placeNew = true;
    Vector newOrigin = waypointOrigin;

    if (waypointOrigin == nullvec)
    {
        if (FNullEnt(g_hostEntity))
            return;
        
        newOrigin = GetEntityOrigin(g_hostEntity);
    }

    if (g_botManager->GetBotsNum())
        g_botManager->RemoveAll();

    g_waypointsChanged = true;

    switch (flags)
    {
    case 9:
    {
        newOrigin = m_learnPosition;
        break;
    }
    case 10:
    {
        index = FindNearest(GetEntityOrigin(g_hostEntity), 25.0f);
        if (IsValidWaypoint(index))
        {
            if ((m_paths[index].origin - GetEntityOrigin(g_hostEntity)).GetLengthSquared() < squaredf(25.0f))
            {
                placeNew = false;
                path = &m_paths[index];

                int accumFlags = 0;
                for (i = 0; i < Const_MaxPathIndex; i++)
                    accumFlags += path->connectionFlags[i];

                if (!accumFlags)
                    path->origin = (path->origin + GetEntityOrigin(g_hostEntity)) * 0.5f;
            }
        }
        break;
    }
    }

    m_lastDeclineWaypoint = index;

    if (placeNew)
    {
        if (g_numWaypoints >= Const_MaxNavAreas)
            return;

        index = m_paths.Size();
        if (!m_paths.Push(Path{}))
        {
            AddLogEntry(Log::Memory, "unexpected memory error");
            return;
        }
        path = &m_paths[index];
        g_numWaypoints = m_paths.Size();

        if (flags == 1)
            path->flags = WAYPOINT_FALLCHECK;
        else
            path->flags = 0;

        // store the origin (location) of this waypoint
        path->origin = newOrigin;
        path->mesh = 0;
        path->gravity = 0;

        for (i = 0; i < Const_MaxPathIndex; i++)
        {
            path->index[i] = -1;
            path->connectionFlags[i] = 0;
        }

        // store the last used waypoint for the auto waypoint code...
        m_lastWaypoint = GetEntityOrigin(g_hostEntity);
    }

    // set the time that this waypoint was originally displayed...
    if (m_waypointDisplayTime)
        m_waypointDisplayTime[index] = 0.0f;

    if (flags == 9)
        m_lastJumpWaypoint = index;
    else if (flags == 10)
    {
        AddPath(m_lastJumpWaypoint, index);

        for (i = 0; i < Const_MaxPathIndex; i++)
        {
            if (m_paths[m_lastJumpWaypoint].index[i] == index)
            {
                m_paths[m_lastJumpWaypoint].connectionFlags[i] |= PATHFLAG_JUMP;
                break;
            }
        }

        CalculateWayzone(index);

        return;
    }

    // disable autocheck if we're analyzing
    if ((!FNullEnt(g_hostEntity) && g_hostEntity->v.flags & FL_DUCKING && !g_analyzewaypoints) || g_analyzeputrequirescrouch)
        path->flags |= WAYPOINT_CROUCH;  // set a crouch waypoint

    if (!FNullEnt(g_hostEntity) && g_hostEntity->v.movetype == MOVETYPE_FLY && !g_analyzewaypoints)
        path->flags |= WAYPOINT_LADDER;
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
        break;

    case 6: // use button waypoints
        path->flags |= WAYPOINT_USEBUTTON;
        break;

    case 100:
        path->flags |= WAYPOINT_GOAL;
        break;
    }

    if (flags == 102)
        m_lastFallWaypoint = index;
    else if (flags == 103 && m_lastFallWaypoint != -1)
    {
        AddPath(m_lastFallWaypoint, index);
        m_lastFallWaypoint = -1;
    }

    if (flags == 104)
        path->flags |= WAYPOINT_ZMHMCAMP;
    else if (flags == 105)
        path->flags |= WAYPOINT_HMCAMPMESH;

    // Ladder waypoints need careful connections
    if (path->flags & WAYPOINT_LADDER)
    {
        float minDistance = FLT_MAX;
        float distance;
        int destIndex = -1;

        TraceResult tr{};

        // calculate all the paths to this new waypoint
        for (i = 0; i < g_numWaypoints; i++)
        {
            if (i == index)
                continue; // skip the waypoint that was just added

            // other ladder waypoints should connect to this
            if (m_paths[i].flags & WAYPOINT_LADDER)
            {
                // check if the waypoint is reachable from the new one
                TraceLine(newOrigin, m_paths[i].origin, true, g_hostEntity, &tr);

                if (tr.flFraction == 1.0f && cabsf(newOrigin.x - m_paths[i].origin.x) < 64 && cabsf(newOrigin.y - m_paths[i].origin.y) < 64 && cabsf(newOrigin.z - m_paths[i].origin.z) < g_autoPathDistance)
                {
                    AddPath(index, i);
                    AddPath(i, index);
                }
            }
            else
            {
                distance = (m_paths[i].origin - newOrigin).GetLengthSquared();
                if (distance < minDistance)
                {
                    destIndex = i;
                    minDistance = distance;
                }

                if (IsNodeReachable(newOrigin, m_paths[destIndex].origin))
                    AddPath(index, destIndex);
            }
        }

        if (IsValidWaypoint(destIndex))
        {
            if (g_analyzewaypoints)
            {
                AddPath(index, destIndex);
                AddPath(destIndex, index);
            }
            else
            {
                // check if the waypoint is reachable from the new one (one-way)
                if (IsNodeReachable(newOrigin, m_paths[destIndex].origin))
                    AddPath(index, destIndex);

                // check if the new one is reachable from the waypoint (other way)
                if (IsNodeReachable(m_paths[destIndex].origin, newOrigin))
                    AddPath(destIndex, index);
            }
        }
    }
    else
    {
        float pathDist;
        const float addDist = squaredf(ebot_analyze_distance.GetFloat() * 1.9f);

        // calculate all the paths to this new waypoint
        for (i = 0; i < g_numWaypoints; i++)
        {
            if (i == index)
                continue; // skip the waypoint that was just added

            if (g_analyzewaypoints) // if we're analyzing, be careful (we dont want path errors)
            {
                pathDist = (m_paths[i].origin - newOrigin).GetLengthSquared2D();
                if (pathDist > addDist)
                    continue;

                if (m_paths[i].flags& WAYPOINT_LADDER && (IsNodeReachable(newOrigin, m_paths[i].origin) || IsNodeReachableWithJump(newOrigin, m_paths[i].origin)))
                {
                    AddPath(index, i);
                    AddPath(i, index);
                }

                if (!IsNodeReachable(newOrigin, m_paths[i].origin) && IsNodeReachableWithJump(newOrigin, m_paths[i].origin))
                    AddPath(index, i, 1);

                if (!IsNodeReachable(m_paths[i].origin, newOrigin) && IsNodeReachableWithJump(m_paths[i].origin, newOrigin))
                    AddPath(i, index, 1);

                if (IsNodeReachable(newOrigin, m_paths[i].origin) && IsNodeReachable(m_paths[i].origin, newOrigin))
                {
                    AddPath(index, i);
                    AddPath(i, index);
                }
                else if (!ebot_analyze_disable_fall_connections.GetBool())
                {
                    if (IsNodeReachable(newOrigin, m_paths[i].origin) && newOrigin.z > m_paths[i].origin.z)
                        AddPath(index, i);

                    if (IsNodeReachable(m_paths[i].origin, newOrigin) && newOrigin.z < m_paths[i].origin.z)
                        AddPath(i, index);
                }
            }
            else
            {
                // check if the waypoint is reachable from the new one (one-way)
                if (IsNodeReachable(newOrigin, m_paths[i].origin))
                    AddPath(index, i);

                // check if the new one is reachable from the waypoint (other way)
                if (IsNodeReachable(m_paths[i].origin, newOrigin))
                    AddPath(i, index);
            }
        }
    }

    PlaySound(g_hostEntity, "weapons/xbow_hit1.wav");
    CalculateWayzone(index); // calculate the wayzone of this waypoint
}

void Waypoint::Delete(void)
{
    const int index = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);
    if (!IsValidWaypoint(index))
        return;

    DeleteByIndex(index);
}

void Waypoint::DeleteByIndex(const int index)
{
    g_waypointsChanged = true;

    if (g_numWaypoints < 1)
        return;

    if (g_botManager->GetBotsNum())
        g_botManager->RemoveAll();

    if (!IsValidWaypoint(index))
        return;

    Path* path = nullptr;

    int i, j;
    for (i = 0; i < g_numWaypoints; i++) // delete all references to Node
    {
        path = &m_paths[i];
        if (!path)
            continue;

        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (path->index[j] == index)
            {
                path->index[j] = -1;  // unassign this path
                path->connectionFlags[j] = 0;
            }
        }
    }

    for (i = 0; i < g_numWaypoints; i++)
    {
        path = &m_paths[i];
        if (!path)
            continue;

        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (path->index[j] > index)
                path->index[j]--;
        }
    }

    m_paths.RemoveAt(index);

    g_numWaypoints--;
    if (m_waypointDisplayTime)
        m_waypointDisplayTime[index] = 0.0f;

    PlaySound(g_hostEntity, "weapons/mine_activate.wav");
}

void Waypoint::DeleteFlags(void)
{
    const int index = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);
    if (!IsValidWaypoint(index))
        return;

    m_paths[index].flags = 0;
    PlaySound(g_hostEntity, "common/wpn_hudon.wav");
}

// this function allow manually changing flags
void Waypoint::ToggleFlags(const int toggleFlag)
{
    const int index = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);
    if (!IsValidWaypoint(index))
        return;

    if (m_paths[index].flags & toggleFlag)
        m_paths[index].flags &= ~toggleFlag;

    else if (!(m_paths[index].flags & toggleFlag))
    {
        if (toggleFlag == WAYPOINT_SNIPER && !(m_paths[index].flags & WAYPOINT_CAMP))
        {
            AddLogEntry(Log::Error, "Cannot assign sniper flag to waypoint #%d. This is not camp waypoint", index);
            return;
        }

        m_paths[index].flags |= toggleFlag;
    }

    // play "done" sound...
    PlaySound(g_hostEntity, "common/wpn_hudon.wav");
}

// this function allow manually setting the zone radius
void Waypoint::SetRadius(const int radius)
{
    const int index = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);
    if (!IsValidWaypoint(index))
        return;

    m_paths[index].radius = static_cast<uint8_t>(cclamp(radius, 0, 255));
    PlaySound(g_hostEntity, "common/wpn_hudon.wav");
}

// this function checks if waypoint A has a connection to waypoint B
bool Waypoint::IsConnected(const int pointA, const int pointB)
{
    if (pointA == -1)
        return false;

    for (const auto& link : m_paths[pointA].index)
    {
        if (link == pointB)
            return true;
    }

    return false;
}

// this function finds waypoint the user is pointing at
int Waypoint::GetFacingIndex(void)
{
    if (FNullEnt(g_hostEntity))
        return -1;

    int pointedIndex = -1;
    float range = 5.32f;
    const int nearestNode = FindNearest(g_hostEntity->v.origin, 54.0f);

    // check bounds from eyes of editor
    const Vector eyePosition = g_hostEntity->v.origin + g_hostEntity->v.view_ofs;

    int i;
    for (i = 0; i < g_numWaypoints; i++)
    {
        const Path* path = &m_paths[i];

        // skip nearest waypoint to editor, since this used mostly for adding / removing paths
        if (nearestNode == i)
            continue;

        const Vector to = path->origin - g_hostEntity->v.origin;
        Vector angles = (to.ToAngles() - g_hostEntity->v.v_angle);
        angles.ClampAngles();

        // skip the waypoints that are too far away from us, and we're not looking at them directly
        if (to.GetLengthSquared() > squaredf(500.0f) || cabsf(angles.y) > range)
            continue;

        // check if visible, (we're not using visiblity tables here, as they not valid at time of waypoint editing)
        TraceResult tr{};
        TraceLine(eyePosition, path->origin, false, false, g_hostEntity, &tr);

        if (tr.flFraction != 1.0f)
            continue;

        const float bestAngle = angles.y;

        angles = -g_hostEntity->v.v_angle;
        angles.x = -angles.x;
        angles += ((path->origin - Vector(0.0f, 0.0f, (path->flags & WAYPOINT_CROUCH) ? 17.0f : 34.0f)) - eyePosition).ToAngles();
        angles.ClampAngles();

        if (angles.x > 0.0f)
            continue;

        pointedIndex = i;
        range = bestAngle;
    }

    return pointedIndex;
}

// this function allow player to manually create a path from one waypoint to another
void Waypoint::CreatePath(const int dir)
{
    const int nodeFrom = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);

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

    if (nodeTo == nodeFrom)
    {
        CenterPrint("Unable to connect waypoint with itself");
        return;
    }

    if (dir == PATHCON_OUTGOING)
        AddPath(nodeFrom, nodeTo);
    else if (dir == PATHCON_INCOMING)
        AddPath(nodeTo, nodeFrom);
    else if (dir == PATHCON_JUMPING)
        AddPath(nodeFrom, nodeTo, 1);
    else if (dir == PATHCON_BOOSTING)
        AddPath(nodeFrom, nodeTo, 2);
    else if (dir == PATHCON_VISIBLE)
        AddPath(nodeFrom, nodeTo, 3);
    else
    {
        AddPath(nodeFrom, nodeTo);
        AddPath(nodeTo, nodeFrom);
    }

    PlaySound(g_hostEntity, "common/wpn_hudon.wav");
    g_waypointsChanged = true;
}

void Waypoint::TeleportWaypoint(void)
{
    m_facingAtIndex = GetFacingIndex();
    if (!IsValidWaypoint(m_facingAtIndex))
        return;
    
    (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[m_facingAtIndex].origin);
}

// this function allow player to manually remove a path from one waypoint to another
void Waypoint::DeletePath(void)
{
    int nodeFrom = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);
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

    int index = 0;
    for (index = 0; index < Const_MaxPathIndex; index++)
    {
        if (m_paths[nodeFrom].index[index] == nodeTo)
        {
            g_waypointsChanged = true;

            m_paths[nodeFrom].index[index] = -1; // unassign this path
            m_paths[nodeFrom].connectionFlags[index] = 0;

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
        if (m_paths[nodeFrom].index[index] == nodeTo)
        {
            g_waypointsChanged = true;

            m_paths[nodeFrom].index[index] = -1; // unassign this path
            m_paths[nodeFrom].connectionFlags[index] = 0;

            PlaySound(g_hostEntity, "weapons/mine_activate.wav");
            return;
        }
    }

    CenterPrint("There is already no path on this waypoint");
}

void Waypoint::DeletePathByIndex(int nodeFrom, int nodeTo)
{
    if (!IsValidWaypoint(nodeFrom))
        return;

    if (!IsValidWaypoint(nodeTo))
        return;

    int index = 0;
    for (index = 0; index < Const_MaxPathIndex; index++)
    {
        if (m_paths[nodeFrom].index[index] == nodeTo)
        {
            g_waypointsChanged = true;

            m_paths[nodeFrom].index[index] = -1; // unassign this path
            m_paths[nodeFrom].connectionFlags[index] = 0;

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
        if (m_paths[nodeFrom].index[index] == nodeTo)
        {
            g_waypointsChanged = true;

            m_paths[nodeFrom].index[index] = -1; // unassign this path
            m_paths[nodeFrom].connectionFlags[index] = 0;

            PlaySound(g_hostEntity, "weapons/mine_activate.wav");
            return;
        }
    }

    CenterPrint("There is already no path on this waypoint");
}

void Waypoint::CacheWaypoint(void)
{
    const int node = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);
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
void Waypoint::CalculateWayzone(const int index)
{
    if (!IsValidWaypoint(index))
        return;

    Path* path = &m_paths[index];
    if ((path->flags & (WAYPOINT_LADDER | WAYPOINT_GOAL | WAYPOINT_CAMP | WAYPOINT_RESCUE | WAYPOINT_CROUCH)) || m_learnJumpWaypoint)
    {
        path->radius = 0;
        return;
    }

    for (const auto& link : path->index)
    {
        if (!IsValidWaypoint(link))
            continue;

        const Path* pointer = &m_paths[link];
        if (pointer->flags & (WAYPOINT_LADDER | WAYPOINT_JUMP))
        {
            path->radius = 0;
            return;
        }
    }

    TraceResult tr{};
    Vector start, direction, radiusStart, radiusEnd, dropStart, dropEnd;
    bool wayBlocked = false;
    int finalRadius = 0;
    float scan;

    uint8_t scanDistance;
    uint16_t circleRadius;
    for (scanDistance = 32; scanDistance < 128; scanDistance += 16)
    {
        scan = static_cast<float>(scanDistance);
        start = path->origin;

        MakeVectors(nullvec);
        direction = g_pGlobals->v_forward * scan;
        direction = direction.ToAngles();

        finalRadius = scan;

        for (circleRadius = 0; circleRadius < 360; circleRadius += 20)
        {
            MakeVectors(direction);
            radiusStart = start + g_pGlobals->v_forward * scan;
            radiusEnd = start + g_pGlobals->v_forward * scan;

            TraceHull(radiusStart, radiusEnd, true, head_hull, g_hostEntity, &tr);
            if (tr.flFraction < 1.0f)
            {
                TraceLine(radiusStart, radiusEnd, true, false, g_hostEntity, &tr);
                if (!FNullEnt(tr.pHit) && (FClassnameIs(tr.pHit, "func_door") || FClassnameIs(tr.pHit, "func_door_rotating")))
                {
                    finalRadius = 0;
                    wayBlocked = true;
                    break;
                }

                wayBlocked = true;
                finalRadius -= 16;
                break;
            }

            dropStart = start + g_pGlobals->v_forward * scan;
            dropEnd = dropStart - Vector(0.0f, 0.0f, scan + 60.0f);

            TraceHull(dropStart, dropEnd, true, head_hull, g_hostEntity, &tr);
            if (tr.flFraction >= 1.0f)
            {
                wayBlocked = true;
                finalRadius -= 16;
                break;
            }

            dropStart = start - g_pGlobals->v_forward * scan;
            dropEnd = dropStart - Vector(0.0f, 0.0f, scan + 60.0f);

            TraceHull(dropStart, dropEnd, true, head_hull, g_hostEntity, &tr);
            if (tr.flFraction >= 1.0f)
            {
                wayBlocked = true;
                finalRadius -= 16;
                break;
            }

            radiusEnd.z += 34.0f;
            TraceHull(radiusStart, radiusEnd, true, head_hull, g_hostEntity, &tr);
            if (tr.flFraction < 1.0f)
            {
                wayBlocked = true;
                finalRadius -= 16;
                break;
            }

            direction.y = AngleNormalize(direction.y + static_cast<float>(circleRadius));
        }

        if (wayBlocked)
            break;
    }

    finalRadius -= 16;
    path->radius = static_cast<uint8_t>(cclamp(finalRadius, 0, 255));
}

Vector Waypoint::GetBottomOrigin(const Path* waypoint)
{
    Vector waypointOrigin = waypoint->origin;
    if (waypoint->flags & WAYPOINT_CROUCH)
        waypointOrigin.z -= 18.0f;
    else
        waypointOrigin.z -= 36.0f;

    return waypointOrigin;
}

void Waypoint::InitTypes(void)
{
    int16_t i;
    uint32_t flags;
    for (i = 0; i < g_numWaypoints; i++)
    {
        flags = m_paths[i].flags;
        if (flags & WAYPOINT_GOAL)
            m_goalPoints.Push(i);
        else if (flags & WAYPOINT_CAMP)
            m_campPoints.Push(i);
        else if (flags & WAYPOINT_RESCUE)
            m_rescuePoints.Push(i);
        else if (flags & WAYPOINT_ZMHMCAMP)
            m_zmHmPoints.Push(i);
        else if (flags & WAYPOINT_HMCAMPMESH)
            m_hmMeshPoints.Push(i);
        else if (flags & WAYPOINT_TERRORIST)
            m_terrorPoints.Push(i);
        else if (flags & WAYPOINT_COUNTER)
            m_ctPoints.Push(i);
    }
}

void Waypoint::InitPathMatrix(void)
{
    int16_t i, j, k;

    if (m_distMatrix)
        delete[] m_distMatrix;

    m_distMatrix = new(std::nothrow) int16_t[g_numWaypoints * g_numWaypoints];
    if (!m_distMatrix)
    {
        m_distMatrixReady = false;
        return;
    }

    if (LoadPathMatrix())
    {
        m_distMatrixReady = true;
        return; // matrix loaded from the file
    }

    for (i = 0; i < g_numWaypoints; i++)
    {
        for (j = 0; j < g_numWaypoints; j++)
            *(m_distMatrix + i * g_numWaypoints + j) = static_cast<int16_t>(32766);
    }

    for (i = 0; i < g_numWaypoints; i++)
    {
        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (IsValidWaypoint(m_paths[i].index[j]))
                *(m_distMatrix + (i * g_numWaypoints) + m_paths[i].index[j]) = static_cast<int16_t>((m_paths[i].origin - m_paths[m_paths[i].index[j]].origin).GetLength());
        }
    }

    for (i = 0; i < g_numWaypoints; i++)
        *(m_distMatrix + (i * g_numWaypoints) + i) = 0.0f;

    for (k = 0; k < g_numWaypoints; k++)
    {
        for (i = 0; i < g_numWaypoints; i++)
        {
            for (j = 0; j < g_numWaypoints; j++)
            {
                if (*(m_distMatrix + (i * g_numWaypoints) + k) + *(m_distMatrix + (k * g_numWaypoints) + j) < (*(m_distMatrix + (i * g_numWaypoints) + j)))
                    *(m_distMatrix + (i * g_numWaypoints) + j) = *(m_distMatrix + (i * g_numWaypoints) + k) + *(m_distMatrix + (k * g_numWaypoints) + j);
            }
        }
    }

    // save path matrix to file for faster access
    SavePathMatrix();
    m_distMatrixReady = true;
}

void Waypoint::SavePathMatrix(void)
{
    char matrixFilePath[1024];
    const char* waypointDir = GetWaypointDir();
    const char* mapName = GetMapName();

    FormatBuffer(matrixFilePath, "%smatrix/%s.emt", waypointDir, mapName);
    File fp(matrixFilePath, "wb");

    // unable to open file
    if (!fp.IsValid())
        return;

    // write number of waypoints
    fp.Write(&g_numWaypoints, sizeof(int16_t));

    // write distance matrix
    fp.Write(m_distMatrix, sizeof(int16_t), g_numWaypoints * g_numWaypoints);

    // and close the file
    fp.Close();
}

bool Waypoint::LoadPathMatrix(void)
{
    char matrixFilePath[1024];
    const char* waypointDir = GetWaypointDir();
    const char* mapName = GetMapName();

    FormatBuffer(matrixFilePath, "%smatrix/%s.emt", waypointDir, mapName);
    File fp(matrixFilePath, "rb");

    // file doesn't exists return false
    if (!fp.IsValid())
        return false;

    int num = 0;

    // read number of waypoints
    fp.Read(&num, sizeof(int16_t));

    if (num != g_numWaypoints)
    {
        fp.Close();
        SavePathMatrix();
        return false;
    }

    // read distance matrixes
    fp.Read(m_distMatrix, sizeof(int16_t), g_numWaypoints * g_numWaypoints);

    // and close the file
    fp.Close();
    return true;
}

float dsq(const float* start, const float* end)
{
    const float dx = start[0] - start[0];
    const float dy = start[1] - start[1];
    const float dz = start[2] - start[2];
    return dx * dx + dy * dy + dz * dz;
}

void Waypoint::Sort(const int16_t self, int16_t index[], const int size)
{
    float pri_i, pri_j;
    int i, j, min, temp;
    for (i = 0; i < size - 1; i++)
    {
        min = i;
        for (j = i + 1; j < size; j++)
        {
            if (IsValidWaypoint(index[i]))
                pri_i = dsq(m_paths[self].origin, m_paths[index[i]].origin);
            else
                pri_i = 65355.0f;

            if (IsValidWaypoint(index[j]))
                pri_j = dsq(m_paths[self].origin, m_paths[index[j]].origin);
            else
                pri_j = 65355.0f;

            if (pri_j < pri_i)
                min = j;
        }

        if (min != i)
        {
            temp = index[i];
            index[i] = index[min];
            index[min] = temp;
        }
    }
}

static int8_t tryDownload;
#ifdef PLATFORM_LINUX
// the WriteCallback function is called by cURL when there is data to be written.
// this is necessary for compatibility with older versions of cURL, which do not
// support the CURLOPT_WRITEDATA option directly (linux)
size_t WriteCallback(void* contents, size_t size, size_t nmemb, FILE* stream)
{
    const size_t written = fwrite(contents, size, nmemb, stream);
    if (written < nmemb)
        ServerPrint("Error: fwrite wrote fewer items than expected: %zu out of %zu", written, nmemb);
    return written;
}
#endif

bool Waypoint::Download(void)
{
    tryDownload++;
#ifdef PLATFORM_WIN32
    // could be missing or corrupted? then avoid crash...
    const HMODULE hUrlMon = LoadLibrary("urlmon.dll");
    if (hUrlMon)
    {
        ServerPrint("UrlMon found on the machine");

        typedef HRESULT(WINAPI* URLDownloadToFileFn)(LPUNKNOWN, LPCSTR, LPCSTR, DWORD, LPBINDSTATUSCALLBACK);
        const URLDownloadToFileFn pURLDownloadToFile = reinterpret_cast<URLDownloadToFileFn>(GetProcAddress(hUrlMon, "URLDownloadToFileA"));

        if (pURLDownloadToFile)
        {
            ServerPrint("UrlMon loaded successfully");
            char tpath[1024];
            FormatBuffer(tpath, "%s/%s.ewp", ebot_download_waypoints_from.GetString(), GetMapName());
            if (SUCCEEDED(pURLDownloadToFile(nullptr, tpath, CheckSubfolderFile(), 0, nullptr)))
            {
                ServerPrint("UrlMon download successful");
                FreeLibrary(hUrlMon);
                return true;
            }
        }
        else
            ServerPrint("Error: Could not find URLDownloadToFileA in UrlMon, UrlMon is courrupted!");

        if (FreeLibrary(hUrlMon))
            ServerPrint("UrlMon unloaded successfully");
        else
            ServerPrint("Cannot able to unload UrlMon!");
    }
    else
        ServerPrint("Error: Could not load UrlMon, could be missing or courrupted");
#else
#ifdef CURL_AVAILABLE
    if (curl_version_info(CURLVERSION_NOW))
    {
        CURL* curl;
        CURLcode res;

        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();

        if (curl)
        {
            char tpath[1024];
            FormatBuffer(tpath, "%s/%s.ewp", ebot_download_waypoints_from.GetString(), GetMapName());
            const char* downloadURL = &tpath;
            curl_easy_setopt(curl, CURLOPT_URL, downloadURL);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            ServerPrint("Downloading from cURL: %s", downloadURL);

            // path to the 'cstrike/maps' directory
            FormatBuffer(tpath, "%s/%s.ewp", GetWaypointDir(), GetMapName());
            const char* filepath = &tpath;
            FILE* fp = fopen(filepath, "wb");
            if (!fp)
            {
                ServerPrint("Error: Could not open file for writing");
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return false;
            }

            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

            res = curl_easy_perform(curl);

            // check HTTP response code
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code != 200)
            {
                ServerPrint("Error: HTTP response code is not 200, but %ld", response_code);
                fclose(fp);
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return false;
            }

            fclose(fp);
            if (res != CURLE_OK)
            {
                ServerPrint("Error: curl_easy_perform() failed: %s", curl_easy_strerror(res));
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return false;
            }

            curl_easy_cleanup(curl);
            ServerPrint("cURL download successful");
            return true;
        }

        ServerPrint("Error: Could not initialize cURL handle");
        curl_global_cleanup();
    }
    else
        ServerPrint("Error: Could not find valid cURL version");
#else
    // check if wget is installed
    if (!system("which wget"))
    {
        // wget is installed
        char downloadURL[1024];
        snprintf(downloadURL, sizeof(downloadURL), "%s/%s.ewp", ebot_download_waypoints_from.GetString(), GetMapName());

        char tpath[1024];
        FormatBuffer(tpath, "%s/%s.ewp", GetWaypointDir(), GetMapName());
        const char* filepath = tpath;

        char command[1024];
        snprintf(command, sizeof(command), "wget -O %s %s", filepath, downloadURL);
        printf("Executing wget command: %s", command);
        const int result = system(command);

        if (!result)
        {
            ServerPrint("wget download successful");
            return true;
        }
        else
        {
            ServerPrint("Error: wget command failed with code %d", result);
            return false;
        }
    }
    else
        ServerPrint("Error: Neither curl nor wget is available");
#endif
#endif

    return false;
}

static int8_t tryLoad;
bool Waypoint::Load(void)
{
    Initialize();
    safedel(m_waypointDisplayTime);

    int i;
    const char* pathtofl = CheckSubfolderFile();
    File fp(pathtofl, "rb");
    if (fp.IsValid())
    {
        WaypointHeader header;
        fp.Read(&header, sizeof(header));

        if (header.fileVersion > static_cast<int32_t>(FV_WAYPOINT))
        {
            g_numWaypoints = 0;
            CenterPrint("Waypoint version is too high, update your ebot!");
            sprintf(m_infoBuffer, "Waypoint version is too high, update your ebot!");
        }
        else if (header.fileVersion == static_cast<int32_t>(FV_WAYPOINT))
        {
            g_numWaypoints = header.pointNumber;
            m_paths.Resize(g_numWaypoints, true);

            Path paths[g_numWaypoints];
            if (Compressor::Uncompress(pathtofl, sizeof(WaypointHeader), reinterpret_cast<uint8_t*>(paths), g_numWaypoints * sizeof(Path)) != -1)
            {
                for (i = 0; i < g_numWaypoints; i++)
                {
                    Sort(i, paths[i].index);
                    m_paths.Push(paths[i]);
                }
            }
        }
        else if (header.fileVersion == static_cast<int32_t>(126))
        {
            g_numWaypoints = header.pointNumber;
            m_paths.Resize(g_numWaypoints, true);

            Path path;
            for (i = 0; i < g_numWaypoints; i++)
            {
                fp.Read(&path, sizeof(Path));
                Sort(i, path.index);
                m_paths.Push(path);
            }
        }
        else if (header.fileVersion == static_cast<int32_t>(125))
        {
            g_numWaypoints = header.pointNumber;
            m_paths.Resize(g_numWaypoints, true);

            struct PathOLD2
            {
                Vector origin{};
                int32 flags{};
                int16 radius{};
                int16 mesh{};
                int16 index[8]{};
                uint16 connectionFlags[8]{};
                float gravity{};
            };

            PathOLD2 paths;
            Path path;

            int C;
            for (i = 0; i < g_numWaypoints; i++)
            {
                fp.Read(&paths, sizeof(PathOLD2));

                path.origin = paths.origin;
                path.radius = static_cast<uint8_t>(cclamp(paths.radius, 0, 255));
                path.flags = static_cast<uint32>(cmax(0, paths.flags));
                path.mesh = static_cast<uint8_t>(cclamp(paths.mesh, 0, 255));
                path.gravity = paths.gravity;

                for (C = 0; C < 8; C++)
                {
                    path.index[C] = static_cast<int16_t>(paths.index[C]);
                    path.connectionFlags[C] = static_cast<uint16_t>(paths.connectionFlags[C]);
                }

                Sort(i, path.index);
                m_paths.Push(path);
            }
        }
        else
        {
            g_numWaypoints = header.pointNumber;
            m_paths.Resize(g_numWaypoints, true);

            struct PathOLD
            {
                int32 pathNumber{};
                int32 flags{};
                Vector origin{};
                float radius{};

                float campStartX{};
                float campStartY{};
                float campEndX{};
                float campEndY{};

                int16 index[8]{};
                uint16 connectionFlags[8]{};
                Vector connectionVelocity[8]{};
                int32 distances[8]{};

                struct Vis_t { uint16 stand{}, crouch{}; } vis;
            };

            PathOLD paths;
            Path path;

            int C;
            for (i = 0; i < g_numWaypoints; i++)
            {
                fp.Read(&paths, sizeof(PathOLD));

                path.origin = paths.origin;
                path.radius = static_cast<uint8_t>(cclampf(paths.radius, 0.0f, 255.0f));
                path.flags = static_cast<uint32>(cmax(0, paths.flags));
                path.mesh = static_cast<uint8_t>(cclampf(paths.campStartX, 0.0f, 255.0f));
                path.gravity = 0.0f;

                for (C = 0; C < 8; C++)
                {
                    path.index[C] = static_cast<int16_t>(paths.index[C]);
                    path.connectionFlags[C] = static_cast<uint16_t>(paths.connectionFlags[C]);
                }

                Sort(i, path.index);
                m_paths.Push(path);
            }

            Save();
        }

        if (cstrncmp(header.author, "EfeDursun125", 12) == 0)
            sprintf(m_infoBuffer, "Using Official Waypoint File By: %s", header.author);
        else
            sprintf(m_infoBuffer, "Using Waypoint File By: %s", header.author);

        fp.Close();
        tryLoad = 0;
        tryDownload = 0;
    }
    else if (tryDownload < 5 && ebot_download_waypoints.GetBool() && Download())
    {
        Load();
        sprintf(m_infoBuffer, "%s.ewp is downloaded from the internet", GetMapName());
        tryLoad = 0;
        tryDownload = 0;
    }
    else
    {
        if (tryLoad < 5)
        {
            Load();
            tryLoad++;
        }
        else
        {
            m_terrorPoints.Destroy();
            m_ctPoints.Destroy();
            m_goalPoints.Destroy();
            m_campPoints.Destroy();
            m_rescuePoints.Destroy();
            m_zmHmPoints.Destroy();
            m_hmMeshPoints.Destroy();
            g_numWaypoints = 0;
        }

        if (tryLoad >= 5 && ebot_analyze_auto_start.GetBool())
        {
            g_waypoint->CreateBasic();
            g_analyzewaypoints = true;
            tryLoad = 0;
            tryDownload = 0;
        }
        else
        {
            sprintf(m_infoBuffer, "%s.ewp does not exist, pleasue use 'ebot wp analyze' for create waypoints! (dont forget using 'ebot wp analyzeoff' when finished)", GetMapName());
            AddLogEntry(Log::Error, m_infoBuffer);
        }

        return false;
    }

    m_terrorPoints.Destroy();
    m_ctPoints.Destroy();
    m_goalPoints.Destroy();
    m_campPoints.Destroy();
    m_rescuePoints.Destroy();
    m_zmHmPoints.Destroy();
    m_hmMeshPoints.Destroy();

    g_waypointsChanged = false;
    m_pathDisplayTime = 0.0f;
    m_arrowDisplayTime = 0.0f;
    g_botManager->InitQuota();

    if (g_numWaypoints > 2)
    {
        // only in lan game
        if (!IsDedicatedServer())
            safeloc(m_waypointDisplayTime, g_numWaypoints);

        InitTypes();
        InitPathMatrix();
    }

    return true;
}

void Waypoint::Save(void)
{
    WaypointHeader header;
    cmemset(header.header, 0, sizeof(header.header));
    cmemset(header.mapName, 0, sizeof(header.mapName));
    cmemset(header.author, 0, sizeof(header.author));

    char waypointAuthor[32];
    if (!FNullEnt(g_hostEntity))
        sprintf(waypointAuthor, "%s", GetEntityName(g_hostEntity));
    else
        sprintf(waypointAuthor, "E-Bot Waypoint Analyzer");

    cstrcpy(header.author, waypointAuthor);

    const char* waypointFilePath = CheckSubfolderFile();

    // remember the original waypoint author
    File rf(waypointFilePath, "rb");
    if (rf.IsValid())
    {
        rf.Read(&header, sizeof(header));
        rf.Close();
    }

    cstrcpy(header.header, FH_WAYPOINT_NEW);
    cstrncpy(header.mapName, GetMapName(), 31);

    header.mapName[31] = 0;
    header.fileVersion = FV_WAYPOINT;
    header.pointNumber = g_numWaypoints;

    File fp(waypointFilePath, "wb");

    // file was opened
    if (fp.IsValid())
    {
        // write the waypoint header to the file...
        fp.Write(&header, sizeof(header), 1);

        int i;
        Path paths[g_numWaypoints];
        for (i = 0; i < g_numWaypoints; i++)
        {
            paths[i] = m_paths[i];
            Sort(i, paths[i].index);
        }

        i = Compressor::Compress(waypointFilePath, reinterpret_cast<uint8_t*>(&header), sizeof(WaypointHeader), reinterpret_cast<uint8_t*>(paths), g_numWaypoints * sizeof(Path));
        if (i == 1)
        {
            ServerPrint("Error: Cannot Save Waypoints");
            CenterPrint("Error: Cannot save waypoints!");
            AddLogEntry(Log::Error, "Error writing '%s' waypoint file", GetMapName());
        }
        else
        {
            ServerPrint("Waypoints Saved");
            CenterPrint("Waypoints are saved!");
            SavePathMatrix();
        }

        fp.Close();
    }
    else
        AddLogEntry(Log::Error, "Error writing '%s' waypoint file", GetMapName());
}

const char* Waypoint::CheckSubfolderFile(void)
{
    static char waypointFilePath[1024]{};
    const char* waypointDir = GetWaypointDir();
    const char* mapName = GetMapName();

    FormatBuffer(waypointFilePath, "%s%s.ewp", waypointDir, mapName);
    if (TryFileOpen(waypointFilePath))
        return &waypointFilePath[0];
    else
    {
        FormatBuffer(waypointFilePath, "%s%s.ewp", waypointDir, mapName);
        if (TryFileOpen(waypointFilePath))
            return &waypointFilePath[0];
    }

    FormatBuffer(waypointFilePath, "%s%s.ewp", waypointDir, mapName);
    return &waypointFilePath[0];
}

// this function returns 2D traveltime to a position
float Waypoint::GetTravelTime(const float maxSpeed, const Vector& src, const Vector& origin)
{
    return (origin - src).GetLength2D() / cabsf(maxSpeed);
}

bool Waypoint::Reachable(edict_t* entity, const int index)
{
    if (FNullEnt(entity))
        return false;

    if (!IsValidWaypoint(index))
        return false;

    const Vector src = GetEntityOrigin(entity);
    const Vector dest = m_paths[index].origin;

    if ((dest - src).GetLengthSquared() > squaredf(1200.0f))
        return false;

    if (entity->v.waterlevel < 2)
    {
        if ((dest.z > src.z + 62.0f || dest.z < src.z - 100.0f) && (!(GetPath(index)->flags & WAYPOINT_LADDER) || (dest - src).GetLengthSquared2D() > squaredf(120.0f)))
            return false;
    }

    TraceResult tr{};
    TraceHull(src, dest, true, head_hull, entity, &tr);
    if (tr.flFraction == 1.0f)
        return true;

    return false;
}

bool Waypoint::IsNodeReachable(const Vector& src, const Vector& destination)
{
    float distance = (destination - src).GetLengthSquared();

    // is the destination not close enough?
    if (distance > squaredf(g_autoPathDistance))
        return false;

    TraceResult tr{};

    // check if we go through a func_illusionary, in which case return false
    TraceHull(src, destination, NO_BOTH, HULL_HEAD, g_hostEntity, &tr);

    if (!FNullEnt(tr.pHit) && cstrcmp("func_illusionary", STRING(tr.pHit->v.classname)) == 0)
        return false; // don't add pathnodes through func_illusionaries

    // check if this waypoint is "visible"...
    TraceLine(src, destination, true, false, g_hostEntity, &tr);

    // if waypoint is visible from current position (even behind head)...
    bool goBehind = false;
    if (tr.flFraction >= 1.0f || (goBehind = IsBreakable(tr.pHit)) || (goBehind = (!FNullEnt(tr.pHit) && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) == 0)))
    {
        // if it's a door check if nothing blocks behind
        if (goBehind)
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

            TraceLine(sourceNew, destinationNew, true, true, g_hostEntity, &tr);

            // check if we didn't hit anything, if not then it's in mid-air
            if (tr.flFraction >= 1.0)
                return false; // can't reach this one
        }

        // check if distance to ground drops more than step height at points between source and destination...
        Vector direction = (destination - src).Normalize(); // 1 unit long
        Vector check = src, down = src;

        down.z = down.z - 1000.0f; // straight down 1000 units

        TraceLine(check, down, true, true, g_hostEntity, &tr);

        float lastHeight = tr.flFraction * 1000.0f; // height from ground
        distance = (destination - check).GetLengthSquared(); // distance from goal

        while (distance > squaredf(10.0f))
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
            distance = (destination - check).GetLengthSquared(); // distance from goal
        }

        return true;
    }

    return false;
}

bool Waypoint::IsNodeReachableWithJump(const Vector& src, const Vector& destination)
{
    float distance = (destination - src).GetLengthSquared();

    // is the destination not close enough?
    if (distance > squaredf(g_autoPathDistance))
        return false;

    TraceResult tr{};

    // check if we go through a func_illusionary, in which case return false
    TraceHull(src, destination, true, head_hull, g_hostEntity, &tr);

    if (!FNullEnt(tr.pHit) && cstrcmp("func_illusionary", STRING(tr.pHit->v.classname)) == 0)
        return false; // don't add pathnodes through func_illusionaries

    // check if this waypoint is "visible"...
    TraceLine(src, destination, true, false, g_hostEntity, &tr);

    // if waypoint is visible from current position (even behind head)...
    bool goBehind = false;
    if (tr.flFraction >= 1.0f || (goBehind = IsBreakable(tr.pHit)) || (goBehind = (!FNullEnt(tr.pHit) && cstrncmp("func_door", STRING(tr.pHit->v.classname), 9) == 0)))
    {
        // if it's a door check if nothing blocks behind
        if (goBehind)
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

            TraceLine(sourceNew, destinationNew, true, true, g_hostEntity, &tr);

            // check if we didn't hit anything, if not then it's in mid-air
            if (tr.flFraction >= 1.0)
                return false; // can't reach this one
        }

        // check if distance to ground drops more than step height at points between source and destination...
        Vector direction = (destination - src).Normalize(); // 1 unit long
        Vector check = src, down = src;

        down.z = down.z - 1000.0f; // straight down 1000 units

        TraceLine(check, down, true, true, g_hostEntity, &tr);

        float lastHeight = tr.flFraction * 1000.0f; // height from ground
        distance = (destination - check).GetLengthSquared(); // distance from goal

        while (distance > squaredf(10.0f))
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
            distance = (destination - check).GetLengthSquared(); // distance from goal
        }

        return true;
    }

    return false;
}

// this function returns path information for waypoint pointed by id
char* Waypoint::GetWaypointInfo(const int id)
{
    Path* path = GetPath(id);

    // if this path is nullptr, return
    if (!path)
        return "\0";

    bool jumpPoint = false;

    // iterate through connections and find, if it's a jump path
    int i;
    for (i = 0; i < Const_MaxPathIndex; i++)
    {
        // check if we got a valid connection
        if (path->index[i] != -1 && (path->connectionFlags[i] & PATHFLAG_JUMP))
        {
            jumpPoint = true;

            if (!(path->flags & WAYPOINT_JUMP))
                path->flags |= WAYPOINT_JUMP;
        }
    }

    char messageBuffer[1024];
    sprintf(messageBuffer, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", 
        (!path->flags && !jumpPoint) ? "(none)" : "", 
        path->flags & WAYPOINT_LIFT ? "LIFT " : "", 
        path->flags & WAYPOINT_CROUCH ? "CROUCH " : "", 
        path->flags & WAYPOINT_CROSSING ? "CROSSING " : "", 
        path->flags & WAYPOINT_CAMP ? "CAMP " : "", 
        path->flags & WAYPOINT_TERRORIST ? "TR " : "", 
        path->flags & WAYPOINT_COUNTER ? "CT " : "", 
        path->flags & WAYPOINT_SNIPER ? "SNIPER " : "", 
        path->flags & WAYPOINT_GOAL ? "GOAL " : "", 
        path->flags & WAYPOINT_LADDER ? "LADDER " : "", 
        path->flags & WAYPOINT_RESCUE ? "RESCUE " : "", 
        path->flags & WAYPOINT_DJUMP ? "ZOMBIE BOOST " : "", 
        path->flags & WAYPOINT_AVOID ? "AVOID " : "", 
        path->flags & WAYPOINT_USEBUTTON ? "USE BUTTON " : "", 
        path->flags & WAYPOINT_FALLCHECK ? "FALL CHECK " : "", 
        jumpPoint ? "JUMP " : "", 
        path->flags & WAYPOINT_ZMHMCAMP ? "HUMAN CAMP " : "", 
        path->flags & WAYPOINT_HMCAMPMESH ? "HUMAN MESH " : "",
        path->flags & WAYPOINT_ZOMBIEONLY ? "ZOMBIE ONLY " : "",
        path->flags & WAYPOINT_HUMANONLY ? "HUMAN ONLY " : "",
        path->flags & WAYPOINT_ZOMBIEPUSH ? "ZOMBIE PUSH " : "",
        path->flags & WAYPOINT_FALLRISK ? "FALL RISK " : "",
        path->flags & WAYPOINT_SPECIFICGRAVITY ? "SPECIFIC GRAVITY " : "",
        path->flags & WAYPOINT_WAITUNTIL ? "WAIT UNTIL GROUND " : "",
        path->flags & WAYPOINT_ONLYONE ? "ONLY ONE BOT " : "");

    // return the message buffer
    return messageBuffer;
}

// this function executes frame of waypoint operation code.
void Waypoint::Think(void)
{
    // this function is only valid on listenserver, and in waypoint enabled mode
    if (FNullEnt(g_hostEntity))
        return;

    ShowWaypointMsg();

    float nearestDistance = FLT_MAX;
    if (m_learnJumpWaypoint)
    {
        if (!m_endJumpPoint)
        {
            if (g_hostEntity->v.buttons & IN_JUMP)
            {
                Add(9);
                m_timeJumpStarted = engine->GetTime();
                m_endJumpPoint = true;
            }
            else
            {
                m_learnVelocity = g_hostEntity->v.velocity;
                m_learnPosition = GetEntityOrigin(g_hostEntity);
            }
        }
        else if (((g_hostEntity->v.flags & FL_PARTIALGROUND) || g_hostEntity->v.movetype == MOVETYPE_FLY) && m_timeJumpStarted + 0.1 < engine->GetTime())
        {
            Add(10);
            m_learnJumpWaypoint = false;
            m_endJumpPoint = false;
        }
    }

    // check if it's a autowaypoint mode enabled
    if (g_autoWaypoint && (g_hostEntity->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)))
    {
        // find the distance from the last used waypoint
        float distance = (m_lastWaypoint - GetEntityOrigin(g_hostEntity)).GetLengthSquared();

        if (distance > 16384)
        {
            int i;

            // check that no other reachable waypoints are nearby...
            for (i = 0; i < g_numWaypoints; i++)
            {
                if (IsNodeReachable(GetEntityOrigin(g_hostEntity), m_paths[i].origin))
                {
                    distance = (m_paths[i].origin - GetEntityOrigin(g_hostEntity)).GetLengthSquared();
                    if (distance < nearestDistance)
                        nearestDistance = distance;
                }
            }

            // make sure nearest waypoint is far enough away...
            if (nearestDistance >= 16384)
                Add(0);  // place a waypoint here
        }
    }
}

void Waypoint::ShowWaypointMsg(void)
{
    m_facingAtIndex = GetFacingIndex();

    // reset the minimal distance changed before
    float nearestDistance = FLT_MAX;
    int nearestIndex = -1;

    auto update = [&](const int i)
        {
            const float distance = (m_paths[i].origin - GetEntityOrigin(g_hostEntity)).GetLengthSquared();

            // check if waypoint is whitin a distance, and is visible
            if ((distance < squaredf(640.0f) && ::IsVisible(m_paths[i].origin, g_hostEntity) && IsInViewCone(m_paths[i].origin, g_hostEntity)) || distance < squaredf(48.0f))
            {
                // check the distance
                if (distance < nearestDistance)
                {
                    nearestIndex = i;
                    nearestDistance = distance;
                }

                // draw mesh links
                if (m_paths[nearestIndex].mesh != static_cast<uint8_t>(0) && IsInViewCone(m_paths[nearestIndex].origin, g_hostEntity) && (m_paths[nearestIndex].flags & WAYPOINT_HMCAMPMESH || m_paths[nearestIndex].flags & WAYPOINT_ZMHMCAMP))
                {
                    int x;
                    for (x = 0; x < g_numWaypoints; x++)
                    {
                        if (!(m_paths[nearestIndex].flags & WAYPOINT_HMCAMPMESH) && !(m_paths[nearestIndex].flags & WAYPOINT_ZMHMCAMP))
                            continue;

                        if (m_paths[nearestIndex].mesh != m_paths[x].mesh)
                            continue;

                        if (!IsInViewCone(m_paths[x].origin, g_hostEntity))
                            continue;

                        const Vector& src = m_paths[nearestIndex].origin + Vector(0, 0, (m_paths[nearestIndex].flags & WAYPOINT_CROUCH) ? 9.0f : 18.0f);
                        const Vector& dest = m_paths[x].origin + Vector(0, 0, (m_paths[x].flags & WAYPOINT_CROUCH) ? 9.0f : 18.0f);

                        // draw links
                        engine->DrawLine(g_hostEntity, src, dest, Color(0, 0, 255, 255), 5, 0, 0, 10);
                    }
                }

                if (m_waypointDisplayTime)
                {
                    if (m_waypointDisplayTime[i] + 1.0f < engine->GetTime())
                    {
                        float nodeHeight = (m_paths[i].flags & WAYPOINT_CROUCH) ? 36.0f : 72.0f; // check the node height
                        float nodeHalfHeight = nodeHeight * 0.5f;

                        // all waypoints are by default are green
                        Color nodeColor = Color(ebot_waypoint_r.GetFloat(), ebot_waypoint_g.GetFloat(), ebot_waypoint_b.GetFloat(), 255);

                        // colorize all other waypoints
                        if (m_paths[i].flags & WAYPOINT_CAMP)
                            nodeColor = Color(0, 255, 255, 255);
                        else if (m_paths[i].flags & WAYPOINT_GOAL)
                            nodeColor = Color(128, 0, 255, 255);
                        else if (m_paths[i].flags & WAYPOINT_LADDER)
                            nodeColor = Color(128, 64, 0, 255);
                        else if (m_paths[i].flags & WAYPOINT_RESCUE)
                            nodeColor = Color(255, 255, 255, 255);
                        else if (m_paths[i].flags & WAYPOINT_AVOID)
                            nodeColor = Color(255, 0, 0, 255);
                        else if (m_paths[i].flags & WAYPOINT_FALLCHECK)
                            nodeColor = Color(128, 128, 128, 255);
                        else if (m_paths[i].flags & WAYPOINT_USEBUTTON)
                            nodeColor = Color(0, 0, 255, 255);
                        else if (m_paths[i].flags & WAYPOINT_ZMHMCAMP)
                            nodeColor = Color(199, 69, 209, 255);
                        else if (m_paths[i].flags & WAYPOINT_HMCAMPMESH)
                            nodeColor = Color(50, 125, 255, 255);
                        else if (m_paths[i].flags & WAYPOINT_ZOMBIEONLY)
                            nodeColor = Color(255, 0, 0, 255);
                        else if (m_paths[i].flags & WAYPOINT_HUMANONLY)
                            nodeColor = Color(0, 0, 255, 255);
                        else if (m_paths[i].flags & WAYPOINT_ZOMBIEPUSH)
                            nodeColor = Color(250, 75, 150, 255);
                        else if (m_paths[i].flags & WAYPOINT_FALLRISK)
                            nodeColor = Color(128, 128, 128, 255);
                        else if (m_paths[i].flags & WAYPOINT_SPECIFICGRAVITY)
                            nodeColor = Color(128, 128, 128, 255);
                        else if (m_paths[i].flags & WAYPOINT_ONLYONE)
                            nodeColor = Color(255, 255, 0, 255);
                        else if (m_paths[i].flags & WAYPOINT_WAITUNTIL)
                            nodeColor = Color(0, 0, 255, 255);

                        // colorize additional flags
                        Color nodeFlagColor = Color(0, 0, 0, 0);

                        // check the colors
                        if (m_paths[i].flags & WAYPOINT_SNIPER)
                            nodeFlagColor = Color(130, 87, 0, 255);
                        else if (m_paths[i].flags & WAYPOINT_TERRORIST)
                            nodeFlagColor = Color(255, 0, 0, 255);
                        else if (m_paths[i].flags & WAYPOINT_COUNTER)
                            nodeFlagColor = Color(0, 0, 255, 255);
                        else if (m_paths[i].flags & WAYPOINT_ZMHMCAMP)
                            nodeFlagColor = Color(0, 0, 255, 255);
                        else if (m_paths[i].flags & WAYPOINT_HMCAMPMESH)
                            nodeFlagColor = Color(0, 0, 255, 255);
                        else if (m_paths[i].flags & WAYPOINT_ZOMBIEONLY)
                            nodeFlagColor = Color(255, 0, 255, 255);
                        else if (m_paths[i].flags & WAYPOINT_HUMANONLY)
                            nodeFlagColor = Color(255, 0, 255, 255);
                        else if (m_paths[i].flags & WAYPOINT_ZOMBIEPUSH)
                            nodeFlagColor = Color(255, 0, 0, 255);
                        else if (m_paths[i].flags & WAYPOINT_FALLRISK)
                            nodeFlagColor = Color(250, 75, 150, 255);
                        else if (m_paths[i].flags & WAYPOINT_SPECIFICGRAVITY)
                            nodeFlagColor = Color(128, 0, 255, 255);
                        else if (m_paths[i].flags & WAYPOINT_WAITUNTIL)
                            nodeFlagColor = Color(250, 75, 150, 255);

                        nodeColor.alpha = 255;
                        nodeFlagColor.alpha = 255;

                        // draw node without additional flags
                        if (!nodeFlagColor.red && !nodeFlagColor.blue && !nodeFlagColor.green)
                            engine->DrawLine(g_hostEntity, m_paths[i].origin - Vector(0.0f, 0.0f, nodeHalfHeight), m_paths[i].origin + Vector(0.0f, 0.0f, nodeHalfHeight), nodeColor, ebot_waypoint_size.GetFloat(), 0, 0, 10);
                        else // draw node with flags
                        {
                            engine->DrawLine(g_hostEntity, m_paths[i].origin - Vector(0.0f, 0.0f, nodeHalfHeight), m_paths[i].origin - Vector(0.0f, 0.0f, nodeHalfHeight - nodeHeight * 0.75f), nodeColor, ebot_waypoint_size.GetFloat(), 0, 0, 10); // draw basic path
                            engine->DrawLine(g_hostEntity, m_paths[i].origin - Vector(0.0f, 0.0f, nodeHalfHeight - nodeHeight * 0.75f), m_paths[i].origin + Vector(0.0f, 0.0f, nodeHalfHeight), nodeFlagColor, ebot_waypoint_size.GetFloat(), 0, 0, 10); // draw additional path
                        }

                        if (m_paths[i].flags & WAYPOINT_FALLCHECK || m_paths[i].flags & WAYPOINT_WAITUNTIL)
                        {
                            TraceResult tr{};
                            TraceLine(m_paths[i].origin, m_paths[i].origin - Vector(0.0f, 0.0f, 60.0f), false, false, g_hostEntity, &tr);

                            if (tr.flFraction == 1.0f)
                                engine->DrawLine(g_hostEntity, m_paths[i].origin, m_paths[i].origin - Vector(0.0f, 0.0f, 60.0f), Color(255, 0, 0, 255), ebot_waypoint_size.GetFloat() - 1.0f, 0, 0, 10);
                            else
                                engine->DrawLine(g_hostEntity, m_paths[i].origin, m_paths[i].origin - Vector(0.0f, 0.0f, 60.0f), Color(0, 0, 255, 255), ebot_waypoint_size.GetFloat() - 1.0f, 0, 0, 10);
                        }

                        m_waypointDisplayTime[i] = engine->GetTime();
                    }
                    else if (m_waypointDisplayTime[i] + 2.0f > engine->GetTime()) // what???
                        m_waypointDisplayTime[i] = 0.0f;
                }
            }
        };

    // now iterate through all waypoints in a map, and draw required ones
    const int random = crandomint(1, 2);
    if (random == 1)
    {
        int i;
        for (i = 0; i < g_numWaypoints; i++)
            update(i);
    }
    else
    {
        int i;
        for (i = (g_numWaypoints - 1); i > 0; i--)
            update(i);
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
                engine->DrawLine(g_hostEntity, m_paths[m_findWPIndex].origin, GetEntityOrigin(g_hostEntity), Color(128, 0, 128, 255), 10, 0, 0, 5, LINE_ARROW);

            // cached waypoint - yellow arrow
            if (IsValidWaypoint(m_cacheWaypointIndex))
                engine->DrawLine(g_hostEntity, m_paths[m_cacheWaypointIndex].origin, GetEntityOrigin(g_hostEntity), Color(255, 255, 0, 255), 10, 0, 0, 5, LINE_ARROW);

            // waypoint user facing at - white arrow
            if (IsValidWaypoint(m_facingAtIndex))
                engine->DrawLine(g_hostEntity, m_paths[m_facingAtIndex].origin, GetEntityOrigin(g_hostEntity), Color(255, 255, 255, 255), 10, 0, 0, 5, LINE_ARROW);

            m_arrowDisplayTime = engine->GetTime();
        }
        else if (m_arrowDisplayTime + 1.0 > engine->GetTime()) // what???
            m_arrowDisplayTime = 0.0f;
    }

    // create path pointer for faster access
    Path* path = &m_paths[nearestIndex];
    if (!path)
        return;

    // draw a paths, camplines and danger directions for nearest waypoint
    if (nearestDistance < squaredf(2048) && m_pathDisplayTime < engine->GetTime())
    {
        m_pathDisplayTime = engine->GetTime() + 1.0f;

        // draw the connections
        int i;
        for (i = 0; i < Const_MaxPathIndex; i++)
        {
            if (path->index[i] == -1)
                continue;

            // jump connection
            if (path->connectionFlags[i] & PATHFLAG_JUMP)
                engine->DrawLine(g_hostEntity, path->origin, m_paths[path->index[i]].origin, Color(255, 0, 0, 255), 5, 0, 0, 10);
            else if (path->connectionFlags[i] & PATHFLAG_DOUBLE) // boosting friend connection
                engine->DrawLine(g_hostEntity, path->origin, m_paths[path->index[i]].origin, Color(0, 0, 255, 255), 5, 0, 0, 10);
            else if (IsConnected(path->index[i], nearestIndex)) // twoway connection
                engine->DrawLine(g_hostEntity, path->origin, m_paths[path->index[i]].origin, Color(255, 255, 0, 255), 5, 0, 0, 10);
            else // oneway connection
                engine->DrawLine(g_hostEntity, path->origin, m_paths[path->index[i]].origin, Color(250, 250, 250, 255), 5, 0, 0, 10);
        }

        // now look for oneway incoming connections
        for (i = 0; i < g_numWaypoints; i++)
        {
            if (IsConnected(i, nearestIndex) && !IsConnected(nearestIndex, i))
                engine->DrawLine(g_hostEntity, path->origin, m_paths[i].origin, Color(0, 192, 96, 255), 5, 0, 0, 10);
        }

        // draw the radius circle
        const Vector origin = (path->flags & WAYPOINT_CROUCH) ? path->origin : path->origin - Vector(00.0f, 00.0f, 18.0f);

        // if radius is nonzero, draw a square
        if (path->radius > 4)
        {
            const float root = static_cast<float>(path->radius);
            const Color& def = Color(0, 0, 255, 255);

            engine->DrawLine(g_hostEntity, origin + Vector(root, root, 0.0f), origin + Vector(-root, root, 0.0f), def, 5, 0, 0, 10);
            engine->DrawLine(g_hostEntity, origin + Vector(root, root, 0.0f), origin + Vector(root, -root, 0.0f), def, 5, 0, 0, 10);
            engine->DrawLine(g_hostEntity, origin + Vector(-root, -root, 0.0f), origin + Vector(root, -root, 0.0f), def, 5, 0, 0, 10);
            engine->DrawLine(g_hostEntity, origin + Vector(-root, -root, 0.0f), origin + Vector(-root, root, 0.0f), def, 5, 0, 0, 10);
        }
        else
        {
            const float root = 5.0f;
            const Color& def = Color(0, 0, 255, 255);

            engine->DrawLine(g_hostEntity, origin + Vector(root, -root, 0.0f), origin + Vector(-root, root, 0.0f), def, 5, 0, 0, 10);
            engine->DrawLine(g_hostEntity, origin + Vector(-root, -root, 0.0f), origin + Vector(root, root, 0.0f), def, 5, 0, 0, 10);
        }

        // display some information
        char tempMessage[4096];
        int length;

        // show the information about that point
        if (path->flags & WAYPOINT_SPECIFICGRAVITY)
        {
            length = sprintf(tempMessage, "\n\n\n\n\n\n\n    Waypoint Information:\n\n"
                "      Waypoint %d of %d, Radius: %d\n"
                "      Flags: %s\n\n      %s %f\n      Waypoint Gravity: %f", nearestIndex, g_numWaypoints, path->radius, GetWaypointInfo(nearestIndex), "Your Gravity:", (g_hostEntity->v.gravity * 800.0f), path->gravity);
        }
        else if (path->flags & WAYPOINT_ZMHMCAMP || path->flags & WAYPOINT_HMCAMPMESH)
        {
            length = sprintf(tempMessage, "\n\n\n\n\n\n\n    Waypoint Information:\n\n"
                "      Waypoint %d of %d, Radius: %d\n"
                "      Flags: %s\n\n      %s %d\n", nearestIndex, g_numWaypoints, path->radius, GetWaypointInfo(nearestIndex), "Human Camp Mesh ID:", static_cast<int> (path->mesh));
        }
        else
        {
            length = sprintf(tempMessage, "\n\n\n\n\n\n\n    Waypoint Information:\n\n"
                "      Waypoint %d of %d, Radius: %d\n"
                "      Flags: %s\n\n", nearestIndex, g_numWaypoints, path->radius, GetWaypointInfo(nearestIndex));
        }

        // check if we need to show the cached point index
        if (m_cacheWaypointIndex != -1)
        {
            length += sprintf(&tempMessage[length], "\n    Cached Waypoint Information:\n\n"
                "      Waypoint %d of %d, Radius: %d\n"
                "      Flags: %s\n", m_cacheWaypointIndex, g_numWaypoints, m_paths[m_cacheWaypointIndex].radius, GetWaypointInfo(m_cacheWaypointIndex), (!(m_paths[m_cacheWaypointIndex].flags & WAYPOINT_HMCAMPMESH) && !(m_paths[m_cacheWaypointIndex].flags & WAYPOINT_ZMHMCAMP)) ? "" : "Mesh ID: %d", static_cast<int>(m_paths[m_cacheWaypointIndex].mesh));
        }

        // check if we need to show the facing point index, only if no menu to show
        if (m_facingAtIndex != -1 && !g_clients[ENTINDEX(g_hostEntity) - 1].menu)
        {
            length += sprintf(&tempMessage[length], "\n    Facing Waypoint Information:\n\n"
                "      Waypoint %d of %d, Radius: %d\n"
                "      Flags: %s\n", m_facingAtIndex, g_numWaypoints, m_paths[m_facingAtIndex].radius, GetWaypointInfo(m_facingAtIndex));
        }

        // draw entire message
        MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, nullptr, g_hostEntity);
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
    else if (m_pathDisplayTime + 2.0f > engine->GetTime()) // what???
        m_pathDisplayTime = 0.0f;
}

bool Waypoint::IsConnected(const int index)
{
    int i;
    int j;
    for (i = 0; i < g_numWaypoints; i++)
    {
        if (i != index)
        {
            for (j = 0; j < Const_MaxPathIndex; j++)
            {
                if (m_paths[i].index[j] == index)
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
    int i, j, k;

    bool haveError = false;

    for (i = 0; i < g_numWaypoints; i++)
    {
        connections = 0;

        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (m_paths[i].index[j] != -1)
            {
                if (m_paths[i].index[j] > g_numWaypoints)
                {
                    AddLogEntry(Log::Warning, "Waypoint %d connected with invalid Waypoint #%d!", i, m_paths[i].index[j]);
                    (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i].origin);
                    haveError = true;
                }

                connections++;
                break;
            }
        }

        if (!connections)
        {
            if (!IsConnected(i))
            {
                AddLogEntry(Log::Warning, "Waypoint %d isn't connected with any other Waypoint!", i);
                (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i].origin);
                haveError = true;
            }
        }

        if (GetGameMode() == GameMode::Original)
        {
            if (m_paths[i].flags & WAYPOINT_TERRORIST)
                terrPoints++;
            else if (m_paths[i].flags & WAYPOINT_COUNTER)
                ctPoints++;
            else if (m_paths[i].flags & WAYPOINT_GOAL)
                goalPoints++;
            else if (m_paths[i].flags & WAYPOINT_RESCUE)
                rescuePoints++;
        }

        for (k = 0; k < Const_MaxPathIndex; k++)
        {
            if (m_paths[i].index[k] != -1)
            {
                if (m_paths[i].index[k] >= g_numWaypoints || m_paths[i].index[k] < -1)
                {
                    AddLogEntry(Log::Warning, "Waypoint %d - Pathindex %d out of Range!", i, k);
                    (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i].origin);

                    g_waypointOn = true;
                    g_editNoclip = true;

                    haveError = true;
                }
                else if (m_paths[i].index[k] == i)
                {
                    AddLogEntry(Log::Warning, "Waypoint %d - Pathindex %d points to itself!", i, k);
                    (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i].origin);

                    g_waypointOn = true;
                    g_editNoclip = true;

                    haveError = true;
                }
            }
        }
    }

    if (g_mapType == MAP_CS && GetGameMode() == GameMode::Original)
    {
        if (!rescuePoints)
        {
            AddLogEntry(Log::Warning, "You didn't set a Rescue Point!");
            haveError = true;
        }
    }

    if (!terrPoints && GetGameMode() == GameMode::Original)
    {
        AddLogEntry(Log::Warning, "You didn't set any Terrorist Important Point!");
        haveError = true;
    }
    else if (!ctPoints && GetGameMode() == GameMode::Original)
    {
        AddLogEntry(Log::Warning, "You didn't set any CT Important Point!");
        haveError = true;
    }
    else if (!goalPoints && GetGameMode() == GameMode::Original)
    {
        AddLogEntry(Log::Warning, "You didn't set any Goal Point!");
        haveError = true;
    }

    CenterPrint("Waypoints are saved!");
    return haveError ? false : true;
}

float Waypoint::GetPathDistance(const int srcIndex, const int destIndex)
{
    if (srcIndex == -1 || destIndex == -1)
        return FLT_MAX;

    return (m_paths[srcIndex].origin - m_paths[destIndex].origin).GetLengthSquared();
}

// this function creates basic waypoint types on map - raeyid was here :)
void Waypoint::CreateBasic(void)
{
    edict_t* ent = nullptr;

    // first of all, if map contains ladder points, create it
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_ladder")))
    {
        Vector ladderLeft = ent->v.absmin;
        Vector ladderRight = ent->v.absmax;
        ladderLeft.z = ladderRight.z;

        TraceResult tr{};
        Vector up, down, front, back;

        const Vector diff = ((ladderLeft - ladderRight) ^ Vector(0.0f, 0.0f, 1.0f)).Normalize() * 15.0f;
        front = back = GetEntityOrigin(ent);

        front = front + diff; // front
        back = back - diff; // back

        up = down = front;
        down.z = ent->v.absmax.z;

        TraceHull(down, up, true, point_hull, g_hostEntity, &tr);

        if (tr.flFraction != 1.0f || POINT_CONTENTS(up) == CONTENTS_SOLID)
        {
            up = down = back;
            down.z = ent->v.absmax.z;
        }

        TraceHull(down, up - Vector(0.0f, 0.0f, 1000.0f), true, point_hull, g_hostEntity, &tr);
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
        const Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);
        if (FindNearest(origin, 50.0f) == -1)
            Add(0, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // then add ct spawnpoints
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_player_start")))
    {
        const Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);
        if (FindNearest(origin, 50.0f) == -1)
            Add(0, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // then vip spawnpoint
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_vip_start")))
    {
        const Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);
        if (FindNearest(origin, 50.0f) == -1)
            Add(0, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // hostage rescue zone
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_hostage_rescue")))
    {
        const Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);
        if (FindNearest(origin, 50.0f) == -1)
            Add(4, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // hostage rescue zone (same as above)
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_hostage_rescue")))
    {
        const Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);
        if (FindNearest(origin, 50.0f) == -1)
            Add(4, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // bombspot zone
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_bomb_target")))
    {
        const Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);
        if (FindNearest(origin, 50.0f) == -1)
            Add(100, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // bombspot zone (same as above)
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_bomb_target")))
    {
        const Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);
        if (FindNearest(origin, 50.0f) == -1)
            Add(100, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // hostage entities
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "hostage_entity")))
    {
        // if already saved || moving skip it
        if (ent->v.effects & EF_NODRAW || ent->v.speed > 0.0f)
            continue;

        const Vector origin = GetWalkablePosition(GetEntityOrigin(ent));
        if (FindNearest(origin, 50.0f) == -1)
            Add(100, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // vip rescue (safety) zone
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_vip_safetyzone")))
    {
        const Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);
        if (FindNearest(origin, 50.0f) == -1)
            Add(100, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // terrorist escape zone
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_escapezone")))
    {
        const Vector origin = GetWalkablePosition(GetEntityOrigin(ent), ent);
        if (FindNearest(origin, 50.0f) == -1)
            Add(100, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }

    // weapons on the map?
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "armoury_entity")))
    {
        const Vector origin = GetWalkablePosition(GetEntityOrigin(ent));
        if (FindNearest(origin, 50.0f) == -1)
            Add(0, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }
}

Path* Waypoint::GetPath(const int id)
{
    // avoid crash
    if (!IsValidWaypoint(id))
        return &m_paths[crandomint(0, g_numWaypoints - 1)];

    return &m_paths[id];
}

// this function stores the bomb position as a vector
void Waypoint::SetBombPosition(const bool shouldReset)
{
    if (shouldReset)
    {
        m_foundBombOrigin = nullvec;
        g_bombPlanted = false;
        return;
    }

    edict_t* ent = nullptr;
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "grenade")))
    {
        if (cstrcmp(STRING(ent->v.model) + 9, "c4.mdl") == 0)
        {
            m_foundBombOrigin = GetEntityOrigin(ent);
            break;
        }
    }
}

void Waypoint::SetLearnJumpWaypoint(const int mod)
{
    if (mod == -1)
        m_learnJumpWaypoint = (m_learnJumpWaypoint ? false : true);
    else
        m_learnJumpWaypoint = (mod == 1 ? true : false);
}

void Waypoint::SetFindIndex(const int index)
{
    if (IsValidWaypoint(index))
    {
        m_findWPIndex = index;
        ServerPrint("Showing Direction to Waypoint #%d", m_findWPIndex);
    }
}

Waypoint::Waypoint(void)
{
    m_endJumpPoint = false;
    m_learnJumpWaypoint = false;
    m_timeJumpStarted = 0.0f;

    m_learnVelocity = nullvec;
    m_learnPosition = nullvec;
    m_cacheWaypointIndex = -1;
    m_lastJumpWaypoint = -1;
    m_findWPIndex = -1;

    m_lastDeclineWaypoint = -1;

    m_lastWaypoint = nullvec;
    m_isOnLadder = false;

    m_pathDisplayTime = 0.0f;
    m_arrowDisplayTime = 0.0f;

    m_terrorPoints.Destroy();
    m_ctPoints.Destroy();
    m_goalPoints.Destroy();
    m_campPoints.Destroy();
    m_rescuePoints.Destroy();
    safedel(m_waypointDisplayTime);
}

Waypoint::~Waypoint(void)
{
    m_pathDisplayTime = 0.0f;
    m_arrowDisplayTime = 0.0f;

    m_terrorPoints.Destroy();
    m_ctPoints.Destroy();
    m_goalPoints.Destroy();
    m_campPoints.Destroy();
    m_rescuePoints.Destroy();
    safedel(m_waypointDisplayTime);
}