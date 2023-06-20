//
// Custom Math For EBot
//

#pragma once

extern int CRandomInt(const int min, const int max);
extern bool ChanceOf(const int number);
extern float SquaredF(const float value);
extern float AddTime(const float time);
extern float cclampf(const float a, const float b, const float c);
extern float cmaxf(const float a, const float b);
extern float cminf(const float a, const float b);
extern int cmax(const int a, const int b);
extern int cmin(const int a, const int b);
extern float csqrtf(const float value);
extern float crsqrtf(const float value);
extern float cabsf(const float value);
extern int cabs(const int value);
extern float cceilf(const float value);
extern float cfloorf(const float value);
extern float croundf(const float value);
extern size_t cstrlen(const char* str);