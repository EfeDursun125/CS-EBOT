#include <core.h>
#include <thread>

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
ConVar ebot_download_waypoints_from("ebot_download_waypoints_from", "https://github.com/EfeDursun125/EBOT-WP/raw/main");
ConVar ebot_waypoint_size("ebot_waypoint_size", "7");
ConVar ebot_waypoint_r("ebot_waypoint_r", "0");
ConVar ebot_waypoint_g("ebot_waypoint_g", "255");
ConVar ebot_waypoint_b("ebot_waypoint_b", "0");

// this function initialize the waypoint structures..
void Waypoint::Initialize(void)
{
    // have any waypoint path nodes been allocated yet?
    if (m_waypointPaths)
    {
        for (int i = 0; i < g_numWaypoints; i++)
        {
            if (m_paths[i] == nullptr)
                continue;

            delete m_paths[i];
            m_paths[i] = nullptr;
        }
    }

    g_numWaypoints = 0;
    m_lastWaypoint = nullvec;
}

void CreateWaypoint(Vector WayVec, Vector Next, float range, const float goalDist)
{
    Next.z += 19.0f;
    TraceResult tr{};
    TraceHull(Next, Next, NO_BOTH, HULL_HEAD, g_hostEntity, &tr);
    Next.z -= 19.0f;

    range *= 0.75f;
    if (tr.flFraction == 1.0f || IsBreakable(tr.pHit))
    {
        const int startindex = g_waypoint->FindNearestInCircle(tr.vecEndPos, range);

        if (!IsValidWaypoint(startindex))
        {
            TraceResult tr2{};
            TraceHull(tr.vecEndPos, Vector(tr.vecEndPos.x, tr.vecEndPos.y, (tr.vecEndPos.z - 800.0f)), NO_BOTH, HULL_HEAD, g_hostEntity, &tr2);

            if (tr2.flFraction != 1.0f)
            {
                bool isBreakable = IsBreakable(tr.pHit);
                Vector TargetPosition = tr2.vecEndPos;
                TargetPosition.z += 19.0f;// 36.0f;

                const int endindex = g_waypoint->FindNearestInCircle(TargetPosition, range);

                if (!IsValidWaypoint(endindex))
                {
                    const int doublecheckindex = g_waypoint->FindNearestInCircle(TargetPosition, range);
                    if (!IsValidWaypoint(doublecheckindex))
                    {
                        const Vector targetOrigin = g_waypoint->GetPath(doublecheckindex)->origin;

                        g_analyzeputrequirescrouch = false;

                        TraceResult upcheck;
                        Vector TargetPosition2 = Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z + 36.0f));
                        TraceLine(TargetPosition, TargetPosition2, true, false, g_hostEntity, &upcheck);

                        if (upcheck.flFraction != 1.0f)
                            g_analyzeputrequirescrouch = true;

                        if ((g_waypoint->IsNodeReachable(targetOrigin, g_analyzeputrequirescrouch ? Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z - 18.0f)) : TargetPosition) &&
                            g_waypoint->IsNodeReachable(g_analyzeputrequirescrouch ? Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z - 18.0f)) : TargetPosition, targetOrigin))
                            || (g_waypoint->IsNodeReachableWithJump(targetOrigin, g_analyzeputrequirescrouch ? Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z - 18.0f)) : TargetPosition, -1) && g_waypoint->IsNodeReachableWithJump(targetOrigin, g_analyzeputrequirescrouch ? Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z - 18.0f)) : TargetPosition, -1)))
                            g_waypoint->Add(isBreakable ? 1 : -1, g_analyzeputrequirescrouch ? Vector(TargetPosition.x, TargetPosition.y, (TargetPosition.z - 17.5f)) : TargetPosition);
                    }
                }
            }
        }
    }
    else if (!FNullEnt(g_hostEntity))
        engine->DrawLine(g_hostEntity, Next - Vector(0.0f, 0.0f, 35.0f), Next + Vector(0.0f, 0.0f, 35.0f), Color(255, 0, 0, 255), 7, 0, 0, 10);
}

void AnalyzeThread(void)
{
    const float goalDist = SquaredF(ebot_analyze_goal_check_distance.GetFloat());

    if (!FNullEnt(g_hostEntity))
    {
        char message[] =
            "+-----------------------------------------------+\n"
            " Analyzing the map for walkable places \n"
            "+-----------------------------------------------+\n";

        HudMessage(g_hostEntity, true, Color(100, 100, 255), message);
    }

    const float time = engine->GetTime();
    if (!IsDedicatedServer() && time < 5.0f) // let the player join first...
        return;

    static float magicTimer;
    for (int i = 0; i < g_numWaypoints; i++)
    {
        if (g_expanded[i])
            continue;

        if (magicTimer > time)
            continue;

        if ((ebot_analyzer_min_fps.GetFloat() + g_pGlobals->frametime) < 1.0f / g_pGlobals->frametime)
            magicTimer = AddTime(g_pGlobals->frametime * 0.066f); // pause

        const Vector WayVec = g_waypoint->GetPath(i)->origin;
        const float range = ebot_analyze_distance.GetFloat();

        for (int dir = 1; dir < 8; dir++)
        {
            switch (dir)
            {
            case 1:
            {
                Vector Next;
                Next.x = WayVec.x + range;
                Next.y = WayVec.y;
                Next.z = WayVec.z;

                CreateWaypoint(WayVec, Next, range, goalDist);
            }
            case 2:
            {
                Vector Next;
                Next.x = WayVec.x - range;
                Next.y = WayVec.y;
                Next.z = WayVec.z;

                CreateWaypoint(WayVec, Next, range, goalDist);
            }
            case 3:
            {
                Vector Next;
                Next.x = WayVec.x;
                Next.y = WayVec.y + range;
                Next.z = WayVec.z;

                CreateWaypoint(WayVec, Next, range, goalDist);
            }
            case 4:
            {
                Vector Next;
                Next.x = WayVec.x;
                Next.y = WayVec.y - range;
                Next.z = WayVec.z;

                CreateWaypoint(WayVec, Next, range, goalDist);
            }
            case 5:
            {
                Vector Next;
                Next.x = WayVec.x + range;
                Next.y = WayVec.y;
                Next.z = WayVec.z + 128.0f;

                CreateWaypoint(WayVec, Next, range, goalDist);
            }
            case 6:
            {
                Vector Next;
                Next.x = WayVec.x - range;
                Next.y = WayVec.y;
                Next.z = WayVec.z + 128.0f;

                CreateWaypoint(WayVec, Next, range, goalDist);
            }
            case 7:
            {
                Vector Next;
                Next.x = WayVec.x;
                Next.y = WayVec.y + range;
                Next.z = WayVec.z + 128.0f;

                CreateWaypoint(WayVec, Next, range, goalDist);
            }
            case 8:
            {
                Vector Next;
                Next.x = WayVec.x;
                Next.y = WayVec.y - range;
                Next.z = WayVec.z + 128.0f;

                CreateWaypoint(WayVec, Next, range, goalDist);
            }
            }
        }

        g_expanded[i] = true;
    }

    if (magicTimer + 2.0f < time)
    {
        g_analyzewaypoints = false;
        g_waypointOn = false;
        g_waypoint->AnalyzeDeleteUselessWaypoints();
        g_waypoint->AddGoals();
        g_waypoint->Save();
        g_waypoint->Load();
        ServerCommand("exec addons/ebot/ebot.cfg");
    }
}

void Waypoint::AddGoals(void)
{
    for (int i = 0; i < g_numWaypoints; i++)
    {
        const Vector targetOrigin = m_paths[i]->origin;
        if (g_mapType & MAP_DE)
        {
            edict_t* ent = nullptr;
            while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_bomb_target")))
            {
                const Vector entOrigin = GetEntityOrigin(ent);
                TraceResult tr{};
                TraceHull(targetOrigin, entOrigin, true, false, g_hostEntity, &tr);
                if (tr.flFraction == 1.0f)
                    m_paths[i]->flags |= WAYPOINT_GOAL;
            }

            while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_bomb_target")))
            {
                const Vector entOrigin = GetEntityOrigin(ent);
                TraceResult tr{};
                TraceHull(targetOrigin, entOrigin, true, false, g_hostEntity, &tr);
                if (tr.flFraction == 1.0f)
                    m_paths[i]->flags |= WAYPOINT_GOAL;
            }
        }
        else if (g_mapType & MAP_CS)
        {
            edict_t* ent = nullptr;
            while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "hostage_entity")))
            {
                // if already saved || moving skip it
                if ((ent->v.effects & EF_NODRAW) && (ent->v.speed > 0))
                    continue;

                const Vector entOrigin = GetEntityOrigin(ent);
                TraceResult tr{};
                TraceHull(targetOrigin, entOrigin, true, false, g_hostEntity, &tr);
                if (tr.flFraction == 1.0f)
                    m_paths[i]->flags |= WAYPOINT_GOAL;
            }
        }

        /*int connections = 0;
        for (int C = 0; C < Const_MaxPathIndex; C++)
        {
            if (m_paths[i]->index[C] != -1)
                connections++;

            if (connections > 0 && connections < 3)
            {
                if (m_paths[i]->flags & WAYPOINT_CROUCH)
                    m_paths[i]->flags |= WAYPOINT_ZMHMCAMP;
                else if (connections == 1)
                    m_paths[i]->flags |= WAYPOINT_ZMHMCAMP;
            }
        }*/
    }
}

void Waypoint::Analyze(void)
{
    if (g_numWaypoints < 1)
        return;

    AnalyzeThread();
}

void Waypoint::AnalyzeDeleteUselessWaypoints(void)
{
    int i, j;

    for (i = 0; i < g_numWaypoints; i++)
    {
        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            const int index = m_paths[i]->index[j];
            if (index != -1)
            {
                if (m_paths[i]->connectionFlags[j] & PATHFLAG_JUMP)
                    continue;

                if (!(m_paths[i]->flags & WAYPOINT_CROUCH) && m_paths[i]->origin.z != m_paths[index]->origin.z)
                    continue;

                TraceResult tr{};
                TraceHull(m_paths[i]->origin, m_paths[index]->origin, NO_BOTH, HULL_HEAD, g_hostEntity, &tr);
                if (tr.flFraction != 1.0f)
                    DeletePathByIndex(i, index);
            }
        }
    }

    for (i = 0; i < g_numWaypoints; i++)
    {
        int connections = 0;

        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (m_paths[i]->index[j] != -1)
            {
                if (m_paths[i]->index[j] > g_numWaypoints)
                    DeleteByIndex(i);

                connections++;
            }
        }

        if (connections == 0)
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

        if (!IsConnected(i))
            DeleteByIndex(i);
    }

    CenterPrint("Waypoints are saved!");
}

void Waypoint::AddPath(const int addIndex, const int pathIndex, const int type)
{
    if (!IsValidWaypoint(addIndex) || !IsValidWaypoint(pathIndex) || addIndex == pathIndex)
        return;

    Path* path = m_paths[addIndex];

    // don't allow paths get connected twice
    for (int i = 0; i < Const_MaxPathIndex; i++)
    {
        if (path->index[i] == pathIndex)
            return;
    }

    // check for free space in the connection indices
    for (int i = 0; i < Const_MaxPathIndex; i++)
    {
        if (path->index[i] == -1)
        {
            path->index[i] = static_cast<int16>(pathIndex);

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
    float maxDistance = FLT_MAX;
    int slotID = -1;

    for (int i = 0; i < Const_MaxPathIndex; i++)
    {
        const float distance = (path->origin - m_paths[path->index[i]]->origin).GetLengthSquared();
        if (distance > maxDistance)
        {
            maxDistance = distance;
            slotID = i;
        }
    }

    if (slotID != -1)
    {
        path->index[slotID] = static_cast<int16>(pathIndex);

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

// find the farest node to that origin, and return the index to this node
void Waypoint::FindFarestThread(const Vector origin, const float maxDistance, int& index)
{
    float squaredDistance = SquaredF(maxDistance);
    for (int i = 0; i < g_numWaypoints; i++)
    {
        const float distance = (m_paths[i]->origin - origin).GetLengthSquared();
        if (distance > squaredDistance)
        {
            index = i;
            squaredDistance = distance;
        }
    }
}

int Waypoint::FindFarest(const Vector origin, const float maxDistance)
{
    int index = -1;
    thread core(&Waypoint::FindFarestThread, this, origin, maxDistance, ref(index));
    core.join();
    return index;
}

// find the farest node to that origin, and return the index to this node
void Waypoint::FindNearestInCircleThread(const Vector origin, const float maxDistance, int& index)
{
    float maxDist = SquaredF(maxDistance);
    for (int i = 0; i < g_numWaypoints; i++)
    {
        const float distance = (m_paths[i]->origin - origin).GetLengthSquared();
        if (distance < maxDist)
        {
            index = i;
            maxDist = distance;
        }
    }
}

int Waypoint::FindNearestInCircle(const Vector origin, const float maxDistance)
{
    int index = -1;
    thread core(&Waypoint::FindNearestInCircleThread, this, origin, maxDistance, ref(index));
    core.join();
    return index;
}

int Waypoint::FindNearest(Vector origin, float minDistance, int flags, edict_t* entity, int* findWaypointPoint, int mode)
{
    float squaredMinDistance = SquaredF(minDistance);
    const int checkPoint = 20;
    float wpDistance[checkPoint];
    int wpIndex[checkPoint];

    for (int i = 0; i < checkPoint; i++)
    {
        wpIndex[i] = -1;
        wpDistance[i] = FLT_MAX;
    }

    for (int i = 0; i < g_numWaypoints; i++)
    {
        if (!IsValidWaypoint(i) || !m_paths[i])
            continue;

        if (flags != -1 && !(m_paths[i]->flags & flags))
            continue;

        float distance = (m_paths[i]->origin - origin).GetLengthSquared();
        if (distance > squaredMinDistance)
            continue;

        const Vector dest = m_paths[i]->origin;
        const float distance2D = (dest - origin).GetLengthSquared2D();
        if (((dest.z > origin.z + 62.0f || dest.z < origin.z - 100.0f) && !(m_paths[i]->flags & WAYPOINT_LADDER)) && distance2D < SquaredF(30.0f))
            continue;

        for (int y = 0; y < checkPoint; y++)
        {
            if (distance > wpDistance[y])
                continue;

            for (int z = checkPoint - 1; z > y; z--)
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

    // move target improve
    if (IsValidWaypoint(mode))
    {
        int cdWPIndex[checkPoint];
        float cdWPDistance[checkPoint];
        for (int i = 0; i < checkPoint; i++)
        {
            cdWPIndex[i] = -1;
            cdWPDistance[i] = FLT_MAX;
        }

        for (int i = 0; i < checkPoint; i++)
        {
            if (!IsValidWaypoint(wpIndex[i]))
                continue;

            const float distance = g_waypoint->GetPathDistance(wpIndex[i], mode);
            for (int y = 0; y < checkPoint; y++)
            {
                if (distance > cdWPDistance[y])
                    continue;

                for (int z = checkPoint - 1; z >= y; z--)
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
            if (!IsValidWaypoint(wpIndex[i]))
                continue;

            const Path* path = m_paths[wpIndex[i]];
            if (path == nullptr)
                continue;

            // use the path variable in the condition     
            if (wpDistance[i] > SquaredF(path->radius) && !Reachable(entity, wpIndex[i]))
                continue;

            if (findWaypointPoint == (int*)-2)
                return wpIndex[i];

            if (firsIndex == -1)
            {
                firsIndex = wpIndex[i];
                continue;
            }

            if (findWaypointPoint && findWaypointPoint != (int*)-2)
                *findWaypointPoint = wpIndex[i];

            return firsIndex;
        }
    }

    if (!IsValidWaypoint(firsIndex))
        firsIndex = wpIndex[0];

    return firsIndex;
}

// returns all waypoints within radius from position
void Waypoint::FindInRadius(const Vector origin, const float radius, int* holdTab, int* count)
{
    int maxCount = *count;
    *count = 0;
    const float squared = SquaredF(radius);

    for (int i = 0; i < g_numWaypoints; i++)
    {
        const Path* path = m_paths[i];
        if (path == nullptr)
            continue;

        if ((path->origin - origin).GetLengthSquared() < squared)
        {
            *holdTab++ = i;
            *count += 1;

            if (*count >= maxCount)
                break;
        }
    }

    *count -= 1;
}

void Waypoint::FindInRadius(Array <int>& queueID, const float radius, const Vector origin)
{
    const float squared = SquaredF(radius);
    for (int i = 0; i < g_numWaypoints; i++)
    {
        const Path* path = m_paths[i];
        if (path == nullptr)
            continue;

        if ((path->origin - origin).GetLengthSquared() < squared)
            queueID.Push(i);
    }
}

void Waypoint::Add(const int flags, const Vector waypointOrigin)
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

    if (g_botManager->GetBotsNum() > 0)
        g_botManager->RemoveAll();

    g_waypointsChanged = true;

    switch (flags)
    {
    case 9:
        newOrigin = m_learnPosition;
        break;
    case 10:
        index = FindNearest(GetEntityOrigin(g_hostEntity), 25.0f);
        if (IsValidWaypoint(index))
        {
            if ((m_paths[index]->origin - GetEntityOrigin(g_hostEntity)).GetLengthSquared() <= SquaredF(25.0f))
            {
                placeNew = false;
                path = m_paths[index];

                int accumFlags = 0;

                for (i = 0; i < Const_MaxPathIndex; i++)
                    accumFlags += path->connectionFlags[i];

                if (accumFlags == 0)
                    path->origin = (path->origin + GetEntityOrigin(g_hostEntity)) * 0.5f;
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
        if (m_paths[index] == nullptr)
        {
            AddLogEntry(Log::Memory, "unexpected memory error");
            return;
        }

        path = m_paths[index];

        // increment total number of waypoints
        g_numWaypoints++;

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
    m_waypointDisplayTime[index] = 0;

    if (flags == 9)
        m_lastJumpWaypoint = index;
    else if (flags == 10)
    {
        AddPath(m_lastJumpWaypoint, index);

        for (i = 0; i < Const_MaxPathIndex; i++)
        {
            if (m_paths[m_lastJumpWaypoint]->index[i] == index)
            {
                m_paths[m_lastJumpWaypoint]->connectionFlags[i] |= PATHFLAG_JUMP;
                break;
            }
        }

        CalculateWayzone(index);

        return;
    }

    // disable autocheck if we're analyzing
    if ((!FNullEnt(g_hostEntity) && g_hostEntity->v.flags & FL_DUCKING && g_analyzewaypoints == false) || g_analyzeputrequirescrouch == true)
        path->flags |= WAYPOINT_CROUCH;  // set a crouch waypoint

    if (!FNullEnt(g_hostEntity) && g_hostEntity->v.movetype == MOVETYPE_FLY && g_analyzewaypoints == false)
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
        int destIndex = -1;

        TraceResult tr{};

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

                if (tr.flFraction == 1.0f && cabsf(newOrigin.x - m_paths[i]->origin.x) < 64 && cabsf(newOrigin.y - m_paths[i]->origin.y) < 64 && cabsf(newOrigin.z - m_paths[i]->origin.z) < g_autoPathDistance)
                {
                    AddPath(index, i);
                    AddPath(i, index);
                }
            }
            else
            {
                const float distance = (m_paths[i]->origin - newOrigin).GetLengthSquared();
                if (distance < minDistance)
                {
                    destIndex = i;
                    minDistance = distance;
                }

                if (IsNodeReachable(newOrigin, m_paths[destIndex]->origin))
                    AddPath(index, destIndex);
            }
        }

        if (IsValidWaypoint(destIndex))
        {
            if (g_analyzewaypoints == true)
            {
                AddPath(index, destIndex);
                AddPath(destIndex, index);
            }
            else
            {
                // check if the waypoint is reachable from the new one (one-way)
                if (IsNodeReachable(newOrigin, m_paths[destIndex]->origin))
                    AddPath(index, destIndex);

                // check if the new one is reachable from the waypoint (other way)
                if (IsNodeReachable(m_paths[destIndex]->origin, newOrigin))
                    AddPath(destIndex, index);
            }
        }
    }
    else
    {
        const float addDist = SquaredF(ebot_analyze_distance.GetFloat() * 1.9f);

        // calculate all the paths to this new waypoint
        for (i = 0; i < g_numWaypoints; i++)
        {
            if (i == index)
                continue; // skip the waypoint that was just added

            if (g_analyzewaypoints == true) // if we're analyzing, be careful (we dont want path errors)
            {
                const float pathDist = (m_paths[i]->origin - newOrigin).GetLengthSquared2D();
                if (m_paths[i]->flags& WAYPOINT_LADDER && (IsNodeReachable(newOrigin, m_paths[i]->origin) || IsNodeReachableWithJump(newOrigin, m_paths[i]->origin, 0)) && pathDist <= addDist)
                {
                    AddPath(index, i);
                    AddPath(i, index);
                }

                if (!IsNodeReachable(newOrigin, m_paths[i]->origin) && IsNodeReachableWithJump(newOrigin, m_paths[i]->origin, m_paths[i]->flags) && pathDist <= addDist)
                    AddPath(index, i, 1);

                if (!IsNodeReachable(m_paths[i]->origin, newOrigin) && IsNodeReachableWithJump(m_paths[i]->origin, newOrigin, m_paths[index]->flags) && pathDist <= addDist)
                    AddPath(i, index, 1);

                if (IsNodeReachable(newOrigin, m_paths[i]->origin) && IsNodeReachable(m_paths[i]->origin, newOrigin) && pathDist <= addDist)
                {
                    AddPath(index, i);
                    AddPath(i, index);
                }
                else if (ebot_analyze_disable_fall_connections.GetInt() == 0)
                {
                    if (IsNodeReachable(newOrigin, m_paths[i]->origin) && newOrigin.z > m_paths[i]->origin.z && pathDist <= addDist)
                        AddPath(index, i);

                    if (IsNodeReachable(m_paths[i]->origin, newOrigin) && newOrigin.z < m_paths[i]->origin.z && pathDist <= addDist)
                        AddPath(i, index);
                }
            }
            else
            {
                // check if the waypoint is reachable from the new one (one-way)
                if (IsNodeReachable(newOrigin, m_paths[i]->origin))
                    AddPath(index, i);

                // check if the new one is reachable from the waypoint (other way)
                if (IsNodeReachable(m_paths[i]->origin, newOrigin))
                    AddPath(i, index);
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

    const int index = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);
    if (!IsValidWaypoint(index))
        return;

    Path* path = nullptr;
    InternalAssert(m_paths[index] != nullptr);
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
            }
        }
    }

    for (i = 0; i < g_numWaypoints; i++)
    {
        path = m_paths[i];

        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (path->index[j] > index)
                path->index[j]--;
        }
    }

    // free deleted node
    delete m_paths[index];
    m_paths[index] = nullptr;

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

    Path* path = nullptr;
    InternalAssert(m_paths[index] != nullptr);

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
            }
        }
    }

    for (i = 0; i < g_numWaypoints; i++)
    {
        path = m_paths[i];

        for (j = 0; j < Const_MaxPathIndex; j++)
        {
            if (path->index[j] > index)
                path->index[j]--;
        }
    }

    // free deleted node
    delete m_paths[index];
    m_paths[index] = nullptr;

    // Rotate Path Array down
    for (i = index; i < g_numWaypoints - 1; i++)
        m_paths[i] = m_paths[i + 1];

    g_numWaypoints--;
    m_waypointDisplayTime[index] = 0;

    PlaySound(g_hostEntity, "weapons/mine_activate.wav");
}

void Waypoint::DeleteFlags(void)
{
    const int index = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);
    if (IsValidWaypoint(index))
    {
        m_paths[index]->flags = 0;
        PlaySound(g_hostEntity, "common/wpn_hudon.wav");
    }
}

// this function allow manually changing flags
void Waypoint::ToggleFlags(int toggleFlag)
{
    const int index = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);
    if (IsValidWaypoint(index))
    {
        if (m_paths[index]->flags & toggleFlag)
            m_paths[index]->flags &= ~toggleFlag;

        else if (!(m_paths[index]->flags & toggleFlag))
        {
            if (toggleFlag == WAYPOINT_SNIPER && !(m_paths[index]->flags & WAYPOINT_CAMP))
            {
                AddLogEntry(Log::Error, "Cannot assign sniper flag to waypoint #%d. This is not camp waypoint", index);
                return;
            }

            m_paths[index]->flags |= toggleFlag;
        }

        // play "done" sound...
        PlaySound(g_hostEntity, "common/wpn_hudon.wav");
    }
}

// this function allow manually setting the zone radius
void Waypoint::SetRadius(const int16 radius)
{
    if (radius < 0)
        return;

    const int index = FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);
    if (IsValidWaypoint(index))
    {
        m_paths[index]->radius = radius;
        PlaySound(g_hostEntity, "common/wpn_hudon.wav");
    }
}

// this function checks if waypoint A has a connection to waypoint B
bool Waypoint::IsConnected(const int pointA, const int pointB)
{
    for (int i = 0; i < Const_MaxPathIndex; i++)
    {
        if (m_paths[pointA]->index[i] == pointB)
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
    auto nearestNode = FindNearest(g_hostEntity->v.origin, 54.0f);

    // check bounds from eyes of editor
    const auto& editorEyes = g_hostEntity->v.origin + g_hostEntity->v.view_ofs;

    for (int i = 0; i < g_numWaypoints; i++)
    {
        auto path = m_paths[i];
        if (path == nullptr)
            continue;

        // skip nearest waypoint to editor, since this used mostly for adding / removing paths
        if (nearestNode == i)
            continue;

        Vector to = path->origin - g_hostEntity->v.origin;
        Vector angles = (to.ToAngles() - g_hostEntity->v.v_angle);
        angles.ClampAngles();

        // skip the waypoints that are too far away from us, and we're not looking at them directly
        if (to.GetLengthSquared() > SquaredF(500.0f) || cabsf(angles.y) > range)
            continue;

        // check if visible, (we're not using visiblity tables here, as they not valid at time of waypoint editing)
        TraceResult tr{};
        TraceLine(editorEyes, path->origin, false, false, g_hostEntity, &tr);

        if (tr.flFraction != 1.0f)
            continue;

        const float bestAngle = angles.y;

        angles = -g_hostEntity->v.v_angle;
        angles.x = -angles.x;
        angles += ((path->origin - Vector(0.0f, 0.0f, (path->flags & WAYPOINT_CROUCH) ? 17.0f : 34.0f)) - editorEyes).ToAngles();
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
    if (m_facingAtIndex != -1)
        (*g_engfuncs.pfnSetOrigin) (g_hostEntity, g_waypoint->m_paths[m_facingAtIndex]->origin);
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
        if (m_paths[nodeFrom]->index[index] == nodeTo)
        {
            g_waypointsChanged = true;

            m_paths[nodeFrom]->index[index] = -1; // unassign this path
            m_paths[nodeFrom]->connectionFlags[index] = 0;

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
        if (m_paths[nodeFrom]->index[index] == nodeTo)
        {
            g_waypointsChanged = true;

            m_paths[nodeFrom]->index[index] = -1; // unassign this path
            m_paths[nodeFrom]->connectionFlags[index] = 0;

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
void Waypoint::CalculateWayzone(int index)
{
    Path* path = m_paths[index];
    Vector start, direction;

    TraceResult tr{};
    bool wayBlocked = false;

    if ((path->flags & (WAYPOINT_LADDER | WAYPOINT_GOAL | WAYPOINT_CAMP | WAYPOINT_RESCUE | WAYPOINT_CROUCH)) || m_learnJumpWaypoint)
    {
        path->radius = 4;
        return;
    }

    for (int i = 0; i < Const_MaxPathIndex; i++)
    {
        if (path->index[i] != -1 && (m_paths[path->index[i]]->flags & WAYPOINT_LADDER))
        {
            path->radius = 4;
            return;
        }
    }

    for (float scanDistance = 16.0f; scanDistance < 160.0f; scanDistance += 16.0f)
    {
        start = path->origin;
        MakeVectors(nullvec);

        direction = g_pGlobals->v_forward * scanDistance;
        direction = direction.ToAngles();

        path->radius = static_cast<int16>(scanDistance);

        for (float circleRadius = 0.0f; circleRadius < 180.0f; circleRadius += 5.0f)
        {
            MakeVectors(direction);

            Vector radiusStart = start - g_pGlobals->v_forward * scanDistance;
            Vector radiusEnd = start + g_pGlobals->v_forward * scanDistance;

            TraceHull(radiusStart, radiusEnd, true, head_hull, g_hostEntity, &tr);

            if (tr.flFraction < 1.0f)
            {
                TraceLine(radiusStart, radiusEnd, true, g_hostEntity, &tr);

                if (!FNullEnt(tr.pHit) && (FClassnameIs(tr.pHit, "func_door") || FClassnameIs(tr.pHit, "func_door_rotating")))
                {
                    path->radius = 4;
                    wayBlocked = true;

                    break;
                }

                wayBlocked = true;
                path->radius -= 16;

                break;
            }

            Vector dropStart = start + (g_pGlobals->v_forward * scanDistance);
            Vector dropEnd = dropStart - Vector(0.0f, 0.0f, scanDistance + 60.0f);

            TraceHull(dropStart, dropEnd, true, head_hull, g_hostEntity, &tr);

            if (tr.flFraction >= 1.0f)
            {
                wayBlocked = true;
                path->radius -= 16;

                break;
            }

            dropStart = start - (g_pGlobals->v_forward * scanDistance);
            dropEnd = dropStart - Vector(0.0f, 0.0f, scanDistance + 60.0f);

            TraceHull(dropStart, dropEnd, true, head_hull, g_hostEntity, &tr);

            if (tr.flFraction >= 1.0f)
            {
                wayBlocked = true;
                path->radius -= 16;
                break;
            }

            radiusEnd.z += 34.0f;
            TraceHull(radiusStart, radiusEnd, true, head_hull, g_hostEntity, &tr);

            if (tr.flFraction < 1.0f)
            {
                wayBlocked = true;
                path->radius -= 16;
                break;
            }

            direction.y = AngleNormalize(direction.y + circleRadius);
        }
        if (wayBlocked)
            break;
    }

    path->radius -= 16;

    if (path->radius < 4)
        path->radius = 4;
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

void Waypoint::InitTypes()
{
    m_terrorPoints.RemoveAll();
    m_ctPoints.RemoveAll();
    m_goalPoints.RemoveAll();
    m_campPoints.RemoveAll();
    m_rescuePoints.RemoveAll();
    m_sniperPoints.RemoveAll();
    m_zmHmPoints.RemoveAll();
    m_hmMeshPoints.RemoveAll();
    m_otherPoints.RemoveAll();

    for (int i = 0; i < g_numWaypoints; i++)
    {
        if (m_paths[i]->flags & WAYPOINT_GOAL)
            m_goalPoints.Push(i);
        else if (m_paths[i]->flags & WAYPOINT_CAMP)
            m_campPoints.Push(i);
        else if (m_paths[i]->flags & WAYPOINT_SNIPER)
            m_sniperPoints.Push(i);
        else if (m_paths[i]->flags & WAYPOINT_RESCUE)
            m_rescuePoints.Push(i);
        else if (m_paths[i]->flags & WAYPOINT_ZMHMCAMP)
            m_zmHmPoints.Push(i);
        else if (m_paths[i]->flags & WAYPOINT_HMCAMPMESH)
            m_hmMeshPoints.Push(i);
        else if (m_paths[i]->flags & WAYPOINT_TERRORIST)
            m_terrorPoints.Push(i);
        else if (m_paths[i]->flags & WAYPOINT_COUNTER)
            m_ctPoints.Push(i);
        else if (m_paths[i]->flags == 0)
            m_otherPoints.Push(i);
    }
}

#ifdef PLATFORM_LINUX
// The WriteCallback function is called by cURL when there is data to be written.
// This is necessary for compatibility with older versions of cURL, which do not
// support the CURLOPT_WRITEDATA option directly (linux)
size_t WriteCallback(void* contents, size_t size, size_t nmemb, FILE* stream)
{
    const size_t written = fwrite(contents, size, nmemb, stream);
    if (written < nmemb)
        ServerPrint("Error: fwrite wrote fewer items than expected: %zu out of %zu\n", written, nmemb);
    return written;
}
#endif

bool Waypoint::Download(void)
{
#ifdef PLATFORM_WIN32
    // could be missing or corrupted? then avoid crash...
    HMODULE hUrlMon = LoadLibrary("urlmon.dll");
    if (hUrlMon != nullptr)
    {
        ServerPrint("UrlMon loaded successfully\n");

        typedef HRESULT(WINAPI* URLDownloadToFileFn)(LPUNKNOWN, LPCSTR, LPCSTR, DWORD, LPBINDSTATUSCALLBACK);
        URLDownloadToFileFn pURLDownloadToFile = reinterpret_cast<URLDownloadToFileFn>(GetProcAddress(hUrlMon, "URLDownloadToFileA"));

        if (pURLDownloadToFile != nullptr)
        {
            ServerPrint("UrlMon loaded successfully\n");

            if (SUCCEEDED(pURLDownloadToFile(nullptr, FormatBuffer("%s/%s.ewp", ebot_download_waypoints_from.GetString(), GetMapName()), (char*)CheckSubfolderFile(false), 0, nullptr)))
            {
                ServerPrint("Download successful\n");
                FreeLibrary(hUrlMon);
                return true;
            }
        }
        else
            ServerPrint("Error: Could not find URLDownloadToFileA in UrlMon, could be courrupted\n");

        FreeLibrary(hUrlMon);
    }
    else
        ServerPrint("Error: Could not load UrlMon, could be missing or courrupted\n");
#else
#ifdef CURL_AVAILABLE
    if (curl_version_info(CURLVERSION_NOW) != nullptr)
    {
        CURL* curl;
        CURLcode res;

        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();

        if (curl)
        {
            const char* downloadURL = FormatBuffer("%s/%s.ewp", ebot_download_waypoints_from.GetString(), GetMapName());
            curl_easy_setopt(curl, CURLOPT_URL, downloadURL);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            ServerPrint("Downloading from URL: %s\n", downloadURL);

            // Path to the 'cstrike/maps' directory
            const char* filepath = FormatBuffer("%s/%s.ewp", GetWaypointDir(), GetMapName());
            FILE* fp = fopen(filepath, "wb");
            if (!fp)
            {
                ServerPrint("Error: Could not open file for writing\n");
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return false;
            }

            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

            res = curl_easy_perform(curl);

            // Check HTTP response code
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code != 200)
            {
                ServerPrint("Error: HTTP response code is not 200, but %ld\n", response_code);
                fclose(fp);
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return false;
            }

            fclose(fp);
            if (res != CURLE_OK)
            {
                ServerPrint("Error: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return false;
            }

            curl_easy_cleanup(curl);
            ServerPrint("Download successful\n");
            return true;
        }

        ServerPrint("Error: Could not initialize cURL handle\n");
        curl_global_cleanup();
    }
    else
        ServerPrint("Error: Could not find valid cURL version\n");
#else
    // check if wget is installed
    if (system("which wget") == 0)
    {
        // wget is installed
        char downloadURL[512];
        snprintf(downloadURL, sizeof(downloadURL), "%s/%s.ewp", ebot_download_waypoints_from.GetString(), GetMapName());

        const char* filepath = FormatBuffer("%s/%s.ewp", GetWaypointDir(), GetMapName());

        char command[512];
        snprintf(command, sizeof(command), "wget -O %s %s", filepath, downloadURL);
        printf("Executing command: %s\n", command);
        const int result = system(command);

        if (result == 0)
        {
            ServerPrint("Download successful wget\n");
            return true;
        }
        else
        {
            ServerPrint("Error: wget command failed with code %d\n", result);
            return false;
        }
    }
    else
        ServerPrint("Error: Neither curl nor wget is available\n");
#endif
#endif

    return false;
}

bool Waypoint::Load(int mode)
{
    m_badMapName = false;

    File fp(CheckSubfolderFile(), "rb");
    if (fp.IsValid())
    {
        WaypointHeader header;
        fp.Read(&header, sizeof(header));

        if (cstricmp(header.mapName, GetMapName()) && mode == 0)
        {
            m_badMapName = true;

            sprintf(m_infoBuffer, "%s.ewp - hacked/broken waypoint file, fileName doesn't match waypoint header information (mapname: '%s', header: '%s')", GetMapName(), GetMapName(), header.mapName);
            AddLogEntry(Log::Error, m_infoBuffer);

            fp.Close();
            return false;
        }
        else
        {
            Initialize();

            if (header.fileVersion >= FV_WAYPOINT)
            {
                g_numWaypoints = header.pointNumber;

                for (int i = 0; i < g_numWaypoints; i++)
                {
                    m_paths[i] = new Path;

                    if (m_paths[i] == nullptr)
                    {
                        AddLogEntry(Log::Memory, "unexpected memory error");
                        return false;
                    }

                    fp.Read(m_paths[i], sizeof(Path));
                }
            }
            else
            {
                g_numWaypoints = header.pointNumber;
                PathOLD* paths[g_numWaypoints];

                for (int i = 0; i < g_numWaypoints; i++)
                {
                    paths[i] = new PathOLD;
                    if (paths[i] == nullptr)
                    {
                        AddLogEntry(Log::Memory, "unexpected memory error");
                        return false;
                    }

                    fp.Read(paths[i], sizeof(PathOLD));

                    m_paths[i] = new Path;
                    if (m_paths[i] == nullptr)
                    {
                        AddLogEntry(Log::Memory, "unexpected memory error");
                        return false;
                    }

                    m_paths[i]->origin = paths[i]->origin;
                    m_paths[i]->radius = paths[i]->radius;
                    m_paths[i]->flags = paths[i]->flags;
                    m_paths[i]->mesh = static_cast<int16>(paths[i]->campStartX);
                    m_paths[i]->gravity = paths[i]->campStartY;

                    for (int C = 0; C < 8; C++)
                    {
                        m_paths[i]->index[C] = paths[i]->index[C];
                        m_paths[i]->connectionFlags[C] = paths[i]->connectionFlags[C];
                    }
                }

                for (int i = 0; i < g_numWaypoints; i++)
                    delete paths[i];

                //Save(); add me in final release, don't break people's waypoints...
            }

            m_waypointPaths = true;
        }

        if (cstrncmp(header.author, "EfeDursun125", 12) == 0 || cstrncmp(header.author, "Mysticpawn", 10) == 0 || cstrncmp(header.author, "Ark | Mysticpawn", 16) == 0)
            sprintf(m_infoBuffer, "Using Official Waypoint File By: %s", header.author);
        else
            sprintf(m_infoBuffer, "Using Waypoint File By: %s", header.author);

        fp.Close();
    }
    else if (ebot_download_waypoints.GetBool())
    {
        if (Download())
        {
            Load();
            sprintf(m_infoBuffer, "%s.ewp is downloaded from the internet", GetMapName());
        }
    }
    else
    {
        if (ebot_analyze_auto_start.GetBool())
        {
            g_waypoint->CreateBasic();

            // no expand
            for (int i = 0; i < (Const_MaxWaypoints - 1); i++)
                g_expanded[i] = false;

            g_analyzewaypoints = true;
        }
        else
        {
            sprintf(m_infoBuffer, "%s.ewp does not exist, pleasue use 'ebot wp analyze' for create waypoints! (dont forget using 'ebot wp analyzeoff' when finished)", GetMapName());
            AddLogEntry(Log::Error, m_infoBuffer);
        }

        return false;
    }

    for (int i = 0; i < g_numWaypoints; i++)
        m_waypointDisplayTime[i] = 0.0f;

    InitTypes();

    g_waypointsChanged = false;

    m_pathDisplayTime = 0.0f;
    m_arrowDisplayTime = 0.0f;

    g_botManager->InitQuota();

    extern ConVar ebot_debuggoal;
    ebot_debuggoal.SetInt(-1);

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

    char* path = CheckSubfolderFile(false);

    // remember the original waypoint author
    File rf(path, "rb");
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

    File fp(path, "wb");

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
        AddLogEntry(Log::Error, "Error writing '%s' waypoint file", GetMapName());
}

void Waypoint::SaveOLD(void)
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

    const char* path = CheckSubfolderFileOLD();

    // remember the original waypoint author
    File rf(path, "rb");
    if (rf.IsValid())
    {
        rf.Read(&header, sizeof(header));
        rf.Close();
    }

    cstrcpy(header.header, FH_WAYPOINT);
    cstrncpy(header.mapName, GetMapName(), 31);

    header.mapName[31] = 0;
    header.fileVersion = 7;
    header.pointNumber = cmin(g_numWaypoints, 1024);

    File fp(path, "wb");

    // file was opened
    if (fp.IsValid())
    {
        // write the waypoint header to the file...
        fp.Write(&header, sizeof(header), 1);

        PathOLD* paths[header.pointNumber];

        for (int i = 0; i < header.pointNumber; i++)
        {
            paths[i] = new PathOLD;

            if (paths[i] == nullptr)
            {
                AddLogEntry(Log::Memory, "unexpected memory error");
                break;
            }

            paths[i]->pathNumber = i;
            paths[i]->origin = m_paths[i]->origin;
            paths[i]->flags = m_paths[i]->flags;
            paths[i]->radius = m_paths[i]->radius;

            // sorry :(
            paths[i]->campStartX = 0;
            paths[i]->campStartY = 0;
            paths[i]->campEndX = 0;
            paths[i]->campEndY = 0;

            for (int C = 0; C < 8; C++)
            {
                paths[i]->index[C] = m_paths[i]->index[C];
                paths[i]->connectionFlags[C] = m_paths[i]->connectionFlags[C];
                paths[i]->connectionVelocity[C] = nullvec;
                paths[i]->distances[C] = 0;
            }
        }

        for (int i = 0; i < header.pointNumber; i++)
        {
            // iterate through connections and find, if it's a jump path
            for (int x = 0; x < Const_MaxPathIndex; x++)
            {
                // check if we got a valid connection
                if (paths[i]->index[x] != -1)
                {
                    Vector myOrigin = paths[i]->origin;
                    Vector waypointOrigin = paths[m_paths[i]->index[x]]->origin;

                    if (paths[m_paths[i]->index[x]]->flags & WAYPOINT_CROUCH)
                        waypointOrigin.z -= 18.0f;
                    else
                        waypointOrigin.z -= 36.0f;

                    if (m_paths[i]->flags & WAYPOINT_CROUCH)
                        myOrigin.z -= 18.0f;
                    else
                        myOrigin.z -= 36.0f;

                    paths[i]->distances[x] = static_cast<int32>((myOrigin - waypointOrigin).GetLength());

                    if (paths[i]->connectionFlags[x] & PATHFLAG_JUMP)
                    {
                        const float timeToReachWaypoint = csqrtf(SquaredF(waypointOrigin.x - myOrigin.x) + SquaredF(waypointOrigin.y - myOrigin.y)) / 250.0f;
                        paths[i]->connectionVelocity[x].x = (waypointOrigin.x - myOrigin.x) / timeToReachWaypoint;
                        paths[i]->connectionVelocity[x].y = (waypointOrigin.y - myOrigin.y) / timeToReachWaypoint;
                        paths[i]->connectionVelocity[x].z = 2.0f * (waypointOrigin.z - myOrigin.z - 0.5f * 1.0f * SquaredF(timeToReachWaypoint)) / timeToReachWaypoint;

                        if (paths[i]->connectionVelocity[x].z > 250.0f)
                            paths[i]->connectionVelocity[x].z = 250.0f;
                    }
                }
            }
        }

        // save the waypoint paths...
        for (int i = 0; i < header.pointNumber; i++)
            fp.Write(paths[i], sizeof(PathOLD));

        for (int i = 0; i < header.pointNumber; i++)
            delete paths[i];

        fp.Close();
    }
    else
        AddLogEntry(Log::Error, "Error writing '%s' waypoint file", GetMapName());
}

String Waypoint::CheckSubfolderFile(const bool pwf)
{
    String returnFile = "";
    returnFile = FormatBuffer("%s/%s.ewp", GetWaypointDir(), GetMapName());

    if (TryFileOpen(returnFile))
        return returnFile;
    else if (pwf)
    {
        returnFile = FormatBuffer("%s/%s.pwf", GetWaypointDir(), GetMapName());
        if (TryFileOpen(returnFile))
            return returnFile;
    }

    return FormatBuffer("%s%s.ewp", GetWaypointDir(), GetMapName());
}

String Waypoint::CheckSubfolderFileOLD(void)
{
    String returnFile = "";
    returnFile = FormatBuffer("%s/%s.pwf", GetWaypointDir(), GetMapName());

    if (TryFileOpen(returnFile))
        return returnFile;

    return FormatBuffer("%s%s.pwf", GetWaypointDir(), GetMapName());
}

// this function returns 2D traveltime to a position
float Waypoint::GetTravelTime(const float maxSpeed, const Vector src, const Vector origin)
{
    // give 10 sec...
    if (src == nullvec || origin == nullvec)
        return 10.0f;

    return (origin - src).GetLength2D() / cabsf(maxSpeed);
}

bool Waypoint::Reachable(edict_t* entity, const int index)
{
    if (FNullEnt(entity))
        return false;

    if (!IsValidWaypoint(index))
        return false;

    const Vector src = GetEntityOrigin(entity);
    const Vector dest = m_paths[index]->origin;

    if ((dest - src).GetLengthSquared() >= SquaredF(1200.0f))
        return false;

    if (entity->v.waterlevel != 2 && entity->v.waterlevel != 3)
    {
        if ((dest.z > src.z + 62.0f || dest.z < src.z - 100.0f) && (!(GetPath(index)->flags & WAYPOINT_LADDER) || (dest - src).GetLengthSquared2D() >= SquaredF(120.0f)))
            return false;
    }

    TraceResult tr{};
    TraceHull(src, dest, true, head_hull, entity, &tr);
    if (tr.flFraction == 1.0f)
        return true;

    return false;
}

bool Waypoint::IsNodeReachable(const Vector src, const Vector destination)
{
    float distance = (destination - src).GetLengthSquared();

    // is the destination not close enough?
    if (distance > SquaredF(g_autoPathDistance))
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
        if (goBehind == true)
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

        while (distance > SquaredF(10.0f))
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

bool Waypoint::IsNodeReachableWithJump(const Vector src, const Vector destination, const int flags)
{
    if (flags > 0)
        return false;

    float distance = (destination - src).GetLengthSquared();

    // is the destination not close enough?
    if (distance > SquaredF(g_autoPathDistance))
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
        if (goBehind == true)
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

        while (distance > SquaredF(10.0f))
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
    if (path == nullptr)
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
                path->flags |= WAYPOINT_JUMP;
        }
    }

    static char messageBuffer[1024];
    sprintf(messageBuffer, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", 
        (path->flags == 0 && !jumpPoint) ? "(none)" : "", 
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
            if (g_hostEntity->v.button & IN_JUMP)
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
            // check that no other reachable waypoints are nearby...
            for (int i = 0; i < g_numWaypoints; i++)
            {
                if (IsNodeReachable(GetEntityOrigin(g_hostEntity), m_paths[i]->origin))
                {
                    distance = (m_paths[i]->origin - GetEntityOrigin(g_hostEntity)).GetLengthSquared();

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
        const float distance = (m_paths[i]->origin - GetEntityOrigin(g_hostEntity)).GetLengthSquared();

        // check if waypoint is whitin a distance, and is visible
        if ((distance < SquaredF(640.0f) && ::IsVisible(m_paths[i]->origin, g_hostEntity) && IsInViewCone(m_paths[i]->origin, g_hostEntity)) || distance < SquaredF(48.0f))
        {
            // check the distance
            if (distance < nearestDistance)
            {
                nearestIndex = i;
                nearestDistance = distance;
            }

            // draw mesh links
            if (m_paths[nearestIndex]->mesh != 0 && IsInViewCone(m_paths[nearestIndex]->origin, g_hostEntity) && (m_paths[nearestIndex]->flags & WAYPOINT_HMCAMPMESH || m_paths[nearestIndex]->flags & WAYPOINT_ZMHMCAMP))
            {
                for (int x = 0; x < g_numWaypoints; x++)
                {
                    if (!(m_paths[nearestIndex]->flags & WAYPOINT_HMCAMPMESH) && !(m_paths[nearestIndex]->flags & WAYPOINT_ZMHMCAMP))
                        continue;

                    if (m_paths[nearestIndex]->mesh != m_paths[x]->mesh)
                        continue;

                    if (!IsInViewCone(m_paths[x]->origin, g_hostEntity))
                        continue;

                    const Vector& src = m_paths[nearestIndex]->origin + Vector(0, 0, (m_paths[nearestIndex]->flags & WAYPOINT_CROUCH) ? 9.0f : 18.0f);
                    const Vector& dest = m_paths[x]->origin + Vector(0, 0, (m_paths[x]->flags & WAYPOINT_CROUCH) ? 9.0f : 18.0f);

                    // draw links
                    engine->DrawLine(g_hostEntity, src, dest, Color(0, 0, 255, 255), 5, 0, 0, 10);
                }
            }

            if (m_waypointDisplayTime[i] + 1.0f < engine->GetTime())
            {
                float nodeHeight = (m_paths[i]->flags & WAYPOINT_CROUCH) ? 36.0f : 72.0f; // check the node height
                float nodeHalfHeight = nodeHeight * 0.5f;

                // all waypoints are by default are green
                Color nodeColor = Color(ebot_waypoint_r.GetFloat(), ebot_waypoint_g.GetFloat(), ebot_waypoint_b.GetFloat(), 255);

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
                else if (m_paths[i]->flags & WAYPOINT_HMCAMPMESH)
                    nodeColor = Color(50, 125, 255, 255);
                else if (m_paths[i]->flags & WAYPOINT_ZOMBIEONLY)
                    nodeColor = Color(255, 0, 0, 255);
                else if (m_paths[i]->flags & WAYPOINT_HUMANONLY)
                    nodeColor = Color(0, 0, 255, 255);
                else if (m_paths[i]->flags & WAYPOINT_ZOMBIEPUSH)
                    nodeColor = Color(250, 75, 150, 255);
                else if (m_paths[i]->flags & WAYPOINT_FALLRISK)
                    nodeColor = Color(128, 128, 128, 255);
                else if (m_paths[i]->flags & WAYPOINT_SPECIFICGRAVITY)
                    nodeColor = Color(128, 128, 128, 255);
                else if (m_paths[i]->flags & WAYPOINT_ONLYONE)
                    nodeColor = Color(255, 255, 0, 255);
                else if (m_paths[i]->flags & WAYPOINT_WAITUNTIL)
                    nodeColor = Color(0, 0, 255, 255);

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
                else if (m_paths[i]->flags & WAYPOINT_HMCAMPMESH)
                    nodeFlagColor = Color(0, 0, 255, 255);
                else if (m_paths[i]->flags & WAYPOINT_ZOMBIEONLY)
                    nodeFlagColor = Color(255, 0, 255, 255);
                else if (m_paths[i]->flags & WAYPOINT_HUMANONLY)
                    nodeFlagColor = Color(255, 0, 255, 255);
                else if (m_paths[i]->flags & WAYPOINT_ZOMBIEPUSH)
                    nodeFlagColor = Color(255, 0, 0, 255);
                else if (m_paths[i]->flags & WAYPOINT_FALLRISK)
                    nodeFlagColor = Color(250, 75, 150, 255);
                else if (m_paths[i]->flags & WAYPOINT_SPECIFICGRAVITY)
                    nodeFlagColor = Color(128, 0, 255, 255);
                else if (m_paths[i]->flags & WAYPOINT_WAITUNTIL)
                    nodeFlagColor = Color(250, 75, 150, 255);

                nodeColor.alpha = 255;
                nodeFlagColor.alpha = 255;

                // draw node without additional flags
                if (nodeFlagColor.red == -1)
                    engine->DrawLine(g_hostEntity, m_paths[i]->origin - Vector(0.0f, 0.0f, nodeHalfHeight), m_paths[i]->origin + Vector(0.0f, 0.0f, nodeHalfHeight), nodeColor, ebot_waypoint_size.GetFloat(), 0, 0, 10);
                else // draw node with flags
                {
                    engine->DrawLine(g_hostEntity, m_paths[i]->origin - Vector(0.0f, 0.0f, nodeHalfHeight), m_paths[i]->origin - Vector(0.0f, 0.0f, nodeHalfHeight - nodeHeight * 0.75f), nodeColor, ebot_waypoint_size.GetFloat(), 0, 0, 10); // draw basic path
                    engine->DrawLine(g_hostEntity, m_paths[i]->origin - Vector(0.0f, 0.0f, nodeHalfHeight - nodeHeight * 0.75f), m_paths[i]->origin + Vector(0.0f, 0.0f, nodeHalfHeight), nodeFlagColor, ebot_waypoint_size.GetFloat(), 0, 0, 10); // draw additional path
                }

                if (m_paths[i]->flags & WAYPOINT_FALLCHECK || m_paths[i]->flags & WAYPOINT_WAITUNTIL)
                {
                    TraceResult tr{};
                    TraceLine(m_paths[i]->origin, m_paths[i]->origin - Vector(0.0f, 0.0f, 60.0f), false, false, g_hostEntity, &tr);

                    if (tr.flFraction == 1.0f)
                        engine->DrawLine(g_hostEntity, m_paths[i]->origin, m_paths[i]->origin - Vector(0.0f, 0.0f, 60.0f), Color(255, 0, 0, 255), ebot_waypoint_size.GetFloat() - 1.0f, 0, 0, 10);
                    else
                        engine->DrawLine(g_hostEntity, m_paths[i]->origin, m_paths[i]->origin - Vector(0.0f, 0.0f, 60.0f), Color(0, 0, 255, 255), ebot_waypoint_size.GetFloat() - 1.0f, 0, 0, 10);
                }

                m_waypointDisplayTime[i] = engine->GetTime();
            }
            else if (m_waypointDisplayTime[i] + 2.0f > engine->GetTime()) // what???
                m_waypointDisplayTime[i] = 0.0f;
        }
    };

    // now iterate through all waypoints in a map, and draw required ones
    const int random = CRandomInt(1, 2);
    if (random == 1)
    {
        for (int i = 0; i < g_numWaypoints; i++)
            update(i);
    }
    else
    {
        for (int i = (g_numWaypoints - 1); i > 0; i--)
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
                engine->DrawLine(g_hostEntity, m_paths[m_findWPIndex]->origin, GetEntityOrigin(g_hostEntity), Color(128, 0, 128, 255), 10, 0, 0, 5, LINE_ARROW);

            // cached waypoint - yellow arrow
            if (IsValidWaypoint(m_cacheWaypointIndex))
                engine->DrawLine(g_hostEntity, m_paths[m_cacheWaypointIndex]->origin, GetEntityOrigin(g_hostEntity), Color(255, 255, 0, 255), 10, 0, 0, 5, LINE_ARROW);

            // waypoint user facing at - white arrow
            if (IsValidWaypoint(m_facingAtIndex))
                engine->DrawLine(g_hostEntity, m_paths[m_facingAtIndex]->origin, GetEntityOrigin(g_hostEntity), Color(255, 255, 255, 255), 10, 0, 0, 5, LINE_ARROW);

            m_arrowDisplayTime = engine->GetTime();
        }
        else if (m_arrowDisplayTime + 1.0 > engine->GetTime()) // what???
            m_arrowDisplayTime = 0.0f;
    }

    // create path pointer for faster access
    Path* path = m_paths[nearestIndex];

    // draw a paths, camplines and danger directions for nearest waypoint
    if (nearestDistance < SquaredF(2048) && m_pathDisplayTime < engine->GetTime())
    {
        m_pathDisplayTime = AddTime(1.0f);

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
            if (IsConnected(i, nearestIndex) && !IsConnected(nearestIndex, i))
                engine->DrawLine(g_hostEntity, path->origin, m_paths[i]->origin, Color(0, 192, 96, 255), 5, 0, 0, 10);
        }

        // draw the radius circle
        const Vector origin = (path->flags & WAYPOINT_CROUCH) ? path->origin : path->origin - Vector(0, 0, 18.0f);

        // if radius is nonzero, draw a square
        if (path->radius > 4)
        {
            const float root = static_cast<float>(path->radius);
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
                "      Flags: %s\n", m_cacheWaypointIndex, g_numWaypoints, m_paths[m_cacheWaypointIndex]->radius, GetWaypointInfo(m_cacheWaypointIndex), (!(m_paths[m_cacheWaypointIndex]->flags & WAYPOINT_HMCAMPMESH) && !(m_paths[m_cacheWaypointIndex]->flags & WAYPOINT_ZMHMCAMP)) ? "" : "Mesh ID: %d", static_cast<int>(m_paths[m_cacheWaypointIndex]->mesh));
        }

        // check if we need to show the facing point index, only if no menu to show
        if (m_facingAtIndex != -1 && g_clients[ENTINDEX(g_hostEntity) - 1].menu == nullptr)
        {
            length += sprintf(&tempMessage[length], "\n    Facing Waypoint Information:\n\n"
                "      Waypoint %d of %d, Radius: %d\n"
                "      Flags: %s\n", m_facingAtIndex, g_numWaypoints, m_paths[m_facingAtIndex]->radius, GetWaypointInfo(m_facingAtIndex));
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
                    AddLogEntry(Log::Warning, "Waypoint %d connected with invalid Waypoint #%d!", i, m_paths[i]->index[j]);
                    (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i]->origin);
                    haveError = true;
                }

                connections++;
                break;
            }
        }

        if (connections == 0)
        {
            if (!IsConnected(i))
            {
                AddLogEntry(Log::Warning, "Waypoint %d isn't connected with any other Waypoint!", i);
                (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i]->origin);
                haveError = true;
            }
        }

        if (m_paths[i]->flags & WAYPOINT_TERRORIST)
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
                    AddLogEntry(Log::Warning, "Waypoint %d - Pathindex %d out of Range!", i, k);
                    (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i]->origin);

                    g_waypointOn = true;
                    g_editNoclip = true;

                    haveError = true;
                }
                else if (m_paths[i]->index[k] == i)
                {
                    AddLogEntry(Log::Warning, "Waypoint %d - Pathindex %d points to itself!", i, k);
                    (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i]->origin);

                    g_waypointOn = true;
                    g_editNoclip = true;

                    haveError = true;
                }
            }
        }
    }

    if (g_mapType & MAP_CS && GetGameMode() == GameMode::Original)
    {
        if (rescuePoints == 0)
        {
            AddLogEntry(Log::Warning, "You didn't set a Rescue Point!");
            haveError = true;
        }
    }

    if (terrPoints == 0 && GetGameMode() == GameMode::Original)
    {
        AddLogEntry(Log::Warning, "You didn't set any Terrorist Important Point!");
        haveError = true;
    }
    else if (ctPoints == 0 && GetGameMode() == GameMode::Original)
    {
        AddLogEntry(Log::Warning, "You didn't set any CT Important Point!");
        haveError = true;
    }
    else if (goalPoints == 0 && GetGameMode() == GameMode::Original)
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

    if (srcIndex == destIndex)
        return 1.0f;

    return (m_paths[srcIndex]->origin - m_paths[destIndex]->origin).GetLengthSquared();
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

        TraceResult tr;
        Vector up, down, front, back;

        Vector diff = (ladderLeft - ladderRight) * 15.0f;
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
        if (ent->v.effects & EF_NODRAW && ent->v.speed > 0.0f)
            continue;

        const Vector origin = GetEntityOrigin(ent);
        if (g_analyzewaypoints && FindNearest(origin, 250.0f) == -1)
            Add(2, Vector(origin.x, origin.y, (origin.z + 36.0f))); // goal waypoints will be added by analyzer
        else if (FindNearest(origin, 50.0f) == -1)
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
        const Vector origin = GetEntityOrigin(ent);
        if (FindNearest(origin, 50.0f) == -1)
            Add(0, Vector(origin.x, origin.y, (origin.z + 36.0f)));
    }
}

Path* Waypoint::GetPath(const int id)
{
    // to avoid crash
    if (!IsValidWaypoint(id))
        return m_paths[CRandomInt(0, g_numWaypoints - 1)];

    return m_paths[id];
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

void Waypoint::SetLearnJumpWaypoint(int mod)
{
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

Waypoint::Waypoint(void)
{
    m_waypointPaths = false;
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

    m_terrorPoints.RemoveAll();
    m_ctPoints.RemoveAll();
    m_goalPoints.RemoveAll();
    m_campPoints.RemoveAll();
    m_rescuePoints.RemoveAll();
    m_sniperPoints.RemoveAll();
    m_otherPoints.RemoveAll();
}

Waypoint::~Waypoint(void)
{
    m_pathDisplayTime = 0.0f;
    m_arrowDisplayTime = 0.0f;
}