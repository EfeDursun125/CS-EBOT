//
// Custom Lib For E-Bot
// E-Bot is ai controlled players for counter-strike 1.6
// 
// For reduce glibc requirements and take full advantage of compiler optimizations
// And to get same results/performance on every OS
//

#include <core.h>

#define USE_SSE2
#include <sse_mathfun.h>
#include <sse_mathfun_extension.h>

float csqrtf(const float value)
{
	return _mm_cvtss_f32(_mm_sqrt_ss(_mm_load_ss(&value)));
}

float crsqrtf(const float value)
{
	return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_load_ss(&value)));
}

float ccosf(const float value)
{
	return _mm_cvtss_f32(cos_ps(_mm_load_ss(&value)));
}

float csinf(const float value)
{
	return _mm_cvtss_f32(sin_ps(_mm_load_ss(&value)));
}

void csincosf(const float radians, float& sine, float& cosine)
{
	__m128 sse_s, sse_c;
	__m128* s = &sse_s;
	__m128* c = &sse_c;
	sincos_ps(_mm_load_ss(&radians), s, c);
	sine = _mm_cvtss_f32(*s);
	cosine = _mm_cvtss_f32(*c);
}

float catan2f(const float x, const float y)
{
	return _mm_cvtss_f32(atan2_ps(_mm_load_ss(&x), _mm_load_ss(&y)));
}

float ctanf(const float value)
{
	return _mm_cvtss_f32(tan_ps(_mm_load_ss(&value)));
}