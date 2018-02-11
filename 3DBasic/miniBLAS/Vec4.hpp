#pragma once

#include "BLASrely.h"
#include "VecBase.hpp"
#include "Vec3.hpp"

namespace miniBLAS
{

/*vector contains 4 float*/
class alignas(Vec4Align) Vec4 :public Vec4Base<float>
{
private:
protected:
public:
    using Vec4Base::x; using Vec4Base::y; using Vec4Base::z; using Vec4Base::w;
    explicit Vec4(const bool setZero = false) noexcept
    {
        if (setZero)
        {
        #ifdef __SSE2__
            Store128PS(data, _mm_setzero_ps());
        #else
            x = y = z = w = 0;
        #endif
        }
    }
    template<class T>
    Vec4(const T x_, const T y_, const T z_, const T w_) noexcept
        :Vec4Base(static_cast<float>(x_), static_cast<float>(y_), static_cast<float>(z_), static_cast<float>(w_))
    {
    };
    template<class T>
    explicit Vec4(const T *ptr) noexcept
        :Vec4Base(static_cast<float>(ptr[0]), static_cast<float>(ptr[1]), static_cast<float>(ptr[2]), static_cast<float>(ptr[3])) 
    {
    };
#ifdef __SSE2__
    explicit Vec4(const float *ptr) noexcept { Store128PS(data, _mm_loadu_ps(ptr)); }
    Vec4(const __m128& dat_) noexcept { Store128PS(data, dat_); };
#endif
    explicit Vec4(const Vec3& v, const float w_) noexcept
    {
        x = v.x, y = v.y, z = v.z, w = w_;
    }
    explicit Vec4(const Vec3& v, const bool isHomogeneous = true) noexcept
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

#ifdef __SSE2__
    VECCALL operator __m128&() noexcept { return float_dat; };
    VECCALL operator const __m128&() const noexcept { return Load128PS(data); };
#endif

    bool operator<(const Vec4& other) const = delete;

    forceinline float VECCALL dot(const Vec4& v) const//dot product
    {
    #ifdef __SSE4_2__
        return _mm_cvtss_f32(_mm_dp_ps(float_dat, v.float_dat, 0b11110001));
    #else
        return x*v.x + y*v.y + z*v.z + w*v.w;
    #endif
    }

    forceinline float VECCALL length() const
    {
    #ifdef __SSE4_2__
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(float_dat, float_dat, 0b11110001)));
    #else
        return std::sqrt(length_sqr());
    #endif
    }

    forceinline float VECCALL length_sqr() const
    {
    #ifdef __SSE4_2__
        return _mm_cvtss_f32(_mm_dp_ps(float_dat, float_dat, 0b11110001));
    #else
        return this->dot(*this);
    #endif
    }

    forceinline Vec4 VECCALL operator+(const float right) const
    {
    #ifdef __SSE2__
        return _mm_add_ps(float_dat, _mm_set1_ps(right));
    #else
        return Vec4(x + right, y + right, z + right, w + right);
    #endif
    }

    forceinline Vec4 VECCALL operator+(const Vec4& right) const
    {
    #ifdef __SSE2__
        return _mm_add_ps(float_dat, right.float_dat);
    #else
        return Vec4(x + right.x, y + right.y, z + right.z, w + right.w);
    #endif
    }

    forceinline Vec4 VECCALL operator-(const float right) const
    {
    #ifdef __SSE2__
        return _mm_sub_ps(float_dat, _mm_set1_ps(right));
    #else
        return Vec4(x - right, y - right, z - right, w - right);
    #endif
    }

    forceinline Vec4 VECCALL operator-(const Vec4& right) const
    {
    #ifdef __SSE2__
        return _mm_sub_ps(float_dat, right.float_dat);
    #else
        return Vec4(x - right.x, y - right.y, z - right.z, w - right.w);
    #endif
    }

    forceinline Vec4 VECCALL operator*(const float right) const
    {
    #ifdef __SSE2__
        return _mm_mul_ps(float_dat, _mm_set1_ps(right));
    #else
        return Vec4(x * right, y * right, z * right, w * right);
    #endif
    }

    forceinline Vec4 VECCALL operator*(const Vec4& right) const
    {
    #ifdef __SSE2__
        return _mm_mul_ps(float_dat, right.float_dat);
    #else
        return Vec4(x * right.x, y * right.y, z * right.z, w * right.w);
    #endif
    }

    forceinline Vec4 VECCALL operator/(const float right) const
    {
    #ifdef __SSE2__
        return _mm_mul_ps(_mm_rcp_ps(_mm_set1_ps(right)), float_dat);
    #else
        return (*this) * (1 / right);
    #endif
    }

    forceinline Vec4 VECCALL operator/(const Vec4& right) const
    {
    #ifdef __SSE2__
        return _mm_mul_ps(_mm_rcp_ps(right.float_dat), float_dat);
    #else
        return Vec4(x / right.x, y / right.y, z / right.z, w / right.w);
    #endif
    }

    forceinline Vec4 VECCALL normalize() const
    {
    #ifdef __SSE4_2__
        const auto len_1 = _mm_rsqrt_ps(_mm_dp_ps(float_dat, float_dat, 0b11111111));
        return _mm_mul_ps(float_dat, len_1);
        //const auto len = _mm_sqrt_ps(_mm_dp_ps(selfVec, selfVec, 0b11111111));
        //return _mm_div_ps(float_dat, len);
    #else
        return (*this) / this->length();
    #endif
    }

    forceinline Vec4 VECCALL rcp() const
    {
    #ifdef __SSE2__
        return _mm_rcp_ps(float_dat);
    #else
        return Vec4(1 / x, 1 / y, 1 / z, 1 / w);
    #endif
    }

    forceinline Vec4 VECCALL sqrt() const
    {
    #ifdef __SSE4_2__
        return _mm_sqrt_ps(float_dat);
    #else
        return Vec4(std::sqrt(x), std::sqrt(y), std::sqrt(z), std::sqrt(w));
    #endif
    }

    forceinline Vec4 VECCALL rsqrt() const
    {
    #ifdef __SSE4_2__
        return _mm_rsqrt_ps(float_dat);
    #else
        return Vec4(1 / std::sqrt(x), 1 / std::sqrt(y), 1 / std::sqrt(z), 1 / std::sqrt(w));
    #endif
    }

    forceinline Vec4& VECCALL operator+=(const float right)
    {
    #ifdef __SSE2__
        Store128PS(data, _mm_add_ps(float_dat, _mm_set1_ps(right)));
    #else
        x += right, y += right, z += right, w += right;
    #endif
        return *this;
    }

    forceinline Vec4& VECCALL operator+=(const Vec4& right)
    {
    #ifdef __SSE2__
        Store128PS(data, _mm_add_ps(float_dat, right.float_dat));
    #else
        x += right.x, y += right.y, z += right.z, w += right.w;
    #endif
        return *this;
    }

    forceinline Vec4& VECCALL operator-=(const float right)
    {
    #ifdef __SSE2__
        Store128PS(data, _mm_sub_ps(float_dat, _mm_set1_ps(right)));
    #else
        x -= right, y -= right, z -= right, w -= right;
    #endif
        return *this;
    }

    forceinline Vec4& VECCALL operator-=(const Vec4& right)
    {
    #ifdef __SSE2__
        Store128PS(data, _mm_sub_ps(float_dat, right.float_dat));
    #else
        x -= right.x, y -= right.y, z -= right.z, w -= right.w;
    #endif
        return *this;
    }

    forceinline Vec4& VECCALL operator*=(const float right)
    {
    #ifdef __SSE2__
        Store128PS(data, _mm_mul_ps(float_dat, _mm_set1_ps(right)));
    #else
        x *= right, y *= right, z *= right, w *= right;
    #endif
        return *this;
    }

    forceinline Vec4& VECCALL operator*=(const Vec4& right)
    {
    #ifdef __SSE2__
        Store128PS(data, _mm_mul_ps(float_dat, right.float_dat));
    #else
        x *= right.x, y *= right.y, z *= right.z, w *= right.w;
    #endif
        return *this;
    }

    forceinline Vec4& VECCALL operator/=(const float right)
    {
    #ifdef __SSE2__
        Store128PS(data, _mm_mul_ps(_mm_rcp_ps(_mm_set1_ps(right)), float_dat));
        return *this;
    #else
        return (*this) *= (1 / right);
    #endif
    }

    forceinline Vec4& VECCALL operator/=(const Vec4& right)
    {
    #ifdef __SSE2__
        Store128PS(data, _mm_mul_ps(_mm_rcp_ps(right.float_dat), float_dat));
        return *this;
    #else
        x /= right.x, y /= right.y, z /= right.z, w /= right.w;
        return *this;
    #endif
    }

    forceinline Vec4& VECCALL normalized()
    {
    #ifdef __SSE4_2__
        const auto len_1 = _mm_rsqrt_ps(_mm_dp_ps(float_dat, float_dat, 0b01110111));
        Store128PS(data, _mm_mul_ps(float_dat, len_1));
    #else
        *this /= this->length();
    #endif
        return *this;
    }

    forceinline Vec4& VECCALL rcped()
    {
    #ifdef __SSE2__
        Store128PS(data, _mm_rcp_ps(float_dat));
    #else
        x = 1 / x, y = 1 / y, z = 1 / z;
    #endif
        return *this;
    }

    forceinline Vec4& VECCALL sqrted()
    {
    #ifdef __SSE2__
        Store128PS(data, _mm_sqrt_ps(float_dat));
    #else
        x = std::sqrt(x), y = std::sqrt(y), z = std::sqrt(z), w = std::sqrt(w);
    #endif
        return *this;
    }

    forceinline Vec4& VECCALL rsqrted()
    {
    #ifdef __SSE2__
        Store128PS(data, _mm_rsqrt_ps(float_dat));
    #else
        x = 1 / std::sqrt(x), y = 1 / std::sqrt(y), z = 1 / std::sqrt(z), w = 1 / std::sqrt(w);
    #endif
        return *this;
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


forceinline Vec4 VECCALL operator+(const float left, const Vec4& right)
{
    return right + left;
}

forceinline Vec4 VECCALL operator-(const float left, const Vec4& right)
{
#ifdef __SSE2__
    return Vec4(_mm_set1_ps(left)) - right;
#else
    return Vec4(left, left, left, left) - right;
#endif
}

forceinline Vec4 VECCALL operator*(const float left, const Vec4& right)
{
    return right * left;
}

forceinline Vec4 VECCALL operator/(const float left, const Vec4& right)
{
#ifdef __SSE2__
    return Vec4(_mm_set1_ps(left)) / right;
#else
    return Vec4(left, left, left, left) / right;
#endif
}

forceinline Vec4 VECCALL max(const Vec4& left, const Vec4& right)
{
#ifdef __SSE2__
    return _mm_max_ps(left, right);
#else
    return Vec4(common::max(left.x, right.x), common::max(left.y, right.y), common::max(left.z, right.z), common::max(left.w, right.w));
#endif
}

forceinline Vec4 VECCALL min(const Vec4& left, const Vec4& right)
{
#ifdef __SSE2__
    return _mm_min_ps(left, right);
#else
    return Vec4(common::min(left.x, right.x), common::min(left.y, right.y), common::min(left.z, right.z), common::min(left.w, right.w));
#endif
}


}