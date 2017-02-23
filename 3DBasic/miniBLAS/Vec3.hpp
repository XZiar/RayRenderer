#pragma once

#include "BLASrely.h"
#include "VecBase.hpp"

namespace miniBLAS
{

Vec3 VECCALL operator/(const Vec3& left, const float right);

/*vector contains 4 float, while only xyz are considered in some calculation*/
class alignas(16) Vec3 :public Vec4Base<float>
{
private:
protected:
public:
	using Vec4Base::x; using Vec4Base::y; using Vec4Base::z;

	explicit Vec3(const bool setZero = false)
	{
		if (setZero)
		{
		#ifdef __SSE2__
			float_dat = _mm_setzero_ps();
		#else
			x = y = z = 0;
		#endif
		}
	}
	template<class T>
	Vec3(const T x_, const T y_, const T z_, const T w_ = static_cast<T>(0))
		:Vec4Base(static_cast<float>(x_), static_cast<float>(y_), static_cast<float>(z_), static_cast<float>(w_)) { };
	template<class T>
	explicit Vec3(const T *ptr) :Vec4Base(static_cast<float>(ptr[0]), static_cast<float>(ptr[1]), static_cast<float>(ptr[2])) { };
#ifdef __SSE2__
	explicit Vec3(const float *ptr) { float_dat = _mm_loadu_ps(ptr); }
	Vec3(const __m128& dat_) { float_dat = dat_; };
#endif

	VECCALL operator float*() { return data; };
	VECCALL operator const float*() const { return data; };
#ifdef __SSE2__
	VECCALL operator __m128&() { return float_dat; };
	VECCALL operator const __m128&() const { return float_dat; };
#endif

	bool operator<(const Vec3& other) const = delete;

	Vec3 VECCALL cross(const Vec3& v) const
	{
	#ifdef __AVX__
		const __m128 t1 = _mm_permute_ps(float_dat, _MM_SHUFFLE(3, 0, 2, 1))/*y,z,x,w*/;
		const __m128 t2 = _mm_permute_ps(v.float_dat, _MM_SHUFFLE(3, 1, 0, 2))/*v.z,v.x,v.y,v.w*/;
		const __m128 t3 = _mm_permute_ps(float_dat, _MM_SHUFFLE(3, 1, 0, 2))/*z,x,y,w*/;
		const __m128 t4 = _mm_permute_ps(v.float_dat, _MM_SHUFFLE(3, 0, 2, 1))/*v.y,v.z,v.x,v.w*/;
		return _mm_sub_ps(_mm_mul_ps(t1, t2), _mm_mul_ps(t3, t4));
	#elif defined(__SSE2__)
		const __m128 t1 = _mm_shuffle_ps(float_dat, float_dat, _MM_SHUFFLE(3, 0, 2, 1))/*y,z,x,w*/;
		const __m128 t2 = _mm_shuffle_ps(v.float_dat, v.float_dat, _MM_SHUFFLE(3, 1, 0, 2))/*v.z,v.x,v.y,v.w*/;
		const __m128 t3 = _mm_shuffle_ps(float_dat, float_dat, _MM_SHUFFLE(3, 1, 0, 2))/*z,x,y,w*/;
		const __m128 t4 = _mm_shuffle_ps(v.float_dat, v.float_dat, _MM_SHUFFLE(3, 0, 2, 1))/*v.y,v.z,v.x,v.w*/;
		return _mm_sub_ps(_mm_mul_ps(t1, t2), _mm_mul_ps(t3, t4));
	#else
		const float a = y*v.z - z*v.y;
		const float b = z*v.x - x*v.z;
		const float c = x*v.y - y*v.x;
		return Vec3(a, b, c);
	#endif
	}

	float VECCALL dot(const Vec3& v) const//dot product
	{
	#ifdef __SSE4_2__
		return _mm_cvtss_f32(_mm_dp_ps(float_dat, v.float_dat, 0b01110001));
	#else
		return x*v.x + y*v.y + z*v.z;
	#endif
	}

	float VECCALL length() const
	{
	#ifdef __SSE4_2__
		return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(float_dat, float_dat, 0b01110001)));
	#else
		return std::sqrt(length_sqr());
	#endif
	}

	float VECCALL length_sqr() const
	{
		return this->dot(*this);
	}

	Vec3 VECCALL normalize() const
	{
	#ifdef __SSE4_2__
		const __m128 ans = _mm_sqrt_ps(_mm_dp_ps(float_dat, float_dat, 0b01110111));
		return _mm_div_ps(float_dat, ans);
	#else
		return (*this) / this->length();
	#endif
	}

	Vec3 VECCALL sqrt() const
	{
	#ifdef __SSE4_2__
		return _mm_sqrt_ps(float_dat);
	#else
		return Vec3(std::sqrt(x), std::sqrt(y), std::sqrt(z));
	#endif
	}
	
	Vec3& VECCALL operator+=(const float right)
	{
	#ifdef __SSE2__
		return *this = _mm_add_ps(float_dat, _mm_set1_ps(right));
	#else
		x += right, y += right, z += right;
		return *this;
	#endif
	}

	Vec3& VECCALL operator+=(const Vec3& right)
	{
	#ifdef __SSE2__
		return *this = _mm_add_ps(float_dat, right);
	#else
		x += right.x, y += right.y, z += right.z;
		return *this;
	#endif
	}
	
	Vec3& VECCALL operator-=(const float right)
	{
	#ifdef __SSE2__
		return *this = _mm_sub_ps(float_dat, _mm_set1_ps(right));
	#else
		x -= right, y -= right, z -= right;
		return *this;
	#endif
	}

	Vec3& VECCALL operator-=(const Vec3& right)
	{
	#ifdef __SSE2__
		return *this = _mm_sub_ps(float_dat, right);
	#else
		x -= right.x, y -= right.y, z -= right.z;
		return *this;
	#endif
	}
	
	Vec3& VECCALL operator*=(const float right)
	{
	#ifdef __SSE2__
		return *this = _mm_mul_ps(float_dat, _mm_set1_ps(right));
	#else
		x *= right, y *= right, z *= right;
		return *this;
	#endif
	}

	Vec3& VECCALL operator*=(const Vec3& right)
	{
	#ifdef __SSE2__
		return *this = _mm_mul_ps(float_dat, right);
	#else
		x *= right.x, y *= right.y, z *= right.z;
		return *this;
	#endif
	}

	Vec3& VECCALL operator/=(const float right)
	{
	#ifdef __SSE2__
		return *this = _mm_div_ps(float_dat, _mm_set1_ps(right));
	#else
		return (*this) *= (1 / right);
	#endif
	}

	Vec3& VECCALL operator/=(const Vec3& right)
	{
	#ifdef __SSE2__
		return *this = _mm_div_ps(float_dat, right.float_dat);
	#else
		x /= right.x, y /= right.y, z /= right.z;
		return *this;
	#endif
	}

	Vec3& VECCALL normalized()
	{
	#ifdef __SSE4_2__
		const __m128 ans = _mm_sqrt_ps(_mm_dp_ps(float_dat, float_dat, 0b01110111));
		return *this = _mm_div_ps(float_dat, ans);
	#else
		return (*this) /= this->length();
	#endif
	}

	Vec3& VECCALL sqrted()
	{
	#ifdef __SSE4_2__
		return *this = _mm_sqrt_ps(float_dat);
	#else
		x = std::sqrt(x), y = std::sqrt(y), z = std::sqrt(z);
		return *this;
	#endif
	}

	static Vec3 VECCALL zero()
	{
	#ifdef __SSE2__
		return _mm_setzero_ps();
	#else
		return Vec4(0, 0, 0, 0);
	#endif
	}

	static Vec3 VECCALL one()
	{
	#ifdef __SSE2__
		return _mm_set1_ps(1);
	#else
		return Vec4(1, 1, 1, 1);
	#endif
	}
};

inline Vec3 VECCALL operator+(const Vec3& left, const float right)
{
#ifdef __SSE2__
	return _mm_add_ps(left, _mm_set1_ps(right));
#else
	return Vec3(left.x + right, left.y + right, left.z + right);
#endif
}

inline Vec3 VECCALL operator+(const Vec3& left, const Vec3& right)
{
#ifdef __SSE2__
	return _mm_add_ps(left, right);
#else
	return Vec3(left.x + right.x, left.y + right.y, left.z + right.z);
#endif
}

inline Vec3 VECCALL operator-(const Vec3& left, const float right)
{
#ifdef __SSE2__
	return _mm_sub_ps(left, _mm_set1_ps(right));
#else
	return Vec3(left.x - right, left.y - right, left.z - right);
#endif
}

inline Vec3 VECCALL operator-(const Vec3& left, const Vec3& right)
{
#ifdef __SSE2__
	return _mm_sub_ps(left, right);
#else
	return Vec3(left.x - right.x, left.y - right.y, left.z - right.z);
#endif
}

inline Vec3 VECCALL operator*(const Vec3& left, const float right)
{
#ifdef __SSE2__
	return _mm_mul_ps(left, _mm_set1_ps(right));
#else
	return Vec3(left.x * right, left.y * right, left.z * right);
#endif
}

inline Vec3 VECCALL operator*(const Vec3& left, const Vec3& right)
{
#ifdef __SSE2__
	return _mm_mul_ps(left, right);
#else
	return Vec3(left.x * right.x, left.y * right.y, left.z * right.z);
#endif
}

inline Vec3 VECCALL operator*(const float left, const Vec3& right)
{
	return right*left;
}

inline Vec3 VECCALL operator/(const Vec3& left, const float right)
{
#ifdef __SSE2__
	return _mm_div_ps(left, _mm_set1_ps(right));
#else
	return left*(1 / right);
#endif
}

inline Vec3 VECCALL operator/(const Vec3& left, const Vec3& right)
{
#ifdef __SSE2__
	return _mm_div_ps(left, right);
#else
	return Vec3(left.x / right.x, left.y / right.y, left.z / right.z);
#endif
}

inline Vec3 VECCALL max(const Vec3& left, const Vec3& right)
{
#ifdef __SSE2__
	return _mm_max_ps(left, right);
#else
	return Vec3(max(left.x, right.x), max(left.y, right.y), max(left.z, right.z));
#endif
}

inline Vec3 VECCALL min(const Vec3& left, const Vec3& right)
{
#ifdef __SSE2__
	return _mm_min_ps(left, right);
#else
	return Vec3(min(left.x, right.x), min(left.y, right.y), min(left.z, right.z));
#endif
}

}