#pragma once

#include "BLASrely.h"
#include "VecBase.hpp"

namespace miniBLAS
{

/*vector contains 4 int*/
class alignas(16) VecI4 :public Vec4Base<int32_t>
{
protected:
public:
	using Vec4Base::x; using Vec4Base::y; using Vec4Base::z; using Vec4Base::w; using Vec4Base::data;

	VecI4(const bool setZero = false) noexcept
	{
		if (setZero)
		{
		#if COMMON_SIMD_LV >= 20
			int_dat = _mm_setzero_si128();
		#else
			x = y = z = w = 0;
		#endif
		}
	}
	template<class T>
	VecI4(const T x_, const T y_, const T z_, const T w_) noexcept
		:Vec4Base(static_cast<int32_t>(x_), static_cast<int32_t>(y_), static_cast<int32_t>(z_), static_cast<int32_t>(w_))
	{ }
	template<class T>
	VecI4(const T *ptr) noexcept
		:Vec4Base(static_cast<int32_t>(ptr[0]), static_cast<int32_t>(ptr[1]), static_cast<int32_t>(ptr[2]), static_cast<int32_t>(ptr[3]))
	{ }
#if COMMON_SIMD_LV >= 20
	VecI4(const int32_t *ptr) noexcept { int_dat = _mm_loadu_si128((__m128i*)ptr); }
	VecI4(const __m128i& dat_) noexcept { Store128I(int_dat, dat_); };
#endif
	//operator Raw&() noexcept { return data; }
	//operator const Raw&() const noexcept { return data; }
#if COMMON_SIMD_LV >= 20
	operator __m128i&() noexcept { return int_dat; };
	operator const __m128i() const noexcept { return Load128I(int_dat); };
#endif

	bool operator<(const VecI4& other) const = delete;

	int32_t dot(const VecI4& v) const//dot product
	{
		return x*v.x + y*v.y + z*v.z + w*v.w;
	}

	
	VecI4& operator+=(const VecI4& right)
	{
	#if COMMON_SIMD_LV >= 20
		return *this = _mm_add_epi32(int_dat, right);
	#else
		x += right.x, y += right.y, z += right.z, w += right.w;
		return *this;
	#endif
	}

	VecI4& operator-=(const VecI4& right)
	{
	#if COMMON_SIMD_LV >= 20
		return *this = _mm_sub_epi32(int_dat, right);
	#else
		x -= right.x, y -= right.y, z -= right.z, w -= right.w;
		return *this;
	#endif
	}

	VecI4& operator*=(const int32_t right)
	{
	#if COMMON_SIMD_LV >= 20
		return *this = _mm_mul_epi32(int_dat, _mm_set1_epi32(right));
	#else
		x *= right, y *= right, z *= right, w *= right;
		return *this;
	#endif
	}

	VecI4& operator*=(const VecI4& right)
	{
	#if COMMON_SIMD_LV >= 20
		return *this = _mm_mul_epi32(int_dat, right.int_dat);
	#else
		x *= right.x, y *= right.y, z *= right.z, w *= right.w;
		return *this;
	#endif
	}

	VecI4& operator/=(const int32_t right)
	{
		x /= right, y /= right, z /= right, w /= right;
		return *this;
	}

	VecI4& operator/=(const VecI4& right)
	{
		x /= right.x, y /= right.y, z /= right.z, w /= right.w;
		return *this;
	}

};


inline VecI4 VECCALL operator+(const VecI4& l, const VecI4& r)
{
#if COMMON_SIMD_LV >= 20
	return _mm_add_epi32(l, r);
#else
	return VecI4(l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w);
#endif
}

inline VecI4 VECCALL operator+(const VecI4& l, const int r)
{
#if COMMON_SIMD_LV >= 20
	return _mm_add_epi32(l, _mm_set1_epi32(r));
#else
	return VecI4(l.x + r, l.y + r, l.z + r, l.w + r);
#endif
}

inline VecI4 VECCALL operator-(const VecI4& l, const VecI4& r)
{
#if COMMON_SIMD_LV >= 20
	return _mm_sub_epi32(l, r);
#else
	return VecI4(l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w);
#endif
}

inline VecI4 VECCALL operator-(const VecI4& l, const int r)
{
#if COMMON_SIMD_LV >= 20
	return _mm_sub_epi32(l, _mm_set1_epi32(r));
#else
	return VecI4(l.x - r, l.y - r, l.z - r, l.w - r);
#endif
}

inline VecI4 VECCALL operator*(const VecI4& l, const int n)
{
#if COMMON_SIMD_LV >= 20
	return _mm_mul_epi32(l, _mm_set1_epi32(n));
#else
	return VecI4(l.x * n, l.y * n, l.z * n, l.w * n);
#endif
}

inline VecI4 VECCALL operator*(const VecI4& l, const VecI4& r)
{
#if COMMON_SIMD_LV >= 20
	return _mm_mul_epi32(l, r);
#else
	return VecI4(l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w);
#endif
}

inline VecI4 VECCALL operator*(const int32_t n, const VecI4& v)
{
	return v*n;
}

inline VecI4 VECCALL operator/(const VecI4& l, const int n)
{
	return VecI4(l.x / n, l.y / n, l.z / n, l.w / n);
}

inline VecI4 VECCALL operator/(const VecI4& l, const VecI4& r)
{
	return VecI4(l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w);
}



}