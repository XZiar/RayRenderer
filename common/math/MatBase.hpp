#pragma once
#include "../CommonRely.hpp"
#include "VecBase.hpp"
#include <cmath>

namespace common::math
{

namespace shared
{

namespace mat
{

template<typename E, size_t N>
struct MatType
{
    static constexpr size_t ElementCount = N;
};

}

}

namespace base
{

namespace mat
{

/*a vector contains 4x4 element(int32 or float)*/
template<typename T, typename V>
class alignas(32) Mat4x4Base : public shared::Storage<sizeof(T), 16>
{
    static_assert(sizeof(T) == 4,  "only  4-byte length type allowed");
    static_assert(sizeof(V) == 16, "only 16-byte length vec type allowed");
protected:
    V X, Y, Z, W;
    constexpr Mat4x4Base(const V& x, const V& y, const V& z, const V& w) noexcept : X(x), Y(y), Z(z), W(w) {}
    forceinline static constexpr Mat4x4Base<T, V> LoadAll(const T* ptr) noexcept
    {
        return { V::LoadAll(ptr), V::LoadAll(ptr + 16), V::LoadAll(ptr + 32), V::LoadAll(ptr + 48) };
    }
public:
    constexpr Mat4x4Base() noexcept { }
    
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
        X.SaveAll(ptr), Y.SaveAll(ptr + 4), Z.SaveAll(ptr + 8), W.SaveAll(ptr + 12);
    }
};


template<typename E, size_t N, typename V, typename T>
struct MatBasic
{
    forceinline constexpr void Save(E* ptr) const noexcept
    {
        auto& self = *static_cast<const T*>(this);
        if constexpr (N == 4)
            self.X.Save(ptr), self.Y.Save(ptr + 4), self.Z.Save(ptr + 8), self.W.Save(ptr + 12);
        else if constexpr (N == 3)
            self.X.Save(ptr), self.Y.Save(ptr + 3), self.Z.Save(ptr + 6);
        else if constexpr (N == 2)
            self.X.Save(ptr), self.Y.Save(ptr + 2);
    }

    forceinline static constexpr T Load(const E* ptr) noexcept
    {
        if constexpr (N == 4)
            return T{ V::Load(ptr), V::Load(ptr + 4), V::Load(ptr + 8), V::Load(ptr + 12) };
        else if constexpr (N == 3)
            return T{ V::Load(ptr), V::Load(ptr + 3), V::Load(ptr + 6) };
        else if constexpr (N == 2)
            return T{ V::Load(ptr), V::Load(ptr + 2) };
    }
    forceinline static constexpr T Zeros() noexcept
    {
        if constexpr (N == 4)
            return T{ V::Zeros(), V::Zeros(), V::Zeros(), V::Zeros() };
        else if constexpr (N == 3)
            return T{ V::Zeros(), V::Zeros(), V::Zeros() };
        else if constexpr (N == 2)
            return T{ V::Zeros(), V::Zeros() };
    }
    forceinline static constexpr T Ones() noexcept
    {
        if constexpr (N == 4)
            return T{ V::Ones(), V::Ones(), V::Ones(), V::Ones() };
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
        if constexpr (N == 4)
            return 
        {
            { self.X.X, self.Y.X, self.Z.X, self.W.X },
            { self.X.Y, self.Y.Y, self.Z.Y, self.W.Y },
            { self.X.Z, self.Y.Z, self.Z.Z, self.W.Z },
            { self.X.W, self.Y.W, self.Z.W, self.W.W }
        };
        else if constexpr (N == 3)
            return
        {
            { self.X.X, self.Y.X, self.Z.X },
            { self.X.Y, self.Y.Y, self.Z.Y },
            { self.X.Z, self.Y.Z, self.Z.Z },
        };
        else if constexpr (N == 2)
            return
        {
            { self.X.X, self.Y.X },
            { self.X.Y, self.Y.Y }
        };
    }
};


template<typename E, size_t N, typename V, typename T>
using FuncSAddSubMulDiv = basics::FuncSameAddSubMulDiv<N, T, T, E>;

template<typename E, size_t N, typename V, typename T>
struct FuncVAddSubMulDiv
{
    forceinline friend constexpr V operator*(const T& left, const V& right) noexcept
    {
        if constexpr (N == 4)
            return { Dot(left.X, right), Dot(left.Y, right), Dot(left.Z, right), Dot(left.W, right) };
        else if constexpr (N == 3)
            return { Dot(left.X, right), Dot(left.Y, right), Dot(left.Z, right) };
        else if constexpr (N == 2)
            return { Dot(left.X, right), Dot(left.Y, right) };
    }
    forceinline friend constexpr V operator*(const V& left, const T& right) noexcept
    {
        if constexpr (N == 4)
            return left.X * right.X + left.Y * right.Y + left.Z * right.Z + left.W * right.W;
        else if constexpr (N == 3)
            return left.X * right.X + left.Y * right.Y + left.Z * right.Z;
        else if constexpr (N == 2)
            return left.X * right.X + left.Y * right.Y;
    }
    forceinline friend constexpr T EleWiseAdd(const T& left, const V& right) noexcept
    {
        return basics::SameAddSubMulDiv<N, T, T, V>::Add(left, right);
    }
    forceinline friend constexpr T EleWiseAdd(const V& left, const T& right) noexcept
    {
        return basics::SameAddSubMulDiv<N, T, T, V>::Add(right, left);
    }
    forceinline friend constexpr T EleWiseSub(const T& left, const V& right) noexcept
    {
        return basics::SameAddSubMulDiv<N, T, T, V>::Sub(left, right);
    }
    forceinline friend constexpr T EleWiseSub(const V& left, const T& right) noexcept
    {
        return basics::SameAddSubMulDiv<N, T, T, V>::Sub(left, right);
    }
    forceinline friend constexpr T EleWiseMul(const T& left, const V& right) noexcept
    {
        return basics::SameAddSubMulDiv<N, T, T, V>::Mul(left, right);
    }
    forceinline friend constexpr T EleWiseMul(const V& left, const T& right) noexcept
    {
        return basics::SameAddSubMulDiv<N, T, T, V>::Mul(right, left);
    }
    forceinline friend constexpr T EleWiseDiv(const T& left, const V& right) noexcept
    {
        return basics::SameAddSubMulDiv<N, T, T, V>::Div(left, right);
    }
    forceinline friend constexpr T EleWiseDiv(const V& left, const T& right) noexcept
    {
        return basics::SameAddSubMulDiv<N, T, T, V>::Div(left, right);
    }
};

template<typename E, size_t N, typename V, typename T>
struct FuncMAddSubMulDiv
{
    forceinline friend constexpr T operator*(const T& left, const T& right) noexcept
    {
        if constexpr (N == 4)
            return { left.X * right, left.Y * right, left.Z * right, left.W * right };
        else if constexpr (N == 3)
            return { left.X * right, left.Y * right, left.Z * right };
        else if constexpr (N == 2)
            return { left.X * right, left.Y * right };
    }
    forceinline friend constexpr T EleWiseAdd(const T& left, const T& right) noexcept
    {
        return basics::EachAddSubMulDiv<N, T, T, T>::Add(left, right);
    }
    forceinline friend constexpr T EleWiseSub(const T& left, const T& right) noexcept
    {
        return basics::EachAddSubMulDiv<N, T, T, T>::Sub(left, right);
    }
    forceinline friend constexpr T EleWiseMul(const T& left, const T& right) noexcept
    {
        return basics::EachAddSubMulDiv<N, T, T, T>::Mul(left, right);
    }
    forceinline friend constexpr T EleWiseDiv(const T& left, const T& right) noexcept
    {
        return basics::EachAddSubMulDiv<N, T, T, T>::Div(left, right);
    }
};


template<size_t N, typename T>
struct FuncMinMax
{
    forceinline friend constexpr T Min(const T& l, const T& r) noexcept
    {
        if constexpr (N == 4)
            return { Min(l.X, r.X), Min(l.Y, r.Y), Min(l.Z, r.Z), Min(l.W, r.W) };
        else if constexpr (N == 3)
            return { Min(l.X, r.X), Min(l.Y, r.Y), Min(l.Z, r.Z) };
        else if constexpr (N == 2)
            return { Min(l.X, r.X), Min(l.Y, r.Y) };
    }
    forceinline friend constexpr T Max(const T& l, const T& r) noexcept
    {
        if constexpr (N == 4)
            return { Max(l.X, r.X), Max(l.Y, r.Y), Max(l.Z, r.Z), Max(l.W, r.W) };
        else if constexpr (N == 3)
            return { Max(l.X, r.X), Max(l.Y, r.Y), Max(l.Z, r.Z) };
        else if constexpr (N == 2)
            return { Max(l.X, r.X), Max(l.Y, r.Y) };
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
            return { self.X.Sqrt(), self.Y.Sqrt(), self.Z.Sqrt(), self.W.Sqrt() };
        else if constexpr (N == 3)
            return { self.X.Sqrt(), self.Y.Sqrt(), self.Z.Sqrt() };
        else if constexpr (N == 2)
            return { self.X.Sqrt(), self.Y.Sqrt() };
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
            return { self.X.Negative(), self.Y.Negative(), self.Z.Negative(), self.W.Negative() };
        else if constexpr (N == 3)
            return { self.X.Negative(), self.Y.Negative(), self.Z.Negative() };
        else if constexpr (N == 2)
            return { self.X.Negative(), self.Y.Negative() };
    }
};

}

#include "MatMain.hpp"


}

}
