// ported from jk botti

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <memory.h>
#ifndef _WIN32
#include <sys/time.h>
#else
#include <chrono>
#endif

#include "core.h"
#include "bot_query_hook.h"

ConVar ebot_query_hook("ebot_query_hook", "0");

struct BotQueryTimeTracker
{
	bool active;
	char name[64];
	double stay_time;
	double connect_time;
};

static BotQueryTimeTracker g_queryTimes[32];

static double GetSecs(void)
{
#ifndef _WIN32
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return (double)tv.tv_sec + ((double)tv.tv_usec) / 1000000.0;
#else
	auto now = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration<double>(now).count();
#endif
}

void BotReplaceConnectionTime(const char* name, float* timeslot)
{
	// clean up inactive slots periodically
	int i, j;
	Bot* bot;
	bool found;
	for (i = 0; i < 32; i++)
	{
		if (g_queryTimes[i].active)
		{
			found = false;
			for (j = 0; j < 32; j++)
			{
				bot = g_botManager->GetBot(j);
				if (bot)
				{
					if (cstrcmp(GetEntityName(bot->GetEntity()), g_queryTimes[i].name) == 0)
					{
						found = true;
						break;
					}
				}
			}

			if (!found)
				g_queryTimes[i].active = false;
		}
	}

	// make sure target name is an active ebot
	bool is_active_bot = false;
	for (i = 0; i < 32; i++)
	{
		bot = g_botManager->GetBot(i);
		if (bot)
		{
			if (cstrcmp(GetEntityName(bot->GetEntity()), name) == 0)
			{
				is_active_bot = true;
				break;
			}
		}
	}

	if (!is_active_bot)
		return;

	double current_time = GetSecs();

	// find or allocate slot
	int slot = -1;
	int free_slot = -1;
	for (i = 0; i < 32; i++)
	{
		if (g_queryTimes[i].active && cstrcmp(g_queryTimes[i].name, name) == 0)
		{
			slot = i;
			break;
		}

		if (!g_queryTimes[i].active && free_slot == -1)
			free_slot = i;
	}

	if (slot == -1)
	{
		if (free_slot != -1)
		{
			slot = free_slot;
			g_queryTimes[slot].active = true;
			cstrcpy(g_queryTimes[slot].name, sizeof(g_queryTimes[slot].name), name);
			g_queryTimes[slot].stay_time = 60.0 * (double)(30 + (rand() % 131));
			double ratio = 0.2 + (double)(rand() % 61) / 100.0;
			g_queryTimes[slot].connect_time = current_time - g_queryTimes[slot].stay_time * ratio;
		}
		else
			return;
	}

	BotQueryTimeTracker &tracker = g_queryTimes[slot];

	// check if stay time has been exceeded
	if (current_time - tracker.connect_time > tracker.stay_time || current_time - tracker.connect_time <= 0)
	{
		tracker.stay_time = 60.0 * (double)(30 + (rand() % 131));
		double ratio = 0.2 + (double)(rand() % 61) / 100.0;
		tracker.connect_time = current_time - tracker.stay_time * ratio;
	}

	*timeslot = (float)(current_time - tracker.connect_time);
}

static bool msg_get_string(const unsigned char* &msg, size_t &len, char* name, int nlen)
{
	int i = 0;
	while (*msg != 0)
	{
		if (i < nlen)
			name[i++] = *msg;

		msg++;
		if (--len == 0)
			return false;
	}

	if (i < nlen)
		name[i] = 0;
	else if (nlen > 0)
		name[nlen - 1] = 0;

	msg++;
	if (--len == 0)
		return false;

	return true;
}

ssize_t PASCAL handle_player_reply(int socket, const void* message, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t dest_len)
{
	if (length > 4096)
		return call_original_sendto(socket, message, length, flags, dest_addr, dest_len);

	unsigned char* newmsg = (unsigned char*)alloca(length);
	if (!newmsg)
		return call_original_sendto(socket, message, length, flags, dest_addr, dest_len);

	cmemcpy(newmsg, message, length);

	size_t len = length - 5;
	const unsigned char* msg = (const unsigned char*)message + 5;

	int i, pcount;
	size_t offset;
	char pname[64];

	pcount = *msg++;
	if (--len == 0)
		return call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len);

	for (i = 0; i < pcount; i++)
	{
		msg++;
		if (--len == 0)
			return call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len);

		cmemset(pname, 0, sizeof(pname));

		if (!msg_get_string(msg, len, pname, sizeof(pname)))
			return call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len);

		if (len <= 4)
			return call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len);

		msg += 4;
		len -= 4;

		if (len < 4)
			return call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len);

		offset = (size_t)msg - (size_t)message;

		float timeval;
		cmemcpy(&timeval, &newmsg[offset], sizeof(float));
		BotReplaceConnectionTime(pname, &timeval);
		cmemcpy(&newmsg[offset], &timeval, sizeof(float));

		msg += 4;
		len -= 4;

		if (len == 0)
			return call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len);
	}

	return call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len);
}

ssize_t PASCAL handle_goldsrc_server_info_reply(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
	if (length > 4096)
		return call_original_sendto(socket, message, length, flags, dest_addr, dest_len);

	unsigned char* newmsg = (unsigned char*)alloca(length);
	if (!newmsg)
		return call_original_sendto(socket, message, length, flags, dest_addr, dest_len);

	cmemcpy(newmsg, message, length);

	ssize_t len = length - 5;
	unsigned char* msg = (unsigned char*)newmsg + 5;
	bool is_mod;

	while (len > 0)
	{
		if (*msg == 0)
		{
			msg++;
			len--;
			break;
		}

		msg++;
		len--;
	}

	if (len <= 0)
		goto out;

	while (len > 0)
	{
		if (*msg == 0)
		{
			msg++;
			len--;
			break;
		}

		msg++;
		len--;
	}

	if (len <= 0)
		goto out;

	while (len > 0)
	{
		if (*msg == 0)
		{
			msg++;
			len--;
			break;
		}

		msg++;
		len--;
	}

	if (len <= 0)
		goto out;

	while (len > 0)
	{
		if (*msg == 0)
		{
			msg++;
			len--;
			break;
		}

		msg++;
		len--;
	}

	if (len <= 0)
		goto out;

	while (len > 0)
	{
		if (*msg == 0)
		{
			msg++;
			len--;
			break;
		}

		msg++;
		len--;
	}

	if (len <= 0)
		goto out;

	// players
	msg++;
	len--;

	if (len <= 0)
		goto out;

	// max players
	if (*msg != engine->GetMaxClients())
		goto out;

	msg++;
	len--;

	if (len <= 0)
		goto out;

	// protocol
	msg++;
	len--;

	if (len <= 0)
		goto out;

	// server type
	if (*msg != 'D' && *msg != 'd' && *msg != 'L' && *msg != 'l')
		goto out;

	msg++;
	len--;

	if (len <= 0)
		goto out;

	// environment
	if (*msg != 'W' && *msg != 'w' && *msg != 'L' && *msg != 'l')
		goto out;

	msg++;
	len--;

	if (len <= 0)
		goto out;

	// visibility
	if (*msg != 0 && *msg != 1)
		goto out;

	msg++;
	len--;

	if (len <= 0)
		goto out;

	// is mod
	is_mod = *msg;
	if (is_mod != 0 && is_mod != 1)
		goto out;

	msg++;
	len--;

	if (len <= 0)
		goto out;

	if (is_mod)
	{
		while (len > 0)
		{
			if (*msg == 0)
			{
				msg++;
				len--;
				break;
			}

			msg++;
			len--;
		}

		if (len <= 0)
			goto out;

		while (len > 0)
		{
			if (*msg == 0)
			{
				msg++;
				len--;
				break;
			}

			msg++;
			len--;
		}

		if (len <= 0)
			goto out;

		if (*msg != 0)
			goto out;

		msg++;
		len--;

		if (len <= 0)
			goto out;

		msg += 4;
		len -= 4;

		if (len <= 0)
			goto out;

		msg += 4;
		len -= 4;

		if (len <= 0)
			goto out;

		if (*msg != 0 && *msg != 1)
			goto out;

		msg++;
		len--;

		if (len <= 0)
			goto out;

		if (*msg != 0 && *msg != 1)
			goto out;

		msg++;
		len--;

		if (len <= 0)
			goto out;
	}

	// VAC
	if (*msg != 0 && *msg != 1)
		goto out;

	msg++;
	len--;

	if (len <= 0)
		goto out;

	// replace bot count with 0
	*msg = 0;

out:
	return call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len);
}

ssize_t PASCAL handle_source_server_info_reply(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
	if (length > 4096)
		return call_original_sendto(socket, message, length, flags, dest_addr, dest_len);

	unsigned char* newmsg = (unsigned char*)alloca(length);
	if (!newmsg)
		return call_original_sendto(socket, message, length, flags, dest_addr, dest_len);

	cmemcpy(newmsg, message, length);

	ssize_t len = length - 5;
	unsigned char* msg = (unsigned char*)newmsg + 5;

	// protocol
	msg++;
	len--;

	if (len <= 0)
		goto out;

	while (len > 0)
	{
		if (*msg == 0)
		{
			msg++;
			len--;
			break;
		}

		msg++;
		len--;
	}

	if (len <= 0)
		goto out;

	while (len > 0)
	{
		if (*msg == 0)
		{
			msg++;
			len--;
			break;
		}

		msg++;
		len--;
	}

	if (len <= 0)
		goto out;

	while (len > 0)
	{
		if (*msg == 0)
		{
			msg++;
			len--;
			break;
		}

		msg++;
		len--;
	}

	if (len <= 0)
		goto out;

	while (len > 0)
	{
		if (*msg == 0)
		{
			msg++;
			len--;
			break;
		}

		msg++;
		len--;
	}

	if (len <= 0)
		goto out;

	// steam id
	msg++;
	len--;

	if (len <= 0)
		goto out;

	msg++;
	len--;

	if (len <= 0)
		goto out;

	// players
	msg++;
	len--;

	if (len <= 0)
		goto out;

	// max players
	if (*msg != g_pGlobals->maxClients)
		goto out;

	msg++;
	len--;

	if (len <= 0)
		goto out;

	// replace bot count with 0
	*msg = 0;

out:
	return call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len);
}

ssize_t PASCAL sendto_hook(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
	const unsigned char* orig_buf = (unsigned char*)message;

	if (!g_pGlobals)
		goto out;

	if (!ebot_query_hook.GetBool())
		goto out;

	if (length > 5 && orig_buf[0] == 0xff && orig_buf[1] == 0xff && orig_buf[2] == 0xff && orig_buf[3] == 0xff)
	{
		if (orig_buf[4] == 'm')
			return handle_goldsrc_server_info_reply(socket, message, length, flags, dest_addr, dest_len);

		if (orig_buf[4] == 'I')
			return handle_source_server_info_reply(socket, message, length, flags, dest_addr, dest_len);

		if (orig_buf[4] == 'D')
			return handle_player_reply(socket, message, length, flags, dest_addr, dest_len);
	}

out:
	return call_original_sendto(socket, message, length, flags, dest_addr, dest_len);
}
