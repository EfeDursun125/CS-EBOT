#include <core.h>
#include <compress.h>
#include <vector>
#include <xmmintrin.h>
#include <emmintrin.h>

void NavMesh::Initialize(void)
{
    if (m_navmeshLoaded)
    {
        for (int i = 0; i < g_numNavAreas; i++)
        {
            if (m_area[i] == nullptr)
                continue;

            delete m_area[i];
            m_area[i] = nullptr;
        }
    }

    g_numNavAreas = 0;
}

void NavMesh::CreateAreaAuto(const Vector start, const Vector goal)
{
    if (!IsWalkableTraceLineClear(start + Vector(0.0f, 0.0f, 10.0f), goal + Vector(0.0f, 0.0f, 10.0f)))
        return;

    auto nav1 = GetNearestNavArea(start);
    auto nav2 = GetNearestNavArea(goal);

    if (nav1 == nav2)
        return;

    if (DoNavAreasIntersect(nav1, nav2))
        return;

    CreateArea(goal);
}

void NavMesh::Analyze(void)
{
    if (g_numNavAreas <= 0)
        return;

    if (!FNullEnt(g_hostEntity))
    {
        char message[] =
            "+-----------------------------------------------+\n"
            " Analyzing the map for walkable places \n"
            "+-----------------------------------------------+\n";

        HudMessage(g_hostEntity, true, Color(100, 100, 255), message);
    }
    else if (!IsDedicatedServer()) // let the player join first...
        return;

    extern ConVar ebot_analyzer_min_fps;

    static float magicTimer;
    for (int i = 0; i < g_numNavAreas; i++)
    {
        if (g_expanded[i])
            continue;

        if (magicTimer >= engine->GetTime())
            continue;

        if ((ebot_analyzer_min_fps.GetFloat() + g_pGlobals->frametime) <= 1.0f / g_pGlobals->frametime)
            magicTimer = engine->GetTime() + g_pGlobals->frametime * 0.054f; // pause

        if (m_area[i] == nullptr)
            continue;

        Vector WayVec = GetCenter(m_area[i]);
        float range = engine->RandomFloat(200.0f, 1000.0f);

        for (int dir = 0; dir < 4; dir++)
        {
            switch (dir)
            {
            case 1:
            {
                Vector Next;
                Next.x = WayVec.x + range;
                Next.y = WayVec.y;
                Next.z = WayVec.z;

                CreateAreaAuto(WayVec, Next);
            }
            case 2:
            {
                Vector Next;
                Next.x = WayVec.x - range;
                Next.y = WayVec.y;
                Next.z = WayVec.z;

                CreateAreaAuto(WayVec, Next);
            }
            case 3:
            {
                Vector Next;
                Next.x = WayVec.x;
                Next.y = WayVec.y + range;
                Next.z = WayVec.z;

                CreateAreaAuto(WayVec, Next);
            }
            case 4:
            {
                Vector Next;
                Next.x = WayVec.x;
                Next.y = WayVec.y - range;
                Next.z = WayVec.z;

                CreateAreaAuto(WayVec, Next);
            }
            case 5:
            {
                Vector Next;
                Next.x = WayVec.x + range;
                Next.y = WayVec.y;
                Next.z = WayVec.z + 128.0f;

                CreateAreaAuto(WayVec, Next);
            }
            case 6:
            {
                Vector Next;
                Next.x = WayVec.x - range;
                Next.y = WayVec.y;
                Next.z = WayVec.z + 128.0f;

                CreateAreaAuto(WayVec, Next);
            }
            case 7:
            {
                Vector Next;
                Next.x = WayVec.x;
                Next.y = WayVec.y + range;
                Next.z = WayVec.z + 128.0f;

                CreateAreaAuto(WayVec, Next);
            }
            case 8:
            {
                Vector Next;
                Next.x = WayVec.x;
                Next.y = WayVec.y - range;
                Next.z = WayVec.z + 128.0f;

                CreateAreaAuto(WayVec, Next);
            }
            }
        }

        g_expanded[i] = true;
    }

    if (magicTimer + 2.0f < engine->GetTime())
    {

    }
}

float EuclideanDistance(const Vector start, const Vector goal)
{
    const float x = fabsf(start.x - goal.x);
    const float y = fabsf(start.y - goal.y);
    const float z = fabsf(start.z - goal.z);
    const float euclidean = squareRoot(power(x, 2.0f) + power(y, 2.0f) + power(z, 2.0f));
    return 1000.0f * (ceilf(euclidean) - euclidean);
}

void NavMesh::ConnectArea(NavArea* start, NavArea* goal)
{
    if (start == nullptr)
        return;

    if (goal == nullptr)
        return;

    if (start == goal)
        return;

    // don't allow paths get connected twice
    for (int i = 0; i < start->connections.GetElementNumber(); i++)
    {
        if (start->connections[i] == goal)
            return;
    }
    
    start->connections.Push(goal);
    start->bakedDist.Push(EuclideanDistance(GetCenter(start), GetCenter(goal)));
    PlaySound(g_hostEntity, "weapons/mine_activate.wav");
}

void NavMesh::DisconnectArea(NavArea* start, NavArea* goal)
{
    for (int i = 0; i < start->connections.GetElementNumber(); i++)
    {
        NavArea* connection = start->connections[i];
        if (connection == nullptr)
            continue;

        if (connection == goal)
        {
            start->connections.RemoveAt(i);
            start->bakedDist.RemoveAt(i); // should be same with connection
            PlaySound(g_hostEntity, "weapons/mine_activate.wav");
            break;
        }
    }
}

NavArea* NavMesh::CreateArea(const Vector origin)
{
    if (origin == nullvec)
        return nullptr;

    int index = -1;
    NavArea* area = nullptr;

    if (g_numNavAreas >= Const_MaxWaypoints)
        return nullptr;

    index = g_numNavAreas;

    m_area[index] = new NavArea;
    if (m_area[index] == nullptr)
    {
        AddLogEntry(LOG_MEMORY, "unexpected memory error");
        return nullptr;
    }

    area = m_area[index];
    if (area == nullptr)
    {
        AddLogEntry(LOG_MEMORY, "unexpected memory error");
        return nullptr;
    }

    area->index = index;

    g_numNavAreas++;

    float size = 10.0f;
    area->corners[0] = origin + Vector(size, size, 0.0f);
    area->corners[1] = origin + Vector(-size, size, 0.0f);
    area->corners[2] = origin + Vector(-size, -size, 0.0f);
    area->corners[3] = origin + Vector(size, -size, 0.0f);

    area->connections.Destory();
    area->bakedDist.Destory();

    ExpandNavArea(area, 50.0f);

    for (int j = 0; j < g_numNavAreas; j++)
    {
        if (m_area[j] == area)
            continue;

        if (IsWalkableTraceLineClear(GetCenter(area), GetCenter(m_area[j])) ? DoNavAreasIntersect(area, m_area[j], 0.33f) : DoNavAreasIntersect(area, m_area[j], 0.15f))
        {
            ConnectArea(area, m_area[j]);

            // TODO Add height or fall check
            ConnectArea(m_area[j], area);
        }
    }

    PlaySound(g_hostEntity, "weapons/xbow_hit1.wav");
    return area;
}

void NavMesh::DeleteArea(NavArea* area)
{
    if (g_numNavAreas < 1)
        return;

    if (area == nullptr)
        return;

    const int index = area->index;
    InternalAssert(m_area[index] != nullptr);

    for (int i = 0; i < area->connections.GetElementNumber(); i++)
    {
        NavArea* connection = area->connections[i];
        if (connection == nullptr)
            continue;

        for (int j = 0; j < connection->connections.GetElementNumber(); j++)
        {
            if (connection->connections[j] == area)
            {
                connection->connections.RemoveAt(j);
                connection->bakedDist.RemoveAt(j); // should be same with connection
            }
        } 
    }

    for (int i = 0; i < g_numNavAreas; i++)
    {
        auto path = m_area[i];

        if (path->index > index)
            path->index--;
    }

    delete m_area[index];
    m_area[index] = nullptr;

    for (int i = index; i < g_numNavAreas - 1; i++)
        m_area[i] = m_area[i + 1];

    g_numNavAreas--;
    PlaySound(g_hostEntity, "weapons/mine_activate.wav");
}

void NavMesh::ExpandNavArea(NavArea* area, const float radius)
{
    if (area == nullptr)
        return;

    const float units = 0.5f;
    const float maxHeight = 62.0f;

    for (float dist = units; dist <= radius; dist += units)
    {
        for (int i = 0; i < 4; i++)
        {
            const Vector& corner = area->corners[i];
            const Vector& temp = GetWalkablePosition((corner + (corner - GetCenter(area)).Normalize() * dist) + Vector(0.0f, 0.0f, maxHeight * 0.5f), g_hostEntity, true, maxHeight);

            if (temp == nullvec)
                continue;

            int C = 0;
            while (C <= 2)
            {
                C++;

                Vector newCorner;

                if (C == 2)
                {
                    newCorner.x = temp.x;
                    newCorner.y = corner.y;
                }
                else
                {
                    newCorner.x = corner.x;
                    newCorner.y = temp.y;
                }

                newCorner.z = temp.z;

                if (!IsWalkableTraceLineClear(corner + Vector(0.0f, 0.0f, 10.0f), newCorner + Vector(0.0f, 0.0f, 10.0f)))
                    continue;

                bool intersect = false;
                for (int j = 0; j < g_numNavAreas; j++)
                {
                    if (m_area[j] == area)
                        continue;

                    if (DoNavAreasIntersect(area, m_area[j]))
                    {
                        intersect = true;
                        break;
                    }
                }

                if (intersect)
                    continue;

                bool stop = false;

                for (int j = 0; j < 4; j++)
                {
                    if (i == j)
                        continue;

                    const Vector& otherCorners = area->corners[j];

                    if (!IsWalkableTraceLineClear(newCorner + Vector(0.0f, 0.0f, 10.0f), otherCorners + Vector(0.0f, 0.0f, 10.0f)))
                    {
                        stop = true;
                        continue;
                    }

                    if (!IsWalkableTraceLineClear(GetCenter(area) + Vector(0.0f, 0.0f, 10.0f), otherCorners + Vector(0.0f, 0.0f, 10.0f)))
                    {
                        stop = true;
                        continue;
                    }
                }

                if (!stop)
                    area->corners[i] = newCorner;
            }
        }
    }
}

void NavMesh::OptimizeNavMesh(void)
{
    /*for (int i = 0; i < g_numNavAreas; i++)
    {
        auto area = GetNavArea(i);
        if (area == nullptr)
            continue;

        Array<NavArea*> mergeList;
        mergeList.Push(area);

        for (int j = 0; j < area->connections.GetElementNumber(); j++)
        {
            auto newArea = area->connections[j];
            if (newArea == nullptr)
                continue;

            bool hit = false;
            for (int m = 0; m < 4; m++)
            {
                if (hit)
                    break;

                for (int k = 0; k < 4; k++)
                {
                    if (!IsWalkableTraceLineClear(area->corners[m], newArea->corners[j]))
                    {
                        hit = true;
                        break;
                    }
                }
            }

            if (!hit)
                mergeList.Push(newArea);
        }

        Vector corner1, corner2;
        GetFarthestCorners(area, corner1, corner2);

        for (int j = 0; j < mergeList.GetElementNumber(); j++)
        {
            NavArea* target;
            mergeList.GetAt(j, target);

            if (target == nullptr)
                continue;
        }
    }*/
}

// this function returns the squared distance between a point and a line segment
float DistanceToLineSegmentSquared(const Vector& point, const Vector& start, const Vector& end)
{
    Vector direction = end - start;
    float lengthSquared = direction.GetLengthSquared();

    // if the line segment has zero length, the point cannot be closer to it
    if (lengthSquared == 0.0f)
        return (point - start).GetLengthSquared();

    // calculate the projection of the point onto the line segment
    Vector toPoint = point - start;
    float projection = DotProduct(toPoint, direction) / lengthSquared;
    projection = Clamp(projection, 0.0f, 1.0f);
    Vector closestPoint = start + projection * direction;

    // calculate the squared distance between the point and the closest point on the line segment
    return (point - closestPoint).GetLengthSquared();
}

// this function returns the closest point on a line segment to a given point
Vector ClosestPointOnLineSegment(const Vector& point, const Vector& start, const Vector& end)
{
    Vector direction = end - start;
    float lengthSquared = direction.GetLengthSquared();

    // if the line segment has zero length, return either endpoint
    if (lengthSquared == 0.0f)
        return start;

    // calculate the projection of the point onto the line segment
    Vector toPoint = point - start;
    float projection = DotProduct(toPoint, direction) / lengthSquared;
    projection = Clamp(projection, 0.0f, 1.0f);
    Vector closestPoint = start + projection * direction;

    return closestPoint;
}

bool LineSegmentIntersect(const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4, const float tolerance)
{
    const float denominator = ((p4.y - p3.y) * (p2.x - p1.x)) - ((p4.x - p3.x) * (p2.y - p1.y));
    if (denominator == 0.0f)
        return false;

    const float ua = (((p4.x - p3.x) * (p1.y - p3.y)) - ((p4.y - p3.y) * (p1.x - p3.x))) / denominator;
    const float ub = (((p2.x - p1.x) * (p1.y - p3.y)) - ((p2.y - p1.y) * (p1.x - p3.x))) / denominator;

    const float high = 1.0f + tolerance;
    const float low = -tolerance;

    // check if the intersection point is within the bounds of both line segments
    if (ua >= low && ua <= high && ub >= low && ub <= high)
    {
        const float z1 = p1.z + ua * (p2.z - p1.z);
        const float z2 = p3.z + ub * (p4.z - p3.z);

        // check if the height difference between the two line segments is within tolerance
        const float heightTolerance = 0.44f;
        if (fabsf(z1 - z2) <= heightTolerance)
            return true;
    }

    return false;
}

bool NavMesh::DoNavAreasIntersect(NavArea* area1, NavArea* area2, const float tolerance)
{
    if (area1 == nullptr)
        return false;

    if (area2 == nullptr)
        return false;

    // if both areas are same they use GetNearestNavArea
    if (area1 == area2)
        return false;

    // check if the sides of the areas intersect
    for (int i = 0; i < 4; i++)
    {
        const Vector& p1 = area1->corners[i];
        const Vector& p2 = area1->corners[(i + 1) % 4];

        for (int j = 0; j < 4; j++)
        {
            const Vector& p3 = area2->corners[j];
            const Vector& p4 = area2->corners[(j + 1) % 4];

            // check if the line segments intersect
            if (LineSegmentIntersect(p1, p2, p3, p4, tolerance))
                return true;
        }
    }

    return false;
}

NavArea* NavMesh::GetNearestNavArea(const Vector origin)
{
    NavArea* area = nullptr;
    float minDist = FLT_MAX;

    // for each NavArea in the NavMesh
    for (int i = 0; i < g_numNavAreas; i++)
    {
        NavArea* navArea = m_area[i];
        if (navArea == nullptr)
            continue;

        // compute the squared distance from the given origin to the NavArea's center
        const float dist = (GetClosestPosition(navArea, origin) - origin).GetLengthSquared();

        // if this NavArea is closer than the previous closest NavArea
        if (dist < minDist)
        {
            // update the closest NavArea and its minimum distance
            area = navArea;
            minDist = dist;
        }
    }

    return area;
}

Vector NavMesh::GetCenter(NavArea* area)
{
    if (area == nullptr)
        return nullvec;

    return (area->corners[0] + area->corners[1] + area->corners[2] + area->corners[3]) * 0.25f;
}

Vector NavMesh::GetCornerPosition(NavArea* area, int corner)
{
    if (area == nullptr)
        return nullvec;

    if (corner < 0 || corner > 3)
        corner = engine->RandomInt(0, 3);

    return area->corners[corner];
}

Vector NavMesh::GetRandomPosition(NavArea* area)
{
    if (area == nullptr)
        return nullvec;

    // pick a random point inside the NavArea's bounds
    float randX = engine->RandomFloat(area->corners[1].x, area->corners[3].x);
    float randY = engine->RandomFloat(area->corners[1].y, area->corners[3].y);

    // use the NavArea's center for the Z-coordinate
    float randZ = (area->corners[0].z + area->corners[1].z + area->corners[2].z + area->corners[3].z) * 0.25f;

    return Vector(randX, randY, randZ);
}

Vector NavMesh::GetClosestPosition(NavArea* area, const Vector origin)
{
    if (area == nullptr)
        return origin;

    Vector closestPoint;
    float closestDist = FLT_MAX;

    for (int i = 0; i < 4; i++)
    {
        Vector edgeStart = area->corners[i];
        Vector edgeEnd = area->corners[(i + 1) % 4];

        // calculate the distance from the origin to the edge
        const float dist = DistanceToLineSegmentSquared(origin, edgeStart, edgeEnd);
        if (dist < closestDist)
        {
            closestDist = dist;
            closestPoint = ClosestPointOnLineSegment(origin, edgeStart, edgeEnd);
        }
    }

    return closestPoint;
}

Vector NavMesh::GetAimPosition(void)
{
    if (FNullEnt(g_hostEntity))
        return nullvec;

    TraceResult tr;
    const auto eyePosition = g_hostEntity->v.origin + g_hostEntity->v.view_ofs;
    MakeVectors(g_hostEntity->v.v_angle);
    TraceLine(eyePosition, eyePosition + g_pGlobals->v_forward * 1000.0f, false, false, g_hostEntity, &tr);

    return tr.vecEndPos;
}

void NavMesh::DrawNavArea(void)
{
    if (FNullEnt(g_hostEntity))
        return;

    Vector aimPosition = GetAimPosition();
    NavArea* nearestArea = GetNearestNavArea(aimPosition);
    
    const float size = 10.0f;
    engine->DrawLine(g_hostEntity, aimPosition + Vector(size, 0.0f, 0.0f), aimPosition + Vector(-size, 0.0f, 0.0f), Color(255, 255, 255, 255), 10, 0, 5, 1, LINE_SIMPLE);
    engine->DrawLine(g_hostEntity, aimPosition + Vector(0.0f, size, 0.0f), aimPosition + Vector(0.0f, -size, 0.0f), Color(255, 255, 255, 255), 10, 0, 5, 1, LINE_SIMPLE);
    engine->DrawLine(g_hostEntity, aimPosition + Vector(0.0f, 0.0f, size), aimPosition + Vector(0.0f, 0.0f, -size), Color(255, 255, 255, 255), 10, 0, 5, 1, LINE_SIMPLE);

    Vector height = Vector(0.0f, 0.0f, 5.0f);

    if (nearestArea == nullptr)
        return;

    for (int i = 0; i < 4; i++)
        engine->DrawLine(g_hostEntity, nearestArea->corners[i] + height, nearestArea->corners[(i + 1) % 4] + height, Color(0, 255, 0, 255), 10, 0, 5, 1, LINE_SIMPLE);

    for (int i = 0; i < nearestArea->connections.GetElementNumber(); i++)
    {
        NavArea* neighbor = nearestArea->connections[i];
        if (neighbor == nullptr)
            continue;

        for (int j = 0; j < 4; j++)
            engine->DrawLine(g_hostEntity, neighbor->corners[j] + height, neighbor->corners[(j + 1) % 4] + height, Color(0, 100, 255, 255), 10, 0, 5, 1, LINE_SIMPLE);

        engine->DrawLine(g_hostEntity, GetCenter(nearestArea) + height, GetCenter(neighbor) + height, Color(255, 255, 255, 255), 10, 0, 5, 1, LINE_SIMPLE);

        for (int j = 0; j < neighbor->connections.GetElementNumber(); j++)
        {
            NavArea* more = neighbor->connections[j];
            if (more == nullptr)
                continue;

            if (more == neighbor)
                continue;

            if (more == nearestArea)
                continue;

            for (int k = 0; k < 4; k++)
                engine->DrawLine(g_hostEntity, more->corners[k] + height, more->corners[(k + 1) % 4] + height, Color(0, 100, 255, 255), 10, 0, 5, 1, LINE_SIMPLE);
            
            engine->DrawLine(g_hostEntity, GetCenter(neighbor) + height, GetCenter(more) + height, Color(255, 255, 255, 255), 10, 0, 5, 1, LINE_SIMPLE);
        }
    }
}

bool NavMesh::IsConnected(int start, int goal)
{
    NavArea* startArea = m_area[start];
    if (startArea == nullptr)
        return false;

    for (int i = 0; i < startArea->connections.GetElementNumber(); i++)
    {
        if (i == goal)
            return true;
    }

    return false;
}

String NavMesh::CheckSubfolderFile(void)
{
    String returnFile = "";
    returnFile = FormatBuffer("%s/%s.nav", FormatBuffer("%s/addons/ebot/navigations/", GetModName()), GetMapName());

    if (TryFileOpen(returnFile))
        return returnFile;

    return FormatBuffer("%s%s.nav", FormatBuffer("%s/addons/ebot/navigations/", GetModName()), GetMapName());
}

bool NavMesh::LoadNav(void)
{
    NavHeader header;
    File fp(CheckSubfolderFile(), "rb");

    if (fp.IsValid())
    {
        fp.Read(&header, sizeof(header));
        
        Initialize();
        g_numNavAreas = header.navNumber;
        
        for (int i = 0; i < g_numNavAreas; i++)
        {
            m_area[i] = new NavArea;
            if (m_area[i] == nullptr)
            {
                AddLogEntry(LOG_MEMORY, "unexpected memory error");
                return false;
            }
            
            fp.Read(m_area[i], sizeof(NavArea));

            // because they're waypoint based bots
            g_waypoint->Add(-1, GetCenter(m_area[i]) + Vector(0.0f, 0.0f, 35.0f));
        }

        m_navmeshLoaded = true;
    }

    return true;
}

void NavMesh::SaveNav(void)
{
    NavHeader header;
    memset(header.author, 0, sizeof(header.author));

    char navAuthor[32];

    if (!FNullEnt(g_hostEntity))
        sprintf(navAuthor, "%s", GetEntityName(g_hostEntity));
    else
        sprintf(navAuthor, "E-Bot NavMesh Analyzer");

    strcpy(header.author, navAuthor);

    // remember the original author
    File rf(CheckSubfolderFile(), "rb");
    if (rf.IsValid())
    {
        rf.Read(&header, sizeof(header));
        rf.Close();
    }

    header.fileVersion = 1;
    header.navNumber = g_numNavAreas;

    File fp(CheckSubfolderFile(), "wb");

    // file was opened
    if (fp.IsValid())
    {
        // write the nav header to the file...
        fp.Write(&header, sizeof(header), 1);

        // save the nav paths...
        for (int i = 0; i < g_numNavAreas; i++)
            fp.Write(m_area[i], sizeof(NavArea));

        fp.Close();
    }
    else
        AddLogEntry(LOG_ERROR, "Error writing '%s' navmesh file", GetMapName());
}

NavArea* NavMesh::GetNavArea(int id)
{
    if (id < 0 || id > g_numNavAreas)
        return m_area[id];

    return m_area[id];
}

NavMesh::NavMesh(void)
{
    m_navmeshLoaded = false;
}

NavMesh::~NavMesh(void)
{

}