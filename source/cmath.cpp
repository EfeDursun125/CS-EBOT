#include <core.h>

int g_seed;
inline int frand()
{
	g_seed = (214013 * g_seed + 2531011);
	return (g_seed >> 16) & 0x7FFF;
}

int RandomInt(int min, int max)
{
	if (min == max)
		return min;
	else if (min > max)
		return frand() % (min - max + 1) + max;

	return frand() % (max - min + 1) + min;
}

float Q_sqrt(float number)
{
	long i;
	float x2, y;
	x2 = number * 0.5f;
	y = number;
	i = *(long*)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(float*)&i;
	return y * number;
}

float Q_rsqrt(float number)
{
	long i;
	float x2, y;
	x2 = number * 0.5f;
	y = number;
	i = *(long*)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(float*)&i;
	return y;
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
	return RandomInt(1, 100) <= number;
}