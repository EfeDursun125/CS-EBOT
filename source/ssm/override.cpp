#include "../../include/core.h"

void Bot::OverrideUpdate(void)
{
    if (m_overrideDefaultChecks && m_isSlowThink)
	{
		FindEnemyEntities();
		FindFriendsAndEnemiens();
		CheckReachable();
    }

    if (m_overrideDefaultLookAI)
        UpdateLooking();
}

void Bot::OverrideEnd(void)
{
    m_overrideDefaultChecks = true;
    m_overrideDefaultLookAI = true;
    m_overrideID = 0;
}