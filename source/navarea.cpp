#include <core.h>

Vector ENavArea::GetClosestPosition(const Vector origin)
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

Vector ENavArea::GetFarestPosition(const Vector origin)
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

Vector ENavArea::GetCornerPosition(const uint_fast8_t corner)
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

Vector ENavArea::AreaDirectionAsVector(const uint_fast8_t corner)
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

bool ENavArea::IsPointOverlapping(const Vector origin)
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

ENavDir GetDirection(const Vector start, const Vector end)
{
    const Vector diff = end - start;
    const float radian = catan2f(diff.y, diff.x);
    float degree = (radian * 180.0f) * 0.318310155049f;
    if (degree < 0.0f)
        degree += 360.0f;

    const int result = static_cast<int>((degree + 45.0f) * 0.0111111111111f) % ENavDir::NumDir;
    return static_cast<ENavDir>(result);
}

void ENavArea::MergeWith(const ENavArea area)
{
    if (direction[ENavDir::Left] > area.direction[ENavDir::Left])
    {
        direction[ENavDir::Left] = area.direction[ENavDir::Left];
        dirHeight[ENavDir::Left] = area.dirHeight[ENavDir::Left];
    }

    if (direction[ENavDir::Right] < area.direction[ENavDir::Right])
    {
        direction[ENavDir::Right] = area.direction[ENavDir::Right];
        dirHeight[ENavDir::Right] = area.dirHeight[ENavDir::Right];
    }

    if (direction[ENavDir::Backward] > area.direction[ENavDir::Backward])
    {
        direction[ENavDir::Backward] = area.direction[ENavDir::Backward];
        dirHeight[ENavDir::Backward] = area.dirHeight[ENavDir::Backward];
    }

    if (direction[ENavDir::Forward] < area.direction[ENavDir::Forward])
    {
        direction[ENavDir::Forward] = area.direction[ENavDir::Forward];
        dirHeight[ENavDir::Forward] = area.dirHeight[ENavDir::Forward];
    }

    ENavArea temp;
    uint16_t i, j;
    for (i = 0; i < g_numNavAreas; i++)
    {
        temp = g_navmesh->GetNavArea(i);
        if (area.index == temp.index)
        {
            for (j = 0; j < area.connectionCount; j++)
            {
                g_navmesh->ConnectArea(index, area.connection[j]);
                if (area.connection[j] == area.index)
                    g_navmesh->ConnectArea(area.connection[j], index);
            }
        }
        else
        {
            for (j = 0; j < temp.connectionCount; j++)
            {
                if (temp.connection[j] != area.index)
                    continue;
                
                g_navmesh->ConnectArea(temp.connection[j], index);
            }
        }
    }

    g_navmesh->DeleteArea(area);
}

void ENavArea::ExpandNavArea(const uint_fast8_t radius)
{
    Vector temp, temp2;
    uint_fast8_t i, rad = radius;
    uint16_t j;
    ENavArea area;

    while (rad > 0)
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