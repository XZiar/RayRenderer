#pragma once
#include "SIMD.hpp"
#include "../EnumEx.hpp"

namespace common
{
namespace simd
{

enum class DotPos : uint8_t
{
    X = 0b1, Y = 0b10, Z = 0b100, W = 0b1000,
    XY = X | Y, XZ = X | Z, XW = X | W, YZ = Y | Z, YW = Y | W, ZW = Z | W, XYZ = X | Y | Z, XYW = X | Y | W, XZW = X | Z | W, YZW = Y | Z | W, XYZW = X | Y | Z | W,
};
MAKE_ENUM_BITFIELD(DotPos)

enum class CastMode : uint8_t
{
    RangeUndef = 0x0, RangeSaturate = 0x1, RangeTrunc = 0x2,
    PrecTrunc = 0x10
};
MAKE_ENUM_BITFIELD(CastMode)

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
    auto Cast() const noexcept;
    template<typename U>
    Pack<U, N> As() const noexcept;
};

namespace detail
{
template<typename To, typename From>
inline To AsType(From) noexcept
{
    static_assert(!AlwaysTrue<To>, "not implemented");
}
template<typename D, typename S, size_t N, size_t... I>
inline Pack<D, N> PackCast(const Pack<S, N>& src, std::index_sequence<I...>)
{
    static_assert(sizeof...(I) == N);
    return Pack<D, N>{ src[I].template Cast<D>()... };
}
template<typename D, typename S, size_t N, size_t... I>
inline Pack<D, N> PackAs(const Pack<S, N>& src, std::index_sequence<I...>)
{
    static_assert(sizeof...(I) == N);
    using X = decltype(std::declval<D>().Data);
    return Pack<D, N>{ AsType<X>(src[I].Data)... };
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
inline auto Pack<T, N>::Cast() const noexcept
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
        static_assert(!AlwaysTrue<T>, "cannot narrow cast");
    }
}
template<typename T, size_t N>
template<typename U>
inline Pack<U, N> Pack<T, N>::As() const noexcept
{
    return detail::PackAs<U, T, N>(*this, std::make_index_sequence<N>{});
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
    T& operator&=(const T& other) 
    {
        const auto self = static_cast<const T*>(this);
        self->Data = self->And(other); 
        return *self;
    }
    T& operator|=(const T& other) 
    {
        const auto self = static_cast<const T*>(this);
        self->Data = self->Or(other);
        return *self;
    }
    T& operator^=(const T& other) 
    {
        const auto self = static_cast<const T*>(this);
        self->Data = self->Xor(other);
        return *self;
    }
    // arithmetic operations
    T operator+(const T& other) const { return static_cast<const T*>(this)->Add(other); }
    T operator-(const T& other) const { return static_cast<const T*>(this)->Sub(other); }
    T& operator+=(const T& other) 
    {
        const auto self = static_cast<const T*>(this);
        self->Data = self->Add(other);
        return *self;
    }
    T& operator-=(const T& other)
    {
        const auto self = static_cast<const T*>(this);
        self->Data = self->Sub(other);
        return *self;
    }
    template<typename U>
    U VECCALL As() const noexcept
    { 
        return AsType<decltype(std::declval<U>().Data)>(static_cast<const T*>(this)->Data);
    }
};

template<typename From, typename To>
inline constexpr CastMode CstMode() noexcept
{
    constexpr auto src = From::VDInfo, dst = To::VDInfo;
    if constexpr (src.Bit > dst.Bit)
    {
        if constexpr (src.Type == VecDataInfo::DataTypes::Float || dst.Type == VecDataInfo::DataTypes::Float)
            return CastMode::RangeUndef;
        return CastMode::RangeTrunc;
    }
    else 
    {
        return CastMode::RangeUndef;
    }
}

}

}
}