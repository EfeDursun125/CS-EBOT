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
// $Id:$
//

#ifndef GLOBALS_INCLUDED
#define GLOBALS_INCLUDED

// SyPB Pro P.31 - AMXX API
extern float API_Version;
extern float amxxDLL_Version;
extern uint16 amxxDLL_bV16[4];
// ****

// SyPB Pro P.42 - SwNPC
extern float SwNPC_Version;
extern uint16 SwNPC_Build[4];
// ****

extern bool g_bombPlanted;
extern bool g_bombSayString;
extern bool g_roundEnded;
extern bool g_waypointOn;
extern bool g_waypointsChanged;
extern bool g_autoWaypoint;
extern bool g_botsCanPause;
extern bool g_editNoclip;
extern bool g_isMetamod;
extern bool g_isFakeCommand;
extern bool g_leaderChoosen[2];
extern bool g_analyzewaypoints;
extern bool g_analyzeputrequirescrouch;
extern bool g_onlyvispath;

extern bool g_sgdWaypoint;
extern bool g_sautoWaypoint;
extern int m_sautoRadius;

extern float g_autoPathDistance;
extern float g_timeBombPlanted;
extern float g_timeNextBombUpdate;
extern float g_lastChatTime;
extern float g_timeRoundEnd;
extern float g_timeRoundMid;
extern float g_timeNextBombUpdate;
extern float g_timeRoundStart;
extern float g_lastRadioTime[2];

extern float g_DelayTimer;

extern int g_mapType;
extern int g_numWaypoints;
extern int g_gameVersion;
extern int g_fakeArgc;
extern unsigned short g_killHistory;

extern int g_normalWeaponPrefs[Const_NumWeapons];
extern int g_rusherWeaponPrefs[Const_NumWeapons];
extern int g_carefulWeaponPrefs[Const_NumWeapons];
extern int g_grenadeBuyPrecent[Const_NumWeapons - 23];
extern int g_grenadeBuyMoney[Const_NumWeapons - 23];
extern int g_radioSelect[32];
extern int g_lastRadio[2];
extern int g_storeAddbotVars[4];
extern int* g_weaponPrefs[];

extern int g_modelIndexLaser;
extern int g_modelIndexArrow;
extern char g_fakeArgv[256];

// SyPB Pro P.42 - Entity Action
extern const int entityNum = 256;
extern int g_entityId[entityNum];
extern int g_entityTeam[entityNum];
extern int g_entityAction[entityNum];
extern int g_entityWpIndex[entityNum];
extern Vector g_entityGetWpOrigin[entityNum];
extern float g_entityGetWpTime[entityNum];

extern Array <Array <String> > g_chatFactory;
extern Array <Array <ChatterItem> > g_chatterFactory;
extern Array <NameItem> g_botNames;
extern Array <KwChat> g_replyFactory;

extern FireDelay g_fireDelay[Const_NumWeapons + 1];
extern WeaponSelect g_weaponSelect[Const_NumWeapons + 1];
extern WeaponProperty g_weaponDefs[Const_MaxWeapons + 1];

extern Client_old g_clients[32];
extern MenuText g_menus[26];
extern SkillDef g_skillTab[6];
extern Task g_taskFilters[];

extern edict_t* g_hostEntity;
extern edict_t* g_worldEdict;
extern Library* g_gameLib;

extern DLL_FUNCTIONS g_functionTable;
extern EntityAPI_t g_entityAPI;
extern FuncPointers_t g_funcPointers;
extern NewEntityAPI_t g_getNewEntityAPI;
extern BlendAPI_t g_serverBlendingAPI;

#endif // GLOBALS_INCLUDED
