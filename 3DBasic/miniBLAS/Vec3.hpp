#pragma once

#include "BLASrely.h"
#include "VecBase.hpp"

namespace miniBLAS
{

/*vector contains 4 float, while only xyz are considered in some calculation*/
class alignas(Vec4Align) Vec3 :public Vec4Base<float>
{
private:
protected:
public:
    using Vec4Base::x; using Vec4Base::y; using Vec4Base::z;

    explicit Vec3(const bool setZero = false) noexcept
    {
        if (setZero)
        {
        #ifdef __SSE2__
            _mm_store_ps(data, _mm_setzero_ps());
        #else
            x = y = z = 0;
        #endif
        }
    }
    template<class T>
    Vec3(const T x_, const T y_, const T z_, const T w_ = static_cast<T>(0)) noexcept
        :Vec4Base(static_cast<float>(x_), static_cast<float>(y_), static_cast<float>(z_), static_cast<float>(w_)) { };
    template<class T>
    explicit Vec3(const T *ptr) noexcept
        :Vec4Base(static_cast<float>(ptr[0]), static_cast<float>(ptr[1]), static_cast<float>(ptr[2])) { };
#ifdef __SSE2__
    explicit Vec3(const float *ptr) noexcept { _mm_store_ps(data, _mm_loadu_ps(ptr)); }
    Vec3(const __m128& dat_) noexcept { _mm_store_ps(data, dat_); };
    VECCALL operator __m128&() noexcept { return float_dat; };
    VECCALL operator __m128() const noexcept { return _mm_load_ps(data); };
#endif

    bool operator<(const Vec3& other) const = delete;

    forceinline Vec3 VECCALL cross(const Vec3& v) const
    {
    #ifdef __SSE2__
        const auto t1 = Permute128PS(float_dat, _MM_SHUFFLE(3, 0, 2, 1))/*y,z,x,w*/;
        const auto t2 = Permute128PS(v.float_dat, _MM_SHUFFLE(3, 1, 0, 2))/*v.z,v.x,v.y,v.w*/;
        const auto t3 = Permute128PS(float_dat, _MM_SHUFFLE(3, 1, 0, 2))/*z,x,y,w*/;
        const auto t4 = Permute128PS(v.float_dat, _MM_SHUFFLE(3, 0, 2, 1))/*v.y,v.z,v.x,v.w*/;
        return _mm_sub_ps(_mm_mul_ps(t1, t2), _mm_mul_ps(t3, t4));
    #else
        const float a = y*v.z - z*v.y;
        const float b = z*v.x - x*v.z;
        const float c = x*v.y - y*v.x;
        return Vec3(a, b, c);
    #endif
    }

    forceinline float VECCALL dot(const Vec3& v) const//dot product
    {
    #ifdef __SSE4_2__
        //const auto selfVec = Load128PS(data), rhsVec = Load128PS(v.data);
        return _mm_cvtss_f32(_mm_dp_ps(float_dat, v, 0b01110001));
    #else
        return x*v.x + y*v.y + z*v.z;
    #endif
    }

    forceinline float VECCALL length() const
    {
    #ifdef __SSE4_2__
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(float_dat, float_dat, 0b01110001)));
    #else
        return std::sqrt(length_sqr());
    #endif
    }

    forceinline float VECCALL length_sqr() const
    {
    #ifdef __SSE4_2__
        return _mm_cvtss_f32(_mm_dp_ps(float_dat, float_dat, 0b01110001));
    #else
        return this->dot(*this);
    #endif
    }

    forceinline Vec3 VECCALL operator+(const float right) const
    {
    #ifdef __SSE2__
        return _mm_add_ps(float_dat, _mm_set1_ps(right));
    #else
        return Vec3(x + right, y + right, z + right);
    #endif
    }

    forceinline Vec3 VECCALL operator+(const Vec3& right) const
    {
    #ifdef __SSE2__
        return _mm_add_ps(float_dat, right.float_dat);
    #else
        return Vec3(x + right.x, y + right.y, z + right.z);
    #endif
    }

    forceinline Vec3 VECCALL operator-(const float right) const
    {
    #ifdef __SSE2__
        return _mm_sub_ps(float_dat, _mm_set1_ps(right));
    #else
        return Vec3(x - right, y - right, z - right);
    #endif
    }

    forceinline Vec3 VECCALL operator-(const Vec3& right) const
    {
    #ifdef __SSE2__
        return _mm_sub_ps(float_dat, right.float_dat);
    #else
        return Vec3(x - right.x, y - right.y, z - right.z);
    #endif
    }

    forceinline Vec3 VECCALL operator*(const float right) const
    {
    #ifdef __SSE2__
        return _mm_mul_ps(float_dat, _mm_set1_ps(right));
    #else
        return Vec3(x * right, y * right, z * right);
    #endif
    }

    forceinline Vec3 VECCALL operator*(const Vec3& right) const
    {
    #ifdef __SSE2__
        return _mm_mul_ps(float_dat, right.float_dat);
    #else
        return Vec3(x * right.x, y * right.y, z * right.z);
    #endif
    }

    forceinline Vec3 VECCALL operator/(const float right) const
    {
    #ifdef __SSE2__
        return _mm_mul_ps(_mm_rcp_ps(_mm_set1_ps(right)), float_dat);
    #else
        return (*this) * (1 / right);
    #endif
    }

    forceinline Vec3 VECCALL operator/(const Vec3& right) const
    {
    #ifdef __SSE2__
        return _mm_mul_ps(_mm_rcp_ps(right.float_dat), float_dat);
    #else
        return Vec3(x / right.x, y / right.y, z / right.z);
    #endif
    }

    forceinline Vec3 VECCALL normalize() const
    {
    #ifdef __SSE4_2__
        const auto len_1 = _mm_rsqrt_ps(_mm_dp_ps(float_dat, float_dat, 0b01110111));
        return _mm_mul_ps(float_dat, len_1);
        //const auto len = _mm_sqrt_ps(_mm_dp_ps(selfVec, selfVec, 0b01110111));
        //return _mm_div_ps(float_dat, len);
    #else
        return (*this) / this->length();
    #endif
    }

    forceinline Vec3 VECCALL rcp() const
    {
    #ifdef __SSE2__
        return _mm_rcp_ps(float_dat);
    #else
        return Vec3(1 / x, 1 / y, 1 / z);
    #endif
    }

    forceinline Vec3 VECCALL sqrt() const
    {
    #ifdef __SSE2__
        return _mm_sqrt_ps(float_dat);
    #else
        return Vec3(std::sqrt(x), std::sqrt(y), std::sqrt(z));
    #endif
    }

    forceinline Vec3 VECCALL rsqrt() const
    {
    #ifdef __SSE2__
        return _mm_rsqrt_ps(float_dat);
    #else
        return Vec3(1 / std::sqrt(x), 1 / std::sqrt(y), 1 / std::sqrt(z));
    #endif
    }
    
    forceinline Vec3& VECCALL operator+=(const float right)
    {
    #ifdef __SSE2__
        _mm_store_ps(data, _mm_add_ps(float_dat, _mm_set1_ps(right)));
    #else
        x += right, y += right, z += right;
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL operator+=(const Vec3& right)
    {
    #ifdef __SSE2__
        _mm_store_ps(data, _mm_add_ps(float_dat, right));
    #else
        x += right.x, y += right.y, z += right.z;
    #endif
        return *this;
    }
    
    forceinline Vec3& VECCALL operator-=(const float right)
    {
    #ifdef __SSE2__
        _mm_store_ps(data, _mm_sub_ps(float_dat, _mm_set1_ps(right)));
    #else
        x -= right, y -= right, z -= right;
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL operator-=(const Vec3& right)
    {
    #ifdef __SSE2__
        _mm_store_ps(data, _mm_sub_ps(float_dat, right.float_dat));
    #else
        x -= right.x, y -= right.y, z -= right.z;
    #endif
        return *this;
    }
    
    forceinline Vec3& VECCALL operator*=(const float right)
    {
    #ifdef __SSE2__
        _mm_store_ps(data, _mm_mul_ps(float_dat, _mm_set1_ps(right)));
    #else
        x *= right, y *= right, z *= right;
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL operator*=(const Vec3& right)
    {
    #ifdef __SSE2__
        _mm_store_ps(data, _mm_mul_ps(float_dat, right.float_dat));
    #else
        x *= right.x, y *= right.y, z *= right.z;
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL operator/=(const float right)
    {
    #ifdef __SSE2__
        _mm_store_ps(data, _mm_mul_ps(_mm_rcp_ps(_mm_set1_ps(right)), float_dat));
        return *this;
    #else
        return (*this) *= (1 / right);
    #endif
    }

    forceinline Vec3& VECCALL operator/=(const Vec3& right)
    {
    #ifdef __SSE2__
        _mm_store_ps(data, _mm_mul_ps(_mm_rcp_ps(right.float_dat), float_dat));
    #else
        x /= right.x, y /= right.y, z /= right.z;
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL normalized()
    {
    #ifdef __SSE4_2__
        const auto len_1 = _mm_rsqrt_ps(_mm_dp_ps(float_dat, float_dat, 0b01110111));
        _mm_store_ps(data, _mm_mul_ps(float_dat, len_1));
    #else
        *this /= this->length();
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL rcped()
    {
    #ifdef __SSE2__
        _mm_store_ps(data, _mm_rcp_ps(float_dat));
    #else
        x = 1 / x, y = 1 / y, z = 1 / z;
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL sqrted()
    {
    #ifdef __SSE2__
        _mm_store_ps(data, _mm_sqrt_ps(float_dat));
    #else
        x = std::sqrt(x), y = std::sqrt(y), z = std::sqrt(z);
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL rsqrted()
    {
    #ifdef __SSE2__
        _mm_store_ps(data, _mm_rsqrt_ps(float_dat));
    #else
        x = 1 / std::sqrt(x), y = 1 / std::sqrt(y), z = 1 / std::sqrt(z);
    #endif
        return *this;
    }

    static Vec3 VECCALL zero()
    {
    #ifdef __SSE2__
        return _mm_setzero_ps();
    #else
        return Vec3(0, 0, 0, 0);
    #endif
    }

    static Vec3 VECCALL one()
    {
    #ifdef __SSE2__
        return _mm_set1_ps(1);
    #else
        return Vec3(1, 1, 1, 1);
    #endif
    }
};

forceinline Vec3 VECCALL operator+(const float left, const Vec3& right)
{
    return right + left;
}

forceinline Vec3 VECCALL operator-(const float left, const Vec3& right)
{
#ifdef __SSE2__
    return Vec3(_mm_set1_ps(left)) - right;
#else
    return Vec3(left, left, left) - right;
#endif
}

forceinline Vec3 VECCALL operator*(const float left, const Vec3& right)
{
    return right * left;
}

forceinline Vec3 VECCALL operator/(const float left, const Vec3& right)
{
#ifdef __SSE2__
    return Vec3(_mm_set1_ps(left)) / right;
#else
    return Vec3(left, left, left) / right;
#endif
}

forceinline Vec3 VECCALL max(const Vec3& left, const Vec3& right)
{
#ifdef __SSE2__
    return _mm_max_ps(left, right);
#else
    return Vec3(common::max(left.x, right.x), common::max(left.y, right.y), common::max(left.z, right.z));
#endif
}

forceinline Vec3 VECCALL min(const Vec3& left, const Vec3& right)
{
#ifdef __SSE2__
    return _mm_min_ps(left, right);
#else
    return Vec3(common::min(left.x, right.x), common::min(left.y, right.y), common::min(left.z, right.z));
#endif
}

}