//
// Custom Math For EBot
//

#include <core.h>
#include <immintrin.h>

int g_seed;
inline int frand()
{
	g_seed = (214013 * g_seed + 2531011);
	return (g_seed >> 16) & 0x7FFF;
}

int CRandomInt(const int min, const int max)
{
	/*if (min == max)
		return min;
	else if (min > max)
		return frand() % (min - max + 1) + max;*/

	return frand() % (max - min + 1) + min;
}

bool ChanceOf(const int number)
{
	return CRandomInt(1, 100) <= number;
}

float SquaredF(const float value)
{
	return value * value;
}

float AddTime(const float time)
{
	return g_pGlobals->time + time;
}

float cclampf(const float a, const float b, const float c)
{
	if (a > c)
		return c;

	if (a < b)
		return b;

	return a;
}

float cmaxf(const float a, const float b)
{
	if (a > b)
		return a;

	return b;
}

float cminf(const float a, const float b)
{
	if (a < b)
		return a;

	return b;
}

int cmax(const int a, const int b)
{
	if (a > b)
		return a;

	return b;
}

int cmin(const int a, const int b)
{
	if (a < b)
		return a;

	return b;
}

float csqrtf(const float value)
{
	return _mm_cvtss_f32(_mm_sqrt_ss(_mm_load_ss(&value)));;
}

float crsqrtf(const float value)
{
	return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_load_ss(&value)));;
}

float cabsf(const float value)
{
	if (value < 0.0f)
		return -value;

	return value;
}

int cabs(const int value)
{
	if (value < 0)
		return -value;

	return value;
}

float cceilf(const float value)
{
	const int intValue = static_cast<int>(value);
	float result = static_cast<float>(intValue);

	if (result < value)
		result += 1.0f;

	return result;
}

float cfloorf(const float value)
{
	return static_cast<float>(static_cast<int>(value));
}

float croundf(const float value)
{
	const int intValue = static_cast<int>(value);
	float result = static_cast<float>(intValue);

	if (result + 0.5f < value)
		result += 1.0f;

	return result;
}

size_t cstrlen(const char* str)
{
    const __m128i zero = _mm_setzero_si128();
    const char* ptr = str;

    while (true)
    {
        const __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
        const __m128i cmp = _mm_cmpeq_epi8(data, zero);
        const int mask = _mm_movemask_epi8(cmp);

        if (mask != 0)
		return static_cast<size_t>(ptr - str) + static_cast<size_t>(__builtin_ctz(static_cast<uint32_t>(mask)));

        ptr += 16;
    }
}
