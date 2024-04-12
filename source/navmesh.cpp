#include <core.h>

bool IsValidNavArea(const uint16_t index)
{
    if (index >= g_numNavAreas)
        return false;

    return true;
}

Vector GetPositionOnGrid(const Vector& origin)
{
    return Vector(static_cast<int>(origin.x / GridSize) * GridSize, static_cast<int>(origin.y / GridSize) * GridSize, origin.z);
}

uint8_t GetNextCorner(const uint8_t corner)
{
    switch (corner)
    {
    case ENavDir::Right:
        return ENavDir::Forward;
    case ENavDir::Left:
        return ENavDir::Backward;
    case ENavDir::Forward:
        return ENavDir::Left;
    case ENavDir::Backward:
        return ENavDir::Right;
    }

    return 0;
}

void ENavMesh::Initialize(void)
{
    m_area.Destroy();
    g_numNavAreas = 0;
}

void ENavMesh::CreateBasic(void)
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

        ENavArea* area = CreateArea((up + down) * 0.5f);
        if (!area)
            continue;

        area->direction[ENavDir::Right] = down.x;
        area->direction[ENavDir::Left] = up.x;
        area->direction[ENavDir::Forward] = down.y;
        area->direction[ENavDir::Backward] = up.y;

        area->dirHeight[ENavDir::Right] = down.z;
        area->dirHeight[ENavDir::Left] = up.z;
        area->dirHeight[ENavDir::Forward] = down.z;
        area->dirHeight[ENavDir::Backward] = up.z;

        area->flags |= NAV_LADDER;
    }

    // then terrortist spawnpoints
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_player_deathmatch")))
        CreateArea(GetPositionOnGrid(GetWalkablePosition(GetEntityOrigin(ent), ent)));

    // then add ct spawnpoints
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_player_start")))
        CreateArea(GetPositionOnGrid(GetWalkablePosition(GetEntityOrigin(ent), ent)));

    // then vip spawnpoint
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_vip_start")))
        CreateArea(GetPositionOnGrid(GetWalkablePosition(GetEntityOrigin(ent), ent)));

    // hostage rescue zone
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_hostage_rescue")))
        CreateArea(GetPositionOnGrid(GetWalkablePosition(GetEntityOrigin(ent), ent)));

    // hostage rescue zone (same as above)
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_hostage_rescue")))
        CreateArea(GetPositionOnGrid(GetWalkablePosition(GetEntityOrigin(ent), ent)));

    // bombspot zone
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_bomb_target")))
        CreateArea(GetPositionOnGrid(GetWalkablePosition(GetEntityOrigin(ent), ent)));

    // bombspot zone (same as above)
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "info_bomb_target")))
        CreateArea(GetPositionOnGrid(GetWalkablePosition(GetEntityOrigin(ent), ent)));

    // hostage entities
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "hostage_entity")))
    {
        // if already saved || moving skip it
        if (ent->v.effects & EF_NODRAW || ent->v.speed > 0.0f)
            continue;

        CreateArea(GetPositionOnGrid(GetWalkablePosition(GetEntityOrigin(ent))));
    }

    // vip rescue (safety) zone
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_vip_safetyzone")))
        CreateArea(GetPositionOnGrid(GetWalkablePosition(GetEntityOrigin(ent), ent)));

    // terrorist escape zone
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "func_escapezone")))
        CreateArea(GetPositionOnGrid(GetWalkablePosition(GetEntityOrigin(ent), ent)));

    // weapons on the map?
    while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "armoury_entity")))
        CreateArea(GetPositionOnGrid(GetWalkablePosition(GetEntityOrigin(ent))));
}

int16_t GetNearestNavAreaCenterID(const Vector& origin, const float maxDist, const int ignore)
{
    int16_t index = -1;
    uint16_t i;
    float dist, minDist = squaredf(maxDist);

    // for each NavArea
    for (i = 0; i < g_numNavAreas; i++)
    {
        // ignore this for expanding
        if (i == ignore)
            continue;

        // compute the squared distance from the given origin to the NavArea's center
        dist = (g_navmesh->GetNavArea(i).GetCenter() - origin).GetLengthSquared();

        // if this NavArea is closer than the previous closest NavArea
        if (dist < minDist)
        {
            // update the closest NavArea and its minimum distance
            index = g_navmesh->GetNavArea(i).index;
            minDist = dist;
        }
    }

    return index;
}

bool IsDeadlyDrop(const Vector& botPos, const Vector& targetOriginPos)
{
    TraceResult tr{};

    const Vector move((targetOriginPos - botPos).ToYaw(), 0.0f, 0.0f);
    MakeVectors(move);

    const Vector direction = (targetOriginPos - botPos).Normalize();  // 1 unit long
    Vector check = botPos;
    Vector down = botPos;

    down.z = down.z - 1000.0f;  // straight down 1000 units

    TraceHull(check, down, true, head_hull, g_hostEntity, &tr);

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

        TraceHull(check, down, true, head_hull, g_hostEntity, &tr);
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

bool IsReachable(const Vector& start, const Vector& end)
{
    if (end.z > (start.z + 62.0f) || end.z < (start.z - 100.0f))
        return false;

    // be smart
    if (POINT_CONTENTS(end) == CONTENTS_LAVA)
        return false;

    TraceResult tr{};
    TraceHull(start, end, true, head_hull, g_hostEntity, &tr);

    // we're not sure, return the current one
    if (tr.fAllSolid)
        return false;

    if (tr.fStartSolid)
        return false;

    if (!FNullEnt(tr.pHit))
    {
        if (cstrcmp("func_illusionary", STRING(tr.pHit->v.classname)) == 0)
            return false;

        if (!IsDeadlyDrop(start, ((start + end)) * 0.5f))
            return true;
        else if (tr.flFraction == 1.0f)
        {
            if (tr.fInWater)
                return true;
            else
                return false;
        }
    }

    if (tr.flFraction == 1.0f && (!tr.fInWater || !IsDeadlyDrop(start, ((start + end)) * 0.5f)))
        return true;

    return false;
}

void ENavMesh::Analyze(void)
{
    if (!g_numNavAreas)
        return;

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

    int16_t i, j;

    // guarantee to have it
    if (!expanded)
    {
        safeloc(expanded, Const_MaxWaypoints);
        for (i = 0; i < Const_MaxWaypoints; i++)
            expanded[i] = false;
    }

    ENavArea* area;
    int what, nav;
    TraceResult tr{};
    Vector temp, temp2;
    int8_t dir;
    for (i = 0; i < g_numNavAreas; i++)
    {
        if (expanded[i])
            continue;

        if (magicTimer > engine->GetTime())
            return;

        for (dir = 0; dir < ENavDir::NumDir; dir++)
        {
            temp = GetWalkablePosition(m_area[i].GetCenter(), nullptr, 100.0f);
            temp.z += GridSize;
            temp2 = temp + Vector(0.0f, 0.0f, GridSize) + DirectionAsVector(dir, GridSize);
            temp2 = GetWalkablePosition(temp2, g_hostEntity, true, 200.0f);
            if (temp2 == nullvec)
                continue;

            temp2.z += GridSize;

            what = POINT_CONTENTS(temp2);
            if (what == CONTENTS_SOLID)
            {
                engine->DrawLine(g_hostEntity, temp, temp2, Color(255, 0, 0, 255), 4, 0, 5, 1, LINE_SIMPLE);
                continue;
            }

            if (what == CONTENTS_SKY)
            {
                engine->DrawLine(g_hostEntity, temp, temp2, Color(255, 0, 0, 255), 4, 0, 5, 1, LINE_SIMPLE);
                continue;
            }

            if (what == CONTENTS_LAVA)
            {
                engine->DrawLine(g_hostEntity, temp, temp2, Color(255, 0, 0, 255), 4, 0, 5, 1, LINE_SIMPLE);
                continue;
            }

            nav = GetNearestNavAreaCenterID(temp2, GridSize * 1.1f, i);
            if (nav != -1)
            {
                ConnectArea(i, nav);
                ConnectArea(nav, i);
                engine->DrawLine(g_hostEntity, temp, temp2, Color(0, 255, 0, 255), 4, 0, 5, 1, LINE_SIMPLE);
                continue;
            }

            for (j = 0; j < g_numNavAreas; j++)
            {
                if (m_area[i].index == m_area[j].index)
                {
                    engine->DrawLine(g_hostEntity, temp, temp2, Color(255, 0, 0, 255), 4, 0, 5, 1, LINE_SIMPLE);
                    continue;
                }

                if (m_area[j].IsPointOverlapping(temp2))
                {
                    engine->DrawLine(g_hostEntity, temp, temp2, Color(255, 0, 0, 255), 4, 0, 5, 1, LINE_SIMPLE);
                    continue;
                }

                if (m_area[j].IsAreaOverlapping(m_area[i]))
                {
                    engine->DrawLine(g_hostEntity, temp, temp2, Color(255, 0, 0, 255), 4, 0, 5, 1, LINE_SIMPLE);
                    continue;
                }
            }

            if (!IsWalkableLineClear(temp, temp2))
            {
                engine->DrawLine(g_hostEntity, temp, temp2, Color(255, 0, 0, 255), 4, 0, 5, 1, LINE_SIMPLE);
                continue;
            }

            engine->DrawLine(g_hostEntity, temp, temp2, Color(255, 255, 255, 255), 4, 0, 5, 1, LINE_SIMPLE);
            temp2.z -= GridSize;

            area = CreateArea(temp2);
            if (area)
            {
                if (IsReachable(area->GetCenter(), GetNavArea(i).GetCenter()))
                    ConnectArea(area->index, i);

                if (IsReachable(GetNavArea(i).GetCenter(), area->GetCenter()))
                    ConnectArea(i, area->index);
            }

            magicTimer = engine->GetTime() + 0.005f;
        }

        expanded[i] = true;
    }

    if (magicTimer + 2.0f < engine->GetTime()) // set to 2.0f
    {
        g_analyzewaypoints = false;
        g_analyzenavmesh = false;
        g_waypointOn = false;
        //g_navmeshOn = false;
        g_editNoclip = false;
        FinishAnalyze();
        safedel(expanded);
    }
}

void ENavMesh::FinishAnalyze(void)
{
    uint16_t i;

    // try to merge nav areas, first try to merge if their z axis is equal
    MiniArray <int> mergeList;
    for (i = 0; i < g_numNavAreas; i++)
    {

    }
}

void ENavMesh::ConnectArea(const int start, const int goal)
{
    if (!IsValidNavArea(start))
        return;

    if (!IsValidNavArea(goal))
        return;

    if (start == goal)
        return;

    ENavArea* area = &m_area[start];
    if (!area)
        return;

    uint8_t newCount = area->connectionCount;
    if (newCount >= 255)
        return;

    // don't allow areas get connected twice
    uint8_t i;
    for (i = 0; i < newCount; i++)
    {
        if (area->connection[i] == goal)
            return;
    }

    newCount++;
    uint16_t* newCons = new(std::nothrow) uint16_t[newCount]{};
    if (!newCons)
        return;

    for (i = 0; i < area->connectionCount; i++)
        newCons[i] = area->connection[i];

    if (area->connection)
        delete[] area->connection;
    
    // add our new path
    newCons[newCount - 1] = goal;

    // set the connections
    area->connection = newCons;
    area->connectionCount = newCount;

    // done
    newCons = nullptr;
    PlaySound(g_hostEntity, "weapons/mine_activate.wav");
}

void ENavMesh::DisconnectArea(const int start, const int goal)
{
    if (!IsValidNavArea(start))
        return;

    if (!IsValidNavArea(goal))
        return;

    ENavArea* area = &m_area[start];
    if (!area)
        return;

    uint8_t i;
    for (i = 0; i < area->connectionCount; ++i)
    {
        if (area->connection[i] == goal)
        {
            cswap(area->connection[i], area->connection[area->connectionCount - 1]);
            --area->connectionCount;

            uint16_t* tempArray = new(std::nothrow) uint16_t[area->connectionCount]{};
            if (!tempArray)
                break;

            cmemcpy(tempArray, area->connection, sizeof(uint16_t) * area->connectionCount);
            delete[] area->connection;
            area->connection = tempArray;
            break;
        }
    }
}

ENavArea* ENavMesh::CreateArea(const Vector origin, const bool expand)
{
    if (g_numNavAreas >= Const_MaxNavAreas)
        return nullptr;

    const uint16_t index = g_numNavAreas;
    if (!m_area.Push(ENavArea{}))
    {
        AddLogEntry(Log::Memory, "unexpected memory error");
        return nullptr;
    }

    g_numNavAreas++;

    ENavArea* area = &m_area[index];
    area->index = index;
    area->connection = nullptr;
    area->connectionCount = 0;
    
    const float size = GridSize * 0.25f;
    area->direction[ENavDir::Right] = origin.x + size;
    area->direction[ENavDir::Left] = origin.x - size;
    area->direction[ENavDir::Forward] = origin.y + size;
    area->direction[ENavDir::Backward] = origin.y - size;

    const float height = 10.0f;
    area->dirHeight[ENavDir::Right] = origin.z + height;
    area->dirHeight[ENavDir::Left] = origin.z + height;
    area->dirHeight[ENavDir::Forward] = origin.z + height;
    area->dirHeight[ENavDir::Backward] = origin.z + height;

    if (expand)
        area->ExpandNavArea(static_cast<uint8_t>(100));
    //else
    //    area->ExpandNavArea(static_cast<uint8_t>(croundf(GridSize * 0.25f)));

    PlaySound(g_hostEntity, "weapons/xbow_hit1.wav");
    return area;
}

void ENavMesh::DeleteArea(const ENavArea area)
{
    DeleteAreaByIndex(area.index);
}

void ENavMesh::DeleteAreaByIndex(const int index)
{
    if (g_numNavAreas < 1)
        return;

    if (!IsValidNavArea(index))
        return;

    // DisconnectArea will handle this
    uint16_t i, j;
    for (i = 0; i < m_area[index].connectionCount; i++)
    {
        DisconnectArea(m_area[index].connection[i], index);
        DisconnectArea(index, m_area[index].connection[i]);
    }

    for (i = 0; i < g_numNavAreas; i++)
    {
        if (index == i)
            continue;

        for (j = 0; j < m_area[i].connectionCount; j++)
        {
            DisconnectArea(m_area[i].connection[j], index);
            DisconnectArea(index, m_area[i].connection[j]);
        }
    }

    for (i = 0; i < g_numNavAreas; i++)
    {
        if (index == i)
            continue;

        for (j = 0; j < m_area[i].connectionCount; j++)
        {
            if (m_area[i].connection[j] > index)
                m_area[i].connection[j]--;
        }

        if (m_area[i].index > index)
            m_area[i].index--;
    }

    m_area.RemoveAt(index);
    if (g_numNavAreas)
        g_numNavAreas--;

    PlaySound(g_hostEntity, "weapons/mine_activate.wav");
}

int ENavMesh::GetNearestNavAreaID(const Vector origin)
{
    int index = -1;
    uint16_t i;
    float dist, minDist = FLT_MAX;

    // for each NavArea
    for (i = 0; i < g_numNavAreas; i++)
    {
        // compute the squared distance from the given origin to the NavArea's nearest edge
        dist = (m_area[i].GetClosestPosition(origin) - origin).GetLengthSquared();

        // if this NavArea is closer than the previous closest NavArea
        if (dist < minDist)
        {
            // update the closest NavArea and its minimum distance
            index = m_area[i].index;
            minDist = dist;
        }
    }

    return index;
}

Vector ENavMesh::DirectionAsVector(const uint8_t corner, const float size)
{
    switch (corner)
    {
    case ENavDir::Right:
        return Vector(size, 0.0f, 0.0f);
    case ENavDir::Left:
        return Vector(-size, 0.0f, 0.0f);
    case ENavDir::Forward:
        return Vector(0.0f, size, 0.0f);
    case ENavDir::Backward:
        return Vector(0.0f, -size, 0.0f);
    }

    return Vector(0.0f, 0.0f, 0.0f);
}

float ENavMesh::DirectionAsFloat(const uint8_t corner)
{
    switch (corner)
    {
    case ENavDir::Right:
        return 1.0f;
    case ENavDir::Left:
        return -1.0f;
    case ENavDir::Forward:
        return 1.0f;
    case ENavDir::Backward:
        return -1.0f;
    }

    return 0.0f;
}

Vector ENavMesh::GetAimingPosition(void)
{
    if (FNullEnt(g_hostEntity))
        return nullvec;

    TraceResult tr{};
    const Vector eyePosition = g_hostEntity->v.origin + g_hostEntity->v.view_ofs;
    MakeVectors(g_hostEntity->v.v_angle);
    TraceHull(eyePosition, eyePosition + g_pGlobals->v_forward * 1000.0f, false, point_hull, g_hostEntity, &tr);

    return tr.vecEndPos;
}

void ENavMesh::SelectNavArea(void)
{
    const Vector aimPosition = GetAimingPosition();
    const int16_t areaIndex = GetNearestNavAreaID(aimPosition);
    if (areaIndex == -1)
    {
        CenterPrint("Cannot find any area");
        return;
    }

    m_selectedNavIndex = areaIndex;
    CenterPrint("Selected nav area: #%d", m_selectedNavIndex);
}

void ENavMesh::UnselectNavArea(const bool msg)
{
    if (msg)
        CenterPrint("Unselected nav area: #%d", m_selectedNavIndex);

    m_selectedNavIndex = -1;
}

void ENavMesh::DrawNavArea(void)
{
    if (FNullEnt(g_hostEntity))
        return;
    
    const Vector aimPosition = GetAimingPosition();

    const float size = 8.0f;
    engine->DrawLine(g_hostEntity, aimPosition + Vector(size, 0.0f, 0.0f), aimPosition + Vector(-size, 0.0f, 0.0f), Color(255, 255, 255, 255), 4, 0, 5, 1, LINE_SIMPLE);
    engine->DrawLine(g_hostEntity, aimPosition + Vector(0.0f, size, 0.0f), aimPosition + Vector(0.0f, -size, 0.0f), Color(255, 255, 255, 255), 4, 0, 5, 1, LINE_SIMPLE);
    engine->DrawLine(g_hostEntity, aimPosition + Vector(0.0f, 0.0f, size), aimPosition + Vector(0.0f, 0.0f, -size), Color(255, 255, 255, 255), 4, 0, 5, 1, LINE_SIMPLE);

    const int areaIndex = GetNearestNavAreaID(aimPosition);
    if (areaIndex == -1)
        return;

    ENavArea nearestArea = m_area[areaIndex];

    uint16_t k;
    uint8_t i, j, link;
    for (i = 0; i < ENavDir::NumDir; i++)
    {
        if (IsValidNavArea(m_selectedNavIndex))
        {
            engine->DrawLine(g_hostEntity, m_area[m_selectedNavIndex].GetCornerPosition(i), m_area[m_selectedNavIndex].GetCornerPosition(GetNextCorner(i)), Color(0, 255, 0, 255), 5, 0, 5, 1, LINE_SIMPLE);

            if (m_selectedNavIndex != areaIndex)
                engine->DrawLine(g_hostEntity, nearestArea.GetCornerPosition(i), nearestArea.GetCornerPosition(GetNextCorner(i)), Color(0, 255, 0, 255), 5, 0, 5, 1, LINE_SIMPLE);
        }
        else
            engine->DrawLine(g_hostEntity, nearestArea.GetCornerPosition(i), nearestArea.GetCornerPosition(GetNextCorner(i)), Color(0, 255, 0, 255), 5, 0, 5, 1, LINE_SIMPLE);
    }

    ENavArea neighbor, more;
    for (link = 0; link < nearestArea.connectionCount; link++)
    {
        if (!IsValidNavArea(nearestArea.connection[link]))
            continue;

        neighbor = m_area[nearestArea.connection[link]];
        if (neighbor.index == nearestArea.index)
            continue;

        for (j = 0; j < ENavDir::NumDir; j++)
            engine->DrawLine(g_hostEntity, neighbor.GetCornerPosition(j), neighbor.GetCornerPosition(GetNextCorner(j)), Color(0, 100, 255, 255), 5, 0, 5, 1, LINE_SIMPLE);

        engine->DrawLine(g_hostEntity, nearestArea.GetCenter(), neighbor.GetCenter(), Color(255, 255, 255, 255), 5, 0, 5, 1, LINE_SIMPLE);

        for (i = 0; i < neighbor.connectionCount; i++)
        {
            if (!IsValidNavArea(neighbor.connection[i]))
                continue;

            more = m_area[neighbor.connection[i]];
            if (more.index == neighbor.index)
                continue;

            if (more.index == nearestArea.index)
                continue;

            for (j = 0; j < ENavDir::NumDir; j++)
                engine->DrawLine(g_hostEntity, more.GetCornerPosition(j), more.GetCornerPosition(GetNextCorner(j)), Color(0, 100, 255, 255), 5, 0, 5, 1, LINE_SIMPLE);

            engine->DrawLine(g_hostEntity, neighbor.GetCenter(), more.GetCenter(), Color(255, 255, 255, 255), 5, 0, 5, 1, LINE_SIMPLE);
        }
    }
}

bool ENavMesh::IsConnected(const int start, const int goal)
{
    const ENavArea area = m_area[start];

    uint8_t i;
    for (i = 0; i < area.connectionCount; i++)
    {
        if (area.connection[i] == goal)
            return true;
    }

    return false;
}

String CheckSubfolderFile(void)
{
    String returnFile = FormatBuffer("%s/%s.nav", FormatBuffer("%s/addons/ebot/navigations/", GetModName()), GetMapName());
    if (TryFileOpen(returnFile))
        return returnFile;

    return FormatBuffer("%s%s.nav", FormatBuffer("%s/addons/ebot/navigations/", GetModName()), GetMapName());
}

bool ENavMesh::LoadNav(void)
{
    ENavHeader header;
    File fp(CheckSubfolderFile(), "rb");

    if (fp.IsValid())
    {
        Initialize();
        fp.Read(&header, sizeof(header));
        g_numNavAreas = header.navNumber;

        uint16_t i;
        for (i = 0; i < g_numNavAreas; i++)
        {
            m_area.Push(ENavArea{});
            fp.Read(&m_area[i].index, sizeof(uint16_t));
            fp.Read(&m_area[i].flags, sizeof(uint32_t));
            fp.Read(&m_area[i].connectionCount, sizeof(uint8_t));
            safeloc(m_area[i].connection, m_area[i].connectionCount);
            fp.Read(m_area[i].connection, m_area[i].connectionCount * sizeof(uint16_t));
            fp.Read(&m_area[i].direction, 4 * sizeof(float));
            fp.Read(&m_area[i].dirHeight, 4 * sizeof(float));
        }

        fp.Close();
    }

    return true;
}

void ENavMesh::SaveNav(void)
{
    ENavHeader header;
    header.fileVersion = 1;
    header.navNumber = g_numNavAreas;
    File fp(CheckSubfolderFile(), "wb");

    // file was opened
    if (fp.IsValid())
    {
        // write the nav header to the file...
        fp.Write(&header, sizeof(header));

        uint16_t i;
        for (i = 0; i < g_numNavAreas; i++)
        {
            fp.Write(&m_area[i].index, sizeof(uint16_t));
            fp.Write(&m_area[i].flags, sizeof(uint32_t));
            fp.Write(&m_area[i].connectionCount, sizeof(uint8_t));
            fp.Write(&m_area[i].connection, m_area[i].connectionCount * sizeof(uint16_t));
            fp.Write(&m_area[i].direction, 4 * sizeof(float));
            fp.Write(&m_area[i].dirHeight, 4 * sizeof(float));
        }

        fp.Close();
    }
    else
        AddLogEntry(Log::Error, "writing '%s' navmesh file, missing navigations file, create one", GetMapName());
}

ENavArea ENavMesh::GetNavArea(const uint16_t id)
{
    return m_area[id];
}

ENavArea* ENavMesh::GetNavAreaP(const uint16_t id)
{
    return &m_area[id];
}

ENavMesh::ENavMesh(void)
{
    g_numNavAreas = 0;
    m_selectedNavIndex = -1;
}

ENavMesh::~ENavMesh(void)
{
    m_area.Destroy();
    g_numNavAreas = 0;
    m_selectedNavIndex = -1;
}