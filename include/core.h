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
#include <glibc.h>
#include <cstdint>
#endif

#include <stdio.h>
#include <clib.h>
#include <engine.h>

using namespace Math;

#include <platform.h>
#include <runtime.h>

#define checkEntityNum 20
#define checkEnemyNum 128

enum class Process : int8_t
{
	Default = 0,
	Attack,
	Hide,
	Camp,
	Escape,
	Plant,
	Defuse,
	Pause,
	DestroyBreakable,
	Pickup,
	ThrowHE,
	ThrowFB,
	ThrowSM,
	Blind,
	Jump
};

// supported cs's
enum Game : int8_t
{
	CStrike = (1 << 1), // Counter-Strike 1.6 and Above
	CZero = (1 << 2), // Counter-Strike: Condition Zero
	Xash = (1 << 3), // Xash3D
	HalfLife = (1 << 4), // Half-Life
	DMC = (1 << 5) // Deathmatch Classic
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

// chat types id's
enum ChatType : int8_t
{
	CHAT_KILL = 0, // id to kill chat array
	CHAT_DEAD, // id to dead chat array
	CHAT_PLANTBOMB, // id to bomb chat array
	CHAT_TEAMATTACK, // id to team-attack chat array
	CHAT_TEAMKILL, // id to team-kill chat array
	CHAT_HELLO, // id to welcome chat array
	CHAT_NOKW, // id to no keyword chat array
	CHAT_NUM // number for array
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

// radio messages
enum Radio
{
	Nothin = 0,
	CoverMe = 1,
	YouTakePoint = 2,
	HoldPosition = 3,
	RegroupTeam = 4,
	FollowMe = 5,
	TakingFire = 6,
	GoGoGo = 11,
	Fallback = 12,
	StickTogether = 13,
	GetInPosition = 14,
	StormTheFront = 15,
	ReportTeam = 16,
	Affirmative = 21,
	EnemySpotted = 22,
	NeedBackup = 23,
	SectorClear = 24,
	InPosition = 25,
	ReportingIn = 26,
	ShesGonnaBlow = 27,
	Negative = 28,
	EnemyDown = 29
};

enum class ChatterMessage
{
	Nothing = 0,
	Yes,
	No,
	Happy,
	SeeksEnemy,
	EnemyDown,
	CoverMe,
	Clear,
	Camp,
	ReportingIn,
	ReportTeam,
	FallBack
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

// half-life weapon id's
enum WeaponHL
{
	Crowbar = 1,
	Glock = 2,
	Python = 3,
	Mp5_HL = 4,
	ChainGun = 5,
	Crossbow = 6,
	Shotgun = 7,
	Rpg = 8,
	Gauss = 9,
	Egon = 10,
	HornetGun = 11,
	HandGrenade = 12,
	TripMine = 13,
	Satchel = 14,
	Snark = 15
};

// defines for pickup items
enum class PickupType
{
	None = 0,
	Weapon,
	DroppedC4,
	PlantedC4,
	Hostage,
	Button,
	Shield,
	DefuseKit,
	GetEntity
};

// reload state
enum ReloadState : int8_t
{
	Nothing = 0, // no reload state currrently
	Primary = 1, // primary weapon reload state
	Secondary = 2  // secondary weapon reload state
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
	CMENU_BUY = 100,
	CMENU_RADIO = 200,
	CMENU_SAY = 10000,
	CMENU_TEAMSAY = 10001
};

// famas/glock burst mode status + m4a1/usp silencer
enum class BurstMode : int8_t
{
	Enabled = 1,
	Disabled = 2
};

// visibility flags
enum Visibility : int8_t
{
	None = (1 << 0),
	Head = (1 << 1),
	Body = (1 << 2),
	Other = (1 << 3)
};

// defines map type
enum MapType
{
	MAP_MIN = 0,
	MAP_AS = 1,
	MAP_CS = 2,
	MAP_DE = 3,
	MAP_ES = 4,
	MAP_KA = 5,
	MAP_FY = 6,
	MAP_AWP = 7,
	MAP_HE = 8,
	MAP_ZE = 9,
	MAP_MAX = 10
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

// E-BOT Supported Game Modes
enum class GameMode : int8_t
{
	Deathmatch = 1,
	ZombiePlague = 2,
	NoTeam = 3, // Monster Mod
	ZombieHell = 4,
	TeamDeathmatch = 5,
	Original = 0
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

#define Const_MaxHostages 8
#define Const_MaxPathIndex 8
#define Const_MaxWaypoints 8192
#define Const_MaxWeapons 32
#define Const_NumWeapons 26
#define Const_NumWeaponsHL 15
#define Const_MaxNavAreas 65535

// weapon masks
#define WeaponBits_Primary ((1 << Weapon::Xm1014) | (1 << Weapon::M3) | (1 << Weapon::Mac10) | (1 << Weapon::Ump45) | (1 << Weapon::Mp5) | (1 << Weapon::Tmp) | (1 << Weapon::P90) | (1 << Weapon::Aug) | (1 << Weapon::M4A1) | (1 << Weapon::Sg552) | (1 << Weapon::Ak47) | (1 << Weapon::Scout) | (1 << Weapon::Sg550) | (1 << Weapon::Awp) | (1 << Weapon::G3SG1) | (1 << Weapon::M249) | (1 << Weapon::Famas) | (1 << Weapon::Galil))
#define WeaponBits_Secondary ((1 << Weapon::P228) | (1 << Weapon::Elite) | (1 << Weapon::Usp) | (1 << Weapon::Glock18) | (1 << Weapon::Deagle) | (1 << Weapon::FiveSeven))
#define WeaponBits_SecondaryNODEFAULT ((1 << Weapon::P228) | (1 << Weapon::Elite) | (1 << Weapon::Deagle) | (1 << Weapon::FiveSeven))

// this structure links waypoints returned from pathfinder
class PathNode
{
private:
	int16_t m_cursor{};
	int16_t m_length{};
	int16_t m_capacity{};
	int16_t* m_path{};
public:
	explicit PathNode(void) = default;
	~PathNode(void)
	{
		safedel(m_path);
		m_capacity = 0;
		m_length = 0;
		m_capacity = 0;
	}
public:
	inline int16_t& Next(void)
	{
		return Get(1);
	}

	inline int16_t& First(void)
	{
		return Get(0);
	}

	inline int16_t& Last(void)
	{
		return Get(Length() - 1);
	}

	inline int16_t& Get(const int16_t index)
	{
		return m_path[m_cursor + index];
	}

	inline void Shift(void)
	{
		m_cursor++;
	}

	inline void Reverse(void)
	{
		int16_t i;
		const int16_t half = m_length / 2;
		const int16_t max = m_length - 1;
		for (i = 0; i < half; i++)
			cswap(m_path[i], m_path[max - i]);
	}

	inline int16_t Length(void) const
	{
		if (m_cursor >= m_length)
			return 0;

		return m_length - m_cursor;
	}

	inline bool HasNext(void) const
	{
		return Length() > m_cursor;
	}

	inline bool IsEmpty(void) const
	{
		return !Length();
	}

	inline void Add(const int waypoint)
	{
		if (waypoint < 0)
			return;

		if (m_length >= m_capacity)
		{
			m_capacity = m_length * 2;
			safereloc(m_path, m_length, m_capacity);
		}

		m_path[m_length] = static_cast<int16_t>(waypoint);
		m_length++;
	}

	inline void Clear(void)
	{
		m_cursor = 0;
		m_length = 0;
		m_path[0] = 0;
	}

	inline void Init(const int16_t length)
	{
		safeloc(m_path, length);
		m_capacity = length;
	}
};

// links keywords and replies together
struct KwChat
{
	Array <String> keywords{};
	Array <String> replies{};
	Array <String> usedReplies{};
};

// botname structure definition
struct NameItem
{
	String name{};
	bool isUsed{};
};

struct WeaponSelect
{
	int id; // the weapon id value
	char* weaponName; // name of the weapon when selecting it
	char* modelName; // model name to separate cs weapons
	int price; // price when buying
	int minPrimaryAmmo; // minimum primary ammo
	int buyGroup; // group in buy menu (standard map)
	int buySelect; // select item in buy menu (standard map)
	int newBuySelectT; // for counter-strike v1.6
	int newBuySelectCT; // for counter-strike v1.6
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
	MenuText* menu{}; // pointer to opened bot menu
	edict_t* ent{}; // pointer to actual edict
	Vector origin{}; // position in the world
	int team{}; // bot team
	int flags{}; // client flags
	int index{}; // client index
};

// bot creation tab
struct CreateItem
{
	int skill{};
	int team{};
	int member{};
	int personality{};
	String name{};
};

// weapon properties structure
struct WeaponProperty
{
	char className[64]{};
	int ammo1{}; // ammo index for primary ammo
	int ammo1Max{}; // max primary ammo
	int slotID{}; // HUD slot (0 based)
	int position{}; // slot position
	int id{}; // weapon ID
	int flags{}; // flags???
};

// define chatting collection structure
struct SayText
{
	int chatProbability{};
	float chatDelay{};
	float timeNextChat{};
	int entityIndex{};
	char sayText[512]{};
	Array <String> lastUsedSentences{};
};

// general waypoint header information structure
struct WaypointHeader
{
	char header[8]{};
	int32_t fileVersion{};
	int32_t pointNumber{};
	char mapName[32]{};
	char author[32]{};
};

// define general waypoint structure
struct Path
{
	Vector origin{};
	uint32_t flags{};
	uint8_t radius{};
	uint8_t mesh{};
	int16_t index[Const_MaxPathIndex]{};
	uint16_t connectionFlags[Const_MaxPathIndex]{};
	float gravity{};
};

// main bot class
class Bot
{
	friend class BotControl;
public:
	float m_moveSpeed{}; // current speed forward/backward
	float m_strafeSpeed{}; // current speed sideways
	float m_tempstrafeSpeed{}; // temp speed sideways

	bool m_isLeader{}; // bot is leader of his team

	int m_messageQueue[33]{}; // stack for messages
	char m_tempStrings[512]{}; // space for strings (say text...)
	int m_tryOpenDoor{}; // attempt's to open the door
	int m_radioSelect{}; // radio entry
	float m_chatterTimer{}; // chatter timer
	ChatterMessage m_lastChatterMessage{}; // for block looping same line...

	LiftState m_liftState{}; // state of lift handling
	float m_liftUsageTime{}; // time to use lift
	edict_t* m_liftEntity{}; // pointer to lift entity
	Vector m_liftTravelPos{}; // lift travel position

	float m_itemCheckTime{}; // time next search for items needs to be done
	PickupType m_pickupType{}; // type of entity which needs to be used/picked up
	Vector m_breakable{}; // origin of breakable
	Vector m_avgDeathOrigin{}; // average death origin

	edict_t* m_pickupItem{}; // pointer to entity of item to use/pickup
	edict_t* m_itemIgnore{}; // pointer to entity to ignore for pickup
	edict_t* m_breakableEntity{}; // pointer to breakable entity

	float m_timeDoorOpen{}; // time to next door open check
	float m_lastChatTime{}; // time bot last chatted

	int m_currentGoalIndex{}; // current goal index for default modes
	int m_currentWaypointIndex{}; // current waypoint index
	int m_prevWptIndex[3]{}; // previous waypoint indices from waypoint find
	int m_loosedBombWptIndex{}; // nearest to loosed bomb waypoint
	int m_knownWaypointIndex[5]{}; // checks if bot already aimed at this waypoint

	uint16_t m_prevTravelFlags{}; // prev of it
	uint16_t m_currentTravelFlags{}; // connection flags like jumping

	Vector m_throw{}; // origin of waypoint to throw grenades
	Vector m_lookAt{}; // vector bot should look at
	Vector m_lookVelocity{}; // interpolate velocity
	float m_lookYawVel{}; // look yaw velocity
	float m_lookPitchVel{}; // look pich velocity
	bool m_updateLooking{}; // should bot update where to look?

	edict_t* m_hostages[Const_MaxHostages]{}; // pointer to used hostage entities
	Vector m_lastWallOrigin{}; // for better zombie avoiding

	bool m_isStuck{}; // bot is stuck
	uint8_t m_stuckWarn{};
	Vector m_stuckArea{};
	float m_stuckTimer{};

	float m_msecInterval{}; // used for leon hartwig's method for msec calculation
	float m_frameInterval{}; // bot's frame interval
	float m_aimInterval{};

	float m_zoomCheckTime{}; // time to check zoom again
	float m_shieldCheckTime{}; // time to check shiled drawing again

	uint8_t m_combatStrafeDir{}; // direction to strafe
	uint8_t m_fightStyle{}; // combat style to use
	float m_lastFightStyleCheck{}; // time checked style
	float m_strafeSetTime{}; // time strafe direction was set

	float m_duckTime{}; // time to duck
	float m_jumpTime{}; // time last jump happened
	float m_buttonPushTime{}; // time to push the button

	Vector m_moveAngles{}; // bot move angles

	int m_zhCampPointIndex{}; // zombie stuff index
	int m_myMeshWaypoint{}; // human mesh stuff index

	void SwitchChatterIcon(const bool show);
	void DebugModeMsg(void);
	void MoveAction(void);
	void PerformWeaponPurchase(void);

	bool CanJumpUp(const Vector& normal);
	bool CantMoveForward(const Vector& normal);

	void CheckMessageQueue(void);
	void CheckRadioCommands(void);
	void CheckBurstMode(const float distance);
	int GetMaxClip(const int& id);

	void CheckSilencer(void);
	bool CheckWallOnForward(void);
	bool CheckWallOnBehind(void);
	bool CheckWallOnLeft(void);
	bool CheckWallOnRight(void);

	bool UpdateLiftHandling(void);
	bool UpdateLiftStates(void);
	bool IsRestricted(const int weaponIndex);

	bool IsInViewCone(const Vector& origin);
	bool CheckVisibility(edict_t* targetEntity);
	bool CheckGrenadeThrow(edict_t* targetEntity);

	edict_t* FindNearestButton(const char* className);
	edict_t* FindButton(void);
	int FindDefendWaypoint(const Vector& origin);
	int FindGoal(void);

	int GetMessageQueue(void);
	float InFieldOfView(const Vector& dest);

	bool IsBombDefusing(const Vector& bombOrigin);
	bool IsWaypointOccupied(const int index);

	bool IsNotAttackLab(edict_t* entity);

	inline bool IsOnLadder(void) { return pev->movetype == MOVETYPE_FLY; }
	inline bool IsOnFloor(void) { return !!(pev->flags & (FL_ONGROUND | FL_PARTIALGROUND)); }
	inline bool IsInWater(void) { return pev->waterlevel == 3; }

	bool ItemIsVisible(const Vector& dest, char* itemName);
	bool IsBehindSmokeClouds(edict_t* ent);

	bool RateGroundWeapon(edict_t* ent);
	void SetStrafeSpeed(const Vector& moveDir, const float strafeSpeed);
	void SetStrafeSpeedNoCost(const Vector& moveDir, const float strafeSpeed);
	void StartGame(void);
	int GetBestWeaponCarried(void);
	int GetBestSecondaryWeaponCarried(void);

	void ChangeWptIndex(const int waypointIndex);
	bool IsDeadlyDrop(const Vector& targetOriginPos);
	bool CampingAllowed(void);
	bool OutOfBombTimer(void);
	void SetWaypointOrigin(void);

	Vector CheckToss(const Vector& start, const Vector& end);
	Vector CheckThrow(const Vector& start, const Vector& end);
	Vector GetEnemyPosition(void);
	float GetZOffset(const float distance);

	int GetNearbyFriendsNearPosition(const Vector& origin, const float radius);
	int GetNearbyEnemiesNearPosition(const Vector& origin, const float radius);

	int CheckGrenades(void);
	bool IsWeaponBadInDistance(const int weaponIndex, const float distance);
	bool DoFirePause(const float distance);
	void FireWeapon(void);

	bool KnifeAttack(const float attackDistance = 64.0f);

	void SelectBestWeapon(void);
	void SelectPistol(void);
	void SelectKnife(void);
	bool IsFriendInLineOfFire(const float distance);
	void SelectWeaponByName(const char* name);
	void SelectWeaponbyNumber(int num);
	int GetHighestWeapon(void);

	bool IsEnemyProtectedByShield(edict_t* enemy);
	bool ParseChat(char* reply);
	bool RepliesToPlayer(void);
	float GetBombTimeleft(void);
	float GetEstimatedReachTime(void);

	void FindPath(int& srcIndex, int& destIndex, edict_t* enemy = nullptr);
	void FindShortestPath(int& srcIndex, int& destIndex);
	void FindEscapePath(int& srcIndex, const Vector& dangerOrigin);
	void CalculatePing(void);
public:
	entvars_t* pev{};

	uint16_t m_numSpawns{}; // used for path randomizing
	int m_wantedTeam{}; // player team bot wants select
	int m_wantedClass{}; // player model bot wants to select
	int m_difficulty{}; // bot difficulty
	int m_basePingLevel{}; // base ping level for randomizing

	int m_skill{}; // bots play skill
	int m_moneyAmount{}; // amount of money in bot's bank
	int m_personality{}; // bot's personality
	bool m_isVIP{}; // bot is vip?
	bool m_isBomber{}; // bot is bomber?

	int m_startAction{}; // team/class selection state
	int m_retryJoin{}; // how many times bot must retry
	int m_team{}; // bot's team
	int m_index{}; // bot's index
	bool m_isAlive{}; // has the player been killed or has he just respawned
	bool m_notStarted{}; // team/class not chosen yet
	bool m_isZombieBot{}; // checks bot if zombie
	int8_t m_voicePitch{}; // bot's voice pitch

	int8_t m_voteMap{}; // number of map to vote for
	bool m_inBombZone{}; // bot in the bomb zone or not
	int8_t m_buyState{}; // current Count in Buying
	float m_nextBuyTime{}; // next buy time

	bool m_inBuyZone{}; // bot currently in buy zone
	bool m_inVIPZone{}; // bot in the vip satefy zone
	bool m_buyingFinished{}; // done with buying
	bool m_hasDefuser{}; // does bot has defuser
	bool m_hasProgressBar{}; // has progress bar on a HUD
	bool m_jumpReady{}; // is double jump ready

	edict_t* m_doubleJumpEntity{}; // pointer to entity that request double jump
	edict_t* m_radioEntity{}; // pointer to entity issuing a radio command
	int8_t m_radioOrder{}; // actual command

	int m_actMessageIndex{}; // current processed message
	int m_pushMessageIndex{}; // offset for next pushed message

	Vector m_waypointOrigin{}; // origin of waypoint
	Vector m_destOrigin{}; // origin of move destination
	Vector m_doubleJumpOrigin{}; // origin of double jump

	SayText m_sayTextBuffer{}; // holds the index & the actual message of the last unprocessed text message of a player
	BurstMode m_weaponBurstMode{}; // bot using burst mode? (famas/glock18, but also silencer mode)
	int m_pingOffset[2]{}; // offset for faking pings
	int m_ping[3]{}; // bots pings in scoreboard
	bool m_isEnemyReachable{}; // direct line to enemy

	float m_seeEnemyTime{}; // time bot sees enemy
	float m_radiotimer{}; // a timer for radio call
	float m_randomattacktimer{}; // a timer for make bots random attack with knife like humans
	float m_slowthinktimer{}; // slow think timer
	float m_stayTime{}; // stay time (for simulate server)

	bool m_isSlowThink{}; // bool for check is slow think? (every second)
	float m_firePause{}; // time to pause firing

	uint8_t m_currentWeapon{}; // one current weapon for each bot
	int m_ammoInClip[Const_MaxWeapons]{}; // ammo in clip for each weapons
	int m_ammo[MAX_AMMO_SLOTS]{}; // total ammo amounts

	Path m_waypoint{}; // current waypoint
	PathNode m_navNode{}; // pointer to current node from path
	int8_t m_visibility{}; // visibility flags

	// NEW VARS
	Process m_currentProcess{};
	Process m_rememberedProcess{};
	float m_currentProcessTime{};
	float m_rememberedProcessTime{};

	bool m_hasEnemiesNear{};
	bool m_hasFriendsNear{};
	bool m_hasEntitiesNear{};
	int8_t m_enemiesNearCount{};
	int8_t m_friendsNearCount{};
	int8_t m_entitiesNearCount{};
	int8_t m_numEnemiesLeft{};
	int8_t m_numFriendsLeft{};
	int8_t m_numEntitiesLeft{};
	float m_enemyDistance{};
	float m_friendDistance{};
	float m_entityDistance{};
	float m_enemySeeTime{};
	float m_friendSeeTime{};
	float m_entitySeeTime{};
	edict_t* m_nearestEnemy{};
	edict_t* m_nearestFriend{};
	edict_t* m_nearestEntity{};
	Vector m_enemyOrigin{};
	Vector m_friendOrigin{};
	Vector m_entityOrigin{};

	int8_t m_senseChance{};

	float m_searchTime{};
	float m_pauseTime{};
	float m_walkTime{};
	float m_spawnTime{};

	float m_frameDelay{};
	float m_baseUpdate{};
	bool m_hasProfile{};

	int m_campIndex{};
	int m_weaponPrefs[Const_NumWeapons]{};

	MiniArray <char*> m_favoritePrimary{};
	MiniArray <char*> m_favoriteSecondary{};
	MiniArray <char*> m_favoriteStuff{};

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
	void MoveTo(const Vector& targetPosition);
	void MoveOut(const Vector& targetPosition);
	void FollowPath(void);
	void FindFriendsAndEnemiens(void);
	void FindEnemyEntities(void);

	bool IsEnemyViewable(edict_t* player);
	bool AllowPickupItem(void);

	void CheckStuck(const float maxSpeed);
	void ResetStuck(void);
	void FindItem(void);
	bool GetNextBestWaypoint(void);
	void AutoJump(void);
	void DoWaypointNav(void);

	int FindWaypoint(void);
	int FindNearestReachableWaypoint(const float maxDistance = 512.0f);

	bool IsAttacking(const edict_t* player);
	bool IsEnemyReachable(void);
	bool IsEnemyReachableToPosition(const Vector& origin);
	bool IsFriendReachableToPosition(const Vector& origin);
	bool CanIReachToPosition(const Vector& origin);

	void SetWalkTime(const float time);
	float GetMaxSpeed(void);
	float GetTargetDistance(void);

	// SSM
	void DefaultStart(void);
	void AttackStart(void);
	void PlantStart(void);
	void DefuseStart(void);
	void EscapeStart(void);
	void CampStart(void);
	void PauseStart(void);
	void DestroyBreakableStart(void);
	void PickupStart(void);
	void ThrowHEStart(void);
	void ThrowFBStart(void);
	void ThrowSMStart(void);
	void BlindStart(void);
	void JumpStart(void);

	void DefaultUpdate(void);
	void AttackUpdate(void);
	void PlantUpdate(void);
	void DefuseUpdate(void);
	void EscapeUpdate(void);
	void CampUpdate(void);
	void PauseUpdate(void);
	void DestroyBreakableUpdate(void);
	void PickupUpdate(void);
	void ThrowHEUpdate(void);
	void ThrowFBUpdate(void);
	void ThrowSMUpdate(void);
	void BlindUpdate(void);
	void JumpUpdate(void);

	void DefaultEnd(void);
	void AttackEnd(void);
	void PlantEnd(void);
	void DefuseEnd(void);
	void EscapeEnd(void);
	void CampEnd(void);
	void PauseEnd(void);
	void DestroyBreakableEnd(void);
	void PickupEnd(void);
	void ThrowHEEnd(void);
	void ThrowFBEnd(void);
	void ThrowSMEnd(void);
	void BlindEnd(void);
	void JumpEnd(void);

	bool DefaultReq(void);
	bool AttackReq(void);
	bool PlantReq(void);
	bool DefuseReq(void);
	bool EscapeReq(void);
	bool CampReq(void);
	bool PauseReq(void);
	bool DestroyBreakableReq(void);
	bool PickupReq(void);
	bool ThrowHEReq(void);
	bool ThrowFBReq(void);
	bool ThrowSMReq(void);
	bool BlindReq(void);
	bool JumpReq(void);

	int GetWeaponID(const char* weapon);
	inline int GetAmmoInClip(void) { return m_ammoInClip[m_currentWeapon]; }

	inline edict_t* GetEntity(void)
	{
		if (!pev)
			return nullptr;

		return pev->pContainingEntity;
	}

	inline EOFFSET GetOffset(void) { return OFFSET(pev); };
	inline int GetIndex(void) { return ENTINDEX(GetEntity()); };

	inline Vector Center(void) { return (pev->absmax + pev->absmin) * 0.5f; };
	inline Vector EyePosition(void) { return pev->origin + pev->view_ofs; };

	void LookAt(const Vector& origin, const Vector& velocity = nullvec);
	void NewRound(void);
	void PushMessageQueue(const int message);
	void PrepareChatMessage(char* text);
	bool EntityIsVisible(const Vector& dest, const bool fromBody = false);

	void CheckTouchEntity(edict_t* entity);
	void TakeBlinded(const Vector& fade, const int alpha);
	void DiscardWeaponForUser(edict_t* user, const bool discardC4);

	void ChatSay(const bool teamSay, const char* text, ...);
	void ChatMessage(const int type, const bool isTeamSay = false);
	void RadioMessage(const int message);
	void PlayChatterMessage(const ChatterMessage& message);

	void Kill(void);
	void Kick(void);
	void ResetDoubleJumpState(void);
	int FindLoosedBomb(void);

	int FindHostage(void);

	bool HasHostage(void);
	bool UsesRifle(void);
	bool UsesPistol(void);
	bool UsesShotgun(void);
	bool UsesSniper(void);
	bool UsesSubmachineGun(void);
	bool UsesZoomableRifle(void);
	bool UsesBadPrimary(void);
	bool HasPrimaryWeapon(void);
	bool HasSecondaryWeapon(void);
	bool HasShield(void);
	bool IsSniper(void);
	bool IsShieldDrawn(void);
};

// manager class
class BotControl : public Singleton <BotControl>
{
private:
	MiniArray <CreateItem> m_creationTab{}; // bot creation tab
	float m_maintainTime{}; // time to maintain bot creation quota
	int8_t m_lastWinner{}; // the team who won previous round
	bool m_economicsGood[2]{}; // is team able to buy anything
protected:
	int CreateBot(String name, int skill, int personality, const int team, const int member);
public:
	Bot* m_bots[32]{}; // all available bots

	MiniArray <String> m_savedBotNames{}; // storing the bot names
	MiniArray <String> m_avatars{}; // storing the steam ids

	BotControl(void);
	~BotControl(void);

	bool EconomicsValid(const int team) { return m_economicsGood[team]; }

	int GetLastWinner(void) const { return m_lastWinner; }
	void SetLastWinner(const int8_t winner) { m_lastWinner = winner; }

	int GetIndex(edict_t* ent);
	int GetIndex(const int index);
	Bot* GetBot(const int index);
	Bot* GetBot(edict_t* ent);
	Bot* FindOneValidAliveBot(void);
	Bot* GetHighestSkillBot(const int team);
	void SelectLeaderEachTeam(const int team);

	int GetBotsNum(void);
	int GetHumansNum(void);

	void Think(void);
	void DoJoinQuitStuff(void);
	void Free(void);
	void Free(const int index);

	void AddRandom(void) { AddBot("", -1, -1, -1, -1); }
	void AddBot(const String& name, const int skill, const int personality, const int team, const int member);
	void FillServer(int selection, const int personality = Personality::Normal, const int skill = -1, const int numToAdd = -1);

	void RemoveAll(void);
	void RemoveRandom(void);
	void RemoveFromTeam(const Team team, const bool removeAll = false);
	void RemoveMenu(edict_t* ent, const int selection);
	void KillAll(const int team = Team::Count);
	void MaintainBotQuota(void);
	void InitQuota(void);

	void ListBots(void);
	void SetWeaponMode(int selection);
	void CheckTeamEconomics(const int team);

	int AddBotAPI(const String& name, const int skill, const int team);
	static void CallGameEntity(entvars_t* vars);
};

// netmessage handler class
class NetworkMsg : public Singleton <NetworkMsg>
{
private:
	Bot* m_bot{};
	int8_t m_state{};
	int m_message{};
	int m_registerdMessages[NETMSG_NUM]{};
public:
	NetworkMsg(void);
	~NetworkMsg(void) { };

	void Execute(void* p);
	void Reset(void) { m_message = NETMSG_UNDEFINED; m_state = 0; m_bot = nullptr; };
	void HandleMessageIfRequired(const int messageType, const int requiredType);

	void SetMessage(const int message) { m_message = message; }
	void SetBot(Bot* bot) { m_bot = bot; }

	int GetId(const int messageType) { return m_registerdMessages[messageType]; }
	void SetId(const int messageType, const int messsageIdentifier) { m_registerdMessages[messageType] = messsageIdentifier; }
};

// waypoint operation class
class Waypoint : public Singleton <Waypoint>
{
	friend class Bot;
private:
	bool m_isOnLadder{};
	bool m_endJumpPoint{};
	bool m_learnJumpWaypoint{};
	float m_timeJumpStarted{};

	bool m_ladderPoint{};
	bool m_fallPoint{};
	int16_t m_lastFallWaypoint{};

	Vector m_learnVelocity{};
	Vector m_learnPosition{};
	Vector m_foundBombOrigin{};
	Vector m_fallPosition{};

	int16_t m_cacheWaypointIndex{};
	int16_t m_lastJumpWaypoint{};
	Vector m_lastWaypoint{};

	int16_t m_lastDeclineWaypoint{};

	float m_pathDisplayTime{};
	float m_arrowDisplayTime{};
	float* m_waypointDisplayTime{};
	int16_t m_findWPIndex{};
	int16_t m_facingAtIndex{};
	char m_infoBuffer[256]{};

	MiniArray <int16_t> m_terrorPoints{};
	MiniArray <int16_t> m_ctPoints{};
	MiniArray <int16_t> m_goalPoints{};
	MiniArray <int16_t> m_campPoints{};
	MiniArray <int16_t> m_rescuePoints{};
	MiniArray <int16_t> m_zmHmPoints{};
	MiniArray <int16_t> m_hmMeshPoints{};
public:
	MiniArray <Path> m_paths{};
	Waypoint(void);
	~Waypoint(void);

	void Initialize(void);
	void Analyze(void);
	void AnalyzeDeleteUselessWaypoints(void);
	void InitTypes(void);
	void AddPath(const int addIndex, const int pathIndex, const int type = 0);

	int GetFacingIndex(void);
	int FindFarest(const Vector& origin, const float maxDistance = 99999.0f);
	int FindNearestInCircle(const Vector& origin, const float maxDistance = 99999.0f);
	int FindNearest(const Vector& origin, const float minDistance = 99999.0f, const int flags = -1, edict_t* entity = nullptr, int* findWaypointPoint = reinterpret_cast<int*>(-2), const int mode = -1);
	void FindInRadius(const Vector& origin, const float radius, int* holdTab, int* count);
	void FindInRadius(MiniArray <int16_t>& queueID, const float& radius, const Vector& origin);

	void Add(const int flags, const Vector& waypointOrigin = nullvec);
	void Delete(void);
	void DeleteByIndex(int index);
	void ToggleFlags(int toggleFlag);
	void SetRadius(const int radius);
	bool IsConnected(const int pointA, const int pointB);
	bool IsConnected(const int num);
	void CreatePath(const int dir);
	void DeletePath(void);
	void DeletePathByIndex(int nodeFrom, int nodeTo);
	void CacheWaypoint(void);

	void DeleteFlags(void);
	void TeleportWaypoint(void);

	float GetTravelTime(const float maxSpeed, const Vector& src, const Vector& origin);
	void CalculateWayzone(const int index);
	Vector GetBottomOrigin(const Path* waypoint);

	void Sort(const int16_t self, int16_t index[], const int size = Const_MaxPathIndex);
	bool Download(void);
	bool Load(void);
	void Save(void);

	bool Reachable(edict_t* entity, const int index);
	bool IsNodeReachable(const Vector& src, const Vector& destination);
	bool IsNodeReachableWithJump(const Vector& src, const Vector& destination);
	void Think(void);
	void ShowWaypointMsg(void);
	bool NodesValid(void);
	void CreateBasic(void);

	float GetPathDistance(const int srcIndex, const int destIndex);

	Path* GetPath(const int id);
	char* GetWaypointInfo(const int id);
	char* GetInfo(void) { return m_infoBuffer; }

	void SetFindIndex(const int index);
	void SetLearnJumpWaypoint(const int mod = -1);

	Vector GetBombPosition(void) { return m_foundBombOrigin; }
	void SetBombPosition(const bool shouldReset = false);
	const char* CheckSubfolderFile(void);
};

#define g_netMsg NetworkMsg::GetObjectPtr()
#define g_botManager BotControl::GetObjectPtr()
#define g_waypoint Waypoint::GetObjectPtr()

// prototypes of bot functions...
extern int GetWeaponReturn(const bool isString, const char* weaponAlias, const int weaponID = -1);
extern int GetTeam(edict_t* ent);
extern GameMode GetGameMode(void);
extern bool IsBreakable(edict_t* ent);
extern bool IsZombieEntity(edict_t* ent);

extern void SetGameMode(const GameMode gamemode);
extern bool IsZombieMode(void);
extern bool IsDeathmatchMode(void);
extern ChatterMessage GetEqualChatter(const int message);
extern char* GetVoice(const ChatterMessage message);
extern float GetShootingConeDeviation(edict_t* ent, const Vector& position);

extern bool IsLinux(void);
extern bool TryFileOpen(char* fileName);
extern bool IsWalkableLineClear(const Vector& from, const Vector& to);
extern bool IsDedicatedServer(void);
extern bool IsVisible(const Vector& origin, edict_t* ent);
extern bool IsAlive(const edict_t* ent);
extern bool IsInViewCone(const Vector& origin, edict_t* ent);
extern bool IsValidBot(edict_t* ent);
extern bool IsValidBot(const int index);
extern bool IsValidPlayer(edict_t* ent);
extern bool OpenConfig(const char* fileName, char* errorIfNotExists, File* outFile);
extern bool FindNearestPlayer(void** holder, edict_t* to, const float searchDistance = 4096.0f, const bool sameTeam = false, const bool needBot = false, const bool needAlive = false, const bool needDrawn = false);

extern char* GetEntityName(edict_t* entity);
extern const char* GetMapName(void);
extern const char* GetWaypointDir(void);
extern const char* GetModName(void);
extern char* GetField(const char* string, const int fieldId, const bool endLine = false);

extern Vector GetWalkablePosition(const Vector& origin, edict_t* ent = nullptr, const bool returnNullVec = false, const float height = 1000.0f);
extern Vector GetEntityOrigin(edict_t* ent);
extern Vector GetBoxOrigin(edict_t* ent);

extern Vector GetTopOrigin(edict_t* ent);
extern Vector GetBottomOrigin(edict_t* ent);
extern Vector GetPlayerHeadOrigin(edict_t* ent);

extern void FreeLibraryMemory(void);
extern void RoundInit(void);
extern void FakeClientCommand(edict_t* fakeClient, const char* format, ...);
extern void CreatePath(char* path);
extern void ServerCommand(const char* format, ...);
extern void RegisterCommand(char* command, void funcPtr(void));
extern void CheckWelcomeMessage(void);
extern void DetectCSVersion(void);
extern void PlaySound(edict_t* ent, const char* soundName);
extern void ServerPrint(const char* format, ...);
extern void ChatPrint(const char* format, ...);
extern void ServerPrintNoTag(const char* format, ...);
extern void CenterPrint(const char* format, ...);
extern void ClientPrint(edict_t* ent, int dest, const char* format, ...);
extern void HudMessage(edict_t* ent, bool toCenter, const Color& rgb, char* format, ...);

extern void AutoLoadGameMode(void);

extern bool SetEntityAction(const int index, const int team, const int action);
extern void SetEntityActionData(const int i, const int index = -1, const int team = -1, const int action = -1);

extern void AddLogEntry(const Log logLevel, const char* format, ...);
extern void MOD_AddLogEntry(const int mode, char* format);

extern void DisplayMenuToClient(edict_t* ent, MenuText* menu);

extern void TraceLine(const Vector& start, const Vector& end, const bool ignoreMonsters, const bool ignoreGlass, edict_t* ignoreEntity, TraceResult* ptr);
extern void TraceLine(const Vector& start, const Vector& end, const bool ignoreMonsters, edict_t* ignoreEntity, TraceResult* ptr);
extern void TraceHull(const Vector& start, const Vector& end, const bool ignoreMonsters, const int hullNumber, edict_t* ignoreEntity, TraceResult* ptr);

inline bool IsNullString(const char* input)
{
	if (!input)
		return true;

	return *input == '\0';
}

// very global convars
extern ConVar ebot_knifemode;
extern ConVar ebot_gamemod;

#include <globals.h>
#include <resource.h>
#include <compress.h>
#include <nav.h>

#endif // EBOT_INCLUDED
