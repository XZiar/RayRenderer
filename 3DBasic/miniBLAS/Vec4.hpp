#pragma once

#include "BLASrely.h"
#include "VecBase.hpp"
#include "Vec3.hpp"

namespace miniBLAS
{

Vec4 VECCALL operator/(const Vec4& left, const float right);

/*vector contains 4 float*/
class alignas(16) Vec4 :public Vec4Base<float>
{
private:
protected:
public:
	using Vec4Base::x; using Vec4Base::y; using Vec4Base::z; using Vec4Base::w;

	explicit Vec4(const bool setZero = false)
	{
		if (setZero)
		{
		#ifdef __SSE2__
			float_dat = _mm_setzero_ps();
		#else
			x = y = z = w = 0;
		#endif
		}
	}
	template<class T>
	Vec4(const T x_, const T y_, const T z_, const T w_)
		:Vec4Base(static_cast<float>(x_), static_cast<float>(y_), static_cast<float>(z_), static_cast<float>(w_))
	{
	};
	template<class T>
	explicit Vec4(const T *ptr)
		:Vec4Base(static_cast<float>(ptr[0]), static_cast<float>(ptr[1]), static_cast<float>(ptr[2]), static_cast<float>(ptr[3])) 
	{
	};
#ifdef __SSE2__
	explicit Vec4(const float *ptr) { float_dat = _mm_loadu_ps(ptr); }
	Vec4(const __m128& dat_) { float_dat = dat_; };
#endif
	explicit Vec4(const Vec3& v, const float w_)
	{
		x = v.x, y = v.y, z = v.z, w = w_;
	}
	explicit Vec4(const Vec3& v, const bool isHomogeneous = true)
	{
		if (isHomogeneous)
		{
			*this = Vec4(v, 1.0f);
		}
		else
		{
			*(Vec4Base*)this = (Vec4Base)v;
		}
	}

	VECCALL operator float*() { return data; };
	VECCALL operator const float*() const { return data; };
#ifdef __SSE2__
	VECCALL operator __m128&() { return float_dat; };
	VECCALL operator const __m128&() const { return float_dat; };
#endif

	float VECCALL dot(const Vec4& v) const//dot product
	{
	#ifdef __SSE4_2__
		return _mm_cvtss_f32(_mm_dp_ps(float_dat, v.float_dat, 0b11110001));
	#else
		return x*v.x + y*v.y + z*v.z + w*v.w;
	#endif
	}

	float VECCALL length() const
	{
	#ifdef __SSE4_2__
		return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(float_dat, float_dat, 0b11110001)));
	#else
		return std::sqrt(length_sqr());
	#endif
	}

	float VECCALL length_sqr() const
	{
		return this->dot(*this);
	}

	Vec4 VECCALL normalize() const
	{
	#ifdef __SSE4_2__
		const __m128 ans = _mm_sqrt_ps(_mm_dp_ps(float_dat, float_dat, 0b11111111));
		return _mm_div_ps(float_dat, ans);
	#else
		return (*this) / this->length();
	#endif
	}

	Vec4 VECCALL sqrt() const
	{
	#ifdef __SSE4_2__
		return _mm_sqrt_ps(float_dat);
	#else
		return Vec4(std::sqrt(x), std::sqrt(y), std::sqrt(z), std::sqrt(w));
	#endif
	}

	Vec4& VECCALL operator+=(const float right)
	{
	#ifdef __SSE2__
		return *this = _mm_add_ps(float_dat, _mm_set1_ps(right));
	#else
		x += right, y += right, z += right, w += right;
		return *this;
	#endif
	}

	Vec4& VECCALL operator+=(const Vec4& right)
	{
	#ifdef __SSE2__
		return *this = _mm_add_ps(float_dat, right);
	#else
		x += right.x, y += right.y, z += right.z, w += right.w;
		return *this;
	#endif
	}

	Vec4& VECCALL operator-=(const float right)
	{
	#ifdef __SSE2__
		return *this = _mm_sub_ps(float_dat, _mm_set1_ps(right));
	#else
		x -= right, y -= right, z -= right, w -= right;
		return *this;
	#endif
	}

	Vec4& VECCALL operator-=(const Vec4& right)
	{
	#ifdef __SSE2__
		return *this = _mm_sub_ps(float_dat, right);
	#else
		x -= right.x, y -= right.y, z -= right.z, w -= right.w;
		return *this;
	#endif
	}

	Vec4& VECCALL operator*=(const float right)
	{
	#ifdef __SSE2__
		return *this = _mm_mul_ps(float_dat, _mm_set1_ps(right));
	#else
		x *= right, y *= right, z *= right, w *= right;
		return *this;
	#endif
	}

	Vec4& VECCALL operator*=(const Vec4& right)
	{
	#ifdef __SSE2__
		return *this = _mm_mul_ps(float_dat, right.float_dat);
	#else
		x *= right.x, y *= right.y, z *= right.z, w *= right.w;
		return *this;
	#endif
	}

	Vec4& VECCALL operator/=(const float right)
	{
	#ifdef __SSE2__
		return *this = _mm_div_ps(float_dat, _mm_set1_ps(right));
	#else
		return (*this) *= (1 / right);
	#endif
	}

	Vec4& VECCALL operator/=(const Vec4& right)
	{
	#ifdef __SSE2__
		return *this = _mm_div_ps(float_dat, right.float_dat);
	#else
		x /= right.x, y /= right.y, z /= right.z, w /= right.w;
		return *this;
	#endif
	}

	Vec4& VECCALL normalized()
	{
	#ifdef __SSE4_2__
		const __m128 ans = _mm_sqrt_ps(_mm_dp_ps(float_dat, float_dat, 0b11111111));
		return *this = _mm_div_ps(float_dat, ans);
	#else
		return (*this) /= this->length();
	#endif
	}

	Vec4& VECCALL sqrted()
	{
	#ifdef __SSE4_2__
		return *this = _mm_sqrt_ps(float_dat);
	#else
		x = std::sqrt(x), y = std::sqrt(y), z = std::sqrt(z), w = std::sqrt(w);
		return *this;
	#endif
	}

	static Vec4 VECCALL zero()
	{
	#ifdef __SSE2__
		return _mm_setzero_ps();
	#else
		return Vec4(0, 0, 0, 0);
	#endif
	}

	static Vec4 VECCALL one()
	{
	#ifdef __SSE2__
		return _mm_set1_ps(1);
	#else
		return Vec4(1, 1, 1, 1);
	#endif
	}
};

inline Vec4 VECCALL operator+(const Vec4& left, const float right)
{
#ifdef __SSE2__
	return _mm_add_ps(left, _mm_set1_ps(right));
#else
	return Vec4(left.x + right, left.y + right, left.z + right, left.w + right);
#endif
}

inline Vec4 VECCALL operator+(const Vec4& left, const Vec4& right)
{
#ifdef __SSE2__
	return _mm_add_ps(left, right);
#else
	return Vec4(left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w);
#endif
}

inline Vec4 VECCALL operator-(const Vec4& left, const float right)
{
#ifdef __SSE2__
	return _mm_sub_ps(left, _mm_set1_ps(right));
#else
	return Vec4(left.x - right, left.y - right, left.z - right, left.w - right);
#endif
}

inline Vec4 VECCALL operator-(const Vec4& left, const Vec4& right)
{
#ifdef __SSE2__
	return _mm_sub_ps(left, right);
#else
	return Vec4(left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w);
#endif
}

inline Vec4 VECCALL operator*(const Vec4& left, const float right)
{
#ifdef __SSE2__
	return _mm_mul_ps(left, _mm_set1_ps(right));
#else
	return Vec4(left.x * right, left.y * right, left.z * right, left.w * right);
#endif
}

inline Vec4 VECCALL operator*(const Vec4& left, const Vec4& right)
{
#ifdef __SSE2__
	return _mm_mul_ps(left, right);
#else
	return Vec4(left.x * right.x, left.y * right.y, left.z * right.z, left.w * right.w);
#endif
}

inline Vec4 VECCALL operator*(const float left, const Vec4& right)
{
	return right*left;
}

inline Vec4 VECCALL operator/(const Vec4& left, const float right)
{
#ifdef __SSE2__
	return _mm_div_ps(left, _mm_set1_ps(right));
#else
	return left*(1 / right);
#endif
}

inline Vec4 VECCALL operator/(const Vec4& left, const Vec4& right)
{
#ifdef __SSE2__
	return _mm_div_ps(left, right);
#else
	return Vec4(left.x / right.x, left.y / right.y, left.z / right.z, left.w / right.w);
#endif
}


}