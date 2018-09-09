#pragma once

#include <cmath>
#include <algorithm>
#include "3DBasic/miniBLAS.hpp"
#include "half/half.hpp"


namespace b3d
{

constexpr float PI_float = (float)(3.14159265358979323846);
constexpr double PI_double = (double)(3.14159265358979323846);

template<typename T>
inline T ang2rad(T in)
{
    return static_cast<T>(in * PI_double / 180);
}

template<typename T>
inline T rad2ang(T in)
{
    return static_cast<T>(in * 180 / PI_double);
}

using half_float::half;

class Coord2D
{
public:
    float u, v;
    Coord2D() noexcept { u = v = 0.0f; };
    template<class T>
    Coord2D(const T& u_, const T& v_) noexcept :u(static_cast<float>(u_)), v(static_cast<float>(v_)) { }

    Coord2D operator+(const Coord2D &c) const
    {
        return Coord2D(u + c.u, v + c.v);
    }
    Coord2D operator*(const float &n) const
    {
        return Coord2D(u*n, v*n);
    }
    Coord2D repos(const float scalex, const float scaley, const float offsetx, const float offsety)
    {
        return Coord2D(u*scalex + offsetx, v*scaley + offsety);
    }
    void regulized_repeat()
    {
        float intpart;
        u = std::modf(u, &intpart);
        if (u < 0)
            u += 1.0f;
        v = std::modf(v, &intpart);
        if (v < 0)
            v += 1.0f;
    }
    void regulized_mirror()
    {
        float intpart;
        u = std::abs(std::modf(u, &intpart));
        v = std::abs(std::modf(v, &intpart));
    }
    operator float*() noexcept { return &u; };
    operator const float*() const noexcept { return &u; };
};


class alignas(16) Vec3 : public miniBLAS::Vec3
{
public:
    static Vec3 Vec3_PI() { return Vec3(PI_float, PI_float, PI_float); }
    static Vec3 Vec3_2PI() { return Vec3(PI_float * 2, PI_float * 2, PI_float * 2); }
    using miniBLAS::Vec3::Vec3;
    Vec3() noexcept : miniBLAS::Vec3(true) { }
    Vec3(const miniBLAS::Vec3& v) noexcept : miniBLAS::Vec3(v) { }
    void RepeatClampPos(const Vec3& range)
    {
        const auto dived = *this / range;
    #if COMMON_SIMD_LV >= 41
        const auto rounded = _mm_round_ps(dived, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
    #  if COMMON_SIMD_LV >= 200
        *this = _mm_fnmadd_ps(rounded, range, *this);
    #  else
        *this = _mm_sub_ps(*this, _mm_mul_ps(rounded, range));
    #  endif
    #else
        this->x -= std::floor(dived.x)*range.x;
        this->y -= std::floor(dived.y)*range.y;
        this->z -= std::floor(dived.z)*range.z;
    #endif
    }
};


class alignas(16) Vec4 : public miniBLAS::Vec4
{
public:
    static Vec4 Vec4_PI() { return Vec4(PI_float, PI_float, PI_float, PI_float); }
    static Vec4 Vec4_2PI() { return Vec4(PI_float * 2, PI_float * 2, PI_float * 2, PI_float * 2); }
    using miniBLAS::Vec4::Vec4;
    Vec4() noexcept : miniBLAS::Vec4(true) { }
    Vec4(const miniBLAS::Vec4& v) noexcept : miniBLAS::Vec4(v) { }

    operator Vec3& () noexcept { return *(Vec3*)this; }
    operator const Vec3& () const noexcept { return *(const Vec3*)this; }

    void RepeatClampPos(const Vec4& range)
    {
        const auto dived = *this / range;
    #if COMMON_SIMD_LV >= 41
        const auto rounded = _mm_round_ps(dived, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
    #  if COMMON_SIMD_LV >= 200
        * this = _mm_fnmadd_ps(rounded, range, *this);
    #  else
        *this = _mm_sub_ps(*this, _mm_mul_ps(rounded, range));
    #  endif
    #else
        this->x -= std::floor(dived.x)*range.x;
        this->y -= std::floor(dived.y)*range.y;
        this->z -= std::floor(dived.z)*range.z;
        this->w -= std::floor(dived.w)*range.w;
    #endif
    }
};


class alignas(16) Normal : public Vec3
{
public:
    Normal() noexcept : Vec3() { };
    Normal(const Vec3& v) noexcept : Vec3(v.normalize()) { }
    template<class T>
    Normal(const T& ix, const T& iy, const T& iz) noexcept :Vec3(ix, iy, iz) { normalized(); }

    Normal& operator=(const Vec3& v)
    {
        *(Vec3*)this = v.normalize();
        return *this;
    }
};


class alignas(32) Mat3x3 : public miniBLAS::Mat3x3
{
public:
    using miniBLAS::Mat3x3::element;
    using miniBLAS::Mat3x3::Mat3x3;
    Mat3x3() noexcept : miniBLAS::Mat3x3() { }
    Mat3x3(const miniBLAS::Mat3x3& m) noexcept : miniBLAS::Mat3x3(m) { }
    //Vec4's xyz define axis, w define angle(in radius)
    static Mat3x3 RotateMat(const Vec4& rv)
    {
        const auto sqr = rv * rv;
        const float rad = rv.w;
        const float sinx = std::sin(rad), cosx = std::cos(rad);
        const float OMcosx = 1 - cosx;
        /*  (1-cos)x^2+cos  xy(1-cos)-zsin  xz(1-cos)+ysin
         *  xy(1-cos)+zsin  (1-cos)y^2+cos  yz(1-cos)-xsin
         *  xz(1-cos)-ysin  yz(1-cos)+xsin  (1-cos)z^2+cos
         **/
        return Mat3x3(
            Vec3(sqr.x*OMcosx + cosx, rv.x*rv.y*OMcosx - rv.z*sinx, rv.x*rv.z*OMcosx + rv.y*sinx),
            Vec3(rv.x*rv.y*OMcosx + rv.z*sinx, sqr.y*OMcosx + cosx, rv.y*rv.z*OMcosx - rv.x*sinx),
            Vec3(rv.x*rv.z*OMcosx - rv.y*sinx, rv.y*rv.z*OMcosx + rv.x*sinx, sqr.z*OMcosx + cosx));
        /*  Rotate x-axis
         *  1         0      0
         *  0      cosx  -sinx
         *  0      sinx   cosx
         *  Rotate y-axis
         *   cosy      0  siny
         *      0      1     0
         *  -siny      0  cosy
         *  Rotate z-axis
         *  cosz  -sinz      0
         *  sinz   cosz      0
         *     0      0      1
         **/
    }
    static Mat3x3 RotateMatXYZ(const Vec3& rrads)
    {
        if (rrads.x != 0.0f)
        {
            Mat3x3 rmat = RotateMat(Vec4(1.0f, 0.0f, 0.0f, rrads.x));
            if (rrads.y != 0.0f)
                rmat *= RotateMat(Vec4(0.0f, 1.0f, 0.0f, rrads.y));
            if (rrads.z != 0.0f)
                rmat *= RotateMat(Vec4(0.0f, 0.0f, 1.0f, rrads.z));
            return rmat;
        }
        else if (rrads.y != 0.0f)
        {
            Mat3x3 rmat = RotateMat(Vec4(0.0f, 1.0f, 0.0f, rrads.y));
            if (rrads.z != 0.0f)
                rmat *= RotateMat(Vec4(0.0f, 0.0f, 1.0f, rrads.z));
            return rmat;
        }
        else if (rrads.z != 0.0f)
        {
            return RotateMat(Vec4(0.0f, 0.0f, 1.0f, rrads.z));
        }
        else
            return Mat3x3::identity();
    }
    static Mat3x3 ScaleMat(const Vec3& sv)
    {
        auto sMat = Mat3x3::zero();
        sMat.x.x = sv.x, sMat.y.y = sv.y, sMat.z.z = sv.z;
        return sMat;
    }
};


class alignas(32) Mat4x4 : public miniBLAS::Mat4x4
{
public:
    using miniBLAS::Mat4x4::element;
    using miniBLAS::Mat4x4::Mat4x4;
    Mat4x4() noexcept : miniBLAS::Mat4x4() { }
    Mat4x4(const miniBLAS::Mat3x3& m) noexcept : miniBLAS::Mat4x4(m, true) { }
    Mat4x4(const miniBLAS::Mat4x4& m) noexcept : miniBLAS::Mat4x4(m) { }
    explicit VECCALL operator Mat3x3&() noexcept { return *(Mat3x3*)this; }
    explicit VECCALL operator const Mat3x3&() const noexcept { return *(Mat3x3*)this; }
    /*pure translate(translata DOT identity-rotate)*/
    static Mat4x4 TranslateMat(const Vec3& tv)
    {
        static Mat4x4 idmat = miniBLAS::Mat4x4::identity();
        return TranslateMat(idmat, tv);
    }
    /*translate DOT rotate*/
    static Mat4x4 TranslateMat(const Mat4x4& rMat, const Vec3& tv)
    {
        Mat4x4 tMat(rMat);
        tMat.x.w = tv.x, tMat.y.w = tv.y, tMat.z.w = tv.z;
        return tMat;
    }
    /*rotate DOT translate*/
    static Mat4x4 TranslateMat(const Vec3& tv, const Mat3x3& rMat)
    {
        const auto rtv = rMat * tv;
        Mat4x4 tMat(rMat);
        tMat.x.w = rtv.x, tMat.y.w = rtv.y, tMat.z.w = rtv.z;
        return tMat;
    }
};


inline float mod(const float &l, const float &r)
{
    float t, e;
    std::modf(l / r, &t);
    e = t*r;
    return l - e;
}



}