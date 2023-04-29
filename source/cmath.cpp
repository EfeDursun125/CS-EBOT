#include <core.h>
#include <immintrin.h>
#include <xmmintrin.h>

float squareRoot(float number)
{
#ifdef USE_AVX2
	__m256 nm = _mm256_loadu_ps(&number);
	__m256 x2 = _mm256_mul_ps(nm, _mm256_set1_ps(0.5f));
	__m256 y = nm;
	__m256i i = _mm256_castps_si256(y);
	i = _mm256_sub_epi32(_mm256_set1_epi32(0x5f3759df), _mm256_srli_epi32(i, 1));
	y = _mm256_castsi256_ps(i);
	y = _mm256_mul_ps(y, nm);
	return _mm256_cvtss_f32(y);
#elif USE_AVX
	return _mm256_cvtss_f32(_mm256_sqrt_ps(_mm256_loadu_ps(&number)));
#else
	__m128 nm = _mm_loadu_ps(&number);
	__m128 x2 = _mm_mul_ps(nm, _mm_set1_ps(0.5f));
	__m128 y = nm;
	__m128i i = _mm_castps_si128(y);
	i = _mm_sub_epi32(_mm_set1_epi32(0x5f3759df), _mm_srli_epi32(i, 1));
	y = _mm_castsi128_ps(i);
	y = _mm_mul_ps(y, nm);
	return _mm_cvtss_f32(y);
#endif
}

float rsqrtf(float number)
{
#ifdef USE_AVX2
	__m256 nm = _mm256_loadu_ps(&number);
	__m256 x2 = _mm256_mul_ps(nm, _mm256_set1_ps(0.5f));
	__m256 y = nm;
	__m256i i = _mm256_castps_si256(y);
	i = _mm256_sub_epi32(_mm256_set1_epi32(0x5f3759df), _mm256_srli_epi32(i, 1));
	y = _mm256_castsi256_ps(i);
	return _mm256_cvtss_f32(y);
#elif USE_AVX
	return _mm256_cvtss_f32(_mm256_rsqrt_ps(_mm256_loadu_ps(&number)));
#else
	__m128 nm = _mm_loadu_ps(&number);
	__m128 x2 = _mm_mul_ps(nm, _mm_set1_ps(0.5f));
	__m128 y = nm;
	__m128i i = _mm_castps_si128(y);
	i = _mm_sub_epi32(_mm_set1_epi32(0x5f3759df), _mm_srli_epi32(i, 1));
	y = _mm_castsi128_ps(i);
	return _mm_cvtss_f32(y);
#endif
}

float power(float x, float y)
{
	return powf(x, y);
}

float tanf_sse(float x)
{
	float x2 = x * x;
	float x3 = x * x2;
	float x5 = x3 * x2;
	float x7 = x5 * x2;

	const float c3 = 0.33333333333f;
	const float c5 = 0.13333333333f;
	const float c7 = 0.05396825396f;

#if defined (USE_AVX2) || defined (USE_AVX)
	__m256 X = _mm256_set_ps(x, x3 * c3, x5 * c5, x7 * c7, 0.0f, 0.0f, 0.0f, 0.0f);
	__m256 C = _mm256_set1_ps(1.0f);
	__m256 T = _mm256_setzero_ps();
	T = _mm256_add_ps(T, X);
	C = _mm256_mul_ps(C, _mm256_set1_ps(-c3));
	X = _mm256_mul_ps(X, _mm256_set_ps(x2, x2, x2, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	T = _mm256_add_ps(T, _mm256_mul_ps(X, C));
	C = _mm256_mul_ps(C, _mm256_set1_ps(-c5));
	X = _mm256_mul_ps(X, _mm256_set_ps(x2, x2, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	T = _mm256_add_ps(T, _mm256_mul_ps(X, C));
	C = _mm256_mul_ps(C, _mm256_set1_ps(-c7));
	X = _mm256_mul_ps(X, _mm256_set_ps(x2, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	T = _mm256_add_ps(T, _mm256_mul_ps(X, C));

	return _mm256_cvtss_f32(T);
#else
	__m128 X = _mm_set_ps(x, x3 * c3, x5 * c5, x7 * c7);
	__m128 C = _mm_set_ps1(1.0f);
	__m128 T = _mm_setzero_ps();
	T = _mm_add_ps(T, X);
	C = _mm_mul_ps(C, _mm_set_ps1(-c3));
	X = _mm_mul_ps(X, _mm_set_ps(x2, x2, x2, 0.0f));
	T = _mm_add_ps(T, _mm_mul_ps(X, C));
	C = _mm_mul_ps(C, _mm_set_ps1(-c5));
	X = _mm_mul_ps(X, _mm_set_ps(x2, x2, 0.0f, 0.0f));
	T = _mm_add_ps(T, _mm_mul_ps(X, C));
	C = _mm_mul_ps(C, _mm_set_ps1(-c7));
	X = _mm_mul_ps(X, _mm_set_ps(x2, 0.0f, 0.0f, 0.0f));
	T = _mm_add_ps(T, _mm_mul_ps(X, C));

	return _mm_cvtss_f32(T);
#endif
}