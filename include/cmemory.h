//
// Custom Lib For E-Bot
// E-Bot is ai controlled players for counter-strike 1.6
// 
// For reduce glibc requirements and take full advantage of compiler optimizations
// And to get same results/performance on every OS
//

#ifndef CMEMORY_INCLUDED
#define CMEMORY_INCLUDED

#include <memory>

#ifndef WIN32
__asm__(".symver malloc,malloc@GLIBC_2.0");
__asm__(".symver realloc,realloc@GLIBC_2.0");
__asm__(".symver free,free@GLIBC_2.0");
#endif

namespace c
{
    inline void memcpy(void* dest, const void* src, const size_t size)
    {
        char* dest2 = static_cast<char*>(dest);
        const char* src2 = static_cast<const char*>(src);

        size_t cache;
        for (cache = 0; cache < size; cache++)
            dest2[cache] = src2[cache];
    }

    inline void memset(void* dest, const int value, const size_t count)
    {
        unsigned char* ptr = static_cast<unsigned char*>(dest);
        const unsigned char byteValue = static_cast<unsigned char>(value);

        size_t cache;
        for (cache = 0; cache < count; cache++)
        {
            *ptr = byteValue;
            ptr++;
        }
    }

    inline void* memmove(void* dest, const void* src, size_t count)
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

    // frees the allocated memory in the safe way
    template <typename T>
    inline void free(T* &ptr)
    {
        if (ptr != nullptr)
        {
            std::free(ptr);
            ptr = nullptr;
        }
    }

    template <typename T>
    inline void malloc(T* &ptr)
    {
        for (;;)
        {
            if (ptr != nullptr)
            {
                std::free(ptr);
                ptr = nullptr;
            }

            ptr = static_cast<T*>(std::malloc(sizeof(T)));
            if (ptr != nullptr)
                break;
        }
    }

    template <typename T>
    inline void malloc(T* &ptr, const size_t count)
    {
        for (;;)
        {
            if (ptr != nullptr)
            {
                std::free(ptr);
                ptr = nullptr;
            }

            ptr = static_cast<T*>(std::malloc(count * sizeof(T)));
            if (ptr != nullptr)
                break;
        }
    }

    inline void* malloc(const size_t count)
    {
        void* ptr = nullptr;

        for (;;)
        {
            ptr = std::malloc(count);
            if (ptr != nullptr)
                break;
        }

        return ptr;
    }

    inline void* realloc(void* old, const size_t count)
    {
        void* ptr = nullptr;

        for (;;)
        {
            ptr = std::realloc(old, count);
            if (ptr != nullptr)
                break;
        }
    }
}

#endif