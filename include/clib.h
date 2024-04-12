//
// Custom Lib For E-Bot
// E-Bot is ai controlled players for counter-strike 1.6
// 
// For reduce glibc requirements and take full advantage of compiler optimizations
// And to get same results/performance on every OS
//

#pragma once
#include <stdint.h>
#include <rng.h>
#ifndef WIN32
#include <limits.h>
#ifndef UINT64_MAX
#define UINT64_MAX (__UINT64_MAX__)
#endif
#ifndef SIZE_MAX
#define SIZE_MAX (~(size_t)0)
#endif
#include <new>
#endif
extern int g_numWaypoints;
template <typename T>
inline bool IsValidWaypoint(const T index)
{
    if (index < 0 || index >= static_cast<T>(g_numWaypoints))
        return false;

    return true;
}
#include <stdarg.h>

inline int crandomint(const int min, const int max)
{
	return frand() % (max - min + 1) + min;
}

inline float crandomfloat(const float min, const float max)
{
	if (min > max)
		return fnext() * (min - max) / UINT64_MAX + max;

	return fnext() * (max - min) / UINT64_MAX + min;
}

// faster, less range because of 32 bit intager, used as a pathfinding seed to randomize every bot's. if all bots goes on same way its not realistic and boring :)
inline float crandomfloatfast(int& seed, float& min, float& max)
{
	seed = (214013 * seed + 2531011);
	return ((seed >> 16) & 32767) * (max - min) * 0.0000305185f + min;
}

inline bool chanceof(const int number)
{
	return crandomint(1, 100) <= number;
}

inline float squaredf(const float value)
{
	return value * value;
}

inline float squaredi(const int value)
{
	return static_cast<float>(value * value);
}

inline int squared(const int value)
{
	return value * value;
}

inline float cclampf(const float a, const float b, const float c)
{
	if (a > c)
		return c;

	if (a < b)
		return b;

	return a;
}

inline int cclamp(const int a, const int b, const int c)
{
	if (a > c)
		return c;

	if (a < b)
		return b;

	return a;
}

inline float cmaxf(const float a, const float b)
{
	if (a > b)
		return a;

	return b;
}

inline float cminf(const float a, const float b)
{
	if (a < b)
		return a;

	return b;
}

inline int cmax(const int a, const int b)
{
	if (a > b)
		return a;

	return b;
}

inline int cmin(const int a, const int b)
{
	if (a < b)
		return a;

	return b;
}

extern float csqrtf(const float value);
extern float crsqrtf(const float value);
extern float ccosf(const float value);
extern float csinf(const float value);
extern void csincosf(const float radians, float& sine, float& cosine);
extern float catan2f(const float x, const float y);
extern float ctanf(const float value);

// https://martin.ankerl.com/2012/01/25/optimized-approximative-pow-in-c-and-cpp/
inline float cpowf(const float a, const float b)
{
	union
	{
		float d;
		int x;
	} u = { a };
	u.x = static_cast<int>(b * (u.x - 1064866805) + 1064866805);
	return u.d;
}

inline double cabsd(const double value)
{
	if (value < 0.0)
		return -value;

	return value;
}

inline float cabsf(const float value)
{
	if (value < 0.0f)
		return -value;

	return value;
}

inline int cabs(const int value)
{
	if (value < 0)
		return -value;

	return value;
}

inline float cceilf(const float value)
{
	float result = static_cast<float>(static_cast<int>(value));
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

inline double cceil(const double value)
{
	double result = static_cast<double>(static_cast<int>(value));
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

inline float cfloorf(const float value)
{
	return static_cast<float>(static_cast<int>(value));
}

inline double cfloor(const double value)
{
	return static_cast<double>(static_cast<int>(value));
}

inline float croundf(const float value)
{
	float result = static_cast<float>(static_cast<int>(value));
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

inline double cround(const double value)
{
	double result = static_cast<double>(static_cast<int>(value));
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

inline int cstrlen(const char* str)
{
	int cache = 0;
	while (str[cache] != '\0')
		cache++;
	return cache;
}

inline int charToInt(const char* str)
{
	int i = 2;
	int sum = 0;
	while (str[i] != '\0')
	{
		sum += static_cast<int>(str[i]);
		i++;
	}
	return sum;
}

inline int cstrcmp(const char* str1, const char* str2)
{
	int t1, t2;
	do
	{
		t1 = *str1;
		t2 = *str2;

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

inline int cstrncmp(const char* str1, const char* str2, const int num)
{
	int cache;
	for (cache = 0; cache < num; ++cache)
	{
		if (str1[cache] != str2[cache])
			return (str1[cache] < str2[cache]) ? -1 : 1;
		else if (str1[cache] == '\0')
			return 0;
	}

	return 0;
}

inline void cstrcpy(char* dest, const char* src)
{
	while (*src != '\0')
	{
		*dest = *src;
		dest++;
		src++;
	}

	*dest = '\0';
}

inline void cstrncpy(char* dest, const char* src, const int count)
{
	char* destPtr = dest;
	const char* srcPtr = src;

	int cache = 0;
	for (; cache < count && *srcPtr != '\0'; cache++, destPtr++, srcPtr++)
		*destPtr = *srcPtr;

	for (; cache < count; cache++, destPtr++)
		*destPtr = '\0';

	destPtr[count] = '\0';
}

inline void cmemcpy(void* dest, const void* src, const int size)
{
	char* dest2 = static_cast<char*>(dest);
	const char* src2 = static_cast<const char*>(src);

	int cache;
	for (cache = 0; cache < size; cache++)
		dest2[cache] = src2[cache];
}

inline void cmemset(void* dest, const int value, const int count)
{
	unsigned char* ptr = static_cast<unsigned char*>(dest);
	const unsigned char byteValue = static_cast<unsigned char>(value);

	int cache;
	for (cache = 0; cache < count; cache++)
	{
		*ptr = byteValue;
		ptr++;
	}
}

inline void* cmemmove(void* dest, const void* src, int count)
{
	unsigned char* d = static_cast<unsigned char*>(dest);
	const unsigned char* s = static_cast<const unsigned char*>(src);

	if (d < s)
	{
		while (count--)
			*d++ = *s++;
	}
	else
	{
		d += count;
		s += count;
		while (count--)
			*--d = *--s;
	}

	return dest;
}

inline int cctz(unsigned int value)
{
	if (!value)
		return sizeof(unsigned int) * 8;

	int cache = 0;
	while (!(value & 1))
	{
		value >>= 1;
		cache++;
	}

	return cache;
}

inline int ctolower(const int value)
{
	if (value >= 'A' && value <= 'Z')
		return value + ('a' - 'A');

	return value;
}

inline int ctoupper(const int value)
{
	if (value >= 'a' && value <= 'z')
		return value - 'a' + 'A';

	return value;
}

inline int cstricmp(const char* str1, const char* str2)
{
	int result;
	while (*str1 && *str2)
	{
		result = ctolower(*str1) - ctolower(*str2);
		if (result != 0)
			return result;

		str1++;
		str2++;
	}

	return ctolower(*str1) - ctolower(*str2);
}

inline bool cspace(const int str)
{
	return (str == L' ' || str == L'\t' || str == L'\n' || str == L'\r' || str == L'\f' || str == L'\v');
}

inline void cstrtrim(char* string)
{
	char* ptr = string;
	int length = 0, toggleFlag = 0, increment = 0, cache = 0;
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

inline char* cstrstr(char* str1, char* str2)
{
	char* p1;
	char* p2;
	while (*str1 != '\0')
	{
		p1 = str1;
		p2 = str2;

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

inline char* cstrcat(char* dest, const char* src)
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

inline int cstrcoll(const char* str1, const char* str2)
{
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

inline int cstrspn(const char* str, char* charset)
{
	int cache = 0;
	bool found;
	char* charset_ptr;

	while (*str != '\0')
	{
		found = false;
		charset_ptr = charset;
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

inline int cstrcspn(const char* str, char* charset)
{
	int cache = 0;
	bool found;
	char* charset_ptr;

	while (*str != '\0')
	{
		found = false;
		charset_ptr = charset;
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

inline int catoi(const char* str)
{
	int result = 0;
	int sign = 1;
	int cache = 0;

	while (str[cache] == ' ')
		cache++;

	if (str[cache] == '-' || str[cache] == '+')
	{
		sign = (str[cache] == '-') ? -1 : 1;
		cache++;
	}

	while (str[cache] >= '0' && str[cache] <= '9')
	{
		result = result * 10 + (str[cache] - '0');
		cache++;
	}

	result *= sign;
	return result;
}

inline float catof(const char* str)
{
	float result = 0.0f;
	float sign = 1.0f;
	int cache = 0;

	while (str[cache] == ' ')
		cache++;

	if (str[cache] == '-' || str[cache] == '+')
	{
		sign = (str[cache] == '-') ? -1.0f : 1.0f;
		cache++;
	}

	while (str[cache] >= '0' && str[cache] <= '9')
	{
		result = result * 10.0f + (str[cache] - '0');
		cache++;
	}

	if (str[cache] == '.')
		cache++;

	float factor = 0.1f;
	while (str[cache] >= '0' && str[cache] <= '9')
	{
		result += (str[cache] - '0') * factor;
		factor *= 0.1f;
		cache++;
	}

	result *= sign;
	return result;
}

inline int cvsnprintf(char* buf, const int size, const char* format, ...)
{
	int len = 0;
	char* ptr = buf;
	va_list args;
	va_start(args, format);

	while (*format)
	{
		if (*format != '%')
		{
			*ptr++ = *format++;
			len++;
			continue;
		}

		format++;
		switch (*format)
		{
		case 's':
		{
			const char* value = va_arg(args, const char*);
			cstrncpy(ptr, value, size - len - 1);
			// crashes here while cstrncpy running...
			ptr += cstrlen(value);
			len += cstrlen(value);
			break;
		}
		case 'i':
		case 'd':
		{
			int value = va_arg(args, int);
			if (value < 0)
			{
				*ptr++ = '-';
				value = -value;
				len++;
			}

			char num[12]; // INT_MAX + 1
			int i = 0;

			do
			{
				num[i++] = '0' + value % 10;
				value /= 10;
			} while (value);

			for (; i-- > 0;)
				*ptr++ = num[i];

			len += i;
			break;
		}
		case 'f':
		{
			const double value = va_arg(args, double);
			long lval = static_cast<long>(value);
			if (lval < 0)
			{
				*ptr++ = '-';
				lval = -lval;
				len++;
			}

			char num[24]; // DBL_MAX
			int i = 0;

			if (static_cast<unsigned long>(cround(cabsd(value - static_cast<double>(lval)) * 1000000.0)) >= 1000000)
			{
				do
				{
					num[i++] = '0' + (lval % 10);
					lval /= 10;
				} while (lval);

				*ptr++ = '.';
				len++;
			}
			else
			{
				do
				{
					num[i++] = '0' + (lval % 10);
					lval /= 10;
				} while (lval);
			}

			for (; i-- > 0;)
				*ptr++ = num[i];

			len += i;
			break;
		}
		default:
			break;
		}

		format++;
	}

	*ptr = '\0';
	va_end(args);
	return len;
}

template <typename T>
inline void cswap(T& a, T& b)
{
	const T temp = a;
	a = b;
	b = temp;
}

// useful on systems with low/bad memory
template <typename T>
inline T* safeloc(const size_t size)
{
	T* any = nullptr;
	while (!any)
	{
		any = new(std::nothrow) T[size]{};
		if (any)
			break;
	}

	return any;
}

// useful on systems with low/bad memory
template <typename T>
inline void safeloc(T*& any, const size_t size)
{
	while (!any)
	{
		any = new(std::nothrow) T[size]{};
		if (any)
			break;
	}
}

template <typename T>
inline void safereloc(T*& any, const size_t oldSize, const size_t newSize)
{
	if (any)
	{
		T* new_array = nullptr;
		while (!new_array)
		{
			new_array = new(std::nothrow) T[newSize]{};
			if (new_array)
				break;
		}

		size_t max;
		if (oldSize > newSize)
			max = newSize;
		else
			max = oldSize;

		size_t i;
		for (i = 0; i < max; i++)
			new_array[i] = any[i];

		delete[] any;
		any = new_array;
		new_array = nullptr;
	}
	else
	{
		while (!any)
		{
			any = new(std::nothrow) T[newSize]{};
			if (any)
				break;
		}
	}
}

template <typename T>
inline void safedel(T*& any)
{
	if (any)
	{
		delete[] any;
		any = nullptr;
	}
}

template <typename T>
class MiniArray
{
private:
	T* m_array{};
	int16_t m_size{};
	int16_t m_capacity{};
public:
	MiniArray(const int16_t size = 0) : m_size(size), m_capacity(size) { safeloc(m_array, size); }
	virtual ~MiniArray(void) { Destroy(); }
public:
	inline bool Resize(const int16_t size, const bool reset = false)
	{
		if (reset)
		{
			Destroy();
			safeloc(m_array, size);
			m_capacity = size;
			return true;
		}

		if (!m_array)
		{
			safeloc(m_array, size);
			m_capacity = size;
			return true;
		}

		safereloc(m_array, m_size, size);
		m_capacity = size;
		return true;
	}

	inline void Destroy(void)
	{
		safedel(m_array);
		m_size = 0;
		m_capacity = 0;
	}

	inline T& Get(const int16_t index)
	{
		if (index >= m_size)
			return m_array[0];

		return m_array[index];
	}

	inline bool Push(const T element, const bool autoSize = true)
	{
		return Push(&element, autoSize);
	}

	inline bool Push(const T* element, const bool autoSize = true)
	{
		if (m_size >= m_capacity)
		{
			if (!autoSize || !Resize(m_size + 1, false))
				return false;
		}

		m_array[m_size] = *element;
		m_size++;
		return true;
	}

	inline bool Has(const T element)
	{
		return Has(&element);
	}

	inline bool Has(const T* element)
	{
		int16_t i;
		for (i = 0; i < m_size; i++)
		{
			if (m_array[i] == *element)
				return true;
		}

		return false;
	}

	inline void RemoveAt(const int16_t index)
	{
		if (index >= m_size)
			return;

		int16_t i;
		for (i = index; i < m_size - 1; i++)
			m_array[i] = m_array[i + 1];

		m_size--;
	}

	inline bool Remove(const T element)
	{
		return Remove(&element);
	}

	inline bool Remove(const T* element)
	{
		int16_t i;
		for (i = 0; i < m_size; i++)
		{
			if (m_array[i] == *element)
			{
				RemoveAt(i);
				return true;
			}
		}

		return false;
	}

	inline void Reverse(void)
	{
		int16_t i;
		const int16_t half = m_size / 2;
		const int16_t val = m_size - 1;
		for (i = 0; i < half; i++)
			cswap(m_array[i], m_array[val - i]);
	}

	inline T Pop(void)
	{
		const T element = m_array[m_size - 1];
		RemoveAt(m_size - 1);
		return element;
	}

	inline void PopNoReturn(void)
	{
		RemoveAt(m_size - 1);
	}

	inline T& Last(void)
	{
		return m_array[m_size - 1];
	}

	inline bool IsEmpty(void) const
	{
		return !m_size;
	}

	inline int16_t Size(void) const
	{
		return m_size;
	}

	inline int16_t Capacity(void) const
	{
		return m_capacity;
	}

	inline T& Random(void) const
	{
		return m_array[crandomint(0, m_size - 1)];
	}

	inline T& operator[] (const int index)
	{
		return m_array[index];
	}
};