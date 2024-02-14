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
extern bool g_analyzenavmesh;
extern bool g_analyzeputrequirescrouch;
extern bool g_hasDoors;
extern bool g_navmeshOn;

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
extern float g_pathTimer;
extern float g_audioTime;

extern int g_mapType;
extern int g_numWaypoints;
extern uint16_t g_numNavAreas;
extern int g_gameVersion;
extern int g_fakeArgc;
extern int g_radioSelect[32];
extern int g_lastRadio[2];
extern int g_storeAddbotVars[4];
extern int g_modelIndexLaser;
extern int g_modelIndexArrow;
extern char g_fakeArgv[256];
#define entityNum 254
extern int g_entityId[entityNum];
extern int g_entityTeam[entityNum];
extern int g_entityAction[entityNum];

extern Array <Array<String>> g_chatFactory;
extern MiniArray <NameItem> g_botNames;
extern Array <KwChat> g_replyFactory;

extern FireDelay g_fireDelay[Const_NumWeapons + 1];
extern WeaponSelect g_weaponSelect[Const_NumWeapons + 1];
extern WeaponSelect g_weaponSelectHL[Const_NumWeaponsHL + 1];
extern WeaponProperty g_weaponDefs[Const_MaxWeapons + 1];

extern Clients g_clients[32];
extern MenuText g_menus[28];

extern edict_t* g_hostEntity;
extern edict_t* g_worldEdict;

#endif // GLOBALS_INCLUDED