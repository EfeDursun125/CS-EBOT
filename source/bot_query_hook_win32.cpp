#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#include <mutex>
#include <memory.h>
#include <stdio.h>

#include "core.h"
#include "bot_query_hook.h"

#define JMP_SIZE 1
#define PTR_SIZE sizeof(void*)
#define BYTES_SIZE (JMP_SIZE + PTR_SIZE)

typedef int (PASCAL *sendto_func)(SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen);

static bool is_sendto_hook_setup = false;
static sendto_func sendto_original = nullptr;
static unsigned char sendto_new_bytes[BYTES_SIZE];
static unsigned char sendto_old_bytes[BYTES_SIZE];

static std::recursive_mutex mutex_replacement_sendto;

static void construct_jmp_instruction(void *x, void *place, void *target)
{
	unsigned char *p = (unsigned char *)x;
	p[0] = 0xe9;
	unsigned long offset = (reinterpret_cast<unsigned long>(target)) - (reinterpret_cast<unsigned long>(place) + BYTES_SIZE);
	cmemcpy(p + 1, &offset, sizeof(unsigned long));
}

inline void restore_original_sendto(void)
{
	cmemcpy(reinterpret_cast<void*>(sendto_original), sendto_old_bytes, BYTES_SIZE);
}

inline void reset_sendto_hook(void)
{
	cmemcpy(reinterpret_cast<void*>(sendto_original), sendto_new_bytes, BYTES_SIZE);
}

static int PASCAL FORCE_STACK_ALIGN __replacement_sendto(SOCKET socket, const char *message, int length, int flags, const struct sockaddr *dest_addr, int dest_len)
{
	return sendto_hook(static_cast<int>(socket), message, static_cast<size_t>(length), flags, dest_addr, static_cast<socklen_t>(dest_len));
}

ssize_t PASCAL call_original_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
	WSABUF iov{};
	iov.buf = const_cast<char*>(reinterpret_cast<const char*>(message));
	iov.len = static_cast<ULONG>(length);
	
	DWORD num_sent = 0;
	int err = WSASendTo(static_cast<SOCKET>(socket), &iov, 1, &num_sent, flags, dest_addr, static_cast<int>(dest_len), nullptr, nullptr);
	if (err == SOCKET_ERROR) {
		errno = WSAGetLastError();
		return -1;
	}
	return static_cast<ssize_t>(num_sent);
}

bool hook_sendto_function(void)
{
	if (is_sendto_hook_setup)
		return true;

	HMODULE ws2 = GetModuleHandle("ws2_32.dll");
	if (!ws2)
		ws2 = GetModuleHandle("wsock32.dll");

	if (!ws2) {
		ServerPrint("Couldn't retrieve Winsock module handle.");
		return false;
	}

	sendto_original = reinterpret_cast<sendto_func>(GetProcAddress(ws2, "sendto"));
	if (!sendto_original) {
		ServerPrint("Couldn't resolve sendto address.");
		return false;
	}

	// Backup original bytes
	cmemcpy(sendto_old_bytes, reinterpret_cast<void*>(sendto_original), BYTES_SIZE);

	// Pre-calculate JMP offset
	construct_jmp_instruction(sendto_new_bytes, reinterpret_cast<void*>(sendto_original), reinterpret_cast<void*>(&__replacement_sendto));

	// Change memory protect to PAGE_EXECUTE_READWRITE
	DWORD old_protect;
	if (!VirtualProtect(reinterpret_cast<void*>(sendto_original), BYTES_SIZE, PAGE_EXECUTE_READWRITE, &old_protect)) {
		ServerPrint("VirtualProtect failed with error code: %lu", GetLastError());
		return false;
	}

	// Write hook
	reset_sendto_hook();
	is_sendto_hook_setup = true;
	return true;
}

bool unhook_sendto_function(void)
{
	if (!is_sendto_hook_setup)
		return true;

	{
		std::lock_guard<std::recursive_mutex> lock(mutex_replacement_sendto);
		restore_original_sendto();
	}

	is_sendto_hook_setup = false;
	return true;
}

#endif // _WIN32
