#ifndef GLOBALS_INCLUDED
#define GLOBALS_INCLUDED

extern bool g_bombPlanted;
extern bool g_bombDefusing;
extern bool g_bombSayString;
extern bool g_roundEnded;
extern bool g_waypointOn;
extern bool g_waypointsChanged;
extern bool g_autoWaypoint;
extern bool g_editNoclip;
extern bool g_isFakeCommand;
extern bool g_analyzewaypoints;
extern bool g_analyzeputrequirescrouch;
extern bool g_expanded[Const_MaxWaypoints];
extern bool g_hasDoors;
extern bool g_messageEnded;

extern float g_autoPathDistance;
extern float g_timeBombPlanted;
extern float g_lastChatTime;
extern float g_timeRoundEnd;
extern float g_timeRoundMid;
extern float g_timeRoundStart;
extern float g_lastRadioTime[2];
extern float g_fakePingUpdate;
extern float g_randomJoinTime;
extern float g_DelayTimer;

extern int g_lastMessageID;
extern int g_numBytesWritten;
extern int g_mapType;
extern int g_numWaypoints;
extern int g_gameVersion;
extern int g_fakeArgc;
extern int g_radioSelect[32];
extern int g_lastRadio[2];
extern int g_storeAddbotVars[4];
extern int g_modelIndexLaser;
extern int g_modelIndexArrow;
extern char g_fakeArgv[256];
const int entityNum = 256;
extern int g_entityId[entityNum];
extern int g_entityTeam[entityNum];
extern int g_entityAction[entityNum];

extern Array <Array <String>> g_chatFactory;
extern Array <NameItem> g_botNames;
extern Array <KwChat> g_replyFactory;

extern FireDelay g_fireDelay[Const_NumWeapons + 1];
extern WeaponSelect g_weaponSelect[Const_NumWeapons + 1];
extern WeaponSelect g_weaponSelectHL[Const_NumWeaponsHL + 1];
extern WeaponProperty g_weaponDefs[Const_MaxWeapons + 1];

extern Clients g_clients[32];
extern MenuText g_menus[28];

extern edict_t* g_hostEntity;
extern edict_t* g_worldEdict;

extern bool IsCStrike(void);

#endif // GLOBALS_INCLUDED