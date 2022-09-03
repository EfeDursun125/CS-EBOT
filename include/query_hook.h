#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

#include <sys/types.h>

typedef int socklen_t;
#else
#include <sys/types.h>
#include <sys/socket.h>

#define PASCAL
#endif

#include <core.h>

size_t PASCAL sendto_hook(int socket, const void* message, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t dest_len);
size_t PASCAL call_original_sendto(int socket, const void* message, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t dest_len);
bool hook_sendto_function(void);
bool unhook_sendto_function(void);

void UTIL_ConsolePrintf(char* fmt, ...);
void BotReplaceConnectionTime(const char* name, float* timeslot);

extern int bot_conntimes;