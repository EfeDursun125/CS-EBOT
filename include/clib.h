#pragma once
#include <stdint.h>
#include "rng.h"
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
extern int16_t g_numWaypoints;
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

inline float cclampf(const float value, const float min, const float max)
{
	if (value > max)
		return max;

	if (value < min)
		return min;

	return value;
}

inline int cclamp(const int value, const int min, const int max)
{
	if (value > max)
		return max;

	if (value < min)
		return min;

	return value;
}

inline float cmaxf(const float min, const float max)
{
	if (min > max)
		return min;

	return max;
}

inline float cminf(const float min, const float max)
{
	if (min < max)
		return min;

	return max;
}

inline int cmax(const int min, const int max)
{
	if (min > max)
		return min;

	return max;
}

inline int cmin(const int min, const int max)
{
	if (min < max)
		return min;

	return max;
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

inline int16_t cabs16(const int16_t value)
{
	if (value < 0)
		return -value;

	return value;
}

inline float cceilf(const float value)
{
	float result = static_cast<float>(static_cast<int>(value));
	if (value > 0.0f && result < value)
		return result + 1.0f;

	return result;
}

inline double cceil(const double value)
{
	double result = static_cast<double>(static_cast<int>(value));
	if (value > 0.0 && result < value)
		return result + 1.0;

	return result;
}

inline float cfloorf(const float value)
{
	float result = static_cast<float>(static_cast<int>(value));
	if (value < 0.0f && result > value)
		return result - 1.0f;

	return result;
}

inline double cfloor(const double value)
{
	double result = static_cast<double>(static_cast<int>(value));
	if (value < 0.0 && result > value)
		return result - 1.0;

	return result;
}

inline float croundf(const float value)
{
	return static_cast<float>(static_cast<int>(value + (value >= 0 ? 0.5f : -0.5f)));
}

inline double cround(const double value)
{
	return static_cast<double>(static_cast<int>(value + (value >= 0 ? 0.5 : -0.5)));
}

inline int cstrlen(const char* str)
{
	if (!str)
		return 0;

	int i = 0;
	while (i < SIZE_MAX && str[i] != '\0')
		i++;

	return i;
}

inline int cstrcmp(const char* str1, const char* str2)
{
	if (!str1)
		return -1;

	if (!str2)
		return -1;

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

	} while(true);

	return -1;
}

inline int cstrncmp(const char* str1, const char* str2, const int num)
{
	if (!str1)
		return 0;

	if (!str2)
		return 0;

	int i;
	for (i = 0; i < num; ++i)
	{
		if (str1[i] != str2[i])
			return (str1[i] < str2[i]) ? -1 : 1;
		else if (str1[i] == '\0')
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

inline void cmemcpy(void* dest, const void* src, const int size)
{
	char* dest2 = static_cast<char*>(dest);
	const char* src2 = static_cast<const char*>(src);

	int i;
	for (i = 0; i < size; i++)
		dest2[i] = src2[i];
}

inline void cmemset(void* dest, const int value, const int count)
{
	unsigned char* ptr = static_cast<unsigned char*>(dest);
	const unsigned char byteValue = static_cast<unsigned char>(value);

	int i;
	for (i = 0; i < count; i++)
	{
		*ptr = byteValue;
		ptr++;
	}
}

inline void* cmemmove(void* dest, const void* src, int count)
{
	unsigned char* d = static_cast<unsigned char*>(dest);
	const unsigned char* s = static_cast<const unsigned char*>(src);

	if (d == s)
		return dest;

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

	int i = 0;
	while (!(value & 1))
	{
		value >>= 1;
		i++;
	}

	return i;
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
	while (*str1 && *str2)
	{
		const int result = ctolower(*str1) - ctolower(*str2);
		if (result)
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
	if (!string)
		return;

	char *end = string;
	while (*end != '\0')
		++end;

	while (end > string && cspace(*--end))
		*end = '\0';

	char *start = string;
	while (cspace(*start) && *start != '\0')
		++start;

	if (start == string)
		return;

	int i = 0;
	while (*start != '\0')
		string[i++] = *start++;

	string[i] = '\0';
}

inline char* cstrstr(char* str1, char* str2)
{
	if (*str2 == '\0')
		return str1;

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

inline char* cstrncpy(char* dest, const char* src, const int count)
{
	char* destPtr = dest;
	const char* srcPtr = src;

	int i = 0;
	for (; i < count && *srcPtr != '\0'; i++, destPtr++, srcPtr++)
		*destPtr = *srcPtr;

	for (; i < count; i++, destPtr++)
		*destPtr = '\0';

	return dest;
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
	if (!str1)
		return 0;

	if (!str2)
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

	if (*str1 != '\0' && *str2 == '\0')
		return 1;

	return 0;
}

inline int cstrspn(const char* str, char* charset)
{
	if (!str)
		return 0;

	if (!charset)
		return 0;

	int i = 0;
	bool found = false;
	char* charset_ptr;
	while (*str != '\0')
	{
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

		i++;
		str++;
	}

	return i;
}

inline int cstrcspn(const char* str, char* charset)
{
	if (!str)
		return 0;

	if (!charset)
		return 0;

	int i = 0;
	bool found = false;
	char* charset_ptr;
	while (*str != '\0')
	{
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

		i++;
		str++;
	}

	return i;
}

inline int catoi(const char* str)
{
	if (!str)
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

inline int16_t catoi16(const char* str)
{
	if (!str)
		return -1;

	int16_t result = 0;
	int16_t sign = 1;
	int16_t i = 0;

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

inline float catof(const char* str)
{
	if (!str)
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

inline int charToInt(const char* str)
{
	if (!str)
		return 0;

	int i = 2;
	int sum = 0;
	while (i < SIZE_MAX && str[i] != '\0')
	{
		sum += static_cast<int>(str[i]);
		++i;
	}

	return sum;
}

template <typename T>
inline void cswap(T& a, T& b)
{
	const T temp = a;
	a = b;
	b = temp;
}

template <typename T>
class CPtr
{
private:
	T* m_ptr{nullptr};
public:
	CPtr(void) = default;
	explicit CPtr(T* ptr) : m_ptr(ptr) {}
	CPtr(CPtr&& other) noexcept : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }
	~CPtr(void)
	{
		if (m_ptr)
		{
			delete m_ptr;
			m_ptr = nullptr;
		}
	}

	inline CPtr& operator = (CPtr&& other) noexcept
	{
		if (this != &other)
		{
			if (m_ptr)
				delete m_ptr;

			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
		}

		return *this;
	}

	template <typename T2>
	inline T& operator[] (T2 index) { return m_ptr[index]; }
	inline T& operator * (void) { return *m_ptr; }
	inline T* operator -> (void) { return m_ptr; }
	inline bool IsAllocated(void)
    {
		if (m_ptr)
			return true;

		return false;
	}

	inline T* Get(void) { return m_ptr; }
	inline T* Release(void)
	{
		T* ptr = m_ptr;
		m_ptr = nullptr;
		return ptr;
	}

	inline void Reset(T* ptr = nullptr)
	{
		if (m_ptr)
		{
			delete m_ptr;
			m_ptr = nullptr;
		}

		m_ptr = ptr;
	}

	inline void Swap(CPtr& other) noexcept
	{
		const T* temp = m_ptr;
		m_ptr = other.m_ptr;
		other.m_ptr = temp;
	}

	inline void Destroy(void)
	{
		if (m_ptr)
		{
			delete m_ptr;
			m_ptr = nullptr;
		}
	}
};

template <typename T>
class CArray
{
private:
	CPtr<T>m_array{nullptr};
	int16_t m_size{0};
	int16_t m_capacity{0};
public:
	CArray(const int16_t size = 0) : m_size(size), m_capacity(size) { m_array.Reset(new(std::nothrow) T[size]); }
public:
	inline bool Resize(const int16_t size, const bool reset = false)
	{
		if (reset)
		{
			Destroy();
			m_array.Reset(new(std::nothrow) T[size]);
			if (m_array.IsAllocated())
			{
				m_capacity = size;
				return true;
			}

			return false;
		}

		if (!m_array.IsAllocated())
		{
			m_array.Reset(new(std::nothrow) T[size]);
			if (m_array.IsAllocated())
			{
				m_capacity = size;
				return true;
			}

			return false;
		}

		CPtr<T>new_array(new(std::nothrow) T[size]);
		if (!new_array.IsAllocated())
			return false;

		size_t max;
		if (m_size > size)
			max = size;
		else
			max = m_size;

		size_t i;
		for (i = 0; i < max; i++)
			new_array[i] = m_array[i];

		m_array.Reset(new_array.Release());
		m_capacity = size;
		return true;
	}

	inline void Destroy(void)
	{
		m_array.Destroy();
		m_size = 0;
		m_capacity = 0;
	}

	inline T& Get(const int16_t index)
	{
		if (index >= m_size)
			return m_array[0];

		return m_array[index];
	}

	inline bool Push(const T element, const bool autoSize = true) { return Push(&element, autoSize); }

	inline bool Push(const T* element, const bool autoSize = true)
	{
		if (m_size >= m_capacity)
		{
			if (!autoSize || !Resize(m_size + 1, false))
				return false;
		}

		if (m_array.IsAllocated())
		{
			m_array[m_size] = *element;
			m_size++;
		}

		return true;
	}

	inline bool Has(const T element) { return Has(&element); }

	inline bool Has(const T* element)
	{
		if (m_array.IsAllocated())
		{
			int16_t i;
			for (i = 0; i < m_size; i++)
			{
				if (m_array[i] == *element)
					return true;
			}
		}

		return false;
	}

	inline void RemoveAt(const int16_t index)
	{
		if (index >= m_size)
			return;

		if (m_array.IsAllocated())
		{
			int16_t i;
			for (i = index; i < m_size - 1; i++)
				m_array[i] = m_array[i + 1];

			m_size--;
		}
	}

	inline bool Remove(const T element) { return Remove(&element); }

	inline bool Remove(const T* element)
	{
		if (element && m_array.IsAllocated())
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
		}

		return false;
	}

	inline void Reverse(void)
	{
		if (m_array.IsAllocated())
		{
			int16_t i;
			const int16_t half = m_size / 2;
			const int16_t val = m_size - 1;
			for (i = 0; i < half; i++)
				cswap(m_array[i], m_array[val - i]);
		}
	}

	inline T Pop(void)
	{
		if (m_array.IsAllocated())
		{
			const T element = m_array[m_size - 1];
			RemoveAt(m_size - 1);
			return element;
		}

		return T();
	}

	inline void PopNoReturn(void) { RemoveAt(m_size - 1); }
	inline T& Last(void) { return m_array[m_size - 1]; }
	inline bool IsEmpty(void) const { return !m_size; }
	inline int16_t Size(void) const { return m_size; }
	inline int16_t Capacity(void) const { return m_capacity; }
	inline T& Random(void) { return m_array[crandomint(0, m_size - 1)]; }
	template <typename T2>
	inline T& operator[] (const T2 index) { return m_array[index]; }
};
