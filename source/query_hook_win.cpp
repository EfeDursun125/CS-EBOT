//
// JK_Botti - be more human!
//
// bot_query_hook_win32.cpp
//

#ifdef _WIN32

#include <string.h>

#include <memory.h>
#include <string.h>
#include <stdio.h>

#include <winsock2.h>
#include <windows.h>

#include "query_hook.h"

//
// win32, based on my "Linux code for dynamic linkents" from metamod-p
//

// 0xe9 is opcode for our function forwarder
#define JMP_SIZE 1

//pointer size on x86-32: 4 bytes
#define PTR_SIZE sizeof(void*)

//constructs new jmp forwarder
#define construct_jmp_instruction(x, place, target) { \
	((unsigned char *)(x))[0] = 0xe9; \
	*(unsigned long *)((char *)(x) + 1) = ((unsigned long)(target)) - (((unsigned long)(place)) + 5); \
}

//opcode + sizeof pointer
#define BYTES_SIZE (JMP_SIZE + PTR_SIZE)

typedef ssize_t(PASCAL* sendto_func)(int socket, const void* message, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t dest_len);

static bool is_sendto_hook_setup = false;

//pointer to original sendto
static sendto_func sendto_original;

//contains jmp to replacement_sendto @sendto_original
static unsigned char sendto_new_bytes[BYTES_SIZE];

//contains original bytes of sendto
static unsigned char sendto_old_bytes[BYTES_SIZE];

//Mutex for our protection
static CRITICAL_SECTION mutex_replacement_sendto;

//restores old sendto
inline void restore_original_sendto(void)
{
	//Copy old sendto bytes back
	memcpy((void*)sendto_original, sendto_old_bytes, BYTES_SIZE);
}

//resets new sendto
inline void reset_sendto_hook(void)
{
	//Copy new sendto bytes back
	memcpy((void*)sendto_original, sendto_new_bytes, BYTES_SIZE);
}

// Replacement sendto function
static ssize_t PASCAL __replacement_sendto(int socket, const void* message, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t dest_len)
{
	return sendto_hook(socket, message, length, flags, dest_addr, dest_len);
}

//
ssize_t PASCAL call_original_sendto(int socket, const void* message, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t dest_len)
{
	WSABUF iov = { 0, };
	iov.buf = (char*)message;
	iov.len = length;
	DWORD num_sent = 0;
	int err;

	err = WSASendTo(socket, &iov, 1, &num_sent, flags, dest_addr, dest_len, NULL, NULL);
	if (err == SOCKET_ERROR) {
		errno = WSAGetLastError();
		return -1;
	}

	return (size_t)num_sent;
}

//
bool hook_sendto_function(void)
{
	DWORD tmp = 0;

	if (is_sendto_hook_setup)
		return(true);

	InitializeCriticalSection(&mutex_replacement_sendto);

	is_sendto_hook_setup = false;

	sendto_original = (sendto_func)GetProcAddress(GetModuleHandle("wsock32.dll"), "sendto");

	//Backup old bytes of "sendto" function
	memcpy(sendto_old_bytes, (void*)sendto_original, BYTES_SIZE);

	//Construct new bytes: "jmp offset[replacement_sendto] @ sendto_original"
	construct_jmp_instruction((void*)&sendto_new_bytes[0], (void*)sendto_original, (void*)&__replacement_sendto);

	//Remove readonly restriction
	if (!VirtualProtect((void*)sendto_original, BYTES_SIZE, PAGE_READWRITE, &tmp))
	{
		UTIL_ConsolePrintf("Couldn't initialize sendto hook, VirtualProtect failed: %i.  Exiting...\n", GetLastError());
		return(false);
	}

	//Write our own jmp-forwarder on "sendto"
	reset_sendto_hook();

	is_sendto_hook_setup = true;

	//done
	return(true);
}

//
bool unhook_sendto_function(void)
{
	if (!is_sendto_hook_setup)
		return(true);

	//Lock before modifing original sendto
	EnterCriticalSection(&mutex_replacement_sendto);

	//reset sendto hook
	restore_original_sendto();

	//unlock
	LeaveCriticalSection(&mutex_replacement_sendto);

	DeleteCriticalSection(&mutex_replacement_sendto);

	is_sendto_hook_setup = false;

	return(true);
}

#endif /*_WIN32*/