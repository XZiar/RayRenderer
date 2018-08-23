#pragma once

#include "BLASrely.h"

#if COMMON_SIMD_LV >= 20
#   include "common/SIMD128.hpp"
#endif

namespace miniBLAS
{
using namespace common::simd;
#if defined(COMPILER_GCC) && COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(COMPILER_CLANG) && COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wnested-anon-types"
#   pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#elif defined(COMPILER_MSVC) && COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4201)
#endif

/*a vector contains 4 element(int32 or float)*/
template<typename T>
class alignas(16) Vec4Base : public common::AlignBase<16>
{
private:
    static_assert(sizeof(T) == 4, "only 4-byte length type allowed");
protected:
    constexpr Vec4Base(const T x_, const T y_, const T z_, const T w_) noexcept :x(x_), y(y_), z(z_), w(w_) { };
public:
    union
    {
        T Data[4];
#if COMMON_SIMD_LV >= 20
        F32x4 FloatData;
        I32x4 IntData;
#endif
        struct
        {
            T x, y, z, w;
        };
        struct
        {
            float float_x, float_y, float_z, float_w;
        };
        struct
        {
            int32_t int_x, int_y, int_z, int_w;
        };
    };
#if COMMON_SIMD_LV >= 20
    constexpr Vec4Base(const F32x4& data) noexcept : FloatData(data) {}
    constexpr Vec4Base(const I32x4& data) noexcept : IntData(data) {}
    constexpr Vec4Base(const __m128&  dat) noexcept : FloatData(dat) { };
    constexpr Vec4Base(const __m128i& dat) noexcept : IntData(dat) { };
    constexpr operator __m128&() noexcept { return FloatData.Data; };
    constexpr operator const __m128&() const noexcept { return FloatData.Data; };
    constexpr operator __m128i&() noexcept { return IntData.Data; };
    constexpr operator const __m128i&() const noexcept { return IntData.Data; };
#endif
    constexpr Vec4Base() noexcept : Data() { }
    constexpr forceinline operator T*() noexcept { return Data; }
    constexpr forceinline operator const T*() const noexcept { return Data; }
    bool operator<(const Vec4Base& other) const = delete;
};

#if defined(COMPILER_GCC) && COMPILER_GCC
#   pragma GCC diagnostic pop
#elif defined(COMPILER_CLANG) && COMPILER_CLANG
#   pragma clang diagnostic pop
#elif defined(COMPILER_MSVC) && COMPILER_MSVC
#   pragma warning(pop)
#endif

class Vec3;
class Vec4;


class alignas(16) Vec3 :public Vec4Base<float>
{
private:
    using Vec4Base<float>::w;
public:
    using Vec4Base<float>::Vec4Base;

    constexpr Vec3() noexcept : Vec4Base() { }
    template<typename T>
    constexpr Vec3(const T x_, const T y_, const T z_) noexcept :Vec4Base(static_cast<float>(x_), static_cast<float>(y_), static_cast<float>(z_), 0.f)
    { }
#if COMMON_SIMD_LV >= 20
    template<typename T> Vec3(const T all) noexcept : Vec4Base(F32x4(all)) { }
#else
    template<typename T> Vec3(const T all) noexcept : Vec3(all, all, all) { }
#endif
    forceinline void VECCALL Save(float* ptr) const noexcept
    {
        ptr[0] = x, ptr[1] = y, ptr[2] = z;
    }

    forceinline Vec3 VECCALL cross(const Vec3& v) const noexcept
    {
    #if COMMON_SIMD_LV >= 20
        const auto t3 =   FloatData.Shuffle<2, 0, 1, 3>();//zxyw
        const auto t4 = v.FloatData.Shuffle<1, 2, 0, 3>();//yzxw
        const auto t34 = t3 * t4;
        const auto t1 =   FloatData.Shuffle<1, 2, 0, 3>();//yzxw
        const auto t2 = v.FloatData.Shuffle<2, 0, 1, 3>();//zxyw
        return t1.MulSub(t2, t34); // (t1 * t2) - (t3 * t4);
    #else
        const float a = y*v.z - z*v.y;
        const float b = z*v.x - x*v.z;
        const float c = x*v.y - y*v.x;
        return Vec3(a, b, c);
    #endif
    }

    forceinline float VECCALL dot(const Vec3& v) const noexcept//dot product
    {
    #if COMMON_SIMD_LV >= 42
        return _mm_cvtss_f32(FloatData.Dot<VecPos::XYZ, VecPos::X>(v.FloatData));
    #else
        return x*v.x + y*v.y + z*v.z;
    #endif
    }

    forceinline float VECCALL length() const noexcept
    {
    #if COMMON_SIMD_LV >= 42
        return _mm_cvtss_f32(FloatData.Dot<VecPos::XYZ, VecPos::X>(FloatData).Sqrt());
    #else
        return std::sqrt(length_sqr());
    #endif
    }

    forceinline float VECCALL length_sqr() const noexcept
    {
        return this->dot(*this);
    }

    forceinline Vec3 VECCALL operator+(const float right) const noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return FloatData + right;
    #else
        return Vec3(x + right, y + right, z + right);
    #endif
    }

    forceinline Vec3 VECCALL operator+(const Vec3& right) const noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return FloatData + right.FloatData;
    #else
        return Vec3(x + right.x, y + right.y, z + right.z);
    #endif
    }

    forceinline Vec3 VECCALL operator-() const noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return FloatData ^ F32x4(-0.0f);
    #else
        return Vec3(-x, -y, -z);
    #endif
    }

    forceinline Vec3 VECCALL operator-(const float right) const noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return FloatData - right;
    #else
        return Vec3(x - right, y - right, z - right);
    #endif
    }

    forceinline Vec3 VECCALL operator-(const Vec3& right) const noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return FloatData - right.FloatData;
    #else
        return Vec3(x - right.x, y - right.y, z - right.z);
    #endif
    }

    forceinline Vec3 VECCALL operator*(const float right) const noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return FloatData * right;
    #else
        return Vec3(x * right, y * right, z * right);
    #endif
    }

    forceinline Vec3 VECCALL operator*(const Vec3& right) const noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return FloatData * right.FloatData;
    #else
        return Vec3(x * right.x, y * right.y, z * right.z);
    #endif
    }

    forceinline Vec3 VECCALL operator/(const float right) const noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return FloatData * F32x4(right).Rcp();
    #else
        return (*this) * (1 / right);
    #endif
    }

    forceinline Vec3 VECCALL operator/(const Vec3& right) const noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return FloatData * right.FloatData.Rcp();
    #else
        return Vec3(x / right.x, y / right.y, z / right.z);
    #endif
    }

    forceinline Vec3 VECCALL normalize() const noexcept
    {
    #if COMMON_SIMD_LV >= 42
        return FloatData * FloatData.Dot<VecPos::XYZ, VecPos::XYZ>(FloatData).Rsqrt();
    #else
        return (*this) / this->length();
    #endif
    }

    forceinline Vec3 VECCALL rcp() const noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return FloatData.Rcp();
    #else
        return Vec3(1 / x, 1 / y, 1 / z);
    #endif
    }

    forceinline Vec3 VECCALL sqrt() const noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return FloatData.Sqrt();
    #else
        return Vec3(std::sqrt(x), std::sqrt(y), std::sqrt(z));
    #endif
    }

    forceinline Vec3 VECCALL rsqrt() const noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return FloatData.Rsqrt();
    #else
        return Vec3(1 / std::sqrt(x), 1 / std::sqrt(y), 1 / std::sqrt(z));
    #endif
    }
    
    forceinline Vec3& VECCALL operator+=(const float right) noexcept
    {
    #if COMMON_SIMD_LV >= 20
        FloatData = FloatData + right;
    #else
        x += right, y += right, z += right;
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL operator+=(const Vec3& right) noexcept
    {
    #if COMMON_SIMD_LV >= 20
        FloatData = FloatData + right.FloatData;
    #else
        x += right.x, y += right.y, z += right.z;
    #endif
        return *this;
    }
    
    forceinline Vec3& VECCALL operator-=(const float right) noexcept
    {
    #if COMMON_SIMD_LV >= 20
        FloatData = FloatData - right;
    #else
        x -= right, y -= right, z -= right;
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL operator-=(const Vec3& right) noexcept
    {
    #if COMMON_SIMD_LV >= 20
        FloatData = FloatData - right.FloatData;
    #else
        x -= right.x, y -= right.y, z -= right.z;
    #endif
        return *this;
    }
    
    forceinline Vec3& VECCALL operator*=(const float right) noexcept
    {
    #if COMMON_SIMD_LV >= 20
        FloatData = FloatData * right;
    #else
        x *= right, y *= right, z *= right;
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL operator*=(const Vec3& right) noexcept
    {
    #if COMMON_SIMD_LV >= 20
        FloatData = FloatData * right.FloatData;
    #else
        x *= right.x, y *= right.y, z *= right.z;
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL operator/=(const float right) noexcept
    {
    #if COMMON_SIMD_LV >= 20
        FloatData = FloatData * F32x4(right).Rcp();
        return *this;
    #else
        return (*this) *= (1 / right);
    #endif
    }

    forceinline Vec3& VECCALL operator/=(const Vec3& right) noexcept
    {
    #if COMMON_SIMD_LV >= 20
        FloatData = FloatData * right.FloatData.Rcp();
    #else
        x /= right.x, y /= right.y, z /= right.z;
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL normalized() noexcept
    {
    #if COMMON_SIMD_LV >= 42
        *this = normalize();
    #else
        *this /= this->length();
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL rcped() noexcept
    {
    #if COMMON_SIMD_LV >= 20
        *this = rcp();
    #else
        x = 1 / x, y = 1 / y, z = 1 / z;
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL sqrted() noexcept
    {
    #if COMMON_SIMD_LV >= 20
        *this = sqrt();
    #else
        x = std::sqrt(x), y = std::sqrt(y), z = std::sqrt(z);
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL rsqrted() noexcept
    {
    #if COMMON_SIMD_LV >= 20
        *this = rsqrt();
    #else
        x = 1 / std::sqrt(x), y = 1 / std::sqrt(y), z = 1 / std::sqrt(z);
    #endif
        return *this;
    }

    forceinline Vec3& VECCALL negatived() noexcept
    {
    #if COMMON_SIMD_LV >= 20
        *this = -*this;
    #else
        x = -x, y = -y, z = -z;
    #endif
        return *this;
    }

    forceinline static Vec3 VECCALL zero() noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return _mm_setzero_ps();
    #else
        return Vec3(0, 0, 0);
    #endif
    }

    forceinline static Vec3 VECCALL one() noexcept
    {
    #if COMMON_SIMD_LV >= 20
        return _mm_set1_ps(1);
    #else
        return Vec3(1, 1, 1);
    #endif
    }
};

forceinline Vec3 VECCALL operator+(const float left, const Vec3& right) noexcept
{
    return right + left;
}

forceinline Vec3 VECCALL operator-(const float left, const Vec3& right) noexcept
{
#if COMMON_SIMD_LV >= 20
    return F32x4(left) - right.FloatData;
#else
    return Vec3(left) - right;
#endif
}

forceinline Vec3 VECCALL operator*(const float left, const Vec3& right) noexcept
{
    return right * left;
}

forceinline Vec3 VECCALL operator/(const float left, const Vec3& right) noexcept
{
#if COMMON_SIMD_LV >= 20
    return F32x4(left) / right.FloatData;
#else
    return Vec3(left) / right;
#endif
}

forceinline Vec3 VECCALL max(const Vec3& left, const Vec3& right) noexcept
{
#if COMMON_SIMD_LV >= 20
    return left.FloatData.Max(right.FloatData);
#else
    return Vec3(common::max(left.x, right.x), common::max(left.y, right.y), common::max(left.z, right.z));
#endif
}

forceinline Vec3 VECCALL min(const Vec3& left, const Vec3& right) noexcept
{
#if COMMON_SIMD_LV >= 20
    return left.FloatData.Min(right.FloatData);
#else
    return Vec3(common::min(left.x, right.x), common::min(left.y, right.y), common::min(left.z, right.z));
#endif
}


class alignas(16) Vec4 :public Vec4Base<float>
{
private:
public:
    using Vec4Base<float>::Vec4Base;

    constexpr Vec4() noexcept : Vec4Base() { }
    template<typename T>
    constexpr Vec4(const Vec3& vec, const T w_) noexcept : Vec4Base(vec) { w = w_; }
    template<typename T>
    constexpr Vec4(const T x_, const T y_, const T z_, const T w_) noexcept
        :Vec4Base(static_cast<float>(x_), static_cast<float>(y_), static_cast<float>(z_), static_cast<float>(w_))
    { }
#if COMMON_SIMD_LV >= 20
    template<typename T> Vec4(const T all) noexcept : Vec4Base(F32x4(all)) { }
#else
    template<typename T> Vec4(const T all) noexcept : Vec4(all, all, all, all) { }
#endif
    forceinline void VECCALL Save(float* ptr) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        FloatData.Save(ptr);
#else
        ptr[0] = x, ptr[1] = y, ptr[2] = z, ptr[3] = w;
#endif
    }
    forceinline const Vec3& VECCALL XYZ() const noexcept { return *reinterpret_cast<const Vec3*>(this); }


    forceinline float VECCALL dot(const Vec4& v) const noexcept//dot product
    {
#if COMMON_SIMD_LV >= 42
        return _mm_cvtss_f32(FloatData.Dot<VecPos::XYZW, VecPos::X>(v.FloatData));
#else
        return x * v.x + y * v.y + z * v.z + w * v.w;
#endif
    }

    forceinline float VECCALL length_sqr() const noexcept
    {
        return this->dot(*this);
    }

    forceinline float VECCALL length() const noexcept
    {
#if COMMON_SIMD_LV >= 42
        return _mm_cvtss_f32(FloatData.Dot<VecPos::XYZW, VecPos::X>(FloatData).Sqrt());
#else
        return std::sqrt(length_sqr());
#endif
    }

    forceinline Vec4 VECCALL operator+(const float right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return FloatData + right;
#else
        return Vec4(x + right, y + right, z + right, w + right);
#endif
    }

    forceinline Vec4 VECCALL operator+(const Vec4& right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return FloatData + right.FloatData;
#else
        return Vec4(x + right.x, y + right.y, z + right.z, w + right.w);
#endif
    }

    forceinline Vec4 VECCALL operator-() const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return FloatData ^ F32x4(-0.0f);
#else
        return Vec4(-x, -y, -z, -w);
#endif
    }

    forceinline Vec4 VECCALL operator-(const float right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return FloatData - right;
#else
        return Vec4(x - right, y - right, z - right, w - right);
#endif
    }

    forceinline Vec4 VECCALL operator-(const Vec4& right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return FloatData - right.FloatData;
#else
        return Vec4(x - right.x, y - right.y, z - right.z, w - right.w);
#endif
    }

    forceinline Vec4 VECCALL operator*(const float right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return FloatData * right;
#else
        return Vec4(x * right, y * right, z * right, w * right);
#endif
    }

    forceinline Vec4 VECCALL operator*(const Vec4& right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return FloatData * right.FloatData;
#else
        return Vec4(x * right.x, y * right.y, z * right.z, w * right.w);
#endif
    }

    forceinline Vec4 VECCALL operator/(const float right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return FloatData * F32x4(right).Rcp();
#else
        return (*this) * (1 / right);
#endif
    }

    forceinline Vec4 VECCALL operator/(const Vec4& right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return FloatData * right.FloatData.Rcp();
#else
        return Vec4(x / right.x, y / right.y, z / right.z, w / right.w);
#endif
    }

    forceinline Vec4 VECCALL normalize() const noexcept
    {
#if COMMON_SIMD_LV >= 42
        return FloatData * FloatData.Dot<VecPos::XYZW, VecPos::XYZW>(FloatData).Rsqrt();
#else
        return (*this) / this->length();
#endif
    }

    forceinline Vec4 VECCALL rcp() const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return FloatData.Rcp();
#else
        return Vec4(1 / x, 1 / y, 1 / z, 1 / w);
#endif
    }

    forceinline Vec4 VECCALL sqrt() const noexcept
    {
#if COMMON_SIMD_LV >= 42
        return FloatData.Sqrt();
#else
        return Vec4(std::sqrt(x), std::sqrt(y), std::sqrt(z), std::sqrt(w));
#endif
    }

    forceinline Vec4 VECCALL rsqrt() const noexcept
    {
#if COMMON_SIMD_LV >= 42
        return FloatData.Rsqrt();
#else
        return Vec4(1 / std::sqrt(x), 1 / std::sqrt(y), 1 / std::sqrt(z), 1 / std::sqrt(w));
#endif
    }

    forceinline Vec4& VECCALL operator+=(const float right) noexcept
    {
#if COMMON_SIMD_LV >= 20
        FloatData = FloatData + right;
#else
        x += right, y += right, z += right, w += right;
#endif
        return *this;
    }

    forceinline Vec4& VECCALL operator+=(const Vec4& right) noexcept
    {
#if COMMON_SIMD_LV >= 20
        FloatData = FloatData + right.FloatData;
#else
        x += right.x, y += right.y, z += right.z, w += right.w;
#endif
        return *this;
    }

    forceinline Vec4& VECCALL operator-=(const float right) noexcept
    {
#if COMMON_SIMD_LV >= 20
        FloatData = FloatData - right;
#else
        x -= right, y -= right, z -= right, w -= right;
#endif
        return *this;
    }

    forceinline Vec4& VECCALL operator-=(const Vec4& right) noexcept
    {
#if COMMON_SIMD_LV >= 20
        FloatData = FloatData - right.FloatData;
#else
        x -= right.x, y -= right.y, z -= right.z, w -= right.w;
#endif
        return *this;
    }

    forceinline Vec4& VECCALL operator*=(const float right) noexcept
    {
#if COMMON_SIMD_LV >= 20
        FloatData = FloatData * right;
#else
        x *= right, y *= right, z *= right, w *= right;
#endif
        return *this;
    }

    forceinline Vec4& VECCALL operator*=(const Vec4& right) noexcept
    {
#if COMMON_SIMD_LV >= 20
        FloatData = FloatData * right.FloatData;
#else
        x *= right.x, y *= right.y, z *= right.z, w *= right.w;
#endif
        return *this;
    }

    forceinline Vec4& VECCALL operator/=(const float right) noexcept
    {
#if COMMON_SIMD_LV >= 20
        FloatData = FloatData * F32x4(right).Rcp();
        return *this;
#else
        return (*this) *= (1 / right);
#endif
    }

    forceinline Vec4& VECCALL operator/=(const Vec4& right) noexcept
    {
#if COMMON_SIMD_LV >= 20
        FloatData = FloatData * right.FloatData.Rcp();
        return *this;
#else
        x /= right.x, y /= right.y, z /= right.z, w /= right.w;
        return *this;
#endif
    }

    forceinline Vec4& VECCALL normalized() noexcept
    {
#if COMMON_SIMD_LV >= 42
        *this = normalize();
#else
        *this /= this->length();
#endif
        return *this;
    }

    forceinline Vec4& VECCALL rcped() noexcept
    {
#if COMMON_SIMD_LV >= 20
        *this = rcp();
#else
        x = 1 / x, y = 1 / y, z = 1 / z;
#endif
        return *this;
    }

    forceinline Vec4& VECCALL sqrted() noexcept
    {
#if COMMON_SIMD_LV >= 20
        *this = sqrt();
#else
        x = std::sqrt(x), y = std::sqrt(y), z = std::sqrt(z), w = std::sqrt(w);
#endif
        return *this;
    }

    forceinline Vec4& VECCALL rsqrted() noexcept
    {
#if COMMON_SIMD_LV >= 20
        *this = rsqrt();
#else
        x = 1 / std::sqrt(x), y = 1 / std::sqrt(y), z = 1 / std::sqrt(z), w = 1 / std::sqrt(w);
#endif
        return *this;
    }

    forceinline Vec4& VECCALL negatived() noexcept
    {
#if COMMON_SIMD_LV >= 20
        *this = -*this;
#else
        x = -x, y = -y, z = -z, w = -w;
#endif
        return *this;
    }

    forceinline static Vec4 VECCALL zero() noexcept
    {
#if COMMON_SIMD_LV >= 20
        return _mm_setzero_ps();
#else
        return Vec4(0, 0, 0, 0);
#endif
    }

    forceinline static Vec4 VECCALL one() noexcept
    {
#if COMMON_SIMD_LV >= 20
        return _mm_set1_ps(1);
#else
        return Vec4(1, 1, 1, 1);
#endif
    }
};

forceinline Vec4 VECCALL operator+(const float left, const Vec4& right) noexcept
{
    return right + left;
}

forceinline Vec4 VECCALL operator-(const float left, const Vec4& right) noexcept
{
#if COMMON_SIMD_LV >= 20
    return F32x4(left) - right.FloatData;
#else
    return Vec4(left) - right;
#endif
}

forceinline Vec4 VECCALL operator*(const float left, const Vec4& right) noexcept
{
    return right * left;
}

forceinline Vec4 VECCALL operator/(const float left, const Vec4& right) noexcept
{
#if COMMON_SIMD_LV >= 20
    return F32x4(left) / right.FloatData;
#else
    return Vec4(left) / right;
#endif
}

forceinline Vec4 VECCALL max(const Vec4& left, const Vec4& right) noexcept
{
#if COMMON_SIMD_LV >= 20
    return left.FloatData.Max(right.FloatData);
#else
    return Vec4(common::max(left.x, right.x), common::max(left.y, right.y), common::max(left.z, right.z), common::max(left.w, right.w));
#endif
}

forceinline Vec4 VECCALL min(const Vec4& left, const Vec4& right) noexcept
{
#if COMMON_SIMD_LV >= 20
    return left.FloatData.Min(right.FloatData);
#else
    return Vec4(common::min(left.x, right.x), common::min(left.y, right.y), common::min(left.z, right.z), common::min(left.w, right.w));
#endif
}


class alignas(16) VecI4 :public Vec4Base<int32_t>
{
private:
public:
    using Vec4Base<int32_t>::Vec4Base;

    constexpr VecI4() noexcept : Vec4Base() { }
    template<typename T>
    constexpr VecI4(const T x_, const T y_, const T z_, const T w_) noexcept
        :Vec4Base(static_cast<int32_t>(x_), static_cast<int32_t>(y_), static_cast<int32_t>(z_), static_cast<int32_t>(w_))
    { }
#if COMMON_SIMD_LV >= 20
    template<typename T> VecI4(const T all) noexcept : Vec4Base(I32x4(all)) { }
#else
    template<typename T> VecI4(const T all) noexcept : VecI4(all, all, all, all) { }
#endif
    forceinline void VECCALL Save(int32_t* ptr) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        IntData.Save(ptr);
#else
        ptr[0] = x, ptr[1] = y, ptr[2] = z, ptr[3] = w;
#endif
    }
    //const VecI3& XYZ() const noexcept { return *reinterpret_cast<const VecI3*>(this); }

    forceinline int32_t VECCALL dot(const VecI4& v) const noexcept//dot product
    {
        return x * v.x + y * v.y + z * v.z + w * v.w;
    }


    forceinline int32_t VECCALL length_sqr() const noexcept
    {
        return this->dot(*this);
    }

    forceinline VecI4 VECCALL operator+(const int32_t right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return IntData + right;
#else
        return VecI4(x + right, y + right, z + right, w + right);
#endif
    }

    forceinline VecI4 VECCALL operator+(const VecI4& right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return IntData + right.IntData;
#else
        return VecI4(x + right.x, y + right.y, z + right.z, w + right.w);
#endif
    }

    forceinline VecI4 VECCALL operator-() const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return IntData ^ I32x4(INT32_MIN);
#else
        return VecI4(-x, -y, -z, -w);
#endif
    }

    forceinline VecI4 VECCALL operator-(const int32_t right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return IntData - right;
#else
        return VecI4(x - right, y - right, z - right, w - right);
#endif
    }

    forceinline VecI4 VECCALL operator-(const VecI4& right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return IntData - right.IntData;
#else
        return VecI4(x - right.x, y - right.y, z - right.z, w - right.w);
#endif
    }

    forceinline VecI4 VECCALL operator*(const int32_t right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return IntData * right;
#else
        return VecI4(x * right, y * right, z * right, w * right);
#endif
    }

    forceinline VecI4 VECCALL operator*(const VecI4& right) const noexcept
    {
#if COMMON_SIMD_LV >= 20
        return IntData * right.IntData;
#else
        return VecI4(x * right.x, y * right.y, z * right.z, w * right.w);
#endif
    }

    forceinline VecI4 VECCALL operator/(const int32_t right) const noexcept
    {
        return (*this) * (1 / right);
    }

    forceinline VecI4 VECCALL operator/(const VecI4& right) const noexcept
    {
        return VecI4(x / right.x, y / right.y, z / right.z, w / right.w);
    }

    forceinline VecI4& VECCALL operator+=(const VecI4& right) noexcept
    {
#if COMMON_SIMD_LV >= 20
        *this = *this + right;
#else
        x += right.x, y += right.y, z += right.z, w += right.w;
#endif
        return *this;
    }

    forceinline VecI4& VECCALL operator-=(const VecI4& right) noexcept
    {
#if COMMON_SIMD_LV >= 20
        * this = *this - right;
#else
        x -= right.x, y -= right.y, z -= right.z, w -= right.w;
#endif
        return *this;
    }

    forceinline VecI4& VECCALL operator*=(const int32_t right) noexcept
    {
#if COMMON_SIMD_LV >= 20
        * this = *this * right;
#else
        x *= right, y *= right, z *= right, w *= right;
#endif
        return *this;
    }

    forceinline VecI4& VECCALL operator*=(const VecI4& right) noexcept
    {
#if COMMON_SIMD_LV >= 20
        * this = *this * right;
#else
        x *= right.x, y *= right.y, z *= right.z, w *= right.w;
#endif
        return *this;
    }

    forceinline VecI4& VECCALL operator/=(const int32_t right) noexcept
    {
        x /= right, y /= right, z /= right, w /= right;
        return *this;
    }

    forceinline VecI4& VECCALL operator/=(const VecI4& right) noexcept
    {
        x /= right.x, y /= right.y, z /= right.z, w /= right.w;
        return *this;
    }

};

forceinline VecI4 VECCALL operator+(const int32_t left, const VecI4& right) noexcept
{
    return right + left;
}

forceinline VecI4 VECCALL operator-(const int32_t left, const VecI4& right) noexcept
{
#if COMMON_SIMD_LV >= 20
    return I32x4(left) - right.IntData;
#else
    return VecI4(left) - right;
#endif
}

forceinline VecI4 VECCALL operator*(const int32_t left, const VecI4& right) noexcept
{
    return right * left;
}

forceinline VecI4 VECCALL operator/(const int32_t left, const VecI4& right) noexcept
{
    return VecI4(left) / right;
}

forceinline VecI4 VECCALL max(const VecI4& left, const VecI4& right) noexcept
{
#if COMMON_SIMD_LV >= 20
    return left.IntData.Max(right.IntData);
#else
    return VecI4(common::max(left.x, right.x), common::max(left.y, right.y), common::max(left.z, right.z), common::max(left.w, right.w));
#endif
}

forceinline VecI4 VECCALL min(const VecI4& left, const VecI4& right) noexcept
{
#if COMMON_SIMD_LV >= 20
    return left.IntData.Min(right.IntData);
#else
    return VecI4(common::min(left.x, right.x), common::min(left.y, right.y), common::min(left.z, right.z), common::min(left.w, right.w));
#endif
}


}

