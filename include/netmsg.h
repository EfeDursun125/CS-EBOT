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
// all copies or substantial portions of the Software.ebot_aim_spring_stiffness_y
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

// netmessage functions
enum NetMsg
{
    NETMSG_UNDEFINED = -1,
    NETMSG_VGUI = 1,
    NETMSG_SHOWMENU = 2,
    NETMSG_WLIST = 3,
    NETMSG_CURWEAPON = 4,
    NETMSG_AMMOX = 5,
    NETMSG_AMMOPICK = 6,
    NETMSG_DAMAGE = 7,
    NETMSG_MONEY = 8,
    NETMSG_STATUSICON = 9,
    NETMSG_DEATH = 10,
    NETMSG_SCREENFADE = 11,
    NETMSG_HLTV = 12,
    NETMSG_TEXTMSG = 13,
    NETMSG_SCOREINFO = 14,
    NETMSG_BARTIME = 15,
    NETMSG_SENDAUDIO = 17,
    NETMSG_SAYTEXT = 18,
    NETMSG_BOTVOICE = 19,
    NETMSG_NUM = 21
};

// netmessage handler class
class NetworkMsg : public Singleton <NetworkMsg>
{
private:
	Bot* m_bot;
	int m_state;
	int m_message;
	int m_registerdMessages[NETMSG_NUM];

public:
	NetworkMsg(void);
	~NetworkMsg(void) {};

	void Execute(void* p);
	void Reset(void) { m_message = NETMSG_UNDEFINED; m_state = 0; m_bot = nullptr; };
	void HandleMessageIfRequired(const int messageType, const int requiredType);

	void SetMessage(const int message) { m_message = message; }
	void SetBot(Bot* bot) { m_bot = bot; }

	int GetId(const int messageType) { return m_registerdMessages[messageType]; }
	void SetId(const int messageType, const int messsageIdentifier) { m_registerdMessages[messageType] = messsageIdentifier; }
};

#define g_netMsg NetworkMsg::GetObjectPtr()