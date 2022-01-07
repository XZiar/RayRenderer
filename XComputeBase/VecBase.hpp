#pragma once
#include "XCompRely.h"
#include <cmath>

namespace xcomp::math
{

namespace rule
{
template<typename E, size_t N, size_t M>
struct ElementBasic 
{
    static constexpr size_t ElementCount = N;
    static constexpr size_t StorageCount = M;
};

template<template<class, size_t, size_t> class T, class U>
struct BasicChecker
{
private:
    template<class V, size_t N, size_t M>
    static decltype(static_cast<const T<V, N, M>&>(std::declval<U>()), std::true_type{})
        Test(const T<V, N, M>&);
    static std::false_type Test(...);
public:
    static constexpr bool IsBasic = decltype(BasicChecker::Test(std::declval<U>()))::value;
};

template<class T, bool IsBasic = BasicChecker<ElementBasic, T>::IsBasic>
struct CheckBasic;
template<typename T>
struct CheckBasic<T, true>
{
    static constexpr bool IsBasic = true;
    using Type = typename T::EleType;
    static constexpr size_t Count = T::ElementCount;
};
template<typename T>
struct CheckBasic<T, false>
{
    static constexpr bool IsBasic = false;
    using Type = T;
    static constexpr size_t Count = 1;
};


template<typename T>
struct Concater
{
    using E = typename T::EleType;
    static constexpr size_t N = CheckBasic<T>::Count;
    static constexpr auto Indexes = std::make_index_sequence<N>();

    template<typename... Args>
    static constexpr std::array<std::pair<uint8_t, uint8_t>, N> Generate() noexcept
    {
        static_assert((... && std::is_same_v<E, typename CheckBasic<Args>::Type>),
            "Concat element type does not match");
        static_assert((... + CheckBasic<Args>::Count) == N,
            "Concat length does not match");
        constexpr std::array<size_t, sizeof...(Args)> Lengths{ CheckBasic<Args>::Count... };
        std::array<std::pair<uint8_t, uint8_t>, N> ret = {};
        uint8_t argIdx = 0, eleIdx = 0;
        for (auto& p : ret)
        {
            p.first = argIdx;
            p.second = Lengths[argIdx] > 1 ? eleIdx : UINT8_MAX;
            if (Lengths[argIdx] <= ++eleIdx)
                argIdx++, eleIdx = 0;
        }
        return ret;
    }

    template<uint8_t ArgIdx, uint8_t EleIdx, typename Tuple>
    forceinline static constexpr E Get(Tuple&& tp) noexcept
    {
        if constexpr (EleIdx == UINT8_MAX)
            return std::get<ArgIdx>(tp);
        else
            return std::get<ArgIdx>(tp)[EleIdx];
    }

    template<typename... Args, size_t... I>
    forceinline static constexpr T Concat_(std::index_sequence<I...>, Args&&... args) noexcept
    {
        constexpr auto Mappings = Generate<common::remove_cvref_t<Args>...>();
        auto tmp = std::forward_as_tuple(args...);
        return { Get<Mappings[I].first, Mappings[I].second>(tmp)... };
    }
    template<typename... Args>
    forceinline static constexpr T Concat(Args&&... args) noexcept
    {
        return Concat_(Indexes, std::forward<Args>(args)...);
    }
};

}

namespace base
{

namespace vec
{

/*a vector contains 2 element(int32 or float)*/
template<typename T>
class alignas(8) Vec2Base
{
    static_assert(sizeof(T) == 4, "only 4-byte length type allowed");
protected:
    T X, Y;
    constexpr Vec2Base(T x, T y) noexcept : X(x), Y(y) {}

public:
    constexpr Vec2Base() noexcept : X(0), Y(0) { }

    template<typename V, typename = std::enable_if_t<std::is_base_of_v<Vec2Base<typename V::EleType>, V>>>
    forceinline V& As() noexcept
    {
        return *reinterpret_cast<V*>(this);
    }
    template<typename V, typename = std::enable_if_t<std::is_base_of_v<Vec2Base<typename V::EleType>, V>>>
    forceinline const V& As() const noexcept
    {
        return *reinterpret_cast<const V*>(this);
    }

    forceinline constexpr T operator[](size_t idx) const noexcept { return (&X)[idx]; }
    forceinline constexpr T& operator[](size_t idx) noexcept { return (&X)[idx]; }
};


/*a vector contains 4 element(int32 or float)*/
template<typename T>
class alignas(16) Vec4Base
{
    static_assert(sizeof(T) == 4, "only 4-byte length type allowed");
protected:
    T X, Y, Z, W;
    constexpr Vec4Base(T x, T y, T z, T w) noexcept : X(x), Y(y), Z(z), W(w) {}

public:
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
        else if constexpr (N == 3)
            ptr[0] = self.X, ptr[1] = self.Y, ptr[2] = self.Z;
        else if constexpr (N == 2)
            ptr[0] = self.X, ptr[1] = self.Y;
    }

    forceinline static constexpr T Load(const E* ptr) noexcept
    {
        if constexpr (N == 4)
            return T{ ptr[0], ptr[1], ptr[2], ptr[3] };
        else if constexpr (N == 3)
            return T{ ptr[0], ptr[1], ptr[2] };
        else if constexpr (N == 2)
            return T{ ptr[0], ptr[1] };
    }
    forceinline static constexpr T Zeros() noexcept
    {
        if constexpr (N == 4)
            return T{ 0, 0, 0, 0 };
        else if constexpr (N == 3)
            return T{ 0, 0, 0 };
        else if constexpr (N == 2)
            return T{ 0, 0 };
    }
    forceinline static constexpr T Ones() noexcept
    {
        if constexpr (N == 4)
            return T{ 1, 1, 1, 1 };
        else if constexpr (N == 3)
            return T{ 1, 1, 1 };
        else if constexpr (N == 2)
            return T{ 1, 1 };
    }
};


template<typename E, size_t N, typename T, typename R>
struct FuncSAdd
{
    forceinline friend constexpr R operator+(const T& left, const E right) noexcept
    {
        if constexpr (N == 4)
            return { left.X + right, left.Y + right, left.Z + right, left.W + right };
        else if constexpr (N == 3)
            return { left.X + right, left.Y + right, left.Z + right };
        else if constexpr (N == 2)
            return { left.X + right, left.Y + right };
    }
    forceinline friend constexpr R operator+(const E left, const T& right) noexcept
    {
        return right + left;
    }
};
template<size_t N, typename T, typename R, typename V>
struct FuncVAddBase
{
    forceinline friend constexpr R operator+(const T& left, const V& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W };
        else if constexpr (N == 3)
            return { left.X + right.X, left.Y + right.Y, left.Z + right.Z };
        else if constexpr (N == 2)
            return { left.X + right.X, left.Y + right.Y };
    }
};


template<typename E, size_t N, typename T, typename R>
struct FuncSSub
{
    forceinline friend constexpr R operator-(const T& left, const E right) noexcept
    {
        if constexpr (N == 4)
            return { left.X - right, left.Y - right, left.Z - right, left.W - right };
        else if constexpr (N == 3)
            return { left.X - right, left.Y - right, left.Z - right };
        else if constexpr (N == 2)
            return { left.X - right, left.Y - right };
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
        else if constexpr (N == 3)
            return { left.X + right.X, left.Y + right.Y, left.Z + right.Z };
        else if constexpr (N == 2)
            return { left.X + right.X, left.Y + right.Y };
    }
};


template<typename E, size_t N, typename T, typename R>
struct FuncSMulDiv
{
    forceinline friend constexpr R operator*(const T& left, const E right) noexcept
    {
        if constexpr (N == 4)
            return { left.X * right, left.Y * right, left.Z * right, left.W * right };
        else if constexpr (N == 3)
            return { left.X * right, left.Y * right, left.Z * right };
        else if constexpr (N == 2)
            return { left.X * right, left.Y * right };
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
            else if constexpr (N == 3)
                return { left.X / right, left.Y / right, left.Z / right };
            else if constexpr (N == 2)
                return { left.X / right, left.Y / right };
        }
    }
    forceinline friend constexpr R operator/(const E left, const T& right) noexcept
    {
        return T{ left } / right;
    }
};
template<size_t N, typename T, typename R, typename V>
struct FuncVMulDivBase
{
    forceinline friend constexpr R operator*(const T& left, const V& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W };
        else if constexpr (N == 3)
            return { left.X * right.X, left.Y * right.Y, left.Z * right.Z };
        else if constexpr (N == 2)
            return { left.X * right.X, left.Y * right.Y };
    }
    forceinline friend constexpr R operator/(const T& left, const V& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X / right.X, left.Y / right.Y, left.Z / right.Z, left.W / right.W };
        else if constexpr (N == 3)
            return { left.X / right.X, left.Y / right.Y, left.Z / right.Z };
        else if constexpr (N == 2)
            return { left.X / right.X, left.Y / right.Y };
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
        else if constexpr (N == 2)
            return { std::min(l.X, r.X), std::min(l.Y, r.Y) };
    }
    forceinline friend constexpr T Max(const T& l, const T& r) noexcept
    {
        if constexpr (N == 4)
            return { std::max(l.X, r.X), std::max(l.Y, r.Y), std::max(l.Z, r.Z), std::max(l.W, r.W) };
        else if constexpr (N == 3)
            return { std::max(l.X, r.X), std::max(l.Y, r.Y), std::max(l.Z, r.Z) };
        else if constexpr (N == 2)
            return { std::max(l.X, r.X), std::max(l.Y, r.Y) };
    }
};


template<typename E, size_t N, typename T>
struct FuncDotBase
{
    forceinline friend constexpr E Dot(const T& l, const T& r) noexcept //dot product
    {
        if constexpr (N == 4)
            return l.X * r.X + l.Y * r.Y + l.Z * r.Z + l.W * r.W;
        else if constexpr (N == 3)
            return l.X * r.X + l.Y * r.Y + l.Z * r.Z;
        else if constexpr (N == 2)
            return l.X * r.X + l.Y * r.Y;
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
        else if constexpr (N == 2)
            return { std::sqrt(self.X), std::sqrt(self.Y) };
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
        else if constexpr (N == 2)
            return { -self.X, -self.Y };
    }
};

}

#define ENABLE_VEC2 1
#include "VecMain.hpp"
#undef ENABLE_VEC2

}

}
