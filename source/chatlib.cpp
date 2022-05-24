// 
// Copyright (c) 2003-2021, by HsK-Dev Blog 
// https://ccnhsk-dev.blogspot.com/ 
// 
// And Thank About Yet Another POD-Bot Development Team.
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

// todo: kill that file

#include <core.h>

ConVar ebot_chat("ebot_chat", "1");

void StripTags(char* buffer)
{
    // this function strips 'clan' tags specified below in given string buffer

    // first three tags for Enhanced POD-Bot (e[POD], 3[POD], E[POD])
    char* tagOpen[] = {"e[P", "3[P", "E[P", "-=", "-[", "-]", "-}", "-{", "<[", "<]", "[-", "]-", "{-", "}-", "[[", "[", "{", "]", "}", "<", ">", "-", "|", "=", "+", "("};
    char* tagClose[] = {"]", "]", "]", "=-", "]-", "[-", "{-", "}-", "]>", "[>", "-]", "-[", "-}", "-{", "]]", "]", "}", "[", "{", ">", "<", "-", "|", "=", "+", ")"};

    int index, fieldStart, fieldStop, i;
    int length = strlen(buffer); // get length of string

    // foreach known tag...
    for (index = 0; index < ARRAYSIZE_HLSDK(tagOpen); index++)
    {
        fieldStart = strstr(buffer, tagOpen[index]) - buffer; // look for a tag start

        // have we found a tag start?
        if (fieldStart >= 0 && fieldStart < 32)
        {
            fieldStop = strstr(buffer, tagClose[index]) - buffer; // look for a tag stop

            // have we found a tag stop?
            if ((fieldStop > fieldStart) && (fieldStop < 32))
            {
                for (i = fieldStart; i < length - (fieldStop + static_cast <int> (strlen(tagClose[index])) - fieldStart); i++)
                    buffer[i] = buffer[i + (fieldStop + strlen(tagClose[index]) - fieldStart)]; // overwrite the buffer with the stripped string

                buffer[i] = 0x0; // terminate the string
            }
        }
    }

    // have we stripped too much (all the stuff)?
    if (strlen(buffer) != 0)
    {
        strtrim(buffer); // if so, string is just a tag

        // strip just the tag part..
        for (index = 0; index < ARRAYSIZE_HLSDK(tagOpen); index++)
        {
            fieldStart = strstr(buffer, tagOpen[index]) - buffer; // look for a tag start

            // have we found a tag start?
            if (fieldStart >= 0 && fieldStart < 32)
            {
                fieldStop = fieldStart + strlen(tagOpen[index]); // set the tag stop

                for (i = fieldStart; i < length - static_cast <int> (strlen(tagOpen[index])); i++)
                    buffer[i] = buffer[i + strlen(tagOpen[index])]; // overwrite the buffer with the stripped string

                buffer[i] = 0x0; // terminate the string

                fieldStart = strstr(buffer, tagClose[index]) - buffer; // look for a tag stop

                // have we found a tag stop ?
                if (fieldStart >= 0 && fieldStart < 32)
                {
                    fieldStop = fieldStart + strlen(tagClose[index]); // set the tag stop

                    for (i = fieldStart; i < length - static_cast <int> (strlen(tagClose[index])); i++)
                        buffer[i] = buffer[i + static_cast <int> (strlen(tagClose[index]))]; // overwrite the buffer with the stripped string

                    buffer[i] = 0; // terminate the string
                }
            }
        }
    }

    strtrim(buffer); // to finish, strip eventual blanks after and before the tag marks
}

char* HumanizeName(char* name)
{
    // this function humanize player name (i.e. trim clan and switch to lower case (sometimes))

    static char outputName[256]; // create return name buffer
    strcpy(outputName, name); // copy name to new buffer

    // drop tag marks, 80 percent of time
    if (engine->RandomInt(1, 100) < 80)
        StripTags(outputName);
    else
        strtrim(outputName);

    // sometimes switch name to lower characters
    // note: since we're using russian names written in english, we reduce this shit to 6 percent
    if (engine->RandomInt(1, 100) <= 6)
    {
        for (int i = 0; i < static_cast <int> (strlen(outputName)); i++)
            outputName[i] = static_cast <char> (tolower(outputName[i])); // to lower case
    }

    return &outputName[0]; // return terminated string
}

void HumanizeChat(char* buffer)
{
    // this function humanize chat string to be more handwritten

    int length = strlen(buffer); // get length of string
    int i = 0;

    // sometimes switch text to lowercase
    // note: since we're using russian chat written in english, we reduce this shit to 4 percent
    if (engine->RandomInt(1, 100) <= 4)
    {
        for (i = 0; i < length; i++)
            buffer[i] = static_cast <char> (tolower(buffer[i])); // switch to lowercase
    }

    if (length > 15)
    {
        // "length / 2" percent of time drop a character
        if (engine->RandomInt(1, 100) < (length / 2))
        {
            int pos = engine->RandomInt((length / 8), length - (length / 8)); // chose random position in string

            for (i = pos; i < length - 1; i++)
                buffer[i] = buffer[i + 1]; // overwrite the buffer with stripped string

            buffer[i] = 0x0; // terminate string;
            length--; // update new string length
        }

        // "length" / 4 precent of time swap character
        if (engine->RandomInt(1, 100) < (length / 4))
        {
            int pos = engine->RandomInt((length / 8), ((3 * length) / 8)); // choose random position in string
            char ch = buffer[pos]; // swap characters

            buffer[pos] = buffer[pos + 1];
            buffer[pos + 1] = ch;
        }
    }

    buffer[length] = 0; // terminate string
}

void Bot::PrepareChatMessage(char* text)
{
    // this function parses messages from the botchat, replaces keywords and converts names into a more human style

    if (!ebot_chat.GetBool() || IsNullString(text))
        return;

    memset(&m_tempStrings, 0, sizeof(m_tempStrings));

    char* textStart = text;
    char* pattern = text;

    edict_t* talkEntity = null;

    while (pattern != null)
    {
        // all replacement placeholders start with a %
        pattern = strstr(textStart, "%");

        if (pattern != null)
        {
            int length = pattern - textStart;

            if (length > 0)
                strncpy(m_tempStrings, textStart, length);

            pattern++;

            // player with most frags?
            if (*pattern == 'f')
            {
                int highestFrags = -9000; // just pick some start value
                edict_t* entity = null;

                for (const auto& client : g_clients)
                {
                    if (!(client.flags & CFLAG_USED) || client.ent == GetEntity())
                        continue;

                    int frags = static_cast <int> (client.ent->v.frags);

                    if (frags > highestFrags)
                    {
                        highestFrags = frags;
                        entity = client.ent;
                    }
                }

                talkEntity = entity;

                if (!FNullEnt(talkEntity))
                    strcat(m_tempStrings, HumanizeName(const_cast <char*> (GetEntityName(talkEntity))));
            }
            // mapname?
            else if (*pattern == 'm')
                strcat(m_tempStrings, GetMapName());
            // roundtime?
            else if (*pattern == 'r')
            {
                int time = static_cast <int> (g_timeRoundEnd - engine->GetTime());
                strcat(m_tempStrings, FormatBuffer("%02d:%02d", time / 60, time % 60));
            }
            // chat reply?
            else if (*pattern == 's')
            {
                talkEntity = INDEXENT(m_sayTextBuffer.entityIndex);

                if (!FNullEnt(talkEntity))
                    strcat(m_tempStrings, HumanizeName(const_cast <char*> (GetEntityName(talkEntity))));
            }
            // teammate alive?
            else if (*pattern == 't')
            {
                edict_t* entity = null;

                for (const auto& client : g_clients)
                {
                    if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || (client.team != m_team) || (client.ent == GetEntity()))
                        continue;

                    entity = client.ent;

                    break;
                }

                if (!FNullEnt(entity))
                {
                    if (!FNullEnt(pev->dmg_inflictor) && (GetTeam(GetEntity()) == GetTeam(pev->dmg_inflictor)))
                        talkEntity = pev->dmg_inflictor;
                    else
                        talkEntity = entity;

                    if (!FNullEnt(talkEntity))
                        strcat(m_tempStrings, HumanizeName(const_cast <char*> (GetEntityName(talkEntity))));
                }
                else // no teammates alive...
                {
                    for (const auto& client : g_clients)
                    {
                        if (!(client.flags & CFLAG_USED) || (client.team != m_team) || (client.ent == GetEntity()))
                            continue;

                        entity = client.ent;

                        break;
                    }

                    if (!FNullEnt(entity))
                    {
                        talkEntity = entity;

                        if (!FNullEnt(talkEntity))
                            strcat(m_tempStrings, HumanizeName(const_cast <char*> (GetEntityName(talkEntity))));
                    }
                }
            }
            else if (*pattern == 'e')
            {
                edict_t* entity = null;

                for (const auto& client : g_clients)
                {
                    if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || (client.team == m_team) || (client.ent == GetEntity()))
                        continue;

                    entity = client.ent;

                    break;
                }

                if (!FNullEnt(entity))
                {
                    talkEntity = entity;

                    if (!FNullEnt(talkEntity))
                        strcat(m_tempStrings, HumanizeName(const_cast <char*> (GetEntityName(talkEntity))));
                }
                else // no teammates alive...
                {
                    for (const auto& client : g_clients)
                    {
                        if (!(client.flags & CFLAG_USED) || (client.team == m_team) || (client.ent == GetEntity()))
                            continue;

                        entity = client.ent;

                        break;
                    }

                    if (!FNullEnt(entity))
                    {
                        talkEntity = entity;

                        if (!FNullEnt(talkEntity))
                            strcat(m_tempStrings, HumanizeName(const_cast <char*> (GetEntityName(talkEntity))));
                    }
                }
            }
            else if (*pattern == 'd')
            {
                if (g_gameVersion == CSVER_CZERO)
                {
                    if (engine->RandomInt(1, 100) < 30)
                        strcat(m_tempStrings, "CZ");
                    else if (engine->RandomInt(1, 100) < 80)
                        strcat(m_tempStrings, "KoHTpa K3");
                    else
                        strcat(m_tempStrings, "Condition Zero");
                }
                else if ((g_gameVersion == CSVER_CSTRIKE) || (g_gameVersion == CSVER_VERYOLD))
                {
                    if (engine->RandomInt(1, 100) < 30)
                        strcat(m_tempStrings, "CS");
                    else if (engine->RandomInt(1, 100) < 80)
                        strcat(m_tempStrings, "KoHTpa");
                    else
                        strcat(m_tempStrings, "Counter-Strike");
                }
            }
            else if (*pattern == 'v')
            {
                talkEntity = m_lastVictim;

                if (!FNullEnt(talkEntity))
                    strcat(m_tempStrings, HumanizeName(const_cast <char*> (GetEntityName(talkEntity))));
            }

            pattern++;
            textStart = pattern;
        }
    }

    // let the bots make some mistakes...
    char tempString[160];
    strncpy(tempString, textStart, 159);

    HumanizeChat(tempString);
    strcat(m_tempStrings, tempString);
}

bool CheckKeywords(char* tempMessage, char* reply)
{
    // this function checks is string contain keyword, and generates relpy to it

    if (!ebot_chat.GetBool() || IsNullString(tempMessage))
        return false;

    ITERATE_ARRAY(g_replyFactory, i)
    {
        ITERATE_ARRAY(g_replyFactory[i].keywords, j)
        {
            // check is keyword has occurred in message
            if (strstr(tempMessage, g_replyFactory[i].keywords[j]) != null)
            {
                if (g_replyFactory[i].usedReplies.GetElementNumber() >= g_replyFactory[i].replies.GetElementNumber() / 2)
                    g_replyFactory[i].usedReplies.RemoveAll();

                bool replyUsed = false;
                const String& generatedReply = g_replyFactory[i].replies.GetRandomElement();

                // don't say this twice
                ITERATE_ARRAY(g_replyFactory[i].usedReplies, k)
                {
                    if (strstr(g_replyFactory[i].usedReplies[k], generatedReply) != null)
                        replyUsed = true;
                }

                // reply not used, so use it
                if (!replyUsed)
                {
                    strcpy(reply, generatedReply); // update final buffer
                    g_replyFactory[i].usedReplies.Push(generatedReply); //add to ignore list

                    return true;
                }
            }
        }
    }

    // didn't find a keyword? 70% of the time use some universal reply
    if (engine->RandomInt(1, 100) < 70 && !g_chatFactory[CHAT_NOKW].IsEmpty())
    {
        strcpy(reply, g_chatFactory[CHAT_NOKW].GetRandomElement());
        return true;
    }

    return false;
}

bool Bot::ParseChat(char* reply)
{
    // this function parse chat buffer, and prepare buffer to keyword searching

    char tempMessage[512];
    strcpy(tempMessage, m_sayTextBuffer.sayText); // copy to safe place

    // text to uppercase for keyword parsing
    for (int i = 0; i < static_cast <int> (strlen(tempMessage)); i++)
        tempMessage[i] = static_cast <char> (toupper(tempMessage[i]));

    return CheckKeywords(tempMessage, reply);
}

bool Bot::RepliesToPlayer(void)
{
    // this function sends reply to a player

    if (m_sayTextBuffer.entityIndex != -1 && !IsNullString(m_sayTextBuffer.sayText))
    {
        char text[256];

        // check is time to chat is good
        if (m_sayTextBuffer.timeNextChat < engine->GetTime())
        {
            if (engine->RandomInt(1, 100) < m_sayTextBuffer.chatProbability + engine->RandomInt(2, 10) && ParseChat(text))
            {
                PrepareChatMessage(text);
                PushMessageQueue(CMENU_SAY);

                m_sayTextBuffer.entityIndex = -1;
                m_sayTextBuffer.sayText[0] = 0x0;
                m_sayTextBuffer.timeNextChat = engine->GetTime() + m_sayTextBuffer.chatDelay;

                return true;
            }

            m_sayTextBuffer.entityIndex = -1;
            m_sayTextBuffer.sayText[0] = 0x0;
        }
    }

    return false;
}

void Bot::ChatSay(bool teamSay, const char* text, ...)
{
    if (IsNullString(text))
        return;

    // block looping same message.
    if (!IsNullString(m_lastStrings) && m_lastStrings == text)
        return;

    // humanize chat
    if (m_lastChatEnt == GetEntity())
        return;

    char botName[80];
    char botTeam[22];
    char tempMessage[256];

    if (!teamSay)
        strcpy(botTeam, "");
    else if (m_team == TEAM_TERRORIST)
        strcpy(botTeam, "(Terrorist)");
    else if (m_team == TEAM_COUNTER)
        strcpy(botTeam, "(Counter-Terrorist)");

    strcpy(botName, GetEntityName(GetEntity()));

    for (const auto& client : g_clients)
    {
        if (!(client.flags & CFLAG_USED) || client.ent == GetEntity() || client.flags & FL_FAKECLIENT)
            continue;

        if (teamSay && client.team != m_team)
            continue;

        if (!m_notKilled)
            sprintf(tempMessage, "%c*DEAD*%s %c%s%c :  %s\n", 0x01, botTeam, 0x03, botName, 0x01, text);
        else
        {
            if (teamSay)
                sprintf(tempMessage, "%c%s %c%s%c :  %s\n", 0x01, botTeam, 0x03, botName, 0x01, text);
            else
                sprintf(tempMessage, "%c%s :  %s\n", 0x02, botName, text);
        }

        m_lastStrings[160] = *text;
        m_lastChatEnt = GetEntity();

        MESSAGE_BEGIN(MSG_ONE, g_netMsg->GetId(NETMSG_SAYTEXT), null, client.ent);
        WRITE_BYTE(GetIndex());
        WRITE_STRING(tempMessage);
        MESSAGE_END();
    }
}