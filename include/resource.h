#ifndef RESOURCE_INCLUDED
#define RESOURCE_INCLUDED

#define PRODUCT_DEV_VERSION_FORTEST ""

// E-Bot Version
#define PRODUCT_VERSION_DWORD 109,20230326,10 // yyyy/mm/dd
#define PRODUCT_VERSION "1.09"
#define PRODUCT_VERSION_F 1.09

// general product information
#define PRODUCT_NAME "E-BOT"
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

