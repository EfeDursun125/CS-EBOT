#include <core.h>
#include <exception>

//
// TODO:
// clean up the code.
// create classes: Tracer, PrintManager, GameManager
//

ConVar ebot_ignore_enemies("ebot_ignore_enemies", "0");
ConVar ebot_zp_delay_custom("ebot_zp_delay_custom", "0.0");

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
	int output = (static_cast<int>(value * scale));

	if (output < 0)
		output = 0;

	if (output > 0xffff)
		output = 0xffff;

	return static_cast<uint16>(output);
}

short FixedSigned16(float value, float scale)
{
	int output = (static_cast<int>(value * scale));

	if (output > 32767)
		output = 32767;

	if (output < -32768)
		output = -32768;

	return static_cast<short>(output);
}

bool IsAlive(const edict_t* ent)
{
	if (FNullEnt(ent))
		return false; // reliability check

	return (ent->v.deadflag == DEAD_NO) && (ent->v.health > 0) && (ent->v.movetype != MOVETYPE_NOCLIP);
}

float GetShootingConeDeviation(edict_t* ent, const Vector* position)
{
	if (FNullEnt(ent))
		return 0.0f;

	const Vector& dir = (*position - (GetEntityOrigin(ent) + ent->v.view_ofs)).Normalize();
	MakeVectors(ent->v.v_angle);

	// he's facing it, he meant it
	return g_pGlobals->v_forward | dir;
}

float DotProduct(const Vector& a, const Vector& b)
{
	return (a.x * b.x + a.y * b.y + a.z * b.z);
}

float DotProduct2D(const Vector& a, const Vector& b)
{
	return (a.x * b.x + a.y * b.y);
}

Vector CrossProduct(const Vector& a, const Vector& b)
{
	return Vector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

bool IsInViewCone(const Vector& origin, edict_t* ent)
{
	if (FNullEnt(ent))
		return true;

	MakeVectors(ent->v.v_angle);

	if (((origin - (GetEntityOrigin(ent) + ent->v.view_ofs)).Normalize() | g_pGlobals->v_forward) >= cosf(((ent->v.fov > 0 ? ent->v.fov : 90.0f) * 0.5f) * 0.0174532925f))
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

// return walkable position on ground
Vector GetWalkablePosition(const Vector& origin, edict_t* ent, bool returnNullVec, float height)
{
	TraceResult tr;
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

		while (!g_isFakeCommand && !g_isMessage && cstrlen(text) >= 64)
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

		if (!g_isFakeCommand && !g_isMessage)
		{
			MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, g_netMsg->GetId(NETMSG_SHOWMENU), nullptr, ent);
			WRITE_SHORT(menu->validSlots);
			WRITE_CHAR(-1);
			WRITE_BYTE(0);
			WRITE_STRING(text);
			MESSAGE_END();
		}

		g_clients[clientIndex].menu = menu;
	}
	else if (!g_isFakeCommand && !g_isMessage)
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
	int i;
	if (index == -1)
	{
		for (i = 0; i < entityNum; i++)
			SetEntityActionData(i);
		return 1;
	}

	edict_t* entity = INDEXENT(index);
	if (FNullEnt(entity) || !IsAlive(entity))
		return -1;

	if (IsValidPlayer(entity))
		return -1;

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

	// someone is using it :(
	if (g_isFakeCommand)
		return;

	if (g_isMessage)
		return;

	if (!IsValidBot(fakeClient))
		return;

	va_list ap;
	static char command[256];
	int stop, i, stringIndex = 0;

	va_start(ap, format);
	vsnprintf(command, sizeof(command), format, ap);
	va_end(ap);

	if (IsNullString(command))
		return;

	g_isFakeCommand = true;
	const int length = cstrlen(command);

	while (stringIndex < length)
	{
		const int start = stringIndex;

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

		int index = 0;
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

	return (&field[0]); // returns the wanted field
}

const char* GetModName(void)
{
	static char modName[256];

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
		if (bot == nullptr)
			continue;

		bot->NewRound();
		g_radioSelect[bot->m_index - 1] = 0;
	}

	g_bombSayString = false;
	g_timeBombPlanted = 0.0f;

	g_lastRadioTime[0] = 0.0f;
	g_lastRadioTime[1] = 0.0f;

	g_waypoint->SetBombPosition(true);

	if (g_gameVersion != HALFLIFE)
	{
		AutoLoadGameMode();

		if (GetGameMode() == MODE_BASE)
		{
			// check team economics
			g_botManager->CheckTeamEconomics(TEAM_TERRORIST);
			g_botManager->CheckTeamEconomics(TEAM_COUNTER);
		}

		// calculate the round mid/end in world time
		g_timeRoundStart = AddTime(engine->GetFreezeTime() + g_pGlobals->frametime);
		g_timeRoundMid = g_timeRoundStart + engine->GetRoundTime() * 30.0f;
		g_timeRoundEnd = g_timeRoundStart + engine->GetRoundTime() * 60.0f;

		g_botManager->SelectLeaderEachTeam(TEAM_COUNTER);
		g_botManager->SelectLeaderEachTeam(TEAM_COUNTER);
	}
	else
	{
		g_timeRoundStart = engine->GetTime();
		g_timeRoundMid = g_timeRoundStart + 999999.0f;
		g_timeRoundEnd = g_timeRoundStart + 999999.0f;
		SetGameMode(MODE_DM);
	}
}

void AutoLoadGameMode(void)
{
	if (!g_isMetamod)
		return;

	const char* getModeName = GetModName();

	static int checkShowTextTime = 0;
	checkShowTextTime++;

	// CS:BTE Support 
	char* Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/bte_player.ini", getModeName);
	if (TryFileOpen(Plugin_INI) || TryFileOpen(FormatBuffer("%s/addons/amxmodx/configs/bte_config/bte_blockresource.txt", getModeName)))
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
			if (TryFileOpen(FormatBuffer("%s/addons/amxmodx/configs/%s.ini", getModeName, bteGameINI[i])))
			{
				if (bteGameModAi[i] == 2 && i != 5)
					g_DelayTimer = AddTime(20.0f + CVAR_GET_FLOAT("mp_freezetime"));

				if (checkShowTextTime < 3 || GetGameMode() != bteGameModAi[i])
					ServerPrint("*** E-BOT Auto Game Mode Setting: CS:BTE [%s] [%d] ***", bteGameINI[i], bteGameModAi[i]);

				if (i == 3 || i == 9)
				{
					ServerPrint("***** E-BOT not support the mode now :( *****");
					SetGameMode(MODE_TDM);
				}
				else
					SetGameMode(bteGameModAi[i]);

				g_gameVersion = CSVER_CZERO;

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
		SetGameMode(MODE_ZP);
		g_DelayTimer = AddTime(ebot_zp_delay_custom.GetFloat() + 2.22f);
		g_mapType |= MAP_DE;
		return;
	}
	else
	{
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
			Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/%s.ini", getModeName, zpGameVersion[i]);
			if (TryFileOpen(Plugin_INI))
			{
				float delayTime = CVAR_GET_FLOAT("zp_delay") + 2.2f;

				if (i == 1 || i == 2)
					delayTime = CVAR_GET_FLOAT("zp_gamemode_delay") + 0.2f;

				if (delayTime > 0.0f)
				{
					if (checkShowTextTime < 3 || GetGameMode() != MODE_ZP)
						ServerPrint("*** E-BOT Auto Game Mode Setting: Zombie Mode (Plague/Escape) ***");

					// zombie escape
					if (g_mapType & MAP_ZE)
					{
						extern ConVar ebot_escape;
						ebot_escape.SetInt(1);
						ServerPrint("*** E-BOT Detected Zombie Escape Map: ebot_zombie_escape_mode is set to 1 ***");
					}

					SetGameMode(MODE_ZP);
					g_DelayTimer = AddTime(delayTime);
					g_mapType |= MAP_DE;
					return;
				}
			}
		}
	}

	// Base Builder
	char* bbVersion[] =
	{
		"plugins-basebuilder",
		"plugins-bb"
	};

	for (int i = 0; i < 2; i++)
	{
		Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/%s.ini", getModeName, bbVersion[i]);
		if (TryFileOpen(Plugin_INI))
		{
			const float delayTime = CVAR_GET_FLOAT("bb_buildtime") + CVAR_GET_FLOAT("bb_preptime") + 2.2f;
			if (delayTime > 0)
			{
				if (checkShowTextTime < 3 || GetGameMode() != MODE_ZP)
					ServerPrint("*** E-BOT Auto Game Mode Setting: Zombie Mode (Base Builder) ***");

				SetGameMode(MODE_ZP);

				g_DelayTimer = AddTime(delayTime);
				g_mapType |= MAP_DE;
				return;
			}
		}
	}

	// DM:KD
	Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/plugins-dmkd.ini", getModeName);
	if (TryFileOpen(Plugin_INI))
	{
		if (CVAR_GET_FLOAT("DMKD_DMMODE") == 1)
		{
			if (checkShowTextTime < 3 || GetGameMode() != MODE_DM)
				ServerPrint("*** E-BOT Auto Game Mode Setting: DM:KD-DM ***");

			SetGameMode(MODE_DM);
		}
		else
		{
			if (checkShowTextTime < 3 || GetGameMode() != MODE_TDM)
				ServerPrint("*** E-BOT Auto Game Mode Setting: DM:KD-TDM ***");

			SetGameMode(MODE_TDM);
		}

		g_mapType |= MAP_DE;
		return;
	}

	// Zombie Hell
	Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/zombiehell.cfg", getModeName);
	if (TryFileOpen(Plugin_INI) && CVAR_GET_FLOAT("zh_zombie_maxslots") > 0)
	{
		if (checkShowTextTime < 3 || GetGameMode() != MODE_ZH)
			ServerPrint("*** E-BOT Auto Game Mode Setting: Zombie Hell ***");

		SetGameMode(MODE_ZH);

		extern ConVar ebot_quota;
		ebot_quota.SetInt(static_cast<int>(CVAR_GET_FLOAT("zh_zombie_maxslots")));
		g_mapType |= MAP_DE;
		return;
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
		Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/%s.ini", getModeName, biohazard[i]);
		if (TryFileOpen(Plugin_INI))
		{
			const float delayTime = CVAR_GET_FLOAT("bh_starttime") + 0.5f;
			if (delayTime > 0)
			{
				if (checkShowTextTime < 3 || GetGameMode() != MODE_ZP)
					ServerPrint("*** E-BOT Auto Game Mode Setting: Zombie Mode (Biohazard) ***");

				SetGameMode(MODE_ZP);

				g_DelayTimer = AddTime(delayTime);
				g_mapType |= MAP_DE;
				return;
			}
		}
	}

	auto dmActive = g_engfuncs.pfnCVarGetPointer("csdm_active");
	if (dmActive != nullptr && dmActive->value > 0.0f)
	{
		auto freeForAll = g_engfuncs.pfnCVarGetPointer("mp_freeforall");
		if (freeForAll != nullptr && freeForAll->value > 0.0f)
		{
			if (checkShowTextTime < 3 || GetGameMode() != MODE_DM)
				ServerPrint("*** E-BOT Auto Game Mode Setting: CSDM-DM ***");

			SetGameMode(MODE_DM);
			g_mapType |= MAP_DE;
			return;
		}
	}

	if (checkShowTextTime < 3)
	{
		if (GetGameMode() == MODE_BASE)
			ServerPrint("*** E-BOT Auto Game Mode Setting: Base Mode ***");
		else
			ServerPrint("*** E-BOT Auto Game Mode Setting: N/A ***");
	}
}
// returns if weapon can pierce through a wall
bool IsWeaponShootingThroughWall(int id)
{
	if (g_gameVersion == HALFLIFE)
		return false;

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

void SetGameMode(int gamemode)
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

bool IsValidWaypoint(const int index)
{
	if (index < 0 || index >= g_numWaypoints)
		return false;

	return true;
}

int GetGameMode(void)
{
	return ebot_gamemod.GetInt();
}

bool IsBreakable(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	extern ConVar ebot_breakable_health_limit;
	if ((FClassnameIs(ent, "func_breakable") || (FClassnameIs(ent, "func_pushable") && (ent->v.spawnflags & SF_PUSH_BREAKABLE)) || FClassnameIs(ent, "func_wall")) && ent->v.health <= ebot_breakable_health_limit.GetFloat())
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
		return TEAM_COUNT;

	int player_team = TEAM_COUNT;
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

	if (ebot_ignore_enemies.GetBool())
		player_team = TEAM_COUNTER;
	else if (GetGameMode() == MODE_ZP)
	{
		if (g_DelayTimer >= engine->GetTime())
			player_team = TEAM_COUNTER;
		else if (g_roundEnded)
			player_team = TEAM_TERRORIST;
		else
			player_team = *((int*)ent->pvPrivateData + OFFSET_TEAM) - 1;
	}
	else if (GetGameMode() == MODE_DM || GetGameMode() == MODE_NOTEAM)
	{
		const int client = ENTINDEX(ent);
		player_team = client * client;
	}
	else
		player_team = *((int*)ent->pvPrivateData + OFFSET_TEAM) - 1;

	return player_team;
}

int SetEntityWaypoint(edict_t* ent, int mode)
{
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
			float distance = (getWpOrigin - origin).GetLengthSquared();
			if (distance >= SquaredF(300.0f))
				needCheckNewWaypoint = true;
			else if (distance >= SquaredF(32.0f))
			{
				const Path* pointer = g_waypoint->GetPath(wpIndex);
				const Vector wpOrigin = pointer->origin;
				distance = (wpOrigin - origin).GetLengthSquared();

				if (distance > SquaredI(pointer->radius + 32))
					needCheckNewWaypoint = true;
				else
				{
					if (traceCheckTime + 5.0f <= engine->GetTime() || (traceCheckTime + 2.5f <= engine->GetTime() && !g_waypoint->Reachable(ent, wpIndex)))
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
		wpIndex = g_waypoint->FindNearest(origin, 999999.0f, -1, ent);
	else
		wpIndex = g_waypoint->FindNearest(origin, 999999.0f, -1, ent, &wpIndex2, mode);

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

		return g_waypoint->FindNearest(GetEntityOrigin(ent), 999999.0f, -1, ent);
	}

	const int client = ENTINDEX(ent) - 1;
	if (g_clients[client].getWPTime < AddTime(1.25f) || (g_clients[client].wpIndex == -1 && g_clients[client].wpIndex2 == -1))
		SetEntityWaypoint(ent);

	return g_clients[client].wpIndex;
}

bool IsZombieEntity(edict_t* ent)
{
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

bool IsValidBot(int index)
{
	if (g_botManager->GetIndex(index) != -1)
		return true;

	return false;
}

bool IsEntityWalkable(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	if (FClassnameIs(ent, "func_door") || FClassnameIs(ent, "func_door_rotating"))
		return true;
	else if (FClassnameIs(ent, "func_breakable") && ent->v.takedamage == DAMAGE_YES)
		return true;

	return false;
}

bool IsWalkableTraceLineClear(const Vector from, const Vector to)
{
	TraceResult result;
	edict_t* pEntIgnore = nullptr;
	Vector useFrom = from;

	while (true)
	{
		TraceLine(useFrom, to, true, pEntIgnore, &result);

		// if we hit a walkable entity, try again
		if (result.flFraction != 1.0f && IsEntityWalkable(result.pHit))
		{
			pEntIgnore = result.pHit;

			// start from just beyond where we hit to avoid infinite loops
			Vector dir = to - from;
			dir.NormalizeInPlace();
			useFrom = result.vecEndPos + 5.0f * dir;
		}
		else
			break;
	}

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
	
	if (!g_isFakeCommand && !g_isMessage)
	{
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
		WRITE_BYTE(static_cast<int>(rgb.red));
		WRITE_BYTE(static_cast<int>(rgb.green));
		WRITE_BYTE(static_cast<int>(rgb.blue));
		WRITE_BYTE(0);
		WRITE_BYTE(CRandomInt(230, 255));
		WRITE_BYTE(CRandomInt(230, 255));
		WRITE_BYTE(CRandomInt(230, 255));
		WRITE_BYTE(200);
		WRITE_SHORT(FixedUnsigned16(0.0078125, 1 << 8));
		WRITE_SHORT(FixedUnsigned16(2, 1 << 8));
		WRITE_SHORT(FixedUnsigned16(6, 1 << 8));
		WRITE_SHORT(FixedUnsigned16(0.1f, 1 << 8));
		WRITE_STRING(const_cast<const char*>(&buffer[0]));
		MESSAGE_END();
	}
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

	if (!g_isFakeCommand && !g_isMessage)
	{
		MESSAGE_BEGIN(MSG_BROADCAST, g_netMsg->GetId(NETMSG_TEXTMSG));
		WRITE_BYTE(HUD_PRINTCENTER);
		WRITE_STRING(FormatBuffer("%s\n", string));
		MESSAGE_END();
	}
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

	if (!g_isFakeCommand && !g_isMessage)
	{
		cstrcat(string, "\n");

		MESSAGE_BEGIN(MSG_BROADCAST, g_netMsg->GetId(NETMSG_TEXTMSG));
		WRITE_BYTE(HUD_PRINTTALK);
		WRITE_STRING(string);
		MESSAGE_END();
	}
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
		cstrcpy(entityName, "nullptr");
	else if (IsValidPlayer(entity))
		cstrcpy(entityName, (char*)STRING(entity->v.netname));
	else
		cstrcpy(entityName, (char*)STRING(entity->v.classname));
	return &entityName[0];
}

// this function gets the map name and store it in the map_name global string variable.
const char* GetMapName(void)
{
	static char mapName[256];
	cstrcpy(mapName, STRING(g_pGlobals->mapname));
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
		receiveTime = AddTime(12.5f);

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
	if (g_engfuncs.pfnCVarGetPointer("host_ver") != nullptr)
		g_gameVersion = CSVER_XASH;

	// switch version returned by dll loader
	if (g_gameVersion == CSVER_XASH)
		ServerPrint(infoBuffer, "Xash Engine", sizeof(Bot));
	else if (g_gameVersion == CSVER_VERYOLD)
		ServerPrint(infoBuffer, "CS 1.x (WON)", sizeof(Bot));
	else if (g_gameVersion == CSVER_CZERO)
		ServerPrint(infoBuffer, "CS: CZ (Steam)", sizeof(Bot));
	else if (g_gameVersion == HALFLIFE)
		ServerPrint(infoBuffer, "Half-Life", sizeof(Bot));
	else if (g_gameVersion == CSVER_CSTRIKE)
	{
		uint8_t* detection = (*g_engfuncs.pfnLoadFileForMe) ("events/galil.sc", nullptr);

		if (detection != nullptr)
		{
			ServerPrint(infoBuffer, "CS 1.6 (Steam)", sizeof(Bot));
			g_gameVersion = CSVER_CSTRIKE; // just to be sure
		}
		else if (detection == nullptr)
		{
			ServerPrint(infoBuffer, "CS 1.5 (WON)", sizeof(Bot));
			g_gameVersion = CSVER_VERYOLD; // reset it to WON
		}

		// if we have loaded the file free it
		if (detection != nullptr)
			(*g_engfuncs.pfnFreeFile) (detection);
	}

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
		cstrcpy(levelString, "Log: ");
		break;

	case LOG_WARNING:
		cstrcpy(levelString, "Warning: ");
		break;

	case LOG_ERROR:
		cstrcpy(levelString, "Error: ");
		break;

	case LOG_FATAL:
		cstrcpy(levelString, "Critical: ");
		break;

	case LOG_MEMORY:
		cstrcpy(levelString, "Memory Error: ");
		ServerPrint("unexpected memory error");
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
		const int buildVersion[4] = {PRODUCT_VERSION_DWORD};
		for (int i = 0; i < 4; i++)
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
		fp.Print("---------- %s Log \n", modName);
		fp.Print("---------- %s Version: %u.%u  \n", modName, mod_bV16[0], mod_bV16[1]);
		fp.Print("---------- %s Build: %u.%u.%u.%u  \n", modName, mod_bV16[0], mod_bV16[1], mod_bV16[2], mod_bV16[3]);
		fp.Print("----------------------------- \n\n");
	}

	checkLogFP.Close();

	if (!fp.IsValid())
		return;

	time_t tickTime = time(&tickTime);
	tm* time = localtime(&tickTime);

	sprintf(logLine, "[%02d:%02d:%02d] %s", time->tm_hour, time->tm_min, time->tm_sec, format);
	fp.Print("%s\n", logLine);

	if (mod != -1)
		fp.Print("E-BOT Build: %d \n", PRODUCT_VERSION);

	fp.Print("----------------------------- \n");
	fp.Close();
}

// this function finds nearest to to, player with set of parameters, like his
// team, live status, search distance etc. if needBot is true, then pvHolder, will
// be filled with bot pointer, else with edict pointer(!).
bool FindNearestPlayer(void** pvHolder, edict_t* to, float searchDistance, bool sameTeam, bool needBot, bool isAlive, bool needDrawn)
{
	edict_t* survive = nullptr; // pointer to temporaly & survive entity
	float nearestPlayer = FLT_MAX; // nearest player

	const Vector toOrigin = GetEntityOrigin(to);

	for (const auto& client : g_clients)
	{
		if (FNullEnt(client.ent))
			continue;

		if (client.ent == to)
			continue;

		if ((sameTeam && client.team != GetTeam(to)) || (isAlive && !IsAlive(client.ent)) || (needBot && !IsValidBot(client.index)) || (needDrawn && (client.ent->v.effects & EF_NODRAW)))
			continue; // filter players with parameters

		const float distance = (client.ent->v.origin - toOrigin).GetLengthSquared();
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
		*pvHolder = reinterpret_cast <void*> (g_botManager->GetBot(survive));
	else
		*pvHolder = reinterpret_cast <void*> (survive);

	return true;
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
		if (cstrncmp(weaponTab[i].alias, weaponAlias, cstrlen(weaponTab[i].alias)) == 0)
			return weaponTab[i].weaponID;
	}

	return -1; // no weapon was found return -1
}