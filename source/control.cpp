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

ConVar ebot_quota("ebot_quota", "10");
ConVar ebot_forceteam("ebot_force_team", "any");
ConVar ebot_auto_players("ebot_auto_players", "-1"); // i don't even know what is this...
ConVar ebot_quota_save("ebot_quota_save", "-1");

ConVar ebot_difficulty("ebot_difficulty", "4");
ConVar ebot_minskill("ebot_min_skill", "1");
ConVar ebot_maxskill("ebot_max_skill", "100");

ConVar ebot_nametag("ebot_name_tag", "2");
ConVar ebot_ping("ebot_fake_ping", "1");
ConVar ebot_display_avatar("ebot_display_avatar", "1");

ConVar ebot_autovacate("ebot_auto_vacate", "1");
ConVar ebot_save_bot_names("ebot_save_bot_names", "0");

ConVar ebot_random_join_quit("ebot_random_join_quit", "0");
ConVar ebot_stay_min("ebot_stay_min", "120"); // 2 minutes
ConVar ebot_stay_max("ebot_stay_max", "3600"); // 1 hours

ConVar ebot_always_use_2d("ebot_always_use_2d_heuristic", "0");
ConVar ebot_heuristic_type("ebot_heuristic_type", "-1");

// this is a bot manager class constructor
BotControl::BotControl(void)
{
	m_lastWinner = -1;

	m_economicsGood[TEAM_TERRORIST] = true;
	m_economicsGood[TEAM_COUNTER] = true;

	cmemset(m_bots, 0, sizeof(m_bots));
	InitQuota();
}

// this is a bot manager class destructor, do not use engine->GetMaxClients () here !!
BotControl::~BotControl(void)
{
	for (int i = 0; i < 32; i++)
	{
		if (m_bots[i])
		{
			delete m_bots[i];
			m_bots[i] = nullptr;
		}
	}
}

// this function calls gamedll player() function, in case to create player entity in game
void BotControl::CallGameEntity(entvars_t* vars)
{
	if (g_isMetamod)
	{
		CALL_GAME_ENTITY(PLID, "player", vars);
		return;
	}

	static EntityPtr_t playerFunction = nullptr;

	if (playerFunction == nullptr)
		playerFunction = (EntityPtr_t)g_gameLib->GetFunctionAddr("player");

	if (playerFunction != nullptr)
		(*playerFunction) (vars);
}

// this function completely prepares bot entity (edict) for creation, creates team, skill, sets name etc, and
// then sends result to bot constructor
int BotControl::CreateBot(String name, int skill, int personality, int team, int member)
{
	edict_t* bot = nullptr;
	if (g_numWaypoints < 1) // don't allow creating bots with no waypoints loaded
	{
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

	if (skill <= 0 || skill > 100)
	{
		if (ebot_difficulty.GetInt() >= 4)
			skill = 100;
		else if (ebot_difficulty.GetInt() == 3)
			skill = CRandomInt(79, 99);
		else if (ebot_difficulty.GetInt() == 2)
			skill = CRandomInt(50, 79);
		else if (ebot_difficulty.GetInt() == 1)
			skill = CRandomInt(30, 50);
		else if (ebot_difficulty.GetInt() == 0)
			skill = CRandomInt(1, 30);
		else
		{
			int maxSkill = ebot_maxskill.GetInt();
			int minSkill = (ebot_minskill.GetInt() == 0) ? 1 : ebot_minskill.GetInt();

			if (maxSkill <= 100 && minSkill > 0)
				skill = CRandomInt(minSkill, maxSkill);
			else
				skill = CRandomInt(0, 100);
		}
	}

	if (personality < 0 || personality > 2)
	{
		int randomPrecent = CRandomInt(1, 3);

		if (randomPrecent == 1)
			personality = PERSONALITY_NORMAL;
		else if (randomPrecent == 2)
			personality = PERSONALITY_RUSHER;
		else
			personality = PERSONALITY_CAREFUL;
	}

	char outputName[33];
	bool addTag = true;

	// restore the bot name
	if (ebot_save_bot_names.GetBool() && !m_savedBotNames.IsEmpty())
	{
		sprintf(outputName, "%s", (char*)m_savedBotNames.Pop());
		addTag = false;
	}
	else if (name.GetLength() <= 0)
	{
		bool getName = false;
		if (!g_botNames.IsEmpty())
		{
			ITERATE_ARRAY(g_botNames, j)
			{
				if (!g_botNames[j].isUsed)
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
				NameItem& botName = g_botNames.GetRandomElement();
				if (!botName.isUsed)
				{
					nameUse = false;
					botName.isUsed = true;
					sprintf(outputName, "%s", (char*)botName.name);
				}
			}
		}
		else
			sprintf(outputName, "e-bot %i", CRandomInt(1, 9999)); // just pick ugly random name
	}
	else
		sprintf(outputName, "%s", (char*)name);

	char botName[64];
	if (ebot_nametag.GetInt() == 2 && addTag)
		snprintf(botName, sizeof(botName), "[E-BOT] %s (%d)", outputName, skill);
	else if (ebot_nametag.GetInt() == 1 && addTag)
		snprintf(botName, sizeof(botName), "[E-BOT] %s", outputName);
	else
		cstrncpy(botName, outputName, sizeof(botName));

	if (FNullEnt((bot = (*g_engfuncs.pfnCreateFakeClient) (botName))))
	{
		CenterPrint(" Unable to create E-Bot, Maximum players reached (%d/%d)", engine->GetMaxClients(), engine->GetMaxClients());
		return -2;
	}

	int index = ENTINDEX(bot) - 1;

	InternalAssert(index >= 0 && index <= 32); // check index
	InternalAssert(m_bots[index] == nullptr); // check bot slot

	m_bots[index] = new Bot(bot, skill, personality, team, member);

	if (m_bots == nullptr)
		return -1;

	ServerPrint("Connecting E-Bot - %s | Skill %d", GetEntityName(bot), skill);

	return index;
}

// this function returns index of bot (using own bot array)
int BotControl::GetIndex(edict_t* ent)
{
	if (FNullEnt(ent))
		return -1;

	int index = ENTINDEX(ent) - 1;
	if (index < 0 || index >= 32)
		return -1;

	if (m_bots[index] != nullptr)
		return index;

	return -1; // if no edict, return -1;
}

// this function returns index of bot (using own bot array)
int BotControl::GetIndex(int index)
{
	if (index < 0 || index >= 32)
		return -1;

	if (m_bots[index] != nullptr)
		return index;

	return -1;
}

// this function finds a bot specified by index, and then returns pointer to it (using own bot array)
Bot* BotControl::GetBot(int index)
{
	if (index < 0 || index >= 32)
		return nullptr;

	if (m_bots[index] != nullptr)
		return m_bots[index];

	return nullptr; // no bot
}

// same as above, but using bot entity
Bot* BotControl::GetBot(edict_t* ent)
{
	return GetBot(GetIndex(ent));
}

// this function finds one bot, alive bot :)
Bot* BotControl::FindOneValidAliveBot(void)
{
	Array <int> foundBots;

	for (const auto& bot : m_bots)
	{
		if (bot != nullptr && bot->m_isAlive)
			foundBots.Push(bot->m_index - 1);
	}

	if (!foundBots.IsEmpty())
		return m_bots[foundBots.GetRandomElement()];

	return nullptr;
}

void BotControl::DoJoinQuitStuff(void)
{
	if (!ebot_random_join_quit.GetBool())
		return;

	// we have reconnecting bots...
	if (ebot_save_bot_names.GetBool() && !m_savedBotNames.IsEmpty())
		AddBot((char*)m_savedBotNames.Pop(), -1, -1, -1, -1);

	if (m_randomJoinTime > engine->GetTime())
		return;

	// add one more
	if (CRandomInt(1, GetHumansNum()) <= 2);
		g_botManager->AddRandom();

	g_botManager->AddRandom();

	if (ebot_stay_min.GetFloat() > ebot_stay_max.GetFloat())
		ebot_stay_min.SetFloat(ebot_stay_max.GetFloat());

	float min = ebot_stay_min.GetFloat() * 2.0f;
	float max = ebot_stay_max.GetFloat() * 0.5f;

	if (min > max)
		max = min * 1.5f;

	m_randomJoinTime = AddTime(CRandomFloat(min, max));
}

void BotControl::Think(void)
{
	g_botManager->DoJoinQuitStuff();

	for (const auto& bot : m_bots)
	{
		if (bot == nullptr)
			continue;

		bot->Think();
		bot->FacePosition();
	}
}

// this function putting bot creation process to queue to prevent engine crashes
void BotControl::AddBot(const String& name, int skill, int personality, int team, int member)
{
	CreateItem queueID;

	// fill the holder
	queueID.name = name;
	queueID.skill = skill;
	queueID.personality = personality;
	queueID.team = team;
	queueID.member = member;

	// put to queue
	m_creationTab.Push(queueID);

	// keep quota number up to date
	if (GetBotsNum() + 1 > ebot_quota.GetInt())
		ebot_quota.SetInt(GetBotsNum() + 1);
}

void BotControl::CheckBotNum(void)
{
	if (ebot_auto_players.GetInt() == -1 && ebot_quota_save.GetInt() == -1)
		return;

	int needBotNumber = 0;
	if (ebot_quota_save.GetInt() != -1)
	{
		if (ebot_quota_save.GetInt() > 32)
			ebot_quota_save.SetInt(32);

		needBotNumber = ebot_quota_save.GetInt();

		File fp(FormatBuffer("%s/addons/ebot/ebot.cfg", GetModName()), "rt+");
		if (fp.IsValid())
		{
			const char quotaCvar[11] = { 's', 'y', 'p', 'b', '_', 'q', 'u', 'o', 't', 'a', ' ' };

			char line[256];
			bool changeed = false;
			while (fp.GetBuffer(line, 255))
			{
				bool trueCvar = true;
				for (int j = 0; (j < 11 && trueCvar); j++)
				{
					if (quotaCvar[j] != line[j])
						trueCvar = false;
				}

				if (!trueCvar)
					continue;

				changeed = true;

				int i = 0;
				for (i = 0; i <= 255; i++)
				{
					if (line[i] == 0)
						break;
				}
				i++;
				fp.Seek(-i, SEEK_CUR);

				if (line[11] == 0 || line[12] == 0 || line[13] == '"' ||
					line[11] == '\n' || line[12] == '\n')
				{
					changeed = false;
					fp.Print("//////////");
					break;
				}

				if (line[11] == '"')
				{
					fp.PutString(FormatBuffer("ebot_quota \"%s%d\"",
						needBotNumber > 10 ? "" : "0", needBotNumber));
				}
				else
					fp.PutString(FormatBuffer("ebot_quota %s%d",
						needBotNumber > 10 ? "" : "0", needBotNumber));

				ServerPrint("ebot_quota save to '%d' - C", needBotNumber);

				break;
			}

			if (!changeed)
			{
				fp.Seek(0, SEEK_END);
				fp.Print(FormatBuffer("\nebot_quota \"%s%d\"\n",
					needBotNumber > 10 ? "" : "0", needBotNumber));
				ServerPrint("ebot_quota save to '%d' - A", needBotNumber);
			}

			fp.Close();
		}
		else
		{
			File fp2(FormatBuffer("%s/addons/ebot/ebot.cfg", GetModName()), "at");
			if (fp2.IsValid())
			{
				fp2.Print(FormatBuffer("\nebot_quota \"%s%d\"\n",
					needBotNumber > 10 ? "" : "0", needBotNumber));
				ServerPrint("ebot_quota save to '%d' - A", needBotNumber);
				fp2.Close();
			}
			else
				ServerPrint("Unknow Problem - Cannot save ebot quota");
		}

		ebot_quota_save.SetInt(-1);
	}

	if (ebot_auto_players.GetInt() != -1)
	{
		if (ebot_auto_players.GetInt() > engine->GetMaxClients())
		{
			ServerPrint("Server Max Clients is %d, You cannot set this value", engine->GetMaxClients());
			ebot_auto_players.SetInt(engine->GetMaxClients());
		}

		needBotNumber = ebot_auto_players.GetInt() - GetHumansNum();
		if (needBotNumber <= 0)
			needBotNumber = 0;
	}

	ebot_quota.SetInt(needBotNumber);
}

int BotControl::AddBotAPI(const String& name, int skill, int team)
{
	if (g_botManager->GetBotsNum() + 1 > ebot_quota.GetInt())
		ebot_quota.SetInt(g_botManager->GetBotsNum() + 1);

	int resultOfCall = CreateBot(name, skill, -1, team, -1);

	// check the result of creation
	if (resultOfCall == -1)
	{
		m_creationTab.RemoveAll(); // something wrong with waypoints, reset tab of creation
		ebot_quota.SetInt(0); // reset quota
		ChartPrint("[E-BOT] You can input [ebot sgdwp on] make the new waypoints!!");
	}
	else if (resultOfCall == -2)
	{
		m_creationTab.RemoveAll(); // maximum players reached, so set quota to maximum players
		ebot_quota.SetInt(GetBotsNum());
	}

	m_maintainTime = AddTime(0.2f);

	return resultOfCall;
}

// this function keeps number of bots up to date, and don't allow to maintain bot creation
// while creation process in process.
void BotControl::MaintainBotQuota(void)
{
	if (!m_creationTab.IsEmpty() && m_maintainTime < engine->GetTime())
	{
		CreateItem last = m_creationTab.Pop();

		int resultOfCall = CreateBot(last.name, last.skill, last.personality, last.team, last.member);

		// check the result of creation
		if (resultOfCall == -1)
		{
			m_creationTab.RemoveAll(); // something wrong with waypoints, reset tab of creation
			ebot_quota.SetInt(0); // reset quota
			
			ChartPrint("[E-BOT] You can input [ebot sgdwp on] make the new waypoints.");
		}
		else if (resultOfCall == -2)
		{
			m_creationTab.RemoveAll(); // maximum players reached, so set quota to maximum players
			ebot_quota.SetInt(GetBotsNum());
		}

		m_maintainTime = AddTime(0.25f);
	}

	if (ebot_random_join_quit.GetBool())
		return;

	g_botManager->CheckBotNum();
	if (m_maintainTime < engine->GetTime())
	{
		int botNumber = GetBotsNum();
		int maxClients = engine->GetMaxClients();
		int desiredBotCount = ebot_quota.GetInt();
		
		if (ebot_autovacate.GetBool())
			desiredBotCount = cmin(desiredBotCount, maxClients - (GetHumansNum() + 1));
		
		if (botNumber > desiredBotCount)
		{
			RemoveRandom();
			m_maintainTime = AddTime(0.25f);
		}
		else if (botNumber < desiredBotCount && botNumber < maxClients)
		{
			AddRandom();
			m_maintainTime = AddTime(0.25f);
		}
		else
		{
			m_maintainTime = AddTime(1.0f);

			if (ebot_save_bot_names.GetBool() && !m_savedBotNames.IsEmpty()) // clear the saved names when quota balancing ended
				m_savedBotNames.Destory();
		}

		if (ebot_quota.GetInt() > maxClients)
			ebot_quota.SetInt(maxClients);
		else if (ebot_quota.GetInt() < 0)
			ebot_quota.SetInt(0);
	}
}

void BotControl::InitQuota(void)
{
	m_maintainTime = AddTime(2.0f);
	m_creationTab.RemoveAll();
	for (int i = 0; i < entityNum; i++)
		SetEntityActionData(i);
}

// this function fill server with bots, with specified team & personality
void BotControl::FillServer(int selection, int personality, int skill, int numToAdd)
{
	// always keep one slot
	int maxClients = ebot_autovacate.GetBool() ? engine->GetMaxClients() - 1 - (IsDedicatedServer() ? 0 : GetHumansNum()) : engine->GetMaxClients();

	if (GetBotsNum() >= maxClients - GetHumansNum())
		return;

	if (selection == 1 || selection == 2)
	{
		CVAR_SET_STRING("mp_limitteams", "0");
		CVAR_SET_STRING("mp_autoteambalance", "0");
	}
	else
		selection = 5;

	char teamDescs[6][12] =
	{
	   "",
	   {"Terrorists"},
	   {"CTs"},
	   "",
	   "",
	   {"Random"},
	};

	int toAdd = numToAdd == -1 ? maxClients - (GetHumansNum() + GetBotsNum()) : numToAdd;

	for (int i = 0; i <= toAdd; i++)
	{
		// since we got constant skill from menu (since creation process call automatic), we need to manually
		// randomize skill here, on given skill there.
		int randomizedSkill = 0;

		if (skill >= 0 && skill <= 20)
			randomizedSkill = CRandomInt(0, 20);
		else if (skill >= 20 && skill <= 40)
			randomizedSkill = CRandomInt(20, 40);
		else if (skill >= 40 && skill <= 60)
			randomizedSkill = CRandomInt(40, 60);
		else if (skill >= 60 && skill <= 80)
			randomizedSkill = CRandomInt(60, 80);
		else if (skill >= 80 && skill <= 99)
			randomizedSkill = CRandomInt(80, 99);
		else if (skill == 100)
			randomizedSkill = skill;

		AddBot("", randomizedSkill, personality, selection, -1);
	}

	ebot_quota.SetInt(toAdd);
	CenterPrint("Filling the server with %s e-bots", &teamDescs[selection][0]);
}

// this function drops all bot clients from server (this function removes only ebots)
void BotControl::RemoveAll(void)
{
	CenterPrint("E-Bots are removed from server");

	for (const auto& bot : m_bots)
	{
		if (bot != nullptr) // is this slot used?
			bot->Kick();
	}

	m_creationTab.RemoveAll();

	// if everyone is kicked, clear the saved bot names
	if (ebot_save_bot_names.GetBool() && !m_savedBotNames.IsEmpty())
		m_savedBotNames.Destory();

	// reset cvars
	ebot_quota.SetInt(0);
	ebot_auto_players.SetInt(-1);
}

// this function remove random bot from specified team (if removeAll value = 1 then removes all players from team)
void BotControl::RemoveFromTeam(Team team, bool removeAll)
{
	for (const auto& bot : m_bots)
	{
		if (bot != nullptr && team == bot->m_team)
		{
			bot->Kick();

			if (!removeAll)
				break;
		}
	}
}

void BotControl::RemoveMenu(edict_t* ent, int selection)
{
	if ((selection > 4) || (selection < 1))
		return;

	char tempBuffer[1024], buffer[1024];
	cmemset(tempBuffer, 0, sizeof(tempBuffer));
	cmemset(buffer, 0, sizeof(buffer));

	int validSlots = (selection == 4) ? (1 << 9) : ((1 << 8) | (1 << 9));
	for (int i = ((selection - 1) * 8); i < selection * 8; ++i)
	{
		if ((m_bots[i] != nullptr) && !FNullEnt(m_bots[i]->GetEntity()))
		{
			validSlots |= 1 << (i - ((selection - 1) * 8));
			sprintf(buffer, "%s %1.1d. %s%s\n", buffer, i - ((selection - 1) * 8) + 1, GetEntityName(m_bots[i]->GetEntity()), GetTeam(m_bots[i]->GetEntity()) == TEAM_COUNTER ? " \\y(CT)\\w" : " \\r(T)\\w");
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
		g_menus[14].validSlots = validSlots & static_cast <unsigned int> (-1);
		g_menus[14].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[14]);
		break;

	case 2:
		g_menus[15].validSlots = validSlots & static_cast <unsigned int> (-1);
		g_menus[15].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[15]);
		break;

	case 3:
		g_menus[16].validSlots = validSlots & static_cast <unsigned int> (-1);
		g_menus[16].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[16]);
		break;

	case 4:
		g_menus[17].validSlots = validSlots & static_cast <unsigned int> (-1);
		g_menus[17].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[17]);
		break;
	}
}

// this function kills all bots on server (only this dll controlled bots)
void BotControl::KillAll(int team)
{
	for (const auto& bot : m_bots)
	{
		if (bot != nullptr)
		{
			if (team != -1 && team != bot->m_team)
				continue;

			bot->Kill();
		}
	}

	CenterPrint("All bots are killed.");
}

// this function removes random bot from server (only ebots)
void BotControl::RemoveRandom(void)
{
	for (const auto& bot : m_bots)
	{
		if (bot != nullptr)  // is this slot used?
		{
			bot->Kick();
			break;
		}
	}
}

// this function sets bots weapon mode
void BotControl::SetWeaponMode(int selection)
{
	int tabMapStandart[7][Const_NumWeapons] =
	{
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Knife only
	   {-1,-1,-1, 2, 2, 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Pistols only
	   {-1,-1,-1,-1,-1,-1,-1, 2, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Shotgun only
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1, 2, 1, 2, 0, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 2,-1}, // Machine Guns only
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 0, 1, 0, 1, 1,-1,-1,-1,-1,-1,-1}, // Rifles only
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 2, 2, 0, 1,-1,-1}, // Snipers only
	   {-1,-1,-1, 2, 2, 0, 1, 2, 2, 2, 1, 2, 0, 2, 0, 0, 1, 0, 1, 1, 2, 2, 0, 1, 2, 1}  // Standard
	};

	int tabMapAS[7][Const_NumWeapons] =
	{
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Knife only
	   {-1,-1,-1, 2, 2, 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Pistols only
	   {-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Shotgun only
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 0, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1}, // Machine Guns only
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1, 1, 0, 1, 1,-1,-1,-1,-1,-1,-1}, // Rifles only
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 0,-1, 1,-1,-1}, // Snipers only
	   {-1,-1,-1, 2, 2, 0, 1, 1, 1, 1, 1, 1, 0, 2, 0,-1, 1, 0, 1, 1, 0, 0,-1, 1, 1, 1}  // Standard
	};

	char modeName[7][12] =
	{
	   {"Knife"},
	   {"Pistol"},
	   {"Shotgun"},
	   {"Machine Gun"},
	   {"Rifle"},
	   {"Sniper"},
	   {"Standard"}
	};
	selection--;

	for (int i = 0; i < Const_NumWeapons; i++)
	{
		g_weaponSelect[i].teamStandard = tabMapStandart[selection][i];
		g_weaponSelect[i].teamAS = tabMapAS[selection][i];
	}

	if (selection == 0)
		ebot_knifemode.SetInt(1);
	else
		ebot_knifemode.SetInt(0);

	CenterPrint("%s weapon mode selected", &modeName[selection][0]);
}

// this function lists bots currently playing on the server
void BotControl::ListBots(void)
{
	ServerPrintNoTag("%-3.5s %-9.13s %-17.18s %-3.4s %-3.4s %-3.4s", "index", "name", "personality", "team", "skill", "frags");

	for (const auto& client : g_clients)
	{
		edict_t* player = client.ent;

		// is this player slot valid
		if (IsValidBot(player) && GetBot(player))
			ServerPrintNoTag("[%-3.1d] %-9.13s %-17.18s %-3.4s %-3.1d %-3.1d", client.index, GetEntityName(player), GetBot(player)->m_personality == PERSONALITY_RUSHER ? "rusher" : GetBot(player)->m_personality == PERSONALITY_NORMAL ? "normal" : "careful", GetTeam(player) != 0 ? "CT" : "T", GetBot(player)->m_skill, static_cast <int> (player->v.frags));
	}
}

// this function returns number of ebot's playing on the server
int BotControl::GetBotsNum(void)
{
	int count = 0;
	for (const auto& bot : m_bots)
	{
		if (bot != nullptr)
			count++;
	}

	return count;
}

// this function returns number of humans playing on the server
int BotControl::GetHumansNum()
{
	int count = 0;
	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (m_bots[client.index] == nullptr)
			count++;
	}

	return count;
}

// this function returns bot with highest frag
Bot* BotControl::GetHighestSkillBot(int team)
{
	Bot* highFragBot = nullptr;

	int bestIndex = 0;
	int bestSkill = -1;

	// search bots in this team
	for (const auto& bot : m_bots)
	{
		if (highFragBot != nullptr && highFragBot->m_team == team)
		{
			if (highFragBot->m_skill > bestSkill)
			{
				bestIndex = bot->m_index - 1;
				bestSkill = highFragBot->m_skill;
			}
		}
	}

	return GetBot(bestIndex);
}

// this function decides is players on specified team is able to buy primary weapons by calculating players
// that have not enough money to buy primary (with economics), and if this result higher 80%, player is can't
// buy primary weapons.
void BotControl::CheckTeamEconomics(int team)
{
	if (GetGameMode() != MODE_BASE)
	{
		m_economicsGood[team] = true;
		return;
	}

	int numPoorPlayers = 0;
	int numTeamPlayers = 0;

	// start calculating
	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (m_bots[client.index] != nullptr && m_bots[client.index]->m_team == team)
		{
			if (m_bots[client.index]->m_moneyAmount < 1600)
				numPoorPlayers++;

			numTeamPlayers++; // update count of team
		}
	}

	m_economicsGood[team] = true;

	if (numTeamPlayers <= 1)
		return;

	// if 80 percent of team have no enough money to purchase primary weapon
	if ((numTeamPlayers * 80) * 0.01 <= numPoorPlayers)
		m_economicsGood[team] = false;

	// winner must buy something!
	if (m_lastWinner == team)
		m_economicsGood[team] = true;
}

// this function free all bots slots (used on server shutdown)
void BotControl::Free(void)
{
	for (int i = 0; i < 32; i++)
	{
		if (m_bots[i] != nullptr)
		{
			if (ebot_save_bot_names.GetBool())
				m_savedBotNames.Push(STRING(m_bots[i]->GetEntity()->v.netname));

			m_bots[i]->m_stayTime = 0.0f;
			delete m_bots[i];
			m_bots[i] = nullptr;
		}
	}
}

// this function frees one bot selected by index (used on bot disconnect)
void BotControl::Free(int index)
{
	m_bots[index]->m_stayTime = 0.0f;
	delete m_bots[index];
	m_bots[index] = nullptr;
}

// this function controls the bot entity
Bot::Bot(edict_t* bot, int skill, int personality, int team, int member)
{
	if (bot == nullptr)
		return;

	char rejectReason[128];
	int clientIndex = ENTINDEX(bot);

	cmemset(reinterpret_cast <void*> (this), 0, sizeof(*this));

	pev = &bot->v;

	if (bot->pvPrivateData != nullptr)
		FREE_PRIVATE(bot);

	bot->pvPrivateData = nullptr;
	bot->v.frags = 0;

	// create the player entity by calling MOD's player function
	BotControl::CallGameEntity(&bot->v);

	// set all info buffer keys for this bot
	char* buffer = GET_INFOKEYBUFFER(bot);
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "_vgui_menus", "0");
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "cl_updaterate", "30");
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "rate ", "3500");
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "cl_dlmax", "16");
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "cl_lc", "0");
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "cl_lw", "0");
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "tracker", "0");
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "dm", "0");
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "_ah", "0");
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "friends", "0");

	if (g_gameVersion == HALFLIFE)
	{
		char c_topcolor[4], c_bottomcolor[4];
		sprintf(c_topcolor, "%d", CRandomInt(1, 254));
		sprintf(c_bottomcolor, "%d", CRandomInt(1, 254));
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "topcolor", c_topcolor);
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "bottomcolor", c_bottomcolor);
	}
	else // we hate this, let bot pick weapon by itself... when you buy/pickup weapon it will select the slot but we dont want this.
	{
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "cl_autowepswitch", "0");
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "_cl_autowepswitch", "0");
	}

	if (g_gameVersion != CSVER_VERYOLD && !ebot_ping.GetBool())
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "*bot", "1");

	rejectReason[0] = 0; // reset the reject reason template string
	MDLL_ClientConnect(bot, "E-BOT", FormatBuffer("%d.%d.%d.%d", CRandomInt(1, 255), CRandomInt(1, 255), CRandomInt(1, 255), CRandomInt(1, 255)), rejectReason);

	// should be set after client connect
	if (ebot_display_avatar.GetBool() && !g_botManager->m_avatars.IsEmpty())
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "*sid", g_botManager->m_avatars.GetRandomElement());

	if (!IsNullString(rejectReason))
	{
		AddLogEntry(LOG_WARNING, "Server refused '%s' connection (%s)", GetEntityName(bot), rejectReason);
		ServerCommand("kick \"%s\"", GetEntityName(bot)); // kick the bot player if the server refused it
		bot->v.flags |= FL_KILLME;
	}

	MDLL_ClientPutInServer(bot);
	bot->v.flags |= FL_CLIENT;

	// initialize all the variables for this bot...
	m_notStarted = true;  // hasn't joined game yet
	m_difficulty = ebot_difficulty.GetInt(); // set difficulty
	m_basePingLevel = CRandomInt(11, 111);

	m_startAction = CMENU_IDLE;
	m_moneyAmount = 0;
	m_logotypeIndex = CRandomInt(0, 5);

	// initialize msec value
	m_msecInterval = engine->GetTime();
	m_msecVal = g_pGlobals->frametime * 1000.0f;

	// assign how talkative this bot will be
	m_sayTextBuffer.chatDelay = CRandomFloat(3.8f, 10.0f);
	m_sayTextBuffer.chatProbability = CRandomInt(1, 100);

	m_isAlive = false;
	m_skill = skill;
	m_weaponBurstMode = BURST_DISABLED;

	m_lastThinkTime = engine->GetTime();
	m_frameInterval = engine->GetTime();

	switch (personality)
	{
	case 1:
		m_personality = PERSONALITY_RUSHER;
		m_baseAgressionLevel = CRandomFloat(0.8f, 1.2f);
		m_baseFearLevel = CRandomFloat(0.0f, 0.5f);
		break;

	case 2:
		m_personality = PERSONALITY_CAREFUL;
		m_baseAgressionLevel = CRandomFloat(0.0f, 0.3f);
		m_baseFearLevel = CRandomFloat(0.75f, 1.0f);
		break;

	default:
		m_personality = PERSONALITY_NORMAL;
		m_baseAgressionLevel = CRandomFloat(0.4f, 0.8f);
		m_baseFearLevel = CRandomFloat(0.4f, 0.8f);
		break;
	}

	cmemset(&m_ammoInClip, 0, sizeof(m_ammoInClip));
	cmemset(&m_ammo, 0, sizeof(m_ammo));

	m_currentWeapon = 0; // current weapon is not assigned at start
	m_voicePitch = CRandomInt(80, 120); // assign voice pitch

	m_agressionLevel = m_baseAgressionLevel;
	m_fearLevel = m_baseFearLevel;
	m_nextEmotionUpdate = engine->GetTime() + 0.5f;

	// just to be sure
	m_actMessageIndex = 0;
	m_pushMessageIndex = 0;

	// assign team and class
	m_wantedTeam = team;
	m_wantedClass = member;

	NewRound();
}

Bot::~Bot(void)
{
	// SwitchChatterIcon (false); // crash on CTRL+C'ing win32 console hlds
	DeleteSearchNodes();
	ResetTasks();

	char botName[64];
	ITERATE_ARRAY(g_botNames, j)
	{
		sprintf(botName, "[E-BOT] %s", (char*)g_botNames[j].name);

		if (cstrcmp(g_botNames[j].name, GetEntityName(GetEntity())) == 0 || cstrcmp(botName, GetEntityName(GetEntity())) == 0)
		{
			g_botNames[j].isUsed = false;
			break;
		}
	}
}

// this function initializes a bot after creation & at the start of each round
void Bot::NewRound(void)
{
	if (ebot_random_join_quit.GetBool() && m_stayTime > 1.0f && m_stayTime < engine->GetTime())
	{
		Kick();
		return;
	}

	if (ebot_always_use_2d.GetBool())
		m_2dH = true;
	else
		m_2dH = static_cast<bool>(CRandomInt(0, 1));

	if (ebot_heuristic_type.GetInt() >= 1 && ebot_heuristic_type.GetInt() <= 4)
		m_heuristic = ebot_heuristic_type.GetInt();
	else
		m_heuristic = CRandomInt(1, 4);

	int i = 0;

	// delete all allocated path nodes
	DeleteSearchNodes();
	m_itaimstart = engine->GetTime();
	m_aimStopTime = engine->GetTime();
	m_weaponSelectDelay = engine->GetTime();
	m_currentWaypointIndex = -1;
	m_currentTravelFlags = 0;
	m_destOrigin = nullvec;
	m_prevGoalIndex = -1;
	m_chosenGoalIndex = -1;
	m_myMeshWaypoint = -1;
	m_loosedBombWptIndex = -1;

	m_duckDefuse = false;
	m_duckDefuseCheckTime = 0.0f;

	m_prevWptIndex = -1;

	m_navTimeset = engine->GetTime();

	// clear all states & tasks
	m_states = 0;
	ResetTasks();

	m_isVIP = false;
	m_isLeader = false;
	m_hasProgressBar = false;
	m_canChooseAimDirection = true;

	m_timeTeamOrder = 0.0f;
	m_askCheckTime = 0.0f;
	m_minSpeed = 260.0f;
	m_prevSpeed = 0.0f;
	m_prevOrigin = Vector(9999.0f, 9999.0f, 9999.0f);
	m_prevTime = engine->GetTime();
	m_blindRecognizeTime = engine->GetTime();

	m_viewDistance = 4096.0f;
	m_maxViewDistance = 4096.0f;

	m_pickupItem = nullptr;
	m_itemIgnore = nullptr;
	m_itemCheckTime = 0.0f;

	m_breakableEntity = nullptr;
	m_breakable = nullvec;
	m_timeDoorOpen = 0.0f;

	ResetCollideState();
	ResetDoubleJumpState();

	SetEnemy(nullptr);
	SetLastEnemy(nullptr);
	SetMoveTarget(nullptr);
	m_trackingEdict = nullptr;
	m_timeNextTracking = 0.0f;

	m_buttonPushTime = 0.0f;
	m_enemyUpdateTime = 0.0f;
	m_seeEnemyTime = 0.0f;
	m_oldCombatDesire = 0.0f;

	m_avoidEntity = nullptr;
	m_needAvoidEntity = 0;

	m_lastDamageType = -1;
	m_voteMap = 0;
	m_doorOpenAttempt = 0;
	m_aimFlags = 0;

	m_position = nullvec;
	m_campposition = nullvec;

	m_idealReactionTime = g_skillTab[m_skill / 20].minSurpriseTime;
	m_actualReactionTime = g_skillTab[m_skill / 20].minSurpriseTime;

	m_targetEntity = nullptr;
	m_followWaitTime = 0.0f;

	for (i = 0; i < Const_MaxHostages; i++)
		m_hostages[i] = nullptr;

	m_isReloading = false;
	m_reloadState = RSTATE_NONE;

	m_reloadCheckTime = 0.0f;
	m_shootTime = engine->GetTime();
	m_playerTargetTime = engine->GetTime();
	m_firePause = 0.0f;

	m_grenadeCheckTime = 0.0f;
	m_isUsingGrenade = false;

	m_blindButton = 0;
	m_blindTime = 0.0f;
	m_jumpTime = 0.0f;
	m_isStuck = false;
	m_jumpFinished = false;

	m_sayTextBuffer.timeNextChat = engine->GetTime();
	m_sayTextBuffer.entityIndex = -1;
	m_sayTextBuffer.sayText[0] = 0x0;

	m_damageTime = 0.0f;
	m_zhCampPointIndex = -1;
	m_checkCampPointTime = 0.0f;

	if (!IsAlive(GetEntity())) // if bot died, clear all weapon stuff and force buying again
	{
		cmemset(&m_ammoInClip, 0, sizeof(m_ammoInClip));
		cmemset(&m_ammo, 0, sizeof(m_ammo));
		m_currentWeapon = 0;
	}

	m_nextBuyTime = AddTime(CRandomFloat(0.6f, 1.2f));
	m_inBombZone = false;

	m_shieldCheckTime = 0.0f;
	m_zoomCheckTime = 0.0f;
	m_strafeSetTime = 0.0f;
	m_combatStrafeDir = 0;
	m_fightStyle = 0;
	m_lastFightStyleCheck = 0.0f;

	m_checkWeaponSwitch = true;
	m_checkKnifeSwitch = true;

	m_radioEntity = nullptr;
	m_radioOrder = 0;
	m_defendedBomb = false;

	m_timeLogoSpray = AddTime(CRandomFloat(0.5f, 2.0f));
	m_spawnTime = engine->GetTime();
	m_lastChatTime = engine->GetTime();
	pev->button = 0;

	m_timeCamping = 0;
	m_campDirection = 0;
	m_nextCampDirTime = 0;
	m_campButtons = 0;

	m_soundUpdateTime = 0.0f;
	m_heardSoundTime = engine->GetTime() - 8.0f;

	// clear its message queue
	for (i = 0; i < 32; i++)
		m_messageQueue[i] = CMENU_IDLE;

	m_actMessageIndex = 0;
	m_pushMessageIndex = 0;
	
	SetEntityWaypoint(GetEntity(), -2);

	// and put buying into its message queue
	if (g_gameVersion == HALFLIFE)
	{
		m_buyState = 7;
		m_buyingFinished = true;
		m_startAction = CMENU_IDLE;
	}
	else
	{
		m_buyingFinished = false;
		m_buyState = 0;
		PushMessageQueue(CMENU_BUY);
	}

	PushTask(TASK_NORMAL, TASKPRI_NORMAL, -1, 1.0f, true);

	// hear range based on difficulty
	m_maxhearrange = float(m_skill * CRandomFloat(7.0f, 15.0f));
	m_moveSpeed = pev->maxspeed;

	m_tempstrafeSpeed = CRandomInt(1, 2) == 1 ? pev->maxspeed : -pev->maxspeed;
}

// this function kills a bot (not just using ClientKill, but like the CSBot does)
// base code courtesy of Lazy (from bots-united forums!)
void Bot::Kill(void)
{
	edict_t* hurtEntity = (*g_engfuncs.pfnCreateNamedEntity) (MAKE_STRING("trigger_hurt"));

	if (FNullEnt(hurtEntity))
		return;

	hurtEntity->v.classname = MAKE_STRING(g_weaponDefs[m_currentWeapon].className);
	hurtEntity->v.dmg_inflictor = GetEntity();
	hurtEntity->v.dmg = 999999.0f;
	hurtEntity->v.dmg_take = 1.0f;
	hurtEntity->v.dmgtime = 2.0f;
	hurtEntity->v.effects |= EF_NODRAW;

	(*g_engfuncs.pfnSetOrigin) (hurtEntity, Vector(-4000, -4000, -4000));

	KeyValueData kv;
	kv.szClassName = const_cast <char*> (g_weaponDefs[m_currentWeapon].className);
	kv.szKeyName = "damagetype";
	kv.szValue = FormatBuffer("%d", (1 << 4));
	kv.fHandled = false;

	MDLL_KeyValue(hurtEntity, &kv);

	MDLL_Spawn(hurtEntity);
	MDLL_Touch(hurtEntity, GetEntity());

	(*g_engfuncs.pfnRemoveEntity) (hurtEntity);
}

void Bot::Kick(void)
{
	auto myName = GetEntityName(GetEntity());
	if (IsNullString(myName))
		return;

	ServerCommand("kick \"%s\"", GetEntityName(GetEntity()));
	CenterPrint("E-Bot '%s' kicked from the server", GetEntityName(GetEntity()));

	if (g_botManager->GetBotsNum() - 1 < ebot_quota.GetInt())
		ebot_quota.SetInt(g_botManager->GetBotsNum() - 1);

	//if (ebot_save_bot_names.GetBool() && !g_botManager->m_savedBotNames.IsEmpty()) CRASH...
	//	g_botManager->m_savedBotNames.PopNoReturn();
}

// this function handles the selection of teams & class
void Bot::StartGame(void)
{
	if (g_gameVersion == HALFLIFE)
	{
		if (CRandomInt(1, 3) == 1)
			ChatMessage(CHAT_HELLO);

		m_notStarted = false;
		m_startAction = CMENU_IDLE;
		return;
	}

	// handle counter-strike stuff here...
	if (m_startAction == CMENU_TEAM)
	{
		m_startAction = CMENU_IDLE;  // switch back to idle

		if (ebot_forceteam.GetString()[0] == 'C' || ebot_forceteam.GetString()[0] == 'c' ||
			ebot_forceteam.GetString()[0] == '2')
			m_wantedTeam = 2;
		else if (ebot_forceteam.GetString()[0] == 'T' || ebot_forceteam.GetString()[0] == 't' ||
			ebot_forceteam.GetString()[0] == '1') // 1 = T, 2 = CT
			m_wantedTeam = 1;

		if (m_wantedTeam != 1 && m_wantedTeam != 2)
			m_wantedTeam = 5;

		// select the team the bot wishes to join...
		FakeClientCommand(GetEntity(), "menuselect %d", m_wantedTeam);
	}
	else if (m_startAction == CMENU_CLASS)
	{
		m_startAction = CMENU_IDLE;  // switch back to idle

		int maxChoice = g_gameVersion == CSVER_CZERO ? 5 : 4;
		m_wantedClass = CRandomInt(1, maxChoice);

		// select the class the bot wishes to use...
		FakeClientCommand(GetEntity(), "menuselect %d", m_wantedClass);

		// bot has now joined the game (doesn't need to be started)
		m_notStarted = false;

		// check for greeting other players, since we connected
		if (CRandomInt(1, 3) == 1)
			ChatMessage(CHAT_HELLO);
	}
	else if ((m_team == TEAM_COUNTER || m_team == TEAM_TERRORIST) && IsAlive(GetEntity())) // something is wrong...
	{
		if (CRandomInt(1, 3) == 1)
			ChatMessage(CHAT_HELLO);

		m_notStarted = false;
		m_startAction = CMENU_IDLE;
	}
}
