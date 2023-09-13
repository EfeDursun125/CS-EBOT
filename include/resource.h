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
// $Id:$
//


#ifndef RESOURCE_INCLUDED
#define RESOURCE_INCLUDED

// This is test version?
// #define PRODUCT_DEV_VERSION

#if defined(PRODUCT_DEV_VERSION)
#define PRODUCT_DEV_VERSION_FORTEST "(PRV)"
//#define PRODUCT_DEV_VERSION_FORTEST "(DEV)"
//#define PRODUCT_DEV_VERSION_FORTEST "(Preview-2)"
// Dev Version Date
#define PV_VERSION_YEAR 2017
#define PV_VERSION_MON 2
#define PV_VERSION_DAY 10
#else
#define PRODUCT_DEV_VERSION_FORTEST ""
#endif

// E-BOT Version
#define PRODUCT_VERSION_DWORD 1,07,20230213,8 // yyyy/mm/dd
#define PRODUCT_VERSION "1.07"
#define PRODUCT_VERSION_F 1.07

// general product information
#define PRODUCT_NAME "E-BOT LEGACY"
#define PRODUCT_AUTHOR "EfeDursun125"
#define PRODUCT_URL "ebots-for-cs.blogspot.com"
#define PRODUCT_EMAIL "efedursun91@gmail.com"
#define PRODUCT_LOGTAG "ebot"
#define PRODUCT_DESCRIPTION "AI bot for Counter-Strike Series"
#define PRODUCT_COPYRIGHT PRODUCT_AUTHOR
#define PRODUCT_LEGAL "Half-Life, Counter-Strike, Steam, Valve is a trademark of Valve Corporation"
#define PRODUCT_ORIGINAL_NAME "ebot.dll"
#define PRODUCT_INTERNAL_NAME "ebot"
#define PRODUCT_SUPPORT_VERSION "1.0 - CZ"
#define PRODUCT_DATE __DATE__

// product optimization type (we're not using crt builds anymore)
#ifndef PRODUCT_OPT_TYPE
#if defined (_DEBUG)
#   if defined (_AFXDLL)
#      define PRODUCT_OPT_TYPE "Debug Build (CRT)"
#   else
#      define PRODUCT_OPT_TYPE "Debug Build"
#   endif
#elif defined (NDEBUG)
#   if defined (_AFXDLL)
#      define PRODUCT_OPT_TYPE "Optimized Build (CRT)"
#   else
#      define PRODUCT_OPT_TYPE "Optimized Build"
#   endif
#else
#   define PRODUCT_OPT_TYPE "Default Release"
#endif
#endif

#endif // RESOURCE_INCLUDED

