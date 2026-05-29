#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <core.h>

struct MenuAnswer
{
    CArray<int> options;
    char menuName[64];
    char optionsString[128];

    bool HasOption(int option) const;
    void ParseOptions();
};

class MenuManager
{
public:
    CArray<MenuAnswer> m_menus;

    void Load(const char* filename);
    int FindAndSelect(const char* menuTitle);
    int SelectRandomOption(const char* menuTitle);
};

extern MenuManager g_menuManager;

#endif