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

    static int index = 0, numPlayers = 0;
    static int state = 0, id = 0;

    static WeaponProperty weaponProp;
    static char* x;

    // now starts of netmessage execution
    switch (m_message)
    {
    case NETMSG_VGUI:
    {
        // this message is sent when a VGUI menu is displayed
        if (!m_state)
        {
            switch (PTR_TO_INT(p))
            {
            case GMENU_TEAM:
            {
                if (m_bot)
                    m_bot->m_startAction = CMENU_TEAM;
                break;
            }
            case GMENU_TERRORIST:
            case GMENU_COUNTER:
            {
                if (m_bot)
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
            cstrcpy(weaponProp.className, PTR_TO_STR(p));
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
            if (weaponProp.id >= 0 && weaponProp.id < Const_MaxWeapons)
            {
                weaponProp.flags = PTR_TO_INT(p);
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
            if (m_bot)
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
            if (m_bot)
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
            if (m_bot)
                m_bot->m_ammo[index] = PTR_TO_INT(p);
            break;
        }
        }
        break;
    }
    case NETMSG_DAMAGE:
    {
        // this message gets sent when the bots are getting damaged
        if (m_state == 2)
        {
            if (m_bot)
                m_bot->IgnoreCollisionShortly();
        }
        break;
    }
    case NETMSG_DEATH:
    {
        // this message sends on death
        if (m_state == 1)
        {
            Bot* victimer = g_botManager->GetBot(PTR_TO_INT(p));
            if (victimer)
            {
                victimer->m_isAlive = false;
                victimer->m_navNode.Clear();
                cmemset(&victimer->m_ammoInClip, 0, sizeof(victimer->m_ammoInClip));
	            cmemset(&victimer->m_ammo, 0, sizeof(victimer->m_ammo));
	            victimer->m_currentWeapon = 0;
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
                g_helicopter = nullptr;
                for (const auto& bot : g_botManager->m_bots)
                {
                    if (bot)
                    {
                        bot->m_hasEnemiesNear = false;
                        bot->m_hasFriendsNear = false;
                        bot->m_hasEntitiesNear = false;
                        bot->m_numEnemiesLeft = 0;
                        bot->m_numFriendsLeft = 0;
                        bot->m_isSlowThink = false;
                    }
                }
            }
        }
        break;
    }
    }
    m_state++; // and finally update network message state
}
