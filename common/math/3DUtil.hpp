#include "../CommonRely.hpp"
#include <cmath>

namespace common::math
{
inline constexpr float  PI_float  = (float) (3.14159265358979323846);
inline constexpr double PI_double = (double)(3.14159265358979323846);

template<typename T>
inline constexpr T Ang2Rad(T in) noexcept
{
    return static_cast<T>(in * static_cast<T>(3.14159265358979323846 / 180));
}

template<typename T>
inline constexpr T Rad2Ang(T in) noexcept
{
    return static_cast<T>(in * static_cast<T>(180 / 3.14159265358979323846));
}

template<typename T, typename U>
inline constexpr T ToHomoCoord(const U& mat) noexcept
{
    return
    {
        {mat.X, 0.f},
        {mat.Y, 0.f},
        {mat.Z, 0.f},
        {0.f, 0.f, 0.f, 1.f},
    };
}

//Vec4's xyz define axis, w define angle(in radius)
template<typename T, typename V>
inline constexpr T RotateMat(const V& rv) noexcept
{
    using U = typename T::VecType;
    const auto sqr = rv * rv;
    const float rad = rv.W;
    const float sinx = std::sin(rad), cosx = std::cos(rad);
    const float OMcosx = 1 - cosx;
    /*  (1-cos)x^2+cos  xy(1-cos)-zsin  xz(1-cos)+ysin
     *  xy(1-cos)+zsin  (1-cos)y^2+cos  yz(1-cos)-xsin
     *  xz(1-cos)-ysin  yz(1-cos)+xsin  (1-cos)z^2+cos
     **/
    return 
    {
        {sqr.X * OMcosx + cosx, rv.X * rv.Y * OMcosx - rv.Z * sinx, rv.X * rv.Z * OMcosx + rv.Y * sinx},
        {rv.X * rv.Y * OMcosx + rv.Z * sinx, sqr.Y * OMcosx + cosx, rv.Y * rv.z * OMcosx - rv.X * sinx},
        {rv.X * rv.Z * OMcosx - rv.Y * sinx, rv.Y * rv.Z * OMcosx + rv.X * sinx, sqr.Z * OMcosx + cosx}
    };
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

template<typename T>
inline constexpr T RotateMatX(const float rad) noexcept
{
    const float sinx = std::sin(rad), cosx = std::cos(rad);
    return
    {
        {1.f,  0.f,   0.f},
        {0.f, cosx, -sinx},
        {0.f, sinx,  cosx}
    };
}
template<typename T>
inline constexpr T RotateMatY(const float rad) noexcept
{
    const float siny = std::sin(rad), cosy = std::cos(rad);
    return
    {
        { cosy, 0.f, siny},
        {  0.f, 1.f,  0.f},
        {-siny, 0.f, cosy}
    };
}
template<typename T>
inline constexpr T RotateMatZ(const float rad) noexcept
{
    const float sinz = std::sin(rad), cosz = std::cos(rad);
    return
    {
        {cosz, -sinz, 0.f},
        {sinz,  cosz, 0.f},
        { 0.f,   0.f, 1.f}
    };
}

template<typename T>
inline constexpr T RotateMatXYZ(const typename T::VecType& rrads) noexcept
{
    if (rrads.X != 0.0f)
    {
        /*Mat3x3 rmat = RotateMat(Vec4(1.0f, 0.0f, 0.0f, rrads.x));
        if (rrads.y != 0.0f)
            rmat *= RotateMat(Vec4(0.0f, 1.0f, 0.0f, rrads.y));
        if (rrads.z != 0.0f)
            rmat *= RotateMat(Vec4(0.0f, 0.0f, 1.0f, rrads.z));*/
        T rmat = RotateMatX<T>(rrads.X);
        if (rrads.Y != 0.0f)
            rmat *= RotateMatY<T>(rrads.Y);
        if (rrads.Z != 0.0f)
            rmat *= RotateMatZ<T>(rrads.Z);
        return rmat;
    }
    else if (rrads.Y != 0.0f)
    {
        /*Mat3x3 rmat = RotateMat(Vec4(0.0f, 1.0f, 0.0f, rrads.y));
        if (rrads.z != 0.0f)
            rmat *= RotateMat(Vec4(0.0f, 0.0f, 1.0f, rrads.z));*/
        T rmat = RotateMatY<T>(rrads.Y);
        if (rrads.Z != 0.0f)
            rmat *= RotateMatZ<T>(rrads.Z);
        return rmat;
    }
    else if (rrads.Z != 0.0f)
    {
        return RotateMatZ<T>(rrads.Z);
    }
    else
        return T::Identity();
}

template<typename T>
inline constexpr T ScaleMat(const typename T::VecType& sv) noexcept
{
    auto sMat = T::Zeros();
    sMat.X.X = sv.X, sMat.Y.Y = sv.Y, sMat.Z.Z = sv.Z;
    return sMat;
}

/*pure translate(translata DOT identity-rotate)*/
template<typename T, typename V>
inline constexpr T TranslateMat(const V& tv) noexcept
{
    T idmat = T::Identity();
    idmat.X.W = tv.X, idmat.Y.W = tv.Y, idmat.Z.W = tv.Z;
    return idmat;
}
/*rotate DOT translate*/
template<typename T, typename V, typename M>
inline constexpr T TranslateMat(const V& tv, const M& rMat) noexcept
{
    const auto rtv = rMat * tv;
    auto tMat = ToHomoCoord<T>(rMat);
    tMat.X.W = rtv.X, tMat.Y.W = rtv.Y, tMat.Z.W = rtv.Z;
    return tMat;
}

}
