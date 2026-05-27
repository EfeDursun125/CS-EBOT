#pragma once

#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <cassert>
#include <cstring>
#include <utility>
#include <new>
#include <cstdarg>

#include "rng.h" 

#ifndef WIN32
#include <climits>
#endif

extern int16_t g_numWaypoints;

template <typename T>
inline bool IsValidWaypoint(const T index)
{
	if (index < 0 || index >= static_cast<T>(g_numWaypoints))
		return false;

	return true;
}

inline int crandomint(const int min, const int max)
{
	if (min > max)
		return min;

	return static_cast<int>(frand() % (max - min + 1)) + min;
}

inline float crandomfloat(const float min, const float max)
{
	if (min > max)
		return static_cast<float>(fnext()) * (min - max) / UINT64_MAX + max;

	return static_cast<float>(fnext()) * (max - min) / UINT64_MAX + min;
}

inline float crandomfloatfast(int& seed, const float min, const float max)
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
	return (min > max) ? min : max;
}

inline float cminf(const float min, const float max)
{
	return (min < max) ? min : max;
}

inline int cmax(const int min, const int max)
{
	return (min > max) ? min : max;
}

inline int cmin(const int min, const int max)
{
	return (min < max) ? min : max;
}

extern float csqrtf(const float value);
extern float crsqrtf(const float value);
extern float ccosf(const float value);
extern float csinf(const float value);
extern void csincosf(const float radians, float& sine, float& cosine);
extern float catan2f(const float x, const float y);
extern float ctanf(const float value);

inline float cpowf(const float a, const float b)
{
	int u_x;
	std::memcpy(&u_x, &a, sizeof(float));
	u_x = static_cast<int>(b * (u_x - 1064866805) + 1064866805);

	float u_d;
	std::memcpy(&u_d, &u_x, sizeof(float));
	return u_d;
}

inline double cabsd(const double value)
{
	return (value < 0.0) ? -value : value;
}

inline float cabsf(const float value)
{
	return (value < 0.0f) ? -value : value;
}

inline int cabs(const int value)
{
	return (value < 0) ? -value : value;
}

inline int16_t cabs16(const int16_t value)
{
	return (value < 0) ? -value : value;
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

inline size_t cstrlen(const char* str)
{
	if (!str)
		return 0;

	size_t i = 0;
	while (str[i] != '\0')
		i++;

	return i;
}

inline int cstrcmp(const char* str1, const char* str2)
{
	if (!str1 && !str2)
		return 0;

	if (!str1)
		return -1;

	if (!str2)
		return 1;

	while (*str1 && (*str1 == *str2))
	{
		str1++;
		str2++;
	}

	return *(unsigned char*)str1 - *(unsigned char*)str2;
}

inline int cstrncmp(const char* str1, const char* str2, size_t num)
{
	if (num == 0)
		return 0;

	if (!str1 && !str2)
		return 0;

	if (!str1)
		return -1;

	if (!str2)
		return 1;

	while (num-- > 0)
	{
		if (*str1 != *str2)
			return *(unsigned char*)str1 - *(unsigned char*)str2;

		if (*str1 == '\0')
			break;

		str1++;
		str2++;
	}

	return 0;
}

inline void cstrcpy(char* dest, size_t destSize, const char* src)
{
	if (!dest || destSize == 0)
		return;

	if (!src)
	{
		dest[0] = '\0';
		return;
	}

	size_t i = 0;
	while (src[i] != '\0' && i < destSize - 1)
	{
		dest[i] = src[i];
		i++;
	}

	dest[i] = '\0';
}

inline void cmemcpy(void* dest, const void* src, size_t size)
{
	if (!dest || !src || size == 0)
		return;

	size_t i;
	char* d = static_cast<char*>(dest);
	const char* s = static_cast<const char*>(src);
	for (i = 0; i < size; i++)
		d[i] = s[i];
}

inline void cmemset(void* dest, int value, size_t count)
{
	if (!dest || count == 0)
		return;

	size_t i;
	unsigned char* ptr = static_cast<unsigned char*>(dest);
	unsigned char byteValue = static_cast<unsigned char>(value);
	for (i = 0; i < count; i++)
		ptr[i] = byteValue;
}

inline void* cmemmove(void* dest, const void* src, size_t count)
{
	if (!dest || !src || count == 0)
		return dest;

	unsigned char* d = static_cast<unsigned char*>(dest);
	const unsigned char* s = static_cast<const unsigned char*>(src);
	if (d == s)
		return dest;

	if (d < s)
	{
		size_t i;
		for (i = 0; i < count; ++i)
			d[i] = s[i];
	}
	else
	{
		size_t i;
		for (i = count; i > 0; --i)
			d[i - 1] = s[i - 1];
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

inline int ctolower(int value)
{
	if (value)
		return (value >= 'A' && value <= 'Z') ? value + ('a' - 'A') : value;

	return 0;
}

inline int ctoupper(int value)
{
	if (value)
		return (value >= 'a' && value <= 'z') ? value - 'a' + 'A' : value;

	return 0;
}

inline int cstricmp(const char* str1, const char* str2)
{
	if (!str1 && !str2)
		return 0;

	if (!str1)
		return -1;

	if (!str2)
		return 1;

	int result;
	while (*str1 && *str2)
	{
		result = ctolower(static_cast<unsigned char>(*str1)) - ctolower(static_cast<unsigned char>(*str2));
		if (result)
			return result;

		str1++;
		str2++;
	}

	return ctolower(static_cast<unsigned char>(*str1)) - ctolower(static_cast<unsigned char>(*str2));
}

inline bool cspace(int str)
{
	if (str)
		return (str == ' ' || str == '\t' || str == '\n' || str == '\r' || str == '\f' || str == '\v');

return false;
}

inline void cstrtrim(char* string)
{
	if (!string || string[0] == '\0')
		return;

	char *end = string;
	while (*end != '\0')
		++end;

	while (end > string && cspace(static_cast<unsigned char>(*--end)))
		*end = '\0';

	char *start = string;
	while (cspace(static_cast<unsigned char>(*start)) && *start != '\0')
		++start;

	if (start == string)
		return;

	size_t i = 0;
	while (*start != '\0')
		string[i++] = *start++;

	string[i] = '\0';
}

inline char* cstrstr(char* str1, const char* str2)
{
	if (!str1 || !str2)
		return nullptr;

	if (*str2 == '\0')
		return str1;

	while (*str1 != '\0')
	{
		char* p1 = str1;
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

inline char* cstrncpy(char* dest, const char* src, size_t count)
{
	if (!dest || count == 0)
		return dest;

	if (!src)
	{
		dest[0] = '\0';
		return dest;
	}

	size_t i = 0;
	for (; i < count - 1 && src[i] != '\0'; i++)
		dest[i] = src[i];

	for (; i < count; i++)
		dest[i] = '\0';

	return dest;
}

inline char* cstrcat(char* dest, size_t destSize, const char* src)
{
	if (!dest || !src || destSize == 0)
		return dest;

	size_t i = 0;
	size_t dest_len = cstrlen(dest);

	while (src[i] != '\0' && (dest_len + i) < (destSize - 1))
	{
		dest[dest_len + i] = src[i];
		i++;
	}

	dest[dest_len + i] = '\0';
	return dest;
}

inline int cstrcoll(const char* str1, const char* str2)
{
	return cstrcmp(str1, str2);
}

inline size_t cstrspn(const char* str, const char* charset)
{
	if (!str || !charset)
		return 0;

	size_t count = 0;
	while (str[count] != '\0')
	{
		const char* c = charset;
		bool found = false;
		while (*c != '\0')
		{
			if (*c == str[count])
			{
				found = true;
				break;
			}

			c++;
		}

		if (!found)
			break;

		count++;
	}

	return count;
}

inline size_t cstrcspn(const char* str, const char* charset)
{
	if (!str || !charset)
		return 0;

	size_t count = 0;
	while (str[count] != '\0')
	{
		const char* c = charset;
		while (*c != '\0')
		{
			if (*c == str[count])
				return count;

			c++;
		}

		count++;
	}

	return count;
}

inline int catoi(const char* str)
{
	if (!str)
		return 0;

	int result = 0;
	int sign = 1;

	size_t i = 0;
	while (cspace(static_cast<unsigned char>(str[i])))
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

	return result * sign;
}

inline int16_t catoi16(const char* str)
{
	return static_cast<int16_t>(catoi(str));
}

inline float catof(const char* str)
{
	if (!str)
		return 0.0f;

	float result = 0.0f;
	float sign = 1.0f;

	size_t i = 0;
	while (cspace(static_cast<unsigned char>(str[i])))
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
	{
		i++;
		float factor = 0.1f;
		while (str[i] >= '0' && str[i] <= '9')
		{
			result += (str[i] - '0') * factor;
			factor *= 0.1f;
			i++;
		}
	}

	if (str[i] == 'e' || str[i] == 'E')
	{
		i++;
		float exp_sign = 1.0f;
		if (str[i] == '-' || str[i] == '+')
		{
			exp_sign = (str[i] == '-') ? -1.0f : 1.0f;
			i++;
		}

		int exp_val = 0;
		while (str[i] >= '0' && str[i] <= '9')
		{
			exp_val = exp_val * 10 + (str[i] - '0');
			i++;
		}

		if (exp_sign < 0.0f)
		{
			int j;
			for (j = 0; j < exp_val; ++j)
				result /= 10.0f;
		}
		else
		{
			int j;
			for (j = 0; j < exp_val; ++j)
				result *= 10.0f;
		}
	}

	return result * sign;
}


inline int charToInt(const char* str)
{
	if (!str)
		return 0;

	size_t i;
	int sum = 0;
	for (i = 0; str[i] != '\0'; ++i)
		sum += static_cast<int>(static_cast<unsigned char>(str[i]));

	return sum;
}

template <typename T>
inline void cswap(T& a, T& b)
{
	T temp = std::move(a);
	a = std::move(b);
	b = std::move(temp);
}

template <typename T>
class CPtr
{
private:
	T* m_ptr{nullptr};
	bool m_isArray{true};
	static bool isAligned(void* ptr)
	{
		return (reinterpret_cast<uintptr_t>(ptr) & (alignof(T) - 1)) == 0;
	}
public:
	CPtr(void) = default;
	explicit CPtr(T* ptr, bool isArray = true) : m_ptr(ptr), m_isArray(isArray) {}
	~CPtr(void) { Reset(); }

	CPtr(const CPtr&) = delete;
	CPtr& operator=(const CPtr&) = delete;

	CPtr(CPtr&& other) noexcept : m_ptr(other.m_ptr), m_isArray(other.m_isArray)
	{
		other.m_ptr = nullptr;
		other.m_isArray = false;
	}

	CPtr& operator=(CPtr&& other) noexcept
	{
		if (this != &other)
		{
			Reset();
			m_ptr = other.m_ptr;
			m_isArray = other.m_isArray;
			other.m_ptr = nullptr;
			other.m_isArray = false;
		}

		return *this;
	}

	bool Reset(T* ptr = nullptr, bool isArray = true)
	{
		bool aligned = true;
		if (m_ptr)
		{
			assert(isAligned(m_ptr) && "Unaligned memory detected!");
			if (m_isArray) delete[] m_ptr;
			else delete m_ptr;
		}

		m_ptr = ptr;
		m_isArray = isArray;
		return aligned;
	}

	T* Release(void)
	{
		T* ptr = m_ptr;
		m_ptr = nullptr;
		m_isArray = false;
		return ptr;
	}

	void Destroy(void) { Reset(); }
	bool IsAllocated(void) const { return m_ptr != nullptr; }

	T& operator[](size_t index) { return m_ptr[index]; }
	const T& operator[](size_t index) const { return m_ptr[index]; }
	T* Get(void) const { return m_ptr; }
};

template <typename T>
class CArray
{
private:
		CPtr<T> m_array{nullptr};
		int m_size{0};
		int m_capacity{0};
public:
		CArray(const int size = 0) : m_size(0), m_capacity(0)
		{
				if (size > 0)
			Resize(size, true);
		}

		CArray(const CArray& other) : m_size(0), m_capacity(0)
		{
				if (other.m_size > 0 && Resize(other.m_size))
		{
			int i;
						for (i = 0; i < other.m_size; ++i)
								m_array[i] = other.m_array[i];

						m_size = other.m_size;
				}
		}

		CArray& operator=(const CArray& other)
		{
				if (this != &other)
		{
						CArray temp(other);
						Swap(temp);
				}

				return *this;
		}

		CArray(CArray&& other) noexcept : m_array(std::move(other.m_array)), m_size(other.m_size), m_capacity(other.m_capacity)
		{
				other.m_size = 0;
				other.m_capacity = 0;
		}

		CArray& operator=(CArray&& other) noexcept
		{
				if (this != &other)
		{
						Destroy();
						m_array = std::move(other.m_array);
						m_size = other.m_size;
						m_capacity = other.m_capacity;
						other.m_size = 0;
						other.m_capacity = 0;
				}

				return *this;
		}

		void Swap(CArray& other) noexcept
		{
				cswap(m_array, other.m_array);
				cswap(m_size, other.m_size);
				cswap(m_capacity, other.m_capacity);
		}

		inline bool Resize(const int size, const bool reset = false)
		{
				if (size < 0)
			return false;

				if (size == 0)
		{
			Destroy();
			return true;
		}

				T* newMemory = new(std::nothrow) T[size];
				if (!newMemory)
			return false;

				CPtr<T> temp_ptr(newMemory, true);
				if (!temp_ptr.IsAllocated())
						return false;

				if (reset || !m_array.IsAllocated())
				{
						m_array.Reset(temp_ptr.Release());
						if (m_array.IsAllocated())
						{
								m_size = 0;
								m_capacity = size;
								return true;
						}
						return false;
				}

				int i;
				int copy_count = (m_size < size) ? m_size : size;
				for (i = 0; i < copy_count; ++i)
						temp_ptr[i] = std::move(m_array[i]);

				m_array.Reset(temp_ptr.Release());
				m_capacity = size;
				m_size = (size < m_size) ? size : m_size;
				return true;
		}

		inline void Destroy(void)
		{
				m_array.Destroy();
				m_size = 0;
				m_capacity = 0;
		}

		inline T& Get(const int index)
		{
				if (m_size <= 0 || !m_array.IsAllocated())
		{
						thread_local T dummy;
						dummy = T{};
						return dummy;
				}

				if (index < 0)
			return m_array[0];

				if (index >= m_size)
			return m_array[m_size - 1];

				return m_array[index];
		}
		
		inline bool Push(const T& element, const bool autoSize = true)
		{
				if (m_size >= m_capacity)
		{
						if (!autoSize)
				return false;

						if (!Resize((m_capacity == 0) ? 1 : m_capacity * 2))
				return false;
				}

				m_array[m_size] = element;
				m_size++;
				return true;
		}

		inline bool Push(T&& element, const bool autoSize = true)
		{
				if (m_size >= m_capacity)
		{
						if (!autoSize)
				return false;

						if (!Resize((m_capacity == 0) ? 1 : m_capacity * 2))
				return false;
				}

				m_array[m_size] = std::move(element);
				m_size++;
				return true;
		}

		inline bool Has(const T& element) const
		{
				if (m_array.IsAllocated())
		{
			int i;
						for (i = 0; i < m_size; i++)
			{
								if (m_array[i] == element)
					return true;
						}
				}

				return false;
		}

		inline void RemoveAt(const int index)
		{
				if (index < 0 || index >= m_size)
			return;

				if (m_array.IsAllocated())
		{
			int i;
						for (i = index; i < m_size - 1; i++)
								m_array[i] = std::move(m_array[i + 1]);

						m_size--;
				}
		}

		inline bool Remove(const T& element)
		{
				if (m_array.IsAllocated())
		{
			int i;
						for (i = 0; i < m_size; i++)
			{
								if (m_array[i] == element)
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
				if (m_array.IsAllocated() && m_size > 1)
		{
			int i;
						int half = m_size / 2;
						int val = m_size - 1;
						for (i = 0; i < half; i++)
								cswap(m_array[i], m_array[val - i]);
				}
		}

		inline T Pop(void)
		{
				if (m_size <= 0 || !m_array.IsAllocated())
						return T{};

				T element = std::move(m_array[m_size - 1]);
				m_size--;
				return element;
		}

		inline void PopNoReturn(void)
		{
				if (m_size > 0 && m_array.IsAllocated())
						m_size--;
		}

		inline T& Last(void)
		{
				if (m_size <= 0 || !m_array.IsAllocated())
		{
						thread_local T dummy;
						dummy = T{};
						return dummy;
				}

				return m_array[m_size - 1];
		}

		inline bool IsEmpty(void) const { return m_size < 1; }
		inline int Size(void) const { return m_size; }
		inline bool NotAllocated(void) const { return m_capacity < 1; }
		inline int Capacity(void) const { return m_capacity; }

		inline T& Random(void)
		{
				if (m_size <= 0 || !m_array.IsAllocated())
		{
						thread_local T dummy;
						dummy = T{};
						return dummy;
				}

				return Get(crandomint(0, m_size - 1));
		}

		template <typename T2>
		inline T& operator[] (const T2 index)
		{
				if (m_capacity <= 0 || !m_array.IsAllocated())
				{
						thread_local T dummy;
						dummy = T{};
						return dummy;
				}
				const int i = static_cast<int>(index);
				if (i < 0) return m_array[0];
				if (i >= m_capacity) return m_array[m_capacity - 1];
				return m_array[i];
		}

		template <typename T2>
		inline const T& operator[] (const T2 index) const
		{
				static T s_dummy = T{};
				if (m_capacity <= 0 || !m_array.IsAllocated())
						return s_dummy;
				const int i = static_cast<int>(index);
				if (i < 0) return m_array[0];
				if (i >= m_capacity) return m_array[m_capacity - 1];
				return m_array[i];
		}
};