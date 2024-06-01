#include <core.h>

Vector ENavArea::GetClosestPosition(const Vector& origin)
{
    Vector finalOrigin = origin;

    if (origin.x < direction[ENavDir::Left])
        finalOrigin.x = direction[ENavDir::Left];
    else if (origin.x > direction[ENavDir::Right])
        finalOrigin.x = direction[ENavDir::Right];

    if (origin.y < direction[ENavDir::Backward])
        finalOrigin.y = direction[ENavDir::Backward];
    else if (origin.y > direction[ENavDir::Forward])
        finalOrigin.y = direction[ENavDir::Forward];

    if (origin.z < dirHeight[ENavDir::Left])
        finalOrigin.z = dirHeight[ENavDir::Left];
    else if (origin.z > dirHeight[ENavDir::Right])
        finalOrigin.z = dirHeight[ENavDir::Right];

    if (origin.z < dirHeight[ENavDir::Backward])
        finalOrigin.z = dirHeight[ENavDir::Backward];
    else if (origin.z > dirHeight[ENavDir::Forward])
        finalOrigin.z = dirHeight[ENavDir::Forward];

    return finalOrigin;
}

Vector ENavArea::GetFarestPosition(const Vector& origin)
{
    Vector finalOrigin = origin;

    if (origin.x > direction[ENavDir::Left])
        finalOrigin.x = direction[ENavDir::Left];
    else if (origin.x < direction[ENavDir::Right])
        finalOrigin.x = direction[ENavDir::Right];

    if (origin.y > direction[ENavDir::Backward])
        finalOrigin.y = direction[ENavDir::Backward];
    else if (origin.y < direction[ENavDir::Forward])
        finalOrigin.y = direction[ENavDir::Forward];

    if (origin.z > dirHeight[ENavDir::Left])
        finalOrigin.z = dirHeight[ENavDir::Left];
    else if (origin.z < dirHeight[ENavDir::Right])
        finalOrigin.z = dirHeight[ENavDir::Right];

    if (origin.z > dirHeight[ENavDir::Backward])
        finalOrigin.z = dirHeight[ENavDir::Backward];
    else if (origin.z < dirHeight[ENavDir::Forward])
        finalOrigin.z = dirHeight[ENavDir::Forward];

    return finalOrigin;
}

Vector ENavArea::GetCenter(void)
{
    return Vector((direction[0] + direction[1]) * 0.5f, (direction[2] + direction[3]) * 0.5f, (dirHeight[0] + dirHeight[1] + dirHeight[2] + dirHeight[3]) * 0.25f);
}

// this function picks a random point inside the NavArea's bounds
Vector ENavArea::GetRandomPosition(void)
{
    return Vector(crandomfloat(direction[0], direction[1]), crandomfloat(direction[2], direction[3]), (dirHeight[0] + dirHeight[1] + dirHeight[2] + dirHeight[3]) * 0.25f);
}

Vector ENavArea::GetCornerPosition(const uint8_t corner)
{
    switch (corner)
    {
    case ENavDir::Right:
        return Vector(direction[ENavDir::Right], direction[ENavDir::Forward], dirHeight[ENavDir::Right]);
    case ENavDir::Left:
        return Vector(direction[ENavDir::Left], direction[ENavDir::Backward], dirHeight[ENavDir::Left]);
    case ENavDir::Forward:
        return Vector(direction[ENavDir::Left], direction[ENavDir::Forward], dirHeight[ENavDir::Forward]);
    case ENavDir::Backward:
        return Vector(direction[ENavDir::Right], direction[ENavDir::Backward], dirHeight[ENavDir::Backward]);
    }

    return Vector(0.0f, 0.0f, 0.0f);
}

Vector ENavArea::AreaDirectionAsVector(const uint8_t corner)
{
    switch (corner)
    {
    case ENavDir::Right:
        return Vector(direction[ENavDir::Right], 0.0f, 0.0f);
    case ENavDir::Left:
        return Vector(direction[ENavDir::Left], 0.0f, 0.0f);
    case ENavDir::Forward:
        return Vector(0.0f, direction[ENavDir::Forward], 0.0f);
    case ENavDir::Backward:
        return Vector(0.0f, direction[ENavDir::Backward], 0.0f);
    }

    return Vector(0.0f, 0.0f, 0.0f);
}

bool ENavArea::IsAreaOverlapping(const ENavArea area)
{
    if (direction[ENavDir::Left] > area.direction[ENavDir::Right])
        return false;

    if (direction[ENavDir::Right] < area.direction[ENavDir::Left])
        return false;

    if (direction[ENavDir::Backward] > area.direction[ENavDir::Forward])
        return false;

    if (direction[ENavDir::Forward] < area.direction[ENavDir::Backward])
        return false;

    if (dirHeight[ENavDir::Right] < area.dirHeight[ENavDir::Left] && dirHeight[ENavDir::Right] < area.dirHeight[ENavDir::Left] && dirHeight[ENavDir::Backward] < area.dirHeight[ENavDir::Backward] && dirHeight[ENavDir::Forward] < area.dirHeight[ENavDir::Forward])
        return false;

    if (dirHeight[ENavDir::Left] > area.dirHeight[ENavDir::Right] && dirHeight[ENavDir::Right] > area.dirHeight[ENavDir::Left] && dirHeight[ENavDir::Backward] > area.dirHeight[ENavDir::Backward] && dirHeight[ENavDir::Forward] > area.dirHeight[ENavDir::Forward])
        return false;

    return true;
}

bool ENavArea::IsPointOverlapping(const Vector& origin)
{
    if (direction[ENavDir::Left] < origin.x)
        return false;

    if (direction[ENavDir::Right] > origin.x)
        return false;

    if (direction[ENavDir::Backward] < origin.y)
        return false;

    if (direction[ENavDir::Forward] > origin.y)
        return false;

    if (dirHeight[ENavDir::Left] < origin.z && dirHeight[ENavDir::Right] < origin.z && dirHeight[ENavDir::Backward] < origin.z && dirHeight[ENavDir::Forward] < origin.z)
        return false;

    if (dirHeight[ENavDir::Right] > origin.z && dirHeight[ENavDir::Left] > origin.z && dirHeight[ENavDir::Backward] > origin.z && dirHeight[ENavDir::Forward] > origin.z)
        return false;

    return true;
}

ENavDir GetDirection(const Vector& start, const Vector& end)
{
    const Vector diff = end - start;
    float degree = (catan2f(diff.y, diff.x) * 180.0f) * 0.318310155049f;
    if (degree < 0.0f)
        degree += 360.0f;

    return static_cast<ENavDir>(static_cast<int>((degree + 45.0f) * 0.0111111111111f) % ENavDir::NumDir);
}

void ENavMesh::MergeNavAreas(const uint16_t area1, const uint16_t area2)
{
    ENavArea* a1 = &m_area[area1];
    ENavArea* a2 = &m_area[area2];

    // try to allocate memory, otherwise don't merge
    uint16_t* newConnections = new(std::nothrow) uint16_t[a1->connectionCount + a2->connectionCount];
    if (!newConnections)
        return;

    uint8_t newConnectionCount = 0;

    // add connections from a1
    uint8_t i, j;
    bool exists;
    for (i = 0; i < a1->connectionCount; i++)
    {
        exists = false;
        for (j = 0; j < newConnectionCount; j++)
        {
            if (newConnections[j] == a1->connection[i])
            {
                exists = true;
                break;
            }
        }

        if (!exists)
            newConnections[newConnectionCount++] = a1->connection[i];
    }

    // add other connections from a2
    for (i = 0; i < a2->connectionCount; i++)
    {
        if (a2->connection[i] == area1)
            continue;

        exists = false;
        for (j = 0; j < newConnectionCount; j++)
        {
            if (newConnections[j] == a2->connection[i])
            {
                exists = true;
                break;
            }
        }

        if (!exists)
            newConnections[newConnectionCount++] = a2->connection[i];
    }

    // increase size and height of the area to make it look like merged
    if (a1->direction[ENavDir::Left] > a2->direction[ENavDir::Left])
    {
        a1->direction[ENavDir::Left] = a2->direction[ENavDir::Left];
        a1->dirHeight[ENavDir::Left] = a2->dirHeight[ENavDir::Left];
    }

    if (a1->direction[ENavDir::Right] < a2->direction[ENavDir::Right])
    {
        a1->direction[ENavDir::Right] = a2->direction[ENavDir::Right];
        a1->dirHeight[ENavDir::Right] = a2->dirHeight[ENavDir::Right];
    }

    if (a1->direction[ENavDir::Backward] > a2->direction[ENavDir::Backward])
    {
        a1->direction[ENavDir::Backward] = a2->direction[ENavDir::Backward];
        a1->dirHeight[ENavDir::Backward] = a2->dirHeight[ENavDir::Backward];
    }

    if (a1->direction[ENavDir::Forward] < a2->direction[ENavDir::Forward])
    {
        a1->direction[ENavDir::Forward] = a2->direction[ENavDir::Forward];
        a1->dirHeight[ENavDir::Forward] = a2->dirHeight[ENavDir::Forward];
    }

    // update connections for other areas
    uint16_t k;
    for (k = 0; k < g_numNavAreas; k++)
    {
        a2 = &m_area[k];
        for (j = 0; j < a2->connectionCount; j++)
        {
            if (a2->connection[j] == area2)
                a2->connection[j] = area1;
        }
    }

    // update connections
    delete[] a1->connection;
    a1->connection = newConnections;
    a1->connectionCount = newConnectionCount;

    // remove merged area
    DeleteAreaByIndex(area2);

    // notify that it was merged
    PlaySound(g_hostEntity, "weapons/mine_activate.wav");
}

void ENavArea::ExpandNavArea(const uint8_t radius)
{
    Vector temp, temp2;
    uint8_t i, rad = radius;
    uint16_t j;
    ENavArea area;

    while (rad)
    {
        for (i = 0; i < ENavDir::NumDir; i++)
        {
            temp = GetWalkablePosition(GetCornerPosition(i), g_hostEntity, true, 50.0f);
            if (temp == nullvec)
                continue;

            temp.z += 10.0f;
            temp2 = temp + Vector(0.0f, 0.0f, 10.0f) + g_navmesh->DirectionAsVector(i, GridSize);
            temp2 = GetWalkablePosition(temp2, g_hostEntity, true, 200.0f);
            if (temp2 == nullvec)
                continue;

            temp2.z += 10.0f;

            if (!IsWalkableLineClear(temp, temp2))
                continue;

            if (!IsWalkableLineClear(GetCenter(), temp2))
                continue;

            for (j = 0; j < g_numNavAreas; j++)
            {
                area = g_navmesh->GetNavArea(j);
                if (index == area.index)
                    continue;

                if (area.IsPointOverlapping(temp2))
                    continue;

                if (area.IsAreaOverlapping(*this))
                    continue;
            }
            
            direction[i] += g_navmesh->DirectionAsFloat(i);
            dirHeight[i] = temp2.z;
        }

        rad -= 1;
    }
}