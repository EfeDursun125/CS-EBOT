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

inline int charToInt(const char* str)
{
	int i = 2;
	int sum = 0;
	while (str && str[i] != '\0')
	{
		sum += static_cast<int>(str[i]);
		i++;
	}
	return sum;
}

inline bool cspace(const int str)
{
	return (str == L' ' || str == L'\t' || str == L'\n' || str == L'\r' || str == L'\f' || str == L'\v');
}

inline void cstrtrim(char* string)
{
	if (!string)
		return;

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

template <typename T>
inline void cswap(T& a, T& b)
{
	const T temp = a;
	a = b;
	b = temp;
}

template <typename T>
class MiniArray
{
private:
	T* m_array{nullptr};
	int16_t m_size{0};
	int16_t m_capacity{0};
public:
	MiniArray(const int16_t size = 0) : m_size(size), m_capacity(size) { m_array = new(std::nothrow) T[size]; }
	virtual ~MiniArray(void) { Destroy(); }
public:
	inline bool Resize(const int16_t size, const bool reset = false)
	{
		if (reset)
		{
			Destroy();
			m_array = new(std::nothrow) T[size];
			if (m_array)
			{
				m_capacity = size;
				return true;
			}

			return false;
		}

		if (!m_array)
		{
			m_array = new(std::nothrow) T[size];
			if (m_array)
			{
				m_capacity = size;
				return true;
			}

			return false;
		}

		T* new_array = new(std::nothrow) T[size];
		if (!new_array)
			return false;

		size_t max;
		if (m_size > size)
			max = size;
		else
			max = m_size;

		size_t i;
		for (i = 0; i < max; i++)
			new_array[i] = m_array[i];

		delete[] m_array;
		m_array = new_array;
		new_array = nullptr;
		m_capacity = size;
		return true;
	}

	inline void Destroy(void)
	{
		if (m_array)
		{
			delete[] m_array;
			m_array = nullptr;
		}

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

		if (m_array)
		{
			m_array[m_size] = *element;
			m_size++;
		}

		return true;
	}

	inline bool Has(const T element)
	{
		return Has(&element);
	}

	inline bool Has(const T* element)
	{
		if (m_array)
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

		if (m_array)
		{
			int16_t i;
			for (i = index; i < m_size - 1; i++)
				m_array[i] = m_array[i + 1];

			m_size--;
		}
	}

	inline bool Remove(const T element)
	{
		return Remove(&element);
	}

	inline bool Remove(const T* element)
	{
		if (m_array && element)
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
		if (m_array)
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
		if (m_array)
		{
			const T element = m_array[m_size - 1];
			RemoveAt(m_size - 1);
			return element;
		}

		return T();
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
