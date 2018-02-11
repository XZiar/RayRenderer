#pragma once

#include "BLASrely.h"
#include "MatBase.hpp"
#include "Vec3.hpp"

namespace miniBLAS
{

/*3x3 matrix, row4's value is not promised while calculation.*/
class alignas(SQMat4Align) Mat3x3 :public SQMat4Base<float, Vec3, __m256>
{
protected:
    static void VECCALL MatrixTranspose3x3(const Mat3x3& in, Mat3x3& out)
    {
    #if defined(__AVX__)
        const __m256 n1 = _mm256_permute_ps(in.xy, _MM_SHUFFLE(3, 1, 2, 0));//x1,z1,y1,w1;x2,z2,y2,w2
        const __m256 n2 = _mm256_permute_ps(in.zw, _MM_SHUFFLE(3, 1, 2, 0));//x3,z3,y3,w3;x4,z4,y4,w4
        const __m256 t1 = _mm256_unpacklo_ps(n1, n2);//x1,x3,z1,z3;x2,x4,z2,z4
        const __m256 t2 = _mm256_unpackhi_ps(n1, n2);//y1,y3,w1,w3;y2,y4,w2,w4
        const __m256 t3 = _mm256_permute2f128_ps(t1, t2, 0b00100000);//x1,x3,z1,z3;y1,y3,w1,w3
        const __m256 t4 = _mm256_permute2f128_ps(t1, t2, 0b00110001);//x2,x4,z2,z4;y2,y4,w2,w4
        Store256PS(out.xy, _mm256_unpacklo_ps(t3, t4));//x1,x2,x3,x4;y1,y2,y3,y4
        Store256PS(out.zw, _mm256_unpackhi_ps(t3, t4));//z1,z2,z3,z4;w1,w2,w3,w4
    #elif defined (__SSE2__)
        const __m128 xy12 = _mm_movelh_ps(in.x, in.y);//x1,y1,x2,y2
        const __m128 zw12 = _mm_movehl_ps(in.y, in.x);//z1,w1,z2,w2
        const __m128 xy34 = _mm_movelh_ps(in.z, in.w);//x3,y3,x4,y4
        const __m128 zw34 = _mm_movehl_ps(in.w, in.z);//x3,y3,x4,y4
        const auto newx = _mm_shuffle_ps(xy12, xy34, _MM_SHUFFLE(2, 0, 2, 0))/*x1,x2,x3,x4*/;
        const auto newy = _mm_shuffle_ps(xy12, xy34, _MM_SHUFFLE(3, 1, 3, 1))/*y1,y2,y3,y4*/;
        const auto newz = _mm_shuffle_ps(zw12, zw34, _MM_SHUFFLE(2, 0, 2, 0))/*z1,z2,z3,z4*/;
        _mm_store_ps((float*)&out.x, newx);
        _mm_store_ps((float*)&out.y, newy);
        _mm_store_ps((float*)&out.z, newz);
    #else
        if (&in == &out)
        {
            std::swap(out(1, 0), out(0, 1));
            std::swap(out(2, 0), out(0, 2));
            std::swap(out(2, 1), out(1, 2));
        }
        else
        {
            out(0, 0) = in(0, 0), out(1, 1) = in(1, 1), out(2, 2) = in(2, 2);
            float tmp;
            tmp = in(0, 1); out(0, 1) = in(1, 0); out(1, 0) = tmp;
            tmp = in(0, 2); out(0, 2) = in(2, 0); out(2, 0) = tmp;
            tmp = in(1, 2); out(1, 2) = in(2, 1); out(2, 1) = tmp;
        }
    #endif
    }
    static void VECCALL MatrixMultiply3x3(const Mat3x3& left, const Mat3x3& right, Mat3x3& out)
    {
    #if defined(__AVX__)
        const __m256 t1 = _mm256_unpacklo_ps(right.xy, right.zw);//x1,x3,y1,y3;x2,x4,y2,y4
        const __m256 t2 = _mm256_unpackhi_ps(right.xy, right.zw);//z1,z3,w1,w3;z2,z4,w2,w4
        const __m256 t3 = _mm256_permute2f128_ps(t1, t2, 0b00100000);//x1,x3,y1,y3;z1,z3,w1,w3
        const __m256 t4 = _mm256_permute2f128_ps(t1, t2, 0b00110001);//x2,x4,y2,y4;z2,z4,w2,w4
        const __m256 rxz = _mm256_unpacklo_ps(t3, t4);//x1,x2,x3,x4;z1,z2,z3,z4
        const __m256 ryw = _mm256_unpackhi_ps(t3, t4);//y1,y2,y3,y4;w1,w2,w3,w4
        const __m256 rxx = _mm256_permute2f128_ps(rxz, ryw, 0x00); const __m256 ryy = _mm256_permute2f128_ps(rxz, ryw, 0x22);
        const __m256 rzz = _mm256_permute2f128_ps(rxz, ryw, 0x11);

        const __m256 xy1 = _mm256_dp_ps(left.xy, rxx, 0x71), xy2 = _mm256_dp_ps(left.xy, ryy, 0x72);
        const __m256 xy3 = _mm256_dp_ps(left.xy, rzz, 0x74);
        const auto newxy = _mm256_blend_ps(_mm256_blend_ps(xy1, xy2, 0b00100010)/*x1,x2,,;y1,y2,,*/, xy3, 0b11001100);//x1,x2,x3;y1,y2,y3
        Store256PS(out.xy, newxy);
        const __m256 xz3 = _mm256_dp_ps(_mm256_broadcast_ps((const __m128*)&left.z), rxz, 0x75);//z1,,z1;z3,,z3
        const __m128 y3 = _mm_dp_ps(left.z, _mm256_castps256_ps128(ryw), 0x72);
        const auto newz = _mm_blend_ps(_mm_blend_ps(_mm256_castps256_ps128(xz3), y3, 0b1110)/*x3,y3,,*/, _mm256_extractf128_ps(xz3, 1), 0b1100);//z1,z2,z3
        Store128PS(out.z, newz);
    #elif defined(__SSE2__)
        //transpose
        const auto rx = _mm_load_ps((const float*)&right.x);
        const auto ry = _mm_load_ps((const float*)&right.y);
        const auto rz = _mm_load_ps((const float*)&right.z);
        const auto rw = _mm_load_ps((const float*)&right.w);
        const __m128 xy12 = _mm_movelh_ps(rx, ry);//x1,y1,x2,y2
        const __m128 zw12 = _mm_movehl_ps(ry, rx);//z1,w1,z2,w2
        const __m128 xy34 = _mm_movelh_ps(rz, rw);//x3,y3,x4,y4
        const __m128 zw34 = _mm_movehl_ps(rw, rz);//x3,y3,x4,y4
        const auto irx = _mm_shuffle_ps(xy12, xy34, _MM_SHUFFLE(2, 0, 2, 0))/*x1,x2,x3,x4*/;
        const auto iry = _mm_shuffle_ps(xy12, xy34, _MM_SHUFFLE(3, 1, 3, 1))/*y1,y2,y3,y4*/;
        const auto irz = _mm_shuffle_ps(zw12, zw34, _MM_SHUFFLE(2, 0, 2, 0))/*z1,z2,z3,z4*/;
        //multiple
        const auto lx = _mm_load_ps((const float*)&left.x);
        const auto ly = _mm_load_ps((const float*)&left.y);
        const auto lz = _mm_load_ps((const float*)&left.z);
        const __m128 x1 = _mm_dp_ps(lx, irx, 0x71), y1 = _mm_dp_ps(lx, iry, 0x72), z1 = _mm_dp_ps(lx, irz, 0x74);
        const __m128 x2 = _mm_dp_ps(ly, irx, 0x71), y2 = _mm_dp_ps(ly, iry, 0x72), z2 = _mm_dp_ps(ly, irz, 0x74);
        const __m128 x3 = _mm_dp_ps(lz, irx, 0x71), y3 = _mm_dp_ps(lz, iry, 0x72), z3 = _mm_dp_ps(lz, irz, 0x74);
        const auto newx = _mm_blend_ps(_mm_blend_ps(x1, y1, 0b0010)/*x1,y1,,*/, z1, 0b1100)/*x1,y1,z1*/;
        const auto newy = _mm_blend_ps(_mm_blend_ps(x2, y2, 0b0010)/*x2,y2,,*/, z2, 0b1100)/*x2,y2,z2*/;
        const auto newz = _mm_blend_ps(_mm_blend_ps(x3, y3, 0b0010)/*x3,y3,,*/, z3, 0b1100)/*x3,y3,z3*/;
        Store128PS(out.x, newx); Store128PS(out.y, newy); Store128PS(out.z, newz);
    #else
        memset(out.element, 0, sizeof(Mat3x3));
        for (int i = 0; i < 3; i++)
            for (int k = 0; k < 3; k++)
                out(i, k) = left(i, 0) * right(0, k) + left(i, 1) * right(1, k) + left(i, 2) * right(2, k);
    #endif
    }
public:
    using SQMat4Base::x; using SQMat4Base::y; using SQMat4Base::z;

    Mat3x3() noexcept { }
    Mat3x3(const Vec3& x_, const Vec3& y_, const Vec3& z_) noexcept :SQMat4Base(x_, y_, z_) { };
#ifdef __AVX__
    Mat3x3(const __m256 xy_, const __m256 zw_) noexcept :SQMat4Base(xy_, zw_) { };
    Mat3x3(const __m256 xy_, const __m128 z_) noexcept { xy = xy_, z = z_; };
#endif
    Mat3x3(const Vec3 *ptr) noexcept :SQMat4Base(ptr[0], ptr[1], ptr[2]) { };
#ifdef __SSE2__
    Mat3x3(const __m128x3& dat) noexcept
    {
        Store128PS(x, _mm_load_ps((const float*)&dat[0]));
        Store128PS(y, _mm_load_ps((const float*)&dat[1]));
        Store128PS(z, _mm_load_ps((const float*)&dat[2]));
    };
#endif
    template<class T>
    Mat3x3(const T *ptr) noexcept
    {
        for (int32_t a = 0, b = 0; b < 3; ++b)
            for (int32_t c = 0; c < 3; ++c, ++a)
                element[b * 4 + c] = static_cast<float>(ptr[a]);
    }
    Mat3x3(const float(&dat)[12]) noexcept
    {
    #ifdef __AVX__
        Store256PS(xy, _mm256_loadu_ps(dat));
        Store128PS(z, _mm_loadu_ps(dat + 8));
    #elif defined (__SSE2__)
        Store128PS(x, _mm_loadu_ps(dat + 0));
        Store128PS(y, _mm_loadu_ps(dat + 4));
        Store128PS(z, _mm_loadu_ps(dat + 8));
    #else
        memcpy(element, dat, sizeof(float) * 12);
    #endif
    };

    VECCALL operator float*() noexcept { return element; };
    VECCALL operator const float*() const noexcept { return element; };
    

    forceinline Mat3x3 VECCALL inv() const
    {
        Mat3x3 ret;
        MatrixTranspose3x3(*this, ret);
        return ret;
    }

    forceinline Mat3x3 VECCALL operator+(const float right) const
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(right);
        return Mat3x3(_mm256_add_ps(xy, v256), _mm256_add_ps(zw, v256));
    #elif defined (__SSE2__)
        const __m128 v128 = _mm_set1_ps(right);
        return Mat3x3(_mm_add_ps(x, v128), _mm_add_ps(y, v128), _mm_add_ps(z, v128));
    #else
        return Mat3x3(x + right, y + right, z + right);
    #endif
    }

    forceinline Mat3x3 VECCALL operator+(const Mat3x3& right) const
    {
    #ifdef __AVX__
        return Mat3x3(_mm256_add_ps(xy, right.xy), _mm256_add_ps(zw, right.zw));
    #elif defined (__SSE2__)
        return Mat3x3(_mm_add_ps(x, right.x), _mm_add_ps(y, right.y), _mm_add_ps(z, right.z));
    #else
        return Mat3x3(x + right.x, y + right.y, z + right.z);
    #endif
    }

    forceinline Mat3x3 VECCALL operator-(const float right) const
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(right);
        return Mat3x3(_mm256_sub_ps(xy, v256), _mm256_sub_ps(zw, v256));
    #elif defined (__SSE2__)
        const __m128 v128 = _mm_set1_ps(right);
        return Mat3x3(_mm_sub_ps(x, v128), _mm_sub_ps(y, v128), _mm_sub_ps(z, v128));
    #else
        return Mat3x3(x - right, y - right, z - right);
    #endif
    }

    forceinline Mat3x3 VECCALL operator-(const Mat3x3& right) const
    {
    #ifdef __AVX__
        return Mat3x3(_mm256_sub_ps(xy, right.xy), _mm256_sub_ps(zw, right.zw));
    #else
        return Mat3x3(x - right.x, y - right.y, z - right.z);
    #endif
    }

    forceinline Mat3x3 VECCALL operator*(const float right) const
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(right);
        return Mat3x3(_mm256_mul_ps(xy, v256), _mm256_mul_ps(zw, v256));
    #else
        return Mat3x3(x * right, y * right, z * right);
    #endif
    }

    forceinline Vec3 VECCALL operator*(const Vec3& right) const
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_broadcast_ps((const __m128*)&right);
        const __m256 ans = _mm256_blend_ps(
            _mm256_dp_ps(xy, v256, 0x77),
            _mm256_dp_ps(zw, v256, 0x77),
            0b11001100)/*xxzz,yyww*/;
        return _mm_blend_ps(_mm256_castps256_ps128(ans), _mm256_extractf128_ps(ans, 1), 0b1010)/*xyz0*/;
    #else
        return Vec3(x.dot(right), y.dot(right), z.dot(right));
    #endif
    }

    forceinline Mat3x3 VECCALL operator*(const Mat3x3& right) const
    {
        Mat3x3 ret;
        MatrixMultiply3x3(*this, right, ret);
        return ret;
    }

    forceinline Mat3x3 VECCALL operator/(const float right) const
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(right);
        return Mat3x3(_mm256_div_ps(xy, v256), _mm_div_ps(z, _mm256_castps256_ps128(v256)));
    #else
        return Mat3x3(x / right, y / right, z / right);
    #endif
    }


    forceinline Mat3x3& VECCALL inved()
    {
        MatrixTranspose3x3(*this, *this);
        return *this;
    }

    forceinline Mat3x3& VECCALL operator+=(const float n)
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(n);
        const auto newxy = _mm256_add_ps(xy, v256), newzw = _mm256_add_ps(zw, v256);
        Store256PS(xy, newxy); Store256PS(zw, newzw);
    #elif defined (__SSE2__)
        const __m128 v128 = _mm_set1_ps(n);
        const auto newx = _mm_add_ps(_mm_load_ps((const float*)&x), v128), newy = _mm_add_ps(_mm_load_ps((const float*)&y), v128), newz = _mm_add_ps(_mm_load_ps((const float*)&z), v128);
        Store128PS(x, newx); Store128PS(y, newy); Store128PS(z, newz);
    #else
        x += n, y += n, z += n;
    #endif
        return *this;
    }

    forceinline Mat3x3& VECCALL operator+=(const Mat3x3& m)
    {
    #ifdef __AVX__
        const auto newxy = _mm256_add_ps(xy, m.xy);
        const auto newzw = _mm256_add_ps(zw, m.zw);
        Store256PS(xy, newxy); Store256PS(zw, newzw);
    #else
        x += m.x, y += m.y, z += m.z;
    #endif
        return *this;
    }

    forceinline Mat3x3& VECCALL operator-=(const float n)
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(n);
        const auto newxy = _mm256_sub_ps(xy, v256), newzw = _mm256_sub_ps(zw, v256);
        Store256PS(xy, newxy); Store256PS(zw, newzw);
    #elif defined (__SSE2__)
        const __m128 v128 = _mm_set1_ps(n);
        const auto newx = _mm_sub_ps(x, v128), newy = _mm_sub_ps(y, v128), newz = _mm_sub_ps(z, v128);
        Store128PS(x, newx); Store128PS(y, newy); Store128PS(z, newz);
    #else
        x -= n, y -= n, z -= n;
    #endif
        return *this;
    }

    forceinline Mat3x3& VECCALL operator-=(const Mat3x3& m)
    {
    #ifdef __AVX__
        const auto newxy = _mm256_sub_ps(xy, m.xy);
        const auto newzw = _mm256_sub_ps(zw, m.zw);
        Store256PS(xy, newxy); Store256PS(zw, newzw);
    #else
        x -= m.x, y -= m.y, z -= m.z;
    #endif
        return *this;
    }

    forceinline Mat3x3& VECCALL operator*=(const float n)
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(n);
        const auto newxy = _mm256_mul_ps(xy, v256), newzw = _mm256_mul_ps(zw, v256);
        Store256PS(xy, newxy); Store256PS(zw, newzw);
    #elif defined (__SSE2__)
        const __m128 v128 = _mm_set1_ps(n);
        const auto newx = _mm_mul_ps(x, v128), newy = _mm_mul_ps(y, v128), newz = _mm_mul_ps(z, v128);
        Store128PS(x, newx); Store128PS(y, newy); Store128PS(z, newz);
    #else
        x *= n, y *= n, z *= n;
    #endif
        return *this;
    }

    forceinline Mat3x3& VECCALL operator*=(const Mat3x3& m)
    {
        MatrixMultiply3x3(*this, m, *this);
        return *this;
    }

    forceinline Mat3x3& VECCALL operator/=(const float n)
    {
    #ifdef __AVX__
        const __m256 v256 = _mm256_set1_ps(n);
        xy = _mm256_div_ps(xy, v256), z = _mm_div_ps(z, _mm256_castps256_ps128(v256));
    #else
        (*this) *= (1 / n);
    #endif
        return *this;
    }


    static Mat3x3 VECCALL identity()
    {
        static constexpr float dat[12] =
        { 1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1, 0 };
        return Mat3x3(dat);
    }

    static Mat3x3 VECCALL zero()
    {
    #ifdef __AVX__
        const auto tmp = _mm256_setzero_ps();
        return Mat3x3(tmp, tmp);
    #elif defined(__SSE2__)
        const auto tmp = _mm_setzero_ps();
        return Mat3x3(tmp, tmp, tmp);
    #else
        Mat3x3 ret;
        memset(&ret, 0x0, sizeof(Mat3x3));
        return ret;
    #endif
    }

    static Mat3x3 VECCALL one()
    {
    #ifdef __AVX__
        const auto tmp = _mm256_set1_ps(1);
        return Mat3x3(tmp, tmp);
    #elif defined(__SSE2__)
        const auto tmp = _mm_set1_ps(1);
        return Mat3x3(tmp, tmp, tmp);
    #else
        static constexpr float dat[12] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        return Mat3x3(dat);
    #endif
    }

    static Mat3x3 VECCALL all(const float val)
    {
    #ifdef __AVX__
        const auto tmp = _mm256_set1_ps(val);
        return Mat3x3(tmp, tmp);
    #elif defined(__SSE2__)
        const auto tmp = _mm_set1_ps(val);
        return Mat3x3(tmp, tmp, tmp);
    #else
        float dat[12] = { val };
        return Mat3x3(dat);
    #endif
    }

};


forceinline Mat3x3 VECCALL operator+(const float left, const Mat3x3& right)
{
    return right + left;
}

forceinline Mat3x3 VECCALL operator-(const float left, const Mat3x3& right)
{
    return Mat3x3::all(left) - right;
}

forceinline Mat3x3 VECCALL operator*(const float left, const Mat3x3& right)
{
    return right*left;
}

forceinline Vec3 VECCALL operator*(const Vec3& left, const Mat3x3& right)
{
    return right.inv()*left;
}

forceinline Vec3& VECCALL operator*=(Vec3& left, const Mat3x3& right)
{
    left = right.inv()*left;
    return left;
}

}