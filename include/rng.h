static int g_seed = time(nullptr);
int frand(void)
{
	g_seed = (214013 * g_seed + 2531011);
	return (g_seed >> 16) & 0x7FFF;
}

// https://vigna.di.unimi.it/xorshift/xoroshiro128plus.c
static uint64_t s[2] = {0x123456789ABCDEF0ULL, 0xABCDEF0123456789ULL};
uint64_t fnext(void)
{
	const uint64_t s0 = s[0];
	uint64_t s1 = s[1];
	const uint64_t result = s0 + s1;
	s1 ^= s0;
	s[0] = (s0 << 55) | (s0 >> 9) ^ s1 ^ (s1 << 14);
	s[1] = (s1 << 36) | (s1 >> 28);
	return result;
}