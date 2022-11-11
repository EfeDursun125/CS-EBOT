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

ConVar ebot_apitestmsg("ebot_apitestmsg", "0");

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

uint16 FixedUnsigned16(float value, float scale)
{
	int output = (static_cast <int> (value * scale));

	if (output < 0)
		output = 0;

	if (output > 0xffff)
		output = 0xffff;

	return static_cast <uint16> (output);
}

short FixedSigned16(float value, float scale)
{
	int output = (static_cast <int> (value * scale));

	if (output > 32767)
		output = 32767;

	if (output < -32768)
		output = -32768;

	return static_cast <short> (output);
}

bool IsAlive(edict_t* ent)
{
	if (FNullEnt(ent))
		return false; // reliability check

	return (ent->v.deadflag == DEAD_NO) && (ent->v.health > 0) && (ent->v.movetype != MOVETYPE_NOCLIP);
}

float GetShootingConeDeviation(edict_t* ent, Vector* position)
{
	const Vector& dir = (*position - (GetEntityOrigin(ent) + ent->v.view_ofs)).Normalize();
	MakeVectors(ent->v.v_angle);

	// he's facing it, he meant it
	return g_pGlobals->v_forward | dir;
}

bool IsInViewCone(Vector origin, edict_t* ent)
{
	if (FNullEnt(ent))
		return true;

	MakeVectors(ent->v.v_angle);

	if (((origin - (GetEntityOrigin(ent) + ent->v.view_ofs)).Normalize() | g_pGlobals->v_forward) >= cosf(((ent->v.fov > 0 ? ent->v.fov : 90.0f) / 2) * Math::MATH_PI / 180.0f))
		return true;

	return false;
}

bool IsVisible(const Vector& origin, edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	TraceResult tr;
	TraceLine(GetEntityOrigin(ent), origin, true, true, ent, &tr);

	if (tr.flFraction != 1.0f)
		return false; // line of sight is not established

	return true; // line of sight is valid.
}

bool IsVisibleForKnifeAttack(const Vector& origin, edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	TraceResult tr;
	TraceHull(GetEntityOrigin(ent), origin, true, human_hull, ent, &tr);

	if (tr.flFraction != 1.0f)
		return false; // line of sight is not established

	return true; // line of sight is valid.
}

// return walkable position on ground
Vector GetWalkablePosition(const Vector& origin, edict_t* ent, bool returnNullVec)
{
	TraceResult tr;
	TraceLine(origin, Vector(origin.x, origin.y, -9999.0f), true, false, ent, &tr);

	if (tr.flFraction != 1.0f)
		return tr.vecEndPos; // walkable ground

	if (returnNullVec)
		return nullvec; // return nullvector for check if we can't hit the ground
	else
		return origin; // return original origin, we cant hit to ground
}

// same with GetWalkablePosition but gets nearest walkable position
Vector GetNearestWalkablePosition(const Vector& origin, edict_t* ent, bool returnNullVec)
{
	TraceResult tr;
	TraceLine(origin, Vector(origin.x, origin.y, -9999.0f), true, false, ent, &tr);

	Vector BestOrigin = origin;

	Vector FirstOrigin;
	Vector SecondOrigin;

	if (tr.flFraction != 1.0f)
		FirstOrigin = tr.vecEndPos; // walkable ground?
	else
		FirstOrigin = nullvec;

	if (g_numWaypoints > 0)
		SecondOrigin = g_waypoint->GetPath(g_waypoint->FindNearest(origin))->origin; // get nearest waypoint for walk
	else
		SecondOrigin = nullvec;

	if (FirstOrigin == nullvec)
		BestOrigin = SecondOrigin;
	else if (SecondOrigin == nullvec)
		BestOrigin = FirstOrigin;
	else
	{
		if ((origin - FirstOrigin).GetLength() < (origin - SecondOrigin).GetLength())
			BestOrigin = FirstOrigin;
		else
			BestOrigin = SecondOrigin;
	}

	if (!returnNullVec && BestOrigin == nullvec)
		BestOrigin = origin;

	return BestOrigin;
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

// Get Entity Top/Bottom Origin
Vector GetTopOrigin(edict_t* ent)
{
	if (FNullEnt(ent))
		return nullvec;

	Vector origin = GetEntityOrigin(ent);
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

	Vector origin = GetEntityOrigin(ent);
	Vector bottomOrigin = origin + ent->v.mins;
	if (bottomOrigin.z > origin.z)
		bottomOrigin = origin + ent->v.maxs;

	bottomOrigin.x = origin.x;
	bottomOrigin.y = origin.y;
	return bottomOrigin;
}

// Get Player Head Origin 
Vector GetPlayerHeadOrigin(edict_t* ent)
{
	if (FNullEnt(ent))
		return nullvec;

	Vector headOrigin = GetTopOrigin(ent);

	if (!(ent->v.flags & FL_DUCKING))
	{
		Vector origin = GetEntityOrigin(ent);
		float hbDistance = headOrigin.z - origin.z;
		hbDistance /= 2.5f;
		headOrigin.z -= hbDistance;
	}
	else
		headOrigin.z -= 1.0f;

	return headOrigin;
}

void DisplayMenuToClient(edict_t* ent, MenuText* menu)
{
	if (!IsValidPlayer(ent))
		return;

	int clientIndex = ENTINDEX(ent) - 1;

	if (menu != nullptr)
	{
		String tempText = String(menu->menuText);
		tempText.Replace("\v", "\n");

		char* text = tempText;
		tempText = String(text);

		// make menu looks best
		for (int i = 0; i <= 9; i++)
			tempText.Replace(FormatBuffer("%d.", i), FormatBuffer("\\r%d.\\w", i));

		text = tempText;

		while (strlen(text) >= 64)
		{
			MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, g_netMsg->GetId(NETMSG_SHOWMENU), nullptr, ent);
			WRITE_SHORT(menu->validSlots);
			WRITE_CHAR(-1);
			WRITE_BYTE(1);

			for (int i = 0; i <= 63; i++)
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

void DecalTrace(entvars_t* pev, TraceResult* trace, int logotypeIndex)
{
	// this function draw spraypaint depending on the tracing results.

	static Array <String> logotypes;

	if (logotypes.IsEmpty())
		logotypes = String("{biohaz;{graf004;{graf005;{lambda06;{target;{hand1").Split(";");

	int entityIndex = -1, message = TE_DECAL;
	int decalIndex = (*g_engfuncs.pfnDecalIndex) (logotypes[logotypeIndex]);

	if (decalIndex < 0)
		decalIndex = (*g_engfuncs.pfnDecalIndex) ("{lambda06");

	if (trace->flFraction == 1.0f)
		return;

	if (!FNullEnt(trace->pHit))
	{
		if (trace->pHit->v.solid == SOLID_BSP || trace->pHit->v.movetype == MOVETYPE_PUSHSTEP)
			entityIndex = ENTINDEX(trace->pHit);
		else
			return;
	}
	else
		entityIndex = 0;

	if (entityIndex != 0)
	{
		if (decalIndex > 255)
		{
			message = TE_DECALHIGH;
			decalIndex -= 256;
		}
	}
	else
	{
		message = TE_WORLDDECAL;

		if (decalIndex > 255)
		{
			message = TE_WORLDDECALHIGH;
			decalIndex -= 256;
		}
	}

	if (logotypes[logotypeIndex].Contains("{"))
	{
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_PLAYERDECAL);
		WRITE_BYTE(ENTINDEX(ENT(pev)));
		WRITE_COORD(trace->vecEndPos.x);
		WRITE_COORD(trace->vecEndPos.y);
		WRITE_COORD(trace->vecEndPos.z);
		WRITE_SHORT(static_cast <short> (ENTINDEX(trace->pHit)));
		WRITE_BYTE(decalIndex);
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(message);
		WRITE_COORD(trace->vecEndPos.x);
		WRITE_COORD(trace->vecEndPos.y);
		WRITE_COORD(trace->vecEndPos.z);
		WRITE_BYTE(decalIndex);

		if (entityIndex)
			WRITE_SHORT(entityIndex);

		MESSAGE_END();
	}
}

// this function free's all allocated memory
void FreeLibraryMemory(void)
{
	g_botManager->Free();
	g_waypoint->Initialize(); // frees waypoint data
}

void SetEntityActionData(int i, int index, int team, int action)
{
	g_entityId[i] = index;
	g_entityTeam[i] = team;
	g_entityAction[i] = action;
	g_entityWpIndex[i] = -1;
	g_entityGetWpOrigin[i] = nullvec;
	g_entityGetWpTime[i] = 0.0f;
}

void FakeClientCommand(edict_t* fakeClient, const char* format, ...)
{
	// the purpose of this function is to provide fakeclients (bots) with the same client
	// command-scripting advantages (putting multiple commands in one line between semicolons)
	// as real players. It is an improved version of botman's FakeClientCommand, in which you
	// supply directly the whole string as if you were typing it in the bot's "console". It
	// is supposed to work exactly like the pfnClientCommand (server-sided client command).

	if (FNullEnt(fakeClient))
		return; // reliability check

	if (!IsValidBot(fakeClient))
		return;

	va_list ap;
	static char string[256];

	va_start(ap, format);
	vsnprintf(string, sizeof(string), format, ap);
	va_end(ap);

	if (IsNullString(string))
		return;

	g_isFakeCommand = true;

	int i, pos = 0;
	int length = strlen(string);
	int stringIndex = 0;

	while (pos < length)
	{
		int start = pos;
		int stop = pos;

		while (pos < length && string[pos] != ';')
			pos++;

		if (string[pos - 1] == '\n')
			stop = pos - 2;
		else
			stop = pos - 1;

		for (i = start; i <= stop; i++)
			g_fakeArgv[i - start] = string[i];

		g_fakeArgv[i - start] = 0;
		pos++;

		int index = 0;
		stringIndex = 0;

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
				while (index < i - start && g_fakeArgv[index] != ' ')
					index++;

			stringIndex++;
		}
		MDLL_ClientCommand(fakeClient);
	}
	g_isFakeCommand = false;
}

const char* GetField(const char* string, int fieldId, bool endLine)
{
	// This function gets and returns a particuliar field in a string where several szFields are
	// concatenated. Fields can be words, or groups of words between quotes ; separators may be
	// white space or tabs. A purpose of this function is to provide bots with the same Cmd_Argv
	// convenience the engine provides to real clients. This way the handling of real client
	// commands and bot client commands is exactly the same, just have a look in engine.cpp
	// for the hooking of pfnCmd_Argc, pfnCmd_Args and pfnCmd_Argv, which redirects the call
	// either to the actual engine functions (when the caller is a real client), either on
	// our function here, which does the same thing, when the caller is a bot.

	static char field[256];

	// reset the string
	memset(field, 0, sizeof(field));

	int length, i, index = 0, fieldCount = 0, start, stop;

	field[0] = 0; // reset field
	length = strlen(string); // get length of string

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
		field[strlen(field) - 1] = 0;

	strtrim(field);

	return (&field[0]); // returns the wanted field
}

void strtrim(char* string)
{
	char* ptr = string;

	int length = 0, toggleFlag = 0, increment = 0;
	int i = 0;

	while (*ptr++)
		length++;

	for (i = length - 1; i >= 0; i--)
	{
#if defined (PLATFORM_WIN32)
		if (!iswspace(string[i]))
#else
		if (!isspace(string[i]))
#endif
			break;
		else
		{
			string[i] = 0;
			length--;
		}
	}

	for (i = 0; i < length; i++)
	{
#if defined (PLATFORM_WIN32)
		if (iswspace(string[i]) && !toggleFlag) // win32 crash fx
#else
		if (isspace(string[i]) && !toggleFlag)
#endif
		{
			increment++;

			if (increment + i < length)
				string[i] = string[increment + i];
		}
		else
		{
			if (!toggleFlag)
				toggleFlag = 1;

			if (increment)
				string[i] = string[increment + i];
		}
	}
	string[length] = 0;
}

const char* GetModName(void)
{
	static char modName[256];

	GET_GAME_DIR(modName); // ask the engine for the MOD directory path
	int length = strlen(modName); // get the length of the returned string

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
	for (char* ofs = path + 1; *ofs; ofs++)
	{
		if (*ofs == '/')
		{
			// create the directory
			*ofs = 0;
#ifdef PLATFORM_WIN32
			mkdir(path);
#else
			mkdir(path, 0777);
#endif
			* ofs = '/';
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
	g_audioTime = 0.0f;

	if (GetGameMode() == MODE_BASE)
	{
		// check team economics
		g_botManager->CheckTeamEconomics(TEAM_TERRORIST);
		g_botManager->CheckTeamEconomics(TEAM_COUNTER);
	}

	for (int i = 0; i < engine->GetMaxClients(); i++)
	{
		if (g_botManager->GetBot(i))
			g_botManager->GetBot(i)->NewRound();

		g_radioSelect[i] = 0;
	}

	g_waypoint->SetBombPosition(true);
	g_waypoint->ClearGoalScore();

	g_waypoint->InitTypes(1);

	g_bombSayString = false;
	g_timeBombPlanted = 0.0f;
	g_timeNextBombUpdate = 0.0f;

	g_leaderChoosen[TEAM_COUNTER] = false;
	g_leaderChoosen[TEAM_TERRORIST] = false;

	g_lastRadioTime[0] = 0.0f;
	g_lastRadioTime[1] = 0.0f;
	g_botsCanPause = false;

	AutoLoadGameMode();

	// calculate the round mid/end in world time
	g_timeRoundStart = engine->GetTime() + engine->GetFreezeTime();
	g_timeRoundMid = g_timeRoundStart + engine->GetRoundTime() * 60 / 2;
	g_timeRoundEnd = g_timeRoundStart + engine->GetRoundTime() * 60;
}

void AutoLoadGameMode(void)
{
	if (!g_isMetamod)
		return;

	static int checkShowTextTime = 0;
	checkShowTextTime++;

	// CS:BTE Support 
	char* Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/bte_player.ini", GetModName());
	if (TryFileOpen(Plugin_INI) || TryFileOpen(FormatBuffer("%s/addons/amxmodx/configs/bte_config/bte_blockresource.txt", GetModName())))
	{
		const int Const_GameModes = 13;
		int bteGameModAi[Const_GameModes] =
		{
			MODE_BASE,		//1
			MODE_TDM,		//2
			MODE_DM,		//3
			MODE_NOTEAM,	//4
			MODE_TDM,		//5
			MODE_ZP,		//6
			MODE_ZP,		//7
			MODE_ZP,		//8
			MODE_ZP,		//9
			MODE_ZH,		//10
			MODE_ZP,		//11
			MODE_NOTEAM,	//12
			MODE_ZP			//13
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

		for (int i = 0; i < Const_GameModes; i++)
		{
			if (TryFileOpen(FormatBuffer("%s/addons/amxmodx/configs/%s.ini", GetModName(), bteGameINI[i])))
			{
				if (bteGameModAi[i] == 2 && i != 5)
					g_DelayTimer = engine->GetTime() + 20.0f + CVAR_GET_FLOAT("mp_freezetime");

				if (checkShowTextTime < 3 || GetGameMode() != bteGameModAi[i])
					ServerPrint("*** E-BOT Auto Game Mode Setting: CS:BTE [%s] [%d] ***", bteGameINI[i], bteGameModAi[i]);

				if (i == 3 || i == 9)
				{
					ServerPrint("***** E-BOT not support the mode now :( *****");

					SetGameMod(MODE_TDM);
				}
				else
					SetGameMod(bteGameModAi[i]);

				g_gameVersion = CSVER_CZERO;

				// Only ZM3 need restart the round
				if (checkShowTextTime < 3 && i == 7)
					ServerCommand("sv_restart 1");

				break;
			}
		}

		goto lastly;
	}

	// Zombie
	char* zpGameVersion[] =
	{
		"plugins-zplague",  // ZP 4.3
		"plugins-zp50_ammopacks", // ZP 5.0
		"plugins-zp50_money", // ZP 5.0
		"plugins-ze", // ZE
		"plugins-zp", // ZP
		"plugins-zescape", // ZE
		"plugins-escape", // ZE
		"plugins-plague" // ZP
	};

	for (int i = 0; i < 8; i++)
	{
		Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/%s.ini", GetModName(), zpGameVersion[i]);
		if (TryFileOpen(Plugin_INI))
		{
			float delayTime = CVAR_GET_FLOAT("zp_delay") + 2.2f;

			if (i != 0)
				delayTime = CVAR_GET_FLOAT("zp_gamemode_delay") + 0.2f;

			if (delayTime > 0)
			{
				if (checkShowTextTime < 3 || GetGameMode() != MODE_ZP)
					ServerPrint("*** E-BOT Auto Game Mode Setting: Zombie Mode (Plague/Escape) ***");

				SetGameMod(MODE_ZP);
				g_DelayTimer = engine->GetTime() + delayTime;
				goto lastly;
			}
		}
	}

	// zombie escape
	if (g_mapType & MAP_ZE)
	{
		extern ConVar ebot_escape;
		ebot_escape.SetInt(1);
		ServerPrint("*** E-BOT Detected Zombie Escape Map: ebot_zombie_escape_mode is set to 1 ***");
	}

	// Base Builder
	char* bbVersion[] =
	{
		"plugins-basebuilder",
		"plugins-bb"
	};

	for (int i = 0; i < 2; i++)
	{
		Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/%s.ini", GetModName(), bbVersion[i]);
		if (TryFileOpen(Plugin_INI))
		{
			float delayTime = CVAR_GET_FLOAT("bb_buildtime") + CVAR_GET_FLOAT("bb_preptime") + 2.2f;

			if (delayTime > 0)
			{
				if (checkShowTextTime < 3 || GetGameMode() != MODE_ZP)
					ServerPrint("*** E-BOT Auto Game Mode Setting: Zombie Mode (Base Builder) ***");

				SetGameMod(MODE_ZP);

				g_DelayTimer = engine->GetTime() + delayTime;

				goto lastly;
			}
		}
	}

	// DM:KD
	Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/plugins-dmkd.ini", GetModName());
	if (TryFileOpen(Plugin_INI))
	{
		if (CVAR_GET_FLOAT("DMKD_DMMODE") == 1)
		{
			if (checkShowTextTime < 3 || GetGameMode() != MODE_DM)
				ServerPrint("*** E-BOT Auto Game Mode Setting: DM:KD-DM ***");

			SetGameMod(MODE_DM);
		}
		else
		{
			if (checkShowTextTime < 3 || GetGameMode() != MODE_TDM)
				ServerPrint("*** E-BOT Auto Game Mode Setting: DM:KD-TDM ***");

			SetGameMod(MODE_TDM);
		}

		goto lastly;
	}

	// Zombie Hell
	Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/zombiehell.cfg", GetModName());
	if (TryFileOpen(Plugin_INI) && CVAR_GET_FLOAT("zh_zombie_maxslots") > 0)
	{
		if (checkShowTextTime < 3 || GetGameMode() != MODE_ZH)
			ServerPrint("*** E-BOT Auto Game Mode Setting: Zombie Hell ***");

		SetGameMod(MODE_ZH);

		extern ConVar ebot_quota;
		ebot_quota.SetInt(static_cast <int> (CVAR_GET_FLOAT("zh_zombie_maxslots")));

		goto lastly;
	}

	// Biohazard
	char* biohazard[] =
	{
		"plugins-biohazard",
		"plugins-bio",
		"plugins-bh"
	};

	for (int i = 0; i < 3; i++)
	{
		Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/%s.ini", GetModName(), biohazard[i]);
		if (TryFileOpen(Plugin_INI))
		{
			float delayTime = CVAR_GET_FLOAT("bh_starttime") + 0.5f;

			if (delayTime > 0)
			{
				if (checkShowTextTime < 3 || GetGameMode() != MODE_ZP)
					ServerPrint("*** E-BOT Auto Game Mode Setting: Zombie Mode (Biohazard) ***");

				SetGameMod(MODE_ZP);

				g_DelayTimer = engine->GetTime() + delayTime;

				goto lastly;
			}
		}
	}

	// Anti-Block
	Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/plugins-tsc.ini", GetModName());
	if (TryFileOpen(Plugin_INI))
	{
		extern ConVar ebot_anti_block;

		ebot_anti_block.SetInt(1);

		ServerPrint("*** E-BOT Anti-Block Enabled ***");
	}

	static auto dmActive = g_engfuncs.pfnCVarGetPointer("csdm_active");
	static auto freeForAll = g_engfuncs.pfnCVarGetPointer("mp_freeforall");

	if (dmActive && freeForAll)
	{
		if (dmActive->value > 0.0f)
		{
			if (freeForAll->value > 0.0f)
			{
				if (checkShowTextTime < 3 || GetGameMode() != MODE_DM)
					ServerPrint("*** E-BOT Auto Game Mode Setting: CSDM-DM ***");

				SetGameMod(MODE_DM);
			}
		}
	}

	if (checkShowTextTime < 3)
	{
		if (GetGameMode() == MODE_BASE)
			ServerPrint("*** E-BOT Auto Game Mode Setting: Base Mode ***");
		else
			ServerPrint("*** E-BOT Auto Game Mode Setting: N/A ***");
	}

lastly:
	if (GetGameMode() != MODE_BASE)
		g_mapType |= MAP_DE;
	else
		g_exp.UpdateGlobalKnowledge(); // update experience data on round start
}

bool IsWeaponShootingThroughWall(int id)
{
	// returns if weapon can pierce through a wall

	int i = 0;

	while (g_weaponSelect[i].id)
	{
		if (g_weaponSelect[i].id == id)
		{
			if (g_weaponSelect[i].shootsThru)
				return true;

			return false;
		}
		i++;
	}

	return false;
}

void SetGameMod(int gamemode)
{
	ebot_gamemod.SetInt(gamemode);
}

bool IsZombieMode(void)
{
	return (ebot_gamemod.GetInt() == MODE_ZP || ebot_gamemod.GetInt() == MODE_ZH);
}

bool IsDeathmatchMode(void)
{
	return (ebot_gamemod.GetInt() == MODE_DM || ebot_gamemod.GetInt() == MODE_TDM);
}

bool ChanceOf(int number)
{
	return engine->RandomInt(1, 100) <= number;
}

bool IsValidWaypoint(int index)
{
	if (index < 0 || index >= g_numWaypoints)
		return false;
	return true;
}

int GetGameMode(void)
{
	return ebot_gamemod.GetInt();
}

float Q_rsqrt(float number)
{
#ifdef __SSE2__
	return _mm_cvtss_f32(_mm_sqrt_ss(_mm_load_ss(&number)));
#else
	long i;
	float x2, y;
	const float threehalfs = 1.5F;
	x2 = number * 0.5F;
	y = number;
	i = *(long*)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(float*)&i;
	y = y * (threehalfs - (x2 * y * y));
	return y * number;
#endif
}

float Clamp(float a, float b, float c)
{
#ifdef __SSE2__
	return _mm_cvtss_f32(_mm_min_ss(_mm_max_ss(_mm_load_ss(&a), _mm_load_ss(&b)), _mm_load_ss(&c)));
#else
	return engine->DoClamp(a, b, c);
#endif
}

float SquaredF(float a)
{
	return MultiplyFloat(a, a);
}

float MultiplyFloat(float a, float b)
{
#ifdef __SSE2__
	return _mm_cvtss_f32(_mm_mul_ss(_mm_load_ss(&a), _mm_load_ss(&b)));
#else
	return a * b;
#endif
}

float AddTime(float a)
{
#ifdef __SSE2__
	return _mm_cvtss_f32(_mm_add_ss(_mm_load_ss(&g_pGlobals->time), _mm_load_ss(&a)));
#else
	return g_pGlobals->time + a;
#endif
}

Vector AddVector(Vector a, Vector b)
{
#ifdef __SSE2__
	Vector newVec;
	newVec.x = _mm_cvtss_f32(_mm_add_ss(_mm_load_ss(&a.x), _mm_load_ss(&b.x)));
	newVec.y = _mm_cvtss_f32(_mm_add_ss(_mm_load_ss(&a.y), _mm_load_ss(&b.y)));
	newVec.z = _mm_cvtss_f32(_mm_add_ss(_mm_load_ss(&a.z), _mm_load_ss(&b.z)));
	return newVec;
#else
	return a + b;
#endif
}

Vector MultiplyVector(Vector a, Vector b)
{
#ifdef __SSE2__
	Vector newVec;
	newVec.x = _mm_cvtss_f32(_mm_mul_ss(_mm_load_ss(&a.x), _mm_load_ss(&b.x)));
	newVec.y = _mm_cvtss_f32(_mm_mul_ss(_mm_load_ss(&a.y), _mm_load_ss(&b.y)));
	newVec.z = _mm_cvtss_f32(_mm_mul_ss(_mm_load_ss(&a.z), _mm_load_ss(&b.z)));
	return newVec;
#else
	return a * b;
#endif
}

int AddInt(int a, int b)
{
#ifdef __SSE2__
	return _mm_cvtsi128_si32(_mm_add_epi32(_mm_loadu_si32(&a), _mm_loadu_si32(&b)));
#else
	return a + b;
#endif
}

float AddFloat(float a, float b)
{
#ifdef __SSE2__
	return _mm_cvtss_f32(_mm_add_ss(_mm_load_ss(&a), _mm_load_ss(&b)));
#else
	return a + b;
#endif
}

float DivideFloat(float a, float b)
{
#ifdef __SSE2__
	return _mm_cvtss_f32(_mm_div_ss(_mm_load_ss(&a), _mm_load_ss(&b)));
#else
	return a / b;
#endif
}

float MaxFloat(float a, float b)
{
#ifdef __SSE2__
		return _mm_cvtss_f32(_mm_max_ss(_mm_load_ss(&a), _mm_load_ss(&b)));
#else
	if (a > b)
		return a;
	else if (b > a)
		return b;

	return b;
#endif
}

float MinFloat(float a, float b)
{
#ifdef __SSE2__
		return _mm_cvtss_f32(_mm_min_ss(_mm_load_ss(&a), _mm_load_ss(&b)));
#else
	if (a < b)
		return a;
	else if (b < a)
		return b;
	return b;
#endif
}

/*float VectorAngle(float x, float y)
{
	if (x == 0) // special cases
		return (y > 0) ? 90 : (y == 0) ? 0 : 270;
	else if (y == 0) // special cases
		return (x >= 0) ? 0 : 180;

	int ret = radToDeg(atanf((float)y / x));
	if (x < 0 && y < 0) // quadrant Ⅲ
		ret = 180 + ret;
	else if (x < 0) // quadrant Ⅱ
		ret = 180 + ret; // it actually substracts
	else if (y < 0) // quadrant Ⅳ
		ret = 270 + (90 + ret); // it actually substracts

	return ret;
}*/

// new get team off set, return player true team
int GetTeam(edict_t* ent)
{
	int client = ENTINDEX(ent) - 1, player_team = TEAM_COUNT;
	if (!IsValidPlayer(ent))
	{
		player_team = 0;
		for (int i = 0; i < entityNum; i++)
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

	if (GetGameMode() == MODE_DM)
		player_team = client * client;
	else if (GetGameMode() == MODE_ZP)
	{
		if (g_DelayTimer > engine->GetTime())
			player_team = TEAM_COUNTER;
		else if (g_roundEnded)
			player_team = TEAM_TERRORIST;
		else
			player_team = *((int*)ent->pvPrivateData + OFFSET_TEAM) - 1;
	}
	else if (GetGameMode() == MODE_NOTEAM)
		player_team = 2;
	else
		player_team = *((int*)ent->pvPrivateData + OFFSET_TEAM) - 1;

	g_clients[client].team = player_team;

	return player_team;
}

int SetEntityWaypoint(edict_t* ent, int mode)
{
	if (FNullEnt(ent))
		return -1;

	Vector origin = GetEntityOrigin(ent);
	if (origin == nullvec)
		return -1;

	bool isPlayer = IsValidPlayer(ent);
	int i = -1;

	if (isPlayer)
		i = ENTINDEX(ent) - 1;
	else
	{
		for (int j = 0; j < entityNum; j++)
		{
			if (g_entityId[j] == -1 || ent != INDEXENT(g_entityId[j]))
				continue;

			i = j;
			break;
		}
	}

	if (i == -1)
		return -1;

	bool needCheckNewWaypoint = false;
	float traceCheckTime = isPlayer ? g_clients[i].getWPTime : g_entityGetWpTime[i];
	if ((isPlayer && g_clients[i].wpIndex == -1) || (!isPlayer && g_entityWpIndex[i] == -1))
		needCheckNewWaypoint = true;
	else if ((!isPlayer && g_entityGetWpTime[i] == engine->GetTime()) ||
		(isPlayer && g_clients[i].getWPTime == engine->GetTime() &&
			mode > 0 && g_clients[i].wpIndex2 == -1))
		needCheckNewWaypoint = false;
	else if (mode != -1)
		needCheckNewWaypoint = true;
	else
	{
		Vector getWpOrigin = nullvec;
		int wpIndex = -1;
		if (isPlayer)
		{
			getWpOrigin = g_clients[i].getWpOrigin;
			wpIndex = g_clients[i].wpIndex;
		}
		else
		{
			getWpOrigin = g_entityGetWpOrigin[i];
			wpIndex = g_entityWpIndex[i];
		}

		if (getWpOrigin != nullvec && wpIndex >= 0 && wpIndex < g_numWaypoints)
		{
			float distance = (getWpOrigin - origin).GetLength();
			if (distance >= 300.0f)
				needCheckNewWaypoint = true;
			else if (distance >= 32.0f)
			{
				Vector wpOrigin = g_waypoint->GetPath(wpIndex)->origin;
				distance = (wpOrigin - origin).GetLength();

				if (distance > g_waypoint->GetPath(wpIndex)->radius + 32.0f)
					needCheckNewWaypoint = true;
				else
				{
					if (traceCheckTime + 5.0f <= engine->GetTime() ||
						(traceCheckTime + 2.5f <= engine->GetTime() && !g_waypoint->Reachable(ent, wpIndex)))
						needCheckNewWaypoint = true;
				}
			}
		}
		else
			needCheckNewWaypoint = true;
	}

	if (!needCheckNewWaypoint)
	{
		if (isPlayer)
		{
			g_clients[i].getWPTime = engine->GetTime();
			return g_clients[i].wpIndex;
		}

		g_entityGetWpTime[i] = engine->GetTime();
		return g_entityWpIndex[i];
	}

	int wpIndex = -1;
	int wpIndex2 = -1;

	if (mode == -1 || g_botManager->GetBot(ent) == nullptr)
		wpIndex = g_waypoint->FindNearest(origin, 9999.0f, -1, ent);
	else
		wpIndex = g_waypoint->FindNearest(origin, 9999.0f, -1, ent, &wpIndex2, mode);

	if (!isPlayer)
	{
		g_entityWpIndex[i] = wpIndex;
		g_entityGetWpOrigin[i] = origin;
		g_entityGetWpTime[i] = engine->GetTime();
	}
	else
	{
		g_clients[i].wpIndex = wpIndex;
		g_clients[i].wpIndex2 = wpIndex2;
		g_clients[i].getWpOrigin = origin;
		g_clients[i].getWPTime = engine->GetTime();
	}

	return wpIndex;
}

int GetEntityWaypoint(edict_t* ent)
{
	if (FNullEnt(ent))
		return -1;

	if (!IsValidPlayer(ent))
	{
		for (int i = 0; i < entityNum; i++)
		{
			if (g_entityId[i] == -1)
				continue;

			if (ent != INDEXENT(g_entityId[i]))
				continue;

			if (g_entityWpIndex[i] >= 0 && g_entityWpIndex[i] < g_numWaypoints)
				return g_entityWpIndex[i];

			return SetEntityWaypoint(ent);
		}

		return g_waypoint->FindNearest(GetEntityOrigin(ent), 99999.0f, -1, ent);
	}

	int client = ENTINDEX(ent) - 1;
	if (g_clients[client].getWPTime < engine->GetTime() + 1.5f || (g_clients[client].wpIndex == -1 && g_clients[client].wpIndex2 == -1))
		SetEntityWaypoint(ent);

	return g_clients[client].wpIndex;
}

bool IsZombieEntity(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	if (!IsValidPlayer(ent))
		return false;

	if (IsZombieMode()) // Zombie Mode
		return GetTeam(ent) == TEAM_TERRORIST;

	return false;
}

bool IsValidPlayer(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	if ((ent->v.flags & (FL_CLIENT | FL_FAKECLIENT)) || (strcmp(STRING(ent->v.classname), "player") == 0))
		return true;

	return false;
}

bool IsValidBot(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	if (ent->v.flags & FL_FAKECLIENT || g_botManager->GetBot(ent) != nullptr)
		return true;

	return false;
}

// return true if server is dedicated server, false otherwise
bool IsDedicatedServer(void)
{
	return (IS_DEDICATED_SERVER() > 0); // ask engine for this
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

void HudMessage(edict_t* ent, bool toCenter, const Color& rgb, char* format, ...)
{
	if (!IsValidPlayer(ent) || IsValidBot(ent))
		return;

	va_list ap;
	char buffer[1024];

	va_start(ap, format);
	vsprintf(buffer, format, ap);
	va_end(ap);

	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, nullptr, ent);
	WRITE_BYTE(TE_TEXTMESSAGE);
	WRITE_BYTE(1);
	WRITE_SHORT(FixedSigned16(-1, 1 << 13));
	WRITE_SHORT(FixedSigned16(toCenter ? -1.0f : 0.0f, 1 << 13));
	WRITE_BYTE(2);
	WRITE_BYTE(static_cast <int> (rgb.red));
	WRITE_BYTE(static_cast <int> (rgb.green));
	WRITE_BYTE(static_cast <int> (rgb.blue));
	WRITE_BYTE(0);
	WRITE_BYTE(engine->RandomInt(230, 255));
	WRITE_BYTE(engine->RandomInt(230, 255));
	WRITE_BYTE(engine->RandomInt(230, 255));
	WRITE_BYTE(200);
	WRITE_SHORT(FixedUnsigned16(0.0078125, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(2, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(6, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(0.1f, 1 << 8));
	WRITE_STRING(const_cast <const char*> (&buffer[0]));
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

void API_TestMSG(const char* format, ...)
{
	if (ebot_apitestmsg.GetBool() == false)
		return;

	va_list ap;
	char string[3072];

	va_start(ap, format);
	vsprintf(string, format, ap);
	va_end(ap);

	SERVER_PRINT(FormatBuffer("[%s-API Test] %s\n", PRODUCT_LOGTAG, string));
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

void ChartPrint(const char* format, ...)
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

	strcat(string, "\n");

	MESSAGE_BEGIN(MSG_BROADCAST, g_netMsg->GetId(NETMSG_TEXTMSG));
	WRITE_BYTE(HUD_PRINTTALK);
	WRITE_STRING(string);
	MESSAGE_END();
}

void ClientPrint(edict_t* ent, int dest, const char* format, ...)
{
	va_list ap;
	char string[2048];

	va_start(ap, format);
	vsprintf(string,format, ap);
	va_end(ap);

	if (FNullEnt(ent) || ent == g_hostEntity)
	{
		if (dest & 0x3ff)
			ServerPrint(string);
		else
			ServerPrintNoTag(string);

		return;
	}
	strcat(string, "\n");

	if (dest & 0x3ff)
		(*g_engfuncs.pfnClientPrintf) (ent, static_cast <PRINT_TYPE> (dest &= ~0x3ff), FormatBuffer("[E-BOT] %s", string));
	else
		(*g_engfuncs.pfnClientPrintf) (ent, static_cast <PRINT_TYPE> (dest), string);

}

// this function returns true if server is running under linux, and false otherwise returns windows
bool IsLinux(void)
{
#ifndef PLATFORM_WIN32
	return true;
#else
	return false;
#endif
}

// this function asks the engine to execute a server command
void ServerCommand(const char* format, ...)
{
	va_list ap;
	static char string[1024];

	// concatenate all the arguments in one string
	va_start(ap, format);
	vsprintf(string, format, ap);
	va_end(ap);

	SERVER_COMMAND(FormatBuffer("%s\n", string)); // execute command
}

const char* GetEntityName(edict_t* entity)
{
	static char entityName[256];
	if (FNullEnt(entity))
		strcpy(entityName, "NULL");
	else if (IsValidPlayer(entity))
		strcpy(entityName, (char*)STRING(entity->v.netname));
	else
		strcpy(entityName, (char*)STRING(entity->v.classname));
	return &entityName[0];
}

// this function gets the map name and store it in the map_name global string variable.
const char* GetMapName(void)
{
	static char mapName[256];
	strcpy(mapName, STRING(g_pGlobals->mapname));

	return &mapName[0]; // and return a pointer to it
}

bool OpenConfig(const char* fileName, char* errorIfNotExists, File* outFile)
{
	if (outFile->IsValid())
		outFile->Close();

	outFile->Open(FormatBuffer("%s/addons/ebot/%s", GetModName(), fileName), "rt");

	if (!outFile->IsValid())
	{
		AddLogEntry(LOG_ERROR, errorIfNotExists);
		return false;
	}

	return true;
}

const char* GetWaypointDir(void)
{
	return FormatBuffer("%s/addons/ebot/waypoints/", GetModName());
}

// this function tells the engine that a new server command is being declared, in addition
// to the standard ones, whose name is command_name. The engine is thus supposed to be aware
// that for every "command_name" server command it receives, it should call the function
// pointed to by "function" in order to handle it.
void RegisterCommand(char* command, void funcPtr(void))
{
	if (IsNullString(command) || funcPtr == nullptr)
		return; // reliability check
	REG_SVR_COMMAND(command, funcPtr); // ask the engine to register this new command
}

void CheckWelcomeMessage(void)
{
	static float receiveTime = -1.0f;

	if (receiveTime == -1.0f && IsAlive(g_hostEntity))
	{
		receiveTime = engine->GetTime() + 10.0f;

#if defined(PRODUCT_DEV_VERSION)	
		receiveTime = engine->GetTime() + 10.0f;
#endif
	}

	if (receiveTime > 0.0f && receiveTime < engine->GetTime())
	{
		int buildVersion[4] = { PRODUCT_VERSION_DWORD };
		int bV16[4] = { buildVersion[0], buildVersion[1], buildVersion[2], buildVersion[3] };

		ChartPrint("----- [%s %s] by %s -----", PRODUCT_NAME, PRODUCT_VERSION, PRODUCT_AUTHOR);
		ChartPrint("***** Build: (%u.%u.%u.%u) *****", bV16[0], bV16[1], bV16[2], bV16[3]);

		// the api

		/*
		if (amxxDLL_Version != -1.0 && amxxDLL_Version == float(SUPPORT_API_VERSION_F))
		{
			ChartPrint("***** E-BOT API: Running - Version:%.2f (%u.%u.%u.%u)",
				amxxDLL_Version, amxxDLL_bV16[0], amxxDLL_bV16[1], amxxDLL_bV16[2], amxxDLL_bV16[3]);
		}
		else
			ChartPrint("***** E-BOT API: FAIL *****");*/

		receiveTime = 0.0f;
	}
}

void DetectCSVersion(void)
{
	uint8_t* detection = nullptr;
	const char* const infoBuffer = "Game Registered: CS %s (0x%d)";

	// switch version returned by dll loader
	switch (g_gameVersion)
	{
		// counter-strike 1.x, WON ofcourse
	case CSVER_VERYOLD:
		ServerPrint(infoBuffer, "1.x (WON)", sizeof(Bot));
		break;

		// counter-strike 1.6 or higher (plus detects for non-steam versions of 1.5)
	case CSVER_CSTRIKE:
		detection = (*g_engfuncs.pfnLoadFileForMe) ("events/galil.sc", nullptr);

		if (detection != nullptr)
		{
			ServerPrint(infoBuffer, "1.6 (Steam)", sizeof(Bot));
			g_gameVersion = CSVER_CSTRIKE; // just to be sure
		}
		else if (detection == nullptr)
		{
			ServerPrint(infoBuffer, "1.5 (WON)", sizeof(Bot));
			g_gameVersion = CSVER_VERYOLD; // reset it to WON
		}

		// if we have loaded the file free it
		if (detection != nullptr)
			(*g_engfuncs.pfnFreeFile) (detection);
		break;

		// counter-strike cz
	case CSVER_CZERO:
		ServerPrint(infoBuffer, "CZ (Steam)", sizeof(Bot));
		break;
	}

	engine->GetGameConVarsPointers(); // !!! TODO !!!
}

void PlaySound(edict_t* ent, const char* name)
{
	// TODO: make this obsolete
	EMIT_SOUND_DYN2(ent, CHAN_WEAPON, name, 1.0, ATTN_NORM, 0, 100);
}

// this function logs a message to the message log file root directory.
void AddLogEntry(int logLevel, const char* format, ...)
{
	va_list ap;
	char buffer[512] = { 0, }, levelString[32] = { 0, }, logLine[1024] = { 0, };

	va_start(ap, format);
	vsprintf(buffer, format, ap);
	va_end(ap);

	switch (logLevel)
	{
	case LOG_DEFAULT:
		strcpy(levelString, "Log: ");
		break;

	case LOG_WARNING:
		strcpy(levelString, "Warning: ");
		break;

	case LOG_ERROR:
		strcpy(levelString, "Error: ");
		break;

	case LOG_FATAL:
		strcpy(levelString, "Critical: ");
		break;
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
		int buildVersion[4] = { PRODUCT_VERSION_DWORD };
		for (int i = 0; i < 4; i++)
			mod_bV16[i] = (uint16)buildVersion[i];
	}
	else if (mod == 0)
	{
		sprintf(modName, "EBOT_API");
		for (int i = 0; i < 4; i++)
			mod_bV16[i] = amxxDLL_bV16[i];
	}

	ServerPrintNoTag("[%s Log] %s", modName, format);

	sprintf(buildVersionName, "%s_build_%u_%u_%u_%u.txt", modName,
		mod_bV16[0], mod_bV16[1], mod_bV16[2], mod_bV16[3]);

	File checkLogFP(FormatBuffer("%s/addons/ebot/logs/%s", GetModName(), buildVersionName), "rb");
	File fp(FormatBuffer("%s/addons/ebot/logs/%s", GetModName(), buildVersionName), "at");


	if (!checkLogFP.IsValid())
	{
		fp.Print("---------- %s Log \n", modName);
		fp.Print("---------- %s Version: %u.%u  \n", modName, mod_bV16[0], mod_bV16[1]);
		fp.Print("---------- %s Build: %u.%u.%u.%u  \n", modName,
			mod_bV16[0], mod_bV16[1], mod_bV16[2], mod_bV16[3]);
		fp.Print("----------------------------- \n\n");
	}

	checkLogFP.Close();

	if (!fp.IsValid())
		return;

	time_t tickTime = time(&tickTime);
	tm* time = localtime(&tickTime);

	int buildVersion[4] = { PRODUCT_VERSION_DWORD };
	uint16 bV16[4] = { (uint16)buildVersion[0], (uint16)buildVersion[1], (uint16)buildVersion[2], (uint16)buildVersion[3] };

	sprintf(logLine, "[%02d:%02d:%02d] %s", time->tm_hour, time->tm_min, time->tm_sec, format);
	fp.Print("%s\n", logLine);
	if (mod != -1)
		fp.Print("E-BOT Build: %u.%u.%u.%u  \n", bV16[0], bV16[1], bV16[2], bV16[3]);
	fp.Print("----------------------------- \n");
	fp.Close();
}

// this function finds nearest to to, player with set of parameters, like his
// team, live status, search distance etc. if needBot is true, then pvHolder, will
// be filled with bot pointer, else with edict pointer(!).
bool FindNearestPlayer(void** pvHolder, edict_t* to, float searchDistance, bool sameTeam, bool needBot, bool isAlive, bool needDrawn)
{
	edict_t* ent = nullptr, * survive = nullptr; // pointer to temporaly & survive entity
	float nearestPlayer = 4096.0f; // nearest player

	while (!FNullEnt(ent = FIND_ENTITY_IN_SPHERE(ent, GetEntityOrigin(to), searchDistance)))
	{
		if (FNullEnt(ent) || !IsValidPlayer(ent) || to == ent)
			continue; // skip invalid players

		if ((sameTeam && GetTeam(ent) != GetTeam(to)) || (isAlive && !IsAlive(ent)) || (needBot && !IsValidBot(ent)) || (needDrawn && (ent->v.effects & EF_NODRAW)))
			continue; // filter players with parameters

		float distance = (GetEntityOrigin(ent) - GetEntityOrigin(to)).GetLength();

		if (distance < nearestPlayer)
		{
			nearestPlayer = distance;
			survive = ent;
		}
	}

	if (FNullEnt(survive))
		return false; // nothing found

	 // fill the holder
	if (needBot)
		*pvHolder = reinterpret_cast <void*> (g_botManager->GetBot(survive));
	else
		*pvHolder = reinterpret_cast <void*> (survive);

	return true;
}

// this function called by the sound hooking code (in emit_sound) enters the played sound into
// the array associated with the entity
void SoundAttachToThreat(edict_t* ent, const char* sample, float volume)
{
	if (FNullEnt(ent) || IsNullString(sample))
		return; // reliability check

	Vector origin = GetEntityOrigin(ent);
	int index = ENTINDEX(ent) - 1;

	if (index < 0 || index >= engine->GetMaxClients())
	{
		float nearestDistance = FLT_MAX;

		// loop through all players
		for (int i = 0; i < engine->GetMaxClients(); i++)
		{
			if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE))
				continue;

			float distance = (GetEntityOrigin(g_clients[i].ent) - origin).GetLength();

			// now find nearest player
			if (distance < nearestDistance)
			{
				index = i;
				nearestDistance = distance;
			}
		}
	}

	if (index < 0 || index >= engine->GetMaxClients())
		return;

	if (strncmp("player/bhit_flesh", sample, 17) == 0 || strncmp("player/headshot", sample, 15) == 0)
	{
		// hit/fall sound?
		g_clients[index].hearingDistance = 768.0f * volume;
		g_clients[index].timeSoundLasting = engine->GetTime() + 0.5f;
		g_clients[index].soundPosition = origin;
	}
	else if (strncmp("items/gunpickup", sample, 15) == 0)
	{
		// weapon pickup?
		g_clients[index].hearingDistance = 768.0f * volume;
		g_clients[index].timeSoundLasting = engine->GetTime() + 0.5f;
		g_clients[index].soundPosition = origin;
	}
	else if (strncmp("weapons/zoom", sample, 12) == 0)
	{
		// sniper zooming?
		g_clients[index].hearingDistance = 512.0f * volume;
		g_clients[index].timeSoundLasting = engine->GetTime() + 0.1f;
		g_clients[index].soundPosition = origin;
	}
	else if (strncmp("items/9mmclip", sample, 13) == 0)
	{
		// ammo pickup?
		g_clients[index].hearingDistance = 512.0f * volume;
		g_clients[index].timeSoundLasting = engine->GetTime() + 0.1f;
		g_clients[index].soundPosition = origin;
	}
	else if (strncmp("hostage/hos", sample, 11) == 0)
	{
		// CT used hostage?
		g_clients[index].hearingDistance = 1024.0f * volume;
		g_clients[index].timeSoundLasting = engine->GetTime() + 5.0f;
		g_clients[index].soundPosition = origin;
	}
	else if (strncmp("debris/bustmetal", sample, 16) == 0 || strncmp("debris/bustglass", sample, 16) == 0)
	{
		// broke something?
		g_clients[index].hearingDistance = 1024.0f * volume;
		g_clients[index].timeSoundLasting = engine->GetTime() + 2.0f;
		g_clients[index].soundPosition = origin;
	}
	else if (strncmp("doors/doormove", sample, 14) == 0)
	{
		// someone opened a door
		g_clients[index].hearingDistance = 1024.0f * volume;
		g_clients[index].timeSoundLasting = engine->GetTime() + 3.0f;
		g_clients[index].soundPosition = origin;
	}
}

// this function returning weapon id from the weapon alias and vice versa.
int GetWeaponReturn(bool needString, const char* weaponAlias, int weaponID)
{
	// structure definition for weapon tab
	struct WeaponTab_t
	{
		int weaponID; // weapon id
		const char* alias; // weapon alias
	};

	// weapon enumeration
	WeaponTab_t weaponTab[] =
	{
	   {WEAPON_USP, "usp"}, // HK USP .45 Tactical
	   {WEAPON_GLOCK18, "glock"}, // Glock18 Select Fire
	   {WEAPON_DEAGLE, "deagle"}, // Desert Eagle .50AE
	   {WEAPON_P228, "p228"}, // SIG P228
	   {WEAPON_ELITE, "elite"}, // Dual Beretta 96G Elite
	   {WEAPON_FN57, "fn57"}, // FN Five-Seven
	   {WEAPON_M3, "m3"}, // Benelli M3 Super90
	   {WEAPON_XM1014, "xm1014"}, // Benelli XM1014
	   {WEAPON_MP5, "mp5"}, // HK MP5-Navy
	   {WEAPON_TMP, "tmp"}, // Steyr Tactical Machine Pistol
	   {WEAPON_P90, "p90"}, // FN P90
	   {WEAPON_MAC10, "mac10"}, // Ingram MAC-10
	   {WEAPON_UMP45, "ump45"}, // HK UMP45
	   {WEAPON_AK47, "ak47"}, // Automat Kalashnikov AK-47
	   {WEAPON_GALIL, "galil"}, // IMI Galil
	   {WEAPON_FAMAS, "famas"}, // GIAT FAMAS
	   {WEAPON_SG552, "sg552"}, // Sig SG-552 Commando
	   {WEAPON_M4A1, "m4a1"}, // Colt M4A1 Carbine
	   {WEAPON_AUG, "aug"}, // Steyr Aug
	   {WEAPON_SCOUT, "scout"}, // Steyr Scout
	   {WEAPON_AWP, "awp"}, // AI Arctic Warfare/Magnum
	   {WEAPON_G3SG1, "g3sg1"}, // HK G3/SG-1 Sniper Rifle
	   {WEAPON_SG550, "sg550"}, // Sig SG-550 Sniper
	   {WEAPON_M249, "m249"}, // FN M249 Para
	   {WEAPON_FBGRENADE, "flash"}, // Concussion Grenade
	   {WEAPON_HEGRENADE, "hegren"}, // High-Explosive Grenade
	   {WEAPON_SMGRENADE, "sgren"}, // Smoke Grenade
	   {WEAPON_KEVLAR, "vest"}, // Kevlar Vest
	   {WEAPON_KEVHELM, "vesthelm"}, // Kevlar Vest and Helmet
	   {WEAPON_DEFUSER, "defuser"}, // Defuser Kit
	   {WEAPON_SHIELDGUN, "shield"}, // Tactical Shield
	};

	// if we need to return the string, find by weapon id
	if (needString && weaponID != -1)
	{
		for (int i = 0; i < ARRAYSIZE_HLSDK(weaponTab); i++)
		{
			if (weaponTab[i].weaponID == weaponID) // is weapon id found?
				return MAKE_STRING(weaponTab[i].alias);
		}
		return MAKE_STRING("(none)"); // return none
	}

	// else search weapon by name and return weapon id
	for (int i = 0; i < ARRAYSIZE_HLSDK(weaponTab); i++)
	{
		if (strncmp(weaponTab[i].alias, weaponAlias, strlen(weaponTab[i].alias)) == 0)
			return weaponTab[i].weaponID;
	}

	return -1; // no weapon was found return -1
}

ChatterMessage GetEqualChatter(int message)
{
	ChatterMessage mine = ChatterMessage::Nothing;

	if (message == Radio_Affirmative)
		mine = ChatterMessage::Yes;
	else if (message == Radio_Negative)
		mine = ChatterMessage::No;
	else if (message == Radio_EnemySpotted)
		mine = ChatterMessage::SeeksEnemy;
	else if (message == Radio_NeedBackup)
		mine = ChatterMessage::SeeksEnemy;
	else if (message == Radio_TakingFire)
		mine = ChatterMessage::SeeksEnemy;
	else if (message == Radio_CoverMe)
		mine = ChatterMessage::CoverMe;
	else if (message == Radio_SectorClear)
		mine = ChatterMessage::Clear;

	return mine;
}

void GetVoiceAndDur(ChatterMessage message, char* *voice, float *dur)
{
	if (message == ChatterMessage::Yes)
	{
		int rV = engine->RandomInt(1, 12);
		if (rV == 1)
		{
			*voice = "affirmative";
			*dur = 0.0f;
		}
		else if (rV == 2)
		{
			*voice = "alright";
			*dur = 0.0f;
		}
		else if (rV == 3)
		{
			*voice = "alright_lets_do_this";
			*dur = 1.0f;
		}
		else if (rV == 4)
		{
			*voice = "alright2";
			*dur = 0.0f;
		}
		else if (rV == 5)
		{
			*voice = "ok";
			*dur = 0.0f;
		}
		else if (rV == 6)
		{
			*voice = "ok_sir_lets_go";
			*dur = 1.0f;
		}
		else if (rV == 7)
		{
			*voice = "ok_cmdr_lets_go";
			*dur = 1.0f;
		}
		else if (rV == 8)
		{
			*voice = "ok2";
			*dur = 0.0f;
		}
		else if (rV == 9)
		{
			*voice = "roger";
			*dur = 0.0f;
		}
		else if (rV == 10)
		{
			*voice = "roger_that";
			*dur = 0.0f;
		}
		else if (rV == 11)
		{
			*voice = "yea_ok";
			*dur = 0.0f;
		}
		else
		{
			*voice = "you_heard_the_man_lets_go";
			*dur = 1.0f;
		}
	}
	else if (message == ChatterMessage::No)
	{
		int rV = engine->RandomInt(1, 13);
		if (rV == 1)
		{
			*voice = "ahh_negative";
			*dur = 1.0f;
		}
		else if (rV == 2)
		{
			*voice = "negative";
			*dur = 0.0f;
		}
		else if (rV == 3)
		{
			*voice = "negative2";
			*dur = 1.0f;
		}
		else if (rV == 4)
		{
			*voice = "no";
			*dur = 0.0f;
		}
		else if (rV == 5)
		{
			*voice = "ok";
			*dur = 0.0f;
		}
		else if (rV == 6)
		{
			*voice = "no_sir";
			*dur = 0.0f;
		}
		else if (rV == 7)
		{
			*voice = "no_thanks";
			*dur = 0.0f;
		}
		else if (rV == 8)
		{
			*voice = "no2";
			*dur = 0.0f;
		}
		else if (rV == 9)
		{
			*voice = "naa";
			*dur = 0.0f;
		}
		else if (rV == 10)
		{
			*voice = "nnno_sir";
			*dur = 0.1f;
		}
		else if (rV == 11)
		{
			*voice = "hes_broken";
			*dur = 0.1f;
		}
		else if (rV == 12)
		{
			*voice = "i_dont_think_so";
			*dur = 0.0f;
		}
		else
		{
			*voice = "noo";
			*dur = 0.0f;
		}
	}
	else if (message == ChatterMessage::SeeksEnemy)
	{
		int rV = engine->RandomInt(1, 15);
		if (rV == 1)
		{
			*voice = "help";
			*dur = 0.0f;
		}
		else if (rV == 2)
		{
			*voice = "need_help";
			*dur = 0.0f;
		}
		else if (rV == 3)
		{
			*voice = "need_help2";
			*dur = 0.0f;
		}
		else if (rV == 4)
		{
			*voice = "taking_fire_need_assistance2";
			*dur = 1.0f;
		}
		else if (rV == 5)
		{
			*voice = "engaging_enemies";
			*dur = 0.8f;
		}
		else if (rV == 6)
		{
			*voice = "attacking";
			*dur = 0.0f;
		}
		else if (rV == 7)
		{
			*voice = "attacking_enemies";
			*dur = 1.0f;
		}
		else if (rV == 8)
		{
			*voice = "a_bunch_of_them";
			*dur = 0.0f;
		}
		else if (rV == 9)
		{
			*voice = "im_pinned_down";
			*dur = 0.25f;
		}
		else if (rV == 10)
		{
			*voice = "im_in_trouble";
			*dur = 1.0f;
		}
		else if (rV == 11)
		{
			*voice = "in_combat";
			*dur = 0.0f;
		}
		else if (rV == 12)
		{
			*voice = "in_combat2";
			*dur = 0.0f;
		}
		else if (rV == 13)
		{
			*voice = "target_acquired";
			*dur = 0.0f;
		}
		else if (rV == 14)
		{
			*voice = "target_spotted";
			*dur = 0.0f;
		}
		else
		{
			*voice = "i_see_our_target";
			*dur = 1.0f;
		}
	}
	else if (message == ChatterMessage::Clear)
	{
		int rV = engine->RandomInt(1, 17);
		if (rV == 1)
		{
			*voice = "clear";
			*dur = 0.0f;
		}
		else if (rV == 2)
		{
			*voice = "clear2";
			*dur = 0.0f;
		}
		else if (rV == 3)
		{
			*voice = "clear3";
			*dur = 0.0f;
		}
		else if (rV == 4)
		{
			*voice = "clear4";
			*dur = 1.0f;
		}
		else if (rV == 5)
		{
			*voice = "where_are_you_hiding";
			*dur = 2.0f;
		}
		else if (rV == 6)
		{
			*voice = "where_could_they_be";
			*dur = 0.0f;

			for (int i = 0; i < engine->GetMaxClients(); i++)
			{
				Bot* otherBot = g_botManager->GetBot(i);
				if (otherBot != nullptr)
					otherBot->m_radioOrder = Radio_ReportTeam;
			}
		}
		else if (rV == 7)
		{
			*voice = "where_is_it";
			*dur = 0.4f;

			for (int i = 0; i < engine->GetMaxClients(); i++)
			{
				Bot* otherBot = g_botManager->GetBot(i);
				if (otherBot != nullptr)
					otherBot->m_radioOrder = Radio_ReportTeam;
			}
		}
		else if (rV == 8)
		{
			*voice = "area_clear";
			*dur = 0.0f;
		}
		else if (rV == 9)
		{
			*voice = "area_secure";
			*dur = 0.0f;
		}
		else if (rV == 10)
		{
			*voice = "anyone_see_anything";
			*dur = 1.0f;

			for (int i = 0; i < engine->GetMaxClients(); i++)
			{
				Bot* otherBot = g_botManager->GetBot(i);
				if (otherBot != nullptr)
					otherBot->m_radioOrder = Radio_ReportTeam;
			}
		}
		else if (rV == 11)
		{
			*voice = "all_clear_here";
			*dur = 1.0f;
		}
		else if (rV == 12)
		{
			*voice = "all_quiet";
			*dur = 1.0f;
		}
		else if (rV == 13)
		{
			*voice = "nothing";
			*dur = 0.7f;
		}
		else if (rV == 14)
		{
			*voice = "nothing_happening_over_here";
			*dur = 1.0f;
		}
		else if (rV == 15)
		{
			*voice = "nothing_here";
			*dur = 0.0f;
		}
		else if (rV == 16)
		{
			*voice = "nothing_moving_over_here";
			*dur = 1.0f;
		}
		else
		{
			*voice = "anyone_see_them";
			*dur = 0.0f;

			for (int i = 0; i < engine->GetMaxClients(); i++)
			{
				Bot* otherBot = g_botManager->GetBot(i);
				if (otherBot != nullptr)
					otherBot->m_radioOrder = Radio_ReportTeam;
			}
		}
	}
	else if (message == ChatterMessage::CoverMe)
	{
		int rV = engine->RandomInt(1, 2);
		if (rV == 1)
		{
			*voice = "cover_me";
			*dur = 0.0f;
		}
		else
		{
			*voice = "cover_me2";
			*dur = 0.0f;
		}
	}
	else if (message == ChatterMessage::Happy)
	{
		int rV = engine->RandomInt(1, 10);
		if (rV == 1)
		{
			*voice = "yea_baby";
			*dur = 0.0f;
		}
		else if (rV == 2)
		{
			*voice = "whos_the_man";
			*dur = 0.0f;
		}
		else if (rV == 3)
		{
			*voice = "who_wants_some_more";
			*dur = 1.0f;
		}
		else if (rV == 4)
		{
			*voice = "yikes";
			*dur = 0.0f;
		}
		else if (rV == 5)
		{
			*voice = "yesss";
			*dur = 1.0f;
		}
		else if (rV == 6)
		{
			*voice = "yesss2";
			*dur = 0.0f;
		}
		else if (rV == 7)
		{
			*voice = "whoo";
			*dur = 0.0f;
		}
		else if (rV == 8)
		{
			*voice = "i_am_dangerous";
			*dur = 1.0f;
		}
		else if (rV == 9)
		{
			*voice = "i_am_on_fire";
			*dur = 1.0f;
		}
		else
		{
			*voice = "whoo2";
			*dur = 0.5f;
		}
	}
}