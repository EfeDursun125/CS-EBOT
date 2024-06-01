#define GridSize 24.0f

struct ENavHeader
{
	int16_t fileVersion{};
	int16_t navNumber{};
};

enum ENavFlag
{
	NAV_JUMP = (1 << 1),
	NAV_CROUCH = (1 << 2),
	NAV_LADDER = (1 << 3),
	NAV_DONT_HIDE = (1 << 4),
	NAV_DONT_JUMP = (1 << 5),
	NAV_DONT_WALK = (1 << 6),
	NAV_DONT_RUN = (1 << 7),
	NAV_AVOID = (1 << 8),
	NAV_FALL_RISK = (1 << 9),
	NAV_GROUND_CHECK = (1 << 10),
	NAV_LOS_CHECK = (1 << 11),
	NAV_ENTITY_CHECK = (1 << 12),
	NAV_LOOK_AHEAD = (1 << 13),
	NAV_NO_COST = (1 << 14)
};

enum ENavDir
{
	Right = 0,
	Left = 1,
	Forward = 2,
	Backward = 3,
	NumDir = 4
};

// square nav area, corners are managed by direction and height to show as square
class ENavArea
{
public:
	uint16_t index{};
	uint32_t flags{};
	uint8_t connectionCount{};
	uint16_t* connection{};
	float direction[ENavDir::NumDir]{};
	float dirHeight[ENavDir::NumDir]{};

	Vector GetClosestPosition(const Vector& origin);
	Vector GetFarestPosition(const Vector& origin);
	Vector GetCenter(void);
	Vector GetRandomPosition(void);
	Vector GetCornerPosition(const uint8_t corner);
	Vector AreaDirectionAsVector(const uint8_t corner);
	bool IsAreaOverlapping(const ENavArea area);
	bool IsPointOverlapping(const Vector& origin);

	void ExpandNavArea(const uint8_t radius = static_cast<uint8_t>(50));
};

class ENavMesh : public Singleton <ENavMesh>
{
	friend class Bot;
private:
	MiniArray <ENavArea> m_area{};
public:
	int16_t m_selectedNavIndex{};

	ENavMesh(void);
	~ENavMesh(void);

	void Initialize(void);
	void CreateBasic(void);
	void Analyze(void);
	void FinishAnalyze(void);

	bool IsConnected(const int start, const int goal);
	void SaveNav(void);
	bool LoadNav(void);

	Vector GetSplitOrigin(const Vector origin);

	void SelectNavArea(void);
	void UnselectNavArea(const bool msg = false);
	void DrawNavArea(void);

	ENavArea* CreateArea(const Vector origin, const bool expand = false);
	void DeleteArea(const ENavArea area);
	void DeleteAreaByIndex(const int index);
	void ConnectArea(const int start, const int end);
	void DisconnectArea(const int start, const int end);
	void MergeNavAreas(const uint16_t area1, const uint16_t area2);
	void OptimizeNavMesh(void);

	ENavArea GetNavArea(const uint16_t id);
	ENavArea* GetNavAreaP(const uint16_t id);
	int GetNearestNavAreaID(const Vector origin);
	Vector DirectionAsVector(const uint8_t corner, const float size);
	float DirectionAsFloat(const uint8_t corner);
	Vector GetAimingPosition(void);
};

#define g_navmesh ENavMesh::GetObjectPtr()