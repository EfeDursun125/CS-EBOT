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

#include <stdio.h>
#include <memory.h>
#include <cmath.h>

using namespace std;

#include <engine.h>

using namespace Math;

#include <platform.h>

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <time.h>

#include <runtime.h>

//#define WORK_ASYNC

#ifdef WORK_ASYNC
#include <future>
#endif

using namespace std;

const int checkEntityNum = 20;
const int checkEnemyNum = 128;

// defines bots tasks
enum BotTask
{
	TASK_NORMAL,
	TASK_PAUSE,
	TASK_MOVETOPOSITION,
	TASK_FOLLOWUSER,
	TASK_PICKUPITEM,
	TASK_CAMP,
	TASK_PLANTBOMB,
	TASK_DEFUSEBOMB,
	TASK_FIGHTENEMY,
	TASK_HUNTENEMY,
	TASK_SEEKCOVER,
	TASK_THROWHEGRENADE,
	TASK_THROWFBGRENADE,
	TASK_THROWSMGRENADE,
	TASK_THROWFLARE,
	TASK_DOUBLEJUMP,
	TASK_ESCAPEFROMBOMB,
	TASK_DESTROYBREAKABLE,
	TASK_HIDE,
	TASK_BLINDED,
	TASK_SPRAYLOGO,
	TASK_MOVETOTARGET,
	TASK_GOINGFORCAMP
};

enum class Process
{
	Default,
	Attack,
	Hide,
	Camp,
	Escape,
	Plant,
	Defuse,
	Pause,
	DestroyBreakable,
	Pickup
};

// supported cs's
enum GameVersion
{
	CSVER_CSTRIKE = 1, // Counter-Strike 1.6 and Above
	CSVER_CZERO = 2, // Counter-Strike: Condition Zero
	CSVER_VERYOLD = 3, // Counter-Strike 1.3-1.5 with/without Steam
	CSVER_XASH = 4, // Xash3D
	HALFLIFE = 5
};

// log levels
enum LogLevel
{
	LOG_DEFAULT = 1, // default log message
	LOG_WARNING = 2, // warning log message
	LOG_ERROR = 3, // error log message
	LOG_IGNORE = 4, // additional flag
	LOG_FATAL = 5, // fatal error log message
	LOG_MEMORY = 6 // memory error log message

};

// chat types id's
enum ChatType
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
enum Personality
{
	PERSONALITY_NORMAL = 0,
	PERSONALITY_RUSHER,
	PERSONALITY_CAREFUL
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

// counter-strike team id's
enum Team
{
	TEAM_TERRORIST = 0,
	TEAM_COUNTER,
	TEAM_COUNT
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
	Nothin = -1,
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

// counter-strike weapon id's
enum Weapon
{
	WEAPON_P228 = 1,
	WEAPON_SHIELDGUN = 2,
	WEAPON_SCOUT = 3,
	WEAPON_HEGRENADE = 4,
	WEAPON_XM1014 = 5,
	WEAPON_C4 = 6,
	WEAPON_MAC10 = 7,
	WEAPON_AUG = 8,
	WEAPON_SMGRENADE = 9,
	WEAPON_ELITE = 10,
	WEAPON_FN57 = 11,
	WEAPON_UMP45 = 12,
	WEAPON_SG550 = 13,
	WEAPON_GALIL = 14,
	WEAPON_FAMAS = 15,
	WEAPON_USP = 16,
	WEAPON_GLOCK18 = 17,
	WEAPON_AWP = 18,
	WEAPON_MP5 = 19,
	WEAPON_M249 = 20,
	WEAPON_M3 = 21,
	WEAPON_M4A1 = 22,
	WEAPON_TMP = 23,
	WEAPON_G3SG1 = 24,
	WEAPON_FBGRENADE = 25,
	WEAPON_DEAGLE = 26,
	WEAPON_SG552 = 27,
	WEAPON_AK47 = 28,
	WEAPON_KNIFE = 29,
	WEAPON_P90 = 30,
	WEAPON_KEVLAR = 31,
	WEAPON_KEVHELM = 32,
	WEAPON_DEFUSER = 33
};

// half-life weapon id's
enum WeaponHL
{
	WEAPON_CROWBAR = 1,
	WEAPON_GLOCK = 2,
	WEAPON_PYTHON = 3,
	WEAPON_MP5_HL = 4,
	WEAPON_CHAINGUN = 5,
	WEAPON_CROSSBOW = 6,
	WEAPON_SHOTGUN = 7,
	WEAPON_RPG = 8,
	WEAPON_GAUSS = 9,
	WEAPON_EGON = 10,
	WEAPON_HORNETGUN = 11,
	WEAPON_HANDGRENADE = 12,
	WEAPON_TRIPMINE = 13,
	WEAPON_SATCHEL = 14,
	WEAPON_SNARK = 15
};

// defines for pickup items
enum PickupType
{
	PICKTYPE_NONE,
	PICKTYPE_WEAPON,
	PICKTYPE_DROPPEDC4,
	PICKTYPE_PLANTEDC4,
	PICKTYPE_HOSTAGE,
	PICKTYPE_BUTTON,
	PICKTYPE_SHIELDGUN,
	PICKTYPE_DEFUSEKIT,
	PICKTYPE_GETENTITY
};

// reload state
enum ReloadState
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

// sensing states
enum SensingState
{
	STATE_SEEINGENEMY = (1 << 0), // seeing an enemy
	STATE_HEARENEMY = (1 << 1), // hearing an enemy
	STATE_PICKUPITEM = (1 << 2), // pickup item nearby
	STATE_THROWEXPLODE = (1 << 3), // could throw he grenade
	STATE_THROWFLASH = (1 << 4), // could throw flashbang
	STATE_THROWSMOKE = (1 << 5) // could throw smokegrenade
};

// positions to aim at
enum AimDest
{
	AIM_NAVPOINT = (1 << 0), // aim at nav point
	AIM_CAMP = (1 << 1), // aim at camp vector
	AIM_PREDICTENEMY = (1 << 2), // aim at predicted path
	AIM_LASTENEMY = (1 << 3), // aim at last enemy
	AIM_ENTITY = (1 << 4), // aim at entity like buttons, hostages
	AIM_ENEMY = (1 << 5), // aim at enemy
	AIM_GRENADE = (1 << 6), // aim for grenade throw
	AIM_OVERRIDE = (1 << 7),  // overrides all others (blinded)
};

// famas/glock burst mode status + m4a1/usp silencer
enum BurstMode
{
	BURST_ENABLED = 1,
	BURST_DISABLED = 2
};

// visibility flags
enum Visibility
{
	None = (1 << 0),
	Head = (1 << 1),
	Body = (1 << 2),
	Other = (1 << 3)
};

// defines map type
enum MapType
{
	MAP_AS = (1 << 0),
	MAP_CS = (1 << 1),
	MAP_DE = (1 << 2),
	MAP_ES = (1 << 3),
	MAP_KA = (1 << 4),
	MAP_FY = (1 << 5),
	MAP_AWP = (1 << 6),
	MAP_HE = (1 << 7),
	MAP_ZE = (1 << 8)
};

// defines for waypoint flags field (32 bits are available)
enum WaypointFlag
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

enum NavAreaFlag
{
	NAV_JUMP = (1 << 1),
	NAV_CROUCH = (1 << 2),
	NAV_LADDER = (1 << 3),
	NAV_DONT_HIDE = (1 << 4)
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
enum GameMode
{
	MODE_DM = 1,
	MODE_ZP = 2,
	MODE_NOTEAM = 3,
	MODE_ZH = 4,
	MODE_TDM = 5,
	MODE_BASE = 0
};

enum class LiftState
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

struct BotProfile
{
	int favoriteWeapons[26];
	Personality personality;
	int campChance;
};

// bot known file 
const char FH_WAYPOINT_NEW[] = "EBOTWAY!";
const char FH_WAYPOINT[] = "PODWAY!";
const char FH_VISTABLE[] = "PODVIS!";

const int FV_WAYPOINT = 7;

// some hardcoded desire defines used to override calculated ones
const float TASKPRI_NORMAL = 35.0f;
const float TASKPRI_PAUSE = 36.0f;
const float TASKPRI_CAMP = 37.0f;
const float TASKPRI_GOINGFORCAMP = 38.0f;
const float TASKPRI_SPRAYLOGO = 39.0f;
const float TASKPRI_FOLLOWUSER = 41.0f;
const float TASKPRI_MOVETOPOSITION = 50.0f;
const float TASKPRI_DEFUSEBOMB = 89.0f;
const float TASKPRI_PLANTBOMB = 89.0f;
const float TASKPRI_FIGHTENEMY = 90.0f;
const float TASKPRI_SEEKCOVER = 91.0f;
const float TASKPRI_HIDE = 92.0f;
const float TASKPRI_MOVETOTARGET = 93.0f;
const float TASKPRI_THROWGRENADE = 99.0f;
const float TASKPRI_DOUBLEJUMP = 99.0f;
const float TASKPRI_BLINDED = 100.0f;
const float TASKPRI_SHOOTBREAKABLE = 100.0f;
const float TASKPRI_ESCAPEFROMBOMB = 100.0f;

const int Const_GrenadeTimer = 3;
const int Const_MaxHostages = 8;
const int Const_MaxPathIndex = 8;
const int Const_MaxDamageValue = 2040;
const int Const_MaxGoalValue = 2040;
const int Const_MaxKillHistory = 16;
const int Const_MaxRegMessages = 256;
const int Const_MaxWaypoints = 8192;
const int Const_MaxWeapons = 32;
const int Const_NumWeapons = 26;
const int Const_NumWeaponsHL = 15;

// A* Stuff
enum class State { Open, Closed, New };

struct AStar_t
{
	float g;
	float f;
	int parent;

	State state;
};

// weapon masks
const int WeaponBits_Primary = ((1 << WEAPON_XM1014) | (1 << WEAPON_M3) | (1 << WEAPON_MAC10) | (1 << WEAPON_UMP45) | (1 << WEAPON_MP5) | (1 << WEAPON_TMP) | (1 << WEAPON_P90) | (1 << WEAPON_AUG) | (1 << WEAPON_M4A1) | (1 << WEAPON_SG552) | (1 << WEAPON_AK47) | (1 << WEAPON_SCOUT) | (1 << WEAPON_SG550) | (1 << WEAPON_AWP) | (1 << WEAPON_G3SG1) | (1 << WEAPON_M249) | (1 << WEAPON_FAMAS) | (1 << WEAPON_GALIL));
const int WeaponBits_Secondary = ((1 << WEAPON_P228) | (1 << WEAPON_ELITE) | (1 << WEAPON_USP) | (1 << WEAPON_GLOCK18) | (1 << WEAPON_DEAGLE) | (1 << WEAPON_FN57));

// this structure links waypoints returned from pathfinder
struct PathNode
{
	int index;
	PathNode* next;
};

// links keywords and replies together
struct KwChat
{
	Array <String> keywords;
	Array <String> replies;
	Array <String> usedReplies;
};

// tasks definition
struct Task
{
	Task* prev; // previuous task in the linked list
	Task* next; // next task in the linked list
	BotTask taskID; // major task/action carried out
	float desire; // desire (filled in) for this task
	int data; // additional data (waypoint index)
	float time; // time task expires
	bool canContinue; // if task can be continued if interrupted
};

// botname structure definition
struct NameItem
{
	String name;
	bool isUsed;
};

struct WeaponSelect
{
	int id; // the weapon id value
	char* weaponName; // name of the weapon when selecting it
	char* modelName; // model name to separate cs weapons
	int price; // price when buying
	int minPrimaryAmmo; // minimum primary ammo
	int teamStandard; // used by team (number) (standard map)
	int teamAS; // used by team (as map)
	int buyGroup; // group in buy menu (standard map)
	int buySelect; // select item in buy menu (standard map)
	int newBuySelectT; // for counter-strike v1.6
	int newBuySelectCT; // for counter-strike v1.6
	bool shootsThru; // can shoot thru walls
	bool primaryFireHold; // hold down primary fire button to use?
};

// skill definitions
struct SkillDef
{
	float minSurpriseTime; // surprise delay (minimum)
	float maxSurpriseTime; // surprise delay (maximum)
	float campStartDelay; // delay befor start camping
	float campEndDelay; // delay before end camping
	float minTurnSpeed; // initial minimum turnspeed
	float maxTurnSpeed; // initial maximum turnspeed
	float aimOffs_X; // X/Y/Z maximum offsets
	float aimOffs_Y; // X/Y/Z maximum offsets
	float aimOffs_Z; // X/Y/Z maximum offsets
	int headshotFrequency; // precent to aiming to player head
	int heardShootThruProb; // precent to shooting throug wall when seen something
	int seenShootThruProb; // precent to shooting throug wall when heard something
	int recoilAmount; // amount of recoil when the bot should pause shooting
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
	MenuText* menu; // pointer to opened bot menu
	edict_t* ent; // pointer to actual edict
	Vector origin; // position in the world
	Vector soundPosition; // position sound was played
	int team; // bot team
	int flags; // client flags
	float hearingDistance; // distance this sound is heared
	float timeSoundLasting; // time sound is played/heared

	int wpIndex;
	int wpIndex2;
	float getWPTime;
	Vector getWpOrigin;

	int index; // client index
};

// bot creation tab
struct CreateItem
{
	int skill;
	int team;
	int member;
	int personality;
	String name;
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
	int flags; // flags???
};

// define chatting collection structure
struct SayText
{
	int chatProbability;
	float chatDelay;
	float timeNextChat;
	int entityIndex;
	char sayText[512];
	Array <String> lastUsedSentences;
};

// general waypoint header information structure
struct WaypointHeader
{
	char header[8];
	int32 fileVersion;
	int32 pointNumber;
	char mapName[32];
	char author[32];
};

struct NavHeader
{
	int32 fileVersion;
	int32 navNumber;
	char author[32];
};

// define general waypoint structure
struct Path
{
	int32 pathNumber;
	int32 flags;
	Vector origin;
	float radius;

	float campStartX;
	float campStartY;
	float campEndX;
	float campEndY;

	int16 index[Const_MaxPathIndex];
	uint16 connectionFlags[Const_MaxPathIndex];
	Vector connectionVelocity[Const_MaxPathIndex];
	int32 distances[Const_MaxPathIndex];

	struct Vis_t { uint16 stand, crouch; } vis;
};

struct NavArea
{
	int index;
	int32 flags;
	Array <NavArea*> connections;
	Array <float> bakedDist;
	Vector corners[4];
};

// main bot class
class Bot
{
	friend class BotControl;

private:
	unsigned int m_states; // sensing bitstates
	Task* m_tasks; // pointer to active tasks/schedules

	float m_moveSpeed; // current speed forward/backward
	float m_strafeSpeed; // current speed sideways
	float m_tempstrafeSpeed; // temp speed sideways
	float m_minSpeed; // minimum speed in normal mode
	float m_oldCombatDesire; // holds old desire for filtering

	bool m_isLeader; // bot is leader of his team
	bool m_checkTerrain; // check for terrain
	bool m_moveToC4; // ct is moving to bomb

	float m_prevTime; // time previously checked movement speed
	float m_prevSpeed; // speed some frames before
	Vector m_prevOrigin; // origin some frames before

	int m_messageQueue[32]; // stack for messages
	char m_tempStrings[512]; // space for strings (say text...)
	char m_lastStrings[161]; // for block looping same text
	edict_t* m_lastChatEnt; // for block looping message from same bot
	int m_tryOpenDoor; // attempt's to open the door
	int m_radioSelect; // radio entry

	LiftState m_liftState; // state of lift handling
	float m_liftUsageTime; // time to use lift
	edict_t* m_liftEntity; // pointer to lift entity
	Vector m_liftTravelPos; // lift travel position

	float m_blindRecognizeTime; // time to recognize enemy
	float m_itemCheckTime; // time next search for items needs to be done
	PickupType m_pickupType; // type of entity which needs to be used/picked up
	Vector m_breakable; // origin of breakable
	Vector m_lastDamageOrigin; // last damage origin

	edict_t* m_pickupItem; // pointer to entity of item to use/pickup
	edict_t* m_itemIgnore; // pointer to entity to ignore for pickup
	edict_t* m_breakableEntity; // pointer to breakable entity

	float m_timeDoorOpen; // time to next door open check
	float m_lastChatTime; // time bot last chatted
	float m_timeLogoSpray; // time bot last spray logo
	float m_knifeAttackTime; // time to rush with knife (at the beginning of the round)
	bool m_defendedBomb; // defend action issued

	float m_damageTime; // tweak for zombie bots
	float m_askCheckTime; // time to ask team
	float m_collideTime; // time last collision
	float m_firstCollideTime; // time of first collision
	float m_probeTime; // time of probing different moves
	float m_lastCollTime; // time until next collision check
	int m_collisionProbeBits; // bits of possible collision moves
	int m_collideMoves[3]; // sorted array of movements
	int m_collStateIndex; // index into collide moves
	CollisionState m_collisionState; // collision State

	PathNode* m_navNode; // pointer to current node from path
	PathNode* m_navNodeStart; // pointer to start of path finding nodes
	uint8_t m_visibility; // visibility flags

	int m_cachedWaypointIndex; // for zombies
	int m_currentWaypointIndex; // current waypoint index
	int m_travelStartIndex; // travel start index to double jump action
	int m_prevWptIndex[3]; // previous waypoint indices from waypoint find
	int32 m_waypointFlags; // current waypoint flags
	int m_loosedBombWptIndex; // nearest to loosed bomb waypoint

	unsigned short m_currentTravelFlags; // connection flags like jumping
	bool m_jumpFinished; // has bot finished jumping
	Vector m_desiredVelocity; // desired velocity for jump waypoints
	float m_navTimeset; // time waypoint chosen by Bot

	unsigned int m_aimFlags; // aiming conditions
	Vector m_lookAt; // vector bot should look at
	Vector m_lookAtCache; // cache for look at
	Vector m_throw; // origin of waypoint to throw grenades
	Vector m_idealAngles; // ideal aim angles
	float m_lookYawVel; // look yaw velocity
	float m_lookPitchVel; // look pich velocity

	Vector m_grenade; // calculated vector for grenades
	Vector m_entity; // origin of entities like buttons etc.
	Vector m_camp; // aiming vector when camping.

	bool m_wantsToFire; // bot needs consider firing
	float m_shootAtDeadTime; // time to shoot at dying players
	edict_t* m_avoidEntity; // pointer to grenade entity to avoid
	char m_needAvoidEntity; // which direction to strafe away

	float m_followWaitTime; // wait to follow time
	edict_t* m_targetEntity; // the entity that the bot is trying to reach
	edict_t* m_hostages[Const_MaxHostages]; // pointer to used hostage entities

	Vector m_moveTargetOrigin; // ...
	Vector m_lastWallOrigin; // for better zombie avoiding

	bool m_isStuck; // bot is stuck
	int m_stuckWarn;
	Vector m_stuckArea;
	float m_stuckTimer;

	bool m_isReloading; // bot is reloading a gun
	int m_reloadState; // current reload state
	int m_voicePitch; // bot voice pitch
	bool m_zombiePush; // we must push???
	bool m_allowWalk; // walk for a few seconds

	bool m_duckDefuse; // should or not bot duck to defuse bomb
	float m_duckDefuseCheckTime; // time to check for ducking for defuse

	float m_msecInterval; // used for leon hartwig's method for msec calculation
	float m_impulse; // lol

	float m_frameInterval; // bot's frame interval
	float m_aimInterval; // bot's aim interval

	float m_reloadCheckTime; // time to check reloading
	float m_zoomCheckTime; // time to check zoom again
	float m_shieldCheckTime; // time to check shiled drawing again
	float m_grenadeCheckTime; // time to check grenade usage

	bool m_checkKnifeSwitch; // is time to check switch to knife action
	bool m_checkWeaponSwitch; // is time to check weapon switch
	bool m_isUsingGrenade; // bot currently using grenade??

	uint8_t m_combatStrafeDir; // direction to strafe
	uint8_t m_fightStyle; // combat style to use
	float m_lastFightStyleCheck; // time checked style
	float m_strafeSetTime; // time strafe direction was set

	float m_timeCamping; // time to camp
	int m_campDirection; // camp Facing direction
	float m_nextCampDirTime; // time next camp direction change
	int m_campButtons; // buttons to press while camping
	int m_doorOpenAttempt; // attempt's to open the door

	float m_duckTime; // time to duck
	float m_jumpTime; // time last jump happened
	float m_soundUpdateTime; // time to update the sound
	float m_heardSoundTime; // last time noise is heard
	float m_buttonPushTime; // time to push the button

	Vector m_moveAngles; // bot move angles
	bool m_moveToGoal; // bot currently moving to goal??
	float m_playerTargetTime; // time last targeting

	float m_checkCampPointTime; // zombie stuff
	int m_zhCampPointIndex; // zombie stuff index
	int m_myMeshWaypoint; // human mesh stuff index

	void BotAI(void);
	void DebugModeMsg(void);
	void MoveAction(void);
	bool IsMorePowerfulWeaponCanBeBought(void);
	void PerformWeaponPurchase(void);
	int BuyWeaponMode(int weaponId);

	bool CanJumpUp(const Vector normal);
	bool CantMoveForward(const Vector normal);

	void CheckMessageQueue(void);
	void CheckRadioCommands(void);
	void CheckReload(void);
	void CheckBurstMode(float distance);

	int CheckMaxClip(int weaponId, int* weaponIndex);

	void AvoidEntity(void);

	void ZombieModeAi(void);
	void ZmCampPointAction(int mode = 0);

	void CheckSilencer(void);
	bool CheckWallOnForward(void);
	bool CheckWallOnBehind(void);
	bool CheckWallOnLeft(void);
	bool CheckWallOnRight(void);
	void ChooseAimDirection(void);
	int ChooseBombWaypoint(void);

	bool DoWaypointNav(void);
	bool UpdateLiftHandling(void);
	bool UpdateLiftStates(void);
	bool IsRestricted(int weaponIndex);

	bool IsOnAttackDistance(edict_t* targetEntity, float distance);

	bool IsInViewCone(const Vector& origin);
	void ReactOnSound(void);
	bool CheckVisibility(edict_t* targetEntity);

	void CheckGrenadeThrow(void);
	void ThrowFireNade(void);
	void ThrowFrostNade(void);

	edict_t* FindNearestButton(const char* className);
	edict_t* FindButton(void);
	int FindCoverWaypoint(float maxDistance);
	int FindDefendWaypoint(const Vector& origin);
	int FindGoal(void);
	void CheckCloseAvoidance(const Vector& dirNormal);

	void GetCampDirection(Vector* dest);
	int GetMessageQueue(void);
	bool GoalIsValid(void);
	bool HeadTowardWaypoint(void);
	bool HasNextPath(void);
	float InFieldOfView(const Vector& dest);

	bool IsBombDefusing(Vector bombOrigin);
	bool IsWaypointOccupied(int index, bool needZeroVelocity = false);

	bool IsNotAttackLab(edict_t* entity);

	inline bool IsOnLadder(void) { return pev->movetype == MOVETYPE_FLY; }
	inline bool IsOnFloor(void) { return (pev->flags & (FL_ONGROUND | FL_PARTIALGROUND)) != 0; }
	inline bool IsInWater(void) { return pev->waterlevel >= 2; }

	float GetWalkSpeed(void);

	bool ItemIsVisible(Vector dest, char* itemName);
	bool LastEnemyShootable(void);
	bool IsBehindSmokeClouds(edict_t* ent);
	void TaskNormal(int i, int destIndex, Vector src);
	void RunTask(void);
	void CheckTasksPriorities(void);
	void PushTask(Task* task);

	bool IsShootableBreakable(edict_t* ent);
	bool RateGroundWeapon(edict_t* ent);
	bool ReactOnEnemy(void);
	void ResetCollideState(void);
	void SetConditions(void);
	void SetStrafeSpeed(Vector moveDir, float strafeSpeed);
	void StartGame(void);
	void TaskComplete(void);
	bool GetBestNextWaypoint(void);
	int GetBestWeaponCarried(void);
	int GetBestSecondaryWeaponCarried(void);

	void RunPlayerMovement(void);
	void GetValidWaypoint(void);
	void ChangeWptIndex(const int waypointIndex);
	bool IsDeadlyDrop(Vector targetOriginPos);
	bool CampingAllowed(void);
	bool OutOfBombTimer(void);
	void SelectLeaderEachTeam(int team);
	void IgnoreCollisionShortly(void);
	void SetWaypointOrigin(void);

	Vector CheckToss(const Vector& start, Vector end);
	Vector CheckThrow(const Vector& start, Vector end);
	Vector GetEnemyPosition(void);
	Vector CheckBombAudible(void);
	float GetZOffset(float distance);

	int CheckGrenades(void);
	void CommandTeam(void);
	void CombatFight(void);
	bool IsWeaponBadInDistance(int weaponIndex, float distance);
	bool DoFirePause(const float distance);
	bool LookupEnemy(void);
	void FireWeapon(void);
	void FocusEnemy(void);

	bool KnifeAttack(float attackDistance = 0.0f);

	void SelectBestWeapon(void);
	void SelectPistol(void);
	void SelectKnife(void);
	bool IsFriendInLineOfFire(float distance);
	bool IsGroupOfEnemies(Vector location, int numEnemies = 2, float radius = 640.0f);
	bool IsShootableThruObstacle(edict_t* entity);
	int GetNearbyEnemiesNearPosition(Vector origin, float radius);
	int GetNearbyFriendsNearPosition(Vector origin, float radius);
	void SelectWeaponByName(const char* name);
	void SelectWeaponbyNumber(int num);
	int GetHighestWeapon(void);

	void ResetCheckEnemy(void);

	bool IsEnemyProtectedByShield(edict_t* enemy);
	bool ParseChat(char* reply);
	bool RepliesToPlayer(void);
	bool CheckKeywords(char* tempMessage, char* reply);
	float GetBombTimeleft(void);
	float GetEstimatedReachTime(void);

	int GetCampAimingWaypoint(void);
	void FindPath(int srcIndex, int destIndex);
	void FindShortestPath(int srcIndex, int destIndex);
	void FindPathNavMesh(NavArea* start, NavArea* dest);
	void SecondThink(void);
	void CalculatePing(void);

public:
	entvars_t* pev;

	int m_wantedTeam; // player team bot wants select
	int m_wantedClass; // player model bot wants to select
	int m_difficulty; // bot difficulty
	int m_basePingLevel; // base ping level for randomizing

	int m_skill; // bots play skill
	int m_moneyAmount; // amount of money in bot's bank

	Personality m_personality;
	float m_spawnTime; // time this bot spawned
	float m_timeTeamOrder; // time of last radio command

	bool m_isVIP; // bot is vip?
	bool m_isBomber; // bot is bomber?
	bool m_bIsDefendingTeam; // bot in defending team on this map

	edict_t* m_avoid; // higher priority player we need to make way for

	int m_startAction; // team/class selection state
	int m_team; // bot's team
	int m_index; // bot's index
	bool m_isAlive; // has the player been killed or has he just respawned
	bool m_notStarted; // team/class not chosen yet
	bool m_isZombieBot; // checks bot if zombie

	float connect_time;
	float stay_time;

	int m_voteMap; // number of map to vote for
	int m_logotypeIndex; // index for logotype

	bool m_inBombZone; // bot in the bomb zone or not
	int m_buyState; // current Count in Buying
	float m_nextBuyTime; // next buy time

	bool m_inBuyZone; // bot currently in buy zone
	bool m_inVIPZone; // bot in the vip satefy zone
	bool m_buyingFinished; // done with buying
	bool m_hasDefuser; // does bot has defuser
	bool m_hasProgressBar; // has progress bar on a HUD
	bool m_jumpReady; // is double jump ready
	bool m_canChooseAimDirection; // can choose aiming direction

	float m_blindTime; // time when bot is blinded
	float m_blindMoveSpeed; // mad speeds when bot is blind
	float m_blindSidemoveSpeed; // mad side move speeds when bot is blind
	int m_blindButton; // buttons bot press, when blind
	int m_blindCampPoint; // blind action

	edict_t* m_doubleJumpEntity; // pointer to entity that request double jump
	edict_t* m_radioEntity; // pointer to entity issuing a radio command
	int m_radioOrder; // actual command

	float m_duckForJump; // is bot needed to duck for double jump
	float m_baseAgressionLevel; // base aggression level (on initializing)
	float m_baseFearLevel; // base fear level (on initializing)
	float m_agressionLevel; // dynamic aggression level (in game)
	float m_fearLevel; // dynamic fear level (in game)
	float m_nextEmotionUpdate; // next time to sanitize emotions

	int m_actMessageIndex; // current processed message
	int m_pushMessageIndex; // offset for next pushed message

	int m_prevGoalIndex; // holds destination goal waypoint
	int m_chosenGoalIndex; // used for experience, same as above

	Vector m_waypointOrigin; // origin of waypoint
	Vector m_destOrigin; // origin of move destination
	Vector m_bestOrigin; // origin of move destination
	Vector m_position; // position to move to in move to position task
	Vector m_doubleJumpOrigin; // origin of double jump
	Vector m_lastBombPosition; // origin of last remembered bomb position
	Vector m_goalaimposition; // goal aim position for tracking
	Vector m_campposition; // camping position

	float m_viewDistance; // current view distance
	float m_maxViewDistance; // maximum view distance
	Vector m_lastEnemyOrigin; // vector to last enemy origin
	SayText m_sayTextBuffer; // holds the index & the actual message of the last unprocessed text message of a player
	BurstMode m_weaponBurstMode; // bot using burst mode? (famas/glock18, but also silencer mode)

	int m_pingOffset[2]; // offset for faking pings
	int m_ping[3]; // bots pings in scoreboard
	float m_trackTime; // more realistic aim
	Vector m_tempAim; // A++
	Vector m_tempVel; // C

	edict_t* m_enemy; // pointer to enemy entity
	float m_enemyUpdateTime; // time to check for new enemies
	float m_enemyReachableTimer; // time to recheck if Enemy reachable
	bool m_isEnemyReachable; // direct line to enemy

	edict_t* m_moveTargetEntity; // target entity for move

	float m_seeEnemyTime; // time bot sees enemy
	float m_radiotimer; // a timer for radio call
	float m_randomattacktimer; // a timer for make bots random attack with knife like humans
	float m_itaimstart; // aim start time
	float m_slowthinktimer; // slow think timer
	float m_maxhearrange; // maximum range for hearing enemy
	float m_aimStopTime; // feel like playing on a phone
	float m_stayTime; // stay time (for simulate server)
	float m_connectTime; // for fake query

	int m_checkEnemyNum; // check enemy num idk
	int m_numFriendsLeft; // number of friends alive
	int m_numEnemiesLeft; // number of enemies alive

	bool m_isSlowThink; // bool for check is slow think? (every second)

	edict_t* m_lastEnemy; // pointer to last enemy entity
	edict_t* m_lastVictim; // pointer to killed entity
	edict_t* m_trackingEdict; // pointer to last tracked player when camping/hiding

	float m_timeNextTracking; // time waypoint index for tracking player is recalculated
	float m_firePause; // time to pause firing
	float m_shootTime; // time to shoot
	float m_timeLastFired; // time to last firing
	float m_weaponSelectDelay; // delay for reload

	int m_lastDamageType; // stores last damage
	int m_currentWeapon; // one current weapon for each bot
	int m_ammoInClip[Const_MaxWeapons]; // ammo in clip for each weapons
	int m_ammo[MAX_AMMO_SLOTS]; // total ammo amounts

	edict_t* m_allEnemy[checkEnemyNum];
	float m_allEnemyDistance[checkEnemyNum];

	edict_t* m_checkEnemy[checkEnemyNum];
	float m_checkEnemyDistance[checkEnemyNum];

	// NEW VARS
	Process m_currentProcess;
	Process m_rememberedProcess;
	float m_currentProcessTime;
	float m_rememberedProcessTime;

	bool m_hasEnemiesNear;
	bool m_hasFriendsNear;
	bool m_hasEntitiesNear;
	int m_enemiesNearCount;
	int m_friendsNearCount;
	int m_entitiesNearCount;
	float m_enemyDistance;
	float m_friendDistance;
	float m_entityDistance;
	float m_enemySeeTime;
	float m_friendSeeTime;
	float m_entitySeeTime;
	edict_t* m_nearestEnemy;
	edict_t* m_nearestFriend;
	edict_t* m_nearestEntity;
	Vector m_enemyOrigin;
	Vector m_friendOrigin;
	Vector m_entityOrigin;

	float m_senseChance;

	float m_searchTime;
	float m_pauseTime;

	int m_heuristic;
	bool m_2dH;

	Bot(edict_t* bot, int skill, int personality, int team, int member);
	~Bot(void);

	// NEW AI
	void BaseUpdate(void);
	void UpdateLooking(void);
	void UpdateProcess(void);
	void CheckSlowThink(void);

	Process GetProcess(void);
	bool SetProcess(const Process process, const char* debugNote = "clear", const bool rememberProcess = false, const float time = 999999.0f);
	void StartProcess(const Process process);
	void EndProcess(const Process process);
	void FinishCurrentProcess(const char* debugNote = "finished by the system");
	bool IsReadyForTheProcess(const Process process);
	char* GetProcessName(const Process process);

	char* GetHeuristicName(void);

	// FUNCTIONS
	void LookAtEnemies(void);
	void LookAtAround(void);
	void MoveTo(const Vector targetPosition);
	void MoveOut(const Vector targetPosition);
	void FollowPath(const Vector targetPosition);
	void FollowPath(const int targetIndex);
	void FindFriendsAndEnemiens(void);
	void FindEnemyEntities(void);

	bool IsEnemyViewable(edict_t* player);
	bool AllowPickupItem(void);
	bool FindItem(void);

	void CheckStuck(void);
	void ResetStuck(void);

	bool IsAttacking(const edict_t* player);

	// GOAP
	void DefaultStart(void);
	void AttackStart(void);
	void PlantStart(void);
	void DefuseStart(void);
	void EscapeStart(void);
	void CampStart(void);
	void PauseStart(void);
	void DestroyBreakableStart(void);
	void PickupStart(void);

	void DefaultUpdate(void);
	void AttackUpdate(void);
	void PlantUpdate(void);
	void DefuseUpdate(void);
	void EscapeUpdate(void);
	void CampUpdate(void);
	void PauseUpdate(void);
	void DestroyBreakableUpdate(void);
	void PickupUpdate(void);

	void DefaultEnd(void);
	void AttackEnd(void);
	void PlantEnd(void);
	void DefuseEnd(void);
	void EscapeEnd(void);
	void CampEnd(void);
	void PauseEnd(void);
	void DestroyBreakableEnd(void);
	void PickupEnd(void);

	bool DefaultReq(void);
	bool AttackReq(void);
	bool PlantReq(void);
	bool DefuseReq(void);
	bool EscapeReq(void);
	bool CampReq(void);
	bool PauseReq(void);
	bool DestroyBreakableReq(void);
	bool PickupReq(void);

	int GetAmmo(void);
	inline int GetAmmoInClip(void) { return m_ammoInClip[m_currentWeapon]; }

	inline edict_t* GetEntity(void) { return pev->pContainingEntity; };
	inline EOFFSET GetOffset(void) { return OFFSET(pev); };
	inline int GetIndex(void) { return ENTINDEX(GetEntity()); };

	inline Vector Center(void) { return (pev->absmax + pev->absmin) * 0.5f; };
	inline Vector EyePosition(void) { return pev->origin + pev->view_ofs; };

	void Think(void);
	void FacePosition(void);
	void UpdateAI(void);
	void NewRound(void);
	void EquipInBuyzone(int buyCount);
	void PushMessageQueue(int message);
	void PrepareChatMessage(char* text);
	int FindWaypoint(bool skipLag = true);
	bool EntityIsVisible(Vector dest, bool fromBody = false);

	void SetEnemy(edict_t* entity);
	void SetMoveTarget(edict_t* entity);
	void SetLastEnemy(edict_t* entity);

	void DeleteSearchNodes(void);
	Task* GetCurrentTask(void);

	void CheckTouchEntity(edict_t* entity);

	void RemoveCertainTask(BotTask taskID);
	void ResetTasks(void);
	void TakeDamage(edict_t* inflictor, int damage, int armor, int bits);
	void TakeBlinded(Vector fade, int alpha);
	void PushTask(BotTask taskID, float desire, int data, float time, bool canContinue, bool force = false);
	void DiscardWeaponForUser(edict_t* user, bool discardC4);

	void ChatSay(bool teamSay, const char* text, ...);

	void ChatMessage(int type, bool isTeamSay = false);
	void RadioMessage(int message);

	void Kill(void);
	void Kick(void);
	void ResetDoubleJumpState(void);
	void MoveToVector(Vector to);
	int FindLoosedBomb(void);

	int FindHostage(void);

	bool HasHostage(void);
	bool UsesRifle(void);
	bool UsesPistol(void);
	bool UsesSniper(void);
	bool UsesSubmachineGun(void);
	bool UsesZoomableRifle(void);
	bool UsesBadPrimary(void);
	bool HasPrimaryWeapon(void);
	bool HasShield(void);
	bool IsSniper(void);
	bool IsShieldDrawn(void);
};

// manager class
class BotControl : public Singleton <BotControl>
{
private:
	Array <CreateItem> m_creationTab; // bot creation tab
	float m_maintainTime; // time to maintain bot creation quota
	int m_lastWinner; // the team who won previous round
	int m_roundCount; // rounds passed
	bool m_economicsGood[2]; // is team able to buy anything

protected:
	int CreateBot(String name, int skill, int personality, int team, int member);

public:
	Bot* m_bots[32]; // all available bots
	

	Array <String> m_savedBotNames; // storing the bot names
	Array <String> m_avatars; // storing the steam ids

	BotControl(void);
	~BotControl(void);

	bool EconomicsValid(int team) { return m_economicsGood[team]; }

	int GetLastWinner(void) const { return m_lastWinner; }
	void SetLastWinner(int winner) { m_lastWinner = winner; }

	int GetIndex(edict_t* ent);
	int GetIndex(int index);
	Bot* GetBot(int index);
	Bot* GetBot(edict_t* ent);
	Bot* FindOneValidAliveBot(void);
	Bot* GetHighestSkillBot(int team);

	int GetBotsNum(void);
	int GetHumansNum(void);

	void Think(void);
	void DoJoinQuitStuff(void);
	void Free(void);
	void Free(int index);
	void CheckBotNum(void);

	void AddRandom(void) { AddBot("", -1, -1, -1, -1); }
	void AddBot(const String& name, int skill, int personality, int team, int member);
	void FillServer(int selection, int personality = PERSONALITY_NORMAL, int skill = -1, int numToAdd = -1);

	void RemoveAll(void);
	void RemoveRandom(void);
	void RemoveFromTeam(Team team, bool removeAll = false);
	void RemoveMenu(edict_t* ent, int selection);
	void KillAll(int team = -1);
	void MaintainBotQuota(void);
	void InitQuota(void);

	void ListBots(void);
	void SetWeaponMode(int selection);
	void CheckTeamEconomics(int team);

	int AddBotAPI(const String& name, int skill, int team);

	static void CallGameEntity(entvars_t* vars);
};

// netmessage handler class
class NetworkMsg : public Singleton <NetworkMsg>
{
private:
	Bot* m_bot;
	int m_state;
	int m_message;
	int m_registerdMessages[NETMSG_NUM];

public:
	NetworkMsg(void);
	~NetworkMsg(void) { };

	void Execute(void* p);
	void Reset(void) { m_message = NETMSG_UNDEFINED; m_state = 0; m_bot = nullptr; };
	void HandleMessageIfRequired(int messageType, int requiredType);

	void SetMessage(int message) { m_message = message; }
	void SetBot(Bot* bot) { m_bot = bot; }

	int GetId(int messageType) { return m_registerdMessages[messageType]; }
	void SetId(int messageType, int messsageIdentifier) { m_registerdMessages[messageType] = messsageIdentifier; }
};

// waypoint operation class
class Waypoint : public Singleton <Waypoint>
{
	friend class Bot;

private:
	Path* m_paths[Const_MaxWaypoints];
	bool m_badMapName;

	bool m_isOnLadder;
	bool m_waypointPaths;
	bool m_endJumpPoint;
	bool m_learnJumpWaypoint;
	float m_timeJumpStarted;

	float m_timeGetProTarGet;
	float m_timeCampWaypoint;
	bool m_ladderPoint;
	bool m_fallPoint;
	int m_lastFallWaypoint;

	Vector m_learnVelocity;
	Vector m_learnPosition;
	Vector m_foundBombOrigin;
	Vector m_fallPosition;

	int m_cacheWaypointIndex;
	int m_lastJumpWaypoint;
	int m_visibilityIndex;
	Vector m_lastWaypoint;
	uint8_t m_visLUT[Const_MaxWaypoints][Const_MaxWaypoints / 4];

	int m_lastDeclineWaypoint;

	float m_pathDisplayTime;
	float m_arrowDisplayTime;
	float m_waypointDisplayTime[Const_MaxWaypoints];
	float m_goalsScore[Const_MaxWaypoints];
	int m_findWPIndex;
	int m_facingAtIndex;
	char m_infoBuffer[256];

	int* m_distMatrix;

	Array <int> m_terrorPoints;
	Array <int> m_ctPoints;
	Array <int> m_goalPoints;
	Array <int> m_campPoints;
	Array <int> m_sniperPoints;
	Array <int> m_rescuePoints;
	Array <int> m_visitedGoals;
	Array <int> m_zmHmPoints;
	Array <int> m_hmMeshPoints;
	Array <int> m_otherPoints;

public:
	bool m_redoneVisibility;

	Waypoint(void);
	~Waypoint(void);

	void Initialize(void);

	void Analyze(void);
	void AnalyzeDeleteUselessWaypoints(void);
	void InitTypes();
	void AddPath(int addIndex, int pathIndex, float distance, int type = 0);

	int GetFacingIndex(void);
	int FindFarest(const Vector& origin, float maxDistance = 99999.0f);
	int FindNearestInCircle(const Vector& origin, float maxDistance = 99999.0f);
	int FindNearest(Vector origin, float minDistance = 99999.0f, int flags = -1, edict_t* entity = nullptr, int* findWaypointPoint = (int*)-2, int mode = -1);
	void FindInRadius(Vector origin, float radius, int* holdTab, int* count);
	void FindInRadius(Array <int>& queueID, float radius, Vector origin);

	void ChangeZBCampPoint(Vector origin);
	bool IsZBCampPoint(int pointID, bool checkMesh = true);

	void Add(int flags, Vector waypointOrigin = nullvec, bool autopath = true);
	void Delete(void);
	void DeleteByIndex(int index);
	void ToggleFlags(int toggleFlag);
	void SetRadius(int radius);
	bool IsConnected(int pointA, int pointB);
	bool IsConnected(int num);
	void InitializeVisibility(void);
	void CreatePath(char dir);
	void DeletePath(void);
	void DeletePathByIndex(int nodeFrom, int nodeTo);
	void CacheWaypoint(void);

	void DeleteFlags(void);

	void TeleportWaypoint(void);
	void SgdWp_Set(const char* modset);

	float GetTravelTime(const float maxSpeed, const Vector src, const Vector origin);
	bool IsVisible(int srcIndex, int destIndex);
	bool IsStandVisible(int srcIndex, int destIndex);
	bool IsDuckVisible(int srcIndex, int destIndex);
	void CalculateWayzone(int index);

	bool Download(void);
	bool Load(int mode = 0);
	void Save(void);
	void SaveOLD(void);

	bool Reachable(edict_t* entity, const int index);
	bool IsNodeReachable(const Vector src, const Vector destination);
	bool IsNodeReachableWithJump(const Vector src, const Vector destination, const int flags);
	void Think(void);
	void ShowWaypointMsg(void);
	bool NodesValid(void);
	void CreateBasic(void);
	void EraseFromHardDisk(void);

	float GetPathDistance(int srcIndex, int destIndex);

	Path* GetPath(int id);
	char* GetWaypointInfo(int id);
	char* GetInfo(void) { return m_infoBuffer; }

	int AddGoalScore(int index, int other[4]);
	void SetFindIndex(int index);
	void SetLearnJumpWaypoint(int mod = -1);
	void ClearGoalScore(void);

	bool IsGoalVisited(int index);
	void SetGoalVisited(int index);

	Vector GetBombPosition(void) { return m_foundBombOrigin; }
	void SetBombPosition(bool shouldReset = false);
	String CheckSubfolderFile(void);
	String CheckSubfolderFileOLD(void);
};

class NavMesh : public Singleton <NavMesh>
{
	friend class Bot;

private:
	NavArea* m_area[Const_MaxWaypoints];

	bool m_navmeshLoaded;

public:
	NavMesh(void);
	~NavMesh(void);

	void Initialize(void);
	String CheckSubfolderFile(void);

	bool IsConnected(int start, int goal);
	void SaveNav(void);
	bool LoadNav(void);
	void DrawNavArea(void);
	void Analyze(void);

	NavArea* CreateArea(const Vector origin);
	void CreateAreaAuto(const Vector start, const Vector goal);
	void DeleteArea(NavArea* area);
	void ConnectArea(NavArea* start, NavArea* end);
	void DisconnectArea(NavArea* start, const NavArea* end);
	void OptimizeNavMesh(void);

	NavArea* GetNavArea(int id);
	NavArea* GetNearestNavArea(const Vector origin);
	bool DoNavAreasIntersect(NavArea* area1, NavArea* area2, const float tolerance = 0.0f);
	bool DoPositionIntersect(const Vector origin, NavArea* area, const float tolerance = 0.0f);
	float GetNavAreaSize(const NavArea* area);

	Vector GetClosestPosition(NavArea* area, const Vector origin);
	Vector GetCenter(const NavArea* area);
	Vector GetCornerPosition(NavArea* area, int corner);
	Vector GetRandomPosition(NavArea* area);
	Vector GetAimingPosition(void);

	void ExpandNavArea(NavArea* area, const float radius = 50.0f);
};

#define g_netMsg NetworkMsg::GetObjectPtr()
#define g_botManager BotControl::GetObjectPtr()
#define g_waypoint Waypoint::GetObjectPtr()
#define g_navmesh NavMesh::GetObjectPtr()

// prototypes of bot functions...
extern int GetWeaponReturn(bool isString, const char* weaponAlias, int weaponID = -1);
extern int GetTeam(edict_t* ent);
extern int GetGameMode(void);
extern bool IsBreakable(edict_t* ent);
extern bool IsZombieEntity(edict_t* ent);

extern void SetGameMode(int gamemode);
extern bool IsZombieMode(void);
extern bool IsDeathmatchMode(void);
extern bool IsValidWaypoint(int index);
extern unsigned int GetPlayerPriority(edict_t* player);
extern edict_t* FindEntityInSphere(edict_t* startEnt, const Vector& vecCenter, const float flRadius);

extern int GetEntityWaypoint(edict_t* ent);
extern int SetEntityWaypoint(edict_t* ent, int mode = -1);

extern float GetShootingConeDeviation(edict_t* ent, const Vector* position);
extern float DotProduct(const Vector& a, const Vector& b);
extern float DotProduct2D(const Vector& a, const Vector& b);
extern Vector CrossProduct(const Vector& a, const Vector& b);

extern bool IsLinux(void);
extern bool TryFileOpen(char* fileName);
extern bool IsWalkableTraceLineClear(const Vector from, const Vector to);
extern bool IsDedicatedServer(void);
extern bool IsVisible(const Vector& origin, edict_t* ent);
extern Vector GetWalkablePosition(const Vector& origin, edict_t* ent = nullptr, bool returnNullVec = false, float height = 1000.0f);
extern bool IsAlive(const edict_t* ent);
extern bool IsInViewCone(const Vector& origin, edict_t* ent);
extern bool IsWeaponShootingThroughWall(int id);
extern bool IsValidBot(edict_t* ent);
extern bool IsValidBot(int index);
extern bool IsValidPlayer(edict_t* ent);
extern bool OpenConfig(const char* fileName, char* errorIfNotExists, File* outFile);
extern bool FindNearestPlayer(void** holder, edict_t* to, float searchDistance = 4096.0, bool sameTeam = false, bool needBot = false, bool needAlive = false, bool needDrawn = false);

extern const char* GetEntityName(edict_t* entity);

extern const char* GetMapName(void);
extern const char* GetWaypointDir(void);
extern const char* GetModName(void);
extern const char* GetField(const char* string, int fieldId, bool endLine = false);

extern Vector GetEntityOrigin(edict_t* ent);
extern Vector GetBoxOrigin(edict_t* ent);

extern Vector GetTopOrigin(edict_t* ent);
extern Vector GetBottomOrigin(edict_t* ent);
extern Vector GetPlayerHeadOrigin(edict_t* ent);

extern void FreeLibraryMemory(void);
extern void RoundInit(void);
extern void FakeClientCommand(edict_t* fakeClient, const char* format, ...);
extern void strtrim(char* string);
extern void CreatePath(char* path);
extern void ServerCommand(const char* format, ...);
extern void RegisterCommand(char* command, void funcPtr(void));
extern void CheckWelcomeMessage(void);
extern void DetectCSVersion(void);
extern void PlaySound(edict_t* ent, const char* soundName);
extern void ServerPrint(const char* format, ...);
extern void ChartPrint(const char* format, ...);
extern void ServerPrintNoTag(const char* format, ...);
extern void CenterPrint(const char* format, ...);
extern void ClientPrint(edict_t* ent, int dest, const char* format, ...);
extern void HudMessage(edict_t* ent, bool toCenter, const Color& rgb, char* format, ...);

extern void AutoLoadGameMode(void);

extern bool SetEntityAction(int index, int team, int action);
extern void SetEntityActionData(int i, int index = -1, int team = -1, int action = -1);

extern void AddLogEntry(int logLevel, const char* format, ...);

extern void MOD_AddLogEntry(int mode, char* format);

extern void DisplayMenuToClient(edict_t* ent, MenuText* menu);
extern void DecalTrace(entvars_t* pev, TraceResult* trace, int logotypeIndex);
extern void SoundAttachToThreat(edict_t* ent, const char* sample, float volume);

extern void TraceLine(const Vector& start, const Vector& end, bool ignoreMonsters, bool ignoreGlass, edict_t* ignoreEntity, TraceResult* ptr);
extern void TraceLine(const Vector& start, const Vector& end, bool ignoreMonsters, edict_t* ignoreEntity, TraceResult* ptr);
extern void TraceHull(const Vector& start, const Vector& end, bool ignoreMonsters, int hullNumber, edict_t* ignoreEntity, TraceResult* ptr);

inline bool IsNullString(const char* input)
{
	if (input == nullptr)
		return true;

	return *input == '\0';
}

// very global convars
extern ConVar ebot_knifemode;
extern ConVar ebot_gamemod;

#include <callbacks.h>
#include <globals.h>
#include <compress.h>
#include <resource.h>

#endif // EBOT_INCLUDED
