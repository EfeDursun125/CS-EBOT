//
// Custom Lib For E-Bot
// E-Bot is ai controlled players for counter-strike 1.6
// 
// For reduce glibc requirements and take full advantage of compiler optimizations
// And to get same results/performance on every OS
//

#include <core.h>
#include <rng.h>

#define USE_SSE2
#include <sse_mathfun.h>
#include <sse_mathfun_extension.h>

#ifndef PLATFORM_WIN32
#include <limits.h>
#ifndef UINT64_MAX
#define UINT64_MAX (__UINT64_MAX__)
#endif
#ifndef SIZE_MAX
#define SIZE_MAX (~(size_t)0)
#endif
#endif

static size_t cache;

int crandomint(const int min, const int max)
{
	if (min > max)
		return frand() % (min - max + 1) + max;

	return frand() % (max - min + 1) + min;
}

float crandomfloat(const float min, const float max)
{
	if (min > max)
		return fnext() * (min - max) / UINT64_MAX + max;

	return fnext() * (max - min) / UINT64_MAX + min;
}

bool chanceof(const int number)
{
	return crandomint(1, 101) < number;
}

float squaredf(const float value)
{
	return value * value;
}

float squaredi(const int value)
{
	return static_cast<float>(value * value);
}

int squared(const int value)
{
	return value * value;
}

float cclampf(const float a, const float b, const float c)
{
	if (a > c)
		return c;

	if (a < b)
		return b;

	return a;
}

int cclamp(const int a, const int b, const int c)
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
	return _mm_cvtss_f32(_mm_sqrt_ss(_mm_load_ss(&value)));
}

float crsqrtf(const float value)
{
	return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_load_ss(&value)));
}

float ccosf(const float value)
{
	return _mm_cvtss_f32(cos_ps(_mm_load_ss(&value)));
}

float csinf(const float value)
{
	return _mm_cvtss_f32(sin_ps(_mm_load_ss(&value)));
}

void csincosf(const float radians, float& sine, float& cosine)
{
	__m128 sse_s, sse_c;
	__m128* s = &sse_s;
	__m128* c = &sse_c;
	sincos_ps(_mm_load_ss(&radians), s, c);
	sine = _mm_cvtss_f32(*s);
	cosine = _mm_cvtss_f32(*c);
}

float catan2f(const float x, const float y)
{
	return _mm_cvtss_f32(atan2_ps(_mm_load_ss(&x), _mm_load_ss(&y)));
}

float ctanf(const float value)
{
	return _mm_cvtss_f32(tan_ps(_mm_load_ss(&value)));
}

// http://wurstcaptures.untergrund.net/assembler_tricks.html
float cpowf(const float a, const float b)
{
	return (a / (b - a * b + a));
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

	if (value > 0.0f)
	{
		if (result < value)
			result += 1.0f;
	}
	else if (value < 0.0f)
	{
		if (result > value)
			result -= 1.0f;
	}

	return result;
}

double cceil(const double value)
{
	const int intValue = static_cast<int>(value);
	double result = static_cast<double>(intValue);

	if (value > 0.0)
	{
		if (result < value)
			result += 1.0;
	}
	else if (value < 0.0)
	{
		if (result > value)
			result -= 1.0;
	}

	return result;
}

float cfloorf(const float value)
{
	return static_cast<float>(static_cast<int>(value));
}

double cfloor(const double value)
{
	return static_cast<double>(static_cast<int>(value));
}

float croundf(const float value)
{
	const int intValue = static_cast<int>(value);
	float result = static_cast<float>(intValue);

	if (value < 0.0f)
	{
		if (result - 0.5f > value)
			result -= 1.0f;
	}
	else
	{
		if (result + 0.5f < value)
			result += 1.0f;
	}

	return result;
}

double cround(const double value)
{
	const int intValue = static_cast<int>(value);
	double result = static_cast<double>(intValue);

	if (value < 0.0)
	{
		if (result - 0.5 > value)
			result -= 1.0;
	}
	else
	{
		if (result + 0.5 < value)
			result += 1.0;
	}

	return result;
}

size_t cstrlen(const char* str)
{
	if (str == nullptr)
		return 0;

	cache = 0;
	while (cache < SIZE_MAX && str[cache] != '\0')
		cache++;

	return cache;
}

// this fixes bot spectator bug in linux builds
// glibc's strcmp is not working like windows's strcmp, so it retuns incorrect value that causing bot always be in spectating team...
int cstrcmp(const char* str1, const char* str2)
{
	if (str1 == nullptr)
		return -1;

	if (str2 == nullptr)
		return -1;

	int t1, t2;

	do
	{
		t1 = *str1; t2 = *str2;
		if (t1 != t2)
		{
			if (t1 > t2)
				return 1;

			return -1;
		}

		if (!t1)
			return 0;

		str1++;
		str2++;

	} while (true);

	return -1;
}

int cstrncmp(const char* str1, const char* str2, const size_t num)
{
	if (str1 == nullptr)
		return 0;

	if (str2 == nullptr)
		return 0;

	for (cache = 0; cache < num; ++cache)
	{
		if (str1[cache] != str2[cache])
			return (str1[cache] < str2[cache]) ? -1 : 1;
		else if (str1[cache] == '\0')
			return 0;
	}

	return 0;
}

void cstrcpy(char* dest, const char* src)
{
	while (*src != '\0')
	{
		*dest = *src;
		dest++;
		src++;
	}

	*dest = '\0';
}

int cctz(unsigned int value)
{
	if (value == 0)
		return sizeof(unsigned int) * 8;

	cache = 0;
	while ((value & 1) == 0)
	{
		value >>= 1;
		cache++;
	}

	return cache;
}

int ctolower(const int value)
{
	if (value >= 'A' && value <= 'Z')
		return value + ('a' - 'A');

	return value;
}

int ctoupper(const int value)
{
	if (value >= 'a' && value <= 'z')
		return value - 'a' + 'A';

	return value;
}

int cstricmp(const char* str1, const char* str2)
{
	while (*str1 && *str2)
	{
		const int result = ctolower(*str1) - ctolower(*str2);
		if (result != 0)
			return result;

		str1++;
		str2++;
	}

	return ctolower(*str1) - ctolower(*str2);
}

bool cspace(const int str)
{
	return (str == L' ' || str == L'\t' || str == L'\n' || str == L'\r' || str == L'\f' || str == L'\v');
}

void cstrtrim(char* string)
{
	if (!string)
		return;

	char* ptr = string;
	int length = 0, toggleFlag = 0, increment = 0;
	while (*ptr++)
		length++;

	for (cache = length - 1; cache >= 0; cache--)
	{
		if (!cspace(string[cache]))
			break;
		else
		{
			string[cache] = 0;
			length--;
		}
	}

	for (cache = 0; cache < length; cache++)
	{
		if (cspace(string[cache]) && !toggleFlag)
		{
			increment++;

			if (increment + cache < length)
				string[cache] = string[increment + cache];
		}
		else
		{
			if (!toggleFlag)
				toggleFlag = 1;

			if (increment)
				string[cache] = string[increment + cache];
		}
	}

	string[length] = 0;
}

char* cstrstr(char* str1, const char* str2)
{
	if (*str2 == '\0')
		return str1;

	while (*str1 != '\0')
	{
		const char* p1 = str1;
		const char* p2 = str2;

		while (*p1 != '\0' && *p2 != '\0' && *p1 == *p2)
		{
			p1++;
			p2++;
		}

		if (*p2 == '\0')
			return str1;

		str1++;
	}

	return nullptr;
}

char* cstrncpy(char* dest, const char* src, const size_t count)
{
	char* destPtr = dest;
	const char* srcPtr = src;

	cache = 0;
	for (; cache < count && *srcPtr != '\0'; cache++, destPtr++, srcPtr++)
		*destPtr = *srcPtr;

	for (; cache < count; cache++, destPtr++)
		*destPtr = '\0';

	return dest;
}

char* cstrcat(char* dest, const char* src)
{
	while (*dest != '\0')
		dest++;

	while (*src != '\0')
	{
		*dest = *src;
		dest++;
		src++;
	}

	*dest = '\0';
	return dest;
}

int cstrcoll(const char* str1, const char* str2)
{
	if (str1 == nullptr || str2 == nullptr)
		return 0;

	while (*str1 != '\0' && *str2 != '\0')
	{
		if (*str1 < *str2)
			return -1;
		else if (*str1 > *str2)
			return 1;

		str1++;
		str2++;
	}

	if (*str1 == '\0' && *str2 != '\0')
		return -1;
	else if (*str1 != '\0' && *str2 == '\0')
		return 1;

	return 0;
}

size_t cstrspn(const char* str, const char* charset)
{
	if (str == nullptr || charset == nullptr)
		return 0;

	cache = 0;
	while (*str != '\0')
	{
		const char* charset_ptr = charset;
		bool found = false;

		while (*charset_ptr != '\0')
		{
			if (*charset_ptr == *str)
			{
				found = true;
				break;
			}

			charset_ptr++;
		}

		if (!found)
			break;

		cache++;
		str++;
	}

	return cache;
}

size_t cstrcspn(const char* str, const char* charset)
{
	if (str == nullptr || charset == nullptr)
		return 0;

	cache = 0;
	while (*str != '\0')
	{
		const char* charset_ptr = charset;
		bool found = false;

		while (*charset_ptr != '\0')
		{
			if (*charset_ptr == *str)
			{
				found = true;
				break;
			}

			charset_ptr++;
		}

		if (found)
			break;

		cache++;
		str++;
	}

	return cache;
}

int catoi(const char* str)
{
	if (str == nullptr)
		return -1;

	int result = 0;
	int sign = 1;
	int i = 0;

	while (str[i] == ' ')
		i++;

	if (str[i] == '-' || str[i] == '+')
	{
		sign = (str[i] == '-') ? -1 : 1;
		i++;
	}

	while (str[i] >= '0' && str[i] <= '9')
	{
		result = result * 10 + (str[i] - '0');
		i++;
	}

	result *= sign;
	return result;
}

float catof(const char* str)
{
	if (str == nullptr)
		return -1.0f;

	float result = 0.0f;
	float sign = 1.0f;
	int i = 0;

	while (str[i] == ' ')
		i++;

	if (str[i] == '-' || str[i] == '+')
	{
		sign = (str[i] == '-') ? -1.0f : 1.0f;
		i++;
	}

	while (str[i] >= '0' && str[i] <= '9')
	{
		result = result * 10.0f + (str[i] - '0');
		i++;
	}

	if (str[i] == '.')
		i++;

	float factor = 0.1f;
	while (str[i] >= '0' && str[i] <= '9')
	{
		result += (str[i] - '0') * factor;
		factor *= 0.1f;
		i++;
	}

	result *= sign;
	return result;
}