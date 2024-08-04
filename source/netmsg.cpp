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

#define PTR_TO_BYTE(in) *reinterpret_cast<uint8_t*>(in)
#define PTR_TO_FLT(in) *reinterpret_cast<float*>(in)
#define PTR_TO_INT(in) *reinterpret_cast<int*>(in)
#define PTR_TO_STR(in) reinterpret_cast<char*>(in)

NetworkMsg::NetworkMsg(void)
{
    m_message = NETMSG_UNDEFINED;
    m_state = 0;
    m_bot = nullptr;

    for (auto& message : m_registerdMessages)
        message = NETMSG_UNDEFINED;
}

void NetworkMsg::HandleMessageIfRequired(const int messageType, const int requiredType)
{
    if (messageType == m_registerdMessages[requiredType])
        SetMessage(requiredType);
}

void NetworkMsg::Execute(void* p)
{
    if (m_message == NETMSG_UNDEFINED)
        return; // no message or not for bot, return

    // some needed variables
    static uint8_t r = 0, g = 0, b = 0;
    static uint8_t enabled = 0;

    static int index = 0, numPlayers = 0;
    static int state = 0, id = 0;
    static int killerIndex = 0, victimIndex = 0;

    static WeaponProperty weaponProp{};
    static char* x;

    // now starts of netmessage execution
    switch (m_message)
    {
    case NETMSG_VGUI:
    {
        // this message is sent when a VGUI menu is displayed
        if (!m_state && m_bot)
        {
            switch (PTR_TO_INT(p))
            {
            case GMENU_TEAM:
            {
                m_bot->m_startAction = CMENU_TEAM;
                break;
            }
            case GMENU_TERRORIST:
            case GMENU_COUNTER:
            {
                m_bot->m_startAction = CMENU_CLASS;
                break;
            }
            }
        }
        break;
    }
    case NETMSG_SHOWMENU:
    {
        // this message is sent when a text menu is displayed
        if (m_state < 3) // ignore first 3 fields of message
            break;

        if (m_bot)
        {
            x = PTR_TO_STR(p);
            if (!x)
                break;

            switch (x[1])
            {
            case 'T':
            {
                switch (charToInt(x))
                {
                case 1010:
                {
                    m_bot->m_startAction = CMENU_TEAM;
                    break;
                }
                case 1616:
                {
                    m_bot->m_startAction = CMENU_TEAM;
                    break;
                }
                case 1593:
                {
                    m_bot->m_startAction = CMENU_CLASS;
                    break;
                }
                }
                break;
            }
            case 'I':
            {
                switch (charToInt(x))
                {
                case 1866:
                {
                    m_bot->m_startAction = CMENU_TEAM;
                    break;
                }
                case 1260:
                {
                    m_bot->m_startAction = CMENU_TEAM;
                    break;
                }
                case 1594:
                {
                    m_bot->m_startAction = CMENU_TEAM;
                    break;
                }
                case 2200:
                {
                    m_bot->m_startAction = CMENU_TEAM;
                    break;
                }
                }
                break;
            }
            case 'C':
            {
                if (charToInt(x) == 787)
                {
                    m_bot->m_startAction = CMENU_CLASS;
                    break;
                }
                break;
            }
            }

            /*if (cstrncmp(x, "#Team_Select", 13) == 0) // team select menu?
                m_bot->m_startAction = CMENU_TEAM;
            else if (cstrncmp(x, "#Team_Select_Spect", 19) == 0) // team select menu?
                m_bot->m_startAction = CMENU_TEAM;
            else if (cstrncmp(x, "#IG_Team_Select_Spect", 22) == 0) // team select menu?
                m_bot->m_startAction = CMENU_TEAM;
            else if (cstrncmp(x, "#IG_Team_Select", 16) == 0) // team select menu?
                m_bot->m_startAction = CMENU_TEAM;
            else if (cstrncmp(x, "#IG_VIP_Team_Select", 20) == 0) // team select menu?
                m_bot->m_startAction = CMENU_TEAM;
            else if (cstrncmp(x, "#IG_VIP_Team_Select_Spect", 26) == 0) // team select menu?
                m_bot->m_startAction = CMENU_TEAM;
            else if (cstrncmp(x, "#Terrorist_Select", 18) == 0) // T model select?
                m_bot->m_startAction = CMENU_CLASS;
            else if (cstrncmp(x, "#CT_Select", 11) == 0) // CT model select menu?
                m_bot->m_startAction = CMENU_CLASS;*/
        }

        break;
    }
    case NETMSG_WLIST:
    {
        // this message is sent when a client joins the game. All of the weapons are sent with the weapon ID and information about what ammo is used
        switch (m_state)
        {
        case 0:
        {
            cstrncpy(weaponProp.className, PTR_TO_STR(p), sizeof(weaponProp.className));
            break;
        }
        case 1:
        {
            weaponProp.ammo1 = PTR_TO_INT(p); // ammo index 1
            break;
        }
        case 2:
        {
            weaponProp.ammo1Max = PTR_TO_INT(p); // max ammo 1
            break;
        }
        case 5:
        {
            weaponProp.slotID = PTR_TO_INT(p); // slot for this weapon
            break;
        }
        case 6:
        {
            weaponProp.position = PTR_TO_INT(p); // position in slot
            break;
        }
        case 7:
        {
            weaponProp.id = PTR_TO_INT(p); // weapon ID
            break;
        }
        case 8:
        {

            if (weaponProp.id > -1 && weaponProp.id < Const_MaxWeapons)
            {
                weaponProp.flags = PTR_TO_INT(p); // flags for weapon (WTF???)
                g_weaponDefs[weaponProp.id] = weaponProp; // store away this weapon with it's ammo information...
            }

            break;
        }
        }
        break;
    }
    case NETMSG_CURWEAPON:
    {
        // this message is sent when a weapon is selected (either by the bot chosing a weapon or by the server auto assigning the bot a weapon). In CS it's also called when Ammo is increased/decreased
        switch (m_state)
        {
        case 0:
        {
            state = PTR_TO_INT(p); // state of the current weapon (WTF???)
            break;
        }
        case 1:
        {
            id = PTR_TO_INT(p); // weapon ID of current weapon
            break;
        }
        case 2:
        {
            if (m_bot && id > -1 && id < Const_MaxWeapons)
            {
                if (state != 0)
                    m_bot->m_currentWeapon = id;
                m_bot->m_ammoInClip[id] = PTR_TO_INT(p);
            }
            break;
        }
        }
        break;
    }
    case NETMSG_AMMOX:
    {
        switch (m_state)
        {
        case 0:
        {
            index = PTR_TO_INT(p); // ammo index (for type of ammo)
            break;
        }
        case 1:
        {
            if (m_bot && index > -1 && index < Const_MaxWeapons)
                m_bot->m_ammo[index] = PTR_TO_INT(p);
            break;
        }
        }
        break;
    }
    case NETMSG_AMMOPICK:
    {
        // this message is sent when the bot picks up some ammo (AmmoX messages are also sent so this message is probably
        // not really necessary except it allows the HUD to draw pictures of ammo that have been picked up
        // The bots don't really need pictures since they don't have any eyes anyway
        switch (m_state)
        {
        case 0:
        {
            index = PTR_TO_INT(p);
            break;
        }
        case 1:
        {
            if (m_bot && index > -1 && index < Const_MaxWeapons)
                m_bot->m_ammo[index] = PTR_TO_INT(p);
            break;
        }
        }
        break;
    }
    case NETMSG_MONEY:
    {
        // this message gets sent when the bots money amount changes
        if (!m_state && m_bot)
            m_bot->m_moneyAmount = PTR_TO_INT(p); // amount of money
        break;
    }
    case NETMSG_STATUSICON:
    {
        switch (m_state)
        {
        case 0:
        {
            enabled = PTR_TO_BYTE(p);
            break;
        }
        case 1:
        {
            if (!(g_gameVersion & Game::HalfLife) && m_bot)
            {
                x = PTR_TO_STR(p);
                if (!x)
                    break;

                // this is around up to 3-5 times faster than cstrncmp one
                switch (x[0])
                {
                case 'b':
                {
                    if (x[1] == 'u' && charToInt(x) == 565)
                        m_bot->m_inBuyZone = (enabled != 0);
                    break;
                }
                case 'd':
                {
                    if (x[1] == 'e' && charToInt(x) == 549)
                        m_bot->m_hasDefuser = (enabled != 0);
                    break;
                }
                case 'v':
                {
                    if (x[1] == 'i' && charToInt(x) == 764)
                        m_bot->m_inVIPZone = (enabled != 0);
                    break;
                }
                case 'c':
                {
                    if (x[1] == '4')
                        m_bot->m_inBombZone = (enabled == 2);
                    break;
                }
                }

                /*if (cstrncmp(x, "defuser", 8) == 0)
                    m_bot->m_hasDefuser = (enabled != 0);
                else if (cstrncmp(x, "buyzone", 8) == 0)
                    m_bot->m_inBuyZone = (enabled != 0);
                else if (cstrncmp(x, "vipsafety", 10) == 0)
                    m_bot->m_inVIPZone = (enabled != 0);
                else if (cstrncmp(x, "c4", 3) == 0)
                    m_bot->m_inBombZone = (enabled == 2);*/
            }
            break;
        }
        }
        break;
    }
    case NETMSG_DEATH: // this message sends on death
    {
        /*if (m_state == 1)
        {
            Bot* victimer = g_botManager->GetBot(PTR_TO_INT(p));
            if (victimer)
            {
                victimer->m_isAlive = false;
                victimer->m_navNode.Clear();
                victimer->m_avgDeathOrigin += victimer->pev->origin;
                victimer->m_avgDeathOrigin *= 0.5f;
            }
        }
        break;*/

        // this can cause to message not has been sent bug...
        switch (m_state)
        {
        case 0:
        {
            killerIndex = PTR_TO_INT(p);
            break;
        }
        case 1:
        {
            victimIndex = PTR_TO_INT(p);
            break;
        }
        case 2: // this one is heavy...
        {
            edict_t* victim = INDEXENT(victimIndex);
            if (!IsValidPlayer(victim))
                break;

            Bot* bot = g_botManager->GetBot(victimIndex);
            if (bot)
            {
                bot->m_isAlive = false;
                bot->m_navNode.Clear();
                bot->m_avgDeathOrigin += bot->pev->origin;
                bot->m_avgDeathOrigin *= 0.5f;
            }

            edict_t* killer = INDEXENT(killerIndex);
            if (!IsValidPlayer(killer))
                break;

            bot = g_botManager->GetBot(killerIndex);
            int index, teamValue = GetTeam(killer);
            float timeCache = engine->GetTime();
            extern ConVar ebot_camp_max;
            if ((bot && bot->GetCurrentState() == Process::Camp) || (!bot && killer->v.flags & FL_DUCKING))
            {
                for (const auto& teammate : g_botManager->m_bots)
                {
                    if (!teammate)
                        continue;

                    if (teammate->m_team == teamValue)
                        continue;

                    if (!teammate->m_isAlive)
                        continue;

                    if ((teammate->pev->origin - killer->v.origin).GetLengthSquared() > squaredf(1280.0f))
                        continue;

                    if (teammate->m_isBomber || teammate->m_isVIP)
                    {
                        if (teammate->CheckGrenadeThrow(killer))
                            teammate->RadioMessage(Radio::Fallback);
                        else
                        {
                            index = teammate->m_navNode.Last();
                            teammate->FindPath(teammate->m_currentWaypointIndex, index, killer);
                        }
                    }
                    else
                    {
                        teammate->m_pauseTime = timeCache + crandomfloat(2.0f, 5.0f);
                        teammate->LookAt(killer->v.origin + killer->v.view_ofs, killer->v.velocity);

                        if (teammate->CheckGrenadeThrow(killer))
                            teammate->RadioMessage(Radio::Fallback);
                        else if ((bot && bot->m_friendsNearCount > teammate->m_friendsNearCount) && (!bot && teammate->pev->health < killer->v.health))
                        {
                            index = teammate->FindDefendWaypoint(teammate->EyePosition());
                            if (IsValidWaypoint(index))
                            {
                                teammate->m_campIndex = index;
                                teammate->SetProcess(Process::Camp, "there's too many... i must camp", true, timeCache + ebot_camp_max.GetFloat());
                            }
                            else // hell no...
                            {
                                index = teammate->m_navNode.Last();
                                teammate->FindPath(teammate->m_currentWaypointIndex, index, killer);
                                teammate->RadioMessage(Radio::RegroupTeam);
                            }
                        }
                        else
                        {
                            index = teammate->m_navNode.Last();
                            teammate->FindPath(teammate->m_currentWaypointIndex, index, killer);
                        }
                    }
                }
            }
            else // not camping (act different, but idk what can i do else ???)
            {
                for (const auto& teammate : g_botManager->m_bots)
                {
                    if (!teammate)
                        continue;

                    if (teammate->m_team == teamValue)
                        continue;

                    if (!teammate->m_isAlive)
                        continue;

                    if ((teammate->pev->origin - killer->v.origin).GetLengthSquared() > squaredf(1280.0f))
                        continue;

                    if (teammate->m_isBomber || teammate->m_isVIP)
                    {
                        index = teammate->m_navNode.Last();
                        teammate->FindPath(teammate->m_currentWaypointIndex, index, killer);
                    }
                    else
                    {
                        teammate->m_pauseTime = timeCache + crandomfloat(2.0f, 5.0f);
                        teammate->LookAt(killer->v.origin + killer->v.view_ofs, killer->v.velocity);

                        if ((bot && bot->m_friendsNearCount > teammate->m_friendsNearCount) && (!bot && teammate->pev->health < killer->v.health))
                        {
                            index = teammate->FindDefendWaypoint(teammate->EyePosition());
                            if (IsValidWaypoint(index))
                            {
                                teammate->m_campIndex = index;
                                teammate->SetProcess(Process::Camp, "there's too many... i must camp", true, timeCache + ebot_camp_max.GetFloat());
                            }
                            else // hell no...
                            {
                                index = teammate->m_navNode.Last();
                                teammate->FindPath(teammate->m_currentWaypointIndex, index, killer);
                                teammate->RadioMessage(Radio::RegroupTeam);
                            }
                        }
                        else
                        {
                            index = teammate->m_navNode.Last();
                            teammate->FindPath(teammate->m_currentWaypointIndex, index, killer);
                        }
                    }
                }
            }
            break;
        }
        }
        break;
    }
    case NETMSG_SCREENFADE: // this message gets sent when the screen fades (flashbang)
    {
        switch (m_state)
        {
        case 3:
        {
            r = PTR_TO_BYTE(p);
            break;
        }
        case 4:
        {
            g = PTR_TO_BYTE(p);
            break;
        }
        case 5:
        {
            b = PTR_TO_BYTE(p);
            break;
        }
        case 6:
        {
            if (m_bot)
                m_bot->TakeBlinded(Vector(r, g, b), PTR_TO_BYTE(p));
            break;
        }
        }
        break;
    }
    case NETMSG_HLTV: // round restart in steam cs
    {
        switch (m_state)
        {
        case 0:
        {
            numPlayers = PTR_TO_INT(p);
            break;
        }
        case 1:
        {
            if (!numPlayers && !PTR_TO_INT(p))
                RoundInit();
            break;
        }
        }
        break;
    }
    case NETMSG_TEXTMSG:
    {
        if (m_state)
        {
            x = PTR_TO_STR(p);
            if (!x)
                break;

            if (cstrncmp(x, "#CTs_Win", 9) == 0 ||
                cstrncmp(x, "#Bomb_Defused", 14) == 0 ||
                cstrncmp(x, "#Terrorists_Win", 16) == 0 ||
                cstrncmp(x, "#Round_Draw", 12) == 0 ||
                cstrncmp(x, "#All_Hostages_Rescued", 22) == 0 ||
                cstrncmp(x, "#Target_Saved", 14) == 0 ||
                cstrncmp(x, "#Hostages_Not_Rescued", 22) == 0 ||
                cstrncmp(x, "#Terrorists_Not_Escaped", 24) == 0 ||
                cstrncmp(x, "#VIP_Not_Escaped", 17) == 0 ||
                cstrncmp(x, "#Escaping_Terrorists_Neutralized", 33) == 0 ||
                cstrncmp(x, "#VIP_Assassinated", 18) == 0 ||
                cstrncmp(x, "#VIP_Escaped", 13) == 0 ||
                cstrncmp(x, "#Terrorists_Escaped", 20) == 0 ||
                cstrncmp(x, "#CTs_PreventEscape", 19) == 0 ||
                cstrncmp(x, "#Target_Bombed", 15) == 0 ||
                cstrncmp(x, "#Game_Commencing", 17) == 0 ||
                cstrncmp(x, "#Game_will_restart_in", 22) == 0)
            {
                g_roundEnded = true;

                if (GetGameMode() == GameMode::Original)
                {
                    if (cstrncmp(x, "#CTs_Win", 9) == 0)
                        g_botManager->SetLastWinner(Team::Counter); // update last winner for economics
                    else if (cstrncmp(x, "#Terrorists_Win", 16) == 0)
                        g_botManager->SetLastWinner(Team::Terrorist); // update last winner for economics
                }

                g_waypoint->SetBombPosition(true);
            }
            else if (!g_bombPlanted && cstrncmp(x, "#Bomb_Planted", 14) == 0)
            {
                g_bombPlanted = true;
                g_bombSayString = true;
                g_timeBombPlanted = engine->GetTime();
                g_waypoint->SetBombPosition();

                for (const auto& bot : g_botManager->m_bots)
                {
                    if (!bot)
                        continue;

                    if (!bot->m_isAlive)
                        continue;

                    bot->m_navNode.Clear();
                }
            }
            else if (m_bot)
            {
                if (cstrncmp(x, "#Switch_To_BurstFire", 21) == 0)
                    m_bot->m_weaponBurstMode = BurstMode::Enabled;
                else if (cstrncmp(x, "#Switch_To_SemiAuto", 20) == 0)
                    m_bot->m_weaponBurstMode = BurstMode::Disabled;
            }
        }
        break;
    }
    case NETMSG_BARTIME:
    {
        if (!m_state && m_bot)
        {
            if (GetGameMode() == GameMode::Original)
            {
                if (PTR_TO_INT(p))
                    m_bot->m_hasProgressBar = true; // the progress bar on a hud
                else
                    m_bot->m_hasProgressBar = false; // no progress bar or disappeared
            }
            else
                m_bot->m_hasProgressBar = false;
        }
        break;
    }
    }
    m_state++; // and finally update network message state
}
