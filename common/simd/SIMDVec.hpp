#pragma once
#include "SIMD.hpp"
#include "../EnumEx.hpp"

namespace common
{
namespace simd
{
//namespace detail
//{
//template<typename T, typename V, typename S, size_t... I>
//inline T PackCast(const S& src, std::index_sequence<I...>)
//{
//    return T{ src[I].template Cast<V>()... };
//}
//}
template<typename T, size_t N>
struct alignas(T) Pack
{
    static_assert(N > 1);
    static constexpr size_t EleCount = N;
    T Data[N];
    template<typename... Ts, typename = std::enable_if_t<sizeof...(Ts) == N>>
    Pack(const Ts&... vals) noexcept : Data{ vals... } {}
    constexpr T& operator[](const size_t idx) noexcept { return Data[idx]; }
    constexpr const T& operator[](const size_t idx) const noexcept { return Data[idx]; }
    template<typename U>
    auto Cast() noexcept;
};

namespace detail
{
template<typename D, typename S, size_t N, size_t... I>
inline Pack<D, N> PackCast(const Pack<S, N>& src, std::index_sequence<I...>)
{
    static_assert(sizeof...(I) == N);
    return Pack<D, N>{ src[I].template Cast<D>()... };
}
template<size_t I, typename D, size_t M, size_t N>
inline D ExtractPackData(const Pack<D, M>(&dat)[N])
{
    return dat[I / M][I % M];
}
template<typename D, size_t M, typename S, size_t N, size_t... I, size_t... J>
inline Pack<D, N * M> PackCastExpand(const Pack<S, N>& src, std::index_sequence<I...>, std::index_sequence<J...>)
{
    static_assert(sizeof...(I) == N);
    static_assert(sizeof...(J) == N * M);
    const Pack<D, M> mid[] = { src[I].template Cast<D>()... };
    return Pack<D, N * M>{ ExtractPackData<J, D, M, N>(mid)... };
}
}

template<typename From, typename To>
struct CastTyper
{
    using Type = std::conditional_t<(From::Count > To::Count), Pack<To, From::Count / To::Count>, To>;
};

template<typename T, size_t N>
template<typename U>
inline auto Pack<T, N>::Cast() noexcept
{
    if constexpr (T::Count == U::Count)
    {
        return detail::PackCast<U, T, N>(*this, std::make_index_sequence<N>{});
    }
    else if constexpr (T::Count > U::Count)
    {
        constexpr auto M = T::Count / U::Count;
        return detail::PackCastExpand<U, M, T, N>(*this, std::make_index_sequence<N>{}, std::make_index_sequence<N* M>{});
    }
    else
    {
        static_assert(AlwaysTrue<T>, "cannot narrow cast");
    }
}


namespace detail
{

template<typename T>
struct CommonOperators
{
    // logic operations
    T operator&(const T& other) const { return static_cast<const T*>(this)->And(other); }
    T operator|(const T& other) const { return static_cast<const T*>(this)->Or(other); }
    T operator^(const T& other) const { return static_cast<const T*>(this)->Xor(other); }
    T operator~() const { return static_cast<const T*>(this)->Not(); }
    // arithmetic operations
    T operator+(const T& other) const { return static_cast<const T*>(this)->Add(other); }
    T operator-(const T& other) const { return static_cast<const T*>(this)->Sub(other); }
};

}

enum class DotPos : uint8_t 
{ 
    X = 0b1, Y = 0b10, Z = 0b100, W = 0b1000, 
    XY = X|Y, XZ = X|Z, XW = X|W, YZ = Y|Z, YW = Y|W, ZW = Z|W, XYZ = X|Y|Z, XYW = X|Y|W, XZW = X|Z|W, YZW = Y|Z|W, XYZW = X|Y|Z|W,
};
MAKE_ENUM_BITFIELD(DotPos)

}
}