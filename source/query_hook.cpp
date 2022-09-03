//
// JK_Botti - be more human!
//
// bot_query_hook.cpp
//

#ifndef _WIN32
#include <string.h>
#endif

#include <malloc.h>
#include <stdlib.h>
#include <memory.h>

#include "query_hook.h"

//
static bool msg_get_string(const unsigned char*& msg, size_t& len, char* name, int nlen)
{
	int i = 0;

	while (*msg != 0)
	{
		if (i < nlen)
			name[i++] = *msg;

		msg++;
		if (--len == 0)
			return(false);
	}

	if (i < nlen)
		name[i] = 0;

	msg++;
	if (--len == 0)
		return(false);

	return(true);
}

// find bot connection times and replace
size_t PASCAL handle_player_reply(int socket, const void* message, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t dest_len)
{
	/*
	(int32) Header
	(char) Character 'D'
	--
	(byte) Player Count
	(byte) Player Number
	(string) Player Name
	(int32) Player Score
	(float32) Player Time
	(byte) Player Number
	(string) Player Name
	(int32) Player Score
	(float32) Player Time
	*/
	unsigned char* newmsg = (unsigned char*)malloc(length);
	memcpy(newmsg, message, length);

	size_t len = length - 5;
	const unsigned char* msg = (const unsigned char*)message + 5;

	int i, pcount;
	size_t offset;
	char pname[64];

	//get player count
	pcount = *msg++;
	if (--len == 0)
		return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));

	//parse player slots
	for (i = 0; i < pcount; i++)
	{
		// skip player number
		msg++;
		if (--len == 0)
			return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));

		memset(pname, 0, sizeof(pname));

		// detect bot by name
		if (!msg_get_string(msg, len, pname, sizeof(pname)))
			return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));

		// skip score
		if (len <= 4)
			return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));
		msg += 4;
		len -= 4;

		// check that there is enough bytes left
		if (len < 4)
			return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));

		offset = (size_t)msg - (size_t)message;

		BotReplaceConnectionTime(pname, (float*)&newmsg[offset]);

		msg += 4;
		len -= 4;

		if (len == 0)
			return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));
	}

	return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));
}

// find number of bots and replace with zero
size_t PASCAL handle_goldsrc_server_info_reply(int socket, const void* message, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t dest_len)
{
	/*
	Header 		byte 	Always equal to 'm' (0x6D.)
	Address 	string 	IP address and port of the server.
	Name 		string 	Name of the server.
	Map 		string 	Map the server has currently loaded.
	Folder 		string 	Name of the folder containing the game files.
	Game 		string 	Full name of the game.
	Players 	byte 	Number of players on the server.
	Max. Players 	byte 	Maximum number of players the server reports it can hold.
	Protocol 	byte 	Protocol version used by the server.
	Server type 	byte 	Indicates the type of server:
		'D' for dedicated server
		'L' for non-dedicated server
		'P' for a HLTV server
	Environment 	byte 	Indicates the operating system of the server:
		'L' for Linux
		'W' for Windows
	Visibility 	byte 	Indicates whether the server requires a password:
		0 for public
		1 for private
	Mod 		byte 	Indicates whether the game is a mod:
		0 for Half-Life
		1 for Half-Life mod
		These fields are only present in the response if "Mod" is 1:
		Data 		Type 	Comment
		Link	 	string 	URL to mod website.
		Download Link 	string 	URL to download the mod.
		NULL	 	byte 	NULL byte (0x00)
		Version 	long 	Version of mod installed on server.
		Size 		long 	Space (in bytes) the mod takes up.
		Type	 	byte 	Indicates the type of mod:
			0 for single and multiplayer mod
			1 for multiplayer only mod
		DLL	 	byte 	Indicates whether mod uses its own DLL:
			0 if it uses the Half-Life DLL
			1 if it uses its own DLL
	VAC 		byte 	Specifies whether the server uses VAC:
		0 for unsecured
		1 for secured
	Bots	 	byte 	Number of bots on the server.
	 */
	unsigned char* newmsg = (unsigned char*)malloc(length);
	memcpy(newmsg, message, length);

	size_t len = length - 5;
	unsigned char* msg = (unsigned char*)newmsg + 5;

	bool is_mod;

	// skip IP address and port
	while (len > 0) {
		if (*msg == 0) {
			msg++;
			len--;
			break;
		}
		msg++;
		len--;
	}
	if (len <= 0)
		goto out; /* something is not right here */

	// skip server name
	// TODO: verify
	while (len > 0) {
		if (*msg == 0) {
			msg++;
			len--;
			break;
		}
		msg++;
		len--;
	}
	if (len <= 0)
		goto out; /* something is not right here */

	// skip map name
	// TODO: verify
	while (len > 0) {
		if (*msg == 0) {
			msg++;
			len--;
			break;
		}
		msg++;
		len--;
	}
	if (len <= 0)
		goto out; /* something is not right here */

	// skip game folder name (valve, gearbox, etc)
	// TODO: verify
	while (len > 0) {
		if (*msg == 0) {
			msg++;
			len--;
			break;
		}
		msg++;
		len--;
	}
	if (len <= 0)
		goto out; /* something is not right here */

	// skip game name (Half-life, severian's mod, might be modified by metamod-plugins)
	while (len > 0) {
		if (*msg == 0) {
			msg++;
			len--;
			break;
		}
		msg++;
		len--;
	}
	if (len <= 0)
		goto out; /* something is not right here */

	// Next we have bytes: players, max.players, protocol, server type,
	// environment, visibility, 'mod'.

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

	// server type, dedicated or listen
	if (*msg != 'D' && *msg != 'd' && *msg != 'L' && *msg != 'l')
		goto out;
	msg++;
	len--;
	if (len <= 0)
		goto out;

	// environment, linux or windows
	if (*msg != 'W' && *msg != 'W' && *msg != 'L' && *msg != 'l')
		goto out;
	msg++;
	len--;
	if (len <= 0)
		goto out;

	// visibility, 0 or 1
	if (*msg != 0 && *msg != 1)
		goto out;
	msg++;
	len--;
	if (len <= 0)
		goto out;

	// is mod?
	is_mod = *msg;
	if (is_mod != 0 && is_mod != 1)
		goto out;
	msg++;
	len--;
	if (len <= 0)
		goto out;

	// if is mod, we have extra fields...
	if (is_mod) {
		// skip, mod homepage url
		while (len > 0) {
			if (*msg == 0) {
				msg++;
				len--;
				break;
			}
			msg++;
			len--;
		}
		if (len <= 0)
			goto out; /* something is not right here */

		// skip, mod download url
		while (len > 0) {
			if (*msg == 0) {
				msg++;
				len--;
				break;
			}
			msg++;
			len--;
		}
		if (len <= 0)
			goto out; /* something is not right here */

		// NULL
		if (*msg != 0)
			goto out;
		msg++;
		len--;
		if (len <= 0)
			goto out;

		// long, version
		msg += 4;
		len -= 4;
		if (len <= 0)
			goto out;

		// long, size
		msg += 4;
		len -= 4;
		if (len <= 0)
			goto out;

		// type, single&multi player == 0, multi only == 1
		if (*msg != 0 && *msg != 1)
			goto out;
		msg++;
		len--;
		if (len <= 0)
			goto out;

		// dll, 0 or 1
		if (*msg != 0 && *msg != 1)
			goto out;
		msg++;
		len--;
		if (len <= 0)
			goto out;
	}

	// VAC, 0 or 1
	if (*msg != 0 && *msg != 1)
		goto out;
	msg++;
	len--;
	if (len <= 0)
		goto out;

	// TODO: verify
	// found bot count, replace with zero
	*msg = 0;
out:
	return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));
}

// find number of bots and replace with zero
size_t PASCAL handle_source_server_info_reply(int socket, const void* message, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t dest_len)
{
	/*
	Header 		byte 	Always equal to 'I' (0x49.)
	Protocol 	byte 	Protocol version used by the server.
	Name 		string 	Name of the server.
	Map 		string 	Map the server has currently loaded.
	Folder 		string 	Name of the folder containing the game files.
	Game 		string 	Full name of the game.
	ID 		short 	Steam Application ID of game.
	Players 	byte 	Number of players on the server.
	Max. Players 	byte 	Maximum number of players the server reports it can hold.
	Bots 		byte 	Number of bots on the server.
	Server type 	byte 	Indicates the type of server:
		'D' for a dedicated server
		'L' for a non-dedicated server
		'P' for a SourceTV server
	Environment 	byte 	Indicates the operating system of the server:
		'L' for Linux
		'W' for Windows
	Visibility 	byte 	Indicates whether the server requires a password:
		0 for public
		1 for private
	VAC 		byte 	Specifies whether the server uses VAC:
		0 for unsecured
		1 for secured
	....Continues....
	 */
	unsigned char* newmsg = (unsigned char*)malloc(length);
	memcpy(newmsg, message, length);

	size_t len = length - 5;
	unsigned char* msg = (unsigned char*)newmsg + 5;

	// protocol
	msg++;
	len--;
	if (len <= 0)
		goto out;

	// skip server name
	// TODO: verify
	while (len > 0) {
		if (*msg == 0) {
			msg++;
			len--;
			break;
		}
		msg++;
		len--;
	}
	if (len <= 0)
		goto out; /* something is not right here */

	// skip map name
	// TODO: verify
	while (len > 0) {
		if (*msg == 0) {
			msg++;
			len--;
			break;
		}
		msg++;
		len--;
	}
	if (len <= 0)
		goto out; /* something is not right here */

	// skip game folder name (valve, gearbox, etc)
	// TODO: verify
	while (len > 0) {
		if (*msg == 0) {
			msg++;
			len--;
			break;
		}
		msg++;
		len--;
	}
	if (len <= 0)
		goto out; /* something is not right here */

	// skip game name (Half-life, severian's mod, might be modified by metamod-plugins)
	while (len > 0) {
		if (*msg == 0) {
			msg++;
			len--;
			break;
		}
		msg++;
		len--;
	}
	if (len <= 0)
		goto out; /* something is not right here */

	// steam id, short
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
	if (*msg != engine->GetMaxClients())
		goto out;

	msg++;
	len--;
	if (len <= 0)
		goto out;

	// TODO: verify
	// found bot count, replace with zero
	*msg = 0;
out:
	return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));
}

//
size_t PASCAL sendto_hook(int socket, const void* message, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t dest_len)
{
	const unsigned char* orig_buf = (unsigned char*)message;

	if (!g_pGlobals)
		goto out;

	if (bot_conntimes < 1)
		goto out;

	if (length > 5 && orig_buf[0] == 0xff && orig_buf[1] == 0xff && orig_buf[2] == 0xff && orig_buf[3] == 0xff)
	{
		// check if this is server info reply packet (ÿÿÿÿm)
		// old GoldSrc format
		if (orig_buf[4] == 'm') {
			return handle_goldsrc_server_info_reply(socket, message, length, flags, dest_addr, dest_len);
		}

		// check if this is server info reply packet (ÿÿÿÿI)
		// new Source format (GoldSrc engine has switched to use this)
		if (orig_buf[4] == 'I') {
			return handle_source_server_info_reply(socket, message, length, flags, dest_addr, dest_len);
		}

		// check if this is player reply packet (ÿÿÿÿD)
		if (orig_buf[4] == 'D') {
			return(handle_player_reply(socket, message, length, flags, dest_addr, dest_len));
		}
	}

out:
	return(call_original_sendto(socket, message, length, flags, dest_addr, dest_len));
}