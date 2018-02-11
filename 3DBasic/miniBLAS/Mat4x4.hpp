#pragma once

#include "BLASrely.h"
#include "MatBase.hpp"
#include "Vec4.hpp"
#include "Mat3x3.hpp"

namespace miniBLAS
{

class alignas(SQMat4Align) Mat4x4 :public SQMat4Base<float, Vec4, __m256>
{
protected:
    static void VECCALL MatrixTranspose4x4(const Mat4x4& in, Mat4x4& out)
    {
    #if defined(__AVX__)
        const __m256 n1 = _mm256_permute_ps(in.xy, _MM_SHUFFLE(3, 1, 2, 0))/*x1,z1,y1,w1;x2,z2,y2,w2*/;
        const __m256 n2 = _mm256_permute_ps(in.zw, _MM_SHUFFLE(3, 1, 2, 0))/*x3,z3,y3,w3;x4,z4,y4,w4*/;
        const __m256 t1 = _mm256_unpacklo_ps(n1, n2)/*x1,x3,z1,z3;x2,x4,z2,z4*/;
        const __m256 t2 = _mm256_unpackhi_ps(n1, n2)/*y1,y3,w1,w3;y2,y4,w2,w4*/;
        const __m256 t3 = _mm256_permute2f128_ps(t1, t2, 0b00100000)/*x1,x3,z1,z3;y1,y3,w1,w3*/;
        const __m256 t4 = _mm256_permute2f128_ps(t1, t2, 0b00110001)/*x2,x4,z2,z4;y2,y4,w2,w4*/;
        out.xy = _mm256_unpacklo_ps(t3, t4)/*x1,x2,x3,x4;y1,y2,y3,y4*/;
        out.zw = _mm256_unpackhi_ps(t3, t4)/*z1,z2,z3,z4;w1,w2,w3,w4*/;
    #elif defined (__SSE2__)
        const __m128 xy12 = _mm_movelh_ps(in.x, in.y)/*x1,y1,x2,y2*/;
        const __m128 zw12 = _mm_movehl_ps(in.y, in.x)/*z1,w1,z2,w2*/;
        const __m128 xy34 = _mm_movelh_ps(in.z, in.w)/*x3,y3,x4,y4*/;
        const __m128 zw34 = _mm_movehl_ps(in.w, in.z)/*x3,y3,x4,y4*/;
        out.x = _mm_shuffle_ps(xy12, xy34, _MM_SHUFFLE(2, 0, 2, 0))/*x1,x2,x3,x4*/;
        out.y = _mm_shuffle_ps(xy12, xy34, _MM_SHUFFLE(3, 1, 3, 1))/*y1,y2,y3,y4*/;
        out.z = _mm_shuffle_ps(zw12, zw34, _MM_SHUFFLE(2, 0, 2, 0))/*z1,z2,z3,z4*/;
        out.w = _mm_shuffle_ps(zw12, zw34, _MM_SHUFFLE(3, 1, 3, 1))/*w1,w2,w3,w4*/;
    #else
        if (&in == &out)
        {
            std::swap(out(1, 0), out(0, 1));
            std::swap(out(2, 0), out(0, 2));
            std::swap(out(2, 1), out(1, 2));
            std::swap(out(3, 0), out(0, 3));
            std::swap(out(3, 1), out(1, 3));
            std::swap(out(3, 2), out(2, 3));
        }
        else
        {
            out(0, 0) = in(0, 0), out(1, 1) = in(1, 1), out(2, 2) = in(2, 2), out(3, 3) = in(3, 3);
            float tmp;
            tmp = in(0, 1); out(0, 1) = in(1, 0); out(1, 0) = tmp;
            tmp = in(0, 2); out(0, 2) = in(2, 0); out(2, 0) = tmp;
            tmp = in(1, 2); out(1, 2) = in(2, 1); out(2, 1) = tmp;
            tmp = in(0, 3); out(0, 3) = in(3, 0); out(3, 0) = tmp;
            tmp = in(1, 3); out(1, 3) = in(3, 1); out(3, 1) = tmp;
            tmp = in(2, 3); out(2, 3) = in(3, 2); out(3, 2) = tmp;
        }
    #endif
    }
    static void VECCALL MatrixMultiply4x4(const Mat4x4& left, const Mat4x4& right, Mat4x4& out)
    {
    #ifdef __AVX__
        const __m256 t1 = _mm256_unpacklo_ps(right.xy, right.zw)/*x1,x3,y1,y3;x2,x4,y2,y4*/;
        const __m256 t2 = _mm256_unpackhi_ps(right.xy, right.zw)/*z1,z3,w1,w3;z2,z4,w2,w4*/;
        const __m256 t3 = _mm256_permute2f128_ps(t1, t2, 0b00100000)/*x1,x3,y1,y3;z1,z3,w1,w3*/;
        const __m256 t4 = _mm256_permute2f128_ps(t1, t2, 0b00110001)/*x2,x4,y2,y4;z2,z4,w2,w4*/;
        const __m256 rxz = _mm256_unpacklo_ps(t3, t4)/*x1,x2,x3,x4;z1,z2,z3,z4*/;
        const __m256 ryw = _mm256_unpackhi_ps(t3, t4)/*y1,y2,y3,y4;w1,w2,w3,w4*/;
        const __m256 rxx = _mm256_permute2f128_ps(rxz, ryw, 0x00); const __m256 ryy = _mm256_permute2f128_ps(rxz, ryw, 0x22);
        const __m256 rzz = _mm256_permute2f128_ps(rxz, ryw, 0x11); const __m256 rww = _mm256_permute2f128_ps(rxz, ryw, 0x33);

        const __m256 xy0 = _mm256_dp_ps(left.xy, rxx, 0xf1), xy1 = _mm256_dp_ps(left.xy, ryy, 0xf2);
        const __m256 xy2 = _mm256_dp_ps(left.xy, rzz, 0xf4), xy3 = _mm256_dp_ps(left.xy, rww, 0xf8);
        out.xy = _mm256_blend_ps(_mm256_blend_ps(xy0, xy1, 0b00100010)/*x0,x1,,;y0,y1,,*/,
            _mm256_blend_ps(xy2, xy3, 0b10001000)/*,,x2,x3;,,y2,y3*/, 0b11001100)/*x0,x1,x2,x3;y0,y1,y2,y3*/;
        const __m256 zw0 = _mm256_dp_ps(left.zw, rxx, 0xf1), zw1 = _mm256_dp_ps(left.zw, ryy, 0xf2);
        const __m256 zw2 = _mm256_dp_ps(left.zw, rzz, 0xf4), zw3 = _mm256_dp_ps(left.zw, rww, 0xf8);
        out.zw = _mm256_blend_ps(_mm256_blend_ps(zw0, zw1, 0b00100010)/*z1,z1,,;w0,w1,,*/,
            _mm256_blend_ps(zw2, zw3, 0b10001000)/*,,z2,z3;,,w2,w3*/, 0b11001100)/*z0,z1,z2,z3;w0,w1,w2,w3*/;
    #elif defined(__SSE2__)
        Mat4x4 ir = right.inv();
        const __m128 x1 = _mm_dp_ps(left.x, ir.x, 0xf1), y1 = _mm_dp_ps(left.x, ir.y, 0xf2), z1 = _mm_dp_ps(left.x, ir.z, 0xf4), w1 = _mm_dp_ps(left.x, ir.w, 0xf8);
        const __m128 x2 = _mm_dp_ps(left.y, ir.x, 0xf1), y2 = _mm_dp_ps(left.y, ir.y, 0xf2), z2 = _mm_dp_ps(left.y, ir.z, 0xf4), w2 = _mm_dp_ps(left.y, ir.w, 0xf8);
        const __m128 x3 = _mm_dp_ps(left.z, ir.x, 0xf1), y3 = _mm_dp_ps(left.z, ir.y, 0xf2), z3 = _mm_dp_ps(left.z, ir.z, 0xf4), w3 = _mm_dp_ps(left.z, ir.w, 0xf8);
        const __m128 x4 = _mm_dp_ps(left.w, ir.x, 0xf1), y4 = _mm_dp_ps(left.w, ir.y, 0xf2), z4 = _mm_dp_ps(left.w, ir.z, 0xf4), w4 = _mm_dp_ps(left.w, ir.w, 0xf8);
        out.x = _mm_blend_ps(_mm_blend_ps(x1, y1, 0b0010)/*x1,y1,,*/, _mm_blend_ps(z1, w1, 0b1000)/*,,z1,w1*/, 0b1100)/*x1,y1,w1,z1*/;
        out.y = _mm_blend_ps(_mm_blend_ps(x2, y2, 0b0010)/*x2,y2,,*/, _mm_blend_ps(z2, w2, 0b1000)/*,,z2,w2*/, 0b1100)/*x2,y2,w2,z2*/;
        out.z = _mm_blend_ps(_mm_blend_ps(x3, y3, 0b0010)/*x3,y3,,*/, _mm_blend_ps(z3, w3, 0b1000)/*,,z3,w3*/, 0b1100)/*x3,y3,w3,z3*/;
        out.w = _mm_blend_ps(_mm_blend_ps(x4, y4, 0b0010)/*x4,y4,,*/, _mm_blend_ps(z4, w4, 0b1000)/*,,z4,w4*/, 0b1100)/*x4,y4,w4,z4*/;
    #else
        memset(out.element, 0, sizeof(Mat4x4));
        for (int32_t k = 0; k < 4; k++)
            for (int32_t i = 0; i < 4; i++)
                for (int32_t j = 0; j < 4; j++)
                    out(i, j) += left(i, k) * right(k, j);
    #endif
    }
public:
    using SQMat4Base::x; using SQMat4Base::y; using SQMat4Base::z; using SQMat4Base::w;

    Mat4x4() noexcept { }
    Mat4x4(const Vec4& x_, const Vec4& y_, const Vec4& z_, const Vec4& w_) noexcept :SQMat4Base(x_, y_, z_, w_) { };
    Mat4x4(const __m256 xy_, const __m256 zw_) noexcept :SQMat4Base(xy_, zw_) { };
    explicit Mat4x4(const Vec4 *ptr) noexcept :SQMat4Base(ptr[0], ptr[1], ptr[2], ptr[3]) { };
#ifdef __SSE2__
    Mat4x4(const __m128x4& dat) noexcept
    {
        x = dat[0]; y = dat[1]; z = dat[2]; w = dat[3];
    };
#endif
    template<class T>
    explicit Mat4x4(const T *ptr) noexcept
    {
        for (int32_t a = 0; a < 16; ++a)
            element[a] = static_cast<float>(ptr[a]);
    }
    explicit Mat4x4(const float *ptr) noexcept
    {
    #ifdef __AVX__
        float_xy = _mm256_loadu_ps(ptr);
        float_zw = _mm256_loadu_ps(ptr + 8);
    #elif defined (__SSE2__)
        float_x = _mm_loadu_ps(ptr);
        float_y = _mm_loadu_ps(ptr + 4);
        float_z = _mm_loadu_ps(ptr + 8);
        float_w = _mm_loadu_ps(ptr + 12);
    #else
        memcpy(element, ptr, sizeof(float) * 16);
    #endif
    };
    explicit Mat4x4(const Mat3x3& m, const bool isHomogeneous = true) noexcept
    {
    #ifdef __SSE2__
        x = Vec4(m.x, .0f), y = Vec4(m.y, .0f), z = Vec4(m.z, .0f);
        if (isHomogeneous)
            w = _mm_setr_ps(0, 0, 0, 1);
    #else
        memcpy(element, &m, sizeof(float) * 12);
        if (isHomogeneous)
            element[3] = element[7] = element[11] = element[12] = element[13] = element[14] = 0, element[15] = 1;
    #endif
    }

    VECCALL operator float*() noexcept { return element; };
    VECCALL operator const float*() const noexcept { return element; };
    explicit VECCALL operator Mat3x3&() noexcept { return *(Mat3x3*)this; }
    explicit VECCALL operator const Mat3x3&() const noexcept { return *(Mat3x3*)this; }


    Mat4x4 VECCALL inv() const
    {
        Mat4x4 ret;
        MatrixTranspose4x4(*this, ret);
        return ret;
    }

    forceinline Mat4x4 VECCALL operator+(const float right) const
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(right);
        return Mat4x4(_mm256_add_ps(xy, v256), _mm256_add_ps(zw, v256));
    #elif defined (__SSE2__)
        const __m128 v128 = _mm_set1_ps(right);
        return Mat4x4(_mm_add_ps(x, v128), _mm_add_ps(y, v128), _mm_add_ps(z, v128), _mm_add_ps(w, v128));
    #else
        return Mat4x4(x + right, y + right, z + right, w + right);
    #endif
    }

    forceinline Mat4x4 VECCALL operator+(const Mat4x4& right) const
    {
    #ifdef __AVX__
        return Mat4x4(_mm256_add_ps(xy, right.xy), _mm256_add_ps(zw, right.zw));
    #else
        return Mat4x4(x + right.x, y + right.y, z + right.z, w + right.w);
    #endif
    }

    forceinline Mat4x4 VECCALL operator-(const float right) const
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(right);
        return Mat4x4(_mm256_sub_ps(xy, v256), _mm256_sub_ps(zw, v256));
    #elif defined (__SSE2__)
        const __m128 v128 = _mm_set1_ps(right);
        return Mat4x4(_mm_sub_ps(x, v128), _mm_sub_ps(y, v128), _mm_sub_ps(z, v128), _mm_sub_ps(w, v128));
    #else
        return Mat4x4(x - right, y - right, z - right, w - right);
    #endif
    }

    forceinline Mat4x4 VECCALL operator-(const Mat4x4& right) const
    {
    #ifdef __AVX__
        return Mat4x4(_mm256_sub_ps(xy, right.xy), _mm256_sub_ps(zw, right.zw));
    #else
        return Mat4x4(x - right.x, y - right.y, z - right.z, w - right.w);
    #endif
    }

    forceinline Mat4x4 VECCALL operator*(const float right) const
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(right);
        return Mat4x4(_mm256_mul_ps(xy, v256), _mm256_mul_ps(zw, v256));
    #else
        return Mat4x4(x * right, y * right, z * right, w * right);
    #endif
    }

    forceinline Vec4 VECCALL operator*(const Vec4& right) const
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_broadcast_ps((const __m128*)&right);
        const __m256 ans = _mm256_blend_ps(
            _mm256_dp_ps(xy, v256, 0xff),
            _mm256_dp_ps(zw, v256, 0xff),
            0b11001100)/*xxzz,yyww*/;
        return _mm_blend_ps(_mm256_castps256_ps128(ans), _mm256_extractf128_ps(ans, 1), 0b1010)/*xyzw*/;
    #else
        return Vec4(x.dot(right), y.dot(right), z.dot(right), w.dot(right));
    #endif
    }

    forceinline Mat4x4 VECCALL operator*(const Mat4x4& right) const
    {
        Mat4x4 ret;
        MatrixMultiply4x4(*this, right, ret);
        return ret;
    }

    forceinline Mat4x4 VECCALL operator/(const float right) const
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(right);
        return Mat4x4(_mm256_div_ps(xy, v256), _mm256_div_ps(zw, v256));
    #else
        return Mat4x4(x / right, y / right, z / right, w / right);
    #endif
    }


    forceinline Mat4x4& VECCALL inved()
    {
        MatrixTranspose4x4(*this, *this);
        return *this;
    }

    forceinline Mat4x4& VECCALL operator+=(const float n)
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(n);
        xy = _mm256_add_ps(xy, v256), zw = _mm256_add_ps(zw, v256);
    #elif defined (__SSE2__)
        const __m128 v128 = _mm_set1_ps(n);
        x = _mm_add_ps(x, v128), y = _mm_add_ps(y, v128), z = _mm_add_ps(z, v128), w = _mm_add_ps(w, v128);
    #else
        x += n, y += n, z += n, w += n;
    #endif
        return *this;
    }

    forceinline Mat4x4& VECCALL operator+=(const Mat4x4& m)
    {
    #ifdef __AVX__
        xy = _mm256_add_ps(xy, m.xy); zw = _mm256_add_ps(zw, m.zw);
    #else
        x += m.x, y += m.y, z += m.z, w += m.w;
    #endif
        return *this;
    }
    
    forceinline Mat4x4& VECCALL operator-=(const float n)
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(n);
        xy = _mm256_sub_ps(xy, v256), zw = _mm256_sub_ps(zw, v256);
    #elif defined (__SSE2__)
        const __m128 v128 = _mm_set1_ps(n);
        x = _mm_sub_ps(x, v128), y = _mm_sub_ps(y, v128), z = _mm_sub_ps(z, v128), w = _mm_sub_ps(w, v128);
    #else
        x -= n, y -= n, z -= n, w -= n;
    #endif
        return *this;
    }

    forceinline Mat4x4& VECCALL operator-=(const Mat4x4& m)
    {
    #ifdef __AVX__
        xy = _mm256_sub_ps(xy, m.xy); zw = _mm256_sub_ps(zw, m.zw);
    #else
        x -= m.x, y -= m.y, z -= m.z, w -= m.w;
    #endif
        return *this;
    }

    forceinline Mat4x4& VECCALL operator*=(const float n)
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(n);
        xy = _mm256_mul_ps(xy, v256), zw = _mm256_mul_ps(zw, v256);
    #elif defined(__SSE2__)
        const __m128 v128 = _mm_set1_ps(n);
        x = _mm_mul_ps(x, v128), y = _mm_mul_ps(y, v128), z = _mm_mul_ps(z, v128), w = _mm_mul_ps(w, v128);
    #else
        x *= n, y *= n, z *= n, w *= n;
    #endif
        return *this;
    }

    forceinline Mat4x4& VECCALL operator*=(const Mat4x4& m)
    {
        MatrixMultiply4x4(*this, m, *this);
        return *this;
    }

    forceinline Mat4x4& operator/=(const float n)
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(n);
        xy = _mm256_div_ps(xy, v256), zw = _mm256_div_ps(zw, v256);
    #else
        (*this) *= (1 / n);
    #endif
        return *this;
    }


    static Mat4x4 VECCALL identity()
    {
        static constexpr float dat[16] = 
        {   1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1 };
        return Mat4x4(dat);
    }

    static Mat4x4 VECCALL zero()
    {
    #ifdef __AVX__
        const auto tmp = _mm256_setzero_ps();
        return Mat4x4(tmp, tmp);
    #elif defined(__SSE2__)
        const auto tmp = _mm_setzero_ps();
        return Mat4x4(tmp, tmp, tmp, tmp);
    #else
        Mat4x4 ret;
        memset(&ret, 0x0, sizeof(Mat4x4));
        return ret;
    #endif
    }

    static Mat4x4 VECCALL one()
    {
    #ifdef __AVX__
        const auto tmp = _mm256_set1_ps(1);
        return Mat4x4(tmp, tmp);
    #elif defined(__SSE2__)
        const auto tmp = _mm_set1_ps(1);
        return Mat4x4(tmp, tmp, tmp, tmp);
    #else
        static constexpr float dat[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        return Mat4x4(dat);
    #endif
    }

    static Mat4x4 VECCALL all(const float val)
    {
    #ifdef __AVX__
        const auto tmp = _mm256_set1_ps(val);
        return Mat4x4(tmp, tmp);
    #elif defined(__SSE2__)
        const auto tmp = _mm_set1_ps(val);
        return Mat4x4(tmp, tmp, tmp);
    #else
        float dat[12] = { val };
        return Mat4x4(dat);
    #endif
    }

};

forceinline Mat4x4 VECCALL operator+(const float left, const Mat4x4& right)
{
    return right + left;
}

forceinline Mat4x4 VECCALL operator-(const float left, const Mat4x4& right)
{
    return Mat4x4::all(left) - right;
}

forceinline Mat4x4 VECCALL operator*(const float left, const Mat4x4& right)
{
    return right*left;
}

forceinline Vec4 VECCALL operator*(const Vec4& left, const Mat4x4& right)
{
    return right.inv()*left;
}

forceinline Vec4& VECCALL operator*=(Vec4& left, const Mat4x4& right)
{
    left = right.inv()*left;
    return left;
}

}