#include <ctime>
static int g_seed = static_cast<int>(time(nullptr));
inline int frand(void)
{
	g_seed = (214013 * g_seed + 2531011);
	return (g_seed >> 16) & 32767;
}

// https://vigna.di.unimi.it/xorshift/xoroshiro128plus.c
// modified version
static uint_fast64_t sr[2] = {static_cast<uint_fast64_t>(time(nullptr)), static_cast<uint_fast64_t>(time(nullptr) * 2)};
inline uint_fast64_t fnext(void)
{
	sr[1] ^= sr[0];
	sr[0] = (sr[0] << 55) | (sr[0] >> 9) ^ sr[1] ^ (sr[1] << 14);
	sr[1] = (sr[1] << 36) | (sr[1] >> 28);
	return sr[0] + sr[1];
}
