#pragma once
#include "../CommonRely.hpp"
#include <cmath>

namespace common::math
{

namespace rule
{
template<typename T, typename... Ts>
inline constexpr bool DecayTypeMatchAny = (... || std::is_same_v<std::decay_t<T>, Ts>);

template<typename E, size_t N, size_t M>
struct ElementBasic 
{
    static constexpr size_t ElementCount = N;
    static constexpr size_t StorageCount = M;
};

enum class ArgType : uint8_t { Basic, Scalar, Pair };

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
template<typename T>
struct Pairchecker
{
    static constexpr bool IsPairSame = false;
};
template<typename T>
struct Pairchecker<std::pair<T, T>>
{
    static constexpr bool IsPairSame = true;
};

template<typename T>
inline constexpr ArgType ToArgType = BasicChecker<ElementBasic, T>::IsBasic ? ArgType::Basic :
    (Pairchecker<T>::IsPairSame ? ArgType::Pair : ArgType::Scalar);


template<typename T, ArgType Type = ToArgType<T>>
struct ArgChecker;
template<typename T>
struct ArgChecker<T, ArgType::Basic>
{
    static constexpr ArgType AType = ArgType::Basic;
    using Type = typename T::EleType;
    static constexpr size_t Count = T::ElementCount;
};
template<typename T>
struct ArgChecker<T, ArgType::Scalar>
{
    static constexpr ArgType AType = ArgType::Scalar;
    using Type = T;
    static constexpr size_t Count = 1;
};
template<typename T>
struct ArgChecker<T, ArgType::Pair>
{
    static_assert(std::is_same_v<typename T::first_type, typename T::second_type>);
    static constexpr ArgType AType = ArgType::Pair;
    using Type = typename T::first_type;
    static constexpr size_t Count = 2;
};


template<typename T>
struct Concater
{
    using E = typename T::EleType;
    static constexpr size_t N = T::ElementCount;
    static constexpr auto Indexes = std::make_index_sequence<N>();

    template<typename... Args>
    static constexpr std::array<std::pair<uint8_t, uint8_t>, N> Generate() noexcept
    {
        static_assert((... && std::is_same_v<E, typename ArgChecker<Args>::Type>),
            "Concat element type does not match");
        static_assert((... + ArgChecker<Args>::Count) == N,
            "Concat length does not match");
        constexpr std::array<ArgType, sizeof...(Args)> Types  { ArgChecker<Args>::AType... };
        constexpr std::array<size_t,  sizeof...(Args)> Lengths{ ArgChecker<Args>::Count... };
        std::array<std::pair<uint8_t, uint8_t>, N> ret = {};
        uint8_t argIdx = 0, eleIdx = 0;
        for (auto& p : ret)
        {
            p.first = argIdx;
            switch (Types[argIdx])
            {
            case ArgType::Basic:  p.second = eleIdx; break;
            case ArgType::Scalar: p.second = UINT8_MAX; break;
            case ArgType::Pair:   p.second = static_cast<uint8_t>(253 + eleIdx); break;
            }
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
        else if constexpr (EleIdx >= 253)
            return std::get<EleIdx - 253>(std::get<ArgIdx>(tp));
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


namespace shared
{

template<typename T>
struct FuncSelfCalc
{
    template<typename X, typename = std::enable_if_t<std::is_same_v<decltype(std::declval<T>() + std::declval<X>()), T>>>
    forceinline constexpr T& operator+=(const X& other) noexcept
    {
        auto& self = *static_cast<T*>(this);
        self = self + other;
        return self;
    }
    template<typename X, typename = std::enable_if_t<std::is_same_v<decltype(std::declval<T>() - std::declval<X>()), T>>>
    forceinline constexpr T& operator-=(const X& other) noexcept
    {
        auto& self = *static_cast<T*>(this);
        self = self - other;
        return self;
    }
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

}


namespace base
{

namespace basics
{

namespace detail
{
template<typename T>
using RcpTyper = decltype(std::declval<const T&>().Rcp());
template<typename T>
inline static constexpr bool HasRcp = common::is_detected_v<RcpTyper, T>;
}


template<size_t N, typename T, typename R, typename U>
struct SameAddSubMulDiv
{
    static_assert(!std::is_same_v<T, U>);
    forceinline static constexpr R Add(const T& left, const U& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X + right, left.Y + right, left.Z + right, left.W + right };
        else if constexpr (N == 3)
            return { left.X + right, left.Y + right, left.Z + right };
        else if constexpr (N == 2)
            return { left.X + right, left.Y + right };
    }
    forceinline static constexpr R Sub(const T& left, const U& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X - right, left.Y - right, left.Z - right, left.W - right };
        else if constexpr (N == 3)
            return { left.X - right, left.Y - right, left.Z - right };
        else if constexpr (N == 2)
            return { left.X - right, left.Y - right };
    }
    forceinline static constexpr R Sub(const U& left, const T& right) noexcept
    {
        if constexpr (N == 4)
            return { left - right.X, left - right.Y, left - right.Z, left - right.W };
        else if constexpr (N == 3)
            return { left - right.X, left - right.Y, left - right.Z };
        else if constexpr (N == 2)
            return { left - right.X, left - right.Y };
    }
    forceinline static constexpr R Mul(const T& left, const U& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X * right, left.Y * right, left.Z * right, left.W * right };
        else if constexpr (N == 3)
            return { left.X * right, left.Y * right, left.Z * right };
        else if constexpr (N == 2)
            return { left.X * right, left.Y * right };
    }
    forceinline static constexpr R Div(const T& left, const U& right) noexcept
    {
        if constexpr (std::is_floating_point_v<U>)
            return Mul(left, (static_cast<U>(1) / right));
        else if constexpr (detail::HasRcp<U>)
            return Mul(left, right.Rcp());
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
    forceinline static constexpr R Div(const U& left, const T& right) noexcept
    {
        if constexpr (N == 4)
            return { left / right.X, left / right.Y, left / right.Z, left / right.W };
        else if constexpr (N == 3)
            return { left / right.X, left / right.Y, left / right.Z };
        else if constexpr (N == 2)
            return { left / right.X, left / right.Y };
    }
};

template<size_t N, typename T, typename R, typename U>
struct EachAddSubMulDiv
{
    forceinline static constexpr R Add(const T& left, const U& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W };
        else if constexpr (N == 3)
            return { left.X + right.X, left.Y + right.Y, left.Z + right.Z };
        else if constexpr (N == 2)
            return { left.X + right.X, left.Y + right.Y };
    }
    forceinline static constexpr R Sub(const T& left, const U& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W };
        else if constexpr (N == 3)
            return { left.X - right.X, left.Y - right.Y, left.Z - right.Z };
        else if constexpr (N == 2)
            return { left.X - right.X, left.Y - right.Y };
    }
    forceinline static constexpr R Mul(const T& left, const U& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W };
        else if constexpr (N == 3)
            return { left.X * right.X, left.Y * right.Y, left.Z * right.Z };
        else if constexpr (N == 2)
            return { left.X * right.X, left.Y * right.Y };
    }
    forceinline static constexpr R Div(const T& left, const U& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X / right.X, left.Y / right.Y, left.Z / right.Z, left.W / right.W };
        else if constexpr (N == 3)
            return { left.X / right.X, left.Y / right.Y, left.Z / right.Z };
        else if constexpr (N == 2)
            return { left.X / right.X, left.Y / right.Y };
    }
};


template<size_t N, typename T, typename R, typename U>
struct FuncSameAddSubMulDiv
{
    static_assert(!std::is_same_v<T, U>);
    forceinline friend constexpr R operator+(const T& left, const U& right) noexcept
    {
        return SameAddSubMulDiv<N, T, R, U>::Add(left, right);
    }
    forceinline friend constexpr R operator+(const U& left, const T& right) noexcept
    {
        return SameAddSubMulDiv<N, T, R, U>::Add(right, left);
    }
    forceinline friend constexpr R operator-(const T& left, const U& right) noexcept
    {
        return SameAddSubMulDiv<N, T, R, U>::Sub(left, right);
    }
    forceinline friend constexpr R operator-(const U& left, const T& right) noexcept
    {
        return SameAddSubMulDiv<N, T, R, U>::Sub(left, right);
    }
    forceinline friend constexpr R operator*(const T& left, const U& right) noexcept
    {
        return SameAddSubMulDiv<N, T, R, U>::Mul(left, right);
    }
    forceinline friend constexpr R operator*(const U& left, const T& right) noexcept
    {
        return SameAddSubMulDiv<N, T, R, U>::Mul(right, left);
    }
    forceinline friend constexpr R operator/(const T& left, const U& right) noexcept
    {
        return SameAddSubMulDiv<N, T, R, U>::Div(left, right);
    }
    forceinline friend constexpr R operator/(const U& left, const T& right) noexcept
    {
        return SameAddSubMulDiv<N, T, R, U>::Div(left, right);
    }
};


template<size_t N, typename T, typename R, typename U>
struct FuncEachAddSubMulDiv
{
    forceinline friend constexpr R operator+(const T& left, const U& right) noexcept
    {
        return EachAddSubMulDiv<N, T, R, U>::Add(left, right);
    }
    forceinline friend constexpr R operator-(const T& left, const U& right) noexcept
    {
        return EachAddSubMulDiv<N, T, R, U>::Sub(left, right);
    }
    forceinline friend constexpr R operator*(const T& left, const U& right) noexcept
    {
        return EachAddSubMulDiv<N, T, R, U>::Mul(left, right);
    }
    forceinline friend constexpr R operator/(const T& left, const U& right) noexcept
    {
        return EachAddSubMulDiv<N, T, R, U>::Div(left, right);
    }
};

}

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
    forceinline static constexpr Vec2Base<T> LoadAll(const T* ptr) noexcept
    {
        return { ptr[0], ptr[1] };
    }
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

    forceinline constexpr T  operator[](size_t idx) const noexcept { return (&X)[idx]; }
    forceinline constexpr T& operator[](size_t idx)       noexcept { return (&X)[idx]; }
    forceinline constexpr const T* Ptr() const noexcept { return &X; }
    forceinline constexpr       T* Ptr()       noexcept { return &X; }
    forceinline constexpr void SaveAll(T* ptr) const noexcept
    {
        ptr[0] = X, ptr[1] = Y;
    }
};


/*a vector contains 4 element(int32 or float)*/
template<typename T>
class alignas(16) Vec4Base
{
    static_assert(sizeof(T) == 4, "only 4-byte length type allowed");
protected:
    T X, Y, Z, W;
    constexpr Vec4Base(T x, T y, T z, T w) noexcept : X(x), Y(y), Z(z), W(w) {}
    forceinline static constexpr Vec4Base<T> LoadAll(const T* ptr) noexcept
    {
        return { ptr[0], ptr[1], ptr[2], ptr[3] };
    }
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

    forceinline constexpr T  operator[](size_t idx) const noexcept { return (&X)[idx]; }
    forceinline constexpr T& operator[](size_t idx)       noexcept { return (&X)[idx]; }
    forceinline constexpr const T* Ptr() const noexcept { return &X; }
    forceinline constexpr       T* Ptr()       noexcept { return &X; }
    forceinline constexpr void SaveAll(T* ptr) const noexcept
    {
        ptr[0] = X, ptr[1] = Y, ptr[2] = Z, ptr[3] = W;
    }
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
            return T{ E(0), E(0), E(0), E(0) };
        else if constexpr (N == 3)
            return T{ E(0), E(0), E(0) };
        else if constexpr (N == 2)
            return T{ E(0), E(0) };
    }
    forceinline static constexpr T Ones() noexcept
    {
        if constexpr (N == 4)
            return T{ E(1), E(1), E(1), E(1) };
        else if constexpr (N == 3)
            return T{ E(1), E(1), E(1) };
        else if constexpr (N == 2)
            return T{ E(1), E(1) };
    }
};


template<typename E, size_t N, typename T>
using FuncSAddSubMulDiv = basics::FuncSameAddSubMulDiv<N, T, T, E>;
template<typename E, size_t N, typename T>
using FuncVAddSubMulDiv = basics::FuncEachAddSubMulDiv<N, T, T, T>;


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
        return 1.f / *static_cast<const T*>(this);
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
