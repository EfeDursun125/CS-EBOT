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

#include <core.h>

ConVar ebot_chat("ebot_chat", "1");

// this function strips 'clan' tags specified below in given string buffer
void StripTags(char* buffer)
{
    // first three tags for Enhanced POD-Bot (e[POD], 3[POD], E[POD])
    char* tagOpen[] = {"e[P", "3[P", "E[P", "-=", "-[", "-]", "-}", "-{", "<[", "<]", "[-", "]-", "{-", "}-", "[[", "[", "{", "]", "}", "<", ">", "-", "|", "=", "+", "("};
    char* tagClose[] = {"]", "]", "]", "=-", "]-", "[-", "{-", "}-", "]>", "[>", "-]", "-[", "-}", "-{", "]]", "]", "}", "[", "{", ">", "<", "-", "|", "=", "+", ")"};

    int index, fieldStart, fieldStop, i;
    const int length = cstrlen(buffer); // get length of string

    // foreach known tag...
    auto size = ARRAYSIZE_HLSDK(tagOpen);
    for (index = 0; index < size; index++)
    {
        fieldStart = cstrstr(buffer, tagOpen[index]) - buffer; // look for a tag start

        // have we found a tag start?
        if (fieldStart >= 0 && fieldStart < 32)
        {
            fieldStop = cstrstr(buffer, tagClose[index]) - buffer; // look for a tag stop

            // have we found a tag stop?
            if ((fieldStop > fieldStart) && (fieldStop < 32))
            {
                const int close = cstrlen(tagClose[index]);
                for (i = fieldStart; i < length - (fieldStop + close - fieldStart); i++)
                    buffer[i] = buffer[i + (fieldStop + close - fieldStart)]; // overwrite the buffer with the stripped string

                buffer[i] = 0x0; // terminate the string
            }
        }
    }

    // have we stripped too much (all the stuff)?
    if (cstrlen(buffer) > 0)
    {
        cstrtrim(buffer); // if so, string is just a tag

        // strip just the tag part..
        size = ARRAYSIZE_HLSDK(tagOpen);
        for (index = 0; index < size; index++)
        {
            fieldStart = cstrstr(buffer, tagOpen[index]) - buffer; // look for a tag start

            // have we found a tag start?
            if (fieldStart >= 0 && fieldStart < 32)
            {
                const int open = cstrlen(tagOpen[index]);
                fieldStop = fieldStart + open; // set the tag stop

                for (i = fieldStart; i < length - open; i++)
                    buffer[i] = buffer[i + open]; // overwrite the buffer with the stripped string

                buffer[i] = 0x0; // terminate the string

                fieldStart = cstrstr(buffer, tagClose[index]) - buffer; // look for a tag stop

                // have we found a tag stop ?
                if (fieldStart >= 0 && fieldStart < 32)
                {
                    const int close = cstrlen(tagClose[index]);
                    fieldStop = fieldStart + close; // set the tag stop

                    for (i = fieldStart; i < length - close; i++)
                        buffer[i] = buffer[i + close]; // overwrite the buffer with the stripped string

                    buffer[i] = 0; // terminate the string
                }
            }
        }
    }

    cstrtrim(buffer); // to finish, strip eventual blanks after and before the tag marks
}

// this function humanize player name (i.e. trim clan and switch to lower case (sometimes))
char* HumanizeName(char* name)
{
    char outputName[256]; // create return name buffer
    cstrncpy(outputName, name, sizeof(outputName)); // copy name to new buffer

    // drop tag marks, 75 percent of time
    if (crandomint(1, 100) < 75)
        StripTags(outputName);
    else
        cstrtrim(outputName);

    // sometimes switch name to lower characters
    if (crandomint(1, 100) < 50)
    {
        size_t i;
        for (i = 0; i < cstrlen(outputName); i++)
            outputName[i] = ctolower(outputName[i]); // to lower case
    }

    return &outputName[0]; // return terminated string
}

// this function humanize chat string to be more handwritten
void HumanizeChat(char* buffer)
{
    int length = cstrlen(buffer); // get length of string
    int i = 0;

    // sometimes switch text to lowercase
    if (crandomint(1, 2) == 1)
    {
        for (i = 0; i < length; i++)
            buffer[i] = static_cast<char>(ctolower(buffer[i])); // switch to lowercase
    }

    if (length > 15)
    {
        // "length / 2" percent of time drop a character
        if (crandomint(1, 100) < (length / 2))
        {
            const int pos = crandomint((length / 8), length - (length / 8)); // chose random position in string
            for (i = pos; i < length - 1; i++)
                buffer[i] = buffer[i + 1]; // overwrite the buffer with stripped string

            buffer[i] = 0; // terminate string;
            length--; // update new string length
        }

        // "length" / 4 precent of time swap character
        if (crandomint(1, 100) < (length / 4))
        {
            const int pos = crandomint((length / 8), ((3 * length) / 8)); // choose random position in string
            const char ch = buffer[pos]; // swap characters
            buffer[pos] = buffer[pos + 1];
            buffer[pos + 1] = ch;
        }
    }

    buffer[length] = 0; // terminate string
}

// this function parses messages from the botchat, replaces keywords and converts names into a more human style
void Bot::PrepareChatMessage(char* text)
{
    if (!ebot_chat.GetBool() || IsNullString(text))
        return;

    c::memset(&m_tempStrings, 0, sizeof(m_tempStrings));

    char* textStart = text;
    char* pattern = text;

    edict_t* talkEntity = nullptr;

    while (pattern != nullptr)
    {
        // all replacement placeholders start with a %
        pattern = cstrstr(textStart, "%");

        if (pattern != nullptr)
        {
            const int length = pattern - textStart;
            if (length > 0)
                cstrncpy(m_tempStrings, textStart, length);

            pattern++;

            // player with most frags?
            if (*pattern == 'f')
            {
                int highestFrags = -99999; // just pick some start value
                edict_t* entity = nullptr;

                for (const auto& client : g_clients)
                {
                    if (client.index < 0)
                        continue;

                    if (FNullEnt(client.ent))
                        continue;

                    if (!(client.flags & CFLAG_USED) || client.ent == pev->pContainingEntity)
                        continue;

                    const int frags = static_cast<int>(client.ent->v.frags);

                    if (frags > highestFrags)
                    {
                        highestFrags = frags;
                        entity = client.ent;
                    }
                }

                talkEntity = entity;

                if (!FNullEnt(talkEntity))
                    cstrcat(m_tempStrings, HumanizeName(const_cast<char*>(GetEntityName(talkEntity))));
            }
            // mapname?
            else if (*pattern == 'm')
                cstrcat(m_tempStrings, GetMapName());
            // roundtime?
            else if (*pattern == 'r')
            {
                const int time = static_cast<int>(g_timeRoundEnd - engine->GetTime());
                cstrcat(m_tempStrings, FormatBuffer("%02d:%02d", time / 60, time % 60));
            }
            // chat reply?
            else if (*pattern == 's')
            {
                talkEntity = INDEXENT(m_sayTextBuffer.entityIndex);

                if (!FNullEnt(talkEntity))
                    cstrcat(m_tempStrings, HumanizeName(const_cast<char*>(GetEntityName(talkEntity))));
            }
            // teammate alive?
            else if (*pattern == 't')
            {
                edict_t* entity = nullptr;

                for (const auto& client : g_clients)
                {
                    if (client.index < 0)
                        continue;

                    if (FNullEnt(client.ent))
                        continue;

                    if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team != m_team || client.ent == pev->pContainingEntity)
                        continue;

                    entity = client.ent;
                    break;
                }

                if (!FNullEnt(entity))
                {
                    if (!FNullEnt(pev->dmg_inflictor) && m_team == GetTeam(pev->dmg_inflictor))
                        talkEntity = pev->dmg_inflictor;
                    else
                        talkEntity = entity;

                    if (!FNullEnt(talkEntity))
                        cstrcat(m_tempStrings, HumanizeName(const_cast<char*>(GetEntityName(talkEntity))));
                }
                else // no teammates alive...
                {
                    for (const auto& client : g_clients)
                    {
                        if (client.index < 0)
                            continue;

                        if (FNullEnt(client.ent))
                            continue;

                        if (!(client.flags & CFLAG_USED) || client.team != m_team || client.ent == pev->pContainingEntity)
                            continue;

                        entity = client.ent;
                        break;
                    }

                    if (!FNullEnt(entity))
                    {
                        talkEntity = entity;

                        if (!FNullEnt(talkEntity))
                            cstrcat(m_tempStrings, HumanizeName(const_cast<char*>(GetEntityName(talkEntity))));
                    }
                }
            }
            else if (*pattern == 'e')
            {
                edict_t* entity = nullptr;

                for (const auto& client : g_clients)
                {
                    if (client.index < 0)
                        continue;

                    if (FNullEnt(client.ent))
                        continue;

                    if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || client.team == m_team || client.ent == pev->pContainingEntity)
                        continue;

                    entity = client.ent;
                    break;
                }

                if (!FNullEnt(entity))
                {
                    talkEntity = entity;

                    if (!FNullEnt(talkEntity))
                        cstrcat(m_tempStrings, HumanizeName(const_cast<char*>(GetEntityName(talkEntity))));
                }
                else // no teammates alive...
                {
                    for (const auto& client : g_clients)
                    {
                        if (client.index < 0)
                            continue;

                        if (FNullEnt(client.ent))
                            continue;

                        if (!(client.flags & CFLAG_USED) || client.team == m_team || client.ent == pev->pContainingEntity)
                            continue;

                        entity = client.ent;
                        break;
                    }

                    if (!FNullEnt(entity))
                    {
                        talkEntity = entity;

                        if (!FNullEnt(talkEntity))
                            cstrcat(m_tempStrings, HumanizeName(const_cast<char*>(GetEntityName(talkEntity))));
                    }
                }
            }
            else if (*pattern == 'd')
            {
                if (g_gameVersion & Game::CZero)
                {
                    if (crandomint(1, 10) < 4)
                        cstrcat(m_tempStrings, "cscz");
                    else
                        cstrcat(m_tempStrings, "Condition Zero");
                }
                else if (g_gameVersion & Game::CStrike)
                {
                    if (crandomint(1, 10) < 4)
                        cstrcat(m_tempStrings, "cs 1.6");
                    else
                        cstrcat(m_tempStrings, "Counter-Strike");
                }
                else if (g_gameVersion & Game::HalfLife)
                {
                    if (crandomint(1, 10) < 4)
                        cstrcat(m_tempStrings, "hl");
                    else
                        cstrcat(m_tempStrings, "Half-Life");
                }
                else
                    cstrcat(m_tempStrings, "this game");
            }
            else if (*pattern == 'v')
            {
                talkEntity = m_nearestEnemy;

                if (!FNullEnt(talkEntity))
                    cstrcat(m_tempStrings, HumanizeName(const_cast<char*>(GetEntityName(talkEntity))));
            }

            pattern++;
            textStart = pattern;
        }
    }

    // let the bots make some mistakes...
    char tempString[160];
    cstrncpy(tempString, textStart, sizeof(tempString));
    HumanizeChat(tempString);
    cstrcat(m_tempStrings, tempString);
}

// this function checks is string contain keyword, and generates relpy to it
bool Bot::CheckKeywords(char* tempMessage, char* reply)
{
    if (!ebot_chat.GetBool() || IsNullString(tempMessage))
        return false;

    ITERATE_ARRAY(g_replyFactory, i)
    {
        ITERATE_ARRAY(g_replyFactory[i].keywords, j)
        {
            // check is keyword has occurred in message
            if (cstrstr(tempMessage, g_replyFactory[i].keywords[j].GetBuffer()) != nullptr)
            {
                cstrcpy(reply, g_replyFactory[i].replies.GetRandomElement()); // update final buffer
                return true;
            }
        }
    }

    // didn't find a keyword? 80% of the time use some universal reply
    if (chanceof(80) && !g_chatFactory[CHAT_NOKW].IsEmpty())
    {
        cstrcpy(reply, g_chatFactory[CHAT_NOKW].GetRandomElement().GetBuffer());
        return true;
    }

    return false;
}

// this function parse chat buffer, and prepare buffer to keyword searching
bool Bot::ParseChat(char* reply)
{
    char tempMessage[512];
    cstrncpy(tempMessage, m_sayTextBuffer.sayText, sizeof(tempMessage)); // copy to safe place

    // text to uppercase for keyword parsing
    size_t i;
    for (i = 0; i < cstrlen(tempMessage); i++)
        tempMessage[i] = ctoupper(tempMessage[i]);

    return CheckKeywords(tempMessage, reply);
}

// this function sends reply to a player
bool Bot::RepliesToPlayer(void)
{
    if (m_sayTextBuffer.entityIndex != -1 && !IsNullString(m_sayTextBuffer.sayText))
    {
        char text[256];

        if (ParseChat(text))
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

    return false;
}

void Bot::ChatSay(const bool teamSay, const char* text, ...)
{
    // someone is using FakeClientCommand, don't break it.
    if (g_isFakeCommand)
        return;

    if (IsNullString(text))
        return;

    // block looping same message
    if (!IsNullString(m_lastStrings) && m_lastStrings == text)
        return;

    edict_t* me = GetEntity();

    // humanize chat
    if (m_lastChatEnt == me)
        return;

    FakeClientCommand(me, "%s \"%s\"", teamSay ? "say_team" : "say", text);
    cstrncpy(m_lastStrings, text, sizeof(m_lastStrings));
    m_lastChatEnt = me;
}