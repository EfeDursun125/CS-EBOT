//
// Custom Math Lib For E-Bot
// E-Bot is ai controlled players for counter-strike 1.6
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
			return static_cast<size_t>(ptr - str) + static_cast<size_t>(ctz(static_cast<uint32_t>(mask)));

        ptr += 16;
    }
}

int cstrcmp(const char* str1, const char* str2)
{
	int idx = 0, t1, t2;

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

	/*const __m128i zero = _mm_setzero_si128(); // performance loss, also it can give incorrect result...
	const __m128i* p1 = (__m128i*)str1;
	const __m128i* p2 = (__m128i*)str2;

	while (true)
	{
		const __m128i chunk1 = _mm_loadu_si128(p1);
		const __m128i chunk2 = _mm_loadu_si128(p2);

		const __m128i cmp = _mm_cmpeq_epi8(chunk1, chunk2);
		const int mask = _mm_movemask_epi8(cmp);

		if (mask != 0xFFFF)
		{
			const int index = ctz(~mask);
			const char* ptr1 = (const char*)(p1);
			const char* ptr2 = (const char*)(p2);

			if (ptr1[index] > ptr2[index])
				return 1;
			else
				return -1;
		}
		else if (_mm_movemask_epi8(_mm_cmpeq_epi8(chunk1, zero)) == 0xFFFF)
			return 0;

		p1++;
		p2++;
	}

	return 0;*/
}

int cstrncmp(const char* str1, const char* str2, const size_t num)
{
	const size_t chunkSize = 16;
	const __m128i* p1 = (__m128i*)str1;
	const __m128i* p2 = (__m128i*)str2;

	for (size_t i = 0; i < num / chunkSize; ++i)
	{
		const __m128i chunk1 = _mm_loadu_si128(p1 + i);
		const __m128i chunk2 = _mm_loadu_si128(p2 + i);

		const __m128i cmp = _mm_cmpeq_epi8(chunk1, chunk2);
		const int mask = _mm_movemask_epi8(cmp);
		if (mask != 0xFFFF)
		{
			const size_t index = i * chunkSize + ctz(~mask);
			if (index < num)
				return str1[index] - str2[index];
			else
				return 0;
		}
	}

	const char* p1Rem = (const char*)(p1 + (num / chunkSize));
	const char* p2Rem = (const char*)(p2 + (num / chunkSize));
	for (size_t i = 0; i < num % chunkSize; ++i)
	{
		if (p1Rem[i] != p2Rem[i])
			return p1Rem[i] - p2Rem[i];

		if (p1Rem[i] == '\0')
			break;
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

void cmemcpy(void* dest, const void* src, const size_t size)
{
	const size_t vectorSize = 16;
	const size_t numVectors = size / vectorSize;

	__m128i* destVector = (__m128i*)dest;
	const __m128i* srcVector = (const __m128i*)src;

	for (size_t i = 0; i < numVectors; ++i)
	{
		const __m128i data = _mm_loadu_si128(srcVector + i);
		_mm_storeu_si128(destVector + i, data);
	}

	const size_t remainingSize = size % vectorSize;
	if (remainingSize > 0)
	{
		const char* srcBytes = (const char*)(srcVector + numVectors);
		char* destBytes = (char*)(destVector + numVectors);
		for (size_t i = 0; i < remainingSize; ++i)
			destBytes[i] = srcBytes[i];
	}
}

void cmemset(void* dest, const int value, const size_t count)
{
	unsigned char* ptr = static_cast<unsigned char*>(dest);
	const unsigned char byteValue = static_cast<unsigned char>(value);
	for (size_t i = 0; i < count; ++i)
	{
		*ptr = byteValue;
		ptr++;
	}
}

int ctz(unsigned int value)
{
	if (value == 0)
		return sizeof(unsigned int) * 8;

	int count = 0;
	while ((value & 1) == 0)
	{
		value >>= 1;
		count++;
	}

	return count;
}

int ctolower(const int value)
{
	if (value >= 'A' && value <= 'Z')
		return value + ('a' - 'A');
	else
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
	char* ptr = string;

	int length = 0, toggleFlag = 0, increment = 0;
	int i = 0;

	while (*ptr++)
		length++;

	for (i = length - 1; i >= 0; i--)
	{
		if (!cspace(string[i]))
			break;
		else
		{
			string[i] = 0;
			length--;
		}
	}

	for (i = 0; i < length; i++)
	{
		if (cspace(string[i]) && !toggleFlag)
		{
			increment++;

			if (increment + i < length)
				string[i] = string[increment + i];
		}
		else
		{
			if (!toggleFlag)
				toggleFlag = 1;

			if (increment)
				string[i] = string[increment + i];
		}
	}

	string[length] = 0;
}

char* cstrstr(char* haystack, const char* needle)
{
	const __m128i first = _mm_set1_epi8(needle[0]);
	const size_t needleLen = cstrlen(needle);

	while (*haystack)
	{
		const __m128i chunk = _mm_loadu_si128((__m128i*)haystack);
		const __m128i cmp = _mm_cmpeq_epi8(chunk, first);
		int mask = _mm_movemask_epi8(cmp);

		while (mask)
		{
			const int index = ctz(mask);
			if (cstrncmp(haystack + index, needle, needleLen) == 0)
				return haystack + index;

			mask ^= (1 << index);
		}

		haystack += 16;
	}

	return nullptr;
}