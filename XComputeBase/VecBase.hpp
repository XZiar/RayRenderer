#pragma once
#include "XCompRely.h"
#include <cmath>

namespace xcomp::math::base
{

namespace vec
{

/*a vector contains 4 element(int32 or float)*/
template<typename T>
class alignas(16) Vec4Base
{
    static_assert(sizeof(T) == 4, "only 4-byte length type allowed");
protected:
    T X, Y, Z, W;
    constexpr Vec4Base(T x, T y, T z, T w) noexcept : X(x), Y(y), Z(z), W(w) {}

public:
    using EleType = T;
    constexpr Vec4Base() noexcept : X(0), Y(0), Z(0), W(0) { }

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

    forceinline constexpr T operator[](size_t idx) const noexcept { return (&X)[idx]; }
    forceinline constexpr T& operator[](size_t idx) noexcept { return (&X)[idx]; }
};


template<typename E, size_t N, typename T>
struct VecBasic
{
    forceinline constexpr void Save(E* ptr) const noexcept
    {
        auto& self = *static_cast<const T*>(this);
        if constexpr (N == 4)
            ptr[0] = self.X, ptr[1] = self.Y, ptr[2] = self.Z, ptr[3] = self.W;
        else
            ptr[0] = self.X, ptr[1] = self.Y, ptr[2] = self.Z;
    }

    forceinline static constexpr T Load(const E* ptr) noexcept
    {
        if constexpr (N == 4)
            return T{ ptr[0], ptr[1], ptr[2], ptr[3] };
        else
            return T{ ptr[0], ptr[1], ptr[2] };
    }
    forceinline static constexpr T Zeros() noexcept
    {
        return T{ 0, 0, 0, 0 };
    }
    forceinline static constexpr T Ones() noexcept
    {
        return T{ 1, 1, 1, 1 };
    }
};


template<typename E, size_t N, typename T, typename R>
struct FuncSAdd
{
    forceinline friend constexpr R operator+(const T& left, const E right) noexcept
    {
        if constexpr (N == 4)
            return { left.X + right, left.Y + right, left.Z + right, left.W + right };
        else
            return { left.X + right, left.Y + right, left.Z + right };
    }
    forceinline friend constexpr R operator+(const E left, const T& right) noexcept
    {
        return right + left;
    }
};
template<size_t N, typename T, typename R, typename V>
struct FuncVAdd
{
    forceinline friend constexpr R operator+(const T& left, const V& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W };
        else
            return { left.X + right.X, left.Y + right.Y, left.Z + right.Z };
    }
    template<typename = std::enable_if_t<!std::is_same_v<T, V>>>
    forceinline friend constexpr R operator+(const typename V left, const T& right) noexcept
    {
        return right + left;
    }
};
template<typename T>
struct FuncSelfAdd
{
    template<typename X, typename = std::enable_if_t<std::is_same_v<decltype(std::declval<T>() + std::declval<X>()), T>>>
    forceinline constexpr T& operator+=(const X& other) noexcept
    {
        auto& self = *static_cast<T*>(this);
        self = self + other;
        return self;
    }
};


template<typename E, size_t N, typename T, typename R>
struct FuncSSub
{
    forceinline friend constexpr R operator-(const T& left, const E right) noexcept
    {
        if constexpr (N == 4)
            return { left.X - right, left.Y - right, left.Z - right, left.W - right };
        else
            return { left.X - right, left.Y - right, left.Z - right };
    }
    forceinline friend constexpr R operator-(const E left, const T& right) noexcept
    {
        return T{ left } - right;
    }
};
template<size_t N, typename T, typename R, typename V>
struct FuncVSub
{
    forceinline friend constexpr R operator-(const T& left, const V& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W };
        else
            return { left.X + right.X, left.Y + right.Y, left.Z + right.Z };
    }
};
template<typename T>
struct FuncSelfSub
{
    template<typename X, typename = std::enable_if_t<std::is_same_v<decltype(std::declval<T>() - std::declval<X>()), T>>>
    forceinline constexpr T& operator-=(const X& other) noexcept
    {
        auto& self = *static_cast<T*>(this);
        self = self + other;
        return self;
    }
};


template<typename E, size_t N, typename T, typename R>
struct FuncSMulDiv
{
    forceinline friend constexpr R operator*(const T& left, const E right) noexcept
    {
        if constexpr (N == 4)
            return { left.X * right, left.Y * right, left.Z * right, left.W * right };
        else
            return { left.X * right, left.Y * right, left.Z * right };
    }
    forceinline friend constexpr R operator*(const E left, const T& right) noexcept
    {
        return right * left;
    }
    forceinline friend constexpr R operator/(const T& left, const E right) noexcept
    {
        if constexpr (std::is_floating_point_v<E>)
        {
            return left * (E(1) / right);
        }
        else
        {
            if constexpr (N == 4)
                return { left.X / right, left.Y / right, left.Z / right, left.W / right };
            else
                return { left.X / right, left.Y / right, left.Z / right };
        }
    }
    forceinline friend constexpr R operator/(const E left, const T& right) noexcept
    {
        return T{ left } / right;
    }
};
template<size_t N, typename T, typename R, typename V>
struct FuncVMulDiv
{
    forceinline friend constexpr R operator*(const T& left, const V& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W };
        else
            return { left.X * right.X, left.Y * right.Y, left.Z * right.Z };
    }
    template<typename = std::enable_if_t<!std::is_same_v<T, V>>>
    forceinline friend constexpr R operator*(const typename V left, const T& right) noexcept
    {
        return right * left;
    }
    forceinline friend constexpr R operator/(const T& left, const V& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X / right.X, left.Y / right.Y, left.Z / right.Z, left.W / right.W };
        else
            return { left.X / right.X, left.Y / right.Y, left.Z / right.Z };
    }
};
template<typename T>
struct FuncSelfMulDiv
{
    template<typename X, typename = std::enable_if_t<std::is_same_v<decltype(std::declval<T>()* std::declval<X>()), T>>>
    forceinline constexpr T& operator*=(const X& other) noexcept
    {
        auto& self = *static_cast<T*>(this);
        self = self * other;
        return self;
    }
    template<typename X, typename = std::enable_if_t<std::is_same_v<decltype(std::declval<T>() / std::declval<X>()), T>>>
    forceinline constexpr T& operator/=(const X& other) noexcept
    {
        auto& self = *static_cast<T*>(this);
        self = self / other;
        return self;
    }
};


template<size_t N, typename T>
struct FuncMinMax
{
    forceinline friend constexpr T Min(const T& l, const T& r) noexcept
    {
        if constexpr (N == 4)
            return { std::min(l.X, r.X), std::min(l.Y, r.Y), std::min(l.Z, r.Z), std::min(l.W, r.W) };
        else if constexpr (N == 3)
            return { std::min(l.X, r.X), std::min(l.Y, r.Y), std::min(l.Z, r.Z) };
    }
    forceinline friend constexpr T Max(const T& l, const T& r) noexcept
    {
        if constexpr (N == 4)
            return { std::max(l.X, r.X), std::max(l.Y, r.Y), std::max(l.Z, r.Z), std::max(l.W, r.W) };
        else if constexpr (N == 3)
            return { std::max(l.X, r.X), std::max(l.Y, r.Y), std::max(l.Z, r.Z) };
    }
};


template<typename E, size_t N, typename T, bool SupportLength>
struct FuncDot
{
    forceinline friend constexpr E Dot(const T& l, const T& r) noexcept //dot product
    {
        if constexpr (N == 4)
            return l.X * r.X + l.Y * r.Y + l.Z * r.Z + l.W * r.W;
        else if constexpr (N == 3)
            return l.X * r.X + l.Y * r.Y + l.Z * r.Z;
    }
    template<typename = std::enable_if_t<SupportLength>>
    forceinline constexpr E LengthSqr() const noexcept
    {
        const auto& self = *static_cast<const T*>(this);
        return Dot(self, self);
    }
    template<typename = std::enable_if_t<std::is_floating_point_v<E>&& SupportLength>>
    forceinline constexpr E Length() const noexcept
    {
        return std::sqrt(LengthSqr());
    }
    template<typename = std::enable_if_t<std::is_floating_point_v<E>&& SupportLength>>
    forceinline constexpr T Normalize() const noexcept
    {
        const auto& self = *static_cast<const T*>(this);
        return self / Length();
    }
};


template<typename T>
struct FuncCross
{
    forceinline friend constexpr T Cross(const T& l, const T& r) noexcept
    {
        const float a = l.Y * r.Z - l.Z * r.Y;
        const float b = l.Z * r.X - l.X * r.Z;
        const float c = l.X * r.Y - l.Y * r.X;
        return { a, b, c };
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
        if constexpr (N == 4)
            return { std::sqrt(self.X), std::sqrt(self.Y), std::sqrt(self.Z), std::sqrt(self.W) };
        else if constexpr (N == 3)
            return { std::sqrt(self.X), std::sqrt(self.Y), std::sqrt(self.Z) };
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
        if constexpr (N == 4)
            return { -self.X, -self.Y, -self.Z, -self.W };
        else if constexpr (N == 3)
            return { -self.X, -self.Y, -self.Z };
    }
};

}

#include "VecMain.hpp"

}
