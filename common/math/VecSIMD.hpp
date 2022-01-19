#pragma once
#include "VecBase.hpp"
#include "../simd/SIMD128.hpp"


namespace common::math::simd
{

namespace simds
{

template<typename T> struct V4SimdConv;
template<> struct V4SimdConv<float>    { using T = common::simd::F32x4; };
template<> struct V4SimdConv<int32_t>  { using T = common::simd::I32x4; };
template<> struct V4SimdConv<uint32_t> { using T = common::simd::U32x4; };
template<typename T> using V4SIMD = typename V4SimdConv<T>::T;

}

namespace vec
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
/*a vector contains 4 element(int32 or float)*/
template<typename T>
class alignas(16) Vec4Base
{
    static_assert(sizeof(T) == 4, "only 4-byte length type allowed");
protected:
    using SIMDType = simds::V4SIMD<T>;
    union
    {
        SIMDType Data;
        struct
        {
            T X, Y, Z, W;
        };
    };
    constexpr Vec4Base(T x, T y, T z, T w) noexcept : Data(x, y, z, w) {}
public:
    constexpr Vec4Base() noexcept : Data(SIMDType::AllZero()) { }
    constexpr Vec4Base(SIMDType val) noexcept : Data(val) {}

    template<typename V, typename = std::enable_if_t<std::is_base_of_v<Vec4Base<typename V::EleType>, V>>>
    forceinline V& As() noexcept
    {
        return *reinterpret_cast<V*>(this);
    }
    template<typename V, typename = std::enable_if_t<std::is_base_of_v<Vec4Base<typename V::EleType>, V>>>
    forceinline const V& As() const noexcept
    {
        return *reinterpret_cast<const V*>(this);
    }

    forceinline constexpr T  operator[](size_t idx) const noexcept { return Data.Val[idx]; }
    forceinline constexpr T& operator[](size_t idx)       noexcept { return Data.Val[idx]; }
    forceinline constexpr const T* Ptr() const noexcept { return &X; }
    forceinline constexpr       T* Ptr()       noexcept { return &X; }
};
#if COMMON_COMPILER_CLANG
#   pragma clang diagnostic pop
#elif COMMON_COMPILER_GCC || COMMON_COMPILER_ICC
#   pragma GCC diagnostic pop
#elif COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


template<typename E, size_t N, typename T>
struct VecBasic
{
    using U = simds::V4SIMD<E>;
    forceinline constexpr void Save(E* ptr) const noexcept
    {
        auto& self = *static_cast<const T*>(this);
        if constexpr (N == 4)
            self.Data.Save(ptr);
        else if constexpr (N == 3)
            ptr[0] = self.X, ptr[1] = self.Y, ptr[2] = self.Z;
    }

    forceinline static constexpr T Load(const E* ptr) noexcept
    {
        if constexpr (N == 4)
            return U::Load(ptr);
        else if constexpr (N == 3)
            return T{ ptr[0], ptr[1], ptr[2] };
    }
    forceinline static constexpr T Zeros() noexcept
    {
        return U::AllZero();
    }
    forceinline static constexpr T Ones() noexcept
    {
        return { 1, 1, 1, 1 };
    }
};


template<typename E, size_t N, typename T>
struct FuncSAddSubMulDiv
{
    using U = simds::V4SIMD<E>;
    forceinline friend constexpr T operator+(const T& left, const E right) noexcept
    {
        return left.Data.Add(right);
    }
    forceinline friend constexpr T operator+(const E left, const T& right) noexcept
    {
        return right.Data.Add(left);
    }
    forceinline friend constexpr T operator-(const T& left, const E right) noexcept
    {
        return left.Data.Sub(right);
    }
    forceinline friend constexpr T operator-(const E left, const T& right) noexcept
    {
        return U(left).Sub(right.Data);
    }
    forceinline friend constexpr T operator*(const T& left, const E right) noexcept
    {
        if constexpr (std::is_floating_point_v<E>)
            return left.Data.Mul(right);
        else
            return left.Data.MulLo(right);
    }
    forceinline friend constexpr T operator*(const E left, const T& right) noexcept
    {
        if constexpr (std::is_floating_point_v<E>)
            return right.Data.Mul(left);
        else
            return right.Data.MulLo(left);
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
            return right.Data.Rcp().Mul(left);
        else
            return T{ left } / right;
    }
};

template<typename E, size_t N, typename T>
struct FuncVAddSubMulDiv
{
    forceinline friend constexpr T operator+(const T& left, const T& right) noexcept
    {
        return left.Data.Add(right.Data);
    }
    forceinline friend constexpr T operator-(const T& left, const T& right) noexcept
    {
        return left.Data.Sub(right.Data);
    }
    forceinline friend constexpr T operator*(const T& left, const T& right) noexcept
    {
        if constexpr (std::is_floating_point_v<typename T::EleType>)
            return left.Data.Mul(right.Data);
        else
            return left.Data.MulLo(right.Data);
    }
    forceinline friend constexpr T operator/(const T& left, const T& right) noexcept
    {
        if constexpr (std::is_floating_point_v<typename T::EleType>)
            return left.Data.Mul(right.Data.Rcp());
        else
        {
            if constexpr (N == 4)
                return { left.X / right.X, left.Y / right.Y, left.Z / right.Z, left.W / right.W };
            else if constexpr (N == 3)
                return { left.X / right.X, left.Y / right.Y, left.Z / right.Z };
        }
    }
};


template<size_t N, typename T>
struct FuncMinMax
{
    forceinline friend constexpr T Min(const T& l, const T& r) noexcept
    {
        return l.Data.Min(r.Data);
    }
    forceinline friend constexpr T Max(const T& l, const T& r) noexcept
    {
        return l.Data.Max(r.Data);
    }
};


template<typename E, size_t N, typename T>
struct FuncDotBase
{
    forceinline friend constexpr E Dot(const T& l, const T& r) noexcept //dot product
    {
        if constexpr (std::is_floating_point_v<E>)
        {
            if constexpr (N == 4)
                return l.Data.template Dot<common::simd::DotPos::XYZW>(r.Data);
            else if constexpr (N == 3)
                return l.Data.template Dot<common::simd::DotPos::XYZ> (r.Data);
        }
        else
        {
            if constexpr (N == 4)
                return l.X * r.X + l.Y * r.Y + l.Z * r.Z + l.W * r.W;
            else if constexpr (N == 3)
                return l.X * r.X + l.Y * r.Y + l.Z * r.Z;
        }
    }
};


template<typename T>
struct FuncCross
{
    forceinline friend constexpr T Cross(const T& l, const T& r) noexcept
    {
        const auto t3 = l.Data.template Shuffle<2, 0, 1, 3>(); //zxyw
        const auto t4 = r.Data.template Shuffle<1, 2, 0, 3>(); //yzxw
        const auto t34 = t3 * t4;
        const auto t1 = l.Data.template Shuffle<1, 2, 0, 3>(); //yzxw
        const auto t2 = r.Data.template Shuffle<2, 0, 1, 3>(); //zxyw
        return t1.MulSub(t2, t34); // (t1 * t2) - (t3 * t4);
    }
};


template<size_t N, typename T>
struct FuncFPMath
{
    forceinline constexpr T Rcp() const noexcept
    {
        return static_cast<const T*>(this)->Data.Rcp();
    }
    forceinline constexpr T Sqrt() const noexcept
    {
        return static_cast<const T*>(this)->Data.Sqrt();
    }
    forceinline constexpr T Rsqrt() const noexcept
    {
        return static_cast<const T*>(this)->Data.Rsqrt();
    }
};


template<size_t N, typename T>
struct FuncNegative
{
    forceinline constexpr T Negative() const noexcept
    {
        return static_cast<const T*>(this)->Data.Neg();
    }
};

}

#define VEC_EXTRA_DEF using Vec4Base::Data;
#include "VecMain.hpp"
#undef VEC_EXTRA_DEF
using  Vec2 = base:: Vec2;
using IVec2 = base::IVec2;
using UVec2 = base::UVec2;

}
