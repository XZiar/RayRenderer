#pragma once

#include "BLASrely.h"
#include "VecBase.hpp"

namespace miniBLAS
{

#define Load256PS(vec) _mm256_load_ps((const float*)&vec)
#define Store256PS(vec, val) _mm256_store_ps((float*)&vec, val)
#define Load256I(vec) _mm256_load_si256(&vec)
#define Store256I(vec, val) _mm256_store_si256(&vec, val)

/*4x4matrix contains 4 row of 4elements-vector, align to 32bytes for AVX*/
inline constexpr uint32_t SQMat4Align = 32;
template<typename T1, typename T2, typename T3>
class alignas(SQMat4Align) SQMat4Base
{
    static_assert(sizeof(T1) == 4, "only 4-byte length type(for a element) allowed");
    static_assert(sizeof(T2) == 16, "only 16-byte length type(for a row) allowed");
    static_assert(sizeof(T3) == 32, "T2 should be twice of T");
protected:
    using RowDataType = T2[4];
    union
    {
        float float_ele[16];
        int int_ele[16];
        T1 element[16];
        __m128 float_row[4];
        __m128i int_row[4];
        T2 row[4];
        __m256 float_row2[2];
        __m256i int_row2[2];
        T3 row2[2];
        struct
        {
            T2 x, y, z, w;
        };
        struct
        {
            __m128 float_x, float_y, float_z, float_w;
        };
        struct
        {
            __m128i int_x, int_y, int_z, int_w;
        };
        struct
        {
            T3 xy, zw;
        };
        struct
        {
            __m256 float_xy, float_zw;
        };
        struct
        {
            __m256i int_xy, int_zw;
        };
    };

    SQMat4Base() noexcept { };
    SQMat4Base(const T2& x_) noexcept :x(x_) { };
    SQMat4Base(const T2& x_, const T2& y_) noexcept :x(x_), y(y_) { };
    SQMat4Base(const T2& x_, const T2& y_, const T2& z_) noexcept :x(x_), y(y_), z(z_) { };
    SQMat4Base(const T2& x_, const T2& y_, const T2& z_, const T2& w_) noexcept :x(x_), y(y_), z(z_), w(w_) { };
    //SQMat4Base(const T3& xy_) noexcept :xy(xy_) { };
    //SQMat4Base(const T3& xy_, const T2& z_) noexcept :xy(xy_), z(z_) { };
    SQMat4Base(const T3& xy_, const T3& zw_) noexcept :xy(xy_), zw(zw_) { };
public:
    operator RowDataType&() { return row; };
    operator const RowDataType&() const { return row; };
    forceinline T2& operator[](const int rowidx)
    {
        return row[rowidx];
    };
    forceinline const T2& operator[](const int rowidx) const
    {
        return row[rowidx];
    };
    forceinline T1& operator()(const int rowidx, const int colidx)
    {
        return element[rowidx * 4 + colidx];
    }
    forceinline const T1& operator()(const int rowidx, const int colidx) const
    {
        return element[rowidx * 4 + colidx];
    }
};

using __m128x3 = __m128[3];
using __m128x4 = __m128[4];
using __m256x2 = __m256[2];

class Mat3x3;
class Mat4x4;

}