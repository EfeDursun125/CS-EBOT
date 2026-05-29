#include "../include/menu_manager.h"
#include <clib.h>

bool MenuAnswer::HasOption(int option) const
{
	for (int i = 0; i < options.Size(); i++)
	{
		if (options[i] == option)
			return true;
	}
	return false;
}

void MenuAnswer::ParseOptions()
{
	options.Destroy();

	if (optionsString[0] == '\0')
		return;

	char temp[16];
	int tempIdx = 0;

	for (int i = 0; optionsString[i] != '\0'; i++)
	{
		char c = optionsString[i];
		if (c == ',' || c == ' ')
		{
			if (tempIdx > 0)
			{
				temp[tempIdx] = '\0';
				options.Push(atoi(temp));
				tempIdx = 0;
			}
		}
		else
		{
			temp[tempIdx++] = c;
			if (tempIdx >= 16)
				break;
		}
	}

	if (tempIdx > 0)
	{
		temp[tempIdx] = '\0';
		options.Push(atoi(temp));
	}
}

void MenuManager::Load(const char* filename)
{
	m_menus.Destroy();

	FILE* fp = fopen(filename, "r");
	if (!fp)
		return;

	char line[512];
	while (fgets(line, sizeof(line), fp))
	{
		if (line[0] == '/' && line[1] == '/')
			continue;

		if (!cstrncmp(line, "menu_answer", 11))
		{
			const char* nameStart = line + 12;
			if (*nameStart != '"')
				continue;

			nameStart++;
			const char* nameEnd = nameStart;
			while (*nameEnd != '"' && *nameEnd != '\0')
				nameEnd++;

			if (*nameEnd != '"')
				continue;

			MenuAnswer answer;
			int nameLen = nameEnd - nameStart;
			if (nameLen >= 64)
				nameLen = 63;

			for (int i = 0; i < nameLen; i++)
				answer.menuName[i] = nameStart[i];
			answer.menuName[nameLen] = '\0';

			const char* optsStart = nameEnd + 2;
			if (*optsStart != '"')
				continue;

			optsStart++;
			const char* optsEnd = optsStart;
			while (*optsEnd != '"' && *optsEnd != '\0')
				optsEnd++;

			if (*optsEnd != '"')
				continue;

			int optsLen = optsEnd - optsStart;
			if (optsLen >= 128)
				optsLen = 127;

			for (int i = 0; i < optsLen; i++)
				answer.optionsString[i] = optsStart[i];
			answer.optionsString[optsLen] = '\0';
			answer.ParseOptions();

			m_menus.Push(answer);
		}
	}

	fclose(fp);
}

int MenuManager::FindAndSelect(const char* menuTitle)
{
	for (int i = 0; i < m_menus.Size(); i++)
	{
		if (cstrstr(const_cast<char*>(menuTitle), m_menus[i].menuName))
		{
			if (m_menus[i].options.Size() == 0)
				return 0;

			return m_menus[i].options.Random();
		}
	}
	return 0;
}

int MenuManager::SelectRandomOption(const char* menuTitle)
{
	return FindAndSelect(menuTitle);
}

MenuManager g_menuManager;