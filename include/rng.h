#include <ctime>
static int g_seed = static_cast<int>(time(nullptr));
inline int frand(void)
{
	g_seed = (214013 * g_seed + 2531011);
	return (g_seed >> 16) & 32767;
}

// https://vigna.di.unimi.it/xorshift/xoroshiro128plus.c
// modified version
static uint_fast64_t s[2] = {static_cast<uint_fast64_t>(time(nullptr)), static_cast<uint_fast64_t>(time(nullptr) * 2)};
inline uint_fast64_t fnext(void)
{
	s[1] ^= s[0];
	s[0] = (s[0] << 55) | (s[0] >> 9) ^ s[1] ^ (s[1] << 14);
	s[1] = (s[1] << 36) | (s[1] >> 28);
	return s[0] + s[1];
}