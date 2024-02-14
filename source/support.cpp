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

#include <core.h>

//
// TODO:
// clean up the code.
// create classes: Tracer, PrintManager, GameManager
//

ConVar ebot_ignore_enemies("ebot_ignore_enemies", "0");
ConVar ebot_zp_delay_custom("ebot_zp_delay_custom", "0.0");
ConVar ebot_auto_gamemode("ebot_auto_gamemode", "1");

void TraceLine(const Vector& start, const Vector& end, bool ignoreMonsters, bool ignoreGlass, edict_t* ignoreEntity, TraceResult* ptr)
{
	// this function traces a line dot by dot, starting from vecStart in the direction of vecEnd,
	// ignoring or not monsters (depending on the value of IGNORE_MONSTERS, true or false), and stops
	// at the first obstacle encountered, returning the results of the trace in the TraceResult structure
	// ptr. Such results are (amongst others) the distance traced, the hit surface, the hit plane
	// vector normal, etc. See the TraceResult structure for details. This function allows to specify
	// whether the trace starts "inside" an entity's polygonal model, and if so, to specify that entity
	// in ignoreEntity in order to ignore it as a possible obstacle.
	// this is an overloaded prototype to add IGNORE_GLASS in the same way as IGNORE_MONSTERS work.

	(*g_engfuncs.pfnTraceLine) (start, end, (ignoreMonsters ? 1 : 0) | (ignoreGlass ? 0x100 : 0), ignoreEntity, ptr);
}

void TraceLine(const Vector& start, const Vector& end, bool ignoreMonsters, edict_t* ignoreEntity, TraceResult* ptr)
{
	// this function traces a line dot by dot, starting from vecStart in the direction of vecEnd,
	// ignoring or not monsters (depending on the value of IGNORE_MONSTERS, true or false), and stops
	// at the first obstacle encountered, returning the results of the trace in the TraceResult structure
	// ptr. Such results are (amongst others) the distance traced, the hit surface, the hit plane
	// vector normal, etc. See the TraceResult structure for details. This function allows to specify
	// whether the trace starts "inside" an entity's polygonal model, and if so, to specify that entity
	// in ignoreEntity in order to ignore it as a possible obstacle.

	(*g_engfuncs.pfnTraceLine) (start, end, ignoreMonsters ? 1 : 0, ignoreEntity, ptr);
}

void TraceHull(const Vector& start, const Vector& end, bool ignoreMonsters, int hullNumber, edict_t* ignoreEntity, TraceResult* ptr)
{
	// this function traces a hull dot by dot, starting from vecStart in the direction of vecEnd,
	// ignoring or not monsters (depending on the value of IGNORE_MONSTERS, true or
	// false), and stops at the first obstacle encountered, returning the results
	// of the trace in the TraceResult structure ptr, just like TraceLine. Hulls that can be traced
	// (by parameter hull_type) are point_hull (a line), head_hull (size of a crouching player),
	// human_hull (a normal body size) and large_hull (for monsters?). Not all the hulls in the
	// game can be traced here, this function is just useful to give a relative idea of spatial
	// reachability (i.e. can a hostage pass through that tiny hole ?) Also like TraceLine, this
	// function allows to specify whether the trace starts "inside" an entity's polygonal model,
	// and if so, to specify that entity in ignoreEntity in order to ignore it as an obstacle.

	(*g_engfuncs.pfnTraceHull) (start, end, ignoreMonsters ? 1 : 0, hullNumber, ignoreEntity, ptr);
}

uint16_t FixedUnsigned16(const float value, const float scale)
{
	return static_cast<uint16_t>(cclamp(static_cast<int>(value * scale), 0, 65535));
}

int16_t FixedSigned16(const float value, const float scale)
{
	return static_cast<int16_t>(cclamp(static_cast<int>(value * scale), -32768, 32767));
}

bool IsAlive(const edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	return (ent->v.deadflag == DEAD_NO) && (ent->v.health > 0.0f) && (ent->v.movetype != MOVETYPE_NOCLIP);
}

float GetShootingConeDeviation(edict_t* ent, const Vector position)
{
	if (FNullEnt(ent))
		return 0.0f;

	const Vector dir = (position - (GetEntityOrigin(ent) + ent->v.view_ofs)).Normalize();
	MakeVectors(ent->v.v_angle);

	// he's facing it, he meant it
	return g_pGlobals->v_forward | dir;
}

bool IsInViewCone(const Vector& origin, edict_t* ent)
{
	if (FNullEnt(ent))
		return true;

	MakeVectors(ent->v.v_angle);

	if (((origin - (GetEntityOrigin(ent) + ent->v.view_ofs)).Normalize() | g_pGlobals->v_forward) >= ccosf(((ent->v.fov > 0 ? ent->v.fov : 91.0f) * 0.51f) * 0.0174532925f))
		return true;

	return false;
}

bool IsVisible(const Vector& origin, edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	TraceResult tr{};
	TraceLine(GetEntityOrigin(ent), origin, true, true, ent, &tr);

	if (tr.flFraction != 1.0f)
		return false; // line of sight is not established

	return true; // line of sight is valid.
}

// return walkable position on ground
Vector GetWalkablePosition(const Vector& origin, edict_t* ent, const bool returnNullVec, const float height)
{
	TraceResult tr{};
	TraceLine(origin, Vector(origin.x, origin.y, -height), true, false, ent, &tr);

	if (tr.flFraction != 1.0f)
		return tr.vecEndPos; // walkable ground

	if (returnNullVec)
		return nullvec; // return nullvector for check if we can't hit the ground
	else
		return origin; // return original origin, we cant hit to ground
}

// this expanded function returns the vector origin of a bounded entity, assuming that any
// entity that has a bounding box has its center at the center of the bounding box itself.
Vector GetEntityOrigin(edict_t* ent)
{
	if (FNullEnt(ent))
		return nullvec;

	Vector entityOrigin = ent->v.origin;
	if (entityOrigin == nullvec)
		entityOrigin = ent->v.absmin + (ent->v.size * 0.5);

	return entityOrigin;
}

Vector GetBoxOrigin(edict_t* ent)
{
	if (FNullEnt(ent))
		return nullvec;

	return ent->v.absmin + (ent->v.size * 0.5);
}

Vector GetTopOrigin(edict_t* ent)
{
	if (FNullEnt(ent))
		return nullvec;

	const Vector origin = GetEntityOrigin(ent);
	Vector topOrigin = origin + ent->v.maxs;
	if (topOrigin.z < origin.z)
		topOrigin = origin + ent->v.mins;

	topOrigin.x = origin.x;
	topOrigin.y = origin.y;
	return topOrigin;
}

Vector GetBottomOrigin(edict_t* ent)
{
	if (FNullEnt(ent))
		return nullvec;

	const Vector origin = GetEntityOrigin(ent);
	Vector bottomOrigin = origin + ent->v.mins;
	if (bottomOrigin.z > origin.z)
		bottomOrigin = origin + ent->v.maxs;

	bottomOrigin.x = origin.x;
	bottomOrigin.y = origin.y;
	return bottomOrigin;
}

Vector GetPlayerHeadOrigin(edict_t* ent)
{
	if (FNullEnt(ent))
		return nullvec;

	Vector headOrigin = GetTopOrigin(ent);

	if (!(ent->v.flags & FL_DUCKING))
	{
		const Vector origin = GetEntityOrigin(ent);
		float hbDistance = headOrigin.z - origin.z;
		hbDistance *= 0.4f;
		headOrigin.z -= hbDistance;
	}
	else
		headOrigin.z -= 1.0f;

	return headOrigin;
}

void DisplayMenuToClient(edict_t* ent, MenuText* menu)
{
	if (!IsValidPlayer(ent) || IsValidBot(ent))
		return;

	const int clientIndex = ENTINDEX(ent) - 1;
	if (menu)
	{
		String tempText = String(menu->menuText);
		tempText.Replace("\v", "\n");

		char* text = tempText;
		tempText = String(text);

		// make menu looks best
		int i;
		for (i = 0; i <= 9; i++)
			tempText.Replace(FormatBuffer("%d.", i), FormatBuffer("\\r%d.\\w", i));

		text = tempText;

		while (cstrlen(text) >= 64)
		{
			MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, g_netMsg->GetId(NETMSG_SHOWMENU), nullptr, ent);
			WRITE_SHORT(menu->validSlots);
			WRITE_CHAR(-1);
			WRITE_BYTE(1);

			for (i = 0; i <= 63; i++)
				WRITE_CHAR(text[i]);

			MESSAGE_END();

			text += 64;
		}

		MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, g_netMsg->GetId(NETMSG_SHOWMENU), nullptr, ent);
		WRITE_SHORT(menu->validSlots);
		WRITE_CHAR(-1);
		WRITE_BYTE(0);
		WRITE_STRING(text);
		MESSAGE_END();

		g_clients[clientIndex].menu = menu;
	}
	else
	{
		MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, g_netMsg->GetId(NETMSG_SHOWMENU), nullptr, ent);
		WRITE_SHORT(0);
		WRITE_CHAR(0);
		WRITE_BYTE(0);
		WRITE_STRING("");
		MESSAGE_END();

		g_clients[clientIndex].menu = nullptr;
	}

	CLIENT_COMMAND(ent, "speak \"player/geiger1\"\n"); // Stops others from hearing menu sounds..
}

// this function free's all allocated memory
void FreeLibraryMemory(void)
{
	g_botManager->Free();
	g_waypoint->Initialize(); // frees waypoint data
}

bool SetEntityAction(const int index, const int team, const int action)
{
	if (index == -1)
	{
		int i;
		for (i = 0; i < entityNum; i++)
			SetEntityActionData(i);

		return 1;
	}

	edict_t* entity = INDEXENT(index);
	if (!IsAlive(entity))
		return -1;

	if (IsValidPlayer(entity))
		return -1;

	int i;
	for (i = 0; i < entityNum; i++)
	{
		if (g_entityId[i] == index)
		{
			if (action != -1)
			{
				if (team != g_entityTeam[i] || action != g_entityAction[i])
					SetEntityActionData(i, index, team, action);
			}
			else
				SetEntityActionData(i);

			return 1;
		}
	}

	if (action == -1)
		return -1;

	for (i = 0; i < entityNum; i++)
	{
		if (g_entityId[i] == -1)
		{
			SetEntityActionData(i, index, team, action);
			return 1;
		}
	}

	return -1;
}

void SetEntityActionData(const int i, const int index, const int team, const int action)
{
	g_entityId[i] = index;
	g_entityTeam[i] = team;
	g_entityAction[i] = action;
}

void FakeClientCommand(edict_t* fakeClient, const char* format, ...)
{
	// the purpose of this function is to provide fakeclients (bots) with the same client
	// command-scripting advantages (putting multiple commands in one line between semicolons)
	// as real players. It is an improved version of botman's FakeClientCommand, in which you
	// supply directly the whole string as if you were typing it in the bot's "console". It
	// is supposed to work exactly like the pfnClientCommand (server-sided client command).

	if (!IsValidBot(fakeClient))
		return;

	va_list ap;
	char command[256];
	int stop, i, stringIndex = 0;

	va_start(ap, format);
	vsnprintf(command, sizeof(command), format, ap);
	va_end(ap);

	if (IsNullString(command))
		return;

	g_isFakeCommand = true;
	const int length = cstrlen(command);
	int start, index;

	while (stringIndex < length)
	{
		start = stringIndex;
		while (stringIndex < length && command[stringIndex] != ';')
			stringIndex++;

		if (command[stringIndex - 1] == '\n')
			stop = stringIndex - 2;
		else
			stop = stringIndex - 1;

		for (i = start; i <= stop; i++)
			g_fakeArgv[i - start] = command[i];

		g_fakeArgv[i - start] = 0;
		stringIndex++;
		index = 0;
		g_fakeArgc = 0;

		while (index < i - start)
		{
			while (index < i - start && g_fakeArgv[index] == ' ')
				index++;

			if (g_fakeArgv[index] == '"')
			{
				index++;
				while (index < i - start && g_fakeArgv[index] != '"')
					index++;

				index++;
			}
			else
			{
				while (index < i - start && g_fakeArgv[index] != ' ')
					index++;
			}

			g_fakeArgc++;
		}

		MDLL_ClientCommand(fakeClient);
	}

	g_fakeArgv[0] = 0;
	g_isFakeCommand = false;
	g_fakeArgc = 0;
}

char* GetField(const char* string, const int fieldId, const bool endLine)
{
	// This function gets and returns a particuliar field in a string where several szFields are
	// concatenated. Fields can be words, or groups of words between quotes ; separators may be
	// white space or tabs. A purpose of this function is to provide bots with the same Cmd_Argv
	// convenience the engine provides to real clients. This way the handling of real client
	// commands and bot client commands is exactly the same, just have a look in engine.cpp
	// for the hooking of pfnCmd_Argc, pfnCmd_Args and pfnCmd_Argv, which redirects the call
	// either to the actual engine functions (when the caller is a real client), either on
	// our function here, which does the same thing, when the caller is a bot.

	char field[256];

	// reset the string
	cmemset(field, 0, sizeof(field));

	int length, i, index = 0, fieldCount = 0, start, stop;

	field[0] = 0; // reset field
	length = cstrlen(string); // get length of string

	// while we have not reached end of line
	while (index < length && fieldCount <= fieldId)
	{
		while (index < length && (string[index] == ' ' || string[index] == '\t'))
			index++; // ignore spaces or tabs

		// is this field multi-word between quotes or single word ?
		if (string[index] == '"')
		{
			index++; // move one step further to bypass the quote
			start = index; // save field start position

			while ((index < length) && (string[index] != '"'))
				index++; // reach end of field

			stop = index - 1; // save field stop position
			index++; // move one step further to bypass the quote
		}
		else
		{
			start = index; // save field start position

			while (index < length && (string[index] != ' ' && string[index] != '\t'))
				index++; // reach end of field

			stop = index - 1; // save field stop position
		}

		// is this field we just processed the wanted one ?
		if (fieldCount == fieldId)
		{
			for (i = start; i <= stop; i++)
				field[i - start] = string[i]; // store the field value in a string

			field[i - start] = 0; // terminate the string
			break; // and stop parsing
		}

		fieldCount++; // we have parsed one field more
	}

	if (endLine)
		field[cstrlen(field) - 1] = 0;

	cstrtrim(field);
	return field; // returns the wanted field
}

char* GetModName(void)
{
	static char modName[256]{};

	// ask the engine for the MOD directory path
	// get the length of the returned string
	GET_GAME_DIR(modName);
	int length = cstrlen(modName);

	// format the returned string to get the last directory name
	int stop = length - 1;
	while ((modName[stop] == '\\' || modName[stop] == '/') && stop > 0)
		stop--; // shift back any trailing separator

	int start = stop;
	while (modName[start] != '\\' && modName[start] != '/' && start > 0)
		start--; // shift back to the start of the last subdirectory name

	if (modName[start] == '\\' || modName[start] == '/')
		start++; // if we reached a separator, step over it

	// now copy the formatted string back onto itself character per character
	for (length = start; length <= stop; length++)
		modName[length - start] = modName[length];

	modName[length - start] = 0; // terminate the string

	return &modName[0];
}

// Create a directory tree
void CreatePath(char* path)
{
	char* ofs;
	for (ofs = path + 1; *ofs; ofs++)
	{
		if (*ofs == '/')
		{
			*ofs = 0;
#ifdef PLATFORM_WIN32
			mkdir(path);
#else
			mkdir(path, 0777);
#endif
			*ofs = '/';
		}
	}

#ifdef PLATFORM_WIN32
	mkdir(path);
#else
	mkdir(path, 0777);
#endif
}

// this is called at the start of each round
void RoundInit(void)
{
	g_roundEnded = false;

	for (const auto& bot : g_botManager->m_bots)
	{
		if (!bot)
			continue;

		bot->NewRound();
		g_radioSelect[bot->m_index - 1] = 0;
	}

	g_bombSayString = false;
	g_timeBombPlanted = 0.0f;

	g_lastRadioTime[0] = 0.0f;
	g_lastRadioTime[1] = 0.0f;

	g_waypoint->SetBombPosition(true);

	if (!(g_gameVersion & Game::HalfLife))
	{
		// auto semiclip detection
		cvar_t* sc = g_engfuncs.pfnCVarGetPointer("semiclip");
		if (sc && sc->value == 1.0f)
		{
			extern ConVar ebot_has_semiclip;
			ebot_has_semiclip.SetInt(1);
		}

		if (ebot_auto_gamemode.GetBool())
			AutoLoadGameMode();

		if (GetGameMode() == GameMode::Original)
		{
			// check team economics
			g_botManager->CheckTeamEconomics(Team::Terrorist);
			g_botManager->CheckTeamEconomics(Team::Counter);
		}

		// calculate the round mid/end in world time
		g_timeRoundStart = engine->GetTime() + engine->GetFreezeTime() + g_pGlobals->frametime;
		g_timeRoundMid = g_timeRoundStart + engine->GetRoundTime() * 30.0f;
		g_timeRoundEnd = g_timeRoundStart + engine->GetRoundTime() * 60.0f;

		g_botManager->SelectLeaderEachTeam(Team::Counter);
		g_botManager->SelectLeaderEachTeam(Team::Counter);
	}
	else
	{
		g_timeRoundStart = engine->GetTime();
		g_timeRoundMid = g_timeRoundStart + 999999.0f;
		g_timeRoundEnd = g_timeRoundStart + 999999.0f;
		SetGameMode(GameMode::Deathmatch);
	}
}

void AutoLoadGameMode(void)
{
	const char* getModeName = GetModName();
	static int8_t checkShowTextTime = 0;
	checkShowTextTime++;

	// CS:BTE Support 
	char* Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/bte_player.ini", getModeName);
	if (TryFileOpen(Plugin_INI) || TryFileOpen(FormatBuffer("%s/addons/amxmodx/configs/bte_config/bte_blockresource.txt", getModeName)))
	{
		const int Const_GameModes = 13;
		GameMode bteGameModAi[Const_GameModes] =
		{
			GameMode::Original,		//1
			GameMode::TeamDeathmatch,		//2
			GameMode::Deathmatch,		//3
			GameMode::NoTeam,	//4
			GameMode::TeamDeathmatch,		//5
			GameMode::ZombiePlague,		//6
			GameMode::ZombiePlague,		//7
			GameMode::ZombiePlague,		//8
			GameMode::ZombiePlague,		//9
			GameMode::ZombieHell,		//10
			GameMode::ZombiePlague,		//11
			GameMode::NoTeam,	//12
			GameMode::ZombiePlague			//13
		};

		char* bteGameINI[Const_GameModes] =
		{
			"plugins-none", //1
			"plugins-td",   //2
			"plugins-dm",   //3
			"plugins-dr",   //4
			"plugins-gd",   //5
			"plugins-ghost",//6
			"plugins-zb1",  //7
			"plugins-zb3",  //8
			"plugins-zb4",  //9 
			"plugins-ze",   //10
			"plugins-zse",  //11
			"plugins-npc",  //12
			"plugins-zb5"   //13
		};

		int i;
		for (i = 0; i < Const_GameModes; i++)
		{
			if (TryFileOpen(FormatBuffer("%s/addons/amxmodx/configs/%s.ini", getModeName, bteGameINI[i])))
			{
				if (bteGameModAi[i] == GameMode(2) && i != 5)
					g_DelayTimer = engine->GetTime() + 20.0f + CVAR_GET_FLOAT("mp_freezetime");

				if (checkShowTextTime < 3 || GetGameMode() != bteGameModAi[i])
					ServerPrint("*** E-BOT Auto Game Mode Setting: CS:BTE [%s] [%d] ***", bteGameINI[i], bteGameModAi[i]);

				if (i == 3 || i == 9)
				{
					ServerPrint("***** E-BOT not support the mode now :( *****");
					SetGameMode(GameMode::TeamDeathmatch);
				}
				else
					SetGameMode(bteGameModAi[i]);

				// Only ZM3 need restart the round
				if (checkShowTextTime < 3 && i == 7)
					ServerCommand("sv_restart 1");

				break;
			}
		}

		return;
	}

	if (ebot_zp_delay_custom.GetFloat() > 0.0f)
	{
		SetGameMode(GameMode::ZombiePlague);
		extern ConVar ebot_prefer_better_pickup;
		ebot_prefer_better_pickup.SetInt(0);
		g_DelayTimer = engine->GetTime() + ebot_zp_delay_custom.GetFloat() + 2.2f;

		// zombie escape
		if (g_mapType == MAP_ZE)
		{
			extern ConVar ebot_escape;
			ebot_escape.SetInt(1);
			ServerPrint("*** E-BOT Detected Zombie Escape Map: ebot_zombie_escape_mode is set to 1 ***");
		}
		else
			g_mapType = MAP_DE;

		return;
	}
	else
	{
		cvar_t* zpCvar = g_engfuncs.pfnCVarGetPointer("zp_delay");
		if (!zpCvar)
			zpCvar = g_engfuncs.pfnCVarGetPointer("zp_gamemode_delay");

		if (zpCvar)
		{
			if (checkShowTextTime < 3 || GetGameMode() != GameMode::ZombiePlague)
				ServerPrint("*** E-BOT Auto Game Mode Setting: Zombie Mode (Plague/Escape) ***");

			SetGameMode(GameMode::ZombiePlague);
			extern ConVar ebot_prefer_better_pickup;
			ebot_prefer_better_pickup.SetInt(0);

			// zombie escape
			if (g_mapType == MAP_ZE)
			{
				extern ConVar ebot_escape;
				ebot_escape.SetInt(1);
				ServerPrint("*** E-BOT Detected Zombie Escape Map: ebot_zombie_escape_mode is set to 1 ***");
			}
			else
				g_mapType = MAP_DE;

			const float delayTime = zpCvar->value + 2.2f;
			if (delayTime > 0.0f)
				g_DelayTimer = engine->GetTime() + delayTime;

			zpCvar = g_engfuncs.pfnCVarGetPointer("zp_lightning");
			if (zpCvar)
			{
				if (zpCvar->string[0] == 'a')
				{
					extern ConVar ebot_force_flashlight;
					ebot_force_flashlight.SetInt(1);
					// TODO: add visiblity option for human bots
				}
				else if (zpCvar->string[0] == 'b')
				{
					extern ConVar ebot_force_flashlight;
					ebot_force_flashlight.SetInt(1);
				}
			}

			return;
		}
	}

	// Base Builder
	const cvar_t* bbCvar = g_engfuncs.pfnCVarGetPointer("bb_buildtime");
	const cvar_t* bbCvar2 = g_engfuncs.pfnCVarGetPointer("bb_preptime");
	if (bbCvar && bbCvar2)
	{
		if (checkShowTextTime < 3 || GetGameMode() != GameMode::ZombiePlague)
			ServerPrint("*** E-BOT Auto Game Mode Setting: Zombie Mode (Base Builder) ***");

		SetGameMode(GameMode::ZombiePlague);
		extern ConVar ebot_prefer_better_pickup;
		ebot_prefer_better_pickup.SetInt(0);

		const float delayTime = bbCvar->value + bbCvar2->value + 2.2f;
		if (delayTime > 0.0f)
			g_DelayTimer = engine->GetTime() + delayTime;

		g_mapType = MAP_DE;
		return;
	}

	// DM:KD
	Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/plugins-dmkd.ini", GetModName());
	if (TryFileOpen(Plugin_INI))
	{
		if (CVAR_GET_FLOAT("DMKD_DMMODE") == 1)
		{
			if (checkShowTextTime < 3 || GetGameMode() != GameMode::Deathmatch)
				ServerPrint("*** E-BOT Auto Game Mode Setting: DM:KD-DM ***");

			SetGameMode(GameMode::Deathmatch);
		}
		else
		{
			if (checkShowTextTime < 3 || GetGameMode() != GameMode::TeamDeathmatch)
				ServerPrint("*** E-BOT Auto Game Mode Setting: DM:KD-TDM ***");

			SetGameMode(GameMode::TeamDeathmatch);
		}

		g_mapType = MAP_DE;
		return;
	}

	// Zombie Hell
	const cvar_t* zhCvar = g_engfuncs.pfnCVarGetPointer("zh_zombie_maxslots");
	if (zhCvar && zhCvar->value > 0.0f)
	{
		if (checkShowTextTime < 3 || GetGameMode() != GameMode::ZombieHell)
			ServerPrint("*** E-BOT Auto Game Mode Setting: Zombie Hell ***");

		SetGameMode(GameMode::ZombieHell);
		extern ConVar ebot_prefer_better_pickup;
		ebot_prefer_better_pickup.SetInt(0);

		extern ConVar ebot_quota;
		ebot_quota.SetInt(static_cast<int>(zhCvar->value));
		g_mapType = MAP_DE;
		return;
	}

	const cvar_t* bhCvar = g_engfuncs.pfnCVarGetPointer("bh_starttime");
	if (bhCvar)
	{
		if (checkShowTextTime < 3 || GetGameMode() != GameMode::ZombiePlague)
			ServerPrint("*** E-BOT Auto Game Mode Setting: Zombie Mode (Biohazard) ***");

		SetGameMode(GameMode::ZombiePlague);
		extern ConVar ebot_prefer_better_pickup;
		ebot_prefer_better_pickup.SetInt(0);

		const float delayTime = bhCvar->value + 1.0f;
		if (delayTime > 0.0f)
			g_DelayTimer = engine->GetTime() + delayTime;

		g_mapType = MAP_DE;
		return;
	}

	const cvar_t* dmActive = g_engfuncs.pfnCVarGetPointer("csdm_active");
	if (dmActive && dmActive->value > 0.0f)
	{
		const cvar_t* freeForAll = g_engfuncs.pfnCVarGetPointer("mp_freeforall");
		if (freeForAll && freeForAll->value > 0.0f)
		{
			if (checkShowTextTime < 3 || GetGameMode() != GameMode::Deathmatch)
				ServerPrint("*** E-BOT Auto Game Mode Setting: CSDM-DM ***");

			SetGameMode(GameMode::Deathmatch);
			g_mapType = MAP_DE;
			return;
		}
		else
		{
			if (checkShowTextTime < 3 || GetGameMode() != GameMode::TeamDeathmatch)
				ServerPrint("*** E-BOT Auto Game Mode Setting: CSDM-TDM ***");

			SetGameMode(GameMode::TeamDeathmatch);
			g_mapType = MAP_DE;
			return;
		}
	}

	if (checkShowTextTime < 3)
	{
		if (GetGameMode() == GameMode::Original)
			ServerPrint("*** E-BOT Auto Game Mode Setting: Original ***");
		else
			ServerPrint("*** E-BOT Auto Game Mode Setting: Unknown ***");
	}
}

void SetGameMode(const GameMode gamemode)
{
	ebot_gamemod.SetInt(static_cast<int>(gamemode));
}

bool IsZombieMode(void)
{
	const GameMode gamemode = GetGameMode();
	return (gamemode == GameMode::ZombiePlague || gamemode == GameMode::ZombieHell);
}

bool IsDeathmatchMode(void)
{
	const GameMode gamemode = GetGameMode();
	return (gamemode == GameMode::Deathmatch || gamemode == GameMode::TeamDeathmatch);
}

#ifndef WIN32
template <typename T>
bool IsValidWaypoint(const T index)
{
	if (index < 0 || index >= static_cast<T>(g_numWaypoints))
		return false;

	return true;
}
#endif

GameMode GetGameMode(void)
{
	return static_cast<GameMode>(ebot_gamemod.GetInt());
}

bool IsBreakable(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	if ((FClassnameIs(ent, "func_breakable") || (FClassnameIs(ent, "func_pushable") && (ent->v.spawnflags & SF_PUSH_BREAKABLE)) || FClassnameIs(ent, "func_wall")))
	{
		if (ent->v.takedamage != DAMAGE_NO && ent->v.impulse <= 0 && !(ent->v.flags & FL_WORLDBRUSH) && !(ent->v.spawnflags & SF_BREAK_TRIGGER_ONLY))
			return (ent->v.movetype == MOVETYPE_PUSH || ent->v.movetype == MOVETYPE_PUSHSTEP);
	}

	return false;
}

// new get team off set, return player true team
int GetTeam(edict_t* ent)
{
	if (FNullEnt(ent))
		return Team::Count;

	int player_team = Team::Count;
	if (!IsValidPlayer(ent))
	{
		int i;
		for (i = 0; i < entityNum; i++)
		{
			if (g_entityId[i] == -1)
				continue;

			if (ent == INDEXENT(g_entityId[i]))
			{
				player_team = g_entityTeam[i];
				break;
			}
		}

		return player_team;
	}

	if (ebot_ignore_enemies.GetBool())
		player_team = Team::Counter;
	else if (GetGameMode() == GameMode::ZombiePlague)
	{
		if (g_DelayTimer > engine->GetTime())
			player_team = Team::Counter;
		else if (g_roundEnded)
			player_team = Team::Terrorist;
		else
			player_team = *((int*)ent->pvPrivateData + OFFSET_TEAM) - 1;
	}
	else if (GetGameMode() == GameMode::Deathmatch || GetGameMode() == GameMode::NoTeam)
	{
		int client = ENTINDEX(ent);
		player_team = client * client;
	}
	else
		player_team = *((int*)ent->pvPrivateData + OFFSET_TEAM) - 1;

	return player_team;
}

bool IsZombieEntity(edict_t* ent)
{
	if (IsZombieMode()) // Zombie Mode
		return GetTeam(ent) == Team::Terrorist;

	return false;
}

bool IsValidPlayer(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	if (ent->v.flags & (FL_CLIENT | FL_FAKECLIENT))
		return true;

	return false;
}

bool IsValidBot(edict_t* ent)
{
	if (g_botManager->GetIndex(ent) != -1)
		return true;

	return false;
}

bool IsValidBot(const int index)
{
	if (g_botManager->GetIndex(index) != -1)
		return true;

	return false;
}

bool IsEntityWalkable(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	if (FClassnameIs(ent, "func_illusionary"))
		return false;

	if (FClassnameIs(ent, "func_door") || FClassnameIs(ent, "func_door_rotating"))
		return true;

	if (FClassnameIs(ent, "func_breakable") && ent->v.takedamage != DAMAGE_NO)
		return true;

	return false;
}

bool IsWalkableLineClear(const Vector from, const Vector to)
{
	TraceResult result{};
	edict_t* pEntIgnore = g_hostEntity;
	Vector useFrom = from, dir;

	while (true)
	{
		TraceHull(useFrom, to, true, point_hull, pEntIgnore, &result);

		// if we hit a walkable entity, try again
		if ((result.flFraction != 1.0f || result.fAllSolid || result.fStartSolid) && IsEntityWalkable(result.pHit))
		{
			pEntIgnore = result.pHit;

			// start from just beyond where we hit to avoid infinite loops
			dir = to - from;
			dir.Normalize();
			useFrom = result.vecEndPos + 5.0f * dir;
		}
		else
			break;
	}

	if (result.fStartSolid)
		return false;

	if (result.fAllSolid)
		return false;

	if (result.vecPlaneNormal.z > 0.7f)
		return false;

	if (result.flFraction == 1.0f)
		return true;

	return false;
}

// return true if server is dedicated server, false otherwise
bool IsDedicatedServer(void)
{
	return (IS_DEDICATED_SERVER() > 0); // ask to engine for this
}

// this function tests if a file exists by attempting to open it
bool TryFileOpen(char* fileName)
{
	File fp;

	// check if got valid handle
	if (fp.Open(fileName, "rb"))
	{
		fp.Close();
		return true;
	}

	return false;
}

void HudMessage(edict_t* ent, const bool toCenter, const Color& rgb, char* format, ...)
{
	if (!IsValidPlayer(ent) || IsValidBot(ent))
		return;
	
	va_list ap;
	char buffer[1024];

	va_start(ap, format);
	vsprintf(buffer, format, ap);
	va_end(ap);

	MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, nullptr, ent);
	WRITE_BYTE(TE_TEXTMESSAGE);
	WRITE_BYTE(1);
	WRITE_SHORT(FixedSigned16(-1.0f, (1 << 13)));
	WRITE_SHORT(FixedSigned16(toCenter ? -1.0f : 0.0f, (1 << 13)));
	WRITE_BYTE(2);
	WRITE_BYTE(static_cast<int>(rgb.red));
	WRITE_BYTE(static_cast<int>(rgb.green));
	WRITE_BYTE(static_cast<int>(rgb.blue));
	WRITE_BYTE(0);
	WRITE_BYTE(crandomint(230, 255));
	WRITE_BYTE(crandomint(230, 255));
	WRITE_BYTE(crandomint(230, 255));
	WRITE_BYTE(200);
	WRITE_SHORT(FixedUnsigned16(0.0078125f, (1 << 8)));
	WRITE_SHORT(FixedUnsigned16(2.0f, (1 << 8)));
	WRITE_SHORT(FixedUnsigned16(6.0f, (1 << 8)));
	WRITE_SHORT(FixedUnsigned16(0.1f, (1 << 8)));
	WRITE_STRING(const_cast<const char*>(&buffer[0]));
	MESSAGE_END();
}

void ServerPrint(const char* format, ...)
{
	va_list ap;
	char string[3072];

	va_start(ap, format);
	vsprintf(string, format, ap);
	va_end(ap);

	SERVER_PRINT(FormatBuffer("[%s] %s\n", PRODUCT_LOGTAG, string));
}

void ServerPrintNoTag(const char* format, ...)
{
	va_list ap;
	char string[3072];

	va_start(ap, format);
	vsprintf(string, format, ap);
	va_end(ap);

	SERVER_PRINT(FormatBuffer("%s\n", string));
}

void CenterPrint(const char* format, ...)
{
	va_list ap;
	char string[2048];

	va_start(ap, format);
	vsprintf(string, format, ap);
	va_end(ap);

	if (IsDedicatedServer())
	{
		ServerPrint(string);
		return;
	}

	MESSAGE_BEGIN(MSG_BROADCAST, g_netMsg->GetId(NETMSG_TEXTMSG));
	WRITE_BYTE(HUD_PRINTCENTER);
	WRITE_STRING(FormatBuffer("%s\n", string));
	MESSAGE_END();
}

void ChatPrint(const char* format, ...)
{
	va_list ap;
	char string[2048];

	va_start(ap, format);
	vsprintf(string, format, ap);
	va_end(ap);

	if (IsDedicatedServer())
	{
		ServerPrint(string);
		return;
	}

	cstrcat(string, "\n");

	MESSAGE_BEGIN(MSG_BROADCAST, g_netMsg->GetId(NETMSG_TEXTMSG));
	WRITE_BYTE(HUD_PRINTTALK);
	WRITE_STRING(string);
	MESSAGE_END();
}

void ClientPrint(edict_t* ent, int dest, const char* format, ...)
{
	if (!IsValidPlayer(ent) || IsValidBot(ent))
		return;

	va_list ap;
	char string[2048];

	va_start(ap, format);
	vsprintf(string, format, ap);
	va_end(ap);

	if (FNullEnt(ent) || ent == g_hostEntity)
	{
		if (dest & 0x3ff)
			ServerPrint(string);
		else
			ServerPrintNoTag(string);

		return;
	}

	cstrcat(string, "\n");

	if (dest & 0x3ff)
		(*g_engfuncs.pfnClientPrintf) (ent, static_cast<PRINT_TYPE>(dest &= ~0x3ff), FormatBuffer("[E-BOT] %s", string));
	else
		(*g_engfuncs.pfnClientPrintf) (ent, static_cast<PRINT_TYPE>(dest), string);
}

// this function returns true if server is running under linux, and false otherwise returns windows
bool IsLinux(void)
{
#ifdef PLATFORM_WIN32
	return false;
#else
	return true;
#endif
}

// this function asks the engine to execute a server command
void ServerCommand(const char* format, ...)
{
	va_list ap;
	char string[1024];

	// concatenate all the arguments in one string
	va_start(ap, format);
	vsprintf(string, format, ap);
	va_end(ap);

	SERVER_COMMAND(FormatBuffer("%s\n", string)); // execute command
}

char* GetEntityName(edict_t* entity)
{
	static char entityName[256]{};
	if (FNullEnt(entity))
		cstrcpy(entityName, "nullptr");
	else if (IsValidPlayer(entity))
		cstrncpy(entityName, STRING(entity->v.netname), sizeof(entityName));
	else
		cstrncpy(entityName, STRING(entity->v.classname), sizeof(entityName));

	return &entityName[0];
}

// this function gets the map name and store it in the map_name global string variable.
char* GetMapName(void)
{
	static char mapName[256]{};
	cstrncpy(mapName, STRING(g_pGlobals->mapname), sizeof(mapName));
	return &mapName[0]; // and return a pointer to it
}

bool OpenConfig(const char* fileName, char* errorIfNotExists, File* outFile)
{
	if (outFile->IsValid())
		outFile->Close();

	outFile->Open(FormatBuffer("%s/addons/ebot/%s", GetModName(), fileName), "rt");

	if (!outFile->IsValid())
	{
		AddLogEntry(Log::Error, errorIfNotExists);
		return false;
	}

	return true;
}

char* GetWaypointDir(void)
{
	return FormatBuffer("%s/addons/ebot/waypoints/", GetModName());
}

// this function tells the engine that a new server command is being declared, in addition
// to the standard ones, whose name is command_name. The engine is thus supposed to be aware
// that for every "command_name" server command it receives, it should call the function
// pointed to by "function" in order to handle it.
void RegisterCommand(char* command, void funcPtr(void))
{
	if (IsNullString(command) || !funcPtr)
		return; // reliability check

	REG_SVR_COMMAND(command, funcPtr); // ask the engine to register this new command
}

void CheckWelcomeMessage(void)
{
	static float receiveTime = -1.0f;

	if (receiveTime == -1.0f && IsAlive(g_hostEntity))
		receiveTime = engine->GetTime() + 12.5f;

	if (receiveTime > 0.0f && receiveTime < engine->GetTime())
	{
		const int buildVersion[4] = { PRODUCT_VERSION_DWORD };
		const int bV16[4] = { buildVersion[0], buildVersion[1], buildVersion[2], buildVersion[3] };

		ChatPrint("----- [%s %s] by %s -----", PRODUCT_NAME, PRODUCT_VERSION, PRODUCT_AUTHOR);
		ChatPrint("***** Build: (%u.%u.%u.%u) *****", bV16[0], bV16[1], bV16[2], bV16[3]);

		receiveTime = 0.0f;
	}
}

void DetectCSVersion(void)
{
	const char* const infoBuffer = "Game Registered: %s (0x%d)";

	// switch version returned by dll loader
	if (g_gameVersion & Game::Xash)
		ServerPrint(infoBuffer, "Xash Engine", sizeof(Bot));
	else if (g_gameVersion & Game::CZero)
		ServerPrint(infoBuffer, "CS: CZ", sizeof(Bot));
	else if (g_gameVersion & Game::HalfLife)
		ServerPrint(infoBuffer, "Half-Life", sizeof(Bot));
	else
		ServerPrint(infoBuffer, "CS 1.6", sizeof(Bot));

	engine->GetGameConVarsPointers(); // !!! TODO !!!
}

void PlaySound(edict_t* ent, const char* name)
{
	if (FNullEnt(ent))
		return;

	// TODO: make this obsolete
	EMIT_SOUND_DYN2(ent, CHAN_WEAPON, name, 1.0, ATTN_NORM, 0, 100);
}

// this function logs a message to the message log file root directory.
void AddLogEntry(Log logLevel, const char* format, ...)
{
	va_list ap;
	char buffer[512] = { 0, }, levelString[32] = { 0, }, logLine[1024] = { 0, };

	va_start(ap, format);
	vsprintf(buffer, format, ap);
	va_end(ap);

	switch (logLevel)
	{
	case Log::Default:
	{
		cstrncpy(levelString, "Log: ", sizeof(levelString));
		break;
	}
	case Log::Warning:
	{
		cstrncpy(levelString, "Warning: ", sizeof(levelString));
		break;
	}
	case Log::Error:
	{
		cstrncpy(levelString, "Error: ", sizeof(levelString));
		break;
	}
	case Log::Fatal:
	{
		cstrncpy(levelString, "Critical: ", sizeof(levelString));
		break;
	}
	case Log::Memory:
	{
		cstrncpy(levelString, "Memory Error: ", sizeof(levelString));
		ServerPrint("unexpected memory error");
		break;
	}
	}

	sprintf(logLine, "%s%s", levelString, buffer);
	MOD_AddLogEntry(-1, logLine);
}

void MOD_AddLogEntry(int mod, char* format)
{
	char modName[32], logLine[1024] = { 0, }, buildVersionName[64];
	uint16 mod_bV16[4];

	if (mod == -1)
	{
		sprintf(modName, "E-BOT");
		const int buildVersion[4] = {PRODUCT_VERSION_DWORD};
		int i;
		for (i = 0; i < 4; i++)
			mod_bV16[i] = (uint16)buildVersion[i];
	}

	ServerPrintNoTag("[%s Log] %s", modName, format);

	sprintf(buildVersionName, "%s_build_%u_%u_%u_%u.txt", modName,
		mod_bV16[0], mod_bV16[1], mod_bV16[2], mod_bV16[3]);

	// if logs folder deleted, it will result a crash... so create it again before writing logs...
	CreatePath(FormatBuffer("%s/addons/ebot/logs", GetModName()));

	File checkLogFP(FormatBuffer("%s/addons/ebot/logs/%s", GetModName(), buildVersionName), "rb");
	File fp(FormatBuffer("%s/addons/ebot/logs/%s", GetModName(), buildVersionName), "at");

	if (!checkLogFP.IsValid())
	{
		fp.Printf("---------- %s Log \n", modName);
		fp.Printf("---------- %s Version: %u.%u  \n", modName, mod_bV16[0], mod_bV16[1]);
		fp.Printf("---------- %s Build: %u.%u.%u.%u  \n", modName, mod_bV16[0], mod_bV16[1], mod_bV16[2], mod_bV16[3]);
		fp.Printf("----------------------------- \n\n");
	}

	checkLogFP.Close();

	if (!fp.IsValid())
		return;

	time_t tickTime = time(&tickTime);
	tm* time = localtime(&tickTime);

	sprintf(logLine, "[%02d:%02d:%02d] %s", time->tm_hour, time->tm_min, time->tm_sec, format);
	fp.Printf("%s\n", logLine);

	if (mod != -1)
		fp.Printf("E-BOT Build: %d \n", PRODUCT_VERSION);

	fp.Printf("----------------------------- \n");
	fp.Close();
}

// this function finds nearest to to, player with set of parameters, like his
// team, live status, search distance etc. if needBot is true, then pvHolder, will
// be filled with bot pointer, else with edict pointer(!).
bool FindNearestPlayer(void** pvHolder, edict_t* to, const float searchDistance, const bool sameTeam, const bool needBot, const bool isAlive, const bool needDrawn)
{
	edict_t* survive = nullptr; // pointer to temporaly & survive entity
	float nearestPlayer = FLT_MAX; // nearest player

	const Vector toOrigin = GetEntityOrigin(to);
	float distance;

	for (const auto& client : g_clients)
	{
		if (FNullEnt(client.ent))
			continue;

		if (client.ent == to)
			continue;

		if ((sameTeam && client.team != GetTeam(to)) || (isAlive && !IsAlive(client.ent)) || (needBot && !IsValidBot(client.index)) || (needDrawn && (client.ent->v.effects & EF_NODRAW)))
			continue; // filter players with parameters

		distance = (client.ent->v.origin - toOrigin).GetLengthSquared();
		if (distance < nearestPlayer)
		{
			nearestPlayer = distance;
			survive = client.ent;
		}
	}

	if (FNullEnt(survive))
		return false; // nothing found

	// fill the holder
	if (needBot)
		*pvHolder = reinterpret_cast<void*>(g_botManager->GetBot(survive));
	else
		*pvHolder = reinterpret_cast<void*>(survive);

	return true;
}

// this function returning weapon id from the weapon alias and vice versa.
int GetWeaponReturn(const bool needString, const char* weaponAlias, const int weaponID)
{
	// structure definition for weapon tab
	struct WeaponTab_t
	{
		int weaponID{}; // weapon id
		const char* alias{}; // weapon alias
	};

	// weapon enumeration
	WeaponTab_t weaponTab[] =
	{
	   {Weapon::Usp, "usp"}, // HK USP .45 Tactical
	   {Weapon::Glock18, "glock"}, // Glock18 Select Fire
	   {Weapon::Deagle, "deagle"}, // Desert Eagle .50AE
	   {Weapon::P228, "p228"}, // SIG P228
	   {Weapon::Elite, "elite"}, // Dual Beretta 96G Elite
	   {Weapon::FiveSeven, "fn57"}, // FN Five-Seven
	   {Weapon::M3, "m3"}, // Benelli M3 Super90
	   {Weapon::Xm1014, "xm1014"}, // Benelli XM1014
	   {Weapon::Mp5, "mp5"}, // HK MP5-Navy
	   {Weapon::Tmp, "tmp"}, // Steyr Tactical Machine Pistol
	   {Weapon::P90, "p90"}, // FN P90
	   {Weapon::Mac10, "mac10"}, // Ingram MAC-10
	   {Weapon::Ump45, "ump45"}, // HK UMP45
	   {Weapon::Ak47, "ak47"}, // Automat Kalashnikov AK-47
	   {Weapon::Galil, "galil"}, // IMI Galil
	   {Weapon::Famas, "famas"}, // GIAT FAMAS
	   {Weapon::Sg552, "sg552"}, // Sig SG-552 Commando
	   {Weapon::M4A1, "m4a1"}, // Colt M4A1 Carbine
	   {Weapon::Aug, "aug"}, // Steyr Aug
	   {Weapon::Scout, "scout"}, // Steyr Scout
	   {Weapon::Awp, "awp"}, // AI Arctic Warfare/Magnum
	   {Weapon::G3SG1, "g3sg1"}, // HK G3/SG-1 Sniper Rifle
	   {Weapon::Sg550, "sg550"}, // Sig SG-550 Sniper
	   {Weapon::M249, "m249"}, // FN M249 Para
	   {Weapon::FbGrenade, "flash"}, // Concussion Grenade
	   {Weapon::HeGrenade, "hegren"}, // High-Explosive Grenade
	   {Weapon::SmGrenade, "sgren"}, // Smoke Grenade
	   {Weapon::Kevlar, "vest"}, // Kevlar Vest
	   {Weapon::KevlarHelmet, "vesthelm"}, // Kevlar Vest and Helmet
	   {Weapon::Defuser, "defuser"}, // Defuser Kit
	   {Weapon::Shield, "shield"}, // Tactical Shield
	};

	// if we need to return the string, find by weapon id
	if (needString && weaponID != -1)
	{
		for (auto pointer : weaponTab)
		{
			if (pointer.weaponID == weaponID) // is weapon id found?
				return MAKE_STRING(pointer.alias);
		}

		return MAKE_STRING("(none)"); // return none
	}

	// else search weapon by name and return weapon id
	for (auto pointer : weaponTab)
	{
		if (cstrncmp(pointer.alias, weaponAlias, cstrlen(pointer.alias)) == 0)
			return pointer.weaponID;
	}

	return -1; // no weapon was found return -1
}

ChatterMessage GetEqualChatter(const int message)
{
	switch (message)
	{
	case Radio::Affirmative:
		return ChatterMessage::Yes;
	case Radio::Negative:
		return ChatterMessage::No;
	case Radio::EnemySpotted:
	case Radio::NeedBackup:
	case Radio::TakingFire:
		return ChatterMessage::SeeksEnemy;
	case Radio::EnemyDown:
		return ChatterMessage::EnemyDown;
	case Radio::CoverMe:
		return ChatterMessage::CoverMe;
	case Radio::SectorClear:
		return ChatterMessage::Clear;
	case Radio::InPosition:
		return ChatterMessage::Camp;
	case Radio::ReportingIn:
		return ChatterMessage::ReportingIn;
	case Radio::ReportTeam:
		return ChatterMessage::ReportTeam;
	}

	return ChatterMessage::Nothing;
}

char* GetVoice(const ChatterMessage message)
{
	switch (message)
	{
	case ChatterMessage::Yes:
	{
		const int rV = crandomint(1, 17);
		switch (rV)
		{
		case 1:
			return "affirmative";
		case 2:
			return "alright";
		case 3:
			return "alright_lets_do_this";
		case 4:
			return "alright2";
		case 5:
			return "ok";
		case 6:
			return "ok_sir_lets_go";
		case 7:
			return "ok_cmdr_lets_go";
		case 8:
			return "ok2";
		case 9:
			return "roger";
		case 10:
			return "roger_that";
		case 11:
			return "yea_ok";
		case 12:
			return "you_heard_the_man_lets_go";
		case 13:
			return "hang_on_im_coming";
		case 14:
			return "im_coming";
		case 15:
			return "sounds_like_a_plan";
		case 16:
			return "be_right_there";
		case 17:
			return "good_idea";
		}
		break;
	}
	case ChatterMessage::No:
	{
		const int rV = crandomint(1, 13);
		switch (rV)
		{
		case 1:
			return "ahh_negative";
		case 2:
			return "negative";
		case 3:
			return "negative2";
		case 4:
			return "no";
		case 5:
			return "no_sir";
		case 6:
			return "no_thanks";
		case 7:
			return "no2";
		case 8:
			return "naa";
		case 9:
			return "nnno_sir";
		case 10:
			return "hes_broken";
		case 11:
			return "i_dont_think_so";
		case 12:
			return "noo";
		case 13:
			return "do_not_mess_with_me";
		}
		break;
	}
	case ChatterMessage::SeeksEnemy:
	{
		const int rV = crandomint(1, 19);
		switch (rV)
		{
		case 1:
			return "help";
		case 2:
			return "need_help";
		case 3:
			return "need_help2";
		case 4:
			return "taking_fire_need_assistance2";
		case 5:
			return "engaging_enemies";
		case 6:
			return "attacking";
		case 7:
			return "attacking_enemies";
		case 8:
			return "a_bunch_of_them";
		case 9:
			return "im_pinned_down";
		case 10:
			return "im_in_trouble";
		case 11:
			return "in_combat";
		case 12:
			return "in_combat2";
		case 13:
			return "target_acquired";
		case 14:
			return "target_spotted";
		case 15:
			return "i_see_our_target";
		case 16:
			return "i_could_use_some_help";
		case 17:
			return "i_could_use_some_help_over_here";
		case 18:
			return "returning_fire";
		case 19:
			return "they_got_me_pinned_down_here";
		}
		break;
	}
	case ChatterMessage::Clear:
	{
		const int rV = crandomint(1, 24);
		switch (rV)
		{
		case 1:
			return "clear";
		case 2:
			return "clear2";
		case 3:
			return "clear3";
		case 4:
			return "clear4";
		case 5:
			return "where_are_you_hiding";
		case 6:
		{
			for (const auto& otherBot : g_botManager->m_bots)
			{
				if (otherBot)
					otherBot->m_radioOrder = Radio::ReportTeam;
			}
			return "where_could_they_be";
		}
		case 7:
		{
			for (const auto& otherBot : g_botManager->m_bots)
			{
				if (otherBot)
					otherBot->m_radioOrder = Radio::ReportTeam;
			}
			return "where_is_it";
		}
		case 8:
			return "area_clear";
		case 9:
			return "area_secure";
		case 10:
		{
			for (const auto& otherBot : g_botManager->m_bots)
			{
				if (otherBot)
					otherBot->m_radioOrder = Radio::ReportTeam;
			}
			return "anyone_see_anything";
		}
		case 11:
			return "all_clear_here";
		case 12:
			return "all_quiet";
		case 13:
			return "nothing";
		case 14:
			return "nothing_happening_over_here";
		case 15:
			return "nothing_here";
		case 16:
			return "nothing_moving_over_here";
		case 17:
		{
			for (const auto& otherBot : g_botManager->m_bots)
			{
				if (otherBot)
					otherBot->m_radioOrder = Radio::ReportTeam;
			}
			return "anyone_see_them";
		}
		case 18:
		{
			for (const auto& otherBot : g_botManager->m_bots)
			{
				if (otherBot)
					otherBot->m_radioOrder = Radio::ReportTeam;
			}
			return "report_in_team";
		}
		case 19:
			return "they_will_not_escape";
		case 20:
			return "they_wont_get_away";
		case 21:
			return "they_wont_get_away2";
		case 22:
			return "come_out_and_fight_like_a_man";
		case 23:
			return "come_out_wherever_you_are";
		case 24:
			return "come_to_papa";
		}
		break;
	}
	case ChatterMessage::CoverMe:
	{
		const int rV = crandomint(1, 2);
		switch (rV)
		{
		case 1:
			return "cover_me";
		case 2:
			return "cover_me2";
		}
		break;
	}
	case ChatterMessage::Happy:
	{
		const int rV = crandomint(1, 21);
		switch (rV)
		{
		case 1:
			return "yea_baby";
		case 2:
			return "whos_the_man";
		case 3:
			return "who_wants_some_more";
		case 4:
			return "yikes";
		case 5:
			return "yesss";
		case 6:
			return "yesss2";
		case 7:
			return "whoo";
		case 8:
			return "i_am_dangerous";
		case 9:
			return "i_am_on_fire";
		case 10:
			return "whoo2";
		case 11:
			return "i_got_more_where_that_came_from";
		case 12:
			return "i_wasnt_worried_for_a_minute";
		case 13:
			return "its_a_party";
		case 14:
			return "thats_the_way_this_is_done";
		case 15:
			return "they_never_knew_what_hit_them";
		case 16:
			return "well_done";
		case 17:
			return "and_thats_how_its_done";
		case 18:
			return "good_idea_team";
		case 19:
			return "good_one";
		case 20:
			return "good_one2";
		case 21:
			return "great";
		}
		break;
	}
	case ChatterMessage::Camp:
	{
		const int rV = crandomint(1, 4);
		switch (rV)
		{
		case 1:
			return "im_going_to_camp";
		case 2:
			return "im_going_to_wait_here";
		case 3:
			return "weve_got_the_situation";
		case 4:
			return "lets_wait_here";
		}
		break;
	}
	case ChatterMessage::EnemyDown:
	{
		const int rV = crandomint(1, 19);
		switch (rV)
		{
		case 1:
			return "took_him_down";
		case 2:
			return "took_him_out";
		case 3:
			return "took_him_out2";
		case 4:
			return "enemy_down";
		case 5:
			return "enemy_down2";
		case 6:
			return "got_him";
		case 7:
			return "hes_done";
		case 8:
			return "hes_dead";
		case 9:
			return "hes_down";
		case 10:
			return "i_got_more_where_that_came_from";
		case 11:
			return "i_wasnt_worried_for_a_minute";
		case 12:
			return "killed_him";
		case 13:
			return "neutralized";
		case 14:
			return "owned";
		case 15:
			return "tag_them_and_bag_them";
		case 16:
			return "thats_the_way_this_is_done";
		case 17:
			return "and_thats_how_its_done";
		case 18:
			return "do_not_mess_with_me";
		case 19:
			return "dropped_him";
		}
		break;
	}
	case ChatterMessage::ReportingIn:
		return "reporting_in";
	case ChatterMessage::FallBack:
	{
		const int rV = crandomint(1, 4);
		switch (rV)
		{
		case 1:
			return "im_gonna_hang_back";
		case 2:
			return "thats_not_good";
		case 3:
			return "aw_hell";
		case 4:
			return "aww_man";
		}
		break;
	}
	case ChatterMessage::ReportTeam:
	{
		const int rV = crandomint(1, 5);
		switch (rV)
		{
		case 1:
			return "report_in_team";
		case 2:
			return "where_could_they_be";
		case 3:
			return "anyone_see_them";
		case 4:
			return "anyone_see_anything";
		case 5:
			return "where_is_it";
		}
		break;
	}
	}

	return nullptr;
}