#pragma once
#include "MatBase.hpp"
#include "VecSIMD.hpp"
#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 200
#   define XCOMP_HAS_SIMD256 1
#   include "../simd/SIMD256.hpp"
#endif
#include <cmath>

namespace common::math::simd
{

namespace simds
{

#ifdef XCOMP_HAS_SIMD256
template<typename T> struct V8SimdConv;
template<> struct V8SimdConv<float>    { using T = COMMON_SIMD_NAMESPACE::F32x8; };
template<> struct V8SimdConv<int32_t>  { using T = COMMON_SIMD_NAMESPACE::I32x8; };
template<> struct V8SimdConv<uint32_t> { using T = COMMON_SIMD_NAMESPACE::U32x8; };
template<typename T> using V8SIMD = typename V8SimdConv<T>::T;
#endif

}

namespace mat
{

#if COMMON_COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wnested-anon-types"
#   pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#elif COMMON_COMPILER_GCC || COMMON_COMPILER_ICC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wpedantic"
#elif COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4201)
#endif
/*a vector contains 4x4 element(int32 or float)*/
template<typename T, typename V>
class alignas(32) Mat4x4Base : public shared::Storage<sizeof(T), 16>
{
    static_assert(sizeof(T) == 4,  "only  4-byte length type allowed");
    static_assert(sizeof(V) == 16, "only 16-byte length vec type allowed");
    static_assert(std::is_same_v<T, float>, "current only support float");
protected:
    using SIMD4Type = simds::V4SIMD<T>;
#ifdef XCOMP_HAS_SIMD256
    using SIMD8Type = simds::V8SIMD<T>;
#endif
    union
    {
#ifdef XCOMP_HAS_SIMD256
        struct
        {
            SIMD8Type XY, ZW;
        };
#endif
        struct
        {
            SIMD4Type RowX, RowY, RowZ, RowW;
        };
        struct
        {
            V X, Y, Z, W;
        };
    };
    constexpr Mat4x4Base(const V& x, const V& y, const V& z, const V& w) noexcept : X(x), Y(y), Z(z), W(w) {}
    forceinline static constexpr Mat4x4Base<T, V> LoadAll(const T* ptr) noexcept
    {
#ifdef XCOMP_HAS_SIMD256
        return { SIMD8Type(ptr), SIMD8Type(ptr + 32) };
#else
        return { SIMD4Type(ptr), SIMD4Type(ptr + 16), SIMD4Type(ptr + 32), SIMD4Type(ptr + 48) };
#endif
    }
public:
    constexpr Mat4x4Base() noexcept { }
#ifdef XCOMP_HAS_SIMD256
    constexpr Mat4x4Base(const SIMD8Type& xy, const SIMD8Type& zw) noexcept : XY(xy), ZW(zw) {}
#endif
    //constexpr Mat4x4Base(const SIMD4Type& x, const SIMD4Type& y, const SIMD4Type& z, const SIMD4Type& w) noexcept : X(x), Y(y), Z(z), W(w) {}

    template<typename M, typename = std::enable_if_t<std::is_base_of_v<shared::Storage<sizeof(T), 16>, M>>>
    forceinline M& As() noexcept
    {
        return *reinterpret_cast<M*>(this);
    }
    template<typename M, typename = std::enable_if_t<std::is_base_of_v<shared::Storage<sizeof(T), 16>, M>>>
    forceinline const M& As() const noexcept
    {
        return *reinterpret_cast<const M*>(this);
    }

    forceinline constexpr const V& operator[](size_t idx) const noexcept { return (&X)[idx]; }
    forceinline constexpr       V& operator[](size_t idx)       noexcept { return (&X)[idx]; }
    forceinline constexpr const T& operator()(size_t row, size_t col) const noexcept { return (&X)[row][col]; }
    forceinline constexpr       T& operator()(size_t row, size_t col)       noexcept { return (&X)[row][col]; }
    forceinline constexpr const T* Ptr() const noexcept { return &X.X; }
    forceinline constexpr       T* Ptr()       noexcept { return &X.X; }
    forceinline constexpr void SaveAll(T* ptr) const noexcept
    {
#ifdef XCOMP_HAS_SIMD256
        XY.Save(ptr); ZW.Save(ptr + 32);
#else
        RowX.Save(ptr); RowY.Save(ptr + 16); RowZ.Save(ptr + 32); RowW.Save(ptr + 48);
#endif
    }
};
#if COMMON_COMPILER_CLANG
#   pragma clang diagnostic pop
#elif COMMON_COMPILER_GCC || COMMON_COMPILER_ICC
#   pragma GCC diagnostic pop
#elif COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


template<typename E, size_t N, typename V, typename T>
struct MatBasic
{
#ifdef XCOMP_HAS_SIMD256
    using T8 = simds::V8SIMD<E>;
#endif
    using T4 = simds::V4SIMD<E>;
    forceinline constexpr void Save(E* ptr) const noexcept
    {
        auto& self = *static_cast<const T*>(this);
        if constexpr (N == 4)
#ifdef XCOMP_HAS_SIMD256
            self.XY.Save(ptr), self.ZW.Save(ptr + 8);
#else
            self.X.Save(ptr), self.Y.Save(ptr + 4), self.Z.Save(ptr + 8), self.W.Save(ptr + 12);
#endif
        else if constexpr (N == 3)
            self.X.Save(ptr), self.Y.Save(ptr + 3), self.Z.Save(ptr + 6);
        else if constexpr (N == 2)
            self.X.Save(ptr), self.Y.Save(ptr + 2);
    }

    forceinline static constexpr T Load(const E* ptr) noexcept
    {
        if constexpr (N == 4)
#ifdef XCOMP_HAS_SIMD256
            return T{ T8::Load(ptr), T8::Load(ptr + 8) };
#else
            return T{ V::Load(ptr), V::Load(ptr + 4), V::Load(ptr + 8), V::Load(ptr + 12) };
#endif
        else if constexpr (N == 3)
            return T{ V::Load(ptr), V::Load(ptr + 3), V::Load(ptr + 6) };
        else if constexpr (N == 2)
            return T{ V::Load(ptr), V::Load(ptr + 2) };
    }
    forceinline static constexpr T Zeros() noexcept
    {
        if constexpr (N == 4)
#ifdef XCOMP_HAS_SIMD256
            return T{ T8::Zeros(), T8::Zeros() };
#else
            return T{ V::Zeros(), V::Zeros(), V::Zeros(), V::Zeros() };
#endif
        else if constexpr (N == 3)
            return T{ V::Zeros(), V::Zeros(), V::Zeros() };
        else if constexpr (N == 2)
            return T{ V::Zeros(), V::Zeros() };
    }
    forceinline static constexpr T Ones() noexcept
    {
        if constexpr (N == 4)
#ifdef XCOMP_HAS_SIMD256
            return T{ T8::Ones(), T8::Ones() };
#else
            return T{ V::Ones(), V::Ones(), V::Ones(), V::Ones() };
#endif
        else if constexpr (N == 3)
            return T{ V::Ones(), V::Ones(), V::Ones() };
        else if constexpr (N == 2)
            return T{ V::Ones(), V::Ones() };
    }
    forceinline static constexpr T Identity() noexcept
    {
        constexpr E y = 1, n = 0;
        if constexpr (N == 4)
            return T{ {y,n,n,n}, {n,y,n,n}, {n,n,y,n}, {n,n,n,y} };
        else if constexpr (N == 3)
            return T{ {y,n,n}, {n,y,n}, {n,n,y} };
        else if constexpr (N == 2)
            return T{ {y,n}, {n,y} };
    }

    forceinline friend constexpr T Transpose(const T& self) noexcept
    {
#ifdef XCOMP_HAS_SIMD256 // COMMON_ARCH_X86
        const auto xzyw12 = self.XY.template ShuffleLane<0, 2, 1, 3>(); // x1,z1,y1,w1;x2,z2,y2,w2
        const auto xzyw34 = self.ZW.template ShuffleLane<0, 2, 1, 3>(); // x3,z3,y3,w3;x4,z4,y4,w4
        const auto xz1324 = xzyw12.ZipLoLane(xzyw34); // x1,x3,z1,z3;x2,x4,z2,z4
        const auto yw1324 = xzyw12.ZipHiLane(xzyw34); // y1,y3,w1,w3;y2,y4,w2,w4
        const auto xzyw13 = xz1324.template PermuteLane<0, 2>(yw1324); // x1,x3,z1,z3;y1,y3,w1,w3
        const auto xzyw24 = xz1324.template PermuteLane<1, 3>(yw1324); // x2,x4,z2,z4;y2,y4,w2,w4
        return { xzyw13.ZipLoLane(xzyw24), xzyw13.ZipHiLane(xzyw24) };
#elif COMMON_ARCH_X86
        const auto xy12 = self.RowX.ZipLo(self.RowY); // x1,x2,y1,y2
        const auto zw12 = self.RowX.ZipHi(self.RowY); // z1,z2,w1,w2
        const auto xy34 = self.RowZ.ZipLo(self.RowW); // x3,x4,y3,y4
        const auto zw34 = self.RowZ.ZipHi(self.RowW); // z3,z4,w3,w4
        T4 x = _mm_movelh_ps(xy12.Data, xy34.Data);
        T4 y = _mm_movehl_ps(xy12.Data, xy34.Data);
        T4 z = _mm_movelh_ps(zw12.Data, zw34.Data);
        if constexpr (N == 4)
        {
            T4 w = _mm_movehl_ps(zw12.Data, zw34.Data);
            return { x,y,z,w };
        }
        else
            return { x,y,z };
#else // COMMON_ARCH_ARM
        const auto mat4 = vld4q_f32(&self.X.X);
        if constexpr (N == 4)
            return { T4{mat4.val[0]}, T4{mat4.val[1]}, T4{mat4.val[2]}, T4{mat4.val[3]} };
        else
            return { T4{mat4.val[0]}, T4{mat4.val[1]}, T4{mat4.val[2]} };
#endif
    }
};


#ifdef XCOMP_HAS_SIMD256
template<typename E, size_t N, typename V, typename T>
struct FuncSAddSubMulDiv
{
    using T8 = simds::V8SIMD<E>;
    forceinline friend constexpr T operator+(const T& left, const E right) noexcept
    {
        return { left.XY.Add(right), left.ZW.Add(right) };
    }
    forceinline friend constexpr T operator+(const E left, const T& right) noexcept
    {
        return { right.XY.Add(left), right.ZW.Add(left) };
    }
    forceinline friend constexpr T operator-(const T& left, const E right) noexcept
    {
        return { left.XY.Sub(right), left.ZW.Sub(right) };
    }
    forceinline friend constexpr T operator-(const E left, const T& right) noexcept
    {
        T8 left_(left);
        return { left_.Sub(right.XY), left_.Sub(right.ZW) };
    }
    forceinline friend constexpr T operator*(const T& left, const E right) noexcept
    {
        if constexpr (std::is_floating_point_v<E>)
            return { left.XY.Mul(right), left.ZW.Mul(right) };
        else
            return { left.XY.MulLo(right), left.ZW.MulLo(right) };
    }
    forceinline friend constexpr T operator*(const E left, const T& right) noexcept
    {
        if constexpr (std::is_floating_point_v<E>)
            return { right.XY.Mul(left), right.ZW.Mul(left) };
        else
            return { right.XY.MulLo(left), right.ZW.MulLo(left) };
    }
    forceinline friend constexpr T operator/(const T& left, const E right) noexcept
    {
        if constexpr (std::is_floating_point_v<E>)
        {
            return left * (static_cast<E>(1) / right);
        }
        else
        {
            if constexpr (N == 4)
                return { left.X / right, left.Y / right, left.Z / right, left.W / right };
            else if constexpr (N == 3)
                return { left.X / right, left.Y / right, left.Z / right };
        }
    }
    forceinline friend constexpr T operator/(const E left, const T& right) noexcept
    {
        if constexpr (std::is_floating_point_v<E>)
            return { right.XY.Rcp().Mul(left), right.ZW.Rcp().Mul(left) };
        else
            return T{ left } / right;
    }
};

template<typename E, size_t N, typename V, typename T>
struct FuncVAddSubMulDiv
{
    using T8 = simds::V8SIMD<E>;
    forceinline friend constexpr V operator*(const T& left, const V& right) noexcept
    {
        using common::simd::DotPos;
        const auto r2 = T8::BroadcastLane(right.Data);
        if constexpr (N == 4)
        {
            const auto xy = left.XY.template Dot2<DotPos::XYZW, DotPos::XYZW>(r2);
            const auto zw = left.ZW.template Dot2<DotPos::XYZW, DotPos::XYZW>(r2);
            const auto xzyw = xy.template SelectWith<0b11001100>(zw);
            return xzyw.GetLoLane().template SelectWith<0b1010>(xzyw.GetHiLane());
        }
        else if constexpr (N == 3)
        {
            const auto xy = left.XY.template Dot2<DotPos::XYZ, DotPos::XY>(r2);
            const auto z  = left.ZW.GetLoLane().template Dot<DotPos::XYZ, DotPos::Z>(right.Data);
            const auto xxz = xy.GetLoLane().template SelectWith<0b0100>(z);
            return xxz.template SelectWith<0b0010>(xy.GetHiLane());
        }
    }
    forceinline friend constexpr V operator*(const V& left, const T& right) noexcept
    {
        if constexpr (N == 4)
        {
            const auto lxy = T8::Combine(left.Data.template Broadcast<0>(), left.Data.template Broadcast<1>());
            const auto lzw = T8::Combine(left.Data.template Broadcast<2>(), left.Data.template Broadcast<3>());
            const auto xzyw = right.XY.MulAdd(lxy, right.ZW.Mul(lzw));
            return xzyw.GetLoLane().Add(xzyw.GetHiLane());
        }
        else if constexpr (N == 3)
        {
            const auto x = right.RowX.Mul(left.Data.template Broadcast<0>());
            const auto xy = right.RowY.MulAdd(left.Data.template Broadcast<1>(), x);
            return right.RowZ.MulAdd(left.Data.template Broadcast<2>(), xy);
        }
    }
    forceinline friend constexpr T EleWiseAdd(const T& left, const V& right) noexcept
    {
        const auto r2 = T8::BroadcastLane(right.Data);
        return { left.XY.Add(r2), left.ZW.Add(r2) };
    }
    forceinline friend constexpr T EleWiseAdd(const V& left, const T& right) noexcept
    {
        const auto l2 = T8::BroadcastLane(left.Data);
        return { right.XY.Add(l2), right.ZW.Add(l2) };
    }
    forceinline friend constexpr T EleWiseSub(const T& left, const V& right) noexcept
    {
        const auto r2 = T8::BroadcastLane(right.Data);
        return { left.XY.Sub(r2), left.ZW.Sub(r2) };
    }
    forceinline friend constexpr T EleWiseSub(const V& left, const T& right) noexcept
    {
        const auto l2 = T8::BroadcastLane(left.Data);
        return { l2.Sub(right.XY), l2.Sub(right.ZW) };
    }
    forceinline friend constexpr T EleWiseMul(const T& left, const V& right) noexcept
    {
        const auto r2 = T8::BroadcastLane(right.Data);
        return { left.XY.Mul(r2), left.ZW.Mul(r2) };
    }
    forceinline friend constexpr T EleWiseMul(const V& left, const T& right) noexcept
    {
        const auto l2 = T8::BroadcastLane(left.Data);
        return { right.XY.Mul(l2), right.ZW.Mul(l2) };
    }
    forceinline friend constexpr T EleWiseDiv(const T& left, const V& right) noexcept
    {
        return EleWiseMul(left, V{ right.Data.Rcp() });
    }
    forceinline friend constexpr T EleWiseDiv(const V& left, const T& right) noexcept
    {
        return EleWiseMul(T{ right.XY.Rcp(), right.ZW.Rcp() }, left);
    }
};

template<typename E, size_t N, typename V, typename T>
struct FuncMAddSubMulDiv
{
    forceinline friend constexpr T operator*(const T& left, const T& right) noexcept
    {
        using T8 = simds::V8SIMD<E>;
        const auto rxx = right.XY.template PermuteLane<0, 0>();
        const auto ryy = right.XY.template PermuteLane<1, 1>();
        const auto rzz = right.ZW.template PermuteLane<0, 0>();
        auto xy = rxx.template MulAddLane<0>(left.XY, T8::AllZero());
             xy = ryy.template MulAddLane<1>(left.XY, xy);
             xy = rzz.template MulAddLane<2>(left.XY, xy);
        auto zw = rxx.template MulAddLane<0>(left.ZW, T8::AllZero());
             zw = ryy.template MulAddLane<1>(left.ZW, zw);
             zw = rzz.template MulAddLane<2>(left.ZW, zw);
        if constexpr (N == 4)
        {
            const auto rww = right.ZW.template PermuteLane<1, 1>();
            xy = rww.template MulAddLane<3>(left.XY, xy);
            zw = rww.template MulAddLane<3>(left.ZW, zw);
        }
        return { xy, zw };
    }
    forceinline friend constexpr T EleWiseAdd(const T& left, const T& right) noexcept
    {
        return { left.XY.Add(right.XY), left.ZW.Add(right.ZW) };
    }
    forceinline friend constexpr T EleWiseSub(const T& left, const T& right) noexcept
    {
        return { left.XY.Sub(right.XY), left.ZW.Sub(right.ZW) };
    }
    forceinline friend constexpr T EleWiseMul(const T& left, const T& right) noexcept
    {
        return { left.XY.Mul(right.XY), left.ZW.Mul(right.ZW) };
    }
    forceinline friend constexpr T EleWiseDiv(const T& left, const T& right) noexcept
    {
        return { left.XY.Mul(right.XY.Rcp()), left.ZW.Mul(right.ZW.Rcp()) };
    }
};


template<size_t N, typename T>
struct FuncMinMax
{
    forceinline friend constexpr T Min(const T& l, const T& r) noexcept
    {
        return { l.XY.Min(r.XY), l.ZW.Min(r.ZW) };
    }
    forceinline friend constexpr T Max(const T& l, const T& r) noexcept
    {
        return { l.XY.Max(r.XY), l.ZW.Max(r.ZW) };
    }
};


template<size_t N, typename T>
struct FuncFPMath
{
    forceinline constexpr T Rcp() const noexcept
    {
        return 1.f / static_cast<const T*>(this);
    }
    forceinline constexpr T Sqrt() const noexcept
    {
        const auto& self = *static_cast<const T*>(this);
        return { self.XY.Sqrt(), self.ZW.Sqrt() };
    }
    forceinline constexpr T Rsqrt() const noexcept
    {
        return 1.f / Sqrt();
    }
};


template<size_t N, typename T>
struct FuncNegative
{
    forceinline constexpr T Negative() const noexcept
    {
        const auto& self = *static_cast<const T*>(this);
        return { self.XY.Negative(), self.ZW.Negative() };
    }
};

#else
template<typename E, size_t N, typename V, typename T>
using FuncSAddSubMulDiv = base::basics::FuncSameAddSubMulDiv<N, T, T, E>;

template<typename E, size_t N, typename V, typename T>
struct FuncVAddSubMulDiv
{
    forceinline friend constexpr V operator*(const T& left, const V& right) noexcept
    {
        using common::simd::DotPos;
        if constexpr (N == 4)
        {
            const auto x = left.RowX.template Dot<DotPos::XYZW, DotPos::X>(right.Data);
            const auto y = left.RowY.template Dot<DotPos::XYZW, DotPos::Y>(right.Data);
            const auto z = left.RowZ.template Dot<DotPos::XYZW, DotPos::Z>(right.Data);
            const auto w = left.RowW.template Dot<DotPos::XYZW, DotPos::W>(right.Data);
            return x.Or(y).Or(z.Or(w));
        }
        else if constexpr (N == 3)
        {
            const auto x = left.RowX.template Dot<DotPos::XYZ, DotPos::X>(right.Data);
            const auto y = left.RowY.template Dot<DotPos::XYZ, DotPos::Y>(right.Data);
            const auto z = left.RowZ.template Dot<DotPos::XYZ, DotPos::Z>(right.Data);
            return x.Or(y).Or(z);
        }
    }
    forceinline friend constexpr V operator*(const V& left, const T& right) noexcept
    {
        const auto x   = right.RowX.Mul   (left.Data.template Broadcast<0>());
        const auto xy  = right.RowY.MulAdd(left.Data.template Broadcast<1>(), x);
        const auto xyz = right.RowZ.MulAdd(left.Data.template Broadcast<2>(), xy);
        if constexpr (N == 4)
            return right.RowW.MulAdd(left.Data.template Broadcast<3>(), xyz);
        else if constexpr (N == 3)
            return xyz;
    }
    forceinline friend constexpr T EleWiseAdd(const T& left, const V& right) noexcept
    {
        return base::basics::SameAddSubMulDiv<N, T, T, V>::Add(left, right);
    }
    forceinline friend constexpr T EleWiseAdd(const V& left, const T& right) noexcept
    {
        return base::basics::SameAddSubMulDiv<N, T, T, V>::Add(right, left);
    }
    forceinline friend constexpr T EleWiseSub(const T& left, const V& right) noexcept
    {
        return base::basics::SameAddSubMulDiv<N, T, T, V>::Sub(left, right);
    }
    forceinline friend constexpr T EleWiseSub(const V& left, const T& right) noexcept
    {
        return base::basics::SameAddSubMulDiv<N, T, T, V>::Sub(left, right);
    }
    forceinline friend constexpr T EleWiseMul(const T& left, const V& right) noexcept
    {
        return base::basics::SameAddSubMulDiv<N, T, T, V>::Mul(left, right);
    }
    forceinline friend constexpr T EleWiseMul(const V& left, const T& right) noexcept
    {
        return base::basics::SameAddSubMulDiv<N, T, T, V>::Mul(right, left);
    }
    forceinline friend constexpr T EleWiseDiv(const T& left, const V& right) noexcept
    {
        return EleWiseMul(left, V{ right.Data.Rcp() });
    }
    forceinline friend constexpr T EleWiseDiv(const V& left, const T& right) noexcept
    {
        return EleWiseMul(T{ right.RowX.Rcp(), right.RowY.Rcp(), right.RowZ.Rcp(), right.RowW.Rcp() }, left);
    }
};

template<typename E, size_t N, typename V, typename T>
struct FuncMAddSubMulDiv
{
    forceinline friend constexpr T operator*(const T& left, const T& right) noexcept
    {
        using T4 = simds::V4SIMD<E>;
        auto x = right.RowX.template MulScalarAdd<0>(left.RowX, T4::AllZero());
             x = right.RowY.template MulScalarAdd<1>(left.RowX, x);
             x = right.RowZ.template MulScalarAdd<2>(left.RowX, x);
        auto y = right.RowX.template MulScalarAdd<0>(left.RowY, T4::AllZero());
             y = right.RowY.template MulScalarAdd<1>(left.RowY, y);
             y = right.RowZ.template MulScalarAdd<2>(left.RowY, y);
        auto z = right.RowX.template MulScalarAdd<0>(left.RowZ, T4::AllZero());
             z = right.RowY.template MulScalarAdd<1>(left.RowZ, z);
             z = right.RowZ.template MulScalarAdd<2>(left.RowZ, z);
        if constexpr (N == 4)
        {
            x = right.RowW.template MulScalarAdd<3>(left.RowX, x);
            y = right.RowW.template MulScalarAdd<3>(left.RowY, y);
            z = right.RowW.template MulScalarAdd<3>(left.RowZ, z);
            auto w = right.RowX.template MulScalarAdd<0>(left.RowW, T4::AllZero());
                 w = right.RowY.template MulScalarAdd<1>(left.RowW, w);
                 w = right.RowZ.template MulScalarAdd<2>(left.RowW, w);
                 w = right.RowW.template MulScalarAdd<3>(left.RowW, w);
            return { x,y,z,w };
        }
        else
            return { x,y,z };
    }
    forceinline friend constexpr T EleWiseAdd(const T& left, const T& right) noexcept
    {
        return base::basics::EachAddSubMulDiv<N, T, T, T>::Add(left, right);
    }
    forceinline friend constexpr T EleWiseSub(const T& left, const T& right) noexcept
    {
        return base::basics::EachAddSubMulDiv<N, T, T, T>::Sub(left, right);
    }
    forceinline friend constexpr T EleWiseMul(const T& left, const T& right) noexcept
    {
        return base::basics::EachAddSubMulDiv<N, T, T, T>::Mul(left, right);
    }
    forceinline friend constexpr T EleWiseDiv(const T& left, const T& right) noexcept
    {
        return base::basics::EachAddSubMulDiv<N, T, T, T>::Div(left, right);
    }
};


template<size_t N, typename T>
using FuncMinMax = base::mat::FuncMinMax<N, T>;
template<size_t N, typename T>
using FuncFPMath = base::mat::FuncFPMath<N, T>;
template<size_t N, typename T>
using FuncNegative = base::mat::FuncNegative<N, T>;

#endif

}

#define MAT_EXTRA_DEF4 using Mat4x4Base::RowX; using Mat4x4Base::RowY; using Mat4x4Base::RowZ; using Mat4x4Base::RowW;
#ifdef XCOMP_HAS_SIMD256
#   define MAT_EXTRA_DEF using Mat4x4Base::XY; using Mat4x4Base::ZW; MAT_EXTRA_DEF4
#else
#   define MAT_EXTRA_DEF MAT_EXTRA_DEF4
#endif
#include "MatMain.hpp"
#undef MAT_EXTRA_DEF
#undef MAT_EXTRA_DEF4

}

#undef XCOMP_HAS_SIMD256
