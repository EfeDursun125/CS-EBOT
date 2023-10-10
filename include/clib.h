//
// Custom Lib For E-Bot
// E-Bot is ai controlled players for counter-strike 1.6
// 
// For reduce glibc requirements and take full advantage of compiler optimizations
// And to get same results/performance on every OS
//

#pragma once

extern int crandomint(const int min, const int max);
extern float crandomfloat(const float min, const float max);
extern bool chanceof(const int number);
extern float squaredf(const float value);
extern float squaredi(const int value);
extern int squared(const int value);
extern float cclampf(const float a, const float b, const float c);
extern int cclamp(const int a, const int b, const int c);
extern float cmaxf(const float a, const float b);
extern float cminf(const float a, const float b);
extern int cmax(const int a, const int b);
extern int cmin(const int a, const int b);
extern float csqrtf(const float value);
extern float crsqrtf(const float value);
extern float ccosf(const float value);
extern float csinf(const float value);
extern void csincosf(const float radians, float& sine, float& cosine);
extern float catan2f(const float x, const float y);
extern float ctanf(const float value);
extern float cpowf(const float a, const float b);
extern float cabsf(const float value);
extern int cabs(const int value);
extern float cceilf(const float value);
extern double cceil(const double value);
extern float cfloorf(const float value);
extern double cfloor(const double value);
extern float croundf(const float value);
extern double cround(const double value);
extern size_t cstrlen(const char* str);
extern int cstrcmp(const char* str1, const char* str2);
extern int cstrncmp(const char* str1, const char* str2, const size_t num);
extern void cstrcpy(char* dest, const char* src);
extern int cctz(unsigned int value);
extern int ctolower(const int value);
extern int ctoupper(const int value);
extern int cstricmp(const char* str1, const char* str2);
extern bool cspace(const int str);
extern void cstrtrim(char* string);
extern char* cstrstr(char* str1, const char* str2);
extern char* cstrncpy(char* dest, const char* src, const size_t count);
extern char* cstrcat(char* dest, const char* src);
extern int cstrcoll(const char* str1, const char* str2);
extern size_t cstrspn(const char* str, const char* charset);
extern size_t cstrcspn(const char* str, const char* charset);
extern int catoi(const char* str);
extern float catof(const char* str);
extern int frand(void);