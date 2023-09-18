#include <core.h>

NetworkMsg::NetworkMsg(void)
{
    m_message = NETMSG_UNDEFINED;
    m_state = 0;
    m_bot.reset();

    for (int i = 0; i < NETMSG_NUM; i++)
        m_registerdMessages[i] = NETMSG_UNDEFINED;
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
    static uint8_t r, g, b;
    static uint8_t enabled;

    static int damageArmor, damageTaken, damageBits;
    static int killerIndex, victimIndex, playerIndex;
    static int index, numPlayers;
    static int state, id, clip;

    static Vector damageOrigin;
    static WeaponProperty weaponProp;

    // now starts of netmessage execution
    switch (m_message)
    {
    case NETMSG_VGUI:
    {
        // this message is sent when a VGUI menu is displayed.
        if (m_state == 0)
        {
            switch (PTR_TO_INT(p))
            {
            case GMENU_TEAM:
                m_bot->m_startAction = CMENU_TEAM;
                break;

            case GMENU_TERRORIST:
            case GMENU_COUNTER:
                m_bot->m_startAction = CMENU_CLASS;
                break;
            }
        }

        break;
    }
    case NETMSG_SHOWMENU:
    {
        // this message is sent when a text menu is displayed.
        if (m_state < 3) // ignore first 3 fields of message
            break;

        {
            const char* x = PTR_TO_STR(p);
            if (cstrcmp(x, "#Team_Select") == 0) // team select menu?
                m_bot->m_startAction = CMENU_TEAM;
            else if (cstrcmp(x, "#Team_Select_Spect") == 0) // team select menu?
                m_bot->m_startAction = CMENU_TEAM;
            else if (cstrcmp(x, "#IG_Team_Select_Spect") == 0) // team select menu?
                m_bot->m_startAction = CMENU_TEAM;
            else if (cstrcmp(x, "#IG_Team_Select") == 0) // team select menu?
                m_bot->m_startAction = CMENU_TEAM;
            else if (cstrcmp(x, "#IG_VIP_Team_Select") == 0) // team select menu?
                m_bot->m_startAction = CMENU_TEAM;
            else if (cstrcmp(x, "#IG_VIP_Team_Select_Spect") == 0) // team select menu?
                m_bot->m_startAction = CMENU_TEAM;
            else if (cstrcmp(x, "#Terrorist_Select") == 0) // T model select?
                m_bot->m_startAction = CMENU_CLASS;
            else if (cstrcmp(x, "#CT_Select") == 0) // CT model select menu?
                m_bot->m_startAction = CMENU_CLASS;
        }

        break;
    }
    case NETMSG_WLIST:
    {
        // this message is sent when a client joins the game. All of the weapons are sent with the weapon ID and information about what ammo is used.
        switch (m_state)
        {
        case 0:
            cstrcpy(weaponProp.className, PTR_TO_STR(p));
            break;

        case 1:
            weaponProp.ammo1 = PTR_TO_INT(p); // ammo index 1
            break;

        case 2:
            weaponProp.ammo1Max = PTR_TO_INT(p); // max ammo 1
            break;

        case 5:
            weaponProp.slotID = PTR_TO_INT(p); // slot for this weapon
            break;

        case 6:
            weaponProp.position = PTR_TO_INT(p); // position in slot
            break;

        case 7:
            weaponProp.id = PTR_TO_INT(p); // weapon ID
            break;

        case 8:
            weaponProp.flags = PTR_TO_INT(p); // flags for weapon (WTF???)
            g_weaponDefs[weaponProp.id] = weaponProp; // store away this weapon with it's ammo information...
            break;
        }
        break;
    }
    case NETMSG_CURWEAPON:
    {
        // this message is sent when a weapon is selected (either by the bot chosing a weapon or by the server auto assigning the bot a weapon). In CS it's also called when Ammo is increased/decreased
        switch (m_state)
        {
        case 0:
            state = PTR_TO_INT(p); // state of the current weapon (WTF???)
            break;

        case 1:
            id = PTR_TO_INT(p); // weapon ID of current weapon
            break;

        case 2:
            clip = PTR_TO_INT(p); // ammo currently in the clip for this weapon

            if (id <= 31)
            {
                if (state != 0)
                    m_bot->m_currentWeapon = id;

                m_bot->m_ammoInClip[id] = clip;
            }
            break;
        }
        break;
    }
    case NETMSG_AMMOX:
    {
        // this message is sent whenever ammo amounts are adjusted (up or down). NOTE: Logging reveals that CS uses it very unreliable!
        switch (m_state)
        {
        case 0:
            index = PTR_TO_INT(p); // ammo index (for type of ammo)
            break;

        case 1:
            m_bot->m_ammo[index] = PTR_TO_INT(p); // store it away
            break;
        }
        break;
    }
    case NETMSG_AMMOPICK:
    {
        // this message is sent when the bot picks up some ammo (AmmoX messages are also sent so this message is probably
        // not really necessary except it allows the HUD to draw pictures of ammo that have been picked up. The bots
        // don't really need pictures since they don't have any eyes anyway.
        switch (m_state)
        {
        case 0:
            index = PTR_TO_INT(p);
            break;

        case 1:
            m_bot->m_ammo[index] = PTR_TO_INT(p);
            break;
        }
        break;
    }
    case NETMSG_MONEY:
    {
        // this message gets sent when the bots money amount changes
        if (m_state == 0)
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
            {
                if (cstrcmp(PTR_TO_STR(p), "defuser") == 0)
                    m_bot->m_hasDefuser = (enabled != 0);
                else if (cstrcmp(PTR_TO_STR(p), "buyzone") == 0)
                {
                    m_bot->m_inBuyZone = (enabled != 0);
                    m_bot->EquipInBuyzone(0);
                }
                else if (cstrcmp(PTR_TO_STR(p), "vipsafety") == 0)
                    m_bot->m_inVIPZone = (enabled != 0);
                else if (cstrcmp(PTR_TO_STR(p), "c4") == 0)
                    m_bot->m_inBombZone = (enabled == 2);

                break;
            }
        }
        }
        break;
    }
    case NETMSG_DEATH: // this message sends on death
    {
        switch (m_state)
        {
        case 0:
            killerIndex = PTR_TO_INT(p);
            break;

        case 1:
            victimIndex = PTR_TO_INT(p);
            break;

        case 2:
            edict_t * victim = INDEXENT(victimIndex);
            if (FNullEnt(victim) || !IsValidPlayer(victim))
                break;

            Bot* victimer = g_botManager->GetBot(victim);
            if (victimer != nullptr)
            {
                victimer->m_isAlive = false;
                victimer->DeleteSearchNodes();
            }

            break;
        }
        break;
    }
    case NETMSG_SCREENFADE: // this message gets sent when the Screen fades (Flashbang)
    {
        switch (m_state)
        {
        case 3:
            r = PTR_TO_BYTE(p);
            break;

        case 4:
            g = PTR_TO_BYTE(p);
            break;

        case 5:
            b = PTR_TO_BYTE(p);
            break;

        case 6:
            m_bot->TakeBlinded(Vector(r, g, b), PTR_TO_BYTE(p));
            break;
        }
        break;
    }
    case NETMSG_HLTV: // round restart in steam cs
    {
        switch (m_state)
        {
        case 0:
            numPlayers = PTR_TO_INT(p);
            break;

        case 1:
            if (numPlayers == 0 && PTR_TO_INT(p) == 0)
                RoundInit();
            break;
        }
        break;
    }
    case NETMSG_TEXTMSG:
    {
        if (m_state == 1)
        {
            const char* x = PTR_TO_STR(p);
            if (FStrEq(x, "#CTs_Win") ||
                FStrEq(x, "#Bomb_Defused") ||
                FStrEq(x, "#Terrorists_Win") ||
                FStrEq(x, "#Round_Draw") ||
                FStrEq(x, "#All_Hostages_Rescued") ||
                FStrEq(x, "#Target_Saved") ||
                FStrEq(x, "#Hostages_Not_Rescued") ||
                FStrEq(x, "#Terrorists_Not_Escaped") ||
                FStrEq(x, "#VIP_Not_Escaped") ||
                FStrEq(x, "#Escaping_Terrorists_Neutralized") ||
                FStrEq(x, "#VIP_Assassinated") ||
                FStrEq(x, "#VIP_Escaped") ||
                FStrEq(x, "#Terrorists_Escaped") ||
                FStrEq(x, "#CTs_PreventEscape") ||
                FStrEq(x, "#Target_Bombed") ||
                FStrEq(x, "#Game_Commencing") ||
                FStrEq(x, "#Game_will_restart_in"))
            {
                g_roundEnded = true;

                if (GetGameMode() == GameMode::Original)
                {
                    if (FStrEq(x, "#CTs_Win"))
                        g_botManager->SetLastWinner(Team::Counter); // update last winner for economics

                    if (FStrEq(x, "#Terrorists_Win"))
                        g_botManager->SetLastWinner(Team::Terrorist); // update last winner for economics
                }

                g_waypoint->SetBombPosition(true);
            }
            else if (!g_bombPlanted && FStrEq(x, "#Bomb_Planted"))
            {
                g_bombPlanted = true;
                g_bombSayString = true;
                g_timeBombPlanted = engine->GetTime();
                g_waypoint->SetBombPosition();

                for (const auto& bot : g_botManager->m_bots)
                {
                    if (bot != nullptr && bot->m_isAlive)
                        bot->DeleteSearchNodes();
                }
            }
            else if (m_bot != nullptr && FStrEq(x, "#Switch_To_BurstFire"))
                m_bot->m_weaponBurstMode = BurstMode::Enabled;
            else if (m_bot != nullptr && FStrEq(x, "#Switch_To_SemiAuto"))
                m_bot->m_weaponBurstMode = BurstMode::Disabled;
        }

        break;
    }
    case NETMSG_BARTIME:
    {
        if (m_state == 0)
        {
            if (GetGameMode() == GameMode::Original)
            {
                const int x = PTR_TO_INT(p);
                if (x > 0)
                    m_bot->m_hasProgressBar = true; // the progress bar on a hud
                else if (x == 0)
                    m_bot->m_hasProgressBar = false; // no progress bar or disappeared
            }
            else
                m_bot->m_hasProgressBar = false;
        }

        break;
    }
    default:
        AddLogEntry(Log::Fatal, "Network message handler error. Call to unrecognized message id (%d).\n", m_message);
    }

    m_state++; // and finally update network message state
}
