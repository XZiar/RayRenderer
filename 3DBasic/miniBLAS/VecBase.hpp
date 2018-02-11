#pragma once

#include "BLASrely.h"

namespace miniBLAS
{

#if defined(__AVX__)
#   define Permute128PS(xmm, imm) _mm_permute_ps(xmm, imm)
#else
#   define Permute128PS(xmm, imm) _mm_shuffle_ps(xmm, xmm, imm)
#endif
#define Load128PS(ptr) _mm_load_ps(ptr)
#define Store128PS(ptr, val) _mm_store_ps(ptr, val)
#define Load128I(vec) _mm_load_si128(&vec)
#define Store128I(vec, val) _mm_store_si128(&vec, val)

/*a vector contains 4 element(int32 or float)*/
static constexpr uint32_t Vec4Align = 16;
template<typename T>
class alignas(Vec4Align) Vec4Base : public common::AlignBase<Vec4Align>
{
private:
    static_assert(sizeof(T) == 4, "only 4-byte length type allowed");
protected:
    using RawDataType = T[4];
    union
    {
        T data[4];
        __m128 float_dat;
        __m128i int_dat;
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

    Vec4Base() noexcept { };
    Vec4Base(const T x_) noexcept :x(x_) { };
    Vec4Base(const T x_, const T y_) noexcept :x(x_), y(y_) { };
    Vec4Base(const T x_, const T y_, const T z_) noexcept :x(x_), y(y_), z(z_) { };
    Vec4Base(const T x_, const T y_, const T z_, const T w_) noexcept :x(x_), y(y_), z(z_), w(w_) { };
public:
    RawDataType& raw() noexcept { return data; }
    const RawDataType& raw() const noexcept { return data; }
    operator RawDataType&() { return data; };
    operator const RawDataType&() const { return data; };
    forceinline T& operator[](int idx)
    {
        return data[idx];
    }
    forceinline T const& operator[](int idx) const
    {
        return data[idx];
    }
    bool operator<(const Vec4Base& other) const = delete;
};


class alignas(Vec4Align) Vec3;
class alignas(Vec4Align) Vec4;

}

