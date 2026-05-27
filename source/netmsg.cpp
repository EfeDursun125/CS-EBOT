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

#include "../include/core.h"

inline void RoundEndMessage(void)
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
			bot->m_isEnemyReachable = false;
			bot->m_isZombieBot = true;
			bot->m_team = Team::Terrorist;
		}
	}
}

NetworkMsg::NetworkMsg(void)
{
	m_message = NETMSG_UNDEFINED;
	m_bot = nullptr;

	for (auto& message : m_registerdMessages)
		message = NETMSG_UNDEFINED;
}

void NetworkMsg::HandleMessageIfRequired(const int messageType, const int requiredType)
{
	if (messageType == m_registerdMessages[requiredType])
		SetMessage(requiredType);
}

void NetworkMsg::Execute(void)
{
	if (m_message == NETMSG_UNDEFINED)
		return;

	switch (m_message)
	{
		case NETMSG_VGUI:
		{
			if (m_args.Size() < 1)
				break;

			const int menuType = m_args[0].longVal;
			switch (menuType)
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
			break;
		}

		case NETMSG_SHOWMENU:
		{
			if (m_args.Size() < 4 || !m_bot)
				break;

			const char* x = m_args[3].strVal;
			if (!x)
				break;

			switch (x[1])
			{
				case 'T':
				{
					switch (charToInt(const_cast<char*>(x)))
					{
						case 1010:
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
					switch (charToInt(const_cast<char*>(x)))
					{
						case 1866:
						case 1260:
						case 1594:
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
					if (charToInt(const_cast<char*>(x)) == 787)
					{
						m_bot->m_startAction = CMENU_CLASS;
					}
					break;
				}
			}
			break;
		}

		case NETMSG_WLIST:
		{
			if (m_args.Size() < 9)
				break;

			WeaponProperty weaponProp;
			cstrcpy(weaponProp.className, sizeof(weaponProp.className), m_args[0].strVal);
			weaponProp.ammo1 = m_args[1].longVal;
			weaponProp.ammo1Max = m_args[2].longVal;
			weaponProp.slotID = m_args[5].longVal;
			weaponProp.position = m_args[6].longVal;
			weaponProp.id = m_args[7].longVal;
			weaponProp.flags = m_args[8].longVal;

			if (weaponProp.id >= 0 && weaponProp.id < Const_MaxWeapons)
			{
				g_weaponDefs[weaponProp.id] = weaponProp;
			}
			break;
		}

		case NETMSG_CURWEAPON:
		{
			if (m_args.Size() < 3 || !m_bot)
				break;

			const int state = m_args[0].longVal;
			const int id = m_args[1].longVal;
			const int clip = m_args[2].longVal;

			if (id >= 0 && id < Const_MaxWeapons)
			{
				if (state != 0)
					m_bot->m_currentWeapon = id;
				m_bot->m_ammoInClip[id] = clip;
			}
			break;
		}

		case NETMSG_AMMOX:
		{
			if (m_args.Size() < 2 || !m_bot)
				break;

			const int index = m_args[0].longVal;
			const int value = m_args[1].longVal;
			if (index >= 0 && index < Const_MaxWeapons)
			{
				m_bot->m_ammo[index] = value;
			}
			break;
		}

		case NETMSG_AMMOPICK:
		{
			if (m_args.Size() < 2 || !m_bot)
				break;

			const int index = m_args[0].longVal;
			const int value = m_args[1].longVal;
			if (index >= 0 && index < Const_MaxWeapons)
			{
				m_bot->m_ammo[index] = value;
			}
			break;
		}

		case NETMSG_DAMAGE:
		{
			if (m_args.Size() < 3 || !m_bot)
				break;

			m_bot->IgnoreCollisionShortly();
			break;
		}

		case NETMSG_DEATH:
		{
			if (m_args.Size() < 2)
				break;

			const int victimerIndex = m_args[1].longVal;
			Bot* victimer = g_botManager->GetBot(victimerIndex - 1);
			if (victimer)
			{
				// CRITICAL FIX: DO NOT reset ammo/weapons here when bot dies.
				// Resetting them now causes nullptr crash during CHalfLifeMultiplay::PlayerThink or CBasePlayer::PreThink
				// because CS GameDLL is still transitioning the bot to spectator mode.
				// Re-init must only happen during stable phases (NewRound).
				victimer->m_isAlive = false;
				victimer->m_navNode.Destroy();
				victimer->m_currentWeapon = 0;
			}
			break;
		}

		case NETMSG_SCREENFADE:
		{
			if (m_args.Size() < 7 || !m_bot)
				break;

			const uint8_t r = m_args[3].longVal;
			const uint8_t g = m_args[4].longVal;
			const uint8_t b = m_args[5].longVal;
			const uint8_t alpha = m_args[6].longVal;

			m_bot->TakeBlinded(Vector(r, g, b), alpha);
			break;
		}

		case NETMSG_HLTV:
		{
			if (m_args.Size() < 2)
				break;

			const int numPlayers = m_args[0].longVal;
			const int fov = m_args[1].longVal;

			if (!numPlayers && !fov)
				RoundInit();
			break;
		}

		case NETMSG_TEXTMSG:
		{
			if (m_args.Size() < 2)
				break;

			const char* x = m_args[1].strVal;
			if (!x)
				break;

			switch (x[1])
			{
				case 'C':
				{
					if (!cstrncmp(x, "#CTs_Win", 9))
						RoundEndMessage();
					break;
				}
				case 'T':
				{
					if (!cstrncmp(x, "#Terrorists_Win", 16))
						RoundEndMessage();
					break;
				}
				case 'R':
				{
					if (!cstrncmp(x, "#Round_Draw", 12))
						RoundEndMessage();
					break;
				}
				case 'G':
				{
					if (!cstrncmp(x, "#Game_Commencing", 17) || !cstrncmp(x, "#Game_will_restart_in", 22))
						RoundEndMessage();
					break;
				}
			}
			break;
		}
	}
}
