//
// Copyright (c) 2003-2009, by Yet Another POD-Bot Development Team.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// $Id$
//

#ifndef PLATFORM_INCLUDED
#define PLATFORM_INCLUDED

// detects the build platform
#if defined (__linux__) || defined (__debian__) || defined (__linux) || (__x86_64__) || defined (__amd64__)
#define PLATFORM_LINUX
#elif defined (_WIN32)
#define PLATFORM_WIN32
#endif

// detects the compiler
#if defined (_MSC_VER)
#define COMPILER_VISUALC _MSC_VER
#elif defined (__BORLANDC__)
#define COMPILER_BORLAND __BORLANDC__
#elif defined (__MINGW32__)
#define COMPILER_MINGW32 __MINGW32__
#endif

// configure export macros
#if defined (COMPILER_VISUALC) || defined (COMPILER_MINGW32)
#define exportc extern "C" __declspec (dllexport)
#elif defined (PLATFORM_LINUX) || defined (COMPILER_BORLAND)
#define exportc extern "C" __attribute__((visibility("default")))
#else
#error "Can't configure export macros. Compiler unrecognized."
#endif

// operating system specific macros, functions and typedefs
#ifdef PLATFORM_WIN32

#include <direct.h>

#define DLL_ENTRYPOINT int STDCALL DllMain (void *, unsigned long dwReason, void *)
#define DLL_DETACHING (dwReason == 0)
#define DLL_RETENTRY return 1

#if defined (COMPILER_VISUALC)
#define DLL_GIVEFNPTRSTODLL extern "C" void STDCALL
#elif defined (COMPILER_MINGW32)
#define DLL_GIVEFNPTRSTODLL exportc void STDCALL
#endif

// specify export parameter
#if defined (COMPILER_VISUALC) && (COMPILER_VISUALC > 1000)
#pragma comment (linker, "/EXPORT:GiveFnptrsToDll=_GiveFnptrsToDll@8,@1")
#pragma comment (linker, "/SECTION:.data,RW")
#endif

typedef int (*EntityAPI_t) (DLL_FUNCTIONS*, int);
typedef int (*NewEntityAPI_t) (NEW_DLL_FUNCTIONS*, int*);
typedef int (*BlendAPI_t) (int, void**, void*, float(*)[3][4], float(*)[128][3][4]);
typedef void(__stdcall* FuncPointers_t) (enginefuncs_t*, globalvars_t*);
typedef void (*EntityPtr_t) (entvars_t*);

#elif defined (PLATFORM_LINUX)

#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>
#include <sys/stat.h>

#define DLL_ENTRYPOINT __attribute__((destructor))  void _fini (void)
#define DLL_DETACHING TRUE
#define DLL_RETENTRY return
#define DLL_GIVEFNPTRSTODLL extern "C" void __attribute__((visibility("default")))

typedef int (*EntityAPI_t) (DLL_FUNCTIONS*, int);
typedef int (*NewEntityAPI_t) (NEW_DLL_FUNCTIONS*, int*);
typedef int (*BlendAPI_t) (int, void**, void*, float(*)[3][4], float(*)[128][3][4]);
typedef void (*FuncPointers_t) (enginefuncs_t*, globalvars_t*);
typedef void (*EntityPtr_t) (entvars_t*);

#else
#error "Platform unrecognized."
#endif

#ifdef PLATFORM_WIN32
#include "urlmon.h"
#pragma comment(lib, "urlmon.lib")

class Library
{
private:
    HMODULE m_ptr;

public:

    Library(const char* fileName)
    {
        if (fileName == nullptr)
            return;

        m_ptr = LoadLibraryA(fileName);
    }

    ~Library(void)
    {
        if (!IsLoaded())
            return;

        FreeLibrary(m_ptr);
    }

public:
    void* GetFunctionAddr(const char* functionName)
    {
        if (!IsLoaded())
            return nullptr;

        return GetProcAddress(m_ptr, functionName);
    }

    inline bool IsLoaded(void) const
    {
        return m_ptr != nullptr;
    }
};

#else

class Library
{
private:
    void* m_ptr;

public:

    Library(const char* fileName)
    {
        if (fileName == nullptr)
            return;

        m_ptr = dlopen(fileName, RTLD_NOW);
    }

    ~Library(void)
    {
        if (!IsLoaded())
            return;

        dlclose(m_ptr);
    }

public:
    void* GetFunctionAddr(const char* functionName)
    {
        if (!IsLoaded())
            return nullptr;

        return dlsym(m_ptr, functionName);
    }

    inline bool IsLoaded(void) const
    {
        return m_ptr != nullptr;
    }
};
#endif
#endif