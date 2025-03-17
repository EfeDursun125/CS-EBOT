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

ConVar ebot_quota("ebot_quota", "10");
ConVar ebot_forceteam("ebot_force_team", "any");

ConVar ebot_difficulty("ebot_difficulty", "4");
ConVar ebot_minskill("ebot_min_skill", "1");
ConVar ebot_maxskill("ebot_max_skill", "100");

ConVar ebot_nametag("ebot_name_tag", "2");
ConVar ebot_ping("ebot_fake_ping", "0");
ConVar ebot_display_avatar("ebot_display_avatar", "0");

ConVar ebot_keep_slots("ebot_keep_slots", "1");

// this is a bot manager class constructor
BotControl::BotControl(void)
{
	cmemset(m_bots, 0, sizeof(m_bots));
	InitQuota();
}

// this is a bot manager class destructor, do not use engine->GetMaxClients() here !!
BotControl::~BotControl(void)
{
	for (auto& bot : m_bots)
	{
		if (bot)
		{
			delete bot;
			bot = nullptr;
		}
	}

	m_creationTab.Destroy();
	m_avatars.Destroy();
}

// this function calls gamedll player() function, in case to create player entity in game
void BotControl::CallGameEntity(entvars_t* vars)
{
	CALL_GAME_ENTITY(PLID, "player", vars);
}

// this function completely prepares bot entity (edict) for creation, creates team, skill, sets name etc, and
// then sends result to bot constructor
int BotControl::CreateBot(char name[32], int skill, int personality, const int team, const int member)
{
	if (g_numWaypoints < 1) // don't allow creating bots with no waypoints loaded
	{
		if (!g_analyzewaypoints)
		{
			extern ConVar ebot_analyze_auto_start;
			if (ebot_analyze_auto_start.GetBool())
        	{
            	g_waypoint->CreateBasic();
            	g_analyzewaypoints = true;
				return -1;
        	}
		}

		ServerPrint("No any waypoints for this map, Cannot Add E-BOT");
		ServerPrint("You can input 'ebot wp menu' to make waypoint");
		CenterPrint("No any waypoints for this map, Cannot Add E-BOT");
		CenterPrint("You can input 'ebot wp menu' to make waypoint");
		return -1;
	}
	else if (g_waypointsChanged) // don't allow creating bots with changed waypoints (distance tables are messed up)
	{
		CenterPrint("Waypoints has been changed. Load waypoints again...");
		return -1;
	}

	if (skill < 1 || skill > 100)
	{
		if (ebot_difficulty.GetInt() >= 4)
			skill = 100;
		else if (ebot_difficulty.GetInt() == 3)
			skill = crandomint(79, 99);
		else if (ebot_difficulty.GetInt() == 2)
			skill = crandomint(50, 79);
		else if (ebot_difficulty.GetInt() == 1)
			skill = crandomint(30, 50);
		else if (!ebot_difficulty.GetInt())
			skill = crandomint(1, 30);
		else
		{
			const int maxSkill = ebot_maxskill.GetInt();
			const int minSkill = (!ebot_minskill.GetInt()) ? 1 : ebot_minskill.GetInt();

			if (maxSkill <= 100 && minSkill)
				skill = crandomint(minSkill, maxSkill);
			else
				skill = crandomint(0, 100);
		}
	}

	if (personality < 0 || personality > 2)
	{
		const int randomPrecent = crandomint(1, 3);
		if (randomPrecent == 1)
			personality = Personality::Normal;
		else if (randomPrecent == 2)
			personality = Personality::Rusher;
		else
			personality = Personality::Careful;
	}

	char outputName[33];
	bool addTag = true;

	if (!name[0])
	{
		bool getName = false;
		if (!g_botNames.IsEmpty())
		{
			int16_t i;
			for (i = 0; i < g_botNames.Size(); i++)
			{
				if (!g_botNames[i].isUsed)
				{
					getName = true;
					break;
				}
			}
		}

		if (getName)
		{
			bool nameUse = true;
			while (nameUse)
			{
				NameItem& botName = g_botNames.Random();
				if (!botName.isUsed)
				{
					nameUse = false;
					botName.isUsed = true;
					sprintf(outputName, "%s", botName.name);
				}
			}
		}
		else
			sprintf(outputName, "e-bot %i", crandomint(1, 9999)); // just pick ugly random name
	}
	else
		sprintf(outputName, "%s", name);

	char botName[64];
	if (ebot_nametag.GetInt() == 2 && addTag)
		snprintf(botName, sizeof(botName), "[E-BOT] %s (%d)", outputName, skill);
	else if (ebot_nametag.GetInt() == 1 && addTag)
		snprintf(botName, sizeof(botName), "[E-BOT] %s", outputName);
	else
		cstrncpy(botName, outputName, sizeof(botName));

	edict_t* bot = nullptr;
	if (FNullEnt((bot = g_engfuncs.pfnCreateFakeClient(botName))))
	{
		const int maxClients = engine->GetMaxClients();
		CenterPrint(" Unable to create E-Bot, Maximum players reached (%d/%d)", maxClients, maxClients);
		return -2;
	}

	const int index = ENTINDEX(bot) - 1;
	if (m_bots[index])
		delete m_bots[index];

	m_bots[index] = new(std::nothrow) Bot(bot, skill, personality, team, member);
	if (!m_bots[index])
	{
		AddLogEntry(Log::Memory, "unexpected memory error -> not enough memory (%i free byte required)", sizeof(Bot));
		return -1;
	}

	const char* ebotName = GetEntityName(bot);
	ServerPrint("Connecting E-Bot: %s - Skill: %d", ebotName, skill);

	// set values
	m_bots[index]->m_senseChance = crandomint(10, 90);
	return index;
}

// this function returns index of bot (using own bot array)
int BotControl::GetIndex(edict_t* ent)
{
	if (FNullEnt(ent))
		return -1;

	const int index = ENTINDEX(ent) - 1;
	if (index < 0 || index >= 32)
		return -1;

	if (m_bots[index])
		return index;

	return -1; // if no edict, return -1;
}

// this function returns index of bot (using own bot array)
int BotControl::GetIndex(const int index)
{
	if (index < 0 || index >= 32)
		return -1;

	if (m_bots[index])
		return index;

	return -1;
}

// this function finds a bot specified by index, and then returns pointer to it (using own bot array)
Bot* BotControl::GetBot(const int index)
{
	if (index < 0 || index >= 32)
		return nullptr;

	return m_bots[index];
}

// same as above, but using bot entity
Bot* BotControl::GetBot(edict_t* ent)
{
	return GetBot(GetIndex(ent));
}

void BotControl::Think(void)
{
	const float time2 = engine->GetTime();
	for (Bot* const &bot : m_bots)
	{
		if (bot && bot->m_updateTimer < time2)
		{
			bot->BaseUpdate();
			bot->m_updateTimer = time2 + 0.025f;
		}
	}
}

// this function putting bot creation process to queue to prevent engine crashes
void BotControl::AddBot(char name[32], const int skill, const int personality, const int team, const int member)
{
	CreateItem queueID;

	// fill the holder
	cstrcpy(queueID.name, name);
	queueID.skill = skill;
	queueID.personality = personality;
	queueID.team = team;
	queueID.member = member;

	// put to queue
	m_creationTab.Push(queueID);

	// keep quota number up to date
	const int botsNum = GetBotsNum() + 1;
	if (botsNum > ebot_quota.GetInt())
		ebot_quota.SetInt(botsNum);
}

int BotControl::AddBotAPI(char name[32], const int skill,const  int team)
{
	const int botsNum = GetBotsNum() + 1;
	if (botsNum > ebot_quota.GetInt())
		ebot_quota.SetInt(botsNum);

	const int resultOfCall = CreateBot(name, skill, -1, team, -1);

	// check the result of creation
	if (resultOfCall == -1)
	{
		m_creationTab.Destroy(); // something wrong with waypoints, reset tab of creation
		ebot_quota.SetInt(0); // reset quota
		ChatPrint("[E-BOT] You can input [ebot wp on] make the new waypoints!!");
	}
	else if (resultOfCall == -2)
	{
		m_creationTab.Destroy(); // maximum players reached, so set quota to maximum players
		ebot_quota.SetInt(GetBotsNum());
	}

	m_maintainTime = engine->GetTime() + 0.24f;

	return resultOfCall;
}

// this function keeps number of bots up to date, and don't allow to maintain bot creation
// while creation process in process.
void BotControl::MaintainBotQuota(void)
{
	const float time = engine->GetTime();
	if (!m_creationTab.IsEmpty() && m_maintainTime < time)
	{
		CreateItem last = m_creationTab.Pop();
		const int resultOfCall = CreateBot(last.name, last.skill, last.personality, last.team, last.member);

		// check the result of creation
		if (resultOfCall == -1)
		{
			m_creationTab.Destroy(); // something wrong with waypoints, reset tab of creation
			ebot_quota.SetInt(0); // reset quota
			ChatPrint("[E-BOT] You can input [ebot wp on] make the new waypoints.");
		}
		else if (resultOfCall == -2)
		{
			m_creationTab.Destroy(); // maximum players reached, so set quota to maximum players
			ebot_quota.SetInt(GetBotsNum());
		}

		m_maintainTime = time + 0.48f;
	}

	if (m_maintainTime < time)
	{
		const int maxClients = engine->GetMaxClients() - ebot_keep_slots.GetInt();
		const int botNumber = GetBotsNum();

		if (botNumber > ebot_quota.GetInt())
			RemoveRandom();

		if (botNumber < ebot_quota.GetInt() && botNumber < maxClients)
		{
			AddRandom();
			m_maintainTime = time + 0.32f;
		}
		else
			m_maintainTime = time + 1.0f;

		// check valid range of quota
		if (ebot_quota.GetInt() > maxClients)
			ebot_quota.SetInt(maxClients);
		else if (ebot_quota.GetInt() < 0)
			ebot_quota.SetInt(0);
	}
}

void BotControl::InitQuota(void)
{
	m_maintainTime = engine->GetTime() + 2.0f;
	m_creationTab.Destroy();
}

// this function fill server with bots, with specified team & personality
void BotControl::FillServer(int selection, const int personality, const int skill, const int numToAdd)
{
	const int maxClients = engine->GetMaxClients();
	const int getHumansNum = GetHumansNum();
	const int getBotsNum = GetBotsNum();

	if (getBotsNum >= (maxClients - getHumansNum))
		return;

	if (selection == 1 || selection == 2)
	{
		CVAR_SET_STRING("mp_limitteams", "0");
		CVAR_SET_STRING("mp_autoteambalance", "0");
	}
	else
		selection = 5;

	const char teamDescs[6][12] =
	{
	   "",
	   {"Terrorists"},
	   {"CTs"},
	   "",
	   "",
	   {"Random"},
	};

	const int toAdd = numToAdd == -1 ? (maxClients - (getHumansNum + getBotsNum)) : numToAdd;
	int i, randomizedSkill;
	for (i = 0; i <= toAdd; i++)
	{
		// since we got constant skill from menu (since creation process call automatic), we need to manually randomize skill here, on given skill there
		randomizedSkill = 0;

		if (skill >= 0 && skill <= 20)
			randomizedSkill = crandomint(0, 20);
		else if (skill >= 20 && skill <= 40)
			randomizedSkill = crandomint(20, 40);
		else if (skill >= 40 && skill <= 60)
			randomizedSkill = crandomint(40, 60);
		else if (skill >= 60 && skill <= 80)
			randomizedSkill = crandomint(60, 80);
		else if (skill >= 80 && skill <= 99)
			randomizedSkill = crandomint(80, 99);
		else if (skill == 100)
			randomizedSkill = skill;

		AddBot("", randomizedSkill, personality, selection, -1);
	}

	ebot_quota.SetInt(toAdd);
	CenterPrint("Filling the server with %s ebots", &teamDescs[selection][0]);
}

// this function drops all bot clients from server (this function removes only ebots)
void BotControl::RemoveAll(void)
{
	CenterPrint("E-Bots are removed from server");

	for (const auto& bot : m_bots)
	{
		if (bot) // is this slot used?
			bot->Kick();
	}

	m_creationTab.Destroy();

	// reset cvars
	ebot_quota.SetInt(0);
}

// this function remove random bot from specified team (if removeAll value = 1 then removes all players from team)
void BotControl::RemoveFromTeam(const Team team, const bool removeAll)
{
	for (const auto& bot : m_bots)
	{
		if (bot && team == bot->m_team)
		{
			bot->Kick();
			if (!removeAll)
				break;
		}
	}
}

void BotControl::RemoveMenu(edict_t* ent, const int selection)
{
	if ((selection > 4) || (selection < 1))
		return;

	char tempBuffer[512], buffer[512];
	cmemset(tempBuffer, 0, sizeof(tempBuffer));
	cmemset(buffer, 0, sizeof(buffer));

	int i;
	int validSlots = (selection == 4) ? (1 << 9) : ((1 << 8) | (1 << 9));
	for (i = ((selection - 1) * 8); i < selection * 8; ++i)
	{
		if (m_bots[i] && !FNullEnt(m_bots[i]->m_myself))
		{
			validSlots |= 1 << (i - ((selection - 1) * 8));
			sprintf(buffer, "%s %1.1d. %s%s\n", buffer, i - ((selection - 1) * 8) + 1, GetEntityName(m_bots[i]->m_myself), GetTeam(m_bots[i]->m_myself) == Team::Counter ? " \\y(CT)\\w" : " \\r(T)\\w");
		}
		else if (!FNullEnt(g_clients[i].ent))
			sprintf(buffer, "%s %1.1d.\\d %s (Not E-BOT) \\w\n", buffer, i - ((selection - 1) * 8) + 1, GetEntityName(g_clients[i].ent));
		else
			sprintf(buffer, "%s %1.1d.\\d Null \\w\n", buffer, i - ((selection - 1) * 8) + 1);
	}

	sprintf(tempBuffer, "\\yE-BOT Remove Menu (%d/4):\\w\n\n%s\n%s 0. Back", selection, buffer, (selection == 4) ? "" : " 9. More...\n");

	switch (selection)
	{
		case 1:
		{
			g_menus[14].validSlots = validSlots & static_cast<unsigned int>(-1);
			g_menus[14].menuText = tempBuffer;
			DisplayMenuToClient(ent, &g_menus[14]);
			break;
		}
		case 2:
		{
			g_menus[15].validSlots = validSlots & static_cast<unsigned int>(-1);
			g_menus[15].menuText = tempBuffer;
			DisplayMenuToClient(ent, &g_menus[15]);
			break;
		}
		case 3:
		{
			g_menus[16].validSlots = validSlots & static_cast<unsigned int>(-1);
			g_menus[16].menuText = tempBuffer;
			DisplayMenuToClient(ent, &g_menus[16]);
			break;
		}
		case 4:
		{
			g_menus[17].validSlots = validSlots & static_cast<unsigned int>(-1);
			g_menus[17].menuText = tempBuffer;
			DisplayMenuToClient(ent, &g_menus[17]);
			break;
		}
	}
}

// this function kills all bots on server (only this dll controlled bots)
void BotControl::KillAll(const int team)
{
	for (const auto &bot : m_bots)
	{
		if (bot)
		{
			if (team != Team::Count && team != bot->m_team)
				continue;

			bot->Kill();
		}
	}

	CenterPrint("All bots are killed.");
}

// this function removes random bot from server (only ebots)
void BotControl::RemoveRandom(void)
{
	for (const auto &bot : m_bots)
	{
		if (bot) // is this slot used?
		{
			bot->Kick();
			break;
		}
	}
}

// this function lists bots currently playing on the server
void BotControl::ListBots(void)
{
	ServerPrintNoTag("%-3.5s %-9.13s %-17.18s %-3.4s %-3.4s %-3.4s", "index", "name", "personality", "team", "skill", "frags");

	edict_t* player;
	for (const auto &client : g_clients)
	{
		if (FNullEnt(client.ent))
			continue;

		player = client.ent;

		// is this player slot valid
		if (IsValidBot(player))
			ServerPrintNoTag("[%-3.1d] %-9.13s %-17.18s %-3.4s %-3.1d %-3.1d", client.index, GetEntityName(player), GetBot(player)->m_personality == Personality::Rusher ? "Rusher" : GetBot(player)->m_personality == Personality::Normal ? "Normal" : "Careful", GetTeam(player) ? "CT" : "TR", GetBot(player)->m_skill, static_cast<int>(player->v.frags));
	}
}

// this function returns number of ebot's playing on the server
int BotControl::GetBotsNum(void)
{
	int count = 0;
	for (const auto& bot : m_bots)
	{
		if (bot)
			count++;
	}

	return count;
}

// this function returns number of humans playing on the server
int BotControl::GetHumansNum(void)
{
	int count = 0;
	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (!m_bots[client.index])
			count++;
	}

	return count;
}

// this function free all bots slots (used on server shutdown)
void BotControl::Free(void)
{
	for (auto& bot : m_bots)
	{
		if (!bot)
			continue;

		bot->m_navNode.Destroy();
		delete bot;
		bot = nullptr;
	}

	m_creationTab.Destroy();
	m_avatars.Destroy();
}

// this function frees one bot selected by index (used on bot disconnect)
void BotControl::Free(const int index)
{
	if (index < 0 || index >= 32)
		return;

	if (!m_bots[index])
		return;

	m_bots[index]->m_navNode.Destroy();
	delete m_bots[index];
	m_bots[index] = nullptr;
}

// this function controls the bot entity
Bot::Bot(edict_t* bot, const int skill, const int personality, const int team, const int member)
{
	if (!bot)
		return;

	char rejectReason[128];
	const int clientIndex = ENTINDEX(bot);
	cmemset(reinterpret_cast<void*>(this), 0, sizeof(*this));

	const int netname = bot->v.netname;
	bot->v = {}; // reset entire the entvars structure (fix from regamedll)

	// restore containing entity, name and client flags
	bot->v.pContainingEntity = bot;
	bot->v.flags = FL_FAKECLIENT | FL_CLIENT;
	bot->v.netname = netname;

	pev = &bot->v;
	if (bot->pvPrivateData)
		FREE_PRIVATE(bot);

	bot->pvPrivateData = nullptr;

	// create the player entity by calling MOD's player function
	BotControl::CallGameEntity(&bot->v);

	// set all info buffer keys for this bot
	char* buffer = GET_INFOKEYBUFFER(bot);
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "_vgui_menus", "0");
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "cl_autowepswitch", "0");
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "_cl_autowepswitch", "0");

	if (!ebot_ping.GetBool())
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "*bot", "1");

	rejectReason[0] = 0;
	char buffi[32];
	FormatBuffer(buffi, "%d.%d.%d.%d", crandomint(1, 255), crandomint(1, 255), crandomint(1, 255), crandomint(1, 255));
	MDLL_ClientConnect(bot, "E-BOT", buffi, rejectReason);

	// should be set after client connect
	if (ebot_display_avatar.GetBool() && !g_botManager->m_avatars.IsEmpty())
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "*sid", g_botManager->m_avatars.Random());

	if (!IsNullString(rejectReason) || !m_navNode.Init(g_numWaypoints))
	{
		const char* name = GetEntityName(bot);
		if (!IsNullString(name))
		{
			AddLogEntry(Log::Warning, "Server refused '%s' connection (%s)", name, rejectReason);
			ServerCommand("kick \"%s\"", name); // kick the bot player if the server refused it
		}

		bot->v.flags |= FL_KILLME;
	}

	MDLL_ClientPutInServer(bot);

	// initialize all the variables for this bot...
	m_notStarted = true;  // hasn't joined game yet
	m_difficulty = ebot_difficulty.GetInt(); // set difficulty
	m_basePingLevel = crandomint(11, 111);

	m_startAction = CMENU_IDLE;

	m_pathInterval = engine->GetTime();
	m_frameInterval = engine->GetTime();

	m_isAlive = false;
	m_skill = skill;

	switch (personality)
	{
		case 1:
		{
			m_personality = Personality::Rusher;
			break;
		}
		case 2:
		{
			m_personality = Personality::Careful;
			break;
		}
		default:
			m_personality = Personality::Normal;
	}

	cmemset(&m_ammoInClip, 1, sizeof(m_ammoInClip));
	cmemset(&m_ammo, 1, sizeof(m_ammo));
	m_currentWeapon = 0; // current weapon is not assigned at start

	// assign team and class
	m_wantedTeam = team;
	m_wantedClass = member;
	m_index = GetIndex() - 1;
	m_zhCampPointIndex = -1;
	m_myMeshWaypoint = -1;

	NewRound();

	const float way = static_cast<float>(g_numWaypoints);
	g_fakeCommandTimer = engine->GetTime() + 1.0f;
}

Bot::~Bot(void)
{
	m_navNode.Destroy();
	const char* name = GetEntityName(m_myself);
	if (IsNullString(name))
		return;

	char botName[64];
	int16_t i;
	for (i = 0; i < g_botNames.Size(); i++)
	{
		if (!cstrcmp(botName, name))
		{
			g_botNames[i].isUsed = false;
			break;
		}

		sprintf(botName, "[E-BOT] %s", g_botNames[i].name);
		if (!cstrcmp(g_botNames[i].name, name))
		{
			g_botNames[i].isUsed = false;
			break;
		}

		sprintf(botName, "[E-BOT] %s (%i)", g_botNames[i].name, m_skill);
		if (!cstrcmp(g_botNames[i].name, name))
		{
			g_botNames[i].isUsed = false;
			break;
		}
	}
}

// this function initializes a bot after creation & at the start of each round
void Bot::NewRound(void)
{
	// clear all allocated path nodes
	m_navNode.Clear();

	m_team = GetTeam(m_myself);
	m_isAlive = IsAlive(m_myself);
	if (!m_isAlive) // if bot died, clear all weapon stuff and force buying again
	{
		cmemset(&m_ammoInClip, 0, sizeof(m_ammoInClip));
		cmemset(&m_ammo, 0, sizeof(m_ammo));
		m_currentWeapon = 0;
		return;
	}

	m_numSpawns++;
	const float time2 = engine->GetTime();

	// initialize msec value
	m_aimInterval = time2;
	m_frameInterval = time2;
	m_pathInterval = time2;

	SetProcess(Process::Default, "i have respawned", true, time2 + 9999.0f);
	m_rememberedProcess = Process::Default;
	m_rememberedProcessTime = 0.0f;

	if (!g_waypoint->m_zmHmPoints.IsEmpty())
	{
		m_zhCampPointIndex = g_waypoint->m_zmHmPoints.Random();
		m_currentGoalIndex = m_zhCampPointIndex;
	}
	else
		m_zhCampPointIndex = -1;
	m_myMeshWaypoint = -1;

	m_hasEnemiesNear = false;
	m_hasEntitiesNear = false;
	m_hasFriendsNear = false;

	m_team = GetTeam(m_myself);
	m_isZombieBot = IsZombieEntity(m_myself);

	m_prevTravelFlags = 0;
	m_currentTravelFlags = 0;

	m_prevWptIndex[0] = -1;
	m_prevWptIndex[1] = -1;
	m_prevWptIndex[2] = -1;
	m_prevWptIndex[3] = -1;

	m_breakableEntity = nullptr;
	m_buttonEntity = nullptr;
	m_timeDoorOpen = 0.0f;

	m_buttonPushTime = 0.0f;
	m_seeEnemyTime = 0.0f;

	m_firePause = 0.0f;
	m_jumpTime = 0.0f;
	m_duckTime = 0.0f;

	m_zoomCheckTime = 0.0f;
	m_buttons = 0;
	m_startAction = CMENU_IDLE;

	m_moveSpeed = pev->maxspeed;
	m_tempstrafeSpeed = crandomint(0, 1) ? pev->maxspeed : -pev->maxspeed;
	g_fakeCommandTimer = time2 + 0.05f;
	m_randomAttackTimer = time2 + crandomfloat(10.0f, 30.0f);
	m_slowThinkTimer = time2 + crandomfloat(1.0f, 3.0f);
	ResetStuck();

	// reset waypoint
	int16_t index = g_waypoint->FindNearestToEntSlow(pev->origin, 4096.0f, m_myself);
	if (!IsValidWaypoint(index))
		index = g_waypoint->FindNearestSlow(pev->origin);

	ChangeWptIndex(index);
	m_waypointTime = engine->GetTime() + 0.5f;
}

// this function kills a bot (not just using ClientKill, but like the CSBot does)
// base code courtesy of Lazy (from bots-united forums!)
void Bot::Kill(void)
{
	edict_t* hurtEntity = g_engfuncs.pfnCreateNamedEntity(MAKE_STRING("trigger_hurt"));
	if (FNullEnt(hurtEntity))
		return;

	hurtEntity->v.classname = MAKE_STRING(g_weaponDefs[m_currentWeapon].className);
	hurtEntity->v.dmg_inflictor = m_myself;
	hurtEntity->v.dmg = 999999.0f;
	hurtEntity->v.dmg_take = 1.0f;
	hurtEntity->v.dmgtime = 2.0f;
	hurtEntity->v.effects |= EF_NODRAW;

	g_engfuncs.pfnSetOrigin(hurtEntity, Vector(-4000.0f, -4000.0f, -4000.0f));

	KeyValueData kv;
	kv.szClassName = const_cast<char*>(g_weaponDefs[m_currentWeapon].className);
	kv.szKeyName = "damagetype";
	char buffer[32];
	FormatBuffer(buffer, "%d", (1 << 4));
	kv.szValue = buffer;
	kv.fHandled = false;

	MDLL_KeyValue(hurtEntity, &kv);
	MDLL_Spawn(hurtEntity);
	MDLL_Touch(hurtEntity, m_myself);

	g_engfuncs.pfnRemoveEntity(hurtEntity);
}

void Bot::Kick(void)
{
	const char* myName = GetEntityName(m_myself);
	if (IsNullString(myName))
		return;

	pev->flags &= ~FL_FAKECLIENT;
	pev->flags |= FL_DORMANT;
	pev->flags |= FL_CLIENT;
	ServerCommand("kick \"%s\"", myName);
	CenterPrint("E-Bot '%s' kicked from the server", myName);

	const int botsNum = g_botManager->GetBotsNum() - 1;
	if (botsNum < ebot_quota.GetInt())
		ebot_quota.SetInt(botsNum);
}

// this function handles the selection of teams & class
void Bot::StartGame(void)
{
	// check if something has assigned team to us
	if ((m_team == Team::Terrorist || m_team == Team::Counter) && IsAlive(m_myself))
	{
		m_notStarted = false;
		m_startAction = CMENU_IDLE;
		return;
	}

	m_retryJoin++;

	if (m_retryJoin > 30)
		m_retryJoin = 0;
	else if (m_retryJoin > 20)
		m_startAction = CMENU_CLASS;
	else if (m_retryJoin > 10)
		m_startAction = CMENU_TEAM;

	// handle counter-strike stuff here...
	if (m_startAction == CMENU_TEAM)
	{
		if (ebot_forceteam.GetString()[0] == 'C' || ebot_forceteam.GetString()[0] == 'c' || ebot_forceteam.GetString()[0] == '2')
			m_wantedTeam = 2;
		else if (ebot_forceteam.GetString()[0] == 'T' || ebot_forceteam.GetString()[0] == 't' || ebot_forceteam.GetString()[0] == '1') // 1 = T, 2 = CT
			m_wantedTeam = 1;

		if (m_wantedTeam != 1 && m_wantedTeam != 2)
			m_wantedTeam = 5;

		// select the team the bot wishes to join...
		g_fakeCommandTimer = 0.0f;
		FakeClientCommand(m_myself, "menuselect %d", m_wantedTeam);
		m_startAction = CMENU_TEAM; // switch to team
	}

	if (m_startAction == CMENU_CLASS)
	{
		m_wantedClass = crandomint(1, g_gameVersion & Game::CZero ? 5 : 4);

		// select the class the bot wishes to use...
		g_fakeCommandTimer = 0.0f;
		FakeClientCommand(m_myself, "menuselect %d", m_wantedClass);

		// bot has now joined the game (doesn't need to be started)
		m_notStarted = false;
		m_startAction = CMENU_IDLE; // switch back to idle
	}
}