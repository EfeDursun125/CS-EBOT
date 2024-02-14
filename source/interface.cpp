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

// console vars
ConVar ebot_password("ebot_password", "ebot", VARTYPE_PASSWORD);
ConVar ebot_password_key("ebot_password_key", "ebot_pass");

ConVar ebot_version("ebot_version", PRODUCT_VERSION, VARTYPE_READONLY);
ConVar ebot_showwp("ebot_show_waypoints", "0");

ConVar ebot_analyze_create_goal_waypoints("ebot_analyze_starter_waypoints", "1");
ConVar ebot_think_fps("ebot_think_fps", "30");

float secondTimer;
float updateTimer;

Bot* bot;

void ebotVersionMSG(edict_t* entity = nullptr)
{
	const int buildVersion[4] = { PRODUCT_VERSION_DWORD };
	const uint16_t bV16[4] = {static_cast<uint16_t>(buildVersion[0]), static_cast<uint16_t>(buildVersion[1]), static_cast<uint16_t>(buildVersion[2]), static_cast<uint16_t>(buildVersion[3])};

	char versionData[1024];
	sprintf(versionData,
		"------------------------------------------------\n"
		"%s %s%s\n"
		"Build: %u.%u.%u.%u\n"
		"------------------------------------------------\n"
		"Builded by: %s\n"
		"URL: %s \n"
		"Compiled: %s, %s (UTC +08:00)\n"
		"Meta-Interface Version: %s\n"
		"------------------------------------------------",
		PRODUCT_NAME, PRODUCT_VERSION, PRODUCT_DEV_VERSION_FORTEST,
		bV16[0], bV16[1], bV16[2], bV16[3],
		PRODUCT_AUTHOR, PRODUCT_URL, __DATE__, __TIME__, META_INTERFACE_VERSION);

	if (IsValidPlayer(entity))
		ClientPrint(entity, print_console, versionData);
	else
		ServerPrintNoTag(versionData);
}

int BotCommandHandler_O(edict_t* ent, const String& arg0, const String& arg1, const String& arg2, const String& arg3, const String& arg4, const String /*&arg5*/)
{
	if (cstricmp(arg0, "addbot") == 0 || cstricmp(arg0, "add") == 0 ||
		cstricmp(arg0, "addbot_hs") == 0 || cstricmp(arg0, "addhs") == 0 ||
		cstricmp(arg0, "addbot_t") == 0 || cstricmp(arg0, "add_t") == 0 ||
		cstricmp(arg0, "addbot_ct") == 0 || cstricmp(arg0, "add_ct") == 0)
		ServerPrint("Pls use the command to change it, use 'ebot help'");

	// kicking off one bot from the terrorist team
	else if (cstricmp(arg0, "kickbot_t") == 0 || cstricmp(arg0, "kick_t") == 0)
		g_botManager->RemoveFromTeam(Team::Terrorist);

	// kicking off one bot from the counter-terrorist team
	else if (cstricmp(arg0, "kickbot_ct") == 0 || cstricmp(arg0, "kick_ct") == 0)
		g_botManager->RemoveFromTeam(Team::Counter);

	// kills all bots on the terrorist team
	else if (cstricmp(arg0, "killbots_t") == 0 || cstricmp(arg0, "kill_t") == 0)
		g_botManager->KillAll(Team::Terrorist);

	// kills all bots on the counter-terrorist team
	else if (cstricmp(arg0, "killbots_ct") == 0 || cstricmp(arg0, "kill_ct") == 0)
		g_botManager->KillAll(Team::Counter);

	// list all bots playeing on the server
	else if (cstricmp(arg0, "listbots") == 0 || cstricmp(arg0, "list") == 0)
		g_botManager->ListBots();

	// kick off all bots from the played server
	else if (cstricmp(arg0, "kickbots") == 0 || cstricmp(arg0, "kickall") == 0)
		g_botManager->RemoveAll();

	// kill all bots on the played server
	else if (cstricmp(arg0, "killbots") == 0 || cstricmp(arg0, "killall") == 0)
		g_botManager->KillAll();

	// kick off one random bot from the played server
	else if (cstricmp(arg0, "kickone") == 0 || cstricmp(arg0, "kick") == 0)
		g_botManager->RemoveRandom();

	// fill played server with bots
	else if (cstricmp(arg0, "fillserver") == 0 || cstricmp(arg0, "fill") == 0)
		g_botManager->FillServer(catoi(arg1), IsNullString(arg2) ? -1 : catoi(arg2), IsNullString(arg3) ? -1 : catoi(arg3), IsNullString(arg4) ? -1 : catoi(arg4));

	// set entity action with command
	else if (cstricmp(arg0, "setentityaction") == 0)
		SetEntityAction(catoi(arg1), catoi(arg2), catoi(arg3));

	// swap counter-terrorist and terrorist teams
	else if (cstricmp(arg0, "swaptteams") == 0 || cstricmp(arg0, "swap") == 0)
	{
		for (const auto& client : g_clients)
		{
			if (FNullEnt(client.ent))
				continue;

			if (!(client.flags & CFLAG_USED))
				continue;

			if (IsValidBot(client.index))
				FakeClientCommand(client.ent, "chooseteam; menuselect %d; menuselect 5", GetTeam(client.ent) == Team::Counter ? 1 : 2);
			else
				(*g_engfuncs.pfnClientCommand) (client.ent, "chooseteam; menuselect %d", GetTeam(client.ent) == Team::Counter ? 1 : 2);
		}
	}

	// select the weapon mode for bots
	else if (cstricmp(arg0, "weaponmode") == 0 || cstricmp(arg0, "wmode") == 0)
	{
		const int selection = catoi(arg1);

		// check is selected range valid
		if (selection >= 1 && selection <= 7)
			g_botManager->SetWeaponMode(selection);
		else
			ClientPrint(ent, print_withtag, "Choose weapon from 1 to 7 range");
	}

	// force all bots to vote to specified map
	else if (cstricmp(arg0, "votemap") == 0)
	{
		if (!IsNullString(arg1))
		{
			const int nominatedMap = catoi(arg1);

			// loop through all players
			for (const auto& bot : g_botManager->m_bots)
			{
				if (bot)
					bot->m_voteMap = nominatedMap;
			}

			ClientPrint(ent, print_withtag, "All dead bots will vote for map #%d", nominatedMap);
		}
	}

	// force bots to execute client command
	else if (cstricmp(arg0, "sendcmd") == 0 || cstricmp(arg0, "order") == 0)
	{
		if (IsNullString(arg1))
			return 1;

		edict_t* cmd = INDEXENT(catoi(arg1) - 1);

		if (IsValidBot(cmd))
		{
			FakeClientCommand(cmd, arg2);
			ClientPrint(cmd, print_withtag, "E-Bot %s executing command %s", GetEntityName(ent), &arg2[0]);
		}
		else
			ClientPrint(cmd, print_withtag, "Player is NOT a E-Bot!");
	}

	// display current time on the server
	else if (cstricmp(arg0, "ctime") == 0 || cstricmp(arg0, "time") == 0)
	{
		const time_t tickTime = time(nullptr);
		const tm* localTime = localtime(&tickTime);

		char date[32];
		strftime(date, 31, "--- Current Time: %H:%M:%S ---", localTime);

		ClientPrint(ent, print_center, date);
	}

	// displays bot about information
	else if (cstricmp(arg0, "about_bot") == 0 || cstricmp(arg0, "about") == 0)
	{
		if (g_gameVersion & Game::HalfLife)
		{
			ServerPrint("Cannot do this on your game version");
			return 1;
		}

		char aboutData[] =
			"+---------------------------------------------------------------------------------+\n"
			" The E-BOT for Counter-Strike 1.6 " PRODUCT_SUPPORT_VERSION "\n"
			" Made by " PRODUCT_AUTHOR ", Based on SyPB & YaPB\n"
			" Website: " PRODUCT_URL ", ASYNC Build: No\n"
			"+---------------------------------------------------------------------------------+\n";

		HudMessage(ent, true, Color(crandomint(33, 255), crandomint(33, 255), crandomint(33, 255)), aboutData);
	}

	// displays version information
	else if (cstricmp(arg0, "version") == 0 || cstricmp(arg0, "ver") == 0)
		ebotVersionMSG(ent);

	// display some sort of help information
	else if (cstricmp(arg0, "?") == 0 || cstricmp(arg0, "help") == 0)
	{
		ClientPrint(ent, print_console, "E-Bot Commands:");
		ClientPrint(ent, print_console, "ebot version            - display version information");
		ClientPrint(ent, print_console, "ebot about              - show bot about information");
		//ClientPrint (ent, print_console, "ebot add                - create a e-bot in current game.");
		ClientPrint(ent, print_console, "ebot fill               - fill the server with random e-bots");
		ClientPrint(ent, print_console, "ebot kickall            - disconnects all e-bots from current game");
		ClientPrint(ent, print_console, "ebot killbots           - kills all e-bots in current game");
		ClientPrint(ent, print_console, "ebot kick               - disconnect one random e-bot from game");
		ClientPrint(ent, print_console, "ebot weaponmode         - select e-bot weapon mode");
		ClientPrint(ent, print_console, "ebot votemap            - allows dead e-bots to vote for specific map");
		ClientPrint(ent, print_console, "ebot cmenu              - displaying e-bots command menu");
		ClientPrint(ent, print_console, "ebot_add                - adds a e-bot in current game");

		if (cstricmp(arg1, "full") == 0 || cstricmp(arg1, "f") == 0 || cstricmp(arg1, "?") == 0)
		{
			ClientPrint(ent, print_console, "ebot_add_t              - creates one random e-bot to terrorist team");
			ClientPrint(ent, print_console, "ebot_add_ct             - creates one random e-bot to ct team");
			ClientPrint(ent, print_console, "ebot kick_t             - disconnect one random e-bot from terrorist team");
			ClientPrint(ent, print_console, "ebot kick_ct            - disconnect one random e-bot from ct team");
			ClientPrint(ent, print_console, "ebot kill_t             - kills all e-bots on terrorist team");
			ClientPrint(ent, print_console, "ebot kill_ct            - kills all e-bots on ct team");
			ClientPrint(ent, print_console, "ebot list               - display list of e-bots currently playing");
			ClientPrint(ent, print_console, "ebot order              - execute specific command on specified e-bot");
			ClientPrint(ent, print_console, "ebot time               - displays current time on server");
			ClientPrint(ent, print_console, "ebot deletewp           - delete waypoint file from hard disk (permanently)");

			if (!IsDedicatedServer())
			{
				ServerPrintNoTag("ebot autowp            - toggle autowppointing");
				ServerPrintNoTag("ebot wp                - toggle waypoint showing");
				ServerPrintNoTag("ebot wp on noclip      - enable noclip cheat");
				ServerPrintNoTag("ebot wp save nocheck   - save waypoints without checking");
				ServerPrintNoTag("ebot wp add            - open menu for waypoint creation");
				ServerPrintNoTag("ebot wp menu           - open main waypoint menu");
				ServerPrintNoTag("ebot wp addbasic       - creates basic waypoints on map");
				ServerPrintNoTag("ebot wp find           - show direction to specified waypoint");
				ServerPrintNoTag("ebot wp load           - wload the waypoint file from hard disk");
				ServerPrintNoTag("ebot wp check          - checks if all waypoints connections are valid");
				ServerPrintNoTag("ebot wp cache          - cache nearest waypoint");
				ServerPrintNoTag("ebot wp teleport       - teleport hostile to specified waypoint");
				ServerPrintNoTag("ebot wp setradius      - manually sets the wayzone radius for this waypoint");
				ServerPrintNoTag("ebot path autodistance - opens menu for setting autopath maximum distance");
				ServerPrintNoTag("ebot path cache        - remember the nearest to player waypoint");
				ServerPrintNoTag("ebot path create       - opens menu for path creation");
				ServerPrintNoTag("ebot path delete       - delete path from cached to nearest waypoint");
				ServerPrintNoTag("ebot path create_in    - creating incoming path connection");
				ServerPrintNoTag("ebot path create_out   - creating outgoing path connection");
				ServerPrintNoTag("ebot path create_both  - creating both-ways path connection");
			}
		}
	}

	// sets health for all available bots
	else if (cstricmp(arg0, "sethealth") == 0 || cstricmp(arg0, "health") == 0)
	{
		if (IsNullString(arg1))
			ClientPrint(ent, print_withtag, "Please specify health");
		else
		{
			ClientPrint(ent, print_withtag, "E-Bot health is set to %d%%", catoi(arg1));

			for (const auto& bot : g_botManager->m_bots)
			{
				if (bot)
					bot->pev->health = cabsf(catof(arg1));
			}
		}
	}

	else if (cstricmp(arg0, "setgravity") == 0 || cstricmp(arg0, "gravity") == 0)
	{
		if (IsNullString(arg1))
			ClientPrint(ent, print_withtag, "Please specify gravity");
		else
		{
			const float gravity = cabsf(catof(arg1));
			ClientPrint(ent, print_withtag, "E-Bot gravity is set to %d%%", gravity);

			for (const auto& bot : g_botManager->m_bots)
			{
				if (bot)
					bot->pev->gravity = gravity;
			}
		}
	}

	// displays main bot menu
	else if (cstricmp(arg0, "botmenu") == 0 || cstricmp(arg0, "menu") == 0)
		DisplayMenuToClient(ent, &g_menus[0]);

	// display command menu
	else if (cstricmp(arg0, "cmdmenu") == 0 || cstricmp(arg0, "cmenu") == 0)
	{
		if (IsAlive(ent))
			DisplayMenuToClient(ent, &g_menus[18]);
		else
		{
			DisplayMenuToClient(ent, nullptr); // reset menu display
			CenterPrint("You're dead, and have no access to this menu");
		}
	}
	// some debug routines
	else if (cstricmp(arg0, "debug") == 0)
	{
		// test random number generator
		if (cstricmp(arg0, "randgen") == 0)
		{
			int i;
			for (i = 0; i < 500; i++)
				ServerPrintNoTag("Result Range[0 - 100]: %d", crandomint(0, 100));
		}
	}

	else if (cstricmp(arg0, "nav") == 0 || cstricmp(arg0, "navmesh") == 0 || cstricmp(arg0, "navigation") == 0)
	{
		if (IsDedicatedServer() || FNullEnt(g_hostEntity))
			return 2;

		else if (cstricmp(arg1, "addbasic") == 0)
		{
			g_navmesh->CreateBasic();
			CenterPrint("Basic navmeshes was Created");
		}

		if (cstricmp(arg1, "analyze") == 0)
		{
			g_navmesh->CreateBasic();
			ServerPrint("NavMesh Analyzing On");
			ServerCommand("ebot nav on");
			g_analyzenavmesh = true;
		}
		else if (cstricmp(arg1, "analyzeoff") == 0)
		{
			ServerPrint("NavMesh Analyzing Off");
			ServerCommand("ebot nav off");
			g_analyzenavmesh = false;
		}
		else if (cstricmp(arg1, "create") == 0)
		{
			const Vector aimPos = g_navmesh->GetAimingPosition();
			if (aimPos != nullvec)
			{
				g_navmeshOn = true;
				g_navmesh->CreateArea(aimPos, true);
				ServerCommand("ebot wp mdl on");
			}
		}
		else if (cstricmp(arg1, "delete") == 0)
		{
			g_navmeshOn = true;

			const Vector aimPos = g_navmesh->GetAimingPosition();
			g_navmesh->DeleteAreaByIndex(g_navmesh->GetNearestNavAreaID(aimPos));

			ServerCommand("ebot wp mdl on");
		}
		else if (cstricmp(arg1, "disconnect") == 0)
		{
			const int16_t seleted = g_navmesh->m_selectedNavIndex;
			if (seleted < 0 || seleted >= g_numNavAreas)
			{
				CenterPrint("Select target nav area first!");
			}
			else
			{
				const Vector aimPos = g_navmesh->GetAimingPosition();
				if (aimPos != nullvec)
				{
					const int16_t index = g_navmesh->GetNearestNavAreaID(aimPos);
					if (index != -1 && index != seleted)
					{
						g_navmeshOn = true;
						g_navmesh->DisconnectArea(seleted, index);
						ServerCommand("ebot wp mdl on");
						g_navmesh->UnselectNavArea();
					}
				}
			}
		}
		else if (cstricmp(arg1, "connect") == 0)
		{
			const int16_t seleted = g_navmesh->m_selectedNavIndex;
			if (seleted < 0 || seleted >= g_numNavAreas)
			{
				CenterPrint("Select target nav area first!");
			}
			else
			{
				const Vector aimPos = g_navmesh->GetAimingPosition();
				if (aimPos != nullvec)
				{
					const int16_t index = g_navmesh->GetNearestNavAreaID(aimPos);
					if (index != -1 && index != seleted)
					{
						g_navmeshOn = true;
						g_navmesh->ConnectArea(seleted, index);
						ServerCommand("ebot wp mdl on");
						g_navmesh->UnselectNavArea();
					}
				}
			}
		}
		else if (cstricmp(arg1, "merge") == 0)
		{
			const int16_t selected = g_navmesh->m_selectedNavIndex;
			if (selected < 0 || selected >= g_numNavAreas)
			{
				CenterPrint("Select target nav area first!");
			}
			else
			{
				const Vector aimPos = g_navmesh->GetAimingPosition();
				if (aimPos != nullvec)
				{
					const int16_t index = g_navmesh->GetNearestNavAreaID(aimPos);
					if (index != -1 && index != selected)
					{
						g_navmeshOn = true;
						g_navmesh->GetNavAreaP(selected)->MergeWith(g_navmesh->GetNavArea(index));
						ServerCommand("ebot wp mdl on");
						g_navmesh->UnselectNavArea();
					}
				}
			}
		}
		else if (cstricmp(arg1, "unselect") == 0)
			g_navmesh->UnselectNavArea(true);
		else if (cstricmp(arg1, "select") == 0)
			g_navmesh->SelectNavArea();
		else if (cstricmp(arg1, "on") == 0)
		{
			g_navmeshOn = true;
			ServerPrint("NavMesh Editing Enabled");
			ServerCommand("ebot wp mdl on");
		}
		else if (cstricmp(arg1, "noclip") == 0)
		{
			if (g_editNoclip)
			{
				g_hostEntity->v.movetype = MOVETYPE_WALK;
				ServerPrint("Noclip Cheat Disabled");
			}
			else
			{
				g_hostEntity->v.movetype = MOVETYPE_NOCLIP;
				ServerPrint("Noclip Cheat Enabled");
			}

			g_editNoclip ^= true; // switch on/off (XOR it!)
		}
		else if (cstricmp(arg1, "off") == 0)
		{
			g_navmeshOn = false;
			g_editNoclip = false;
			g_hostEntity->v.movetype = MOVETYPE_WALK;

			ServerPrint("NavMesh Editing Disabled");
			ServerCommand("ebot wp mdl off");
		}
		else if (cstricmp(arg1, "save") == 0)
			g_navmesh->SaveNav();
		else if (cstricmp(arg1, "load") == 0)
			g_navmesh->LoadNav();
	}

	// waypoint manimupulation (really obsolete, can be edited through menu) (supported only on listen server)
	else if (cstricmp(arg0, "waypoint") == 0 || cstricmp(arg0, "wp") == 0 || cstricmp(arg0, "wpt") == 0)
	{
		if (IsDedicatedServer() || FNullEnt(g_hostEntity))
			return 2;

		// enables or disable waypoint displaying
		if (cstricmp(arg1, "analyze") == 0)
		{
			ServerPrint("Waypoint Analyzing On (Please Manually Edit Waypoints For Better Result)");
			ServerCommand("ebot wp on");
			if (ebot_analyze_create_goal_waypoints.GetInt() == 1)
				g_waypoint->CreateBasic();

			g_analyzewaypoints = true;
		}

		else if (cstricmp(arg1, "analyzeoff") == 0)
		{
			g_waypoint->AnalyzeDeleteUselessWaypoints();
			g_analyzewaypoints = false;
			ServerPrint("Waypoint Analyzing Off");
			g_waypoint->Save();
			ServerCommand("ebot wp off");
			g_analyzeputrequirescrouch = false;
		}

		else if (cstricmp(arg1, "analyzefix") == 0)
		{
			g_waypoint->AnalyzeDeleteUselessWaypoints();
			ServerCommand("ebot wp on");
		}

		else if (cstricmp(arg1, "on") == 0)
		{
			g_waypointOn = true;
			ServerPrint("Waypoint Editing Enabled");

			ServerCommand("ebot wp mdl on");
		}

		// enables noclip cheat
		else if (cstricmp(arg1, "noclip") == 0)
		{
			if (g_editNoclip)
			{
				g_hostEntity->v.movetype = MOVETYPE_WALK;
				ServerPrint("Noclip Cheat Disabled");
			}
			else
			{
				g_hostEntity->v.movetype = MOVETYPE_NOCLIP;
				ServerPrint("Noclip Cheat Enabled");
			}

			g_editNoclip ^= true; // switch on/off (XOR it!)
		}

		// switching waypoint editing off
		else if (cstricmp(arg1, "off") == 0)
		{
			g_waypointOn = false;
			g_editNoclip = false;
			g_hostEntity->v.movetype = MOVETYPE_WALK;

			ServerPrint("Waypoint Editing Disabled");
			ServerCommand("ebot wp mdl off");
		}

		// toggles displaying player models on spawn spots
		else if (cstricmp(arg1, "mdl") == 0 || cstricmp(arg1, "models") == 0)
		{
			edict_t* spawnEntity = nullptr;

			if (cstricmp(arg2, "on") == 0)
			{
				while (!FNullEnt(spawnEntity = FIND_ENTITY_BY_CLASSNAME(spawnEntity, "info_player_start")))
					spawnEntity->v.effects &= ~EF_NODRAW;
				while (!FNullEnt(spawnEntity = FIND_ENTITY_BY_CLASSNAME(spawnEntity, "info_player_deathmatch")))
					spawnEntity->v.effects &= ~EF_NODRAW;
				while (!FNullEnt(spawnEntity = FIND_ENTITY_BY_CLASSNAME(spawnEntity, "info_vip_start")))
					spawnEntity->v.effects &= ~EF_NODRAW;

				ServerCommand("mp_roundtime 9"); // reset round time to maximum
				ServerCommand("mp_timelimit 0"); // disable the time limit
				ServerCommand("mp_freezetime 0"); // disable freezetime
			}
			else if (cstricmp(arg2, "off") == 0)
			{
				while (!FNullEnt(spawnEntity = FIND_ENTITY_BY_CLASSNAME(spawnEntity, "info_player_start")))
					spawnEntity->v.effects |= EF_NODRAW;
				while (!FNullEnt(spawnEntity = FIND_ENTITY_BY_CLASSNAME(spawnEntity, "info_player_deathmatch")))
					spawnEntity->v.effects |= EF_NODRAW;
				while (!FNullEnt(spawnEntity = FIND_ENTITY_BY_CLASSNAME(spawnEntity, "info_vip_start")))
					spawnEntity->v.effects |= EF_NODRAW;
			}
		}

		// show direction to specified waypoint
		else if (cstricmp(arg1, "find") == 0)
			g_waypoint->SetFindIndex(catoi(arg2));

		// opens adding waypoint menu
		else if (cstricmp(arg1, "add") == 0)
		{
			g_waypointOn = true;  // turn waypoints on
			DisplayMenuToClient(g_hostEntity, &g_menus[12]);
		}

		// sets mesh for waypoint
		else if (cstricmp(arg1, "setmesh") == 0)
		{
			if (IsNullString(arg2))
				ClientPrint(ent, print_withtag, "Please set mesh <number>, min 0, max 255");
			else
			{
				const int index = g_waypoint->FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);
				if (IsValidWaypoint(index))
				{
					g_waypoint->GetPath(index)->mesh = static_cast<uint8_t>(cabsf(catof(arg2)));
					ClientPrint(ent, print_withtag, "Waypoint mesh set to %d", g_waypoint->GetPath(index)->mesh);
				}
				else
					ClientPrint(ent, print_withtag, "Waypoint is not valid");
			}
		}

		// sets gravity for waypoint
		else if (cstricmp(arg1, "setgravity") == 0)
		{
			if (IsNullString(arg2))
				ClientPrint(ent, print_withtag, "Please set gravity <number>");
			else
			{
				const int index = g_waypoint->FindNearest(GetEntityOrigin(g_hostEntity), 75.0f);
				if (IsValidWaypoint(index))
				{
					g_waypoint->GetPath(index)->gravity = cabsf(catof(arg2));
					ClientPrint(ent, print_withtag, "Waypoint gravity set to %f", g_waypoint->GetPath(index)->gravity);
				}
				else
					ClientPrint(ent, print_withtag, "Waypoint is not valid");
			}
		}

		// creates basic waypoints on the map (ladder/spawn points/goals)
		else if (cstricmp(arg1, "addbasic") == 0)
		{
			g_waypoint->CreateBasic();
			CenterPrint("Basic waypoints was Created");
		}

		// delete nearest to host edict waypoint
		else if (cstricmp(arg1, "delete") == 0)
		{
			g_waypointOn = true; // turn waypoints on
			g_waypoint->Delete();
		}

		// save waypoint data into file on hard disk
		else if (cstricmp(arg1, "save") == 0)
		{
			if (cstrncmp(arg2, "nocheck", 8) == 0)
				g_waypoint->Save();
			else if (g_waypoint->NodesValid())
				g_waypoint->Save();
		}

		// load all waypoints again (overrides all changes, that wasn't saved)
		else if (cstricmp(arg1, "load") == 0)
		{
			if (g_waypoint->Load())
				ServerPrint("Waypoints loaded");
		}

		// check all nodes for validation
		else if (cstricmp(arg1, "check") == 0)
		{
			if (g_waypoint->NodesValid())
				CenterPrint("Nodes work Fine");
		}

		// opens menu for setting (removing) waypoint flags
		else if (cstricmp(arg1, "flags") == 0)
			DisplayMenuToClient(g_hostEntity, &g_menus[13]);

		// setting waypoint radius
		else if (cstricmp(arg1, "setradius") == 0)
			g_waypoint->SetRadius(catoi(arg2));

		// remembers nearest waypoint
		else if (cstricmp(arg1, "cache") == 0)
			g_waypoint->CacheWaypoint();

		// teleport player to specified waypoint
		else if (cstricmp(arg1, "teleport") == 0)
		{
			const int teleportPoint = catoi(arg2);
			if (teleportPoint < g_numWaypoints)
			{
				(*g_engfuncs.pfnSetOrigin) (g_hostEntity, g_waypoint->GetPath(teleportPoint)->origin);
				g_waypointOn = true;

				ServerPrint("Player '%s' teleported to waypoint #%d (x:%.1f, y:%.1f, z:%.1f)", GetEntityName(g_hostEntity), teleportPoint, g_waypoint->GetPath(teleportPoint)->origin.x, g_waypoint->GetPath(teleportPoint)->origin.y, g_waypoint->GetPath(teleportPoint)->origin.z);
				g_editNoclip = true;
			}
		}

		// displays waypoint menu
		else if (cstricmp(arg1, "menu") == 0)
			DisplayMenuToClient(g_hostEntity, &g_menus[9]);

		// otherwise display waypoint current status
		else
			ServerPrint("Waypoints are %s", g_waypointOn ? "Enabled" : "Disabled");
	}

	// path waypoint editing system (supported only on listen server)
	else if (cstricmp(arg0, "pathwaypoint") == 0 || cstricmp(arg0, "path") == 0 || cstricmp(arg0, "pwp") == 0)
	{
		if (IsDedicatedServer() || FNullEnt(g_hostEntity))
			return 2;

		// opens path creation menu
		if (cstricmp(arg1, "create") == 0)
			DisplayMenuToClient(g_hostEntity, &g_menus[20]);

		// creates incoming path from the cached waypoint
		else if (cstricmp(arg1, "create_in") == 0)
			g_waypoint->CreatePath(PATHCON_INCOMING);

		// creates outgoing path from current waypoint
		else if (cstricmp(arg1, "create_out") == 0)
			g_waypoint->CreatePath(PATHCON_OUTGOING);

		// creates bidirectional path from cahed to current waypoint
		else if (cstricmp(arg1, "create_both") == 0)
			g_waypoint->CreatePath(PATHCON_BOTHWAYS);

		// creates jump path from cahed to current waypoint
		else if (cstricmp(arg1, "create_jump") == 0)
			g_waypoint->CreatePath(PATHCON_JUMPING);

		// creates boosting path from cahed to current waypoint
		else if (cstricmp(arg1, "create_boost") == 0)
			g_waypoint->CreatePath(PATHCON_BOOSTING);

		// creates visible only path from cahed to current waypoint
		else if (cstricmp(arg1, "create_visible") == 0)
			g_waypoint->CreatePath(PATHCON_VISIBLE);

		// delete special path
		else if (cstricmp(arg1, "delete") == 0)
			g_waypoint->DeletePath();

		// sets auto path maximum distance
		else if (cstricmp(arg1, "autodistance") == 0)
			DisplayMenuToClient(g_hostEntity, &g_menus[19]);
	}

	// automatic waypoint handling (supported only on listen server)
	else if (cstricmp(arg0, "autowaypoint") == 0 || cstricmp(arg0, "autowp") == 0)
	{
		if (IsDedicatedServer() || FNullEnt(g_hostEntity))
			return 2;

		// enable autowaypointing
		if (cstricmp(arg1, "on") == 0)
		{
			g_autoWaypoint = true;
			g_waypointOn = true; // turn this on just in case
		}

		// disable autowaypointing
		else if (cstricmp(arg1, "off") == 0)
			g_autoWaypoint = false;

		// display status
		ServerPrint("Auto-Waypoint %s", g_autoWaypoint ? "Enabled" : "Disabled");
	}
	return 0; // command is not handled by bot

	return 1; // command was handled by bot
}

int BotCommandHandler(edict_t* ent, const char* arg0, const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5)
{
	return BotCommandHandler_O(ent, arg0, arg1, arg2, arg3, arg4, arg5);
}

void CommandHandler(void)
{
	// this function is the dedicated server command handler for the new ebot server command we
	// registered at game start. It will be called by the engine each time a server command that
	// starts with "ebot" is entered in the server console. It works exactly the same way as
	// ClientCommand() does, using the CmdArgc() and CmdArgv() facilities of the engine. Argv(0)
	// is the server command itself (here "ebot") and the next ones are its arguments. Just like
	// the stdio command-line parsing in C when you write "long main (long argc, char **argv)".

	// check status for dedicated server command
	BotCommandHandler(g_hostEntity, IsNullString(CMD_ARGV(1)) ? "help" : CMD_ARGV(1), CMD_ARGV(2), CMD_ARGV(3), CMD_ARGV(4), CMD_ARGV(5), CMD_ARGV(6));
}

void ebot_Version_Command(void)
{
	ebotVersionMSG();
}

void CheckEntityAction(void)
{
	int i;
	int workEntityWork = 0;
	edict_t* entity = nullptr;
	char action[33], team[33];

	for (i = 0; i < entityNum; i++)
	{
		if (g_entityId[i] == -1)
			continue;

		entity = INDEXENT(g_entityId[i]);
		if (FNullEnt(entity) || entity->v.effects & EF_NODRAW)
			continue;

		if (g_entityAction[i] == 1)
			sprintf(action, "Enemy");
		else if (g_entityAction[i] == 2)
			sprintf(action, "Need Avoid");
		else if (g_entityAction[i] == 3)
			sprintf(action, "Pick Up");

		sprintf(team, (g_entityTeam[i] == Team::Counter) ? "CT" : (g_entityTeam[i] == Team::Terrorist) ? "TR" : "Team-%d", g_entityTeam[i]);

		workEntityWork++;
		ServerPrintNoTag("Entity Num: %d | Action: %d (%s) | Team: %d (%s) | Entity Name: %s", workEntityWork, g_entityAction[i], action, g_entityTeam[i], team, GetEntityName(entity));
	}

	ServerPrintNoTag("Total Entity Action Num: %d", workEntityWork);
}

void LoadEntityData(void)
{
	int i;
	edict_t* entity;

	for (i = 0; i < entityNum; i++)
	{
		if (g_entityId[i] == -1)
			continue;

		entity = INDEXENT(g_entityId[i]);
		if (!IsAlive(entity))
			SetEntityActionData(i);
	}

	for (i = 0; i < engine->GetMaxClients(); i++)
	{
		entity = INDEXENT(i + 1);
		if (FNullEnt(entity))
		{
			g_clients[i].flags = 0;
			g_clients[i].ent = nullptr;
			g_clients[i].origin = nullvec;
			g_clients[i].index = -1;
			g_clients[i].team = Team::Count;
			continue;
		}

		g_clients[i].ent = entity;
		g_clients[i].index = i;
		g_clients[i].flags |= CFLAG_USED;

		if (IsAlive(entity))
		{
			g_clients[i].flags |= CFLAG_ALIVE;
			g_clients[i].team = GetTeam(entity);
			g_clients[i].origin = GetEntityOrigin(entity);
			continue;
		}

		g_clients[i].flags &= ~CFLAG_ALIVE;
	}
}

void AddBot(void)
{
	g_botManager->AddRandom();
}

void AddBot_TR(void)
{
	g_botManager->AddBotAPI("", -1, 1);
}

void AddBot_CT(void)
{
	g_botManager->AddBotAPI("", -1, 2);
}

void InitConfig(void)
{
	File fp;
	char command[80], line[256];

	KwChat replyKey;
	int chatType = -1;

#define SKIP_COMMENTS() if ((line[0] == '/') || (line[0] == '\r') || (line[0] == '\n') || (line[0] == 0) || (line[0] == ' ') || (line[0] == '\t')) continue;

	if (!g_botNames.IsEmpty())
	{
		int16_t i;
		for (i = 0; i < g_botNames.Size(); i++)
			g_botNames[i].isUsed = false;
	}

	// NAME SYSTEM INITIALIZATION
	if (g_botNames.IsEmpty() && OpenConfig("names.cfg", "Name configuration file not found.", &fp))
	{
		while (fp.GetBuffer(line, 255))
		{
			SKIP_COMMENTS();

			cstrtrim(line);
			NameItem item;

			char Name[33];
			sprintf(Name, "%s", line);

			item.name = Name;
			item.isUsed = false;
			g_botNames.Push(item);
		}

		fp.Close();
	}

	// CHAT SYSTEM CONFIG INITIALIZATION
	if (g_chatFactory.IsEmpty())
	{
		if (OpenConfig("chat.cfg", "Chat file not found.", &fp))
		{
			int chatType = -1;
			g_chatFactory.SetSize(CHAT_NUM);

			while (fp.GetBuffer(line, 255))
			{
				SKIP_COMMENTS();
				cstrcpy(command, GetField(line, 0, 1));

				if (cstrcmp(command, "[KILLED]") == 0)
				{
					chatType = 0;
					continue;
				}
				else if (cstrcmp(command, "[BOMBPLANT]") == 0)
				{
					chatType = 1;
					continue;
				}
				else if (cstrcmp(command, "[DEADCHAT]") == 0)
				{
					chatType = 2;
					continue;
				}
				else if (cstrcmp(command, "[REPLIES]") == 0)
				{
					chatType = 3;
					continue;
				}
				else if (cstrcmp(command, "[UNKNOWN]") == 0)
				{
					chatType = 4;
					continue;
				}
				else if (cstrcmp(command, "[TEAMATTACK]") == 0)
				{
					chatType = 5;
					continue;
				}
				else if (cstrcmp(command, "[WELCOME]") == 0)
				{
					chatType = 6;
					continue;
				}
				else if (cstrcmp(command, "[TEAMKILL]") == 0)
				{
					chatType = 7;
					continue;
				}

				if (chatType != 3)
					line[79] = 0;

				cstrtrim(line);

				switch (chatType)
				{
				case 0:
				{
					g_chatFactory[CHAT_KILL].Push(line);
					break;
				}
				case 1:
				{
					g_chatFactory[CHAT_PLANTBOMB].Push(line);
					break;
				}
				case 2:
				{
					g_chatFactory[CHAT_DEAD].Push(line);
					break;
				}
				case 3:
				{
					if (cstrstr(line, "@KEY"))
					{
						if (!replyKey.keywords.IsEmpty() && !replyKey.replies.IsEmpty())
						{
							g_replyFactory.Push(replyKey);
							replyKey.replies.Destroy();
						}

						replyKey.keywords.Destroy();
						replyKey.keywords = String(&line[4]).Split(',');

						ITERATE_ARRAY(replyKey.keywords, i)
							replyKey.keywords[i].Trim().TrimQuotes();
					}
					else if (!replyKey.keywords.IsEmpty())
						replyKey.replies.Push(line);
					break;
				}
				case 4:
				{
					g_chatFactory[CHAT_NOKW].Push(line);
					break;
				}
				case 5:
				{
					g_chatFactory[CHAT_TEAMATTACK].Push(line);
					break;
				}
				case 6:
				{
					g_chatFactory[CHAT_HELLO].Push(line);
					break;
				}
				case 7:
				{
					g_chatFactory[CHAT_TEAMKILL].Push(line);
					break;
				}
				}
			}

			fp.Close();
		}
		else
		{
			extern ConVar ebot_chat;
			ebot_chat.SetInt(0);
		}
	}

	// AVATARS INITITALIZATION
	if (OpenConfig("avatars.cfg", "Avatars config file not found. Avatars will not be displayed.", &fp))
	{
		while (fp.GetBuffer(line, 255))
		{
			SKIP_COMMENTS();
			cstrtrim(line);
			g_botManager->m_avatars.Push(line);
		}

		fp.Close();
	}
}

void GameDLLInit(void)
{
	// this function is a one-time call, and appears to be the second function called in the
	// DLL after FuncPointers_t() has been called. Its purpose is to tell the MOD DLL to
	// initialize the game before the engine actually hooks into it with its video frames and
	// clients connecting. Note that it is a different step than the *server* initialization.
	// This one is called once, and only once, when the game process boots up before the first
	// server is enabled. Here is a good place to do our own game session initialization, and
	// to register by the engine side the server commands we need to administrate our bots.

	engine->PushRegisteredConVarsToEngine();

	RegisterCommand("ebot_about", ebot_Version_Command);

	RegisterCommand("ebot_add_t", AddBot_TR);
	RegisterCommand("ebot_add_tr", AddBot_TR);
	RegisterCommand("ebot_add_ct", AddBot_CT);
	RegisterCommand("ebot_add", AddBot);

	RegisterCommand("ebot_entity_check", CheckEntityAction);

	// register server command(s)
	RegisterCommand("ebot", CommandHandler);

	// execute main config
	ServerCommand("exec addons/ebot/ebot.cfg");

	RETURN_META(MRES_IGNORED);
}

int Spawn(edict_t* ent)
{
	// this function asks the game DLL to spawn (i.e, give a physical existence in the virtual
	// world, in other words to 'display') the entity pointed to by ent in the game. The
	// Spawn() function is one of the functions any entity is supposed to have in the game DLL,
	// and any MOD is supposed to implement one for each of its entities.
	const char* entityClassname = STRING(ent->v.classname);
	if (cstrcmp(entityClassname, "worldspawn") == 0)
	{
		PRECACHE_SOUND("weapons/xbow_hit1.wav");      // waypoint add
		PRECACHE_SOUND("weapons/mine_activate.wav");  // waypoint delete
		PRECACHE_SOUND("common/wpn_hudoff.wav");      // path add/delete start
		PRECACHE_SOUND("common/wpn_hudon.wav");       // path add/delete done
		PRECACHE_SOUND("common/wpn_moveselect.wav");  // path add/delete cancel
		PRECACHE_SOUND("common/wpn_denyselect.wav");  // path add/delete error

		g_modelIndexLaser = PRECACHE_MODEL("sprites/laserbeam.spr");
		g_modelIndexArrow = PRECACHE_MODEL("sprites/arrow1.spr");
		g_roundEnded = true;

		RoundInit();

		g_hasDoors = false; // reset doors if they are removed by a custom plugin
		g_worldEdict = ent; // save the world entity for future use

		if (g_gameVersion & Game::HalfLife)
		{
			if (!IsDedicatedServer())
				PRECACHE_MODEL("models/player/gordon/gordon.mdl");
		}
	}
	else if (cstrcmp(entityClassname, "func_door") == 0 || cstrcmp(entityClassname, "func_door_rotating") == 0)
		g_hasDoors = true;
	else if (cstrcmp(entityClassname, "player_weaponstrip") == 0)
	{
		if (g_gameVersion & Game::HalfLife && (STRING(ent->v.target))[0] == '\0')
		{
			ent->v.target = MAKE_STRING("fake");
			ent->v.targetname = MAKE_STRING("fake");
		}
		else
		{
			REMOVE_ENTITY(ent);
			RETURN_META_VALUE(MRES_SUPERCEDE, 0);
		}
	}
	else
	{
		if (g_gameVersion & Game::HalfLife)
		{
			if (!IsDedicatedServer())
			{
				if (cstrcmp(entityClassname, "info_player_start") == 0)
				{
					SET_MODEL(ent, "models/player/gordon/gordon.mdl");
					ent->v.rendermode = kRenderTransAlpha; // set its render mode to transparency
					ent->v.renderamt = 127; // set its transparency amount
					ent->v.effects |= EF_NODRAW;
				}
				else if (cstrcmp(entityClassname, "info_player_deathmatch") == 0)
				{
					SET_MODEL(ent, "models/player/gordon/gordon.mdl");
					ent->v.rendermode = kRenderTransAlpha; // set its render mode to transparency
					ent->v.renderamt = 127; // set its transparency amount
					ent->v.effects |= EF_NODRAW;
				}
			}
		}
		else
		{
			if (!IsDedicatedServer() && cstrcmp(entityClassname, "info_player_start") == 0)
			{
				SET_MODEL(ent, "models/player/gsg9/gsg9.mdl");
				ent->v.rendermode = kRenderTransAlpha; // set its render mode to transparency
				ent->v.renderamt = 127; // set its transparency amount
				ent->v.effects |= EF_NODRAW;
			}
			else if (!IsDedicatedServer() && cstrcmp(entityClassname, "info_player_deathmatch") == 0)
			{
				SET_MODEL(ent, "models/player/terror/terror.mdl");
				ent->v.rendermode = kRenderTransAlpha; // set its render mode to transparency
				ent->v.renderamt = 127; // set its transparency amount
				ent->v.effects |= EF_NODRAW;
			}
			else if (!IsDedicatedServer() && cstrcmp(entityClassname, "info_vip_start") == 0)
			{
				SET_MODEL(ent, "models/player/vip/vip.mdl");
				ent->v.rendermode = kRenderTransAlpha; // set its render mode to transparency
				ent->v.renderamt = 127; // set its transparency amount
				ent->v.effects |= EF_NODRAW;
			}
			else if (cstrcmp(entityClassname, "func_vip_safetyzone") == 0 || cstrcmp(entityClassname, "info_vip_safetyzone") == 0)
				g_mapType = MAP_AS; // assassination map
			else if (cstrcmp(entityClassname, "hostage_entity") == 0 || cstrcmp(entityClassname, "monster_scientist") == 0)
				g_mapType = MAP_CS; // rescue map
			else if (cstrcmp(entityClassname, "func_bomb_target") == 0 || cstrcmp(entityClassname, "info_bomb_target") == 0)
				g_mapType = MAP_DE; // defusion map
			else if (cstrcmp(entityClassname, "func_escapezone") == 0)
				g_mapType = MAP_ES;
			else
			{
				// next maps doesn't have map-specific entities, so determine it by name
				const char* map = GetMapName();
				if (cstrncmp(map, "fy_", 3) == 0) // fun map
					g_mapType = MAP_FY;
				else if (cstrncmp(map, "ka_", 3) == 0) // knife arena map
					g_mapType = MAP_KA;
				else if (cstrncmp(map, "awp_", 4) == 0) // awp only map
					g_mapType = MAP_AWP;
				else if (cstrncmp(map, "he_", 3) == 0) // grenade wars
					g_mapType = MAP_HE;
				else if (cstrncmp(map, "ze_", 3) == 0) // zombie escape
					g_mapType = MAP_ZE;
			}
		}
	}

	if (g_mapType != MAP_DE)
		g_bombPlanted = false;

	if (g_mapType <= MAP_MIN || g_mapType >= MAP_MAX)
		g_mapType = MAP_DE;

	RETURN_META_VALUE(MRES_IGNORED, 0);
}

int Spawn_Post(edict_t* ent)
{
	// this function asks the game DLL to spawn (i.e, give a physical existence in the virtual
	// world, in other words to 'display') the entity pointed to by ent in the game. The
	// Spawn() function is one of the functions any entity is supposed to have in the game DLL,
	// and any MOD is supposed to implement one for each of its entities. Post version called
	// only by metamod.

	// solves the bots unable to see through certain types of glass bug.
	if (ent->v.rendermode == kRenderTransTexture)
		ent->v.flags &= ~FL_WORLDBRUSH; // clear the FL_WORLDBRUSH flag out of transparent ents

	// reset bot
	bot = g_botManager->GetBot(ent);
	if (bot)
		bot->NewRound();

	RETURN_META_VALUE(MRES_IGNORED, 0);
}

void Touch_Post(edict_t* pentTouched, edict_t* pentOther)
{
	// this function is called when two entities' bounding boxes enter in collision. For example,
	// when a player walks upon a gun, the player entity bounding box collides to the gun entity
	// bounding box, and the result is that this function is called. It is used by the game for
	// taking the appropriate action when such an event occurs (in our example, the player who
	// is walking upon the gun will "pick it up"). Entities that "touch" others are usually
	// entities having a velocity, as it is assumed that static entities (entities that don't
	// move) will never touch anything. Hence, in our example, the pentTouched will be the gun
	// (static entity), whereas the pentOther will be the player (as it is the one moving). When
	// the two entities both have velocities, for example two players colliding, this function
	// is called twice, once for each entity moving.

	bot = g_botManager->GetBot(pentOther);
	if (bot)
		bot->CheckTouchEntity(pentTouched);

	RETURN_META(MRES_IGNORED);
}

int ClientConnect(edict_t* ent, const char* name, const char* addr, char rejectReason[128])
{
	// this function is called in order to tell the MOD DLL that a client attempts to connect the
	// game. The entity pointer of this client is ent, the name under which he connects is
	// pointed to by the pszName pointer, and its IP address string is pointed by the pszAddress
	// one. Note that this does not mean this client will actually join the game ; he could as
	// well be refused connection by the server later, because of latency timeout, unavailable
	// game resources, or whatever reason. In which case the reason why the game DLL (read well,
	// the game DLL, *NOT* the engine) refuses this player to connect will be printed in the
	// rejectReason string in all letters. Understand that a client connecting process is done
	// in three steps. First, the client requests a connection from the server. This is engine
	// internals. When there are already too many players, the engine will refuse this client to
	// connect, and the game DLL won't even notice. Second, if the engine sees no problem, the
	// game DLL is asked. This is where we are. Once the game DLL acknowledges the connection,
	// the client downloads the resources it needs and synchronizes its local engine with the one
	// of the server. And then, the third step, which comes *AFTER* ClientConnect (), is when the
	// client officially enters the game, through the ClientPutInServer () function, later below.
	// Here we hook this function in order to keep track of the listen server client entity,
	// because a listen server client always connects with a "loopback" address string. Also we
	// tell the bot manager to check the bot population, in order to always have one free slot on
	// the server for incoming clients.

   // callbacks->OnClientConnect (ent, name, addr);

	// check if this client is the listen server client
	if (cstrncmp(addr, "loopback", 9) == 0)
		g_hostEntity = ent; // save the edict of the listen server client...

	LoadEntityData();
	RETURN_META_VALUE(MRES_IGNORED, 0);
}

void ClientDisconnect(edict_t* ent)
{
	// this function is called whenever a client is VOLUNTARILY disconnected from the server,
	// either because the client dropped the connection, or because the server dropped him from
	// the game (latency timeout). The effect is the freeing of a client slot on the server. Note
	// that clients and bots disconnected because of a level change NOT NECESSARILY call this
	// function, because in case of a level change, it's a server shutdown, and not a normal
	// disconnection. I find that completely stupid, but that's it. Anyway it's time to update
	// the bots and players counts, and in case the client disconnecting is a bot, to back its
	// brain(s) up to disk. We also try to notice when a listenserver client disconnects, so as
	// to reset his entity pointer for safety. There are still a few server frames to go once a
	// listen server client disconnects, and we don't want to send him any sort of message then.

	const int clientIndex = ENTINDEX(ent) - 1;

	// check if its a bot
	bot = g_botManager->GetBot(clientIndex);
	if (bot && bot->pev->pContainingEntity == ent)
		g_botManager->Free(clientIndex);

	LoadEntityData();
	RETURN_META(MRES_IGNORED);
}

void ClientUserInfoChanged(edict_t* ent, char* infobuffer)
{
	// this function is called when a player changes model, or changes team. Occasionally it
	// enforces rules on these changes (for example, some MODs don't want to allow players to
	// change their player model). But most commonly, this function is in charge of handling
	// team changes, recounting the teams population, etc...

	if (IsValidBot(ent))
		RETURN_META(MRES_IGNORED);

	const char* passwordField = ebot_password_key.GetString();
	const char* password = ebot_password.GetString();

	if (IsNullString(passwordField) || IsNullString(password))
		RETURN_META(MRES_IGNORED);

	const int clientIndex = ENTINDEX(ent) - 1;
	if (cstrncmp(password, INFOKEY_VALUE(infobuffer, const_cast<char*>(passwordField)), sizeof(password)) == 0)
		g_clients[clientIndex].flags |= CFLAG_OWNER;
	else
		g_clients[clientIndex].flags &= ~CFLAG_OWNER;

	RETURN_META(MRES_IGNORED);
}

void ClientCommand(edict_t* ent)
{
	// this function is called whenever the client whose player entity is ent issues a client
	// command. How it works is that clients all have a global string in their client DLL that
	// stores the command string; if ever that string is filled with characters, the client DLL
	// sends it to the engine as a command to be executed. When the engine has executed that
	// command, that string is reset to zero. By the server side, we can access this string
	// by asking the engine with the CmdArgv(), CmdArgs() and CmdArgc() functions that work just
	// like executable files argument processing work in C (argc gets the number of arguments,
	// command included, args returns the whole string, and argv returns the wanted argument
	// only). Here is a good place to set up either bot debug commands the listen server client
	// could type in his game console, or real new client commands, but we wouldn't want to do
	// so as this is just a bot DLL, not a MOD. The purpose is not to add functionality to
	// clients. Hence it can lack of commenting a bit, since this code is very subject to change.

	const char* command = CMD_ARGV(0);
	const char* arg1 = CMD_ARGV(1);

	static int fillServerTeam = 5;
	static bool fillCommand = false;

	if (!g_isFakeCommand && (ent == g_hostEntity || (g_clients[ENTINDEX(ent) - 1].flags & CFLAG_OWNER)))
	{
		if (cstricmp(command, "ebot") == 0)
		{
			const int state = BotCommandHandler(ent, IsNullString(CMD_ARGV(1)) ? "help" : CMD_ARGV(1), CMD_ARGV(2), CMD_ARGV(3), CMD_ARGV(4), CMD_ARGV(5), CMD_ARGV(6));

			switch (state)
			{
			case 0:
			{
				ClientPrint(ent, print_withtag, "Unknown command: %s", arg1);
				break;
			}
			case 3:
			{
				ClientPrint(ent, print_withtag, "CVar ebot_%s, can be only set via RCON access.", CMD_ARGV(2));
				break;
			}
			case 2:
			{
				ClientPrint(ent, print_withtag, "Command %s, can only be executed from server console.", arg1);
				break;
			}
			}

			RETURN_META(MRES_SUPERCEDE);
		}
		else if (cstricmp(command, "menuselect") == 0 && !IsNullString(arg1) && g_clients[ENTINDEX(ent) - 1].menu)
		{
			Clients* client = &g_clients[ENTINDEX(ent) - 1];
			const int selection = catoi(arg1);

			if (client->menu == &g_menus[12])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				{
					g_waypoint->Add(selection - 1);
					break;
				}
				case 8:
				{
					g_waypoint->Add(100);
					break;
				}
				case 9:
				{
					g_waypoint->SetLearnJumpWaypoint();
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[13]) // set waypoint flag
			{
				switch (selection)
				{
				case 1:
				{
					g_waypoint->ToggleFlags(WAYPOINT_FALLCHECK);
					break;
				}
				case 2:
				{
					g_waypoint->ToggleFlags(WAYPOINT_TERRORIST);
					break;
				}
				case 3:
				{
					g_waypoint->ToggleFlags(WAYPOINT_COUNTER);
					break;
				}
				case 4:
				{
					g_waypoint->ToggleFlags(WAYPOINT_LIFT);
					break;
				}
				case 5:
				{
					g_waypoint->ToggleFlags(WAYPOINT_SNIPER);
					break;
				}
				case 6:
				{
					g_waypoint->ToggleFlags(WAYPOINT_ZMHMCAMP);
					break;
				}
				case 7:
				{
					g_waypoint->DeleteFlags();
					break;
				}
				case 8:
				{
					DisplayMenuToClient(ent, &g_menus[27]);
					break;
				}
				case 9:
				{
					DisplayMenuToClient(ent, &g_menus[26]);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[26]) // set waypoint flag 2
			{
				switch (selection)
				{
				case 1:
				{
					g_waypoint->ToggleFlags(WAYPOINT_USEBUTTON);
					break;
				}
				case 2:
				{
					g_waypoint->ToggleFlags(WAYPOINT_HMCAMPMESH);
					break;
				}
				case 3:
				{
					g_waypoint->ToggleFlags(WAYPOINT_ZOMBIEONLY);
					break;
				}
				case 4:
				{
					g_waypoint->ToggleFlags(WAYPOINT_HUMANONLY);
					break;
				}
				case 5:
				{
					g_waypoint->ToggleFlags(WAYPOINT_ZOMBIEPUSH);
					break;
				}
				case 6:
				{
					g_waypoint->ToggleFlags(WAYPOINT_FALLRISK);
					break;
				}
				case 7:
				{
					g_waypoint->ToggleFlags(WAYPOINT_SPECIFICGRAVITY);
					break;
				}
				case 8:
				{
					DisplayMenuToClient(ent, &g_menus[13]);
					break;
				}
				case 9:
				{
					DisplayMenuToClient(ent, &g_menus[27]);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[27]) // set waypoint flag 3
			{
				switch (selection)
				{
				case 1:
				{
					g_waypoint->ToggleFlags(WAYPOINT_CROUCH);
					break;
				}
				case 2:
				{
					g_waypoint->ToggleFlags(WAYPOINT_ONLYONE);
					break;
				}
				case 3:
				{
					g_waypoint->ToggleFlags(WAYPOINT_WAITUNTIL);
					break;
				}
				case 4:
				{
					g_waypoint->ToggleFlags(WAYPOINT_AVOID);
					break;
				}
				case 8:
				{
					DisplayMenuToClient(ent, &g_menus[26]);
					break;
				}
				case 9:
				{
					DisplayMenuToClient(ent, &g_menus[13]);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[9])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				{
					if (g_waypointOn)
						ServerCommand("ebot waypoint off");
					else
						ServerCommand("ebot waypoint on");
					break;
				}
				case 2:
				{
					g_waypointOn = true;
					g_waypoint->CacheWaypoint();
					break;
				}
				case 3:
				{
					g_waypointOn = true;
					DisplayMenuToClient(ent, &g_menus[20]);
					break;
				}
				case 4:
				{
					g_waypointOn = true;
					g_waypoint->DeletePath();
					break;
				}
				case 5:
				{
					g_waypointOn = true;
					DisplayMenuToClient(ent, &g_menus[12]);
					break;
				}
				case 6:
				{
					g_waypointOn = true;
					g_waypoint->Delete();
					break;
				}
				case 7:
				{
					g_waypointOn = true;
					DisplayMenuToClient(ent, &g_menus[19]);
					break;
				}
				case 8:
				{
					g_waypointOn = true;
					DisplayMenuToClient(ent, &g_menus[11]);
					break;
				}
				case 9:
				{
					DisplayMenuToClient(ent, &g_menus[10]);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[10])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				{
					int terrPoints = 0;
					int ctPoints = 0;
					int goalPoints = 0;
					int rescuePoints = 0;
					int campPoints = 0;
					int sniperPoints = 0;
					int avoidPoints = 0;
					int meshPoints = 0;
					int usePoints = 0;

					int i;
					Path* pointer;
					for (i = 0; i < g_numWaypoints; i++)
					{
						pointer = g_waypoint->GetPath(i);
						if (!pointer)
							continue;

						if (pointer->flags & WAYPOINT_TERRORIST)
							terrPoints++;

						if (pointer->flags & WAYPOINT_COUNTER)
							ctPoints++;

						if (pointer->flags & WAYPOINT_GOAL)
							goalPoints++;

						if (pointer->flags & WAYPOINT_RESCUE)
							rescuePoints++;

						if (pointer->flags & WAYPOINT_CAMP)
							campPoints++;

						if (pointer->flags & WAYPOINT_SNIPER)
							sniperPoints++;

						if (pointer->flags & WAYPOINT_AVOID)
							avoidPoints++;

						if (pointer->flags & WAYPOINT_USEBUTTON)
							usePoints++;

						if (pointer->flags & WAYPOINT_HMCAMPMESH)
							meshPoints++;
					}

					ServerPrintNoTag("Waypoints: %d - T Points: %d\n"
						"CT Points: %d - Goal Points: %d\n"
						"Rescue Points: %d - Camp Points: %d\n"
						"Avoid Points: %d - Sniper Points: %d\n"
						"Use Points: %d - Mesh Points: %d", g_numWaypoints, terrPoints, ctPoints, goalPoints, rescuePoints, campPoints, avoidPoints, sniperPoints, usePoints, meshPoints);

					break;
				}
				case 2:
				{
					g_waypointOn = true;
					g_autoWaypoint &= 1;
					g_autoWaypoint ^= 1;

					CenterPrint("Auto-Waypoint %s", g_autoWaypoint ? "Enabled" : "Disabled");
					break;
				}
				case 3:
				{
					g_waypointOn = true;
					DisplayMenuToClient(ent, &g_menus[13]);
					break;
				}
				case 4:
				{
					if (g_waypoint->NodesValid())
						g_waypoint->Save();
					else
						CenterPrint("Waypoint not saved\nThere are errors, see console");
					break;
				}
				case 5:
				{
					g_waypoint->Save();
					break;
				}
				case 6:
				{
					g_waypoint->Load();
					break;
				}
				case 7:
				{
					if (g_waypoint->NodesValid())
						CenterPrint("Nodes work Find");
					else
						CenterPrint("There are errors, see console");
					break;
				}
				case 8:
				{
					ServerCommand("ebot wp noclip");
					break;
				}
				case 9:
				{
					DisplayMenuToClient(ent, &g_menus[9]);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[11])
			{
				g_waypointOn = true;  // turn waypoints on in case

				const int16 radiusValue[] = { 0, 8, 16, 32, 48, 64, 80, 96, 128 };

				if ((selection >= 1) && (selection <= 9))
					g_waypoint->SetRadius(radiusValue[selection - 1]);

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[0])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				{
					fillCommand = false;
					DisplayMenuToClient(ent, &g_menus[2]);
					break;
				}
				case 2:
				{
					DisplayMenuToClient(ent, &g_menus[1]);
					break;
				}
				case 3:
				{
					fillCommand = true;
					DisplayMenuToClient(ent, &g_menus[6]);
					break;
				}
				case 4:
				{
					g_botManager->KillAll();
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[2])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				{
					g_botManager->AddRandom();
					break;
				}
				case 2:
				{
					DisplayMenuToClient(ent, &g_menus[5]);
					break;
				}
				case 3:
				{
					g_botManager->RemoveRandom();
					break;
				}
				case 4:
				{
					g_botManager->RemoveAll();
					break;
				}
				case 5:
				{
					g_botManager->RemoveMenu(ent, 1);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[1])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display
				extern ConVar ebot_debug;

				switch (selection)
				{
				case 1:
				{
					DisplayMenuToClient(ent, &g_menus[3]);
					break;
				}
				case 2:
				{
					DisplayMenuToClient(ent, &g_menus[9]);
					break;
				}
				case 3:
				{
					DisplayMenuToClient(ent, &g_menus[4]);
					break;
				}
				case 4:
				{
					ebot_debug.SetInt(ebot_debug.GetInt() ^ 1);
					break;
				}
				case 5:
				{
					if (IsAlive(ent))
						DisplayMenuToClient(ent, &g_menus[18]);
					else
					{
						DisplayMenuToClient(ent, nullptr); // reset menu display
						CenterPrint("You're dead, and have no access to this menu");
					}
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[18])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display
				bot = nullptr;

				switch (selection)
				{
				case 1:
				case 2:
				{
					if (FindNearestPlayer(reinterpret_cast<void**>(&bot), client->ent, 4096.0f, true, true, true))
					{
						if (!(bot->pev->weapons & (1 << Weapon::C4)) && !bot->HasHostage())
						{
							if (selection == 1)
							{
								// TODO: add process for this
								bot->ResetDoubleJumpState();
								bot->m_doubleJumpOrigin = GetEntityOrigin(client->ent);
								bot->m_doubleJumpEntity = client->ent;
								bot->ChatSay(true, FormatBuffer("Ok %s, i will help you!", GetEntityName(ent)));
							}
							else if (selection == 2)
								bot->ResetDoubleJumpState();
							break;
						}
					}
					break;
				}
				case 3:
				case 4:
				{
					if (FindNearestPlayer(reinterpret_cast<void**>(&bot), ent, 300.0f, true, true, true))
						bot->DiscardWeaponForUser(ent, selection == 4 ? false : true);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[19])
			{
				const float autoDistanceValue[] = { 0.0f, 100.0f, 130.0f, 160.0f, 190.0f, 220.0f, 250.0f };

				if (selection >= 1 && selection <= 7)
					g_autoPathDistance = autoDistanceValue[selection - 1];

				if (g_autoPathDistance == 0.0f)
					CenterPrint("AutoPath disabled");
				else
					CenterPrint("AutoPath maximum distance set to %f", g_autoPathDistance);

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[20])
			{
				switch (selection)
				{
				case 1:
				{
					g_waypoint->CreatePath(PATHCON_OUTGOING);
					break;
				}
				case 2:
				{
					g_waypoint->CreatePath(PATHCON_INCOMING);
					break;
				}
				case 3:
				{
					g_waypoint->CreatePath(PATHCON_BOTHWAYS);
					break;
				}
				case 4:
				{
					g_waypoint->CreatePath(PATHCON_JUMPING);
					break;
				}
				case 5:
				{
					g_waypoint->CreatePath(PATHCON_BOOSTING);
					break;
				}
				case 6:
				{
					g_waypoint->DeletePath();
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[21])
			{
				DisplayMenuToClient(ent, nullptr);
				switch (selection)
				{
				case 1:
				{
					DisplayMenuToClient(ent, &g_menus[22]);  // Add Waypoint
					break;
				}
				case 2:
				{
					DisplayMenuToClient(ent, &g_menus[13]); //Set Waypoint Flag
					break;
				}
				case 3:
				{
					DisplayMenuToClient(ent, &g_menus[20]); // Create Path
					break;
				}
				case 4:
				{
					DisplayMenuToClient(ent, &g_menus[11]); // Set Waypoint Radius
					break;
				}
				case 5:
				{
					DisplayMenuToClient(ent, &g_menus[21]);
					g_waypoint->TeleportWaypoint(); // Teleport to Waypoint
					break;
				}
				case 6:
				{
					DisplayMenuToClient(ent, &g_menus[21]);
					g_waypoint->Delete(); // Delete Waypoint 
					break;
				}
				case 9:
				{
					g_waypoint->Save();
					DisplayMenuToClient(ent, &g_menus[26]);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[22])
			{
				switch (selection)
				{
				case 1:
				{
					g_waypoint->Add(0);
					g_waypoint->SetRadius(64);
					break;
				}
				case 2:
				case 3:
				case 5:
				{
					g_waypoint->Add(selection - 1);
					g_waypoint->SetRadius(64);
					break;
				}
				case 4:
				{
					g_waypoint->Add(selection - 1);
					g_waypoint->SetRadius(0);
					break;
				}
				case 6:
				{
					g_waypoint->Add(100);
					g_waypoint->SetRadius(32);
					break;
				}
				case 7:
				{
					g_waypoint->Add(5);
					g_waypoint->Add(6);
					g_waypoint->SetRadius(0);
					DisplayMenuToClient(ent, &g_menus[24]);
					break;
				}
				case 8:
				{
					g_waypoint->SetLearnJumpWaypoint();
					ChatPrint("[SgdWP] You could Jump now, system will auto save your Jump Point");
					break;
				}
				case 9:
				{
					DisplayMenuToClient(ent, &g_menus[23]);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, &g_menus[21]);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[23])
			{
				DisplayMenuToClient(ent, &g_menus[23]);

				switch (selection)
				{
				case 1:
				{
					g_waypoint->Add(0);
					g_waypoint->ToggleFlags(WAYPOINT_LIFT);
					g_waypoint->SetRadius(0);
					break;
				}
				case 2:
				{
					g_waypoint->Add(5);
					g_waypoint->Add(6);
					g_waypoint->ToggleFlags(WAYPOINT_SNIPER);
					g_waypoint->SetRadius(0);
					DisplayMenuToClient(ent, &g_menus[24]);
					break;
				}
				case 3:
				{
					g_waypoint->Add(0);
					g_waypoint->ToggleFlags(WAYPOINT_ZMHMCAMP);
					g_waypoint->SetRadius(64);
					break;
				}
				case 4:
				{
					g_waypoint->Add(0);
					g_waypoint->ToggleFlags(WAYPOINT_HMCAMPMESH);
					g_waypoint->SetRadius(64);
					break;
				}
				case 9:
				{
					DisplayMenuToClient(ent, &g_menus[22]);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, &g_menus[21]);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[24])
			{
				DisplayMenuToClient(ent, &g_menus[22]);

				switch (selection)
				{
				case 1:
				{
					g_waypoint->ToggleFlags(WAYPOINT_TERRORIST);
					break;
				}
				case 2:
				{
					g_waypoint->ToggleFlags(WAYPOINT_COUNTER);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[25])
			{
				DisplayMenuToClient(ent, nullptr);

				switch (selection)
				{
				case 1:
				{
					g_waypoint->Save();
					break;
				}
				case 2:
				{
					DisplayMenuToClient(ent, &g_menus[21]);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[5])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				client->menu = &g_menus[4];

				switch (selection)
				{
				case 1:
				{
					g_storeAddbotVars[0] = crandomint(0, 20);
					break;
				}
				case 2:
				{
					g_storeAddbotVars[0] = crandomint(20, 40);
					break;
				}
				case 3:
				{
					g_storeAddbotVars[0] = crandomint(40, 60);
					break;
				}
				case 4:
				{
					g_storeAddbotVars[0] = crandomint(60, 80);
					break;
				}
				case 5:
				{
					g_storeAddbotVars[0] = crandomint(80, 99);
					break;
				}
				case 6:
				{
					g_storeAddbotVars[0] = 100;
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				if (client->menu == &g_menus[4])
					DisplayMenuToClient(ent, &g_menus[4]);

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[6] && fillCommand)
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				{
					// turn off cvars if specified team
					CVAR_SET_STRING("mp_limitteams", "0");
					CVAR_SET_STRING("mp_autoteambalance", "0");
				}
				case 5:
				{
					fillServerTeam = selection;
					DisplayMenuToClient(ent, &g_menus[5]);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[4] && fillCommand)
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
					g_botManager->FillServer(fillServerTeam, selection - 2, g_storeAddbotVars[0]);
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[6])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 5:
				{
					g_storeAddbotVars[1] = selection;
					if (selection == 5)
					{
						g_storeAddbotVars[2] = 5;
						g_botManager->AddBot("", g_storeAddbotVars[0], g_storeAddbotVars[3], g_storeAddbotVars[1], g_storeAddbotVars[2]);
					}
					else
					{
						if (selection == 1)
							DisplayMenuToClient(ent, &g_menus[7]);
						else
							DisplayMenuToClient(ent, &g_menus[8]);
					}
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[4])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				{
					g_storeAddbotVars[3] = selection - 2;
					DisplayMenuToClient(ent, &g_menus[6]);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[7] || client->menu == &g_menus[8])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				{
					g_storeAddbotVars[2] = selection;
					g_botManager->AddBot("", g_storeAddbotVars[0], g_storeAddbotVars[3], g_storeAddbotVars[1], g_storeAddbotVars[2]);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[3])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				{
					g_botManager->SetWeaponMode(selection);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, nullptr);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[14])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				{
					g_botManager->GetBot(selection - 1)->Kick();
					break;
				}
				case 9:
				{
					g_botManager->RemoveMenu(ent, 2);
					break;
				}
				case 10:
				{
					DisplayMenuToClient(ent, &g_menus[2]);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[15])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				{
					g_botManager->GetBot(selection + 8 - 1)->Kick();
					break;
				}
				case 9:
				{
					g_botManager->RemoveMenu(ent, 3);
					break;
				}
				case 10:
				{
					g_botManager->RemoveMenu(ent, 1);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[16])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				{
					g_botManager->GetBot(selection + 16 - 1)->Kick();
					break;
				}
				case 9:
				{
					g_botManager->RemoveMenu(ent, 4);
					break;
				}
				case 10:
				{
					g_botManager->RemoveMenu(ent, 2);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
			else if (client->menu == &g_menus[17])
			{
				DisplayMenuToClient(ent, nullptr); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				{
					g_botManager->GetBot(selection + 24 - 1)->Kick();
					break;
				}
				case 10:
				{
					g_botManager->RemoveMenu(ent, 3);
					break;
				}
				}

				RETURN_META(MRES_SUPERCEDE);
			}
		}
	}

	if (!g_isFakeCommand && (cstricmp(command, "say") == 0 || cstricmp(command, "say_team") == 0))
	{
		bot = nullptr;

		if (cstrncmp(arg1, "dropme", 7) == 0 || cstrncmp(arg1, "dropc4", 7) == 0)
		{
			if (FindNearestPlayer(reinterpret_cast<void**>(&bot), ent, 300.0f, true, true, true))
			{
				char* c4;
				cstrcpy(c4, arg1);
				bot->DiscardWeaponForUser(ent, IsNullString(cstrstr(c4, "c4")) ? false : true);
			}

			return;
		}

		int team = -1;
		if (cstrncmp(command, "say_team", 9) == 0)
			team = GetTeam(ent);

		for (const auto& bot : g_botManager->m_bots)
		{
			if (!bot)
				continue;

			if (team != -1 && team != bot->m_team)
				continue;

			bot->m_sayTextBuffer.entityIndex = ENTINDEX(ent);

			if (IsNullString(CMD_ARGS()))
				continue;

			cstrncpy(bot->m_sayTextBuffer.sayText, CMD_ARGS(), sizeof(bot->m_sayTextBuffer.sayText));
			bot->m_sayTextBuffer.timeNextChat = engine->GetTime() + bot->m_sayTextBuffer.chatDelay;
		}
	}

	const int clientIndex = ENTINDEX(ent) - 1;

	// check if this player alive, and issue something
	if ((g_clients[clientIndex].flags & CFLAG_ALIVE) && g_radioSelect[clientIndex] != 0 && cstrncmp(command, "menuselect", 10) == 0)
	{
		int radioCommand = catoi(arg1);
		if (radioCommand != 0)
		{
			radioCommand += 10 * (g_radioSelect[clientIndex] - 1);

			if (radioCommand != Radio::Affirmative && radioCommand != Radio::Negative && radioCommand != Radio::ReportingIn)
			{
				for (const auto& bot : g_botManager->m_bots)
				{
					// validate bot
					if (bot && bot->m_team == g_clients[clientIndex].team && VARS(ent) != bot->pev && bot->m_radioOrder == 0)
					{
						bot->m_radioOrder = radioCommand;
						bot->m_radioEntity = ent;
					}
				}
			}

			if (g_clients[clientIndex].team == 0 || g_clients[clientIndex].team == 1)
				g_lastRadioTime[g_clients[clientIndex].team] = engine->GetTime();
		}
		g_radioSelect[clientIndex] = 0;
	}
	else if (cstrncmp(command, "radio", 5) == 0)
		g_radioSelect[clientIndex] = catoi(&command[5]);

	RETURN_META(MRES_IGNORED);
}

void ServerActivate(edict_t* pentEdictList, int edictCount, int clientMax)
{
	// this function is called when the server has fully loaded and is about to manifest itself
	// on the network as such. Since a mapchange is actually a server shutdown followed by a
	// restart, this function is also called when a new map is being loaded. Hence it's the
	// perfect place for doing initialization stuff for our bots, such as reading the BSP data,
	// loading the bot profiles, and drawing the world map (ie, filling the navigation hashtable).
	// Once this function has been called, the server can be considered as "running".

	// initialize all config files
	InitConfig();

	// do level initialization stuff here...
	g_waypoint->Initialize();
	g_waypoint->Load();

	// execute main config
	ServerCommand("exec addons/ebot/ebot.cfg");

	if (TryFileOpen(FormatBuffer("%s/maps/%s_ebot.cfg", GetModName(), GetMapName())))
	{
		ServerCommand("exec maps/%s_ebot.cfg", GetMapName());
		ServerPrint("Executing Map-Specific config file");
	}

	g_botManager->InitQuota();

	secondTimer = 0.0f;
	updateTimer = 0.0f;
	g_pathTimer = 0.0f;

	RETURN_META(MRES_IGNORED);
}

void SetPing(edict_t* to)
{
	if (FNullEnt(to))
		return;

	if (!(to->v.flags & FL_CLIENT))
		return;

	if (g_fakePingUpdate < engine->GetTime())
	{
		// update timer if someone lookin' at scoreboard
		if (to->v.buttons & IN_SCORE || to->v.oldbuttons & IN_SCORE)
			g_fakePingUpdate = engine->GetTime() + 2.0f;
		else
			return;
	}

	int index;
	static int sending{};
	for (const auto& bot : g_botManager->m_bots)
	{
		if (!bot)
			continue;

		index = bot->m_index - 1;
		switch (sending)
		{
		case 0:
		{
			// start a new message
			MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, 17, nullptr, to);
			WRITE_BYTE((bot->m_pingOffset[sending] * 64) + (1 + 2 * index));
			WRITE_SHORT(bot->m_ping[sending]);
			sending++;
		}
		case 1:
		{
			// append additional data
			WRITE_BYTE((bot->m_pingOffset[sending] * 128) + (2 + 4 * index));
			WRITE_SHORT(bot->m_ping[sending]);
			sending++;
		}
		case 2:
		{
			// append additional data and end message
			WRITE_BYTE(4 + 8 * index);
			WRITE_SHORT(bot->m_ping[sending]);
			WRITE_BYTE(0);
			MESSAGE_END();
			sending = 0;
		}
		}
	}

	// end message if not yet sent
	if (sending)
	{
		WRITE_BYTE(0);
		MESSAGE_END();
	}
}

void UpdateClientData(const struct edict_s* ent, int sendweapons, struct clientdata_s* cd)
{
	extern ConVar ebot_ping;
	if (ebot_ping.GetBool())
		SetPing(const_cast<edict_t*>(ent));

	RETURN_META(MRES_IGNORED);
}

void JustAStuff(void)
{
	// code below is executed only on dedicated server
	if (IsDedicatedServer())
	{
		const char* password = ebot_password.GetString();
		const char* key = ebot_password_key.GetString();
		for (const auto& client : g_clients)
		{
			if (client.index < 0)
				continue;

			if (FNullEnt(client.ent))
				continue;

			if (!(client.ent->v.flags & FL_CLIENT))
				continue;

			if (client.flags & CFLAG_OWNER)
			{
				if (IsNullString(key) && IsNullString(password))
					g_clients[client.index].flags &= ~CFLAG_OWNER;
				else if (cstrcmp(password, INFOKEY_VALUE(GET_INFOKEYBUFFER(client.ent), key)) == 0)
				{
					g_clients[client.index].flags &= ~CFLAG_OWNER;
					ServerPrint("Player %s had lost remote access to ebot.", GetEntityName(client.ent));
				}
			}
			else if (IsNullString(key) && IsNullString(password))
			{
				if (cstrcmp(password, INFOKEY_VALUE(GET_INFOKEYBUFFER(client.ent), key)) == 0)
				{
					g_clients[client.index].flags |= CFLAG_OWNER;
					ServerPrint("Player %s had gained full remote access to ebot.", GetEntityName(client.ent));
				}
			}
		}
	}
	else if (!FNullEnt(g_hostEntity))
	{
		if (g_navmeshOn)
		{
			for (const auto& bot : g_botManager->m_bots)
			{
				if (bot)
				{
					g_botManager->RemoveAll();
					break;
				}
			}

			g_navmesh->DrawNavArea();
		}
		else if (g_waypointOn)
		{
			for (const auto& bot : g_botManager->m_bots)
			{
				if (bot)
				{
					g_botManager->RemoveAll();
					break;
				}
			}

			g_waypoint->Think();

			if (ebot_showwp.GetBool())
				ebot_showwp.SetInt(0);
		}
		else if (ebot_showwp.GetBool())
			g_waypoint->ShowWaypointMsg();
		else
			CheckWelcomeMessage();
	}
}

void FrameThread(void)
{
	LoadEntityData();
	JustAStuff();

	g_bombDefusing = false;
	if (g_bombPlanted)
		g_waypoint->SetBombPosition();

	if (g_gameVersion & Game::Xash)
	{
		const auto simulate = g_engfuncs.pfnCVarGetPointer("sv_forcesimulating");
		if (simulate && simulate->value != 1.0f)
			g_engfuncs.pfnCVarSetFloat("sv_forcesimulating", 1.0f);
	}

	float ut = 0.5f;
	if (g_navmeshOn && !g_analyzewaypoints)
		ut = 0.05f;

	secondTimer = engine->GetTime() + ut;
}

void StartFrame(void)
{
	// this function starts a video frame. It is called once per video frame by the engine. If
	// you run Half-Life at 90 fps, this function will then be called 90 times per second. By
	// placing a hook on it, we have a good place to do things that should be done continuously
	// during the game, for example making the bots think (yes, because no Think() function exists
	// for the bots by the MOD side, remember). Also here we have control on the bot population,
	// for example if a new player joins the server, we should disconnect a bot, and if the
	// player population decreases, we should fill the server with other bots.

	if (secondTimer < engine->GetTime())
		FrameThread();
	else
	{
		if (g_analyzenavmesh)
			g_navmesh->Analyze();
		else if (g_analyzewaypoints)
			g_waypoint->Analyze();
		else // keep bot number up to date
			g_botManager->MaintainBotQuota();
	}

	RETURN_META(MRES_IGNORED);
}

void StartFrame_Post(void)
{
	// this function starts a video frame. It is called once per video frame by the engine. If
	// you run Half-Life at 90 fps, this function will then be called 90 times per second. By
	// placing a hook on it, we have a good place to do things that should be done continuously
	// during the game, for example making the bots think (yes, because no Think() function exists
	// for the bots by the MOD side, remember).  Post version called only by metamod.

	// **** AI EXECUTION STARTS ****
	if (updateTimer < engine->GetTime())
	{
		g_botManager->Think();
		updateTimer = engine->GetTime() + (1.0f / ebot_think_fps.GetFloat());
	}
	// **** AI EXECUTION FINISH ****

	RETURN_META(MRES_IGNORED);
}

void GameDLLInit_Post(void)
{
	// this function is a one-time call, and appears to be the second function called in the
	// DLL after FuncPointers_t() has been called. Its purpose is to tell the MOD DLL to
	// initialize the game before the engine actually hooks into it with its video frames and
	// clients connecting. Note that it is a different step than the *server* initialization.
	// This one is called once, and only once, when the game process boots up before the first
	// server is enabled. Here is a good place to do our own game session initialization, and
	// to register by the engine side the server commands we need to administrate our bots. Post
	// version, called only by metamod.

	// determine version of currently running cs
	DetectCSVersion();

	RETURN_META(MRES_IGNORED);
}

const char* pfnCmd_Args(void)
{
	// this function returns a pointer to the whole current client command string. Since bots
	// have no client DLL and we may want a bot to execute a client command, we had to implement
	// a g_xgv string in the bot DLL for holding the bots' commands, and also keep track of the
	// argument count. Hence this hook not to let the engine ask an unexistent client DLL for a
	// command we are holding here. Of course, real clients commands are still retrieved the
	// normal way, by asking the engine.

	// is this a bot issuing that client command?
	if (g_isFakeCommand)
	{
		// is it a "say" or "say_team" client command?
		if (cstrncmp("say ", g_fakeArgv, 4) == 0)
			RETURN_META_VALUE(MRES_SUPERCEDE, g_fakeArgv + 4);

		if (cstrncmp("say_team ", g_fakeArgv, 9) == 0)
			RETURN_META_VALUE(MRES_SUPERCEDE, g_fakeArgv + 9);

		RETURN_META_VALUE(MRES_SUPERCEDE, g_fakeArgv);
	}

	RETURN_META_VALUE(MRES_IGNORED, nullptr);
}

const char* pfnCmd_Argv(int argc)
{
	// this function returns a pointer to a certain argument of the current client command. Since
	// bots have no client DLL and we may want a bot to execute a client command, we had to
	// implement a g_xgv string in the bot DLL for holding the bots' commands, and also keep
	// track of the argument count. Hence this hook not to let the engine ask an unexistent client
	// DLL for a command we are holding here. Of course, real clients commands are still retrieved
	// the normal way, by asking the engine.

	// is this a bot issuing that client command ?
	if (g_isFakeCommand)
		RETURN_META_VALUE(MRES_SUPERCEDE, GetField(g_fakeArgv, argc));

	RETURN_META_VALUE(MRES_IGNORED, nullptr);
}

gamedll_funcs_t gameDLLFunc;
exportc int GetEntityAPI2(DLL_FUNCTIONS* functionTable, int* /*interfaceVersion*/)
{
	// this function is called right after FuncPointers_t() by the engine in the game DLL (or
	// what it BELIEVES to be the game DLL), in order to copy the list of MOD functions that can
	// be called by the engine, into a memory block pointed to by the functionTable pointer
	// that is passed into this function (explanation comes straight from botman). This allows
	// the Half-Life engine to call these MOD DLL functions when it needs to spawn an entity,
	// connect or disconnect a player, call Think() functions, Touch() functions, or Use()
	// functions, etc. The bot DLL passes its OWN list of these functions back to the Half-Life
	// engine, and then calls the MOD DLL's version of GetEntityAPI to get the REAL gamedll
	// functions this time (to use in the bot code).

	cmemset(functionTable, 0, sizeof(DLL_FUNCTIONS));
	functionTable->pfnGameInit = GameDLLInit;
	functionTable->pfnSpawn = Spawn;
	functionTable->pfnClientConnect = ClientConnect;
	functionTable->pfnClientDisconnect = ClientDisconnect;
	functionTable->pfnClientUserInfoChanged = ClientUserInfoChanged;
	functionTable->pfnClientCommand = ClientCommand;
	functionTable->pfnServerActivate = ServerActivate;
	functionTable->pfnStartFrame = StartFrame;
	functionTable->pfnUpdateClientData = UpdateClientData;

	return true;
}

exportc int GetEntityAPI2_Post(DLL_FUNCTIONS* functionTable, int* /*interfaceVersion*/)
{
	// this function is called right after FuncPointers_t() by the engine in the game DLL (or
	// what it BELIEVES to be the game DLL), in order to copy the list of MOD functions that can
	// be called by the engine, into a memory block pointed to by the functionTable pointer
	// that is passed into this function (explanation comes straight from botman). This allows
	// the Half-Life engine to call these MOD DLL functions when it needs to spawn an entity,
	// connect or disconnect a player, call Think() functions, Touch() functions, or Use()
	// functions, etc. The bot DLL passes its OWN list of these functions back to the Half-Life
	// engine, and then calls the MOD DLL's version of GetEntityAPI to get the REAL gamedll
	// functions this time (to use in the bot code). Post version, called only by metamod.

	cmemset(functionTable, 0, sizeof(DLL_FUNCTIONS));
	functionTable->pfnSpawn = Spawn_Post;
	functionTable->pfnStartFrame = StartFrame_Post;
	functionTable->pfnGameInit = GameDLLInit_Post;
	functionTable->pfnTouch = Touch_Post;

	return true;
}

exportc int GetEngineFunctions(enginefuncs_t* functionTable, int* /*interfaceVersion*/)
{
	cmemset(functionTable, 0, sizeof(enginefuncs_t));

	functionTable->pfnMessageBegin = [](int msgDest, int msgType, const float* origin, edict_t* ed)
	{
			// store the message type in our own variables, since the GET_USER_MSG_ID will just do a lot of strcmp's...
			if (g_netMsg->GetId(NETMSG_MONEY) == -1)
			{
				g_netMsg->SetId(NETMSG_VGUI, GET_USER_MSG_ID(PLID, "VGUIMenu", nullptr));
				g_netMsg->SetId(NETMSG_SHOWMENU, GET_USER_MSG_ID(PLID, "ShowMenu", nullptr));
				g_netMsg->SetId(NETMSG_WLIST, GET_USER_MSG_ID(PLID, "WeaponList", nullptr));
				g_netMsg->SetId(NETMSG_CURWEAPON, GET_USER_MSG_ID(PLID, "CurWeapon", nullptr));
				g_netMsg->SetId(NETMSG_AMMOX, GET_USER_MSG_ID(PLID, "AmmoX", nullptr));
				g_netMsg->SetId(NETMSG_AMMOPICK, GET_USER_MSG_ID(PLID, "AmmoPickup", nullptr));
				//g_netMsg->SetId(NETMSG_DAMAGE, GET_USER_MSG_ID(PLID, "Damage", nullptr));
				g_netMsg->SetId(NETMSG_MONEY, GET_USER_MSG_ID(PLID, "Money", nullptr));
				g_netMsg->SetId(NETMSG_STATUSICON, GET_USER_MSG_ID(PLID, "StatusIcon", nullptr));
				g_netMsg->SetId(NETMSG_DEATH, GET_USER_MSG_ID(PLID, "DeathMsg", nullptr));
				g_netMsg->SetId(NETMSG_SCREENFADE, GET_USER_MSG_ID(PLID, "ScreenFade", nullptr));
				g_netMsg->SetId(NETMSG_HLTV, GET_USER_MSG_ID(PLID, "HLTV", nullptr));
				g_netMsg->SetId(NETMSG_TEXTMSG, GET_USER_MSG_ID(PLID, "TextMsg", nullptr));
				g_netMsg->SetId(NETMSG_SCOREINFO, GET_USER_MSG_ID(PLID, "ScoreInfo", nullptr));
				g_netMsg->SetId(NETMSG_BARTIME, GET_USER_MSG_ID(PLID, "BarTime", nullptr));
				g_netMsg->SetId(NETMSG_SENDAUDIO, GET_USER_MSG_ID(PLID, "SendAudio", nullptr));
				g_netMsg->SetId(NETMSG_SAYTEXT, GET_USER_MSG_ID(PLID, "SayText", nullptr));
				g_netMsg->SetId(NETMSG_BOTVOICE, GET_USER_MSG_ID(PLID, "BotVoice", nullptr));
			}

			if (msgDest == MSG_SPEC && msgType == g_netMsg->GetId(NETMSG_HLTV))
				g_netMsg->SetMessage(NETMSG_HLTV);

			g_netMsg->HandleMessageIfRequired(msgType, NETMSG_WLIST);

			if (msgDest == MSG_ALL)
			{
				//g_netMsg->HandleMessageIfRequired (msgType, NETMSG_SCOREINFO);
				g_netMsg->HandleMessageIfRequired(msgType, NETMSG_DEATH);
				g_netMsg->HandleMessageIfRequired(msgType, NETMSG_TEXTMSG);

				if (msgType == SVC_INTERMISSION)
				{
					for (const auto& bot : g_botManager->m_bots)
					{
						if (bot)
							bot->m_isAlive = false;
					}
				}
			}
			else
			{
				bot = g_botManager->GetBot(ed);

				// is this message for a bot?
				if (bot && bot->GetEntity() == ed)
				{
					g_netMsg->SetBot(bot);

					// message handling is done in usermsg.cpp
					g_netMsg->HandleMessageIfRequired(msgType, NETMSG_VGUI);
					g_netMsg->HandleMessageIfRequired(msgType, NETMSG_CURWEAPON);
					g_netMsg->HandleMessageIfRequired(msgType, NETMSG_AMMOX);
					g_netMsg->HandleMessageIfRequired(msgType, NETMSG_AMMOPICK);
					//g_netMsg->HandleMessageIfRequired(msgType, NETMSG_DAMAGE);
					g_netMsg->HandleMessageIfRequired(msgType, NETMSG_MONEY);
					g_netMsg->HandleMessageIfRequired(msgType, NETMSG_STATUSICON);
					g_netMsg->HandleMessageIfRequired(msgType, NETMSG_SCREENFADE);
					g_netMsg->HandleMessageIfRequired(msgType, NETMSG_BARTIME);
					g_netMsg->HandleMessageIfRequired(msgType, NETMSG_TEXTMSG);
					g_netMsg->HandleMessageIfRequired(msgType, NETMSG_SHOWMENU);
				}
			}

		RETURN_META(MRES_IGNORED);
	};

	functionTable->pfnMessageEnd = [](void)
	{
		g_netMsg->Reset();
		RETURN_META(MRES_IGNORED);
	};

	functionTable->pfnWriteByte = [](int value)
	{
		g_netMsg->Execute((void*)&value);
		RETURN_META(MRES_IGNORED);
	};

	functionTable->pfnWriteChar = [](int value)
	{
		g_netMsg->Execute((void*)&value);
		RETURN_META(MRES_IGNORED);
	};

	functionTable->pfnWriteShort = [](int value)
	{
		g_netMsg->Execute((void*)&value);
		RETURN_META(MRES_IGNORED);
	};

	functionTable->pfnWriteLong = [](int value)
	{
		g_netMsg->Execute((void*)&value);
		RETURN_META(MRES_IGNORED);
	};

	functionTable->pfnWriteAngle = [](float value)
	{
		g_netMsg->Execute((void*)&value);
		RETURN_META(MRES_IGNORED);
	};

	functionTable->pfnWriteCoord = [](float value)
	{
		g_netMsg->Execute((void*)&value);
		RETURN_META(MRES_IGNORED);
	};

	functionTable->pfnWriteString = [](const char* sz)
	{
		g_netMsg->Execute((void*)sz);
		RETURN_META(MRES_IGNORED);
	};

	functionTable->pfnWriteEntity = [](int value)
	{
		g_netMsg->Execute((void*)&value);
		RETURN_META(MRES_IGNORED);
	};

	functionTable->pfnClientPrintf = [](edict_t* ent, PRINT_TYPE printType, const char* message)
	{
		// this function prints the text message string pointed to by message by the client side of
		// the client entity pointed to by ent, in a manner depending of printType (print_console,
		// print_center or print_chat). Be certain never to try to feed a bot with this function,
		// as it will crash your server. Why would you, anyway ? bots have no client DLL as far as
		// we know, right ? But since stupidity rules this world, we do a preventive check :)

		if (IsValidBot(ent) || ent->v.flags & FL_FAKECLIENT || ent->v.flags & FL_DORMANT)
			RETURN_META(MRES_SUPERCEDE);

		RETURN_META(MRES_IGNORED);
	};

	functionTable->pfnCmd_Args = pfnCmd_Args;
	functionTable->pfnCmd_Argv = pfnCmd_Argv;

	functionTable->pfnCmd_Argc = [](void)
	{
		// this function returns the number of arguments the current client command string has. Since
		// bots have no client DLL and we may want a bot to execute a client command, we had to
		// implement a g_xgv string in the bot DLL for holding the bots' commands, and also keep
		// track of the argument count. Hence this hook not to let the engine ask an unexistent client
		// DLL for a command we are holding here. Of course, real clients commands are still retrieved
		// the normal way, by asking the engine.

		// is this a bot issuing that client command ?
		if (g_isFakeCommand)
			RETURN_META_VALUE(MRES_SUPERCEDE, g_fakeArgc);

		RETURN_META_VALUE(MRES_IGNORED, 0);
	};

	functionTable->pfnSetClientMaxspeed = [](const edict_t* ent, float newMaxspeed)
	{
		bot = g_botManager->GetBot(const_cast<edict_t*>(ent));

		// check wether it's not a bot, fair cheat :(
		if (bot)
		{
			bot->pev->maxspeed = newMaxspeed * 1.05f;
			(*g_engfuncs.pfnSetClientMaxspeed) (ent, bot->pev->maxspeed);
			RETURN_META(MRES_SUPERCEDE);
		}

		RETURN_META(MRES_IGNORED);
	};

	functionTable->pfnGetPlayerAuthId = [](edict_t* e) -> const char* 
	{
		//if (IsValidBot(e))
		RETURN_META_VALUE(MRES_SUPERCEDE, "EBOT");

		//RETURN_META_VALUE(MRES_IGNORED, 0);
	};

	functionTable->pfnGetPlayerWONId = [](edict_t* e) -> unsigned int
	{
		if (IsValidBot(e))
			RETURN_META_VALUE(MRES_SUPERCEDE, 0);

		RETURN_META_VALUE(MRES_IGNORED, 0);
	};

	return true;
}

exportc int Meta_Query(char* ifvers, plugin_info_t** pPlugInfo, mutil_funcs_t* pMetaUtilFuncs)
{
	// this function is the first function ever called by metamod in the plugin DLL. Its purpose
	// is for metamod to retrieve basic information about the plugin, such as its meta-interface
	// version, for ensuring compatibility with the current version of the running metamod.

	// keep track of the pointers to metamod function tables metamod gives us
	gpMetaUtilFuncs = pMetaUtilFuncs;
	*pPlugInfo = &Plugin_info;

	// check for interface version compatibility
	if (cstrcmp(ifvers, Plugin_info.ifvers) != 0)
	{
		int metaMajor = 0, metaMinor = 0, pluginMajor = 0, pluginMinor = 0;

		LOG_CONSOLE(PLID, "%s: meta-interface version mismatch (metamod: %s, %s: %s)", Plugin_info.name, ifvers, Plugin_info.name, Plugin_info.ifvers);
		LOG_MESSAGE(PLID, "%s: meta-interface version mismatch (metamod: %s, %s: %s)", Plugin_info.name, ifvers, Plugin_info.name, Plugin_info.ifvers);

		// if plugin has later interface version, it's incompatible (update metamod)
		sscanf(ifvers, "%d:%d", &metaMajor, &metaMinor);
		sscanf(META_INTERFACE_VERSION, "%d:%d", &pluginMajor, &pluginMinor);

		if (pluginMajor > metaMajor || (pluginMajor == metaMajor && pluginMinor > metaMinor))
		{
			LOG_CONSOLE(PLID, "metamod version is too old for this plugin; update metamod");
			LOG_MMERROR(PLID, "metamod version is too old for this plugin; update metamod");

			return false;
		}

		// if plugin has older major interface version, it's incompatible (update plugin)
		else if (pluginMajor < metaMajor)
		{
			LOG_CONSOLE(PLID, "metamod version is incompatible with this plugin; please find a newer version of this plugin");
			LOG_MMERROR(PLID, "metamod version is incompatible with this plugin; please find a newer version of this plugin");

			return false;
		}
	}

	return true; // tell metamod this plugin looks safe
}

exportc int Meta_Attach(PLUG_LOADTIME now, metamod_funcs_t* functionTable, meta_globals_t* pMGlobals, gamedll_funcs_t* pGamedllFuncs)
{
	// this function is called when metamod attempts to load the plugin. Since it's the place
	// where we can tell if the plugin will be allowed to run or not, we wait until here to make
	// our initialization stuff, like registering CVARs and dedicated server commands.

	// are we allowed to load this plugin now ?
	if (now > Plugin_info.loadable)
	{
		LOG_CONSOLE(PLID, "%s: plugin NOT attaching (can't load plugin right now)", Plugin_info.name);
		LOG_MMERROR(PLID, "%s: plugin NOT attaching (can't load plugin right now)", Plugin_info.name);

		return false; // returning false prevents metamod from attaching this plugin
	}

	// keep track of the pointers to engine function tables metamod gives us
	gpMetaGlobals = pMGlobals;
	cmemcpy(functionTable, &gMetaFunctionTable, sizeof(metamod_funcs_t));
	gpGamedllFuncs = pGamedllFuncs;

	return true; // returning true enables metamod to attach this plugin
}

exportc int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason)
{
	// this function is called when metamod unloads the plugin. A basic check is made in order
	// to prevent unloading the plugin if its processing should not be interrupted.

	// is metamod allowed to unload the plugin?
	if (now > Plugin_info.unloadable && reason != PNL_CMD_FORCED)
	{
		LOG_CONSOLE(PLID, "%s: plugin not detaching (can't unload plugin right now)", Plugin_info.name);
		LOG_MMERROR(PLID, "%s: plugin not detaching (can't unload plugin right now)", Plugin_info.name);

		return false; // returning false prevents metamod from unloading this plugin
	}

	g_botManager->RemoveAll(); // kick all bots off this server

	return true;
}

exportc void Meta_Init(void)
{
	// this function is called by metamod, before any other interface functions. Purpose of this
	// function to give plugin a chance to determine is plugin running under metamod or not.
}

DLL_GIVEFNPTRSTODLL GiveFnptrsToDll(enginefuncs_t* functionTable, globalvars_t* pGlobals)
{
	// this is the very first function that is called in the game DLL by the engine. Its purpose
	// is to set the functions interfacing up, by exchanging the functionTable function list
	// along with a pointer to the engine's global variables structure pGlobals, with the game
	// DLL. We can there decide if we want to load the normal game DLL just behind our bot DLL,
	// or any other game DLL that is present, such as Will Day's metamod. Also, since there is
	// a known bug on Win32 platforms that prevent hook DLLs (such as our bot DLL) to be used in
	// single player games (because they don't export all the stuff they should), we may need to
	// build our own array of exported symbols from the actual game DLL in order to use it as
	// such if necessary. Nothing really bot-related is done in this function. The actual bot
	// initialization stuff will be done later, when we'll be certain to have a multilayer game.

	struct ModSupport
	{
		char name[32]{};
		char linuxLib[32]{};
		char winLib[32]{};
		char desc[256]{};
		Game modType{};
	} s_supportedMods[] =
	{
		{ "cstrike", "cs_i386.so", "mp.dll", "Counter-Strike v1.6", Game::CStrike },
		{ "cstrike", "cs.so", "mp.dll", "Counter-Strike v1.6 (Newer)", Game::CStrike },
		{ "czero", "cs_i386.so", "mp.dll", "Counter-Strike: Condition Zero", Game::CZero },
		{ "czero", "cs.so", "mp.dll", "Counter-Strike: Condition Zero (Newer)", Game::CZero },
		{ "valve", "hl.so", "hl.dll", "Half-Life", Game::HalfLife },
		{ "gearbox", "opfor.so", "opfor.dll", "Half-Life: Opposing Force", Game::HalfLife },
		{ "dmc", "dmc.so", "dmc.dll", "Deathmatch Classic", Game::DMC },
		{ "", "", "", "", Game::HalfLife }
	};

	// get the engine functions from the engine...
	cmemcpy(&g_engfuncs, functionTable, sizeof(enginefuncs_t));
	g_pGlobals = pGlobals;

	ModSupport* knownMod = nullptr;
	char gameDLLName[256];

	// TODO: FIX HERE
	int i;
	for (i = 0; s_supportedMods[i].name; i++)
	{
		ModSupport* mod = &s_supportedMods[i];
		if (cstrcmp(mod->name, GetModName()) == 0)
		{
			knownMod = mod;
			break;
		}
	}

	if (knownMod)
		g_gameVersion = knownMod->modType;
	else
	{
		g_gameVersion = Game::CStrike;
		AddLogEntry(Log::Fatal, "Mod that you has started, not supported by this bot (gamedir: %s)", GetModName());
	}

	if (g_engfuncs.pfnCVarGetPointer("host_ver"))
		g_gameVersion |= Game::Xash;
}

DLL_ENTRYPOINT
{
	// dynamic library entry point, can be used for uninitialization stuff. NOT for initializing
	// anything because if you ever attempt to wander outside the scope of this function on a
	// DLL attach, LoadLibrary() will simply fail. And you can't do I/Os here either. Nice eh ?

	// dynamic library detaching ??
	if (DLL_DETACHING)
	   FreeLibraryMemory(); // free everything that's freeable

	DLL_RETENTRY; // the return data type is OS specific too
}