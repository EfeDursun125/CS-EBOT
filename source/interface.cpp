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
ConVar ebot_password("ebot_password", "", VARTYPE_PASSWORD);
ConVar ebot_password_key("ebot_password_key", "ebot_wp");

ConVar ebot_language("ebot_language", "en");
ConVar ebot_version("ebot_version", PRODUCT_VERSION, VARTYPE_READONLY);

ConVar ebot_lockzbot("ebot_lock_zbot", "1");
ConVar ebot_showwp("ebot_show_waypoints", "0");

ConVar ebot_analyze_create_goal_waypoints("ebot_analyze_starter_waypoints", "1");

void ebotVersionMSG(edict_t* entity = null)
{
	int buildVersion[4] = { PRODUCT_VERSION_DWORD };
	uint16 bV16[4] = { (uint16)buildVersion[0], (uint16)buildVersion[1], (uint16)buildVersion[2], (uint16)buildVersion[3] };

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
	if (stricmp(arg0, "addbot") == 0 || stricmp(arg0, "add") == 0 ||
		stricmp(arg0, "addbot_hs") == 0 || stricmp(arg0, "addhs") == 0 ||
		stricmp(arg0, "addbot_t") == 0 || stricmp(arg0, "add_t") == 0 ||
		stricmp(arg0, "addbot_ct") == 0 || stricmp(arg0, "add_ct") == 0)
		ServerPrint("Pls use the command to change it");

	// kicking off one bot from the terrorist team
	else if (stricmp(arg0, "kickbot_t") == 0 || stricmp(arg0, "kick_t") == 0)
		g_botManager->RemoveFromTeam(TEAM_TERRORIST);

	// kicking off one bot from the counter-terrorist team
	else if (stricmp(arg0, "kickbot_ct") == 0 || stricmp(arg0, "kick_ct") == 0)
		g_botManager->RemoveFromTeam(TEAM_COUNTER);

	// kills all bots on the terrorist team
	else if (stricmp(arg0, "killbots_t") == 0 || stricmp(arg0, "kill_t") == 0)
		g_botManager->KillAll(TEAM_TERRORIST);

	// kills all bots on the counter-terrorist team
	else if (stricmp(arg0, "killbots_ct") == 0 || stricmp(arg0, "kill_ct") == 0)
		g_botManager->KillAll(TEAM_COUNTER);

	// list all bots playeing on the server
	else if (stricmp(arg0, "listbots") == 0 || stricmp(arg0, "list") == 0)
		g_botManager->ListBots();

	// kick off all bots from the played server
	else if (stricmp(arg0, "kickbots") == 0 || stricmp(arg0, "kickall") == 0)
		g_botManager->RemoveAll();

	// kill all bots on the played server
	else if (stricmp(arg0, "killbots") == 0 || stricmp(arg0, "killall") == 0)
		g_botManager->KillAll();

	// kick off one random bot from the played server
	else if (stricmp(arg0, "kickone") == 0 || stricmp(arg0, "kick") == 0)
		g_botManager->RemoveRandom();

	// fill played server with bots
	else if (stricmp(arg0, "fillserver") == 0 || stricmp(arg0, "fill") == 0)
		g_botManager->FillServer(atoi(arg1), IsNullString(arg2) ? -1 : atoi(arg2), IsNullString(arg3) ? -1 : atoi(arg3), IsNullString(arg4) ? -1 : atoi(arg4));

	// swap counter-terrorist and terrorist teams
	else if (stricmp(arg0, "swaptteams") == 0 || stricmp(arg0, "swap") == 0)
	{
		for (int i = 0; i < engine->GetMaxClients(); i++)
		{
			if (!(g_clients[i].flags & CFLAG_USED))
				continue;

			if (IsValidBot(g_clients[i].ent))
				FakeClientCommand(g_clients[i].ent, "chooseteam; menuselect %d; menuselect 5", GetTeam(g_clients[i].ent) == TEAM_COUNTER ? 1 : 2);
			else
				(*g_engfuncs.pfnClientCommand) (g_clients[i].ent, "chooseteam; menuselect %d", GetTeam(g_clients[i].ent) == TEAM_COUNTER ? 1 : 2);
		}
	}

	// select the weapon mode for bots
	else if (stricmp(arg0, "weaponmode") == 0 || stricmp(arg0, "wmode") == 0)
	{
		int selection = atoi(arg1);

		// check is selected range valid
		if (selection >= 1 && selection <= 7)
			g_botManager->SetWeaponMode(selection);
		else
			ClientPrint(ent, print_withtag, "Choose weapon from 1 to 7 range");
	}

	// force all bots to vote to specified map
	else if (stricmp(arg0, "votemap") == 0)
	{
		if (!IsNullString(arg1))
		{
			int nominatedMap = atoi(arg1);

			// loop through all players
			for (int i = 0; i < engine->GetMaxClients(); i++)
			{
				if (g_botManager->GetBot(i) != null)
					g_botManager->GetBot(i)->m_voteMap = nominatedMap;
			}

			ClientPrint(ent, print_withtag, "All dead bots will vote for map #%d", nominatedMap);
		}
	}

	// force bots to execute client command
	// force bots to execute client command
	else if (stricmp(arg0, "sendcmd") == 0 || stricmp(arg0, "order") == 0)
	{
		if (IsNullString(arg1))
			return 1;

		edict_t* cmd = INDEXENT(atoi(arg1) - 1);

		if (IsValidBot(cmd))
		{
			FakeClientCommand(cmd, arg2);
			ClientPrint(cmd, print_withtag, "Bot %s executing command %s", GetEntityName(ent), &arg2[0]);
		}
		else
			ClientPrint(cmd, print_withtag, "Player is NOT Bot!");
	}

	// display current time on the server
	else if (stricmp(arg0, "ctime") == 0 || stricmp(arg0, "time") == 0)
	{
		time_t tickTime = time(null);
		tm* localTime = localtime(&tickTime);

		char date[32];
		strftime(date, 31, "--- Current Time: %H:%M:%S ---", localTime);

		ClientPrint(ent, print_center, date);
	}

	// displays bot about information
	else if (stricmp(arg0, "about_bot") == 0 || stricmp(arg0, "about") == 0)
	{
		if (g_gameVersion == CSVER_VERYOLD)
		{
			ServerPrint("Cannot do this on CS 1.5");
			return 1;
		}

		char aboutData[] =
			"+---------------------------------------------------------------------------------+\n"
			" The E-BOT for Counter-Strike 1.6 " PRODUCT_SUPPORT_VERSION "\n"
			" Made by " PRODUCT_AUTHOR ", Using SyPB & YaPB Code\n"
			" Website: " PRODUCT_URL "\n"
			"+---------------------------------------------------------------------------------+\n";

		HudMessage(ent, true, Color(engine->RandomInt(33, 255), engine->RandomInt(33, 255), engine->RandomInt(33, 255)), aboutData);
	}

	// displays version information
	else if (stricmp(arg0, "version") == 0 || stricmp(arg0, "ver") == 0)
		ebotVersionMSG(ent);

	// display some sort of help information
	else if (stricmp(arg0, "?") == 0 || stricmp(arg0, "help") == 0)
	{
		ClientPrint(ent, print_console, "Bot Commands:");
		ClientPrint(ent, print_console, "ebot version            - display version information.");
		ClientPrint(ent, print_console, "ebot about              - show bot about information.");
		//ClientPrint (ent, print_console, "ebot add                - create a bot in current game.");
		ClientPrint(ent, print_console, "ebot fill               - fill the server with random bots.");
		ClientPrint(ent, print_console, "ebot kickall            - disconnects all bots from current game.");
		ClientPrint(ent, print_console, "ebot killbots           - kills all bots in current game.");
		ClientPrint(ent, print_console, "ebot kick               - disconnect one random bot from game.");
		ClientPrint(ent, print_console, "ebot weaponmode         - select bot weapon mode.");
		ClientPrint(ent, print_console, "ebot votemap            - allows dead bots to vote for specific map.");
		ClientPrint(ent, print_console, "ebot cmenu              - displaying bots command menu.");


		ClientPrint(ent, print_console, "ebot_add                - create a bot in current game.");

		// SyPB Pro P.23 - SgdWP
		if (!IsDedicatedServer())
			ServerPrintNoTag("ebot sgdwp on           - New making waypoint system from ebot");

		if (stricmp(arg1, "full") == 0 || stricmp(arg1, "f") == 0 || stricmp(arg1, "?") == 0)
		{
			ClientPrint(ent, print_console, "ebot add_t              - creates one random bot to terrorist team.");
			ClientPrint(ent, print_console, "ebot add_ct             - creates one random bot to ct team.");
			ClientPrint(ent, print_console, "ebot kick_t             - disconnect one random bot from terrorist team.");
			ClientPrint(ent, print_console, "ebot kick_ct            - disconnect one random bot from ct team.");
			ClientPrint(ent, print_console, "ebot kill_t             - kills all bots on terrorist team.");
			ClientPrint(ent, print_console, "ebot kill_ct            - kills all bots on ct team.");
			ClientPrint(ent, print_console, "ebot list               - display list of bots currently playing.");
			ClientPrint(ent, print_console, "ebot order              - execute specific command on specified bot.");
			ClientPrint(ent, print_console, "ebot time               - displays current time on server.");
			ClientPrint(ent, print_console, "ebot deletewp           - erase waypoint file from hard disk (permanently).");

			if (!IsDedicatedServer())
			{
				ServerPrintNoTag("ebot autowp            - toggle autowppointing.");
				ServerPrintNoTag("ebot wp                - toggle waypoint showing.");
				ServerPrintNoTag("ebot wp on noclip      - enable noclip cheat");
				ServerPrintNoTag("ebot wp save nocheck   - save waypoints without checking.");
				ServerPrintNoTag("ebot wp add            - open menu for waypoint creation.");
				ServerPrintNoTag("ebot wp menu           - open main waypoint menu.");
				ServerPrintNoTag("ebot wp addbasic       - creates basic waypoints on map.");
				ServerPrintNoTag("ebot wp find           - show direction to specified waypoint.");
				ServerPrintNoTag("ebot wp load           - wload the waypoint file from hard disk.");
				ServerPrintNoTag("ebot wp check          - checks if all waypoints connections are valid.");
				ServerPrintNoTag("ebot wp cache          - cache nearest waypoint.");
				ServerPrintNoTag("ebot wp teleport       - teleport hostile to specified waypoint.");
				ServerPrintNoTag("ebot wp setradius      - manually sets the wayzone radius for this waypoint.");
				ServerPrintNoTag("ebot path autodistance - opens menu for setting autopath maximum distance.");
				ServerPrintNoTag("ebot path cache        - remember the nearest to player waypoint.");
				ServerPrintNoTag("ebot path create       - opens menu for path creation.");
				ServerPrintNoTag("ebot path delete       - delete path from cached to nearest waypoint.");
				ServerPrintNoTag("ebot path create_in    - creating incoming path connection.");
				ServerPrintNoTag("ebot path create_out   - creating outgoing path connection.");
				ServerPrintNoTag("ebot path create_both  - creating both-ways path connection.");
				ServerPrintNoTag("ebot exp save          - save the experience data.");
			}
		}
	}

	// sets health for all available bots
	else if (stricmp(arg0, "sethealth") == 0 || stricmp(arg0, "health") == 0)
	{
		if (IsNullString(arg1))
			ClientPrint(ent, print_withtag, "Please specify health");
		else
		{
			ClientPrint(ent, print_withtag, "Bot health is set to %d%%", atoi(arg1));

			for (int i = 0; i < engine->GetMaxClients(); i++)
			{
				if (g_botManager->GetBot(i) != null)
					g_botManager->GetBot(i)->pev->health = fabsf(static_cast <float> (atof(arg1)));
			}
		}
	}

	// displays main bot menu
	else if (stricmp(arg0, "botmenu") == 0 || stricmp(arg0, "menu") == 0)
		DisplayMenuToClient(ent, &g_menus[0]);

	// display command menu
	else if (stricmp(arg0, "cmdmenu") == 0 || stricmp(arg0, "cmenu") == 0)
	{
		if (IsAlive(ent))
			DisplayMenuToClient(ent, &g_menus[18]);
		else
		{
			DisplayMenuToClient(ent, null); // reset menu display
			CenterPrint("You're dead, and have no access to this menu");
		}
	}
	// some debug routines
	else if (stricmp(arg0, "debug") == 0)
	{
		// test random number generator
		if (stricmp(arg0, "randgen") == 0)
		{
			for (int i = 0; i < 500; i++)
				ServerPrintNoTag("Result Range[0 - 100]: %d", engine->RandomInt(0, 100));
		}
	}

	// waypoint manimupulation (really obsolete, can be edited through menu) (supported only on listen server)
	else if (stricmp(arg0, "waypoint") == 0 || stricmp(arg0, "wp") == 0 || stricmp(arg0, "wpt") == 0)
	{
		if (IsDedicatedServer() || FNullEnt(g_hostEntity))
			return 2;

		// enables or disable waypoint displaying
		if (stricmp(arg1, "analyze") == 0)
		{
			g_analyzewaypoints = true;
			ServerPrint("Waypoint Analyzing On (Please Manually Edit Waypoints For Best Results)");
			ServerCommand("ebot wp on");
			if (ebot_analyze_create_goal_waypoints.GetInt() == 1)
				g_waypoint->CreateBasic();
		}

		else if (stricmp(arg1, "analyzeoff") == 0)
		{
			g_waypoint->AnalyzeDeleteUselessWaypoints();
			g_analyzewaypoints = false;
			ServerPrint("Waypoint Analyzing Off");
			g_waypoint->Save();
			ServerCommand("ebot wp off");
			g_analyzeputrequirescrouch = false;
		}

		else if (stricmp(arg1, "analyze off") == 0)
		{
			g_waypoint->AnalyzeDeleteUselessWaypoints();
			g_analyzewaypoints = false;
			ServerPrint("Waypoint Analyzing Off");
			g_waypoint->Save();
			ServerCommand("ebot wp off");
			g_analyzeputrequirescrouch = false;
		}

		else if (stricmp(arg1, "analyzefix") == 0)
		{
			g_waypoint->AnalyzeDeleteUselessWaypoints();
			ServerCommand("ebot wp on");
		}

		else if (stricmp(arg1, "analyze fix") == 0)
		{
			g_waypoint->AnalyzeDeleteUselessWaypoints();
			ServerCommand("ebot wp on");
		}

		else if (stricmp(arg1, "on") == 0)
		{
			g_waypointOn = true;
			ServerPrint("Waypoint Editing Enabled");

			// enables noclip cheat
			if (stricmp(arg2, "noclip") == 0)
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

			ServerCommand("ebot wp mdl on");
		}

		// switching waypoint editing off
		else if (stricmp(arg1, "off") == 0)
		{
			g_waypointOn = false;
			g_editNoclip = false;
			g_hostEntity->v.movetype = MOVETYPE_WALK;

			ServerPrint("Waypoint Editing Disabled");
			ServerCommand("ebot wp mdl off");
		}

		// toggles displaying player models on spawn spots
		else if (stricmp(arg1, "mdl") == 0 || stricmp(arg1, "models") == 0)
		{
			edict_t* spawnEntity = null;

			if (stricmp(arg2, "on") == 0)
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
			else if (stricmp(arg2, "off") == 0)
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
		else if (stricmp(arg1, "find") == 0)
			g_waypoint->SetFindIndex(atoi(arg2));

		// opens adding waypoint menu
		else if (stricmp(arg1, "add") == 0)
		{
			g_waypointOn = true;  // turn waypoints on
			DisplayMenuToClient(g_hostEntity, &g_menus[12]);
		}

		// creates basic waypoints on the map (ladder/spawn points/goals)
		else if (stricmp(arg1, "addbasic") == 0)
		{
			g_waypoint->CreateBasic();
			CenterPrint("Basic waypoints was Created");
		}

		// delete nearest to host edict waypoint
		else if (stricmp(arg1, "delete") == 0)
		{
			g_waypointOn = true; // turn waypoints on
			g_waypoint->Delete();
		}

		// save waypoint data into file on hard disk
		else if (stricmp(arg1, "save") == 0)
		{
			char* waypointSaveMessage = g_localizer->TranslateInput("Waypoints Saved");

			if (FStrEq(arg2, "nocheck"))
			{
				g_waypoint->Save();
				ServerPrint(waypointSaveMessage);
				CenterPrint("Waypoints are saved!");
			}
			else if (g_waypoint->NodesValid())
			{
				g_waypoint->Save();
				ServerPrint(waypointSaveMessage);
			}
		}

		// remove waypoint and all corresponding files from hard disk
		else if (stricmp(arg1, "erase") == 0)
			g_waypoint->EraseFromHardDisk();

		// load all waypoints again (overrides all changes, that wasn't saved)
		else if (stricmp(arg1, "load") == 0)
		{
			if (g_waypoint->Load())
				ServerPrint("Waypoints loaded");
		}

		// check all nodes for validation
		else if (stricmp(arg1, "check") == 0)
		{
			if (g_waypoint->NodesValid())
				CenterPrint("Nodes work Fine");
		}

		// opens menu for setting (removing) waypoint flags
		else if (stricmp(arg1, "flags") == 0)
			DisplayMenuToClient(g_hostEntity, &g_menus[13]);

		// setting waypoint radius
		else if (stricmp(arg1, "setradius") == 0)
			g_waypoint->SetRadius(atoi(arg2));

		// remembers nearest waypoint
		else if (stricmp(arg1, "cache") == 0)
			g_waypoint->CacheWaypoint();

		// teleport player to specified waypoint
		else if (stricmp(arg1, "teleport") == 0)
		{
			int teleportPoint = atoi(arg2);

			if (teleportPoint < g_numWaypoints)
			{
				(*g_engfuncs.pfnSetOrigin) (g_hostEntity, g_waypoint->GetPath(teleportPoint)->origin);
				g_waypointOn = true;

				ServerPrint("Player '%s' teleported to waypoint #%d (x:%.1f, y:%.1f, z:%.1f)", GetEntityName(g_hostEntity), teleportPoint, g_waypoint->GetPath(teleportPoint)->origin.x, g_waypoint->GetPath(teleportPoint)->origin.y, g_waypoint->GetPath(teleportPoint)->origin.z);
				g_editNoclip = true;
			}
		}

		// displays waypoint menu
		else if (stricmp(arg1, "menu") == 0)
			DisplayMenuToClient(g_hostEntity, &g_menus[9]);

		// otherwise display waypoint current status
		else
			ServerPrint("Waypoints are %s", g_waypointOn == true ? "Enabled" : "Disabled");
	}

	// path waypoint editing system (supported only on listen server)
	else if (stricmp(arg0, "pathwaypoint") == 0 || stricmp(arg0, "path") == 0 || stricmp(arg0, "pwp") == 0)
	{
		if (IsDedicatedServer() || FNullEnt(g_hostEntity))
			return 2;

		// opens path creation menu
		if (stricmp(arg1, "create") == 0)
			DisplayMenuToClient(g_hostEntity, &g_menus[20]);

		// creates incoming path from the cached waypoint
		else if (stricmp(arg1, "create_in") == 0)
			g_waypoint->CreatePath(PATHCON_INCOMING);

		// creates outgoing path from current waypoint
		else if (stricmp(arg1, "create_out") == 0)
			g_waypoint->CreatePath(PATHCON_OUTGOING);

		// creates bidirectional path from cahed to current waypoint
		else if (stricmp(arg1, "create_both") == 0)
			g_waypoint->CreatePath(PATHCON_BOTHWAYS);

		// creates jump path from cahed to current waypoint
		else if (stricmp(arg1, "create_jump") == 0)
			g_waypoint->CreatePath(PATHCON_JUMPING);

		// creates boosting path from cahed to current waypoint
		else if (stricmp(arg1, "create_boost") == 0)
			g_waypoint->CreatePath(PATHCON_BOOSTING);

		// creates visible only path from cahed to current waypoint
		else if (stricmp(arg1, "create_visible") == 0)
			g_waypoint->CreatePath(PATHCON_VISIBLE);

		// delete special path
		else if (stricmp(arg1, "delete") == 0)
			g_waypoint->DeletePath();

		// sets auto path maximum distance
		else if (stricmp(arg1, "autodistance") == 0)
			DisplayMenuToClient(g_hostEntity, &g_menus[19]);
	}

	// automatic waypoint handling (supported only on listen server)
	else if (stricmp(arg0, "autowaypoint") == 0 || stricmp(arg0, "autowp") == 0)
	{
		if (IsDedicatedServer() || FNullEnt(g_hostEntity))
			return 2;

		// enable autowaypointing
		if (stricmp(arg1, "on") == 0)
		{
			g_autoWaypoint = true;
			g_waypointOn = true; // turn this on just in case
		}

		// disable autowaypointing
		else if (stricmp(arg1, "off") == 0)
			g_autoWaypoint = false;

		// display status
		ServerPrint("Auto-Waypoint %s", g_autoWaypoint ? "Enabled" : "Disabled");
	}
	
	else if (stricmp(arg0, "sgdwaypoint") == 0 || stricmp(arg0, "sgdwp") == 0)
	{
		if (IsDedicatedServer() || FNullEnt(g_hostEntity))
			return 2;
		g_waypoint->SgdWp_Set(arg1);
	}

	// experience system handling (supported only on listen server)
	else if (stricmp(arg0, "experience") == 0 || stricmp(arg0, "exp") == 0)
	{
		if (IsDedicatedServer() || FNullEnt(g_hostEntity))
			return 2;

		// write experience table (and visibility table) to hard disk
		if (stricmp(arg1, "save") == 0)
		{
			g_exp.Unload();

			ServerPrint("Experience tab saved");
		}
	}
	else
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
	if (BotCommandHandler(g_hostEntity, IsNullString(CMD_ARGV(1)) ? "help" : CMD_ARGV(1), CMD_ARGV(2), CMD_ARGV(3), CMD_ARGV(4), CMD_ARGV(5), CMD_ARGV(6)) == 0)
		ServerPrint("Unknown command: %s", CMD_ARGV(1));
}

// SyPB Pro P.35 - new command
void ebot_Version_Command(void)
{
	ebotVersionMSG();
}

// SyPB Pro P.46 - Entity Action Check Cvar
void CheckEntityAction(void)
{
	int i;
	int workEntityWork = 0;
	edict_t* entity = null;

	for (i = 0; i < entityNum; i++)
	{
		if (g_entityId[i] == -1)
			continue;

		entity = INDEXENT(g_entityId[i]);
		if (FNullEnt(entity) || entity->v.effects & EF_NODRAW)
			continue;

		// SyPB Pro P.47 - Entity Action Check Cvar improve
		char action[33], team[33];
		if (g_entityAction[i] == 1)
			sprintf(action, "Enemy");
		else if (g_entityAction[i] == 2)
			sprintf(action, "Need Avoid");
		else if (g_entityAction[i] == 3)
			sprintf(action, "Pick Up");
		sprintf(team,
			(g_entityTeam[i] == TEAM_COUNTER) ? "CT" : (g_entityTeam[i] == TEAM_TERRORIST) ? "TR" : "Team-%d",
			g_entityTeam[i]);

		workEntityWork++;
		ServerPrintNoTag("Entity Num: %d | Action: %d (%s) | Team: %d (%s) | Entity Name: %s",
			workEntityWork,
			g_entityAction[i], action,
			g_entityTeam[i], team,
			GetEntityName(entity));
	}

	ServerPrintNoTag("Total Entity Action Num: %d", workEntityWork);
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

	// fixes for crashing if configs couldn't be accessed
	g_chatFactory.SetSize(CHAT_NUM);

#define SKIP_COMMENTS() if ((line[0] == '/') || (line[0] == '\r') || (line[0] == '\n') || (line[0] == 0) || (line[0] == ' ') || (line[0] == '\t')) continue;

	// SyPB Pro P.42 - Bot Name fixed
	if (!g_botNames.IsEmpty())
	{
		ITERATE_ARRAY(g_botNames, j)
			g_botNames[j].isUsed = false;
	}

	// NAMING SYSTEM INITIALIZATION
	if (OpenConfig("names.cfg", "Name configuration file not found.", &fp, true) && g_botNames.IsEmpty()) // SyPB Pro P.20 - Bot Name
	{
		while (fp.GetBuffer(line, 255))
		{
			SKIP_COMMENTS();

			strtrim(line);
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
	if (OpenConfig("chat.cfg", "Chat file not found.", &fp, true))
	{
		while (fp.GetBuffer(line, 255))
		{
			SKIP_COMMENTS();
			strcpy(command, GetField(line, 0, 1));

			if (strcmp(command, "[KILLED]") == 0)
			{
				chatType = 0;
				continue;
			}
			else if (strcmp(command, "[BOMBPLANT]") == 0)
			{
				chatType = 1;
				continue;
			}
			else if (strcmp(command, "[DEADCHAT]") == 0)
			{
				chatType = 2;
				continue;
			}
			else if (strcmp(command, "[REPLIES]") == 0)
			{
				chatType = 3;
				continue;
			}
			else if (strcmp(command, "[UNKNOWN]") == 0)
			{
				chatType = 4;
				continue;
			}
			else if (strcmp(command, "[TEAMATTACK]") == 0)
			{
				chatType = 5;
				continue;
			}
			else if (strcmp(command, "[WELCOME]") == 0)
			{
				chatType = 6;
				continue;
			}
			else if (strcmp(command, "[TEAMKILL]") == 0)
			{
				chatType = 7;
				continue;
			}

			if (chatType != 3)
				line[79] = 0;

			strtrim(line);

			switch (chatType)
			{
			case 0:
				g_chatFactory[CHAT_KILL].Push(line);
				break;

			case 1:
				g_chatFactory[CHAT_PLANTBOMB].Push(line);
				break;

			case 2:
				g_chatFactory[CHAT_DEAD].Push(line);
				break;

			case 3:
				if (strstr(line, "@KEY") != null)
				{
					if (!replyKey.keywords.IsEmpty() && !replyKey.replies.IsEmpty())
					{
						g_replyFactory.Push(replyKey);
						replyKey.replies.RemoveAll();
					}

					replyKey.keywords.RemoveAll();
					replyKey.keywords = String(&line[4]).Split(",");

					ITERATE_ARRAY(replyKey.keywords, i)
						replyKey.keywords[i].Trim().TrimQuotes();
				}
				else if (!replyKey.keywords.IsEmpty())
					replyKey.replies.Push(line);

				break;

			case 4:
				g_chatFactory[CHAT_NOKW].Push(line);
				break;

			case 5:
				g_chatFactory[CHAT_TEAMATTACK].Push(line);
				break;

			case 6:
				g_chatFactory[CHAT_HELLO].Push(line);
				break;

			case 7:
				g_chatFactory[CHAT_TEAMKILL].Push(line);
				break;
			}
		}
		fp.Close();
	}
	else
	{
		extern ConVar ebot_chat;
		ebot_chat.SetInt(0);
	}

	// GENERAL DATA INITIALIZATION
	if (OpenConfig("general.cfg", "General configuration file not found. Loading defaults", &fp))
	{
		while (fp.GetBuffer(line, 255))
		{
			SKIP_COMMENTS();

			Array <String> pair = String(line).Split("=");

			if (pair.GetElementNumber() != 2)
				continue;

			pair[0].Trim().Trim();
			pair[1].Trim().Trim();

			Array <String> splitted = pair[1].Split(",");

			if (pair[0] == "MapStandard")
			{
				if (splitted.GetElementNumber() != Const_NumWeapons)
					AddLogEntry(LOG_FATAL, "%s entry in general config is not valid.", &pair[0][0]);

				for (int i = 0; i < Const_NumWeapons; i++)
					g_weaponSelect[i].teamStandard = splitted[i];
			}
			else if (pair[0] == "MapAS")
			{
				if (splitted.GetElementNumber() != Const_NumWeapons)
					AddLogEntry(LOG_FATAL, "%s entry in general config is not valid.", &pair[0][0]);

				for (int i = 0; i < Const_NumWeapons; i++)
					g_weaponSelect[i].teamAS = splitted[i];
			}
			else if (pair[0] == "GrenadePercent")
			{
				if (splitted.GetElementNumber() != 3)
					AddLogEntry(LOG_FATAL, "%s entry in general config is not valid.", &pair[0][0]);

				for (int i = 0; i < 3; i++)
					g_grenadeBuyPrecent[i] = splitted[i];
			}
			else if (pair[0] == "GrenadeMoney")
			{
				if (splitted.GetElementNumber() != 3)
					AddLogEntry(LOG_FATAL, "%s entry in general config is not valid.", &pair[0][0]);

				for (int i = 0; i < 3; i++)
					g_grenadeBuyMoney[i] = splitted[i];
			}
			else if (pair[0] == "PersonalityNormal")
			{
				if (splitted.GetElementNumber() != Const_NumWeapons)
					AddLogEntry(LOG_FATAL, "%s entry in general config is not valid.", &pair[0][0]);

				for (int i = 0; i < Const_NumWeapons; i++)
					g_normalWeaponPrefs[i] = splitted[i];
			}
			else if (pair[0] == "PersonalityRusher")
			{
				if (splitted.GetElementNumber() != Const_NumWeapons)
					AddLogEntry(LOG_FATAL, "%s entry in general config is not valid.", &pair[0][0]);

				for (int i = 0; i < Const_NumWeapons; i++)
					g_rusherWeaponPrefs[i] = splitted[i];
			}
			else if (pair[0] == "PersonalityCareful")
			{
				if (splitted.GetElementNumber() != Const_NumWeapons)
					AddLogEntry(LOG_FATAL, "%s entry in general config is not valid.", &pair[0][0]);

				for (int i = 0; i < Const_NumWeapons; i++)
					g_carefulWeaponPrefs[i] = splitted[i];
			}
			else if (pair[0].Has("Skill"))
			{
				if (splitted.GetElementNumber() != 8)
					AddLogEntry(LOG_FATAL, "%s entry in general config is not valid.", &pair[0][0]);

				int parserState = 0;

				if (pair[0].Has("Easy"))
					parserState = 0;
				else if (pair[0].Has("Fair"))
					parserState = 1;
				else if (pair[0].Has("Normal"))
					parserState = 2;
				else if (pair[0].Has("Hard"))
					parserState = 3;
				else if (pair[0].Has("Expert"))
					parserState = 4;
				else if (pair[0].Has("Elite"))
					parserState = 5;

				for (int i = 0; i < 8; i++)
				{
					switch (i)
					{
					case 0:
						g_skillTab[parserState].minSurpriseTime = splitted[i];
						break;

					case 1:
						g_skillTab[parserState].maxSurpriseTime = splitted[i];
						break;

					case 2:
						g_skillTab[parserState].minTurnSpeed = splitted[i];
						break;

					case 3:
						g_skillTab[parserState].maxTurnSpeed = splitted[i];
						break;

					case 4:
						g_skillTab[parserState].headshotFrequency = splitted[i];
						break;

					case 5:
						g_skillTab[parserState].heardShootThruProb = splitted[i];
						break;

					case 6:
						g_skillTab[parserState].seenShootThruProb = splitted[i];
						break;

					case 7:
						g_skillTab[parserState].recoilAmount = splitted[i];
						break;
					}
				}
			}
		}

		fp.Close();
	}

	extern ConVar ebot_communication_type;

	// LOCALIZER INITITALIZATION
	if (OpenConfig("lang.cfg", "Specified language not found", &fp, true) && g_gameVersion != CSVER_VERYOLD)
	{
		if (IsDedicatedServer())
			return; // dedicated server will use only english translation

		enum Lang_t { Lang_Original, Lang_Translate, Lang_Default } langState = Lang_Default;

		char buffer[1024];
		LanguageItem temp = { "", "" };

		while (fp.GetBuffer(line, 255))
		{
			if (strncmp(line, "[ORIGINAL]", 10) == 0)
			{
				langState = Lang_Original;

				if (!IsNullString(buffer))
				{
					strtrim(buffer);
					temp.translated = buffer;
					buffer[0] = 0x0;
				}

				if (!IsNullString(temp.translated) && !IsNullString(temp.original))
					g_localizer->m_langTab.Push(temp);
			}
			else if (strncmp(line, "[TRANSLATED]", 12) == 0)
			{
				strtrim(buffer);
				temp.original = buffer;
				buffer[0] = 0x0;

				langState = Lang_Translate;
			}
			else
			{
				switch (langState)
				{
				case Lang_Original:
					strncat(buffer, line, 1024 - strlen(buffer));
					break;

				case Lang_Translate:
					strncat(buffer, line, 1024 - strlen(buffer));
					break;
				}
			}
		}
		fp.Close();
	}
	else if (g_gameVersion == CSVER_VERYOLD)
		AddLogEntry(LOG_DEFAULT, "Multilingual system disabled, due to your Counter-Strike Version!");

	// set personality weapon pointers here
	g_weaponPrefs[PERSONALITY_NORMAL] = reinterpret_cast <int*> (&g_normalWeaponPrefs);
	g_weaponPrefs[PERSONALITY_RUSHER] = reinterpret_cast <int*> (&g_rusherWeaponPrefs);
	g_weaponPrefs[PERSONALITY_CAREFUL] = reinterpret_cast <int*> (&g_carefulWeaponPrefs);
}

void CommandHandler_NotMM(void)
{
	// this function is the dedicated server command handler for the new ebot server command we
	// registered at game start. It will be called by the engine each time a server command that
	// starts with "meta" is entered in the server console. It works exactly the same way as
	// ClientCommand() does, using the CmdArgc() and CmdArgv() facilities of the engine. Argv(0)
	// is the server command itself (here "meta") and the next ones are its arguments. Just like
	// the stdio command-line parsing in C when you write "long main (long argc, char **argv)".
	// this function is handler for non-metamod launch of ebot, it's just print error message.

	ServerPrint("You're launched standalone version of ebot. Metamod is not installed or not enabled!");
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
	RegisterCommand("ebot_add_ct", AddBot_CT);
	RegisterCommand("ebot_add", AddBot);

	RegisterCommand("ebot_entity_check", CheckEntityAction);

	// register server command(s)
	RegisterCommand("ebot", CommandHandler);

	// execute main config
	ServerCommand("exec addons/ebot/ebot.cfg");

	// register fake metamod command handler if we not! under mm
	if (!g_isMetamod)
		RegisterCommand("meta", CommandHandler_NotMM);

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	(*g_functionTable.pfnGameInit) ();

	DetectCSVersion(); // determine version of currently running cs
}

extern ConVar ebot_force_flashlight;

int Spawn(edict_t* ent)
{
	// this function asks the game DLL to spawn (i.e, give a physical existence in the virtual
	// world, in other words to 'display') the entity pointed to by ent in the game. The
	// Spawn() function is one of the functions any entity is supposed to have in the game DLL,
	// and any MOD is supposed to implement one for each of its entities.

	// zp & biohazard flashlight support
	if (ebot_force_flashlight.GetInt() == 1 && !(ent->v.effects & EF_DIMLIGHT))
		ent->v.impulse = 100;

	const char* entityClassname = STRING(ent->v.classname);

	if (strcmp(entityClassname, "worldspawn") == 0)
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

		g_mapType = null; // reset map type as worldspawn is the first entity spawned
		g_worldEdict = ent; // save the world entity for future use
	}
	// SyPB Pro P.34 - Base Change
	else if (strcmp(entityClassname, "player_weaponstrip") == 0)
	{
		if (g_gameVersion == CSVER_VERYOLD && (STRING(ent->v.target))[0] == '\0')
		{
			ent->v.target = MAKE_STRING("fake");
			ent->v.targetname = MAKE_STRING("fake");
		}
		else
		{
			REMOVE_ENTITY(ent);

			if (g_isMetamod)
				RETURN_META_VALUE(MRES_SUPERCEDE, 0);

			return (*g_functionTable.pfnSpawn) (ent);
		}
	}
	else if (strcmp(entityClassname, "info_player_start") == 0)
	{
		SET_MODEL(ent, "models/player/urban/urban.mdl");

		ent->v.rendermode = kRenderTransAlpha; // set its render mode to transparency
		ent->v.renderamt = 127; // set its transparency amount
		ent->v.effects |= EF_NODRAW;
	}
	else if (strcmp(entityClassname, "info_player_deathmatch") == 0)
	{
		SET_MODEL(ent, "models/player/terror/terror.mdl");

		ent->v.rendermode = kRenderTransAlpha; // set its render mode to transparency
		ent->v.renderamt = 127; // set its transparency amount
		ent->v.effects |= EF_NODRAW;
	}

	else if (strcmp(entityClassname, "info_vip_start") == 0)
	{
		SET_MODEL(ent, "models/player/vip/vip.mdl");

		ent->v.rendermode = kRenderTransAlpha; // set its render mode to transparency
		ent->v.renderamt = 127; // set its transparency amount
		ent->v.effects |= EF_NODRAW;
	}
	else if (strcmp(entityClassname, "func_vip_safetyzone") == 0 ||
		strcmp(entityClassname, "info_vip_safetyzone") == 0)
		g_mapType |= MAP_AS; // assassination map

	else if (strcmp(entityClassname, "hostage_entity") == 0)
		g_mapType |= MAP_CS; // rescue map

	else if (strcmp(entityClassname, "func_bomb_target") == 0 ||
		strcmp(entityClassname, "info_bomb_target") == 0)
		g_mapType |= MAP_DE; // defusion map

	else if (strcmp(entityClassname, "func_escapezone") == 0)
		g_mapType |= MAP_ES;

	// next maps doesn't have map-specific entities, so determine it by name
	else if (strncmp(GetMapName(), "fy_", 3) == 0) // fun map
		g_mapType |= MAP_FY;
	else if (strncmp(GetMapName(), "ka_", 3) == 0) // knife arena map
		g_mapType |= MAP_KA;
	else if (strncmp(GetMapName(), "awp_", 4) == 0) // SyPB Pro P.32 - AWP MAP TYPE
		g_mapType |= MAP_AWP;
	else if (strncmp(GetMapName(), "he_", 4) == 0) // ebot 1.57 - grenade wars
		g_mapType |= MAP_HE;

	if (g_isMetamod)
		RETURN_META_VALUE(MRES_IGNORED, 0);

	int result = (*g_functionTable.pfnSpawn) (ent); // get result

	if (ent->v.rendermode == kRenderTransTexture)
		ent->v.flags &= ~FL_WORLDBRUSH; // clear the FL_WORLDBRUSH flag out of transparent ents

	return result;
}

void Touch(edict_t* pentTouched, edict_t* pentOther)
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

	// SyPB Pro P.38 - Breakable improve
	if (!FNullEnt(pentTouched) && (pentOther->v.flags & FL_FAKECLIENT))
	{
		Bot* bot = g_botManager->GetBot(const_cast <edict_t*> (pentOther));
		if (bot != null)
			bot->CheckTouchEntity(pentTouched);
	}

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	(*g_functionTable.pfnTouch) (pentTouched, pentOther);
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
	if (strcmp(addr, "loopback") == 0)
		g_hostEntity = ent; // save the edict of the listen server client...

	if (g_isMetamod)
		RETURN_META_VALUE(MRES_IGNORED, 0);

	return (*g_functionTable.pfnClientConnect) (ent, name, addr, rejectReason);
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

	//callbacks->OnClientDisconnect (ent);
	int i = ENTINDEX(ent) - 1;

	InternalAssert(i >= 0 && i < 32);

	// check if its a bot
	if (g_botManager->GetBot(i) != null && g_botManager->GetBot(i)->pev == &ent->v)
		g_botManager->Free(i);

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	(*g_functionTable.pfnClientDisconnect) (ent);
}

void ClientUserInfoChanged(edict_t* ent, char* infobuffer)
{
	// this function is called when a player changes model, or changes team. Occasionally it
	// enforces rules on these changes (for example, some MODs don't want to allow players to
	// change their player model). But most commonly, this function is in charge of handling
	// team changes, recounting the teams population, etc...

	const char* passwordField = ebot_password_key.GetString();
	const char* password = ebot_password.GetString();

	if (IsNullString(passwordField) || IsNullString(password) || IsValidBot(ent))
	{
		if (g_isMetamod)
			RETURN_META(MRES_IGNORED);

		(*g_functionTable.pfnClientUserInfoChanged) (ent, infobuffer);
	}

	int clientIndex = ENTINDEX(ent) - 1;

	if (strcmp(password, INFOKEY_VALUE(infobuffer, const_cast <char*> (passwordField))) == 0)
		g_clients[clientIndex].flags |= CFLAG_OWNER;
	else
		g_clients[clientIndex].flags &= ~CFLAG_OWNER;

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	(*g_functionTable.pfnClientUserInfoChanged) (ent, infobuffer);
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
		if (stricmp(command, "ebot") == 0)
		{
			int state = BotCommandHandler(ent, IsNullString(CMD_ARGV(1)) ? "help" : CMD_ARGV(1), CMD_ARGV(2), CMD_ARGV(3), CMD_ARGV(4), CMD_ARGV(5), CMD_ARGV(6));

			switch (state)
			{
			case 0:
				ClientPrint(ent, print_withtag, "Unknown command: %s", arg1);
				break;

			case 3:
				ClientPrint(ent, print_withtag, "CVar ebot_%s, can be only set via RCON access.", CMD_ARGV(2));
				break;

			case 2:
				ClientPrint(ent, print_withtag, "Command %s, can only be executed from server console.", arg1);
				break;
			}
			if (g_isMetamod)
				RETURN_META(MRES_SUPERCEDE);

			return;
		}
		else if (stricmp(command, "menuselect") == 0 && !IsNullString(arg1) && g_clients[ENTINDEX(ent) - 1].menu != null)
		{
			Clients* client = &g_clients[ENTINDEX(ent) - 1];
			int selection = atoi(arg1);

			if (client->menu == &g_menus[12])
			{
				DisplayMenuToClient(ent, null); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
					g_waypoint->Add(selection - 1);
					break;

				case 8:
					g_waypoint->Add(100);
					break;

				case 9:
					g_waypoint->SetLearnJumpWaypoint();
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[13])
			{
				if (g_sgdWaypoint)
					DisplayMenuToClient(ent, &g_menus[13]);
				else
					DisplayMenuToClient(ent, null);

				switch (selection)
				{
				case 1:
					g_waypoint->ToggleFlags(WAYPOINT_FALLCHECK);
					break;

				case 2:
					g_waypoint->ToggleFlags(WAYPOINT_TERRORIST);
					break;

				case 3:
					g_waypoint->ToggleFlags(WAYPOINT_COUNTER);
					break;

				case 4:
					g_waypoint->ToggleFlags(WAYPOINT_LIFT);
					break;

				case 5:
					g_waypoint->ToggleFlags(WAYPOINT_SNIPER);
					break;

				case 6:
					g_waypoint->ToggleFlags(WAYPOINT_ZMHMCAMP);
					break;

				case 7:
					g_waypoint->ToggleFlags(WAYPOINT_CROUCH);
					break;

				case 8:
					g_waypoint->ToggleFlags(WAYPOINT_USEBUTTON);
					break;

				case 9:
					g_waypoint->DeleteFlags();
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[9])
			{
				DisplayMenuToClient(ent, null); // reset menu display

				switch (selection)
				{
				case 1:
					if (g_waypointOn)
						ServerCommand("ebot waypoint off");
					else
						ServerCommand("ebot waypoint on");
					break;

				case 2:
					g_waypointOn = true;
					g_waypoint->CacheWaypoint();
					break;

				case 3:
					g_waypointOn = true;
					DisplayMenuToClient(ent, &g_menus[20]);
					break;

				case 4:
					g_waypointOn = true;
					g_waypoint->DeletePath();
					break;

				case 5:
					g_waypointOn = true;
					DisplayMenuToClient(ent, &g_menus[12]);
					break;

				case 6:
					g_waypointOn = true;
					g_waypoint->Delete();
					break;

				case 7:
					g_waypointOn = true;
					DisplayMenuToClient(ent, &g_menus[19]);
					break;

				case 8:
					g_waypointOn = true;
					DisplayMenuToClient(ent, &g_menus[11]);
					break;

				case 9:
					DisplayMenuToClient(ent, &g_menus[10]);
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[10])
			{
				DisplayMenuToClient(ent, null); // reset menu display

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
					int usePoints = 0;

					for (int i = 0; i < g_numWaypoints; i++)
					{
						if (g_waypoint->GetPath(i)->flags & WAYPOINT_TERRORIST)
							terrPoints++;

						if (g_waypoint->GetPath(i)->flags & WAYPOINT_COUNTER)
							ctPoints++;

						if (g_waypoint->GetPath(i)->flags & WAYPOINT_GOAL)
							goalPoints++;

						if (g_waypoint->GetPath(i)->flags & WAYPOINT_RESCUE)
							rescuePoints++;

						if (g_waypoint->GetPath(i)->flags & WAYPOINT_CAMP)
							campPoints++;

						if (g_waypoint->GetPath(i)->flags & WAYPOINT_SNIPER)
							sniperPoints++;

						if (g_waypoint->GetPath(i)->flags & WAYPOINT_AVOID)
							avoidPoints++;

						if (g_waypoint->GetPath(i)->flags & WAYPOINT_USEBUTTON)
							usePoints++;
					}

					ServerPrintNoTag("Waypoints: %d - T Points: %d\n"
						"CT Points: %d - Goal Points: %d\n"
						"Rescue Points: %d - Camp Points: %d\n"
						"Avoid Points: %d - Sniper Points: %d\n"
						"Use Points: %d", g_numWaypoints, terrPoints, ctPoints, goalPoints, rescuePoints, campPoints, avoidPoints, sniperPoints, usePoints);
				}
				break;

				case 2:
					g_waypointOn = true;
					g_autoWaypoint &= 1;
					g_autoWaypoint ^= 1;

					CenterPrint("Auto-Waypoint %s", g_autoWaypoint ? "Enabled" : "Disabled");
					break;

				case 3:
					g_waypointOn = true;
					DisplayMenuToClient(ent, &g_menus[13]);
					break;

				case 4:
					if (g_waypoint->NodesValid())
						g_waypoint->Save();
					else
						CenterPrint("Waypoint not saved\nThere are errors, see console");
					break;

				case 5:
					g_waypoint->Save();
					break;

				case 6:
					g_waypoint->Load();
					break;

				case 7:
					if (g_waypoint->NodesValid())
						CenterPrint("Nodes work Find");
					else
						CenterPrint("There are errors, see console");
					break;

				case 8:
					ServerCommand("ebot wp on noclip");
					break;

				case 9:
					DisplayMenuToClient(ent, &g_menus[9]);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[11])
			{
				DisplayMenuToClient(ent, null); // reset menu display

				// SyPB Pro P.30 - SgdWP
				if (g_sgdWaypoint)
					DisplayMenuToClient(ent, &g_menus[21]);
				else
					DisplayMenuToClient(ent, null);

				g_waypointOn = true;  // turn waypoints on in case

				const int radiusValue[] = { 0, 8, 16, 32, 48, 64, 80, 96, 128 };

				if ((selection >= 1) && (selection <= 9))
					g_waypoint->SetRadius(radiusValue[selection - 1]);

				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[0])
			{
				DisplayMenuToClient(ent, null); // reset menu display

				switch (selection)
				{
				case 1:
					fillCommand = false;
					DisplayMenuToClient(ent, &g_menus[2]);
					break;

				case 2:
					DisplayMenuToClient(ent, &g_menus[1]);
					break;

				case 3:
					fillCommand = true;
					DisplayMenuToClient(ent, &g_menus[6]);
					break;

				case 4:
					g_botManager->KillAll();
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;

				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[2])
			{
				DisplayMenuToClient(ent, null); // reset menu display

				switch (selection)
				{
				case 1:
					g_botManager->AddRandom();
					break;

				case 2:
					DisplayMenuToClient(ent, &g_menus[5]);
					break;

				case 3:
					g_botManager->RemoveRandom();
					break;

				case 4:
					g_botManager->RemoveAll();
					break;

				case 5:
					g_botManager->RemoveMenu(ent, 1);
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[1])
			{
				DisplayMenuToClient(ent, null); // reset menu display

				extern ConVar ebot_debug;

				switch (selection)
				{
				case 1:
					DisplayMenuToClient(ent, &g_menus[3]);
					break;

				case 2:
					DisplayMenuToClient(ent, &g_menus[9]);
					break;

				case 3:
					DisplayMenuToClient(ent, &g_menus[4]);
					break;

				case 4:
					ebot_debug.SetInt(ebot_debug.GetInt() ^ 1);
					break;

				case 5:
					if (IsAlive(ent))
						DisplayMenuToClient(ent, &g_menus[18]);
					else
					{
						DisplayMenuToClient(ent, null); // reset menu display
						CenterPrint("You're dead, and have no access to this menu");
					}
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[18])
			{
				DisplayMenuToClient(ent, null); // reset menu display
				Bot* bot = null;

				switch (selection)
				{
				case 1:
				case 2:
					if (FindNearestPlayer(reinterpret_cast <void**> (&bot), client->ent, 4096.0, true, true, true))
					{
						if (!(bot->pev->weapons & (1 << WEAPON_C4)) && !bot->HasHostage() && (bot->GetCurrentTask()->taskID != TASK_PLANTBOMB) && (bot->GetCurrentTask()->taskID != TASK_DEFUSEBOMB))
						{
							if (selection == 1)
							{
								bot->ResetDoubleJumpState();

								bot->m_doubleJumpOrigin = GetEntityOrigin(client->ent);
								bot->m_doubleJumpEntity = client->ent;

								bot->PushTask(TASK_DOUBLEJUMP, TASKPRI_DOUBLEJUMP, -1, engine->GetTime(), true);
								bot->ChatSay(true, FormatBuffer("Ok %s, i will help you!", GetEntityName(ent)));
							}
							else if (selection == 2)
								bot->ResetDoubleJumpState();
							break;
						}
					}
					break;

				case 3:
				case 4:
					if (FindNearestPlayer(reinterpret_cast <void**> (&bot), ent, 300.0, true, true, true))
						bot->DiscardWeaponForUser(ent, selection == 4 ? false : true);
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}

				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[19])
			{
				DisplayMenuToClient(ent, null); // reset menu display

				const float autoDistanceValue[] = { 0.0f, 100.0f, 130.0f, 160.0f, 190.0f, 220.0f, 250.0f };

				if (selection >= 1 && selection <= 7)
					g_autoPathDistance = autoDistanceValue[selection - 1];

				if (g_autoPathDistance == 0)
					CenterPrint("AutoPath disabled");
				else
					CenterPrint("AutoPath maximum distance set to %f", g_autoPathDistance);

				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[20])
			{
				//DisplayMenuToClient (ent, null); // reset menu display

				if (g_sgdWaypoint)
					DisplayMenuToClient(ent, &g_menus[21]);
				else
					DisplayMenuToClient(ent, null);

				switch (selection)
				{
				case 1:
					g_waypoint->CreatePath(PATHCON_OUTGOING);
					break;

				case 2:
					g_waypoint->CreatePath(PATHCON_INCOMING);
					break;

				case 3:
					g_waypoint->CreatePath(PATHCON_BOTHWAYS);
					break;

				case 4:
					g_waypoint->CreatePath(PATHCON_JUMPING);
					break;

				case 5:
					g_waypoint->CreatePath(PATHCON_BOOSTING);
					break;

				case 6:
					g_waypoint->DeletePath();
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}

				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			// SyPB Pro P.20 - SgdWP
			else if (client->menu == &g_menus[21])
			{
				DisplayMenuToClient(ent, null);

				// SyPB Pro P.30 - SgdWP
				switch (selection)
				{
				case 1:
					DisplayMenuToClient(ent, &g_menus[22]);  // Add Waypoint
					break;

				case 2:
					DisplayMenuToClient(ent, &g_menus[13]); //Set Waypoint Flag
					break;

				case 3:
					DisplayMenuToClient(ent, &g_menus[20]); // Create Path
					break;

				case 4:
					DisplayMenuToClient(ent, &g_menus[11]); // Set Waypoint Radius
					break;

				case 5:
					DisplayMenuToClient(ent, &g_menus[21]);
					g_waypoint->TeleportWaypoint(); // Teleport to Waypoint
					break;

				case 6:
					DisplayMenuToClient(ent, &g_menus[21]);
					g_waypoint->Delete(); // Delete Waypoint 
					break;

				case 7:
					DisplayMenuToClient(ent, &g_menus[21]);
					g_sautoWaypoint = g_sautoWaypoint ? false : true; // Auto Put Waypoint Mode
					g_waypoint->SetLearnJumpWaypoint(g_sautoWaypoint ? 1 : 0);
					// SyPB Pro P.34 - SgdWP
					ChartPrint("[SgdWP] Auto Put Waypoint Mode: %s", (g_sautoWaypoint ? "On" : "Off"));
					break;

				case 9:
					g_waypoint->SgdWp_Set("save");

					// SyPB Pro P.45 - SgdWP
					if (g_sgdWaypoint)
						DisplayMenuToClient(ent, &g_menus[25]);
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}

				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[22])
			{
				DisplayMenuToClient(ent, &g_menus[22]);

				switch (selection)
				{
				case 1:
					g_waypoint->Add(0);
					g_waypoint->SetRadius(64);
					break;

				case 2:
				case 3:
				case 5:
					g_waypoint->Add(selection - 1);
					g_waypoint->SetRadius(64);
					break;

				case 4:
					g_waypoint->Add(selection - 1);
					g_waypoint->SetRadius(0);
					break;

				case 6:
					g_waypoint->Add(100);
					g_waypoint->SetRadius(32);
					break;

				case 7:
					g_waypoint->Add(5);
					g_waypoint->Add(6);
					g_waypoint->SetRadius(0);
					DisplayMenuToClient(ent, &g_menus[24]);
					break;

				case 8:
					g_waypoint->SetLearnJumpWaypoint();
					// SyPB Pro P.34 - SgdWP
					ChartPrint("[SgdWP] You could Jump now, system will auto save your Jump Point");
					break;

				case 9:
					DisplayMenuToClient(ent, &g_menus[23]);
					break;

				case 10:
					DisplayMenuToClient(ent, &g_menus[21]);
					break;
				}

				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[23])
			{
				DisplayMenuToClient(ent, &g_menus[23]);

				switch (selection)
				{
				case 1:
					g_waypoint->Add(0);
					g_waypoint->ToggleFlags(WAYPOINT_LIFT);
					g_waypoint->SetRadius(0);
					break;

				case 2:
					g_waypoint->Add(5);
					g_waypoint->Add(6);
					g_waypoint->ToggleFlags(WAYPOINT_SNIPER);
					g_waypoint->SetRadius(0);
					DisplayMenuToClient(ent, &g_menus[24]);
					break;

				case 3:
					// SyPB Pro P.29 - Zombie Mode Hm Camp Waypoints
					g_waypoint->Add(0);
					g_waypoint->ToggleFlags(WAYPOINT_ZMHMCAMP);
					g_waypoint->SetRadius(64);
					break;

				case 9:
					DisplayMenuToClient(ent, &g_menus[22]);
					break;

				case 10:
					DisplayMenuToClient(ent, &g_menus[21]);
					break;
				}

				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[24])
			{
				DisplayMenuToClient(ent, &g_menus[22]);

				switch (selection)
				{
				case 1:
					g_waypoint->ToggleFlags(WAYPOINT_TERRORIST);
					break;

				case 2:
					g_waypoint->ToggleFlags(WAYPOINT_COUNTER);
					break;

					break;
				}

				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			// SyPB Pro P.45 - SgdWP 
			else if (client->menu == &g_menus[25])
			{
				DisplayMenuToClient(ent, null);

				switch (selection)
				{
				case 1:
					g_waypoint->SgdWp_Set("save_non-check");
					break;

				case 2:
					DisplayMenuToClient(ent, &g_menus[21]);
					break;
				}

				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[5])
			{
				DisplayMenuToClient(ent, null); // reset menu display

				client->menu = &g_menus[4];

				switch (selection)
				{
				case 1:
					g_storeAddbotVars[0] = engine->RandomInt(0, 20);
					break;

				case 2:
					g_storeAddbotVars[0] = engine->RandomInt(20, 40);
					break;

				case 3:
					g_storeAddbotVars[0] = engine->RandomInt(40, 60);
					break;

				case 4:
					g_storeAddbotVars[0] = engine->RandomInt(60, 80);
					break;

				case 5:
					g_storeAddbotVars[0] = engine->RandomInt(80, 99);
					break;

				case 6:
					g_storeAddbotVars[0] = 100;
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}

				if (client->menu == &g_menus[4])
					DisplayMenuToClient(ent, &g_menus[4]);

				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[6] && fillCommand)
			{
				DisplayMenuToClient(ent, null); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
					// turn off cvars if specified team
					CVAR_SET_STRING("mp_limitteams", "0");
					CVAR_SET_STRING("mp_autoteambalance", "0");

				case 5:
					fillServerTeam = selection;
					DisplayMenuToClient(ent, &g_menus[5]);
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[4] && fillCommand)
			{
				DisplayMenuToClient(ent, null); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
					g_botManager->FillServer(fillServerTeam, selection - 2, g_storeAddbotVars[0]);

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[6])
			{
				DisplayMenuToClient(ent, null); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 5:
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

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[4])
			{
				DisplayMenuToClient(ent, null); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
					g_storeAddbotVars[3] = selection - 2;
					DisplayMenuToClient(ent, &g_menus[6]);
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[7] || client->menu == &g_menus[8])
			{
				DisplayMenuToClient(ent, null); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
					g_storeAddbotVars[2] = selection;
					g_botManager->AddBot("", g_storeAddbotVars[0], g_storeAddbotVars[3], g_storeAddbotVars[1], g_storeAddbotVars[2]);
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[3])
			{
				DisplayMenuToClient(ent, null); // reset menu display

				switch (selection)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
					g_botManager->SetWeaponMode(selection);
					break;

				case 10:
					DisplayMenuToClient(ent, null);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[14])
			{
				DisplayMenuToClient(ent, null); // reset menu display

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
					g_botManager->GetBot(selection - 1)->Kick();
					break;

				case 9:
					g_botManager->RemoveMenu(ent, 2);
					break;

				case 10:
					DisplayMenuToClient(ent, &g_menus[2]);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[15])
			{
				DisplayMenuToClient(ent, null); // reset menu display

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
					g_botManager->GetBot(selection + 8 - 1)->Kick();
					break;

				case 9:
					g_botManager->RemoveMenu(ent, 3);
					break;

				case 10:
					g_botManager->RemoveMenu(ent, 1);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[16])
			{
				DisplayMenuToClient(ent, null); // reset menu display

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
					g_botManager->GetBot(selection + 16 - 1)->Kick();
					break;

				case 9:
					g_botManager->RemoveMenu(ent, 4);
					break;

				case 10:
					g_botManager->RemoveMenu(ent, 2);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
			else if (client->menu == &g_menus[17])
			{
				DisplayMenuToClient(ent, null); // reset menu display

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
					g_botManager->GetBot(selection + 24 - 1)->Kick();
					break;

				case 10:
					g_botManager->RemoveMenu(ent, 3);
					break;
				}
				if (g_isMetamod)
					RETURN_META(MRES_SUPERCEDE);

				return;
			}
		}
	}

	if (!g_isFakeCommand && (stricmp(command, "say") == 0 || stricmp(command, "say_team") == 0))
	{
		Bot* bot = null;

		if (FStrEq(arg1, "dropme") || FStrEq(arg1, "dropc4"))
		{
			if (FindNearestPlayer(reinterpret_cast <void**> (&bot), ent, 300.0, true, true, true))
				bot->DiscardWeaponForUser(ent, IsNullString(strstr(arg1, "c4")) ? false : true);

			return;
		}

		bool isAlive = IsAlive(ent);
		int team = -1;

		if (FStrEq(command, "say_team"))
			team = GetTeam(ent);

		for (int i = 0; i < engine->GetMaxClients(); i++)
		{
			if (!(g_clients[i].flags & CFLAG_USED) || (team != -1 && team != g_clients[i].team) || isAlive != IsAlive(g_clients[i].ent))
				continue;

			Bot* iter = g_botManager->GetBot(i);

			if (iter != null)
			{
				iter->m_sayTextBuffer.entityIndex = ENTINDEX(ent);

				if (IsNullString(CMD_ARGS()))
					continue;

				strcpy(iter->m_sayTextBuffer.sayText, CMD_ARGS());
				iter->m_sayTextBuffer.timeNextChat = engine->GetTime() + iter->m_sayTextBuffer.chatDelay;
			}
		}
	}
	int clientIndex = ENTINDEX(ent) - 1;

	// check if this player alive, and issue something
	if ((g_clients[clientIndex].flags & CFLAG_ALIVE) && g_radioSelect[clientIndex] != 0 && strncmp(command, "menuselect", 10) == 0)
	{
		int radioCommand = atoi(arg1);

		if (radioCommand != 0)
		{
			radioCommand += 10 * (g_radioSelect[clientIndex] - 1);

			if (radioCommand != Radio_Affirmative && radioCommand != Radio_Negative && radioCommand != Radio_ReportingIn)
			{
				for (int i = 0; i < engine->GetMaxClients(); i++)
				{
					Bot* bot = g_botManager->GetBot(i);

					// validate bot
					if (bot != null && GetTeam(bot->GetEntity()) == g_clients[clientIndex].team && VARS(ent) != bot->pev && bot->m_radioOrder == 0)
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
	else if (strncmp(command, "radio", 5) == 0)
		g_radioSelect[clientIndex] = atoi(&command[5]);

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	(*g_functionTable.pfnClientCommand) (ent);
}


void ServerActivate(edict_t* pentEdictList, int edictCount, int clientMax)
{
	// this function is called when the server has fully loaded and is about to manifest itself
	// on the network as such. Since a mapchange is actually a server shutdown followed by a
	// restart, this function is also called when a new map is being loaded. Hence it's the
	// perfect place for doing initialization stuff for our bots, such as reading the BSP data,
	// loading the bot profiles, and drawing the world map (ie, filling the navigation hashtable).
	// Once this function has been called, the server can be considered as "running".

	InitConfig(); // initialize all config files

	// do level initialization stuff here...
	g_waypoint->Initialize();
	g_exp.Unload();
	g_waypoint->Load();

	// execute main config
	ServerCommand("exec addons/ebot/ebot.cfg");

	if (TryFileOpen(FormatBuffer("%s/maps/%s_ebot.cfg", GetModName(), GetMapName())))
	{
		ServerCommand("exec maps/%s_ebot.cfg", GetMapName());
		ServerPrint("Executing Map-Specific config file");
	}

	g_botManager->InitQuota();

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	(*g_functionTable.pfnServerActivate) (pentEdictList, edictCount, clientMax);

	g_waypoint->InitializeVisibility();
}

void ServerDeactivate(void)
{
	// this function is called when the server is shutting down. A particular note about map
	// changes: changing the map means shutting down the server and starting a new one. Of course
	// this process is transparent to the user, but either in single player when the hero reaches
	// a new level and in multiplayer when it's time for a map change, be aware that what happens
	// is that the server actually shuts down and restarts with a new map. Hence we can use this
	// function to free and deinit anything which is map-specific, for example we free the memory
	// space we m'allocated for our BSP data, since a new map means new BSP data to interpret. In
	// any case, when the new map will be booting, ServerActivate() will be called, so we'll do
	// the loading of new bots and the new BSP data parsing there.

	// save collected experience on shutdown
	g_exp.Unload();

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	(*g_functionTable.pfnServerDeactivate) ();
}

void KeyValue(edict_t* ent, KeyValueData* data)
{
	// this function is called when the game requests a pointer to some entity's keyvalue data.
	// The keyvalue data is held in each entity's infobuffer (basically a char buffer where each
	// game DLL can put the stuff it wants) under - as it says - the form of a key/value pair. A
	// common example of key/value pair is the "model", "(name of player model here)" one which
	// is often used for client DLLs to display player characters with the right model (else they
	// would all have the dull "models/player.mdl" one). The entity for which the keyvalue data
	// pointer is requested is pentKeyvalue, the pointer to the keyvalue data structure pkvd.

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	(*g_functionTable.pfnKeyValue) (ent, data);
}

void LoadEntityData(void)
{
	edict_t* entity;
	int i;

	for (i = 0; i < entityNum; i++)
	{
		if (g_entityId[i] == -1)
			continue;

		entity = INDEXENT(g_entityId[i]);
		if (FNullEnt(entity) || !IsAlive(entity))
		{
			SetEntityActionData(i);
			continue;
		}

		if (g_entityGetWpTime[i] + 1.5f < engine->GetTime() || g_entityWpIndex[i] == -1)
			SetEntityWaypoint(entity);
	}

	for (i = 0; i < engine->GetMaxClients(); i++)
	{
		entity = INDEXENT(i + 1);

		if (FNullEnt(entity) || !(entity->v.flags & FL_CLIENT))
		{
			g_clients[i].flags &= ~(CFLAG_USED | CFLAG_ALIVE);
			g_clients[i].ent = null;
			g_clients[i].wpIndex = -1;
			g_clients[i].wpIndex2 = -1;
			g_clients[i].getWpOrigin = nullvec;
			g_clients[i].getWPTime = 0.0f;

			g_clients[i].isZombiePlayerAPI = -1;
			continue;
		}

		g_clients[i].ent = entity;
		g_clients[i].flags |= CFLAG_USED;

		if (IsAlive(entity))
			g_clients[i].flags |= CFLAG_ALIVE;
		else
			g_clients[i].flags &= ~CFLAG_ALIVE;

		if (g_clients[i].flags & CFLAG_ALIVE)
		{
			// keep the clipping mode enabled, or it can be turned off after new round has started
			if (g_hostEntity == entity && g_editNoclip && g_waypointOn)
				g_hostEntity->v.movetype = MOVETYPE_NOCLIP;

			g_clients[i].origin = GetEntityOrigin(entity);
			if (g_clients[i].getWPTime + 1.2f < engine->GetTime() || (g_clients[i].wpIndex == -1 && g_clients[i].wpIndex2 == -1))
				SetEntityWaypoint(entity);

			SoundSimulateUpdate(i);
			continue;
		}

		g_clients[i].wpIndex = -1;
		g_clients[i].wpIndex2 = -1;
		g_clients[i].getWpOrigin = nullvec;
		g_clients[i].getWPTime = 0.0f;

		g_clients[i].isZombiePlayerAPI = -1;
	}
}

void CalculatePings()
{
	int average[2] = { 0, 0 };
	int numHumans = 0;

	const auto emit = [](int s0, int s1, int s2)
	{
		return (s0 & ((1ULL << s1) - 1)) << s2;
	};

	// first get average ping on server, and store real client pings
	for (auto& client : g_clients)
	{
		if (!(client.flags & CFLAG_USED) || IsValidBot(client.ent))
			continue;

		int ping, loss;
		PLAYER_CNX_STATS(client.ent, &ping, &loss);

		// store normal client ping
		client.ping = emit(loss, 7, 18) | emit(ping > 0 ? ping / 2 : engine->RandomInt(8, 16), 12, 6) | emit(ENTINDEX(client.ent), 5, 1) | 1;
		client.pingUpdate = true;

		numHumans++;

		average[0] += ping;
		average[1] += loss;
	}

	if (numHumans > 0)
	{
		average[0] /= numHumans;
		average[1] /= numHumans;
	}
	else
	{
		average[0] = engine->RandomInt(30, 40);
		average[1] = engine->RandomInt(5, 10);
	}

	// now calculate bot ping based on average from players
	for (auto& client : g_clients)
	{
		if (!(client.flags & CFLAG_USED))
			continue;

		// we're only intrested in bots here
		auto bot = g_botManager->GetBot(client.ent);
		if (bot == null)
			continue;

		int part = static_cast <int> (average[0] * 0.2f);

		int botPing = bot->m_basePingLevel + engine->RandomInt(average[0] - part, average[0] + part) + engine->RandomInt(bot->m_difficulty / 2, bot->m_difficulty); // remember me
		int botLoss = engine->RandomInt(average[1] / 2, average[1]);

		if (botPing <= 5)
			botPing = engine->RandomInt(10, 23);
		else if (botPing > 70)
			botPing = engine->RandomInt(30, 40);
		
		client.ping = emit(botLoss, 7, 18) | emit(botPing, 12, 6) | emit(ENTINDEX(client.ent), 5, 1) | 1;
		client.pingUpdate = true; // force resend ping
	}
}

void SetPing(edict_t* to)
{
	// missing from sdk
	constexpr int gamePingSVC = 17;

	for (auto& client : g_clients)
	{
		if (!(client.flags & CFLAG_USED))
			continue;
		
		if (!client.pingUpdate)
			continue;

		client.pingUpdate = false;

		// no ping, no fun
		if (client.ping <= 0)
			client.ping = engine->RandomInt(15, 50);

		MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, gamePingSVC, nullptr, to);
		WRITE_LONG(client.ping);
		MESSAGE_END();
	}
}

static float secondTimer = 0.0;
void StartFrame(void)
{
	// this function starts a video frame. It is called once per video frame by the engine. If
	// you run Half-Life at 90 fps, this function will then be called 90 times per second. By
	// placing a hook on it, we have a good place to do things that should be done continuously
	// during the game, for example making the bots think (yes, because no Think() function exists
	// for the bots by the MOD side, remember). Also here we have control on the bot population,
	// for example if a new player joins the server, we should disconnect a bot, and if the
	// player population decreases, we should fill the server with other bots.

	if (g_analyzewaypoints == true)
		g_waypoint->Analyze();

	if (ebot_lockzbot.GetBool())
	{
		if (CVAR_GET_FLOAT("bot_quota") > 0)
		{
			CVAR_SET_FLOAT("ebot_quota", CVAR_GET_FLOAT("bot_quota"));
			ServerPrint("ebot_lockzbot is 1, you cannot add Z-Bot");
			ServerPrint("You can input ebot_lockzbot unlock add Z-Bot");
			ServerPrint("But, If you have use AMXX plug-in or Zombie Mod, I think this is not good choice");
			CVAR_SET_FLOAT("bot_quota", 0);
		}
	}

	/*extern ConVar ebot_ping;
	if (ebot_ping.GetBool())
	{
		for (auto& client : g_clients)
		{
			if (!(client.flags & CFLAG_USED))
				continue;

			SetPing(client.ent);
		}
	}*/

	if (secondTimer < engine->GetTime())
	{
		LoadEntityData();

		//if (ebot_ping.GetBool())
		//	CalculatePings();

		int i;
		if (IsDedicatedServer())
		{
			for (i = 0; i < engine->GetMaxClients(); i++)
			{
				edict_t* player = INDEXENT(i + 1);

				// code below is executed only on dedicated server
				if (!FNullEnt(player) && (player->v.flags & FL_CLIENT) && !(player->v.flags & FL_FAKECLIENT))
				{
					const char* password = ebot_password.GetString();
					const char* key = ebot_password_key.GetString();

					if (g_clients[i].flags & CFLAG_OWNER)
					{
						if (IsNullString(key) && IsNullString(password))
							g_clients[i].flags &= ~CFLAG_OWNER;
						else if (strcmp(password, INFOKEY_VALUE(GET_INFOKEYBUFFER(g_clients[i].ent), (char*)key)) == 0)
						{
							g_clients[i].flags &= ~CFLAG_OWNER;
							ServerPrint("Player %s had lost remote access to ebot.", GetEntityName(player));
						}
					}
					else if (IsNullString(key) && IsNullString(password))
					{
						if (strcmp(password, INFOKEY_VALUE(GET_INFOKEYBUFFER(g_clients[i].ent), (char*)key)) == 0)
						{
							g_clients[i].flags |= CFLAG_OWNER;
							ServerPrint("Player %s had gained full remote access to ebot.", GetEntityName(player));
						}
					}
				}
			}
		}
		else if (!FNullEnt(g_hostEntity))
		{
			if (g_waypointOn)
			{
				bool hasBot = false;
				for (i = 0; i < engine->GetMaxClients(); i++)
				{
					if (g_botManager->GetBot(i))
					{
						hasBot = true;
						g_botManager->RemoveAll();
						break;
					}
				}

				if (!hasBot)
					g_waypoint->Think();

				if (ebot_showwp.GetBool() == true)
					ebot_showwp.SetInt(0);
			}
			else if (ebot_showwp.GetBool() == true)
				g_waypoint->ShowWaypointMsg();

			CheckWelcomeMessage();
		}

		extern ConVar ebot_dangerfactor;

		if (ebot_dangerfactor.GetFloat() > 1024.0f)
			ebot_dangerfactor.SetFloat(1024.0f);

		if (ebot_dangerfactor.GetFloat() < 256.0f)
			ebot_dangerfactor.SetFloat(256.0f);

		secondTimer = engine->GetTime() + 1.0f;
	}

	if (g_bombPlanted)
		g_waypoint->SetBombPosition();

	// keep bot number up to date
	g_botManager->MaintainBotQuota();

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	(*g_functionTable.pfnStartFrame) ();
}

void StartFrame_Post(void)
{
	// this function starts a video frame. It is called once per video frame by the engine. If
	// you run Half-Life at 90 fps, this function will then be called 90 times per second. By
	// placing a hook on it, we have a good place to do things that should be done continuously
	// during the game, for example making the bots think (yes, because no Think() function exists
	// for the bots by the MOD side, remember).  Post version called only by metamod.

	// **** AI EXECUTION STARTS ****
	g_botManager->Think();
	// **** AI EXECUTION FINISH ****

	RETURN_META(MRES_IGNORED);
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

	RETURN_META_VALUE(MRES_IGNORED, 0);
}

void ServerActivate_Post(edict_t* /*pentEdictList*/, int /*edictCount*/, int /*clientMax*/)
{
	// this function is called when the server has fully loaded and is about to manifest itself
	// on the network as such. Since a mapchange is actually a server shutdown followed by a
	// restart, this function is also called when a new map is being loaded. Hence it's the
	// perfect place for doing initialization stuff for our bots, such as reading the BSP data,
	// loading the bot profiles, and drawing the world map (ie, filling the navigation hashtable).
	// Once this function has been called, the server can be considered as "running". Post version
	// called only by metamod.

	g_waypoint->InitializeVisibility();

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

	DetectCSVersion(); // determine version of currently running cs
	RETURN_META(MRES_IGNORED);
}

void pfnChangeLevel(char* s1, char* s2)
{
	// the purpose of this function is to ask the engine to shutdown the server and restart a
	// new one running the map whose name is s1. It is used ONLY IN SINGLE PLAYER MODE and is
	// transparent to the user, because it saves the player state and equipment and restores it
	// back in the new level. The "changelevel trigger point" in the old level is linked to the
	// new level's spawn point using the s2 string, which is formatted as follows: "trigger_name
	// to spawnpoint_name", without spaces (for example, "tr_1atotr_2lm" would tell the engine
	// the player has reached the trigger point "tr_1a" and has to spawn in the next level on the
	// spawn point named "tr_2lm".

	// save collected experience on map change
	g_exp.Unload();

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	CHANGE_LEVEL(s1, s2);
}

edict_t* pfnFindEntityByString(edict_t* edictStartSearchAfter, const char* field, const char* value)
{
	// round starts in counter-strike 1.5
	if (strcmp(value, "info_map_parameters") == 0)
		RoundInit();

	if (g_isMetamod)
		RETURN_META_VALUE(MRES_IGNORED, 0);

	return FIND_ENTITY_BY_STRING(edictStartSearchAfter, field, value);
}

void pfnEmitSound(edict_t* entity, int channel, const char* sample, float volume, float attenuation, int flags, int pitch)
{
	// this function tells the engine that the entity pointed to by "entity", is emitting a sound
	// which fileName is "sample", at level "channel" (CHAN_VOICE, etc...), with "volume" as
	// loudness multiplicator (normal volume VOL_NORM is 1.0f), with a pitch of "pitch" (normal
	// pitch PITCH_NORM is 100.0f), and that this sound has to be attenuated by distance in air
	// according to the value of "attenuation" (normal attenuation ATTN_NORM is 0.8 ; ATTN_NONE
	// means no attenuation with distance). Optionally flags "fFlags" can be passed, which I don't
	// know the heck of the purpose. After we tell the engine to emit the sound, we have to call
	// SoundAttachToThreat() to bring the sound to the ears of the bots. Since bots have no client DLL
	// to handle this for them, such a job has to be done manually.

	SoundAttachToThreat(entity, sample, volume);

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	(*g_engfuncs.pfnEmitSound) (entity, channel, sample, volume, attenuation, flags, pitch);
}

void pfnClientCommand(edict_t* ent, char* format, ...)
{
	// this function forces the client whose player entity is ent to issue a client command.
	// How it works is that clients all have a g_xgv global string in their client DLL that
	// stores the command string; if ever that string is filled with characters, the client DLL
	// sends it to the engine as a command to be executed. When the engine has executed that
	// command, this g_xgv string is reset to zero. Here is somehow a curious implementation of
	// ClientCommand: the engine sets the command it wants the client to issue in his g_xgv, then
	// the client DLL sends it back to the engine, the engine receives it then executes the
	// command therein. Don't ask me why we need all this complicated crap. Anyhow since bots have
	// no client DLL, be certain never to call this function upon a bot entity, else it will just
	// make the server crash. Since hordes of uncautious, not to say stupid, programmers don't
	// even imagine some players on their servers could be bots, this check is performed less than
	// sometimes actually by their side, that's why we strongly recommend to check it here too. In
	// case it's a bot asking for a client command, we handle it like we do for bot commands, ie
	// using FakeClientCommand().

	va_list ap;
	char buffer[1024];

	va_start(ap, format);
	vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);

	// is the target entity an official bot, or a third party bot ?
	if (IsValidBot(ent) || (ent->v.flags & FL_DORMANT))
	{
		if (g_isMetamod)
			RETURN_META(MRES_SUPERCEDE); // prevent bots to be forced to issue client commands

		return;
	}

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	CLIENT_COMMAND(ent, buffer);
}

void pfnMessageBegin(int msgDest, int msgType, const float* origin, edict_t* ed)
{
	// this function called each time a message is about to sent.

	// store the message type in our own variables, since the GET_USER_MSG_ID () will just do a lot of strcmp()'s...
	if (g_isMetamod && g_netMsg->GetId(NETMSG_MONEY) == -1)
	{
		g_netMsg->SetId(NETMSG_VGUI, GET_USER_MSG_ID(PLID, "VGUIMenu", null));
		g_netMsg->SetId(NETMSG_SHOWMENU, GET_USER_MSG_ID(PLID, "ShowMenu", null));
		g_netMsg->SetId(NETMSG_WLIST, GET_USER_MSG_ID(PLID, "WeaponList", null));
		g_netMsg->SetId(NETMSG_CURWEAPON, GET_USER_MSG_ID(PLID, "CurWeapon", null));
		g_netMsg->SetId(NETMSG_AMMOX, GET_USER_MSG_ID(PLID, "AmmoX", null));
		g_netMsg->SetId(NETMSG_AMMOPICK, GET_USER_MSG_ID(PLID, "AmmoPickup", null));
		g_netMsg->SetId(NETMSG_DAMAGE, GET_USER_MSG_ID(PLID, "Damage", null));
		g_netMsg->SetId(NETMSG_MONEY, GET_USER_MSG_ID(PLID, "Money", null));
		g_netMsg->SetId(NETMSG_STATUSICON, GET_USER_MSG_ID(PLID, "StatusIcon", null));
		g_netMsg->SetId(NETMSG_DEATH, GET_USER_MSG_ID(PLID, "DeathMsg", null));
		g_netMsg->SetId(NETMSG_SCREENFADE, GET_USER_MSG_ID(PLID, "ScreenFade", null));
		g_netMsg->SetId(NETMSG_HLTV, GET_USER_MSG_ID(PLID, "HLTV", null));
		g_netMsg->SetId(NETMSG_TEXTMSG, GET_USER_MSG_ID(PLID, "TextMsg", null));
		g_netMsg->SetId(NETMSG_SCOREINFO, GET_USER_MSG_ID(PLID, "ScoreInfo", null));
		g_netMsg->SetId(NETMSG_BARTIME, GET_USER_MSG_ID(PLID, "BarTime", null));
		g_netMsg->SetId(NETMSG_SENDAUDIO, GET_USER_MSG_ID(PLID, "SendAudio", null));
		g_netMsg->SetId(NETMSG_SAYTEXT, GET_USER_MSG_ID(PLID, "SayText", null));

		if (g_gameVersion != CSVER_VERYOLD)
			g_netMsg->SetId(NETMSG_BOTVOICE, GET_USER_MSG_ID(PLID, "BotVoice", null));
	}
	g_netMsg->Reset();

	if (msgDest == MSG_SPEC && msgType == g_netMsg->GetId(NETMSG_HLTV) && g_gameVersion != CSVER_VERYOLD)
		g_netMsg->SetMessage(NETMSG_HLTV);

	g_netMsg->HandleMessageIfRequired(msgType, NETMSG_WLIST);

	if (!FNullEnt(ed))
	{
		int index = g_botManager->GetIndex(ed);

		// is this message for a bot?
		if (index != -1 && !(ed->v.flags & FL_DORMANT) && g_botManager->GetBot(index)->GetEntity() == ed)
		{
			g_netMsg->Reset();

			g_netMsg->SetBot(g_botManager->GetBot(index));

			// message handling is done in usermsg.cpp
			g_netMsg->HandleMessageIfRequired(msgType, NETMSG_VGUI);
			g_netMsg->HandleMessageIfRequired(msgType, NETMSG_CURWEAPON);
			g_netMsg->HandleMessageIfRequired(msgType, NETMSG_AMMOX);
			g_netMsg->HandleMessageIfRequired(msgType, NETMSG_AMMOPICK);
			g_netMsg->HandleMessageIfRequired(msgType, NETMSG_DAMAGE);
			g_netMsg->HandleMessageIfRequired(msgType, NETMSG_MONEY);
			g_netMsg->HandleMessageIfRequired(msgType, NETMSG_STATUSICON);
			g_netMsg->HandleMessageIfRequired(msgType, NETMSG_SCREENFADE);
			g_netMsg->HandleMessageIfRequired(msgType, NETMSG_BARTIME);
			g_netMsg->HandleMessageIfRequired(msgType, NETMSG_TEXTMSG);
			g_netMsg->HandleMessageIfRequired(msgType, NETMSG_SHOWMENU);
		}
	}
	else if (msgDest == MSG_ALL)
	{
		g_netMsg->Reset();

		//g_netMsg->HandleMessageIfRequired (msgType, NETMSG_SCOREINFO);
		g_netMsg->HandleMessageIfRequired(msgType, NETMSG_DEATH);
		g_netMsg->HandleMessageIfRequired(msgType, NETMSG_TEXTMSG);

		if (msgType == SVC_INTERMISSION)
		{
			for (int i = 0; i < engine->GetMaxClients(); i++)
			{
				Bot* bot = g_botManager->GetBot(i);

				if (bot != null)
					bot->m_notKilled = false;
			}
		}
	}

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	MESSAGE_BEGIN(msgDest, msgType, origin, ed);
}

void pfnMessageEnd(void)
{
	g_netMsg->Reset();

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	MESSAGE_END();
}

void pfnWriteByte(int value)
{
	// if this message is for a bot, call the client message function...
	g_netMsg->Execute((void*)&value);

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	WRITE_BYTE(value);
}

void pfnWriteChar(int value)
{
	// if this message is for a bot, call the client message function...
	g_netMsg->Execute((void*)&value);

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	WRITE_CHAR(value);
}

void pfnWriteShort(int value)
{
	// if this message is for a bot, call the client message function...
	g_netMsg->Execute((void*)&value);

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	WRITE_SHORT(value);
}

void pfnWriteLong(int value)
{
	// if this message is for a bot, call the client message function...
	g_netMsg->Execute((void*)&value);

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	WRITE_LONG(value);
}

void pfnWriteAngle(float value)
{
	// if this message is for a bot, call the client message function...
	g_netMsg->Execute((void*)&value);

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	WRITE_ANGLE(value);
}

void pfnWriteCoord(float value)
{
	// if this message is for a bot, call the client message function...
	g_netMsg->Execute((void*)&value);

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	WRITE_COORD(value);
}

void pfnWriteString(const char* sz)
{
	//Bot *bot = g_botManager->FindOneValidAliveBot ();

	// if this message is for a bot, call the client message function...
	g_netMsg->Execute((void*)sz);

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	WRITE_STRING(sz);
}

void pfnWriteEntity(int value)
{
	// if this message is for a bot, call the client message function...
	g_netMsg->Execute((void*)&value);

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	WRITE_ENTITY(value);
}

int pfnCmd_Argc(void)
{
	// this function returns the number of arguments the current client command string has. Since
	// bots have no client DLL and we may want a bot to execute a client command, we had to
	// implement a g_xgv string in the bot DLL for holding the bots' commands, and also keep
	// track of the argument count. Hence this hook not to let the engine ask an unexistent client
	// DLL for a command we are holding here. Of course, real clients commands are still retrieved
	// the normal way, by asking the engine.

	// is this a bot issuing that client command ?
	if (g_isFakeCommand)
	{
		if (g_isMetamod)
			RETURN_META_VALUE(MRES_SUPERCEDE, g_fakeArgc);

		return g_fakeArgc; // if so, then return the argument count we know
	}

	if (g_isMetamod)
		RETURN_META_VALUE(MRES_IGNORED, 0);

	return CMD_ARGC(); // ask the engine how many arguments there are
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
		if (strncmp("say ", g_fakeArgv, 4) == 0)
		{
			if (g_isMetamod)
				RETURN_META_VALUE(MRES_SUPERCEDE, g_fakeArgv + 4);

			return g_fakeArgv + 4; // skip the "say" bot client command
		}
		else if (strncmp("say_team ", g_fakeArgv, 9) == 0)
		{
			if (g_isMetamod)
				RETURN_META_VALUE(MRES_SUPERCEDE, g_fakeArgv + 9);

			return g_fakeArgv + 9; // skip the "say_team" bot client command
		}

		if (g_isMetamod)
			RETURN_META_VALUE(MRES_SUPERCEDE, g_fakeArgv);

		return g_fakeArgv; // else return the whole bot client command string we know
	}

	if (g_isMetamod)
		RETURN_META_VALUE(MRES_IGNORED, null);

	return CMD_ARGS(); // ask the client command string to the engine
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
	{
		if (g_isMetamod)
			RETURN_META_VALUE(MRES_SUPERCEDE, GetField(g_fakeArgv, argc));

		return GetField(g_fakeArgv, argc); // if so, then return the wanted argument we know
	}
	if (g_isMetamod)
		RETURN_META_VALUE(MRES_IGNORED, null);

	return CMD_ARGV(argc); // ask the argument number "argc" to the engine
}

void pfnClientPrintf(edict_t* ent, PRINT_TYPE printType, const char* message)
{
	// this function prints the text message string pointed to by message by the client side of
	// the client entity pointed to by ent, in a manner depending of printType (print_console,
	// print_center or print_chat). Be certain never to try to feed a bot with this function,
	// as it will crash your server. Why would you, anyway ? bots have no client DLL as far as
	// we know, right ? But since stupidity rules this world, we do a preventive check :)

	if (ent->v.flags & FL_FAKECLIENT)
	{
		if (g_isMetamod)
			RETURN_META(MRES_SUPERCEDE);

		return;
	}

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	CLIENT_PRINTF(ent, printType, message);
}

void pfnSetClientMaxspeed(const edict_t* ent, float newMaxspeed)
{
	Bot* bot = g_botManager->GetBot(const_cast <edict_t*> (ent));

	// check wether it's not a bot
	if (bot != null)
		bot->pev->maxspeed = newMaxspeed;

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	(*g_engfuncs.pfnSetClientMaxspeed) (ent, newMaxspeed);
}

int pfnRegUserMsg_Post(const char* name, int size)
{
	if (g_isMetamod)
		RETURN_META_VALUE(MRES_IGNORED, 0);

	return REG_USER_MSG(name, size);
}

int pfnRegUserMsg(const char* name, int size)
{
	// this function registers a "user message" by the engine side. User messages are network
	// messages the game DLL asks the engine to send to clients. Since many MODs have completely
	// different client features (Counter-Strike has a radar and a timer, for example), network
	// messages just can't be the same for every MOD. Hence here the MOD DLL tells the engine,
	// "Hey, you have to know that I use a network message whose name is pszName and it is size
	// packets long". The engine books it, and returns the ID number under which he recorded that
	// custom message. Thus every time the MOD DLL will be wanting to send a message named pszName
	// using pfnMessageBegin (), it will know what message ID number to send, and the engine will
	// know what to do, only for non-metamod version

	if (g_isMetamod)
		RETURN_META_VALUE(MRES_IGNORED, 0);

	int message = REG_USER_MSG(name, size);

	if (strcmp(name, "VGUIMenu") == 0)
		g_netMsg->SetId(NETMSG_VGUI, message);
	else if (strcmp(name, "ShowMenu") == 0)
		g_netMsg->SetId(NETMSG_SHOWMENU, message);
	else if (strcmp(name, "WeaponList") == 0)
		g_netMsg->SetId(NETMSG_WLIST, message);
	else if (strcmp(name, "CurWeapon") == 0)
		g_netMsg->SetId(NETMSG_CURWEAPON, message);
	else if (strcmp(name, "AmmoX") == 0)
		g_netMsg->SetId(NETMSG_AMMOX, message);
	else if (strcmp(name, "AmmoPickup") == 0)
		g_netMsg->SetId(NETMSG_AMMOPICK, message);
	else if (strcmp(name, "Damage") == 0)
		g_netMsg->SetId(NETMSG_DAMAGE, message);
	else if (strcmp(name, "Money") == 0)
		g_netMsg->SetId(NETMSG_MONEY, message);
	else if (strcmp(name, "StatusIcon") == 0)
		g_netMsg->SetId(NETMSG_STATUSICON, message);
	else if (strcmp(name, "DeathMsg") == 0)
		g_netMsg->SetId(NETMSG_DEATH, message);
	else if (strcmp(name, "ScreenFade") == 0)
		g_netMsg->SetId(NETMSG_SCREENFADE, message);
	else if (strcmp(name, "HLTV") == 0)
		g_netMsg->SetId(NETMSG_HLTV, message);
	else if (strcmp(name, "TextMsg") == 0)
		g_netMsg->SetId(NETMSG_TEXTMSG, message);
	//else if (strcmp (name, "ScoreInfo") == 0)
	   //g_netMsg->SetId (NETMSG_SCOREINFO, message);
	else if (strcmp(name, "BarTime") == 0)
		g_netMsg->SetId(NETMSG_BARTIME, message);
	else if (strcmp(name, "SendAudio") == 0)
		g_netMsg->SetId(NETMSG_SENDAUDIO, message);
	else if (strcmp(name, "SayText") == 0)
		g_netMsg->SetId(NETMSG_SAYTEXT, message);
	else if (strcmp(name, "BotVoice") == 0)
		g_netMsg->SetId(NETMSG_BOTVOICE, message);
	else if (strcmp(name, "ResetHUD") == 0)
		g_netMsg->SetId(NETMSG_BOTVOICE, message);

	return message;
}

void pfnAlertMessage(ALERT_TYPE alertType, char* format, ...)
{
	va_list ap;
	char buffer[1024];

	va_start(ap, format);
	vsprintf(buffer, format, ap);
	va_end(ap);

	if (strstr(buffer, "_Defuse_") != null)
	{
		// notify all terrorists that CT is starting bomb defusing
		for (int i = 0; i < engine->GetMaxClients(); i++)
		{
			Bot* bot = g_botManager->GetBot(i);

			if (bot != null && GetTeam(bot->GetEntity()) == TEAM_TERRORIST && IsAlive(bot->GetEntity()))
			{
				bot->ResetTasks();
				bot->MoveToVector(g_waypoint->GetBombPosition());
			}
		}
	}

	if (g_isMetamod)
		RETURN_META(MRES_IGNORED);

	(*g_engfuncs.pfnAlertMessage) (alertType, buffer);
}

const char* pfnGetPlayerAuthId(edict_t* e)
{
	/*if (IsValidBot(e))
	{
		if (g_isMetamod)
			RETURN_META_VALUE(MRES_SUPERCEDE, "BOT");

		return "BOT";
	}*/

	if (g_isMetamod)
		RETURN_META_VALUE(MRES_IGNORED, 0);

	return (*g_engfuncs.pfnGetPlayerAuthId) (e);
}

unsigned int pfnGetPlayerWONId(edict_t* e)
{
	/*if (IsValidBot(e))
	{
		if (g_isMetamod)
			RETURN_META_VALUE(MRES_SUPERCEDE, 0);

		return 0;
	}*/

	if (g_isMetamod)
		RETURN_META_VALUE(MRES_IGNORED, 0);

	return (*g_engfuncs.pfnGetPlayerWONId) (e);
}

gamedll_funcs_t gameDLLFunc;

export int GetEntityAPI2(DLL_FUNCTIONS* functionTable, int* /*interfaceVersion*/)
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

	memset(functionTable, 0, sizeof(DLL_FUNCTIONS));

	if (!g_isMetamod)
	{
		// pass other DLLs engine callbacks to function table...
		if ((*g_entityAPI) (&g_functionTable, 140) == 0)
		{
			AddLogEntry(LOG_FATAL, "GetEntityAPI2: ERROR - Not Initialized.");
			return false;  // error initializing function table!!!
		}

		gameDLLFunc.dllapi_table = &g_functionTable;
		gpGamedllFuncs = &gameDLLFunc;

		memcpy(functionTable, &g_functionTable, sizeof(DLL_FUNCTIONS));
	}

	functionTable->pfnGameInit = GameDLLInit;
	functionTable->pfnSpawn = Spawn;
	functionTable->pfnClientConnect = ClientConnect;
	functionTable->pfnClientDisconnect = ClientDisconnect;
	functionTable->pfnClientUserInfoChanged = ClientUserInfoChanged;
	functionTable->pfnClientCommand = ClientCommand;
	functionTable->pfnServerActivate = ServerActivate;
	functionTable->pfnServerDeactivate = ServerDeactivate;
	functionTable->pfnKeyValue = KeyValue;
	functionTable->pfnStartFrame = StartFrame;
	functionTable->pfnTouch = Touch;

	return true;
}

export int GetEntityAPI2_Post(DLL_FUNCTIONS* functionTable, int* /*interfaceVersion*/)
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

	memset(functionTable, 0, sizeof(DLL_FUNCTIONS));

	functionTable->pfnSpawn = Spawn_Post;
	functionTable->pfnServerActivate = ServerActivate_Post;
	functionTable->pfnStartFrame = StartFrame_Post;
	functionTable->pfnGameInit = GameDLLInit_Post;

	return true;
}

export int GetNewDLLFunctions(NEW_DLL_FUNCTIONS* functionTable, int* interfaceVersion)
{
	// it appears that an extra function table has been added in the engine to gamedll interface
	// since the date where the first enginefuncs table standard was frozen. These ones are
	// facultative and we don't hook them, but since some MODs might be featuring it, we have to
	// pass them too, else the DLL interfacing wouldn't be complete and the game possibly wouldn't
	// run properly.

	if (g_getNewEntityAPI == null)
		return false;

	if (!(*g_getNewEntityAPI) (functionTable, interfaceVersion))
	{
		AddLogEntry(LOG_FATAL, "GetNewDLLFunctions: ERROR - Not Initialized.");
		return false;
	}

	gameDLLFunc.newapi_table = functionTable;
	return true;
}

export int GetEngineFunctions_Post(enginefuncs_t* functionTable, int* /*interfaceVersion*/)
{
	if (g_isMetamod)
		memset(functionTable, 0, sizeof(enginefuncs_t));

	functionTable->pfnRegUserMsg = pfnRegUserMsg_Post;

	return true;
}

export int GetEngineFunctions(enginefuncs_t* functionTable, int* /*interfaceVersion*/)
{
	if (g_isMetamod)
		memset(functionTable, 0, sizeof(enginefuncs_t));

	functionTable->pfnChangeLevel = pfnChangeLevel;
	functionTable->pfnFindEntityByString = pfnFindEntityByString;
	functionTable->pfnEmitSound = pfnEmitSound;
	functionTable->pfnClientCommand = pfnClientCommand;
	functionTable->pfnMessageBegin = pfnMessageBegin;
	functionTable->pfnMessageEnd = pfnMessageEnd;
	functionTable->pfnWriteByte = pfnWriteByte;
	functionTable->pfnWriteChar = pfnWriteChar;
	functionTable->pfnWriteShort = pfnWriteShort;
	functionTable->pfnWriteLong = pfnWriteLong;
	functionTable->pfnWriteAngle = pfnWriteAngle;
	functionTable->pfnWriteCoord = pfnWriteCoord;
	functionTable->pfnWriteString = pfnWriteString;
	functionTable->pfnWriteEntity = pfnWriteEntity;
	functionTable->pfnRegUserMsg = pfnRegUserMsg;
	functionTable->pfnClientPrintf = pfnClientPrintf;
	functionTable->pfnCmd_Args = pfnCmd_Args;
	functionTable->pfnCmd_Argv = pfnCmd_Argv;
	functionTable->pfnCmd_Argc = pfnCmd_Argc;
	functionTable->pfnSetClientMaxspeed = pfnSetClientMaxspeed;
	functionTable->pfnAlertMessage = pfnAlertMessage;
	functionTable->pfnGetPlayerAuthId = pfnGetPlayerAuthId;
	functionTable->pfnGetPlayerWONId = pfnGetPlayerWONId;

	return true;
}

export int Server_GetBlendingInterface(int version, void** ppinterface, void* pstudio, float(*rotationmatrix)[3][4], float(*bonetransform)[128][3][4])
{
	// this function synchronizes the studio model animation blending interface (i.e, what parts
	// of the body move, which bones, which hitboxes and how) between the server and the game DLL.
	// some MODs can be using a different hitbox scheme than the standard one.

	if (g_serverBlendingAPI == null)
		return false;

	return (*g_serverBlendingAPI) (version, ppinterface, pstudio, rotationmatrix, bonetransform);
}

export int Meta_Query(char* ifvers, plugin_info_t** pPlugInfo, mutil_funcs_t* pMetaUtilFuncs)
{
	// this function is the first function ever called by metamod in the plugin DLL. Its purpose
	// is for metamod to retrieve basic information about the plugin, such as its meta-interface
	// version, for ensuring compatibility with the current version of the running metamod.

	// keep track of the pointers to metamod function tables metamod gives us
	gpMetaUtilFuncs = pMetaUtilFuncs;
	*pPlugInfo = &Plugin_info;

	// check for interface version compatibility
	if (strcmp(ifvers, Plugin_info.ifvers) != 0)
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

export int Meta_Attach(PLUG_LOADTIME now, metamod_funcs_t* functionTable, meta_globals_t* pMGlobals, gamedll_funcs_t* pGamedllFuncs)
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
	memcpy(functionTable, &gMetaFunctionTable, sizeof(metamod_funcs_t));
	gpGamedllFuncs = pGamedllFuncs;

	return true; // returning true enables metamod to attach this plugin
}

export int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason)
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

export void Meta_Init(void)
{
	// this function is called by metamod, before any other interface functions. Purpose of this
	// function to give plugin a chance to determine is plugin running under metamod or not.

	g_isMetamod = true;
}

// SyPB Pro P.30 - AMXX API
export bool Amxx_Runebot(void)
{
	API_Version = float(SUPPORT_API_VERSION_F); // ebot API_P
	API_TestMSG("Amxx_Runebot Checking - Done");
	return true;
}

export float Amxx_APIVersion(void)
{
	API_TestMSG("Amxx_APIVersion Checking - API Version:%.2f - Done", API_Version);
	return API_Version;
}

export void Amxx_Check_amxxdllversion(float version, int bu1, int bu2, int bu3, int bu4)
{
	amxxDLL_Version = version;
	amxxDLL_bV16[0] = (uint16)bu1;
	amxxDLL_bV16[1] = (uint16)bu2;
	amxxDLL_bV16[2] = (uint16)bu3;
	amxxDLL_bV16[3] = (uint16)bu4;
}

export int Amxx_Isebot(int index) // 1.30
{
	index -= 1;
	API_TestMSG("Amxx_Isebot Checking - Done");
	return (g_botManager->GetBot(index) != null);
}

export int Amxx_CheckEnemy(int index) // 1.30
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	API_TestMSG("Amxx_CheckEnemy Checking - Done");
	return (bot->m_enemy == null) ? -1 : (ENTINDEX(bot->m_enemy));
}

export int Amxx_CheckMoveTarget(int index) // 1.30
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	API_TestMSG("Amxx_CheckMoveTarget Checking - Done");
	return (bot->m_moveTargetEntity == null) ? -1 : (ENTINDEX(bot->m_moveTargetEntity));
}

export int Amxx_SetEnemy(int index, int target, float blockCheckTime) // 1.30
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	edict_t* targetEnt = INDEXENT(target);
	if (target == -1 || FNullEnt(targetEnt) || !IsAlive(targetEnt))
	{
		bot->m_blockCheckEnemyTime = engine->GetTime();
		bot->m_enemyAPI = null;
		API_TestMSG("Amxx_SetEnemy Checking - targetName:%s | blockCheckTime:%.2f - Done",
			"Not target", blockCheckTime);

		return -1;
	}

	bot->m_blockCheckEnemyTime = engine->GetTime() + blockCheckTime;
	bot->m_enemyAPI = targetEnt;
	API_TestMSG("Amxx_SetEnemy Checking - targetName:%s | blockCheckTime:%.2f - Done",
		GetEntityName(targetEnt), blockCheckTime);

	return 1;
}

export int Amxx_SetBotMove(int index, int pluginSet) // 1.30
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	bot->m_moveAIAPI = pluginSet <= 0 ? false : true;
	API_TestMSG("Amxx_SetBotMove Checking - %d - Done", bot->m_moveAIAPI);
	return 1;
}

export int Amxx_SetBotLookAt(int index, Vector lookAt) // 1.30
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	bot->m_lookAtAPI = lookAt;
	API_TestMSG("Amxx_SetBotLookAt Checking - %.2f | %.2f | %.2f - Done", lookAt[0], lookAt[1], lookAt[2]);
	return 1;
}

export int Amxx_SetWeaponClip(int index, int weaponClip) // 1.30
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	bot->m_weaponClipAPI = weaponClip;
	API_TestMSG("Amxx_SetWeaponClip Checking - %d - Done", bot->m_weaponClipAPI);
	return 1;
}

export int Amxx_BlockWeaponReload(int index, int blockWeaponReload) // 1.30
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	bot->m_weaponReloadAPI = (blockWeaponReload == 0) ? false : true;
	API_TestMSG("Amxx_BlockWeaponReload Checking - %s - Done", (bot->m_weaponReloadAPI) ? "True" : "False");
	return 1;
}

// SyPB Pro P.31 - AMXX API
export int Amxx_SetKADistance(int index, int k1d, int k2d) // 1.31
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	bot->m_knifeDistance1API = k1d;
	bot->m_knifeDistance2API = k2d;

	API_TestMSG("Amxx_Set_KA_Distance Checking - %d %d - Done", k1d, k2d);
	return 1;
}


// SyPB Pro P.34 - AMXX API
export int Amxx_Addebot(const char* name, int skill, int team) // 1.34
{
	int botId = g_botManager->AddBotAPI(name, skill, team);

	if (botId < 0)
	{
		API_TestMSG("Amxx_Addebot Checking - Fail");
		return -1;
	}

	API_TestMSG("Amxx_Addebot Checking - %s | Skill:%d  | Team:%d - Done", name, skill, team);
	return botId + 1;
}

// SyPB Pro P.35 - AMXX API
export int Amxx_SetGunADistance(int index, int minDistance, int maxDistance)  // 1.35
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	bot->m_gunMinDistanceAPI = minDistance;
	bot->m_gunMaxDistanceAPI = maxDistance;

	API_TestMSG("Amxx_Set_GunA_Distance Checkimh - min:%d max:%d - Done", minDistance, maxDistance);
	return 1;
}

// SyPB Pro P.38 - AMXX API
export int Amxx_IsZombieBot(int index)
{
	edict_t* player = INDEXENT(index);

	if (FNullEnt(player))
		return -2;

	API_TestMSG("Amxx_IsZombieBot Checking - Done - %d - %s -", index, GetEntityName(player));
	if (IsZombieEntity(player))
		return 1;

	return 0;
}

export int Amxx_SetZombieBot(int index, int zombieBot)
{
	index -= 1;
	if (FNullEnt(g_clients[index].ent))
		return -2;

	if (zombieBot > 0)
		g_clients[index].isZombiePlayerAPI = 1;
	else if (zombieBot == 0)
		g_clients[index].isZombiePlayerAPI = 0;
	else
		g_clients[index].isZombiePlayerAPI = -1;

	API_TestMSG("Amxx_SetZombieBot Checking - ZombieBot:%s - Done - %s -",
		g_clients[index].isZombiePlayerAPI == 1 ? "True" : (g_clients[index].isZombiePlayerAPI == -1 ? "Not Set" : "False"),
		GetEntityName(g_clients[index].ent));

	return 1;
}

export int Amxx_GetOriginPoint(Vector origin) // 1.38
{
	int pointId = g_waypoint->FindNearest(origin);

	API_TestMSG("Amxx_GetOriginPoint Checking - %.2f | %.2f | %.2f", origin[0], origin[1], origin[2]);
	API_TestMSG("Amxx_GetOriginPoint Checking - point:%d - Done", pointId);

	if (pointId >= 0 && pointId < g_numWaypoints)
		return pointId;

	return -1;
}

export int Amxx_GetBotPoint(int index, int mod) // 1.38
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	int pointId = bot->CheckBotPointAPI(mod);
	API_TestMSG("Amxx_GetBotPoint Checking - pointId:%d - Done", pointId);

	if (pointId >= 0 && pointId < g_numWaypoints)
		return pointId;

	return -1;
}

// SyPB Pro P.40 - AMXX API
export int Amxx_GetBotNavNum(int index) // 1.40
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	int navNum = bot->GetNavData(-1);
	API_TestMSG("Amxx_GetBotNavNum Checking - navNum:%d - Done", navNum);

	return navNum;
}

export int Amxx_GetBotNavPointId(int index, int pointNum) // 1.40
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	int navId = bot->GetNavData(pointNum);
	API_TestMSG("Amxx_GetBotNavPointId Checking - navNum:%d navId:%d - Done", pointNum, navId);
	return navId;
}

export int Amxx_SetEntityAction(int index, int team, int action) // 1.40
{
	// SyPB Pro P.42 - AMXX API Entity Action
	int i;
	if (index == -1)
	{
		for (i = 0; i < entityNum; i++)
			SetEntityActionData(i);

		API_TestMSG("Amxx_SetEntityAction Checking - Del All - Done");
		return 1;
	}

	edict_t* entity = INDEXENT(index);
	if (FNullEnt(entity) || !IsAlive(entity))
	{
		API_TestMSG("Amxx_SetEntityAction Checking - Not work entity - Done");
		return -1;
	}

	if (IsValidPlayer(entity))
	{
		API_TestMSG("Amxx_SetEntityAction Checking - Cannot Set the player - Done");
		return -1;
	}

	if (action == -1)
	{
		index = -1;
		team = -1;
	}

	for (i = 0; i < entityNum; i++)
	{
		if (g_entityId[i] == index)
		{
			if (action != -1)
				API_TestMSG("Amxx_SetEntityAction Checking - Change Id:%d Team:%d Action:%d - Done", index, team, action);
			else
				API_TestMSG("Amxx_SetEntityAction Checking - Del Id:%d - Done", index);

			SetEntityActionData(i, index, team, action);

			return 1;
		}
	}

	if (action == -1)
	{
		API_TestMSG("Amxx_SetEntityAction Checking - Cannot Del Id:%d - Not Find - Done", index);
		return -1;
	}

	for (i = 0; i < entityNum; i++)
	{
		if (g_entityId[i] == -1)
		{
			SetEntityActionData(i, index, team, action);
			API_TestMSG("Amxx_SetEntityAction Checking - Add Id:%d Team:%d Action:%d - Done", index, team, action);
			return 1;
		}
	}

	API_TestMSG("Amxx_SetEntityAction Checking - Cannot Add Entity Action - Unknow problem - Done");
	return 1;
}

// SyPB Pro P.42 - AMXX API
export void Amxx_AddLog(char* logText) // 1.42 - Not API
{
	MOD_AddLogEntry(0, logText);
}

export int Amxx_SetBotGoal(int index, int goal) // 1.42
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	if (goal < 0 || goal >= g_numWaypoints)
		return -1;

	API_TestMSG("Amxx_SetBotGoal Checking - Index:%d[%s] - goal:%d - Done", index, GetEntityName(bot->GetEntity()), goal);
	bot->m_waypointGoalAPI = goal;
	return 1;
}

export int Amxx_BlockWeaponPick(int index, int blockWeaponPick) // 1.42
{
	index -= 1;
	Bot* bot = g_botManager->GetBot(index);
	if (bot == null)
		return -2;

	if (IsZombieEntity(bot->GetEntity()))
	{
		bot->m_blockWeaponPickAPI = false;
		API_TestMSG("Amxx_BlockWeaponPick Checking - Index:%d[%s] - Zombie Bot cannot set this - Done",
			index, GetEntityName(bot->GetEntity()));
		return -1;
	}

	bot->m_blockWeaponPickAPI = (blockWeaponPick == 1) ? true : false;
	API_TestMSG("Amxx_BlockWeaponPick Checking - Index:%d[%s] - Block:%s - Done",
		index, GetEntityName(bot->GetEntity()),
		bot->m_blockWeaponPickAPI ? "Blocking" : "None (Base Mode)");
	return 1;
}

// SyPB Pro P.48 - AMXX API
export int Amxx_GetEntityWaypointId(int index) // 1.48
{
	edict_t* entity = INDEXENT(index);
	if (!IsAlive(entity) || !IsValidPlayer(entity))
		return -2;

	API_TestMSG("Amxx_GetEntityWaypointId Checking - [%s] - Done", GetEntityName(entity));

	return GetEntityWaypoint(entity);
}
// AMXX ebot API End

// SyPB Pro P.42 - SwNPC API
export void SwNPC_GetHostEntity(edict_t** hostEntity)
{
	if (!IsDedicatedServer() && !FNullEnt(g_hostEntity))
	{
		*hostEntity = g_hostEntity;
		return;
	}

	*hostEntity = null;
}

export float SwNPC_ebotSupportVersion(void)
{
	return float(SUPPORT_SWNPC_VERSION_F);
}

export void SwNPC_CheckBuild(float version, int bu1, int bu2, int bu3, int bu4)
{
	SwNPC_Version = version;
	SwNPC_Build[0] = (uint16)bu1;
	SwNPC_Build[1] = (uint16)bu2;
	SwNPC_Build[2] = (uint16)bu3;
	SwNPC_Build[3] = (uint16)bu4;
}

export void SwNPC_AddLog(char* logText)
{
	MOD_AddLogEntry(1, logText);
}

export int SwNPC_GetWaypointData(Vector** origin, float** radius, int32** flags, int16*** index, uint16*** cnFlags, int32*** cnDistance)
{
	int numWaypoints = 0;
	Vector wpOrigin[Const_MaxWaypoints];
	float wpRadius[Const_MaxWaypoints];
	int32 wpFlags[Const_MaxWaypoints];
	int16 wpCnIndex[Const_MaxWaypoints][Const_MaxPathIndex];
	uint16 wpCnFlags[Const_MaxWaypoints][Const_MaxPathIndex];
	int32 wpCnDistance[Const_MaxWaypoints][Const_MaxPathIndex];

	for (int i = 0; i < Const_MaxWaypoints; i++)
	{
		if (i >= 0 && i < g_numWaypoints)
		{
			wpOrigin[i] = g_waypoint->GetPath(i)->origin;
			wpRadius[i] = g_waypoint->GetPath(i)->radius;
			wpFlags[i] = g_waypoint->GetPath(i)->flags;

			for (int j = 0; j < Const_MaxPathIndex; j++)
			{
				wpCnIndex[i][j] = g_waypoint->GetPath(i)->index[j];
				wpCnFlags[i][j] = g_waypoint->GetPath(i)->connectionFlags[j];
				wpCnDistance[i][j] = g_waypoint->GetPath(i)->distances[j];
			}

			numWaypoints++;
			continue;
		}

		wpOrigin[i] = nullvec;
		wpRadius[i] = -1.0f;
		wpFlags[i] = -1;

		for (int j = 0; j < Const_MaxPathIndex; j++)
		{
			wpCnIndex[i][j] = -1;
			wpCnFlags[i][j] = 0;
			wpCnDistance[i][j] = 9999999;
		}
	}

	*origin = wpOrigin;
	*radius = wpRadius;
	*flags = wpFlags;

	int16* wpCnIndexRam[Const_MaxWaypoints];
	uint16* wpCnFlagsRam[Const_MaxWaypoints];
	int32* wpCnDistanceRam[Const_MaxWaypoints];

	for (int i = 0; i < Const_MaxWaypoints; i++)
	{
		wpCnIndexRam[i] = wpCnIndex[i];
		wpCnFlagsRam[i] = wpCnFlags[i];
		wpCnDistanceRam[i] = wpCnDistance[i];
	}

	*index = wpCnIndexRam;
	*cnFlags = wpCnFlagsRam;
	*cnDistance = wpCnDistanceRam;

	return numWaypoints;
}

export int SwNPC_GetEntityWaypointIndex(edict_t* entity)
{
	return GetEntityWaypoint(entity);
}

export void SwNPC_LoadEntityWaypointIndex(edict_t* getEntity, edict_t* targetEntity)
{
	if (FNullEnt(targetEntity))
		SetEntityWaypoint(getEntity);
	else
		SetEntityWaypoint(getEntity, GetEntityWaypoint(targetEntity));
}
// SwNPC API End

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

	// initialize random generator
	engine->InitFastRNG();

	static struct ModSupport_t
	{
		char name[32];
		char linuxLib[32];
		char winLib[32];
		char desc[256];
		int modType;
	} s_supportedMods[] =
	{
	   {"cstrike", "cs_i386.so", "mp.dll", "Counter-Strike v1.6+", CSVER_CSTRIKE },
	   {"czero", "cs_i386.so", "mp.dll", "Counter-Strike: Condition Zero", CSVER_CZERO},
	   {"csv15", "cs_i386.so", "mp.dll", "CS 1.5 for Steam", CSVER_VERYOLD},
	   {"cs13", "cs_i386.so", "mp.dll", "Counter-Strike v1.3", CSVER_VERYOLD}, // assume cs13 = cs15
	   {"retrocs", "rcs_i386.so", "rcs.dll", "Retro Counter-Strike", CSVER_VERYOLD},
	   {"", "", "", "", 0}
	};

	// get the engine functions from the engine...
	memcpy(&g_engfuncs, functionTable, sizeof(enginefuncs_t));
	g_pGlobals = pGlobals;


	ModSupport_t* knownMod = null;
	char gameDLLName[256];

	for (int i = 0; s_supportedMods[i].name; i++)
	{
		ModSupport_t* mod = &s_supportedMods[i];

		if (strcmp(mod->name, GetModName()) == 0)
		{
			knownMod = mod;
			break;
		}
	}

	if (knownMod != null)
	{
		g_gameVersion = knownMod->modType;

		if (g_isMetamod)
			return; // we should stop the attempt for loading the real gamedll, since metamod handle this for us

		sprintf(gameDLLName, "%s/dlls/%s", knownMod->name, !IsLinux() ? knownMod->winLib : knownMod->linuxLib);
		g_gameLib = new Library(gameDLLName);

		if ((g_gameLib == null || (g_gameLib && !g_gameLib->IsLoaded())))
		{
			// try to extract the game dll out of the steam cache
			AddLogEntry(LOG_WARNING | LOG_IGNORE, "Trying to extract dll '%s' out of the steam cache", gameDLLName);

			int size;
			uint8_t* buffer = (*g_engfuncs.pfnLoadFileForMe) (gameDLLName, &size);

			if (buffer)
			{
				CreatePath(FormatBuffer("%s/dlls", GetModName()));
				File fp(gameDLLName, "wb");

				if (fp.IsValid())
				{
					// dump the game dll file and then close it
					fp.Write(buffer, size);
					fp.Close();
				}
				FREE_FILE(buffer);
			}
			g_gameLib = new Library(gameDLLName);
		}
	}
	else
		AddLogEntry(LOG_FATAL | LOG_IGNORE, "Mod that you has started, not supported by this bot (gamedir: %s)", GetModName());

	g_funcPointers = (FuncPointers_t)g_gameLib->GetFunctionAddr("GiveFnptrsToDll");
	g_entityAPI = (EntityAPI_t)g_gameLib->GetFunctionAddr("GetEntityAPI");
	g_getNewEntityAPI = (NewEntityAPI_t)g_gameLib->GetFunctionAddr("GetNewDLLFunctions");
	g_serverBlendingAPI = (BlendAPI_t)g_gameLib->GetFunctionAddr("Server_GetBlendingInterface");

	if (!g_funcPointers || !g_entityAPI || !g_getNewEntityAPI || !g_serverBlendingAPI)
		return;

	GetEngineFunctions(functionTable, null);

	// give the engine functions to the other DLL...
	(*g_funcPointers) (functionTable, pGlobals);
}

DLL_ENTRYPOINT
{
	// dynamic library entry point, can be used for uninitialization stuff. NOT for initializing
	// anything because if you ever attempt to wander outside the scope of this function on a
	// DLL attach, LoadLibrary() will simply fail. And you can't do I/Os here either. Nice eh ?

	// dynamic library detaching ??
	if (DLL_DETACHING)
	{
	   FreeLibraryMemory(); // free everything that's freeable
	   if (g_gameLib != null)
		  delete g_gameLib; // if dynamic link library of mod is load, free it
	}
	DLL_RETENTRY; // the return data type is OS specific too
}

#define LINK_ENTITY(entityFunction) \
export void entityFunction (entvars_t *pev); \
\
void entityFunction (entvars_t *pev) \
{ \
   static EntityPtr_t funcPtr = null; \
   \
   if (funcPtr == null) \
      funcPtr = (EntityPtr_t) g_gameLib->GetFunctionAddr (#entityFunction); \
   \
   if (funcPtr == null) \
      return; \
   \
   (*funcPtr) (pev); \
} \

// entities in counter-strike...
LINK_ENTITY(DelayedUse)
LINK_ENTITY(ambient_generic)
LINK_ENTITY(ammo_338magnum)
LINK_ENTITY(ammo_357sig)
LINK_ENTITY(ammo_45acp)
LINK_ENTITY(ammo_50ae)
LINK_ENTITY(ammo_556nato)
LINK_ENTITY(ammo_556natobox)
LINK_ENTITY(ammo_57mm)
LINK_ENTITY(ammo_762nato)
LINK_ENTITY(ammo_9mm)
LINK_ENTITY(ammo_buckshot)
LINK_ENTITY(armoury_entity)
LINK_ENTITY(beam)
LINK_ENTITY(bodyque)
LINK_ENTITY(button_target)
LINK_ENTITY(cycler)
LINK_ENTITY(cycler_prdroid)
LINK_ENTITY(cycler_sprite)
LINK_ENTITY(cycler_weapon)
LINK_ENTITY(cycler_wreckage)
LINK_ENTITY(env_beam)
LINK_ENTITY(env_beverage)
LINK_ENTITY(env_blood)
LINK_ENTITY(env_bombglow)
LINK_ENTITY(env_bubbles)
LINK_ENTITY(env_debris)
LINK_ENTITY(env_explosion)
LINK_ENTITY(env_fade)
LINK_ENTITY(env_funnel)
LINK_ENTITY(env_global)
LINK_ENTITY(env_glow)
LINK_ENTITY(env_laser)
LINK_ENTITY(env_lightning)
LINK_ENTITY(env_message)
LINK_ENTITY(env_rain)
LINK_ENTITY(env_render)
LINK_ENTITY(env_shake)
LINK_ENTITY(env_shooter)
LINK_ENTITY(env_snow)
LINK_ENTITY(env_sound)
LINK_ENTITY(env_spark)
LINK_ENTITY(env_sprite)
LINK_ENTITY(fireanddie)
LINK_ENTITY(func_bomb_target)
LINK_ENTITY(func_breakable)
LINK_ENTITY(func_button)
LINK_ENTITY(func_buyzone)
LINK_ENTITY(func_conveyor)
LINK_ENTITY(func_door)
LINK_ENTITY(func_door_rotating)
LINK_ENTITY(func_escapezone)
LINK_ENTITY(func_friction)
LINK_ENTITY(func_grencatch)
LINK_ENTITY(func_guntarget)
LINK_ENTITY(func_healthcharger)
LINK_ENTITY(func_hostage_rescue)
LINK_ENTITY(func_illusionary)
LINK_ENTITY(func_ladder)
LINK_ENTITY(func_monsterclip)
LINK_ENTITY(func_mortar_field)
LINK_ENTITY(func_pendulum)
LINK_ENTITY(func_plat)
LINK_ENTITY(func_platrot)
LINK_ENTITY(func_pushable)
LINK_ENTITY(func_rain)
LINK_ENTITY(func_recharge)
LINK_ENTITY(func_rot_button)
LINK_ENTITY(func_rotating)
LINK_ENTITY(func_snow)
LINK_ENTITY(func_tank)
LINK_ENTITY(func_tankcontrols)
LINK_ENTITY(func_tanklaser)
LINK_ENTITY(func_tankmortar)
LINK_ENTITY(func_tankrocket)
LINK_ENTITY(func_trackautochange)
LINK_ENTITY(func_trackchange)
LINK_ENTITY(func_tracktrain)
LINK_ENTITY(func_train)
LINK_ENTITY(func_traincontrols)
LINK_ENTITY(func_vehicle)
LINK_ENTITY(func_vehiclecontrols)
LINK_ENTITY(func_vip_safetyzone)
LINK_ENTITY(func_wall)
LINK_ENTITY(func_wall_toggle)
LINK_ENTITY(func_water)
LINK_ENTITY(func_weaponcheck)
LINK_ENTITY(game_counter)
LINK_ENTITY(game_counter_set)
LINK_ENTITY(game_end)
LINK_ENTITY(game_player_equip)
LINK_ENTITY(game_player_hurt)
LINK_ENTITY(game_player_team)
LINK_ENTITY(game_score)
LINK_ENTITY(game_team_master)
LINK_ENTITY(game_team_set)
LINK_ENTITY(game_text)
LINK_ENTITY(game_zone_player)
LINK_ENTITY(gibshooter)
LINK_ENTITY(grenade)
LINK_ENTITY(hostage_entity)
LINK_ENTITY(info_bomb_target)
LINK_ENTITY(info_hostage_rescue)
LINK_ENTITY(info_intermission)
LINK_ENTITY(info_landmark)
LINK_ENTITY(info_map_parameters)
LINK_ENTITY(info_null)
LINK_ENTITY(info_player_deathmatch)
LINK_ENTITY(info_player_start)
LINK_ENTITY(info_target)
LINK_ENTITY(info_teleport_destination)
LINK_ENTITY(info_vip_start)
LINK_ENTITY(infodecal)
LINK_ENTITY(item_airtank)
LINK_ENTITY(item_antidote)
LINK_ENTITY(item_assaultsuit)
LINK_ENTITY(item_battery)
LINK_ENTITY(item_healthkit)
LINK_ENTITY(item_kevlar)
LINK_ENTITY(item_longjump)
LINK_ENTITY(item_security)
LINK_ENTITY(item_sodacan)
LINK_ENTITY(item_suit)
LINK_ENTITY(item_thighpack)
LINK_ENTITY(light)
LINK_ENTITY(light_environment)
LINK_ENTITY(light_spot)
LINK_ENTITY(momentary_door)
LINK_ENTITY(momentary_rot_button)
LINK_ENTITY(monster_hevsuit_dead)
LINK_ENTITY(monster_mortar)
LINK_ENTITY(monster_scientist)
LINK_ENTITY(multi_manager)
LINK_ENTITY(multisource)
LINK_ENTITY(path_corner)
LINK_ENTITY(path_track)
LINK_ENTITY(player)
LINK_ENTITY(player_loadsaved)
LINK_ENTITY(player_weaponstrip)
LINK_ENTITY(soundent)
LINK_ENTITY(spark_shower)
LINK_ENTITY(speaker)
LINK_ENTITY(target_cdaudio)
LINK_ENTITY(test_effect)
LINK_ENTITY(trigger)
LINK_ENTITY(trigger_auto)
LINK_ENTITY(trigger_autosave)
LINK_ENTITY(trigger_camera)
LINK_ENTITY(trigger_cdaudio)
LINK_ENTITY(trigger_changelevel)
LINK_ENTITY(trigger_changetarget)
LINK_ENTITY(trigger_counter)
LINK_ENTITY(trigger_endsection)
LINK_ENTITY(trigger_gravity)
LINK_ENTITY(trigger_hurt)
LINK_ENTITY(trigger_monsterjump)
LINK_ENTITY(trigger_multiple)
LINK_ENTITY(trigger_once)
LINK_ENTITY(trigger_push)
LINK_ENTITY(trigger_relay)
LINK_ENTITY(trigger_teleport)
LINK_ENTITY(trigger_transition)
LINK_ENTITY(weapon_ak47)
LINK_ENTITY(weapon_aug)
LINK_ENTITY(weapon_awp)
LINK_ENTITY(weapon_c4)
LINK_ENTITY(weapon_deagle)
LINK_ENTITY(weapon_elite)
LINK_ENTITY(weapon_famas)
LINK_ENTITY(weapon_fiveseven)
LINK_ENTITY(weapon_flashbang)
LINK_ENTITY(weapon_g3sg1)
LINK_ENTITY(weapon_galil)
LINK_ENTITY(weapon_glock18)
LINK_ENTITY(weapon_hegrenade)
LINK_ENTITY(weapon_knife)
LINK_ENTITY(weapon_m249)
LINK_ENTITY(weapon_m3)
LINK_ENTITY(weapon_m4a1)
LINK_ENTITY(weapon_mac10)
LINK_ENTITY(weapon_mp5navy)
LINK_ENTITY(weapon_p228)
LINK_ENTITY(weapon_p90)
LINK_ENTITY(weapon_scout)
LINK_ENTITY(weapon_sg550)
LINK_ENTITY(weapon_sg552)
LINK_ENTITY(weapon_shield)
LINK_ENTITY(weapon_shieldgun)
LINK_ENTITY(weapon_smokegrenade)
LINK_ENTITY(weapon_tmp)
LINK_ENTITY(weapon_ump45)
LINK_ENTITY(weapon_usp)
LINK_ENTITY(weapon_xm1014)
LINK_ENTITY(weaponbox)
LINK_ENTITY(world_items)
LINK_ENTITY(worldspawn)