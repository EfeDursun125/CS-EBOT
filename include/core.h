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
// There are lots of code from:
//  PODBot-MM, by KWo.
//  EPODBot, by THE_STORM.
//  RACC, by Pierre-Marie Baty.
//  JKBotti, by Jussi Kivilinna.
//    and others...
//
// Huge thanks to them.
//
// $Id:$
//

#ifndef EBOT_INCLUDED
#define EBOT_INCLUDED

// reduce glibc version...
#ifndef _WIN32
#include "glibc.h"
#include <cstdint>
#endif

#include <stdio.h>
#include "clib.h"
#include "engine.h"

using namespace Math;

#include "platform.h"
#include "runtime.h"

enum class Process : int8_t
{
	Default = 0,
	Pause,
	DestroyBreakable,
	ThrowHE,
	ThrowFB,
	ThrowSM,
	Blind,
	UseButton,
	Override
};

// supported cs's
enum Game : int8_t
{
	CStrike = (1 << 1), // Counter-Strike 1.6 and Above
	CZero = (1 << 2), // Counter-Strike: Condition Zero
	Xash = (1 << 3), // Xash3D
};

// log levels
enum class Log : int8_t
{
	Default = 1, // default log message
	Warning = 2, // warning log message
	Error = 3, // error log message
	Fatal = 4, // fatal error log message
	Memory = 5 // memory error log message
};

// personalities defines
enum Personality : int8_t
{
	Normal = 0,
	Rusher,
	Careful
};

// counter-strike team id's
enum Team
{
	Terrorist = 0,
	Counter,
	Spectator,
	Unassinged,
	Count
};

// client flags
enum ClientFlags
{
	CFLAG_USED = (1 << 0),
	CFLAG_ALIVE = (1 << 1),
	CFLAG_OWNER = (1 << 2)
};

// counter-strike weapon id's
enum Weapon
{
	P228 = 1,
	Shield = 2,
	Scout = 3,
	HeGrenade = 4,
	Xm1014 = 5,
	C4 = 6,
	Mac10 = 7,
	Aug = 8,
	SmGrenade = 9,
	Elite = 10,
	FiveSeven = 11,
	Ump45 = 12,
	Sg550 = 13,
	Galil = 14,
	Famas = 15,
	Usp = 16,
	Glock18 = 17,
	Awp = 18,
	Mp5 = 19,
	M249 = 20,
	M3 = 21,
	M4A1 = 22,
	Tmp = 23,
	G3SG1 = 24,
	FbGrenade = 25,
	Deagle = 26,
	Sg552 = 27,
	Ak47 = 28,
	Knife = 29,
	P90 = 30,
	Kevlar = 31,
	KevlarHelmet = 32,
	Defuser = 33
};

// vgui menus (since latest steam updates is obsolete, but left for old cs)
enum GraphicalMenu
{
	GMENU_TEAM = 2, // menu select team
	GMENU_TERRORIST = 26, // terrorist select menu
	GMENU_COUNTER = 27  // ct select menu
};

// game start messages for counter-strike...
enum StartMsg
{
	CMENU_IDLE = 1,
	CMENU_TEAM = 2,
	CMENU_CLASS = 3,
};

// visibility flags
enum Visibility : int8_t
{
	None = (1 << 0),
	Head = (1 << 1),
	Body = (1 << 2),
	Other = (1 << 3)
};

// defines for waypoint flags field (32 bits are available)
enum WaypointFlag : uint32_t
{
	WAYPOINT_LIFT = (1 << 1), // wait for lift to be down before approaching this waypoint
	WAYPOINT_CROUCH = (1 << 2), // must crouch to reach this waypoint
	WAYPOINT_CROSSING = (1 << 3), // a target waypoint
	WAYPOINT_GOAL = (1 << 4), // mission goal point (bomb, hostage etc.)
	WAYPOINT_LADDER = (1 << 5), // waypoint is on ladder
	WAYPOINT_RESCUE = (1 << 6), // waypoint is a hostage rescue point
	WAYPOINT_CAMP = (1 << 7), // waypoint is a camping point
	WAYPOINT_DJUMP = (1 << 9), // bot help's another bot (requster) to get somewhere (using djump)
	WAYPOINT_ZMHMCAMP = (1 << 10), // bots will camp at this waypoint
	WAYPOINT_AVOID = (1 << 11), // bots will avoid these waypoints mostly
	WAYPOINT_USEBUTTON = (1 << 12), // bots will use button
	WAYPOINT_HMCAMPMESH = (1 << 13), // human camp mesh
	WAYPOINT_ZOMBIEONLY = (1 << 14), // only zombie bots can use this waypoint
	WAYPOINT_HUMANONLY = (1 << 15), // only humans bots can use this waypoint
	WAYPOINT_ZOMBIEPUSH = (1 << 16), // zombies never return back on this waypoint
	WAYPOINT_FALLRISK = (1 << 17), // bots never do strafing while on this waypoint
	WAYPOINT_SPECIFICGRAVITY = (1 << 18), // specific jump gravity check for bot
	WAYPOINT_ONLYONE = (1 << 19), // to avoid multiple bots stuck on same waypoint
	WAYPOINT_WAITUNTIL = (1 << 20), // inverse fall check
	WAYPOINT_HELICOPTER = (1 << 21), // helicopter for zombie escape maps
	WAYPOINT_FALLCHECK = (1 << 26), // bots will check ground
	WAYPOINT_JUMP = (1 << 27), // for jump points
	WAYPOINT_SNIPER = (1 << 28), // it's a specific sniper point
	WAYPOINT_TERRORIST = (1 << 29), // it's a specific terrorist point
	WAYPOINT_COUNTER = (1 << 30)  // it's a specific ct point
};

// defines for waypoint connection flags field (16 bits are available)
enum PathFlag
{
	PATHFLAG_JUMP = (1 << 0), // must jump for this connection
	PATHFLAG_DOUBLE = (1 << 1), // must use friend for this connection
};

// defines waypoint connection types
enum PathConnection
{
	PATHCON_OUTGOING = 0,
	PATHCON_INCOMING,
	PATHCON_BOTHWAYS,
	PATHCON_JUMPING,
	PATHCON_BOOSTING,
	PATHCON_VISIBLE
};

// collision states
enum CollisionState
{
	COSTATE_UNDECIDED,
	COSTATE_PROBING,
	COSTATE_NOMOVE,
	COSTATE_JUMP,
	COSTATE_DUCK,
	COSTATE_STRAFELEFT,
	COSTATE_STRAFERIGHT
};

// collision probes
enum CollisionProbe
{
	COPROBE_JUMP = (1 << 0), // probe jump when colliding
	COPROBE_DUCK = (1 << 1), // probe duck when colliding
	COPROBE_STRAFE = (1 << 2) // probe strafing when colliding
};

enum class LiftState : int8_t
{
	None = 0,
	LookingButtonOutside,
	WaitingFor,
	EnteringIn,
	WaitingForTeammates,
	LookingButtonInside,
	TravelingBy,
	Leaving
};

// bot known file
#define FH_WAYPOINT_NEW "EBOTWP!"
#define FV_WAYPOINT 127

#define Const_MaxPathIndex 8
#define Const_MaxWaypoints 8192
#define Const_NumWeapons 24
#define Const_MaxWeapons 32

// weapon masks
#define WeaponBits_Primary ((1 << Weapon::Xm1014) | (1 << Weapon::M3) | (1 << Weapon::Mac10) | (1 << Weapon::Ump45) | (1 << Weapon::Mp5) | (1 << Weapon::Tmp) | (1 << Weapon::P90) | (1 << Weapon::Aug) | (1 << Weapon::M4A1) | (1 << Weapon::Sg552) | (1 << Weapon::Ak47) | (1 << Weapon::Scout) | (1 << Weapon::Sg550) | (1 << Weapon::Awp) | (1 << Weapon::G3SG1) | (1 << Weapon::M249) | (1 << Weapon::Famas) | (1 << Weapon::Galil))
#define WeaponBits_Secondary ((1 << Weapon::P228) | (1 << Weapon::Elite) | (1 << Weapon::Usp) | (1 << Weapon::Glock18) | (1 << Weapon::Deagle) | (1 << Weapon::FiveSeven))

// this structure links waypoints returned from pathfinder
class PathManager
{
private:
	bool m_follow{false};
	int16_t m_cursor{0};
	int16_t m_length{0};
	CPtr<int16_t>m_path{nullptr};
public:
	explicit PathManager(void) = default;
public:
	inline int16_t& Next(void) { return Get(1); }
	inline int16_t& First(void) { return Get(0); }
	inline int16_t& Last(void) { return Get(Length() - 1); }
	inline int16_t& Get(const int16_t index) { return m_path[m_cursor + index]; }
	inline void Shift(void) { m_cursor++; }
	inline void Reverse(void)
	{
		int16_t start = m_cursor;
		int16_t end = m_length - 1;
		while (start < end)
		{
			cswap(m_path[start], m_path[end]);
			start++;
			end--;
		}
	}

	inline int16_t Length(void) const
	{
		if (m_cursor >= m_length)
			return 0;

		return m_length - m_cursor;
	}

	inline bool HasNext(void) const { return Length() > 1; }
	inline bool IsEmpty(void) const { return !Length(); }

	inline bool Add(const int16_t waypoint)
	{
		if (m_path.IsAllocated())
		{
			m_path[m_length] = waypoint;
			m_length++;
			return true;
		}

		return false;
	}

	inline void Set(const int16_t index, const int16_t waypoint)
	{
		if ((m_cursor + index) < m_length)
			m_path[m_cursor + index] = waypoint;
	}

	inline void Clear(void)
	{
		m_cursor = 0;
		m_length = 0;
		if (m_path.IsAllocated())
			m_path[0] = 0;
	}

	inline void Destroy(void)
	{
		m_cursor = 0;
		m_length = 0;
		m_path.Destroy();
	}

	inline bool Init(const int16_t length)
	{
		m_cursor = 0;
		m_length = 0;
		m_path.Reset(new(std::nothrow) int16_t[length]);
		if (m_path.IsAllocated())
			return true;

		return false;
	}

	inline bool CanFollowPath(void) { return m_follow && Length(); }
	inline void Start(void) { m_follow = true; }
	inline void Stop(void) { m_follow = false; }
};

// botname structure definition
struct NameItem
{
	char name[32];
	bool isUsed{false};
};

struct WeaponSelect
{
	int id; // the weapon id value
	char* weaponName; // name of the weapon when selecting it
	bool primaryFireHold; // hold down primary fire button to use?
};

// fire delay definiton
struct FireDelay
{
	int weaponIndex;
	int maxFireBullets;
	float minBurstPauseFactor;
	float primaryBaseDelay;
	float primaryMinDelay[6];
	float primaryMaxDelay[6];
	float secondaryBaseDelay;
	float secondaryMinDelay[5];
	float secondaryMaxDelay[5];
};

// struct for menus
struct MenuText
{
	int validSlots; // ored together bits for valid keys
	char* menuText; // ptr to actual string
};

// array of clients struct
struct Clients
{
	MenuText* menu{nullptr}; // pointer to opened bot menu
	edict_t* ent{nullptr}; // pointer to actual edict
	Vector origin{nullvec}; // position in the world
	int team{Team::Count}; // bot team
	int flags{0}; // client flags
	int index{-1}; // client index
	int16_t wp{-1}; // waypoint index
	bool ignore{false}; // should bots ignore this?
};

// bot creation tab
struct CreateItem
{
	int skill;
	int team;
	int member;
	int personality;
	char name[32];
};

// weapon properties structure
struct WeaponProperty
{
	char className[64];
	int ammo1; // ammo index for primary ammo
	int ammo1Max; // max primary ammo
	int slotID; // HUD slot (0 based)
	int position; // slot position
	int id; // weapon ID
	int flags{0}; // flags
};

// general waypoint header information structure
struct WaypointHeader
{
	char header[8];
	int32_t fileVersion;
	int32_t pointNumber;
	char mapName[32];
	char author[32];
};

// define general waypoint structure
struct Path
{
	Vector origin;
	uint32_t flags;
	uint8_t radius;
	uint8_t mesh;
	int16_t index[Const_MaxPathIndex];
	uint16_t connectionFlags[Const_MaxPathIndex];
	float gravity;
};

// main bot class
class Bot
{
	friend class BotControl;
public:
	float m_moveSpeed{0.0f}; // current speed forward/backward
	float m_strafeSpeed{0.0f}; // current speed sideways
	float m_tempstrafeSpeed{0.0f}; // temp speed sideways

	edict_t* m_myself{nullptr};
	edict_t* m_avoid{nullptr};
	uint16_t m_buttons{0};
	uint16_t m_oldButtons{0}; // buttons from previous frame
	uint8_t m_impulse{0}; // bot's impulse command

	int m_tryOpenDoor{0}; // attempt's to open the door
	LiftState m_liftState{}; // state of lift handling
	float m_liftUsageTime{0.0f}; // time to use lift
	edict_t* m_liftEntity{nullptr}; // pointer to lift entity
	Vector m_liftTravelPos{nullvec}; // lift travel position
	float m_timeDoorOpen{0.0f}; // time to next door open check

	Vector m_breakableOrigin{nullvec}; // origin of the breakable
	edict_t* m_breakableEntity{nullptr}; // pointer to breakable entity
	edict_t* m_ignoreEntity{nullptr}; // pointer to entity to ignore
	edict_t* m_buttonEntity{nullptr}; // pointer to button entity

	int16_t m_currentGoalIndex{-1}; // current goal index for default modes
	int16_t m_currentWaypointIndex{-1}; // current waypoint index
	int16_t m_prevWptIndex[4]{-1}; // previous waypoint indices from waypoint find
	int16_t m_knownWaypointIndex[6]{-1}; // checks if bot already aimed at this waypoint

	uint16_t m_prevTravelFlags{0}; // prev of it
	uint16_t m_currentTravelFlags{0}; // connection flags like jumping

	Vector m_throw{nullvec}; // origin of waypoint to throw grenades
	Vector m_lookAt{nullvec}; // vector bot should look at
	Vector m_lookVelocity{nullvec}; // interpolate velocity
	bool m_updateLooking{false}; // should bot update where to look?

	bool m_isStuck{false}; // is bot stuck?
	float m_prevTime{0.0f}; // time previously checked movement speed
	float m_prevSpeed{0.0f}; // speed some frames before
	Vector m_prevOrigin{nullvec}; // origin some frames before
	float m_prevVelocity{0.0f}; // velocity some frames before
	float m_stuckTime{0.0f}; // how long bot has been stuck?

	float m_damageTime{0.0f}; // tweak for zombie bots
	float m_movedDistance{0.0f}; // moved distance
	float m_collideTime{0.0f}; // time last collision
	float m_firstCollideTime{0.0f}; // time of first collision
	float m_probeTime{0.0f}; // time of probing different moves
	float m_lastCollTime{0.0f}; // time until next collision check
	int m_collisionProbeBits{0}; // bits of possible collision moves
	int m_collideMoves[3]{0}; // sorted array of movements
	int m_collStateIndex{0}; // index into collide moves
	CollisionState m_collisionState{CollisionState::COSTATE_UNDECIDED}; // collision state

	uint8_t m_msecVal{0}; // stored msec val
	float m_msecInterval{0.0f}; // used for leon hartwig's method for msec calculation
	float m_frameInterval{0.0f}; // bot's frame interval
	float m_aimInterval{0.0f}; // bot's aim interval
	float m_pathInterval{0.0f}; // bot's path interval
	float m_zoomCheckTime{0.0f}; // time to check zoom again

	float m_duckTime{0.0f}; // time to duck
	float m_jumpTime{0.0f}; // time last jump happened
	float m_buttonPushTime{0.0f}; // time to push the button

	Vector m_moveAngles{nullvec}; // bot move angles
	float m_lookYawVel{0.0f}; // look yaw velocity
	float m_lookPitchVel{0.0f}; // look pitch velocity

	int16_t m_zhCampPointIndex{-1}; // zombie stuff index
	int16_t m_myMeshWaypoint{-1}; // human mesh stuff index

	void DebugModeMsg(void);
	void MoveAction(void);

	bool CanJumpUp(const Vector& normal);
	bool CanDuckUnder(const Vector& normal);
	bool CantMoveForward(const Vector& normal);

	bool CheckWallOnForward(void);
	bool CheckWallOnBehind(void);
	bool CheckWallOnLeft(void);
	bool CheckWallOnRight(void);

	bool UpdateLiftHandling(void);
	bool UpdateLiftStates(void);

	bool IsInViewCone(const Vector& origin);
	bool CheckVisibility(edict_t* targetEntity);
	bool CheckGrenadeThrow(edict_t* targetEntity);

	edict_t* FindNearestButton(const char* className);
	edict_t* FindButton(void);
	int16_t FindGoalZombie(void);
	int16_t FindGoalHuman(void);

	float InFieldOfView(const Vector& dest);
	bool IsWaypointOccupied(const int16_t index);
	bool IsEnemyHidden(edict_t* entity);
	bool IsEnemyInvincible(edict_t* entity);

	inline bool IsOnLadder(void) { return pev->movetype == MOVETYPE_FLY; }
	inline bool IsOnFloor(void) { return !!(pev->flags & (FL_ONGROUND | FL_PARTIALGROUND)); }
	inline bool IsInWater(void) { return pev->waterlevel > 2; }

	void SetStrafeSpeed(const Vector& moveDir, const float strafeSpeed);
	void SetStrafeSpeedNoCost(const Vector& moveDir, const float strafeSpeed);
	void StartGame(void);

	void ChangeWptIndex(const int16_t waypointIndex);
	bool IsDeadlyDrop(const Vector& targetOriginPos);
	void SetWaypointOrigin(void);

	Vector CheckToss(const Vector& start, const Vector& end);
	Vector CheckThrow(const Vector& start, const Vector& end);
	Vector GetEnemyPosition(void);
	float GetZOffset(const float distance);

	int GetNearbyFriendsNearPosition(const Vector& origin, const float radius);
	int GetNearbyEnemiesNearPosition(const Vector& origin, const float radius);

	int CheckGrenades(void);
	bool IsWeaponBadInDistance(const int weaponIndex, const float distance);
	void KnifeAttack(void);
	void FireWeapon(const float distance);

	void SelectBestWeapon(void);
	void SelectKnife(void);
	void SelectWeaponByName(const char* name);
	int GetHighestWeapon(void);

	void FindPath(int16_t& srcIndex, int16_t& destIndex);
	void FindShortestPath(int16_t& srcIndex, int16_t& destIndex);
	void FindEscapePath(int16_t& srcIndex, const Vector& dangerOrigin);
	void CalculatePing(void);
public:
	entvars_t* pev{nullptr}; // pev

	float m_updateTimer{0.0f}; // used to update bots

	uint16_t m_numSpawns{0}; // used for path randomizing
	int m_wantedTeam{0}; // player team bot wants select
	int m_wantedClass{0}; // player model bot wants to select
	int m_difficulty{0}; // bot difficulty
	int m_basePingLevel{0}; // base ping level for randomizing

	int m_skill{0}; // bots play skill
	int m_personality{0}; // bot's personality

	int m_startAction{0}; // team/class selection state
	int m_retryJoin{0}; // how many times bot must retry
	int m_team{0}; // bot's team
	int m_index{0}; // bot's index
	bool m_isAlive{false}; // has the player been killed or has he just respawned
	bool m_notStarted{false}; // team/class not chosen yet
	bool m_isZombieBot{false}; // checks bot if zombie
	bool m_jumpReady{false}; // get ready for jump at next frame
	bool m_waitForLanding{false}; // wait until land somewhere

	Vector m_destOrigin{nullvec}; // origin of move destination
	Vector m_waypointOrigin{nullvec}; // post waypoint origin, m_waypoint.origin returns center

	int m_pingOffset[2]{0}; // offset for faking pings
	int m_ping[3]{0}; // bots pings in scoreboard
	bool m_isEnemyReachable{false}; // direct line to enemy

	float m_seeEnemyTime{0.0f}; // time bot sees enemy
	float m_randomAttackTimer{0.0f}; // a timer for make bots random attack with knife like humans
	float m_slowThinkTimer{0.0f}; // slow think timer

	bool m_isSlowThink{false}; // bool for check is slow think? (every second)
	float m_firePause{0.0f}; // time to pause firing

	int m_currentWeapon{0}; // one current weapon for each bot
	int m_ammoInClip[Const_MaxWeapons + 1]{0}; // ammo in clip for each weapons
	int m_ammo[Const_MaxWeapons + 1]{0}; // total ammo amounts

	Path m_waypoint{}; // current waypoint
	float m_waypointTime{0.0f}; // avg reach time
	PathManager m_navNode{}; // pointer to current node from path

	bool m_checkFall{false}; // check bot fall
	Vector m_checkFallPoint[2]{nullvec}; // check fall point

	// NEW VARS
	Process m_currentProcess{Process::Default};
	Process m_rememberedProcess{Process::Default};
	float m_currentProcessTime{0.0f};
	float m_rememberedProcessTime{0.0f};

	bool m_hasEnemiesNear{false};
	bool m_hasFriendsNear{false};
	bool m_hasEntitiesNear{false};
	int8_t m_numEnemiesLeft{0};
	int8_t m_numFriendsLeft{0};
	int8_t m_numEntitiesLeft{0};
	float m_enemyDistance{0.0f};
	float m_friendDistance{0.0f};
	float m_entityDistance{0.0f};
	float m_enemySeeTime{0.0f};
	float m_friendSeeTime{0.0f};
	float m_entitySeeTime{0.0f};
	edict_t* m_nearestEnemy{nullptr};
	edict_t* m_nearestFriend{nullptr};
	edict_t* m_nearestEntity{nullptr};
	Vector m_enemyOrigin{nullvec};
	Vector m_friendOrigin{nullvec};
	Vector m_entityOrigin{nullvec};
	int8_t m_numFriendsNear{0};

	int8_t m_senseChance{50};

	float m_searchTime{0.0f};
	float m_pauseTime{0.0f};

	float m_frameDelay{0.0f};
	float m_baseUpdate{0.0f};
	float m_rpm{0.0f};

	int m_overrideID{0};
	bool m_overrideDefaultLookAI{true};
	bool m_overrideDefaultChecks{true};

	Bot(edict_t* bot, const int skill, const int personality, const int team, const int member);
	~Bot(void);

	// NEW AI
	void BaseUpdate(void);
	void UpdateLooking(void);
	void UpdateProcess(void);
	void CheckSlowThink(void);

	Process GetCurrentState(void);
	float GetCurrentStateTime(void);

	bool SetProcess(const Process& process, const char* debugNote = "clear", const bool rememberProcess = false, const float time = 9999999.0f);
	void StartProcess(const Process& process);
	void EndProcess(const Process& process);
	void FinishCurrentProcess(const char* debugNote = "finished by the system");
	bool IsReadyForTheProcess(const Process& process);
	char* GetProcessName(const Process& process);

	// FUNCTIONS
	void LookAtAround(void);
	void MoveTo(const Vector& targetPosition, const bool checkStuck = true);
	void MoveOut(const Vector& targetPosition, const bool checkStuck = true);
	void FollowPath(void);
	void FindFriendsAndEnemiens(void);
	void FindEnemyEntities(void);

	bool IsEnemyViewable(edict_t* player);
	bool CheckWaypoint(void);

	void CheckStuck(const Vector& directionNormal, const float finterval);
	void ResetStuck(void);
	void ResetCollideState(void);
	void IgnoreCollisionShortly(void);
	void DoWaypointNav(void);

	int16_t FindWaypoint(void);

	bool IsAttacking(const edict_t* player);
	bool CheckReachable(void);
	bool IsEnemyReachableToPosition(const Vector& origin);
	bool IsFriendReachableToPosition(const Vector& origin);
	bool CanIReachToPosition(const Vector& origin);

	// SSM
	void DefaultStart(void);
	void PauseStart(void);
	void DestroyBreakableStart(void);
	void ThrowHEStart(void);
	void ThrowFBStart(void);
	void ThrowSMStart(void);
	void BlindStart(void);
	void UseButtonStart(void);

	void DefaultUpdate(void);
	void PauseUpdate(void);
	void DestroyBreakableUpdate(void);
	void ThrowHEUpdate(void);
	void ThrowFBUpdate(void);
	void ThrowSMUpdate(void);
	void BlindUpdate(void);
	void UseButtonUpdate(void);
	void OverrideUpdate(void);

	void DefaultEnd(void);
	void PauseEnd(void);
	void DestroyBreakableEnd(void);
	void ThrowHEEnd(void);
	void ThrowFBEnd(void);
	void ThrowSMEnd(void);
	void BlindEnd(void);
	void UseButtonEnd(void);
	void OverrideEnd(void);

	bool DefaultReq(void);
	bool PauseReq(void);
	bool DestroyBreakableReq(void);
	bool ThrowHEReq(void);
	bool ThrowFBReq(void);
	bool ThrowSMReq(void);
	bool BlindReq(void);
	bool UseButtonReq(void);

	inline int GetAmmoInClip(void) { return m_ammoInClip[m_currentWeapon]; }
	inline edict_t* GetEntity(void) { return ENT(pev); }
	inline EOFFSET GetOffset(void) { return OFFSET(pev); };
	inline int GetIndex(void) { return ENTINDEX(GetEntity()); };

	inline Vector Center(void) { return (pev->absmax + pev->absmin) * 0.5f; };
	inline Vector EyePosition(void) { return pev->origin + pev->view_ofs; };

	void LookAt(const Vector& origin, const Vector& velocity = nullvec);
	void NewRound(void);

	void CheckTouchEntity(edict_t* entity);
	void TakeBlinded(const Vector& fade, const int alpha);

	void Kill(void);
	void Kick(void);

	bool UsesRifle(void);
	bool UsesPistol(void);
	bool UsesShotgun(void);
	bool UsesSniper(void);
	bool UsesSubmachineGun(void);
	bool UsesZoomableRifle(void);
	bool UsesBadPrimary(void);
	bool IsSniper(void);
};

// manager class
class BotControl : public Singleton <BotControl>
{
private:
	CArray<CreateItem>m_creationTab{}; // bot creation tab
	float m_maintainTime{0.0f}; // time to maintain bot creation quota
protected:
	int CreateBot(char name[32], int skill, int personality, const int team, const int member);
public:
	Bot* m_bots[32]{nullptr}; // all available bots
	CArray<char*>m_avatars{}; // storing the steam ids

	BotControl(void);
	~BotControl(void);

	int GetIndex(edict_t* ent);
	int GetIndex(const int index);
	Bot* GetBot(const int index);
	Bot* GetBot(edict_t* ent);

	int GetBotsNum(void);
	int GetHumansNum(void);

	void Think(void);
	void Free(void);
	void Free(const int index);

	void AddRandom(void) { AddBot("", -1, -1, -1, -1); }
	void AddBot(char name[32], const int skill, const int personality, const int team, const int member);
	void FillServer(int selection, const int personality = Personality::Normal, const int skill = -1, const int numToAdd = -1);

	void RemoveAll(void);
	void RemoveRandom(void);
	void RemoveFromTeam(const Team team, const bool removeAll = false);
	void RemoveMenu(edict_t* ent, const int selection);
	void KillAll(const int team = Team::Count);
	void MaintainBotQuota(void);
	void InitQuota(void);

	void ListBots(void);

	int AddBotAPI(char name[32], const int skill, const int team);
	static void CallGameEntity(entvars_t* vars);
};

// netmessage handler class
class NetworkMsg : public Singleton <NetworkMsg>
{
private:
	Bot* m_bot{nullptr};
	int8_t m_state{0};
	int m_message{0};
	int m_registerdMessages[NETMSG_NUM]{0};
public:
	NetworkMsg(void);
	~NetworkMsg(void) { m_bot = nullptr; };

	void Execute(void* p);
	void Reset(void) { m_message = NETMSG_UNDEFINED; m_state = 0; m_bot = nullptr; };
	void HandleMessageIfRequired(const int messageType, const int requiredType);

	void SetMessage(const int message) { m_message = message; }
	void SetBot(Bot* bot) { m_bot = bot; }

	int GetId(const int messageType) { return m_registerdMessages[messageType]; }
	void SetId(const int messageType, const int messsageIdentifier) { m_registerdMessages[messageType] = messsageIdentifier; }
};

#define MAX_WAYPOINT_BUCKET_SIZE static_cast<int>(Const_MaxWaypoints * 0.65)
#define MAX_WAYPOINT_BUCKET_MAX Const_MaxWaypoints * 8 / MAX_WAYPOINT_BUCKET_SIZE + 1

// waypoint operation class
class Waypoint : public Singleton <Waypoint>
{
	friend class Bot;
private:
	bool m_isOnLadder{false};
	bool m_endJumpPoint{false};
	bool m_learnJumpWaypoint{false};
	float m_timeJumpStarted{0.0f};

	bool m_ladderPoint{false};
	bool m_fallPoint{false};
	int16_t m_lastFallWaypoint{0};

	Vector m_learnVelocity{nullvec};
	Vector m_learnPosition{nullvec};
	Vector m_fallPosition{nullvec};

	int16_t m_cacheWaypointIndex{-1};
	int16_t m_lastJumpWaypoint{-1};
	Vector m_lastWaypoint{nullvec};

	int16_t m_lastDeclineWaypoint{-1};

	float m_pathDisplayTime{0.0f};
	float m_arrowDisplayTime{0.0f};
	int16_t m_findWPIndex{-1};
	int16_t m_facingAtIndex{-1};
	char m_infoBuffer[256]{"\0"};

	CArray<int16_t>m_terrorPoints{};
	CArray<int16_t>m_zmHmPoints{};
	CArray<int16_t>m_hmMeshPoints{};
	CArray<int16_t>m_buckets[MAX_WAYPOINT_BUCKET_MAX][MAX_WAYPOINT_BUCKET_MAX][MAX_WAYPOINT_BUCKET_MAX];
public:
	struct Bucket { int16_t x, y, z; };
	CPtr<float>m_waypointDisplayTime{nullptr};
	CPtr<int16_t>m_distMatrix{nullptr};
	CArray<Path>m_paths{};

	Waypoint(void);
	~Waypoint(void);

	void Initialize(void);
	void Analyze(void);
	void AnalyzeDeleteUselessWaypoints(void);
	void InitTypes(void);
	void AddZMCamps(void);
	void AddPath(const int16_t addIndex, const int16_t pathIndex, const int type = 0);

	int16_t GetFacingIndex(void);
	int16_t FindFarest(const Vector& origin, float maxDistance = 99999.0f);
	int16_t FindNearest(const Vector& origin, float minDistance = 99999.0f);
	int16_t FindNearestSlow(const Vector& origin, float minDistance = 99999.0f);
	int16_t FindNearestToEnt(const Vector& origin, float minDistance, edict_t* entity);
	int16_t FindNearestToEntSlow(const Vector& origin, float minDistance, edict_t* entity);
	int16_t FindNearestAnalyzer(const Vector& origin, float minDistance = 99999.0f, const float range = 99999.0f);
	void FindInRadius(const Vector& origin, float radius, int16_t* holdTab, int16_t* count);
	void FindInRadius(CArray<int16_t>&queueID, const float& radius, const Vector& origin);

	void Add(const int flags, const Vector& waypointOrigin = nullvec, const float analyzeRange = 128.0f);
	void Delete(void);
	void DeleteByIndex(int16_t index);
	void ToggleFlags(int toggleFlag);
	void SetRadius(const int radius);
	bool IsConnected(const int16_t pointA, const int16_t pointB);
	bool IsConnected(const int num);
	void CreateWaypointPath(const PathConnection dir);
	void DeletePath(void);
	void DeletePathByIndex(int16_t nodeFrom, int16_t nodeTo);
	void CacheWaypoint(void);

	void DeleteFlags(void);
	void TeleportWaypoint(void);

	void CalculateWayzone(const int16_t index);
	Vector GetBottomOrigin(const Path* waypoint);

	void Sort(const int16_t self, int16_t index[], const int16_t size = static_cast<int16_t>(Const_MaxPathIndex));
	bool Download(void);
	bool Load(void);
	void Save(void);

	void InitPathMatrix(void);
	void SavePathMatrix(void);
	bool LoadPathMatrix(void);

	bool Reachable(edict_t* entity, const int16_t index);
	bool IsNodeReachable(Vector src, Vector dest);
	bool IsNodeReachableAnalyze(const Vector& src, const Vector& destination, const float range, const bool hull = true);
	bool MustJump(const Vector src, const Vector destination);
	void Think(void);
	void ShowWaypointMsg(void);
	bool NodesValid(void);
	void CreateBasic(void);

	void DestroyBuckets(void);
	void AddToBucket(const Vector pos, const int16_t index);
	void EraseFromBucket(const Vector pos, const int16_t index);
	Bucket LocateBucket(const Vector pos);
	CArray<int16_t>&GetWaypointsInBucket(const Vector &pos);

	Path* GetPath(const int16_t id);
	char* GetWaypointInfo(const int16_t id);
	char* GetInfo(void) { return m_infoBuffer; }

	void SetFindIndex(const int16_t index);
	void SetLearnJumpWaypoint(const int mod = -1);

	const char* CheckSubfolderFile(void);
};

#define g_netMsg NetworkMsg::GetObjectPtr()
#define g_botManager BotControl::GetObjectPtr()
#define g_waypoint Waypoint::GetObjectPtr()

// prototypes of bot functions...
extern int GetTeam(edict_t* ent);
extern bool IsBreakable(edict_t* ent);
extern bool IsZombieEntity(edict_t* ent);
extern float GetShootingConeDeviation(edict_t* ent, const Vector& position);

extern bool IsLinux(void);
extern bool TryFileOpen(char* fileName);
extern bool IsWalkableLineClear(const Vector& from, const Vector& to);
extern bool IsWalkableHullClear(const Vector& from, const Vector& to);
extern bool IsDedicatedServer(void);
extern bool IsVisible(const Vector& origin, edict_t* ent);
extern bool IsAlive(const edict_t* ent);
extern bool IsInViewCone(const Vector& origin, edict_t* ent);
extern bool IsValidBot(edict_t* ent);
extern bool IsValidBot(const int index);
extern bool IsValidPlayer(edict_t* ent);
extern bool OpenConfig(const char* fileName, const char* errorIfNotExists, File* outFile);
extern bool FindNearestPlayer(void** holder, edict_t* to, const float searchDistance = 4096.0f, const bool sameTeam = false, const bool needBot = false, const bool needAlive = false, const bool needDrawn = false);

extern char* GetEntityName(edict_t* entity);
extern char* GetMapName(void);
extern char* GetWaypointDir(void);
extern char* GetModName(void);
extern char* GetField(const char* string, const int fieldId, const bool endLine = false);
extern void CreatePath(char* path);

extern Vector GetWalkablePosition(const Vector& origin, edict_t* ent = nullptr, const bool returnNullVec = false, const float height = 1000.0f);
extern Vector GetEntityOrigin(edict_t* ent);
extern Vector GetBoxOrigin(edict_t* ent);

extern Vector GetTopOrigin(edict_t* ent);
extern Vector GetBottomOrigin(edict_t* ent);
extern Vector GetPlayerHeadOrigin(edict_t* ent);

extern void FreeLibraryMemory(void);
extern void RoundInit(void);
extern void FakeClientCommand(edict_t* fakeClient, const char* format, ...);
extern void CreateWaypointPath(char* path);
extern void ServerCommand(const char* format, ...);
extern void RegisterCommand(const char* command, void funcPtr(void));
extern void DetectCSVersion(void);
extern void PlaySound(edict_t* ent, const char* soundName);
extern void ServerPrint(const char* format, ...);
extern void ChatPrint(const char* format, ...);
extern void ServerPrintNoTag(const char* format, ...);
extern void CenterPrint(const char* format, ...);
extern void ClientPrint(edict_t* ent, int dest, const char* format, ...);
extern void HudMessage(edict_t* ent, bool toCenter, const Color& rgb, char* format, ...);

extern void AddLogEntry(const Log logLevel, const char* format, ...);
extern void MOD_AddLogEntry(const int mode, char* format);

extern void DisplayMenuToClient(edict_t* ent, MenuText* menu);

extern void TraceLine(const Vector& start, const Vector& end, const int& ignoreFlags, edict_t* ignoreEntity, TraceResult* ptr);
extern void TraceHull(const Vector& start, const Vector& end, const int& ignoreFlags, const int& hullNumber, edict_t* ignoreEntity, TraceResult* ptr);

inline bool IsNullString(const char* input)
{
	if (!input)
		return true;
	
	if (!*input)
		return true;

	return *input == '\0';
}

// very global convars
extern ConVar ebot_gamemod;

#include "globals.h"
#include "resource.h"
#include "compress.h"

#endif // EBOT_INCLUDED
