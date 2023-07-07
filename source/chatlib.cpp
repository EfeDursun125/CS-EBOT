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
    for (index = 0; index < ARRAYSIZE_HLSDK(tagOpen); index++)
    {
        fieldStart = cstrstr(buffer, tagOpen[index]) - buffer; // look for a tag start

        // have we found a tag start?
        if (fieldStart >= 0 && fieldStart < 32)
        {
            fieldStop = cstrstr(buffer, tagClose[index]) - buffer; // look for a tag stop

            // have we found a tag stop?
            if ((fieldStop > fieldStart) && (fieldStop < 32))
            {
                for (i = fieldStart; i < length - (fieldStop + static_cast<int>(cstrlen(tagClose[index])) - fieldStart); i++)
                    buffer[i] = buffer[i + (fieldStop + cstrlen(tagClose[index]) - fieldStart)]; // overwrite the buffer with the stripped string

                buffer[i] = 0x0; // terminate the string
            }
        }
    }

    // have we stripped too much (all the stuff)?
    if (cstrlen(buffer) != 0)
    {
        cstrtrim(buffer); // if so, string is just a tag

        // strip just the tag part..
        for (index = 0; index < ARRAYSIZE_HLSDK(tagOpen); index++)
        {
            fieldStart = cstrstr(buffer, tagOpen[index]) - buffer; // look for a tag start

            // have we found a tag start?
            if (fieldStart >= 0 && fieldStart < 32)
            {
                fieldStop = fieldStart + cstrlen(tagOpen[index]); // set the tag stop

                for (i = fieldStart; i < length - static_cast<int>(cstrlen(tagOpen[index])); i++)
                    buffer[i] = buffer[i + cstrlen(tagOpen[index])]; // overwrite the buffer with the stripped string

                buffer[i] = 0x0; // terminate the string

                fieldStart = cstrstr(buffer, tagClose[index]) - buffer; // look for a tag stop

                // have we found a tag stop ?
                if (fieldStart >= 0 && fieldStart < 32)
                {
                    fieldStop = fieldStart + cstrlen(tagClose[index]); // set the tag stop

                    for (i = fieldStart; i < length - static_cast<int>(cstrlen(tagClose[index])); i++)
                        buffer[i] = buffer[i + static_cast<int>(cstrlen(tagClose[index]))]; // overwrite the buffer with the stripped string

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
    static char outputName[256]; // create return name buffer
    cstrcpy(outputName, name); // copy name to new buffer

    // drop tag marks, 75 percent of time
    if (CRandomInt(1, 100) < 75)
        StripTags(outputName);
    else
        cstrtrim(outputName);

    // sometimes switch name to lower characters
    if (CRandomInt(1, 100) < 50)
    {
        for (int i = 0; i < static_cast<int>(cstrlen(outputName)); i++)
            outputName[i] = static_cast<char>(ctolower(outputName[i])); // to lower case
    }

    return &outputName[0]; // return terminated string
}

// this function parses messages from the botchat, replaces keywords and converts names into a more human style
void Bot::PrepareChatMessage(char* text)
{
    if (!ebot_chat.GetBool() || IsNullString(text))
        return;

    cmemset(&m_tempStrings, 0, sizeof(m_tempStrings));

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
                    if (FNullEnt(client.ent))
                        continue;

                    if (!(client.flags & CFLAG_USED) || client.ent == GetEntity())
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
                    if (FNullEnt(client.ent))
                        continue;

                    if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || (client.team != m_team) || (client.ent == GetEntity()))
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
                        cstrcat(m_tempStrings, HumanizeName(const_cast <char*> (GetEntityName(talkEntity))));
                }
                else // no teammates alive...
                {
                    for (const auto& client : g_clients)
                    {
                        if (FNullEnt(client.ent))
                            continue;

                        if (!(client.flags & CFLAG_USED) || (client.team != m_team) || (client.ent == GetEntity()))
                            continue;

                        entity = client.ent;

                        break;
                    }

                    if (!FNullEnt(entity))
                    {
                        talkEntity = entity;

                        if (!FNullEnt(talkEntity))
                            cstrcat(m_tempStrings, HumanizeName(const_cast <char*> (GetEntityName(talkEntity))));
                    }
                }
            }
            else if (*pattern == 'e')
            {
                edict_t* entity = nullptr;

                for (const auto& client : g_clients)
                {
                    if (FNullEnt(client.ent))
                        continue;

                    if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) || (client.team == m_team) || (client.ent == GetEntity()))
                        continue;

                    entity = client.ent;

                    break;
                }

                if (!FNullEnt(entity))
                {
                    talkEntity = entity;

                    if (!FNullEnt(talkEntity))
                        cstrcat(m_tempStrings, HumanizeName(const_cast <char*> (GetEntityName(talkEntity))));
                }
                else // no teammates alive...
                {
                    for (const auto& client : g_clients)
                    {
                        if (FNullEnt(client.ent))
                            continue;

                        if (!(client.flags & CFLAG_USED) || (client.team == m_team) || (client.ent == GetEntity()))
                            continue;

                        entity = client.ent;
                        break;
                    }

                    if (!FNullEnt(entity))
                    {
                        talkEntity = entity;

                        if (!FNullEnt(talkEntity))
                            cstrcat(m_tempStrings, HumanizeName(const_cast <char*> (GetEntityName(talkEntity))));
                    }
                }
            }
            else if (*pattern == 'd')
            {
                if (g_gameVersion == CSVER_CZERO)
                {
                    if (CRandomInt(1, 10) <= 3)
                        cstrcat(m_tempStrings, "CZ");
                    else
                        cstrcat(m_tempStrings, "Condition Zero");
                }
                else if ((g_gameVersion == CSVER_CSTRIKE) || (g_gameVersion == CSVER_VERYOLD))
                {
                    if (CRandomInt(1, 10) <= 3)
                        cstrcat(m_tempStrings, "CS");
                    else
                        cstrcat(m_tempStrings, "Counter-Strike");
                }
            }
            else if (*pattern == 'v')
            {
                talkEntity = m_lastVictim;

                if (!FNullEnt(talkEntity))
                    cstrcat(m_tempStrings, HumanizeName(const_cast <char*> (GetEntityName(talkEntity))));
            }

            pattern++;
            textStart = pattern;
        }
    }

    // let the bots make some mistakes...
    char tempString[160];
    cstrncpy(tempString, textStart, 159);
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
    if (ChanceOf(80) && !g_chatFactory[CHAT_NOKW].IsEmpty())
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
    cstrcpy(tempMessage, m_sayTextBuffer.sayText); // copy to safe place

    // text to uppercase for keyword parsing
    for (int i = 0; i < static_cast<int>(cstrlen(tempMessage)); i++)
        tempMessage[i] = toupper(tempMessage[i]);

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
    if (IsNullString(text))
        return;

    // block looping same message
    if (!IsNullString(m_lastStrings) && m_lastStrings == text)
        return;

    // humanize chat
    if (m_lastChatEnt == GetEntity())
        return;

    char botName[80];
    char botTeam[22];
    char tempMessage[256];

    if (g_gameVersion != HALFLIFE)
    {
        if (teamSay)
        {
            if (m_team == TEAM_TERRORIST)
                cstrcpy(botTeam, "(Terrorist)");
            else if (m_team == TEAM_COUNTER)
                cstrcpy(botTeam, "(Counter-Terrorist)");
            else // WHAT???
                cstrcpy(botTeam, "");
        }
        else
            cstrcpy(botTeam, "");
    }

    cstrcpy(botName, GetEntityName(GetEntity()));

    for (const auto& client : g_clients)
    {
        if (FNullEnt(client.ent))
            continue;

        if (!(client.flags & CFLAG_USED) || client.ent == GetEntity())
            continue;

        if (teamSay && client.team != m_team)
            continue;

        if (g_gameVersion == HALFLIFE)
            sprintf(tempMessage, "%c%s: %s\n", 0x02, botName, text);
        else if (!m_isAlive)
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

        MessageSender(MSG_ONE, g_netMsg->GetId(NETMSG_SAYTEXT), nullptr, client.ent)
            .WriteByte(m_index - 1)
            .WriteString(tempMessage);
    }
}