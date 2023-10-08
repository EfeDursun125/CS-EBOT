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

ConVar ebot_difficulty("ebot_difficulty", "4");
ConVar ebot_minskill("ebot_min_skill", "1");
ConVar ebot_maxskill("ebot_max_skill", "100");

ConVar ebot_nametag("ebot_name_tag", "2");
ConVar ebot_ping("ebot_fake_ping", "1");
ConVar ebot_display_avatar("ebot_display_avatar", "1");

ConVar ebot_keep_slots("ebot_keep_slots", "1");
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

	m_economicsGood[Team::Terrorist] = true;
	m_economicsGood[Team::Counter] = true;

	cmemset(m_bots, 0, sizeof(m_bots));
	InitQuota();
}

// this is a bot manager class destructor, do not use engine->GetMaxClients () here !!
BotControl::~BotControl(void)
{
	for (auto& bot : m_bots)
	{
		if (bot == nullptr)
			continue;

		delete bot;
		bot = nullptr;
	}
}

// this function calls gamedll player() function, in case to create player entity in game
void BotControl::CallGameEntity(entvars_t* vars)
{
	CALL_GAME_ENTITY(PLID, "player", vars);
}

// this function completely prepares bot entity (edict) for creation, creates team, skill, sets name etc, and
// then sends result to bot constructor
int BotControl::CreateBot(String name, int skill, int personality, const int team, const int member)
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
			skill = crandomint(79, 99);
		else if (ebot_difficulty.GetInt() == 2)
			skill = crandomint(50, 79);
		else if (ebot_difficulty.GetInt() == 1)
			skill = crandomint(30, 50);
		else if (ebot_difficulty.GetInt() == 0)
			skill = crandomint(1, 30);
		else
		{
			const int maxSkill = ebot_maxskill.GetInt();
			const int minSkill = (ebot_minskill.GetInt() == 0) ? 1 : ebot_minskill.GetInt();

			if (maxSkill <= 100 && minSkill > 0)
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
			sprintf(outputName, "e-bot %i", crandomint(1, 9999)); // just pick ugly random name
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

	const int index = ENTINDEX(bot) - 1;
	m_bots[index] = new Bot(bot, skill, personality, team, member);
	if (!m_bots[index])
	{
		AddLogEntry(Log::Memory, "unexpected memory error");
		return -1;
	}

	auto ebotName = GetEntityName(bot);
	ServerPrint("Connecting E-Bot - %s | Skill %d", ebotName, skill);

	// set values
	m_bots[index]->m_senseChance = crandomint(10, 90);
	m_bots[index]->m_hasProfile = false;

	m_bots[index]->m_favoritePrimary.Destroy();
	m_bots[index]->m_favoriteSecondary.Destroy();
	m_bots[index]->m_favoriteStuff.Destroy();

	if (g_gameVersion & Game::CStrike || g_gameVersion & Game::CZero)
	{
		int i;
		char* folder = FormatBuffer("%s/addons/ebot/profiles", GetModName());
		CreatePath(folder);
		const char* filePath = FormatBuffer("%s/%s.ep", folder, ebotName);
		File file(filePath, "rt+");
		if (file.IsValid())
		{
			char line[255];
			while (file.GetBuffer(line, 255))
			{
				if ((line[0] == '/') || (line[0] == '\r') || (line[0] == '\n') || (line[0] == 0) || (line[0] == ' ') || (line[0] == '\t'))
					continue;

				Array <String> pair = String(line).Split('=');

				if (pair.GetElementNumber() != 2)
					continue;

				pair[0].Trim().Trim();
				pair[1].Trim().Trim();

				if (pair[0] == "Personaltiy")
				{
					if (pair[1] == "Rusher")
						m_bots[index]->m_personality = Personality::Rusher;
					else if (pair[1] == "Careful")
						m_bots[index]->m_personality = Personality::Careful;
					else if (pair[1] == "Normal")
						m_bots[index]->m_personality = Personality::Normal;
					else
						m_bots[index]->m_personality = personality;
				}
				else if (pair[0] == "FavoritePrimary")
				{
					Array <String> splitted = pair[1].Split(',');
					for (i = 0; i < splitted.GetElementNumber(); i++)
						m_bots[index]->m_favoritePrimary.Push(splitted[i].Trim().Trim());
				}
				else if (pair[0] == "FavoriteSecondary")
				{
					Array <String> splitted = pair[1].Split(',');
					for (i = 0; i < splitted.GetElementNumber(); i++)
						m_bots[index]->m_favoriteSecondary.Push(splitted[i].Trim().Trim());
				}
				else if (pair[0] == "FavoriteStuff")
				{
					Array <String> splitted = pair[1].Split(',');
					for (i = 0; i < splitted.GetElementNumber(); i++)
						m_bots[index]->m_favoriteStuff.Push(splitted[i].Trim().Trim());
				}
				else if (ebot_display_avatar.GetBool() && pair[0] == "SteamAvatar")
					SET_CLIENT_KEYVALUE(index, GET_INFOKEYBUFFER(bot), "*sid", pair[1]);
			}

			ServerPrint("E-Bot profile loaded for %s!", ebotName);
			m_bots[index]->m_hasProfile = true;

			file.Close(); // Close the file after reading
		}

		if (!m_bots[index]->m_hasProfile)
		{
			if (crandomint(1, 4) == 1)
				m_bots[index]->m_favoritePrimary.Push("m249");

			if (crandomint(1, 3) == 1)
				m_bots[index]->m_favoritePrimary.Push("g3sg1");

			if (crandomint(1, 3) == 1)
				m_bots[index]->m_favoritePrimary.Push("sg550");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("awp");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("sg552");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("aug");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("ak47");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("m4a1");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("xm1014");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("scout");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("famas");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("galil");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("m3");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("ump45");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("mp5");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("mac10");

			if (crandomint(1, 2) == 1)
				m_bots[index]->m_favoritePrimary.Push("tmp");

			if (crandomint(1, 3) == 1)
				m_bots[index]->m_favoritePrimary.Push("shield");

			if (crandomint(1, 3) == 1)
				m_bots[index]->m_favoriteSecondary.Push("deagle");
			else
			{
				const int id = crandomint(1, 5);
				if (id == 1)
					m_bots[index]->m_favoriteSecondary.Push("fiveseven");
				else if (id == 2)
					m_bots[index]->m_favoriteSecondary.Push("elites");
				else if (id == 3)
					m_bots[index]->m_favoriteSecondary.Push("p228");
				else if (id == 4)
					m_bots[index]->m_favoriteSecondary.Push("glock");
				else
					m_bots[index]->m_favoriteSecondary.Push("usp");
			}
		}

		if (!m_bots[index]->m_favoritePrimary.IsEmpty())
		{
			for (i = 0; i < m_bots[index]->m_favoritePrimary.GetElementNumber(); i++)
				m_bots[index]->m_weaponPrefs[i] = m_bots[index]->GetWeaponID(m_bots[index]->m_favoritePrimary.GetAt(i));
		}

		if (!m_bots[index]->m_favoriteSecondary.IsEmpty())
		{
			for (i = 0; i < m_bots[index]->m_favoriteSecondary.GetElementNumber(); i++)
				m_bots[index]->m_weaponPrefs[i] = m_bots[index]->GetWeaponID(m_bots[index]->m_favoriteSecondary.GetAt(i));
		}
	}
	else if (g_gameVersion & Game::HalfLife)
	{
		m_bots[index]->m_weaponPrefs[0] = WeaponHL::Snark;
		m_bots[index]->m_weaponPrefs[1] = WeaponHL::Rpg;
		m_bots[index]->m_weaponPrefs[1] = WeaponHL::HandGrenade;
		m_bots[index]->m_weaponPrefs[2] = WeaponHL::Egon;
		m_bots[index]->m_weaponPrefs[3] = WeaponHL::Gauss;
		m_bots[index]->m_weaponPrefs[5] = WeaponHL::Crossbow;
		m_bots[index]->m_weaponPrefs[6] = WeaponHL::Shotgun;
		m_bots[index]->m_weaponPrefs[7] = WeaponHL::Mp5_HL;
		m_bots[index]->m_weaponPrefs[7] = WeaponHL::HornetGun;
		m_bots[index]->m_weaponPrefs[8] = WeaponHL::Python;
		m_bots[index]->m_weaponPrefs[9] = WeaponHL::Glock;
	}

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

	if (m_bots[index] != nullptr)
		return index;

	return -1; // if no edict, return -1;
}

// this function returns index of bot (using own bot array)
int BotControl::GetIndex(const int index)
{
	if (index < 0 || index >= 32)
		return -1;

	if (m_bots[index] != nullptr)
		return index;

	return -1;
}

// this function finds a bot specified by index, and then returns pointer to it (using own bot array)
Bot* BotControl::GetBot(const int index)
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
			foundBots.Push(bot->GetIndex() - 1);
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
		AddBot(m_savedBotNames.Pop(), -1, -1, -1, -1);

	const float time = engine->GetTime();
	if (g_randomJoinTime > time)
		return;

	// add one more
	if (crandomint(1, GetHumansNum()) <= 2);
		AddRandom();

	AddRandom();

	if (ebot_stay_min.GetFloat() > ebot_stay_max.GetFloat())
		ebot_stay_min.SetFloat(ebot_stay_max.GetFloat());

	const float min = ebot_stay_min.GetFloat() * 2.0f;
	float max = ebot_stay_max.GetFloat() * 0.5f;

	if (min > max)
		max = min * 1.5f;

	g_randomJoinTime = time + crandomfloat(min, max);
}

void BotControl::Think(void)
{
	DoJoinQuitStuff();

	for (const auto& bot : m_bots)
	{
		if (!bot)
			continue;

		bot->BaseUpdate();
	}
}

// this function putting bot creation process to queue to prevent engine crashes
void BotControl::AddBot(const String& name, const int skill, const int personality, const int team, const int member)
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
	const int botsNum = GetBotsNum() + 1;
	if (botsNum > ebot_quota.GetInt())
		ebot_quota.SetInt(botsNum);
}

int BotControl::AddBotAPI(const String& name, const int skill,const  int team)
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

		m_maintainTime = time + 0.24f;
	}

	if (ebot_random_join_quit.GetBool())
		return;

	if (m_maintainTime < time)
	{
		const int maxClients = engine->GetMaxClients() - ebot_keep_slots.GetInt();
		const int botNumber = GetBotsNum();

		if (botNumber > ebot_quota.GetInt())
			RemoveRandom();

		if (botNumber < ebot_quota.GetInt() && botNumber < maxClients)
		{
			AddRandom();
			m_maintainTime = time + 0.16f;
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

	int i;
	for (i = 0; i < entityNum; i++)
		SetEntityActionData(i);
}

// this function fill server with bots, with specified team & personality
void BotControl::FillServer(int selection, const int personality, const int skill, const int numToAdd)
{
	const int maxClients = engine->GetMaxClients();
	const int getHumansNum = GetHumansNum();
	const int getBotsNum = GetBotsNum();

	if (getBotsNum >= maxClients - getHumansNum)
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

	const int toAdd = numToAdd == -1 ? maxClients - (getHumansNum + getBotsNum) : numToAdd;

	for (int i = 0; i <= toAdd; i++)
	{
		// since we got constant skill from menu (since creation process call automatic), we need to manually
		// randomize skill here, on given skill there.
		int randomizedSkill = 0;

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
		if (bot != nullptr) // is this slot used?
			bot->Kick();
	}

	m_creationTab.Destroy();

	// if everyone is kicked, clear the saved bot names
	if (ebot_save_bot_names.GetBool() && !m_savedBotNames.IsEmpty())
		m_savedBotNames.Destroy();

	// reset cvars
	ebot_quota.SetInt(0);
}

// this function remove random bot from specified team (if removeAll value = 1 then removes all players from team)
void BotControl::RemoveFromTeam(const Team team, const bool removeAll)
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

void BotControl::RemoveMenu(edict_t* ent, const int selection)
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
			sprintf(buffer, "%s %1.1d. %s%s\n", buffer, i - ((selection - 1) * 8) + 1, GetEntityName(m_bots[i]->GetEntity()), GetTeam(m_bots[i]->GetEntity()) == Team::Counter ? " \\y(CT)\\w" : " \\r(T)\\w");
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
		g_menus[14].validSlots = validSlots & static_cast<unsigned int>(-1);
		g_menus[14].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[14]);
		break;

	case 2:
		g_menus[15].validSlots = validSlots & static_cast<unsigned int>(-1);
		g_menus[15].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[15]);
		break;

	case 3:
		g_menus[16].validSlots = validSlots & static_cast<unsigned int>(-1);
		g_menus[16].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[16]);
		break;

	case 4:
		g_menus[17].validSlots = validSlots & static_cast<unsigned int>(-1);
		g_menus[17].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[17]);
		break;
	}
}

// this function kills all bots on server (only this dll controlled bots)
void BotControl::KillAll(const int team)
{
	for (const auto &bot : m_bots)
	{
		if (bot != nullptr)
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

	for (const auto &client : g_clients)
	{
		if (FNullEnt(client.ent))
			continue;

		edict_t* player = client.ent;

		// is this player slot valid
		if (IsValidBot(player))
			ServerPrintNoTag("[%-3.1d] %-9.13s %-17.18s %-3.4s %-3.1d %-3.1d", client.index, GetEntityName(player), GetBot(player)->m_personality == Personality::Rusher ? "Rusher" : GetBot(player)->m_personality == Personality::Normal ? "Normal" : "Careful", GetTeam(player) != 0 ? "CT" : "TR", GetBot(player)->m_skill, static_cast<int>(player->v.frags));
	}
}

// this function returns number of ebot's playing on the server
int BotControl::GetBotsNum(void)
{
	int count = 0;
	for (const auto &bot : m_bots)
	{
		if (bot != nullptr)
			count++;
	}

	return count;
}

// this function returns number of humans playing on the server
int BotControl::GetHumansNum(void)
{
	int count = 0;
	for (const auto &client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (FNullEnt(client.ent))
			continue;

		if (!m_bots[client.index])
			count++;
	}

	return count;
}

// this function returns bot with highest frag
Bot* BotControl::GetHighestSkillBot(const int team)
{
	Bot* highFragBot = nullptr;

	int bestIndex = 0;
	int bestSkill = -1;

	// search bots in this team
	for (const auto &bot : m_bots)
	{
		if (highFragBot != nullptr && (team == Team::Count || highFragBot->m_team == team))
		{
			if (highFragBot->m_skill > bestSkill)
			{
				bestIndex = bot->GetIndex() - 1;
				bestSkill = highFragBot->m_skill;
			}
		}
	}

	return GetBot(bestIndex);
}

void BotControl::SelectLeaderEachTeam(const int team)
{
	Bot* botLeader = nullptr;

	if (GetGameMode() == GameMode::Original || GetGameMode() == GameMode::TeamDeathmatch)
	{
		if (team == Team::Terrorist)
		{
			botLeader = g_botManager->GetHighestSkillBot(team);

			if (botLeader != nullptr)
			{
				botLeader->m_isLeader = true;
				botLeader->RadioMessage(Radio::FollowMe);
			}
		}
		else if (team == Team::Counter)
		{
			botLeader = g_botManager->GetHighestSkillBot(team);

			if (botLeader != nullptr)
			{
				botLeader->m_isLeader = true;
				botLeader->RadioMessage(Radio::FollowMe);
			}
		}
	}
}

// this function decides is players on specified team is able to buy primary weapons by calculating players
// that have not enough money to buy primary (with economics), and if this result higher 80%, player is can't
// buy primary weapons.
void BotControl::CheckTeamEconomics(const int team)
{
	if (GetGameMode() != GameMode::Original)
	{
		m_economicsGood[team] = true;
		return;
	}

	int numPoorPlayers = 0;
	int numTeamPlayers = 0;

	// start calculating
	for (const auto &client : g_clients)
	{
		if (FNullEnt(client.ent))
			continue;

		if (m_bots[client.index] != nullptr && m_bots[client.index]->m_team == team)
		{
			if (m_bots[client.index]->m_moneyAmount < 2000)
				numPoorPlayers++;

			numTeamPlayers++; // update count of team
		}
	}

	m_economicsGood[team] = true;

	if (numTeamPlayers <= 1)
		return;

	// if 75 percent of team have no enough money to purchase primary weapon
	if (float(numTeamPlayers * 75) * 0.01f <= float(numPoorPlayers))
		m_economicsGood[team] = false;

	// winner must buy something!
	if (m_lastWinner == team)
		m_economicsGood[team] = true;
}

// this function free all bots slots (used on server shutdown)
void BotControl::Free(void)
{
	for (auto& bot : m_bots)
	{
		if (bot == nullptr)
			continue;

		if (ebot_save_bot_names.GetBool())
			m_savedBotNames.Push(STRING(bot->GetEntity()->v.netname));

		bot->m_stayTime = -1.0f;
		delete bot;
		bot = nullptr;
	}
}

// this function frees one bot selected by index (used on bot disconnect)
void BotControl::Free(const int index)
{
	if (index < 0 || index >= 32)
		return;

	m_bots[index]->m_stayTime = -1.0f;
	delete m_bots[index];
	m_bots[index] = nullptr;
}

// this function controls the bot entity
Bot::Bot(edict_t* bot, const int skill, const int personality, const int team, const int member)
{
	if (bot == nullptr)
		return;

	char rejectReason[128];
	const int clientIndex = ENTINDEX(bot);
	const float time = engine->GetTime();

	cmemset(reinterpret_cast<void*>(this), 0, sizeof(*this));

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

	if (g_gameVersion & Game::HalfLife)
	{
		char c_topcolor[4], c_bottomcolor[4], c_model[32];
		sprintf(c_topcolor, "%d", crandomint(0, 255));
		sprintf(c_bottomcolor, "%d", crandomint(0, 255));
		Array <String> models = String("barney,gina,gman,gordon,helmet,hgrunt,recon,robo,scientist,zombie").Split(",");
		sprintf(c_model, "%s", models[crandomint(0, models.GetElementNumber() - 1)].GetBuffer());
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "topcolor", c_topcolor);
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "bottomcolor", c_bottomcolor);
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "model", c_model);
	}
	else // we hate this, let bot pick weapon by itself... when you buy/pickup weapon it will select the slot but we dont want this.
	{
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "cl_autowepswitch", "0");
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "_cl_autowepswitch", "0");
	}

	if (!ebot_ping.GetBool())
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "*bot", "1");

	rejectReason[0] = 0; // reset the reject reason template string
	MDLL_ClientConnect(bot, "E-BOT", FormatBuffer("%d.%d.%d.%d", crandomint(1, 255), crandomint(1, 255), crandomint(1, 255), crandomint(1, 255)), rejectReason);

	// should be set after client connect
	if (ebot_display_avatar.GetBool() && !g_botManager->m_avatars.IsEmpty())
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "*sid", g_botManager->m_avatars.GetRandomElement());

	if (!IsNullString(rejectReason))
	{
		const char* name = GetEntityName(bot);
		AddLogEntry(Log::Warning, "Server refused '%s' connection (%s)", name, rejectReason);
		ServerCommand("kick \"%s\"", name); // kick the bot player if the server refused it
		bot->v.flags |= FL_KILLME;
	}

	MDLL_ClientPutInServer(bot);
	bot->v.flags |= FL_FAKECLIENT;

	// initialize all the variables for this bot...
	m_notStarted = true;  // hasn't joined game yet
	m_difficulty = ebot_difficulty.GetInt(); // set difficulty
	m_basePingLevel = crandomint(11, 111);

	m_startAction = CMENU_IDLE;
	m_moneyAmount = 0;

	// initialize msec value
	m_msecInterval = time;

	// assign how talkative this bot will be
	m_sayTextBuffer.chatDelay = crandomfloat(3.8f, 10.0f);
	m_sayTextBuffer.chatProbability = crandomint(1, 100);

	m_isAlive = false;
	m_skill = skill;
	m_weaponBurstMode = BurstMode::Disabled;

	m_frameInterval = time;

	switch (personality)
	{
	case 1:
		m_personality = Personality::Rusher;
		break;

	case 2:
		m_personality = Personality::Careful;
		break;

	default:
		m_personality = Personality::Normal;
	}

	cmemset(&m_ammoInClip, 0, sizeof(m_ammoInClip));
	cmemset(&m_ammo, 0, sizeof(m_ammo));
	m_currentWeapon = 0; // current weapon is not assigned at start

	// just to be sure
	m_actMessageIndex = 0;
	m_pushMessageIndex = 0;

	// init path
	m_navNode.Init(g_numWaypoints + 32);

	// assign team and class
	m_wantedTeam = team;
	m_wantedClass = member;

	NewRound();
}

Bot::~Bot(void)
{
	DeleteSearchNodes();

	edict_t* me = GetEntity();

	char botName[64];
	ITERATE_ARRAY(g_botNames, j)
	{
		sprintf(botName, "[E-BOT] %s", (char*)g_botNames[j].name);
		const char* name = GetEntityName(me);
		if (cstrcmp(g_botNames[j].name, name) == 0 || cstrcmp(botName, name) == 0)
		{
			g_botNames[j].isUsed = false;
			break;
		}
	}
}

// this function initializes a bot after creation & at the start of each round
void Bot::NewRound(void)
{
	const float time = engine->GetTime();
	if (ebot_random_join_quit.GetBool() && m_stayTime > 1.0f && m_stayTime < time)
	{
		Kick();
		return;
	}

	SetProcess(Process::Default, "i have respawned", true, time + 999999.0f);
	m_rememberedProcess = Process::Default;
	m_rememberedProcessTime = 0.0f;

	if (ebot_always_use_2d.GetBool())
		m_2dH = true;
	else
		m_2dH = static_cast<bool>(crandomint(0, 1));

	if (ebot_heuristic_type.GetInt() > 0 && ebot_heuristic_type.GetInt() < 5)
		m_heuristic = ebot_heuristic_type.GetInt();
	else
		m_heuristic = crandomint(1, 4);

	if (!g_waypoint->m_zmHmPoints.IsEmpty())
		m_zhCampPointIndex = g_waypoint->m_zmHmPoints.GetRandomElement();
	else
		m_zhCampPointIndex = -1;

	m_spawnTime = time;
	m_walkTime = 0.0f;

	ResetStuck();
	m_stuckArea = pev->origin;
	m_stuckTimer = time + engine->GetFreezeTime() + 1.28f;

	m_hasEnemiesNear = false;
	m_hasEntitiesNear = false;
	m_hasFriendsNear = false;

	m_nearestEnemy = nullptr;
	m_nearestEntity = nullptr;
	m_nearestFriend = nullptr;

	// delete all allocated path nodes
	DeleteSearchNodes();
	m_weaponSelectDelay = time;
	m_currentWaypointIndex = -1;
	m_currentTravelFlags = 0;
	m_prevGoalIndex = -1;
	m_chosenGoalIndex = -1;
	m_myMeshWaypoint = -1;
	m_loosedBombWptIndex = -1;

	m_prevWptIndex[0] = -1;
	m_prevWptIndex[1] = -1;
	m_prevWptIndex[2] = -1;
	m_prevWptIndex[3] = -1;

	m_isVIP = false;
	m_isLeader = false;
	m_hasProgressBar = false;

	m_pickupItem = nullptr;
	m_itemIgnore = nullptr;
	m_itemCheckTime = 0.0f;

	m_breakableEntity = nullptr;
	m_breakable = nullvec;
	m_timeDoorOpen = 0.0f;

	m_buttonPushTime = 0.0f;
	m_seeEnemyTime = 0.0f;

	m_voteMap = 0;

	for (auto& hostage : m_hostages)
		hostage = nullptr;

	m_isReloading = false;
	m_reloadState = ReloadState::Nothing;

	m_reloadCheckTime = 0.0f;
	m_firePause = 0.0f;

	m_jumpTime = 0.0f;
	m_duckTime = 0.0f;
	m_isStuck = false;
	m_jumpFinished = false;

	m_sayTextBuffer.timeNextChat = time;
	m_sayTextBuffer.entityIndex = -1;
	m_sayTextBuffer.sayText[0] = 0x0;

	m_nextBuyTime = time + crandomfloat(0.6f, 1.2f);
	m_inBombZone = false;

	m_shieldCheckTime = 0.0f;
	m_zoomCheckTime = 0.0f;
	m_strafeSetTime = 0.0f;
	m_combatStrafeDir = 0;
	m_fightStyle = 0;
	m_lastFightStyleCheck = 0.0f;

	m_radioEntity = nullptr;
	m_radioOrder = 0;

	m_lastChatTime = time;
	pev->button = 0;

	// clear its message queue
	for (auto& message : m_messageQueue)
		message = CMENU_IDLE;

	m_actMessageIndex = 0;
	m_pushMessageIndex = 0;

	// and put buying into its message queue
	if (g_gameVersion & Game::HalfLife)
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

	m_moveSpeed = pev->maxspeed;
	m_tempstrafeSpeed = crandomint(1, 2) == 1 ? pev->maxspeed : -pev->maxspeed;

	if (!IsAlive(GetEntity())) // if bot died, clear all weapon stuff and force buying again
	{
		cmemset(&m_ammoInClip, 0, sizeof(m_ammoInClip));
		cmemset(&m_ammo, 0, sizeof(m_ammo));
		m_currentWeapon = 0;
	}
}

// this function kills a bot (not just using ClientKill, but like the CSBot does)
// base code courtesy of Lazy (from bots-united forums!)
void Bot::Kill(void)
{
	edict_t* hurtEntity = (*g_engfuncs.pfnCreateNamedEntity) (MAKE_STRING("trigger_hurt"));
	if (FNullEnt(hurtEntity))
		return;

	edict_t* me = GetEntity();

	hurtEntity->v.classname = MAKE_STRING(g_weaponDefs[m_currentWeapon].className);
	hurtEntity->v.dmg_inflictor = me;
	hurtEntity->v.dmg = 999999.0f;
	hurtEntity->v.dmg_take = 1.0f;
	hurtEntity->v.dmgtime = 2.0f;
	hurtEntity->v.effects |= EF_NODRAW;

	(*g_engfuncs.pfnSetOrigin) (hurtEntity, Vector(-4000.0f, -4000.0f, -4000.0f));

	KeyValueData kv;
	kv.szClassName = const_cast<char*>(g_weaponDefs[m_currentWeapon].className);
	kv.szKeyName = "damagetype";
	kv.szValue = FormatBuffer("%d", (1 << 4));
	kv.fHandled = false;

	MDLL_KeyValue(hurtEntity, &kv);

	MDLL_Spawn(hurtEntity);
	MDLL_Touch(hurtEntity, me);

	(*g_engfuncs.pfnRemoveEntity) (hurtEntity);
}

void Bot::Kick(void)
{
	const char* myName = GetEntityName(GetEntity());
	if (IsNullString(myName))
		return;

	ServerCommand("kick \"%s\"", myName);
	CenterPrint("E-Bot '%s' kicked from the server", myName);

	const int botsNum = g_botManager->GetBotsNum() - 1;
	if (botsNum < ebot_quota.GetInt())
		ebot_quota.SetInt(botsNum);

	if (ebot_save_bot_names.GetBool() && !g_botManager->m_savedBotNames.IsEmpty())
		g_botManager->m_savedBotNames.PopNoReturn();
}

// this function handles the selection of teams & class
void Bot::StartGame(void)
{
	if (g_gameVersion == Game::HalfLife)
	{
		if (crandomint(1, 5) == 1)
			ChatMessage(CHAT_HELLO);

		m_notStarted = false;
		m_startAction = CMENU_IDLE;
		return;
	}

	// check if something has assigned team to us
	if ((m_team == Team::Terrorist || m_team == Team::Counter) && IsAlive(GetEntity()))
	{
		if (crandomint(1, 5) == 1)
			ChatMessage(CHAT_HELLO);

		m_notStarted = false;
		m_startAction = CMENU_IDLE;
		return;
	}
	else if (m_team == Team::Unassinged && m_retryJoin > 5)
		m_startAction = CMENU_TEAM;

	// if bot was unable to join team, and no menus popups, check for stacked team
	if (m_startAction == CMENU_IDLE)
	{
		if (++m_retryJoin > 10)
		{
			m_retryJoin = 0;
			Kick();
			return;
		}
	}

	// handle counter-strike stuff here...
	if (m_startAction == CMENU_TEAM)
	{
		m_startAction = CMENU_IDLE; // switch back to idle

		if (ebot_forceteam.GetString()[0] == 'C' || ebot_forceteam.GetString()[0] == 'c' || ebot_forceteam.GetString()[0] == '2')
			m_wantedTeam = 2;
		else if (ebot_forceteam.GetString()[0] == 'T' || ebot_forceteam.GetString()[0] == 't' || ebot_forceteam.GetString()[0] == '1') // 1 = T, 2 = CT
			m_wantedTeam = 1;

		if (m_wantedTeam != 1 && m_wantedTeam != 2)
			m_wantedTeam = 5;

		// select the team the bot wishes to join...
		FakeClientCommand(GetEntity(), "menuselect %d", m_wantedTeam);
	}
	else if (m_startAction == CMENU_CLASS)
	{
		m_startAction = CMENU_IDLE; // switch back to idle

		const int maxChoice = g_gameVersion == Game::CZero ? 5 : 4;
		m_wantedClass = crandomint(1, maxChoice);

		// select the class the bot wishes to use...
		FakeClientCommand(GetEntity(), "menuselect %d", maxChoice);

		// bot has now joined the game (doesn't need to be started)
		m_notStarted = false;

		// check for greeting other players, since we connected
		if (crandomint(1, 5) == 1)
			ChatMessage(CHAT_HELLO);
	}
}