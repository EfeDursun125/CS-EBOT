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

//
// TODO:
// clean up the code.
// create classes: Tracer, PrintManager, GameManager
//

ConVar ebot_ignore_enemies("ebot_ignore_enemies", "0");
ConVar ebot_zp_delay_custom("ebot_zp_delay_custom", "0.0");

void TraceLine(const Vector& start, const Vector& end, const int& ignoreFlags, edict_t* ignoreEntity, TraceResult* ptr)
{
	// this function traces a line dot by dot, starting from vecStart in the direction of vecEnd,
	// ignoring or not monsters (depending on the value of IGNORE_MONSTERS, true or false), and stops
	// at the first obstacle encountered, returning the results of the trace in the TraceResult structure
	// ptr. Such results are (amongst others) the distance traced, the hit surface, the hit plane
	// vector normal, etc. See the TraceResult structure for details. This function allows to specify
	// whether the trace starts "inside" an entity's polygonal model, and if so, to specify that entity
	// in ignoreEntity in order to ignore it as a possible obstacle.
	// this is an overloaded prototype to add IGNORE_GLASS in the same way as IGNORE_MONSTERS work.

	int engineFlags = 0;
	if (ignoreFlags & TraceIgnore::Monsters)
		engineFlags = 1;

	if (ignoreFlags & TraceIgnore::Glass)
		engineFlags |= 0x100;

	(*g_engfuncs.pfnTraceLine) (start, end, engineFlags, ignoreEntity, ptr);
}

void TraceHull(const Vector& start, const Vector& end, const int& ignoreFlags, const int& hullNumber, edict_t* ignoreEntity, TraceResult* ptr)
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

	(*g_engfuncs.pfnTraceHull) (start, end, !!(ignoreFlags & TraceIgnore::Monsters), hullNumber, ignoreEntity, ptr);
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

	return !ent->v.deadflag && ent->v.health > 0.0f && ent->v.movetype != MOVETYPE_NOCLIP;
}

float GetShootingConeDeviation(edict_t* ent, const Vector& position)
{
	if (FNullEnt(ent))
		return 0.0f;

	MakeVectors(ent->v.v_angle);
	return g_pGlobals->v_forward | (position - (GetEntityOrigin(ent) + ent->v.view_ofs)).Normalize();
}

bool IsInViewCone(const Vector& origin, edict_t* ent)
{
	if (FNullEnt(ent))
		return true;

	MakeVectors(ent->v.v_angle);
	if (((origin - (GetEntityOrigin(ent) + ent->v.view_ofs)).Normalize() | g_pGlobals->v_forward) >= ccosf(((ent->v.fov > 0.0f ? ent->v.fov : 91.0f) * 0.51f) * 0.0174532925f))
		return true;

	return false;
}

bool IsVisible(const Vector& origin, edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	TraceResult tr;
	TraceLine(GetEntityOrigin(ent), origin, TraceIgnore::Everything, ent, &tr);

	if (tr.flFraction < 1.0f)
		return false; // line of sight is not established

	return true; // line of sight is valid.
}

// return walkable position on ground
Vector GetWalkablePosition(const Vector& origin, edict_t* ent, const bool returnNullVec, const float height)
{
	TraceResult tr;
	TraceLine(origin, Vector(origin.x, origin.y, -height), TraceIgnore::Monsters, ent, &tr);

	if (tr.flFraction < 1.0f)
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
		entityOrigin = ent->v.absmin + (ent->v.size * 0.5f);

	return entityOrigin;
}

Vector GetBoxOrigin(edict_t* ent)
{
	if (FNullEnt(ent))
		return nullvec;

	return ent->v.absmin + (ent->v.size * 0.5f);
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

int Find(char* buffer, const char chr, const int startIndex)
{
	if (!buffer)
		return -1;

    if (startIndex < 0 || startIndex >= cstrlen(buffer))
        return -1;

    char* ptr = buffer + startIndex;
    for (; *ptr != '\0'; ++ptr)
    {
        if (*ptr == chr)
            return static_cast<int>(ptr - buffer);
    }

    return -1;
}

int Replace(char* buffer, const char oldChar, const char newChar)
{
    if (!buffer)
		return 0;

    if (oldChar == newChar)
		return 0;

    int number = 0;
    int pos = 0;
    while ((pos = Find(buffer, oldChar, pos)) >= 0 && buffer[pos])
    {
        buffer[pos] = newChar;
        pos++;
        number++;
    }

    return number;
}

int Find2(char* buffer, const char* str, const int startIndex)
{
	if (!buffer)
		return -1;

    if (!str || !cstrlen(str))
		return -1;

    if (startIndex < 0 || startIndex >= cstrlen(buffer))
		return -1;

    char* ptr = buffer + startIndex;
    int strLen = cstrlen(str);
    for (; *ptr != '\0'; ++ptr)
    {
        if (cstrncmp(ptr, str, strLen) == 0)
            return static_cast<int>(ptr - buffer);
    }

    return -1;
}

int Replace2(char* buffer, const char* oldStr, const char* newStr)
{
    if (!buffer || !oldStr || !newStr)
		return 0;

    if (cstrcmp(oldStr, newStr) == 0)
		return 0;

    int number = 0;
    int pos = 0;
    int oldStrLen = cstrlen(oldStr);
    int newStrLen = cstrlen(newStr);
    while ((pos = Find2(buffer, oldStr, pos)) >= 0)
    {
        if (newStrLen <= oldStrLen)
        {
            cmemcpy(buffer + pos, newStr, newStrLen);
            if (newStrLen < oldStrLen)
                cmemmove(buffer + pos + newStrLen, buffer + pos + oldStrLen, cstrlen(buffer + pos + oldStrLen) + 1);
        }
        else
        {
            cmemmove(buffer + pos + newStrLen, buffer + pos + oldStrLen, cstrlen(buffer + pos + oldStrLen) + 1);
            cmemcpy(buffer + pos, newStr, newStrLen);
        }

        pos += newStrLen;
        number++;
    }

    return number;
}

void DisplayMenuToClient(edict_t* ent, MenuText* menu)
{
	if (!IsValidPlayer(ent) || IsValidBot(ent))
		return;

	const int clientIndex = ENTINDEX(ent) - 1;
	if (menu && menu->menuText)
	{
		char tempText[384];
		cstrcpy(tempText, menu->menuText);
		Replace(tempText, '\v', '\n');
		char* text = tempText;

		// make menu looks best
		char buffer[64];
		char buffer2[64];
		int i;
		for (i = 0; i <= 9; i++)
		{
			FormatBuffer(buffer, "%d.", i);
			FormatBuffer(buffer2, "\\r%d.\\w", i);
			Replace2(tempText, buffer, buffer2);
		}

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

	if (g_isFakeCommand || g_fakeCommandTimer > engine->GetTime() || IsNullString(format) || !IsValidBot(fakeClient))
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
	g_fakeCommandTimer = engine->GetTime() + 0.02;
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
	g_fakeArgc = 0;
	g_isFakeCommand = false;
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
	return &field[0]; // returns the wanted field
}

char* GetModName(void)
{
	static char modName[256];

	// ask the engine for the MOD directory path
	// get the length of the returned string
	GET_GAME_DIR(modName);
	int length = cstrlen(modName);

	// format the returned string to get the last directory name
	int stop = length - 1;
	while (stop && (modName[stop] == '\\' || modName[stop] == '/'))
		stop--; // shift back any trailing separator

	int start = stop;
	while (start && modName[start] != '\\' && modName[start] != '/')
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
	// auto semiclip detection
	cvar_t* sc = g_engfuncs.pfnCVarGetPointer("semiclip");
	if (sc && sc->value == 1.0f)
	{
		extern ConVar ebot_has_semiclip;
		ebot_has_semiclip.SetInt(1);
	}

	if (ebot_zp_delay_custom.GetFloat() > 0.0f)
		g_DelayTimer = engine->GetTime() + ebot_zp_delay_custom.GetFloat() + 2.2f;
	else
	{
		cvar_t* zpCvar = g_engfuncs.pfnCVarGetPointer("zp_delay");
		if (!zpCvar)
			zpCvar = g_engfuncs.pfnCVarGetPointer("zp_gamemode_delay");

		if (zpCvar)
		{
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
					extern ConVar ebot_dark_mode;
					ebot_dark_mode.SetInt(1);
				}
				else if (zpCvar->string[0] == 'b')
				{
					extern ConVar ebot_force_flashlight;
					ebot_force_flashlight.SetInt(1);
				}
			}
		}
		else
		{
			zpCvar = g_engfuncs.pfnCVarGetPointer("bh_starttime");
			if (zpCvar)
			{
				const float delayTime = zpCvar->value + 1.0f;
				if (delayTime > 0.0f)
					g_DelayTimer = engine->GetTime() + delayTime;
			}
		}
	}

	for (Bot* const &bot : g_botManager->m_bots)
	{
		if (bot)
			bot->NewRound();
	}

	g_roundEnded = false;
}

bool IsBreakable(edict_t* ent)
{
	if (FNullEnt(ent))
		return false;

	if ((FClassnameIs(ent, "func_breakable") || (FClassnameIs(ent, "func_pushable") && (ent->v.spawnflags & SF_PUSH_BREAKABLE)) || FClassnameIs(ent, "func_wall")))
	{
		if (ent->v.takedamage != DAMAGE_NO && !ent->v.impulse && !(ent->v.flags & FL_WORLDBRUSH) && !(ent->v.spawnflags & SF_BREAK_TRIGGER_ONLY))
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
	else
	{
		if (g_DelayTimer > engine->GetTime())
			player_team = Team::Counter;
		else if (g_roundEnded)
			player_team = Team::Terrorist;
		else
			player_team = *(reinterpret_cast<int*>(ent->pvPrivateData) + OFFSET_TEAM) - 1;
	}

	return player_team;
}

bool IsZombieEntity(edict_t* ent)
{
	return GetTeam(ent) == Team::Terrorist;
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

	if (FClassnameIs(ent, "func_wall"))
		return false;

	if (FClassnameIs(ent, "func_illusionary"))
		return false;

	if (FClassnameIs(ent, "func_door") || FClassnameIs(ent, "func_door_rotating"))
		return true;

	if (FClassnameIs(ent, "func_breakable") && ent->v.takedamage != DAMAGE_NO)
		return true;

	return false;
}

bool IsWalkableLineClear(const Vector& from, const Vector& to)
{
	TraceResult result;
	edict_t* pEntIgnore = g_hostEntity;
	edict_t* pEntPrev = g_hostEntity;
	Vector useFrom = from, dir;

	int8_t i;
	for (i = 0; i < 64; i++)
	{
		TraceLine(useFrom, to, TraceIgnore::Monsters, pEntIgnore, &result);

		// if we hit a walkable entity, try again
		if (result.flFraction < 1.0f && (result.pHit && IsEntityWalkable(result.pHit)))
		{
			if (result.pHit == pEntPrev)
				return false; // deadlock, give up

			pEntPrev = pEntIgnore;
			pEntIgnore = result.pHit;

			// start from just beyond where we hit to avoid infinite loops
			dir = to - from;
			dir.Normalize();
			useFrom = result.vecEndPos + 5.0f * dir;
		}
		else
			break;
	}

	if (result.flFraction >= 1.0f)
		return true;

	return false;
}

bool IsWalkableHullClear(const Vector& from, const Vector& to)
{
	TraceResult result;
	edict_t* pEntIgnore = g_hostEntity;
	edict_t* pEntPrev = g_hostEntity;
	Vector useFrom = from, dir;

	int8_t i;
	for (i = 0; i < 64; i++)
	{
		TraceHull(useFrom, to, TraceIgnore::Monsters, head_hull, pEntIgnore, &result);

		// if we hit a walkable entity, try again
		if (result.flFraction < 1.0f && (result.pHit && IsEntityWalkable(result.pHit)))
		{
			if (result.pHit == pEntPrev)
				return false; // deadlock, give up

			pEntPrev = pEntIgnore;
			pEntIgnore = result.pHit;

			// start from just beyond where we hit to avoid infinite loops
			dir = to - from;
			dir.Normalize();
			useFrom = result.vecEndPos + 5.0f * dir;
		}
		else
			break;
	}

	if (result.flFraction >= 1.0f)
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
	char string[3072];
	char string2[3072];

	va_list ap;
	va_start(ap, format);
	vsprintf(string, format, ap);
	va_end(ap);

	FormatBuffer(string2, "[%s] %s\n", PRODUCT_LOGTAG, string);
	SERVER_PRINT(string2);
}

void ServerPrintNoTag(const char* format, ...)
{
	char string[3072];
	char string2[3072];

	va_list ap;
	va_start(ap, format);
	vsprintf(string, format, ap);
	va_end(ap);

	FormatBuffer(string2, "%s\n", string);
	SERVER_PRINT(string2);
}

void CenterPrint(const char* format, ...)
{
	char string[2048];

	va_list ap;
	va_start(ap, format);
	vsprintf(string, format, ap);
	va_end(ap);

	if (IsDedicatedServer())
	{
		ServerPrint(string);
		return;
	}

	char string2[2048];
	FormatBuffer(string2, "%s\n", string);
	MESSAGE_BEGIN(MSG_BROADCAST, g_netMsg->GetId(NETMSG_TEXTMSG));
	WRITE_BYTE(HUD_PRINTCENTER);
	WRITE_STRING(string2);
	MESSAGE_END();
}

void ChatPrint(const char* format, ...)
{
	char string[2048];

	va_list ap;
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

	char string[2048];

	va_list ap;
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
	{
		char string2[2048];
		FormatBuffer(string2, "[E-BOT] %s", string);
		(*g_engfuncs.pfnClientPrintf) (ent, static_cast<PRINT_TYPE>(dest &= ~0x3ff), string2);
	}
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
	char string[1024];
	char string2[1024];

	// concatenate all the arguments in one string
	va_list ap;
	va_start(ap, format);
	vsprintf(string, format, ap);
	va_end(ap);

	FormatBuffer(string2, "%s\n", string);
	SERVER_COMMAND(string2); // execute command
}

char* GetEntityName(edict_t* entity)
{
	static char entityName[32];
	if (!g_pGlobals || FNullEnt(entity))
		cstrcpy(entityName, "nullptr");
	else if (entity->v.flags & FL_CLIENT || entity->v.flags & FL_FAKECLIENT)
		cstrncpy(entityName, STRING(entity->v.netname), sizeof(entityName));
	else
		cstrncpy(entityName, STRING(entity->v.classname), sizeof(entityName));

	return &entityName[0];
}

// this function gets the map name and store it in the map_name global string variable.
char* GetMapName(void)
{
	static char mapName[32];
	cstrncpy(mapName, STRING(g_pGlobals->mapname), sizeof(mapName));
	return &mapName[0]; // and return a pointer to it
}

bool OpenConfig(const char* fileName, const char* errorIfNotExists, File* outFile)
{
	if (outFile->IsValid())
		outFile->Close();

	char buffer[1024];
	FormatBuffer(buffer, "%s/addons/ebot/%s", GetModName(), fileName);
	outFile->Open(buffer, "rt");
	if (!outFile->IsValid())
	{
		AddLogEntry(Log::Error, errorIfNotExists);
		return false;
	}

	return true;
}

char* GetWaypointDir(void)
{
	static char wpDir[1024];
    sprintf(wpDir, "%s/addons/ebot/waypoints/", GetModName());
	return &wpDir[0];
}

// this function tells the engine that a new server command is being declared, in addition
// to the standard ones, whose name is command_name. The engine is thus supposed to be aware
// that for every "command_name" server command it receives, it should call the function
// pointed to by "function" in order to handle it.
void RegisterCommand(const char* command, void funcPtr(void))
{
	if (IsNullString(command) || !funcPtr)
		return; // reliability check

	// it must be const char*
	REG_SVR_COMMAND(command, funcPtr); // ask the engine to register this new command
}

void DetectCSVersion(void)
{
	const char* infoBuffer = "Game Registered: %s (0x%d)";

	// switch version returned by dll loader
	if (g_gameVersion & Game::Xash)
	{
		if (g_gameVersion & Game::CZero)
			ServerPrint(infoBuffer, "CS: CZ (Xash Engine)", sizeof(Bot));
		else
		{
			g_gameVersion = (Game::Xash | Game::CStrike);
			ServerPrint(infoBuffer, "CS 1.6 (Xash Engine)", sizeof(Bot));
		}
	}
	else if (g_gameVersion & Game::CZero)
		ServerPrint(infoBuffer, "CS: CZ", sizeof(Bot));
	else
	{
		g_gameVersion = Game::CStrike;
		ServerPrint(infoBuffer, "CS 1.6", sizeof(Bot));
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
void AddLogEntry(const Log logLevel, const char* format, ...)
{
	char buffer[512] = { 0, }, levelString[32] = { 0, }, logLine[1024] = { 0, };

	va_list ap;
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

void MOD_AddLogEntry(const int mod, char* format)
{
	char modName[32], logLine[1024] = { 0, }, buildVersionName[64];
	uint16_t mod_bV16[4];

	if (mod == -1)
	{
		sprintf(modName, "E-BOT");
		const int buildVersion[4] = {PRODUCT_VERSION_DWORD};
		int i;
		for (i = 0; i < 4; i++)
			mod_bV16[i] = static_cast<uint16_t>(buildVersion[i]);
	}

	ServerPrintNoTag("[%s Log] %s", modName, format);
	sprintf(buildVersionName, "%s_build_%u_%u_%u_%u.txt", modName, mod_bV16[0], mod_bV16[1], mod_bV16[2], mod_bV16[3]);

	// if logs folder deleted, it will result a crash... so create it again before writing logs...
	char buffer[1024];
	FormatBuffer(buffer, "%s/addons/ebot/logs", GetModName());
	CreatePath(buffer);

	FormatBuffer(buffer, "%s/addons/ebot/logs/%s", GetModName(), buildVersionName);
	File checkLogFP(buffer, "rb");

	FormatBuffer(buffer, "%s/addons/ebot/logs/%s", GetModName(), buildVersionName);
	File fp(buffer, "at");

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
	float nearestPlayer = 9999999.0f; // nearest player

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