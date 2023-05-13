//
// Custom Math For EBot
//

#include <core.h>
#include <xmmintrin.h>

int g_seed;
inline int frand()
{
	g_seed = (214013 * g_seed + 2531011);
	return (g_seed >> 16) & 0x7FFF;
}

int CRandomInt(int min, int max)
{
	/*if (min == max)
		return min;
	else if (min > max)
		return frand() % (min - max + 1) + max;*/

	return frand() % (max - min + 1) + min;
}

float csqrt(float number)
{
	return _mm_cvtss_f32(_mm_sqrt_ss(_mm_load_ss(&number)));;
}

float crsqrt(float number)
{
	return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_load_ss(&number)));;
}

float Clamp(float a, float b, float c)
{
	return (a > c ? c : (a < b ? b : a));
}

float SquaredF(float a)
{
	return a * a;
}

float AddTime(float a)
{
	return g_pGlobals->time + a;
}

float MaxFloat(float a, float b)
{
	return a > b ? a : b;
}

float MinFloat(float a, float b)
{
	return a < b ? a : b;
}

int MinInt(int a, int b)
{
	return a < b ? a : b;
}

bool ChanceOf(int number)
{
	return CRandomInt(1, 100) <= number;
}