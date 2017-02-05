#pragma once

#include "BLASrely.h"
#include "VecBase.hpp"

namespace miniBLAS
{

/*vector contains 4 int*/
class alignas(16) VecI4 :public Vec4Base<int>
{
protected:
public:
	using Vec4Base::x; using Vec4Base::y; using Vec4Base::z; using Vec4Base::w;

	VecI4(const bool setZero = false)
	{
		if (setZero)
		{
		#ifdef __SSE2__
			int_dat = _mm_setzero_si128();
		#else
			x = y = z = w = 0;
		#endif
		}
	}
	template<class T>
	VecI4(const T x_, const T y_, const T z_, const T w_)
		:Vec4Base(static_cast<int>(x_), static_cast<int>(y_), static_cast<int>(z_), static_cast<int>(w_))
	{
	};
	template<class T>
	VecI4(const T *ptr)
		:Vec4Base(static_cast<int>(ptr[0]), static_cast<int>(ptr[1]), static_cast<int>(ptr[2]), static_cast<int>(ptr[3]))
	{
	};
#ifdef __SSE2__
	VecI4(const int32_t *ptr) { int_dat = _mm_loadu_si128((__m128i*)ptr); }
#endif
	VecI4(const __m128i& dat_) { int_dat = dat_; };

	operator int*() { return data; };
	operator const int*() const { return data; };
	operator __m128i&() { return int_dat; };
	operator const __m128i&() const { return int_dat; };
	operator Vec3&() { return *this; }
	operator const Vec3&() const { return *this; }

	VecI4 operator+(const VecI4& v) const
	{
	#ifdef __SSE2__
		return _mm_add_epi32(int_dat, v);
	#else
		return VecI4(x + v.x, y + v.y, z + v.z, w + v.w);
	#endif
	}

	VecI4 operator-(const VecI4& v) const
	{
	#ifdef __SSE2__
		return _mm_sub_epi32(int_dat, v);
	#else
		return VecI4(x - v.x, y - v.y, z - v.z, w - v.w);
	#endif
	}

	VecI4 operator*(const int& n) const
	{
	#ifdef __SSE2__
		return _mm_mul_epi32(int_dat, _mm_set1_epi32(n));
	#else
		return VecI4(x * n, y * n, z * n, w * n);
	#endif
	}

	VecI4 operator*(const VecI4& v) const
	{
	#ifdef __SSE2__
		return _mm_mul_epi32(int_dat, v.int_dat);
	#else
		return VecI4(x * v.x, y * v.y, z * v.z, w * v.w);
	#endif
	}

	VecI4 operator/(const int& n) const
	{
		return VecI4(x / n, y / n, z / n, w / n);
	}

	VecI4 operator/(const VecI4& v) const
	{
		return VecI4(x / v.x, y / v.y, z / v.z, w / v.w);
	}

	int32_t dot(const VecI4& v) const//dot product
	{
		return x*v.x + y*v.y + z*v.z + w*v.w;
	}

	
	VecI4& operator+=(const VecI4& right)
	{
	#ifdef __SSE2__
		return *this = _mm_add_epi32(int_dat, right);
	#else
		x += right.x, y += right.y, z += right.z, w += right.w;
		return *this;
	#endif
	}

	VecI4& operator-=(const VecI4& right)
	{
	#ifdef __SSE2__
		return *this = _mm_sub_epi32(int_dat, right);
	#else
		x -= right.x, y -= right.y, z -= right.z, w -= right.w;
		return *this;
	#endif
	}

	VecI4& operator*=(const int32_t right)
	{
	#ifdef __SSE2__
		return *this = _mm_mul_epi32(int_dat, _mm_set1_epi32(right));
	#else
		x *= right, y *= right, z *= right, w *= right;
		return *this;
	#endif
	}

	VecI4& operator*=(const VecI4& right)
	{
	#ifdef __SSE2__
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

inline VecI4 operator*(const int32_t n, const VecI4& v)
{
	return v*n;
}


}