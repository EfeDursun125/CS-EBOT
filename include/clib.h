//
// Custom Lib For E-Bot
// E-Bot is ai controlled players for counter-strike 1.6
// 
// For reduce glibc requirements and take full advantage of compiler optimizations
// And to get same results/performance on every OS
//

extern int CRandomInt(const int min, const int max);
extern float CRandomFloat(const float min, const float max);
extern bool ChanceOf(const int number);
extern float SquaredF(const float value);
extern float SquaredI(const int value);
extern int Squared(const int value);
extern float AddTime(const float time);
extern float cclampf(const float a, const float b, const float c);
extern float cmaxf(const float a, const float b);
extern float cminf(const float a, const float b);
extern int cmax(const int a, const int b);
extern int cmin(const int a, const int b);
extern float csqrtf(const float value);
extern float crsqrtf(const float value);
extern float cpowf(const float x, size_t y);
extern float cabsf(const float value);
extern int cabs(const int value);
extern float cceilf(const float value);
extern float cfloorf(const float value);
extern float croundf(const float value);
extern size_t cstrlen(const char* str);
extern int cstrcmp(const char* str1, const char* str2);
extern int cstrncmp(const char* str1, const char* str2, const size_t num);
extern void cstrcpy(char* dest, const char* src);
extern void cmemcpy(void* dest, const void* src, const size_t size);
extern void cmemset(void* dest, const int value, const size_t count);
extern void* cmemmove(void* dest, const void* src, size_t count);
extern int cctz(unsigned int value);
extern int ctolower(const int value);
extern int ctoupper(const int value);
extern int cstricmp(const char* str1, const char* str2);
extern bool cspace(const int str);
extern void cstrtrim(char* string);
extern char* cstrstr(char* str1, const char* str2);
extern char* cstrncpy(char* dest, const char* src, const size_t count);
extern char* cstrcat(char* dest, const char* src);
extern int catoi(const char* str);
extern float catof(const char* str);