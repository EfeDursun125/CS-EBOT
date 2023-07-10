#ifndef GLOBALS_INCLUDED
#define GLOBALS_INCLUDED

extern float m_randomJoinTime;

extern bool g_bombPlanted;
extern bool g_bombDefusing;
extern bool g_bombSayString;
extern bool g_roundEnded;
extern bool g_waypointOn;
extern bool g_navmeshOn;
extern bool g_waypointsChanged;
extern bool g_autoWaypoint;
extern bool g_editNoclip;
extern bool g_isMetamod;
extern bool g_isFakeCommand;
extern bool g_analyzewaypoints;
extern bool g_analyzenavmesh;
extern bool g_analyzeputrequirescrouch;
extern bool g_expanded[Const_MaxWaypoints];
extern bool g_isUsingNAV;
extern bool g_hasDoors;

extern bool g_sgdWaypoint;
extern bool g_sautoWaypoint;
extern int m_sautoRadius;

extern float g_autoPathDistance;
extern float g_timeBombPlanted;
extern float g_lastChatTime;
extern float g_timeRoundEnd;
extern float g_timeRoundMid;
extern float g_timeRoundStart;
extern float g_lastRadioTime[2];
extern float g_fakePingUpdate;

extern float g_DelayTimer;

extern int g_mapType;
extern int g_numWaypoints;
extern int g_numNavAreas;
extern int g_gameVersion;
extern int g_fakeArgc;
extern int bot_conntimes;

extern int g_normalWeaponPrefs[Const_NumWeapons];
extern int g_rusherWeaponPrefs[Const_NumWeapons];
extern int g_carefulWeaponPrefs[Const_NumWeapons];
extern int g_radioSelect[32];
extern int g_lastRadio[2];
extern int g_storeAddbotVars[4];
extern int* g_weaponPrefs[];

extern int g_modelIndexLaser;
extern int g_modelIndexArrow;
extern char g_fakeArgv[256];

const int entityNum = 256;
extern int g_entityId[entityNum];
extern int g_entityTeam[entityNum];
extern int g_entityAction[entityNum];
extern int g_entityWpIndex[entityNum];
extern Vector g_entityGetWpOrigin[entityNum];
extern float g_entityGetWpTime[entityNum];

extern Array <Array <String> > g_chatFactory;
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
extern Library* g_gameLib;

extern DLL_FUNCTIONS g_functionTable;
extern EntityAPI_t g_entityAPI;
extern FuncPointers_t g_funcPointers;
extern NewEntityAPI_t g_getNewEntityAPI;
extern BlendAPI_t g_serverBlendingAPI;

#endif // GLOBALS_INCLUDED
