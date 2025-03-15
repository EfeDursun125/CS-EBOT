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

#include "../include/core.h"

bool g_roundEnded = true;
bool g_editNoclip = false;
bool g_waypointOn = false;
bool g_waypointsChanged = true;
bool g_autoWaypoint = false;
bool g_bLearnJumpWaypoint = false;
bool g_analyzewaypoints = false;
bool g_analyzeputrequirescrouch = false;
bool g_hasDoors = false;
bool g_isFakeCommand = false;
bool g_isMatrixReady = false;

float g_autoPathDistance = 250.0f;
float g_fakePingUpdate = 0.0f;
float g_DelayTimer = 0.0f;
float g_pathTimer = 0.0f;
float g_fakeCommandTimer = 0.0f;

int g_storeAddbotVars[4];
int g_fakeArgc = 0;
int g_gameVersion = Game::CStrike;
int16_t g_numWaypoints = 0;

int g_modelIndexLaser = 0;
int g_modelIndexArrow = 0;
char g_fakeArgv[256];

// Entity Data
int g_entityId[entityNum];
int g_entityTeam[entityNum];
int g_entityAction[entityNum];
//******

meta_globals_t* gpMetaGlobals = nullptr;
gamedll_funcs_t* gpGamedllFuncs = nullptr;
mutil_funcs_t* gpMetaUtilFuncs = nullptr;

enginefuncs_t g_engfuncs;
WeaponProperty g_weaponDefs[Const_MaxWeapons + 1];
Clients g_clients[32];
CArray<NameItem>g_botNames;

edict_t* g_worldEdict = nullptr;
edict_t* g_hostEntity = nullptr;
edict_t* g_helicopter = nullptr;
globalvars_t* g_pGlobals = nullptr;

// metamod engine & dllapi function tables
metamod_funcs_t gMetaFunctionTable =
{
   nullptr, // pfnEntityAPI_t ()
   nullptr, // pfnEntityAPI_t_Post ()
   GetEntityAPI2, // pfnEntityAPI_t2 ()
   GetEntityAPI2_Post, // pfnEntityAPI_t2_Post ()
   nullptr, // pfnGetNewDLLFunctions ()
   nullptr, // pfnGetNewDLLFunctions_Post ()
   GetEngineFunctions, // pfnGetEngineFunctions ()
   nullptr, // pfnGetEngineFunctions_Post ()
};

// metamod plugin information
plugin_info_t Plugin_info =
{
   META_INTERFACE_VERSION, // interface version
   PRODUCT_NAME, // plugin name
   PRODUCT_VERSION, // plugin version
   PRODUCT_DATE, // date of creation
   PRODUCT_AUTHOR, // plugin author
   PRODUCT_URL, // plugin URL
   PRODUCT_LOGTAG, // plugin logtag
   PT_STARTUP, // when loadable
   PT_NEVER, // when unloadable
};

WeaponSelect g_weaponSelect[Const_NumWeapons + 1] =
{
	{Weapon::Knife, "weapon_knife", true},
	{Weapon::Usp, "weapon_usp", false},
	{Weapon::Glock18, "weapon_glock18", false},
	{Weapon::Deagle, "weapon_deagle", false},
	{Weapon::P228, "weapon_p228", false},
	{Weapon::Elite, "weapon_elite", false},
	{Weapon::FiveSeven, "weapon_fiveseven",false},
	{Weapon::M3, "weapon_m3", false},
	{Weapon::Xm1014, "weapon_xm1014", false},
	{Weapon::Mp5, "weapon_mp5navy", true},
	{Weapon::Tmp, "weapon_tmp", true},
	{Weapon::P90, "weapon_p90", true},
	{Weapon::Mac10, "weapon_mac10", true},
	{Weapon::Ump45, "weapon_ump45", true},
	{Weapon::Ak47, "weapon_ak47", true},
	{Weapon::Sg552, "weapon_sg552", true},
	{Weapon::M4A1, "weapon_m4a1", true},
	{Weapon::Galil, "weapon_galil", true},
	{Weapon::Famas, "weapon_famas", true},
	{Weapon::Aug, "weapon_aug", true},
	{Weapon::Scout, "weapon_scout", false},
	{Weapon::Awp, "weapon_awp", false},
	{Weapon::G3SG1, "weapon_g3sg1", false},
	{Weapon::Sg550, "weapon_sg550", false},
	{Weapon::M249, "weapon_m249", true}
};

// weapon firing delay based on skill (min and max delay for each weapon)
FireDelay g_fireDelay[Const_NumWeapons + 1] =
{
   {Weapon::Knife,    255, 256, 0.10f, {0.0f, 0.2f, 0.3f, 0.4f, 0.6f, 0.8f}, {0.1f, 0.3f, 0.5f, 0.7f, 1.0f, 1.2f}, 0.0f, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},
   {Weapon::Usp,      3,   853, 0.15f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Glock18,  5,   853, 0.15f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Deagle,   2,   640, 0.20f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::P228,     4,   853, 0.14f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Elite,    3,   640, 0.20f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::FiveSeven,     4,   731, 0.14f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::M3,       8,   365, 0.86f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Xm1014,   7,   512, 0.15f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Mp5,      4,   731, 0.10f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Tmp,      3,   731, 0.05f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::P90,      4,   731, 0.10f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Mac10,    3,   731, 0.06f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Ump45,    4,   731, 0.15f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Ak47,     2,   512, 0.09f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Sg552,    3,   512, 0.11f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::M4A1,     3,   512, 0.08f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Galil,    4,   512, 0.09f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Famas,    4,   512, 0.10f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Aug,      3,   512, 0.11f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Scout,    10,  256, 0.18f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Awp,      10,  170, 0.22f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::G3SG1,    4,   256, 0.25f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::Sg550,    4,   256, 0.25f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}},
   {Weapon::M249,     3,   640, 0.10f, {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f}, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.7f}, 0.2f, {0.0f, 0.0f, 0.1f, 0.1f, 0.2f}, {0.1f, 0.1f, 0.2f, 0.2f, 0.4f}}
};

// bot menus
MenuText g_menus[28] =
{
	// main menu
	{
		0x2ff,
		"\\yE-Bot Main Menu\\w\v\v"
		"1. E-Bot Control\v"
		"2. Features\v\v"
		"3. Fill Server\v"
		"4. End Round\v\v"
		"0. Exit"
	},

	// bot features menu
	{
		0x25f,
		"\\yE-Bot Features\\w\v\v"
		"1. Weapon Mode Menu\v"
		"2. Waypoint Menu\v"
		"3. Select Personality\v\v"
		"4. Toggle Debug Mode\v"
		"5. Command Menu\v\v"
		"0. Exit"
	},

	// bot control menu
	{
		0x2ff,
		"\\yE-Bot Control Menu\\w\v\v"
		"1. Add a E-Bot, Quick\v"
		"2. Add a E-Bot, Specified\v\v"
		"3. Remove Random Bot\v"
		"4. Remove All Bots\v\v"
		"5. Remove Bot Menu\v\v"
		"0. Exit"
	},

	// weapon mode select menu
	{
		0x27f,
		"\\yE-Bot Weapon Mode\\w\v\v"
		"1. Knives only\v"
		"2. Pistols only\v"
		"3. Shotguns only\v"
		"4. Machine Guns only\v"
		"5. Rifles only\v"
		"6. Sniper Weapons only\v"
		"7. All Weapons\v\v"
		"0. Exit"
	},

	// personality select menu
	{
		0x20f,
		"\\yE-Bot Personality\\w\v\v"
		"1. Random\v"
		"2. Normal\v"
		"3. Rusher\v"
		"4. Careful\v\v"
		"0. Exit"
	},

	// skill select menu
	{
		0x23f,
		"\\yE-Bot Skill Level\\w\v\v"
		"1. Poor (0-20)\v"
		"2. Easy (20-40)\v"
		"3. Normal (40-60)\v"
		"4. Hard (60-80)\v"
		"5. Expert (80-99)\v"
		"6. Legendary (100)\v\v"
		"0. Exit"
	},

	// team select menu
	{
		0x213,
		"\\ySelect a team\\w\v\v"
		"1. Terrorist Force\v"
		"2. Counter-Terrorist Force\v\v"
		"5. Auto-select\v\v"
		"0. Exit"
	},

	// terrorist model select menu
	{
		0x21f,
		"\\ySelect an appearance\\w\v\v"
		"1. Phoenix Connexion\v"
		"2. L337 Krew\v"
		"3. Arctic Avengers\v"
		"4. Guerilla Warfare\v\v"
		"5. Auto-select\v\v"
		"0. Exit"
	},

	// counter-terrirust model select menu
	{
		0x21f,
		"\\ySelect an appearance\\w\v\v"
		"1. Seal Team 6 (DEVGRU)\v"
		"2. German GSG-9\v"
		"3. UK SAS\v"
		"4. French GIGN\v\v"
		"5. Auto-select\v\v"
		"0. Exit"
	},

	// main waypoint menu
	{
		0x3ff,
		"\\yWaypoint Operations (Page 1/2)\\w\v\v"
		"1. Show/Hide waypoints\v"
		"2. Cache waypoint\v"
		"3. Create path\v"
		"4. Delete path\v"
		"5. Add waypoint\v"
		"6. Delete waypoint\v"
		"7. Set Autopath Distance\v"
		"8. Set Radius\v\v"
		"9. Next...\v\v"
		"0. Exit"
	},

	// main waypoint menu (page 2)
	{
		0x3ff,
		"\\yWaypoint Operations (Page 2/2)\\w\v\v"
		"1. Waypoint stats\v"
		"2. Autowaypoint on/off\v"
		"3. Set flags\v"
		"4. Save waypoints\v"
		"5. Save without checking\v"
		"6. Load waypoints\v"
		"7. Check waypoints\v"
		"8. Noclip cheat on/off\v\v"
		"9. Previous...\v\v"
		"0. Exit"
	},

	// select waypoint radius menu
	{
		0x3ff,
		"\\yWaypoint Radius\\w\v\v"
		"1. SetRadius 0\v"
		"2. SetRadius 8\v"
		"3. SetRadius 16\v"
		"4. SetRadius 32\v"
		"5. SetRadius 48\v"
		"6. SetRadius 64\v"
		"7. SetRadius 80\v"
		"8. SetRadius 96\v"
		"9. SetRadius 128\v\v"
		"0. Exit"
	},

	// waypoint add menu
	{
		0x3ff,
		"\\yWaypoint Type\\w\v\v"
		"1. Normal\v"
		"\\r2. Terrorist Important\v"
		"3. Counter-Terrorist Important\v"
		"\\w4. Avoid\v"
		"\\y5. Rescue Zone\v"
		"\\w6. Camping\v"
		"7. Helicopter\v"
		"\\r8. Map Goal\v"
		"\\w9. Jump\v\v"
		"0. Exit"
	},

	// set waypoint flag menu
	{
		0x3ff,
		"\\yToggle Waypoint Flags (Page 1/3)\\w\v\v"
		"1. Fall Check\v"
		"2. Terrorists Specific\v"
		"3. CTs Specific\v"
		"4. Use Elevator\v"
		"5. Helicopter\v"
		"6. Zombie Mode Camp\v"
		"7. \\yDelete All Flags!\\ \v\v"
		"8. Previous...\v"
		"9. Next...\v\v"
		"0. Exit"
	},

	// kickmenu #1
	{
		0x0,
		nullptr,
	},

	// kickmenu #2
	{
		0x0,
		nullptr,
	},

	// kickmenu #3
	{
		0x0,
		nullptr,
	},

	// kickmenu #4
	{
		0x0,
		nullptr,
	},

	// command menu
	{
		0x23f,
		"\\yE-Bot Command Menu\\w\v\v"
		"1. Make Double Jump\v"
		"2. Finish Double Jump\v\v"
		"3. Drop the C4 Bomb\v"
		"4. Drop the Weapon\v\v"
		"0. Exit"
	},

	// auto-path max distance
	{
		0x27f,
		"\\yAutoPath Distance\\w\v\v"
		"1. Distance 0\v"
		"2. Distance 100\v"
		"3. Distance 130\v"
		"4. Distance 160\v"
		"5. Distance 190\v"
		"6. Distance 220\v"
		"7. Distance 250 (Default)\v\v"
		"0. Exit"
	},

	// path connections
	{
		//0x207,
		0x3ff,
		"\\yCreate Path (Choose Direction)\\w\v\v"
		"1. Outgoing Path\v"
		"2. Incoming Path\v"
		"3. Bidirectional (Both Ways)\v"
		"4. Jump Path\v"
		"5. Zombie Boosting Path\v\v"
		"6. Delete Path\v"
		"\v0. Exit"
	},

	{
		0x3ff,
		"\\ySgdWP Menu\\w\v\v"
		"1. Add Waypoint\v"
		"2. Set Waypoint Flag\v"
		"3. Create Path\v"
		"4. Set Waypoint Radius\v"
		"5. Teleport to Waypoint\v"
		"6. Delete Waypoint\v\v"
		"7. Auto Put Waypoint Mode\v"
		"9. Save Waypoint\v"
		"\v0. Exit"
	},

	{
		0x3ff,
		"\\ySgdWP Add Waypoint Menu\\w\v\v"
		"1. Normal\v"
		"2. Terrorist Important\v"
		"3. Counter-Terrorist Important\v"
		"4. Avoid\v"
		"5. Rescue Zone\v"
		"6. Map Goal\v"
		"7. Camp\v"
		"8. Jump\v"
		"9. Next\v"
		"\v0. Exit"
	},

	{
		0x3ff,
		"\\ySgdWP Add Waypoint Menu2\\w\v\v"
		"1. Use Elevator\v"
		"2. Sniper Camp\v"
		"3. Zomibe Mode Hm Camp\v"
		"\v0. Exit"
	},

	{
		0x3ff,
		"\\ySgdWP Waypoint Team\\w\v\v"
		"1. Terrorist\v"
		"2. Counter-Terrorist\v"
		"3. All\v"
		"\v0. Exit"
	},

	{
		0x3ff,
		"\\ySgdWP Save \\w\v"
		"Your waypoint have a problem\v"
		"Are You Sure Save it?\v"
		"1. Yes, Save\v"
		"2. No, I will change it"
	},
	
	// set waypoint flag menu
	{
		0x3ff,
		"\\yToggle Waypoint Flags (Page 2/3)\\w\v\v"
		"1. Use Button\v"
		"2. Human Camp Mesh\v"
		"3. Zombie Only\v"
		"4. Human Only\v"
		"5. Zombie Push\v"
		"6. Fall Risk\v"
		"7. Specific Gravity (1 = 800)\v\v"
		"8. Previous...\v"
		"9. Next...\v\v"
		"0. Exit"
	},

	// set waypoint flag menu
	{
		0x3ff,
		"\\yToggle Waypoint Flags (Page 3/3)\\w\v\v"
		"1. Crouch\v"
		"2. Only One Bot\v"
		"3. Wait Until Ground\v"
		"4. Avoid\v"
		"\v"
		"\v"
		"\v\v"
		"8. Previous...\v"
		"9. Next...\v\v"
		"0. Exit"
	}
};
