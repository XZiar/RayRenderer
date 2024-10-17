#pragma once
#include "CommonRely.hpp"

namespace common
{

namespace detail
{

template<size_t Bits>
inline constexpr auto ChooseUIntTypeByBits() noexcept
{
    if constexpr (Bits <= 8)
        return uint8_t(0);
    else if constexpr (Bits <= 16)
        return uint16_t(0);
    else if constexpr (Bits <= 32)
        return uint32_t(0);
    else if constexpr (Bits <= 64)
        return uint64_t(0);
    else
        static_assert(!common::AlwaysTrue2<Bits>, "exceed 64 bits");
}
template<typename T>
inline constexpr void PackBitsInSequence(T& out, T val, uint32_t& shifter, uint32_t bits) noexcept
{
    out |= (val << shifter);
    shifter += bits;
}
template<typename T, typename U, typename... Args>
inline constexpr void PackBitsByIndex(T& out, T keepMask, const uint32_t bits, const uint32_t idx, const U& dat, const Args&... args)
{
    const auto val = static_cast<T>(static_cast<T>(dat) & keepMask);
    const auto shifter = bits * idx;
    out |= (val << shifter);
    if constexpr (sizeof...(Args) > 0)
        PackBitsByIndex(out, keepMask, bits, args...);
}

}

template<typename S, uint32_t Bits>
struct BitsPack
{
    static_assert(std::is_integral_v<S>&& std::is_unsigned_v<S>);
    static_assert(Bits < sizeof(S) * 8);
    S Storage;
    constexpr BitsPack(S val) noexcept : Storage(val) {}
    template<typename T>
    [[nodiscard]] forceinline constexpr T Get(uint32_t index) const noexcept
    {
        constexpr auto OutBits = sizeof(T) * 8;
        static_assert(Bits <= OutBits);
        using U = decltype(detail::ChooseUIntTypeByBits<OutBits>());
        constexpr U AllOnes = static_cast<U>(~U(0));
        constexpr U KeepMask = AllOnes >> (OutBits - Bits);
        const auto val = Storage >> (index * Bits);
        return static_cast<T>(static_cast<U>(val) & KeepMask);
    }
};

template<uint32_t Bits, typename... Args>
inline constexpr auto BitsPackFrom(Args&&... args) noexcept
{
    constexpr auto Count = sizeof...(Args);
    using T = decltype(detail::ChooseUIntTypeByBits<Count * Bits>());
    constexpr T AllOnes = static_cast<T>(~T(0));
    constexpr T KeepMask = AllOnes >> (sizeof(T) * 8 - Bits);
    T val = 0;
    uint32_t shifter = 0;
    (..., detail::PackBitsInSequence(val, static_cast<T>(static_cast<T>(args) & KeepMask), shifter, Bits));
    return BitsPack<T, Bits>(val);
}

template<uint32_t Bits, size_t Count, typename... Args>
inline constexpr auto BitsPackFromIndexed(Args&&... args) noexcept
{
    constexpr auto ArgCount = sizeof...(Args);
    static_assert(ArgCount % 2 == 0, "need (idx, val) pair");
    using T = decltype(detail::ChooseUIntTypeByBits<Count * Bits>());
    constexpr T AllOnes = static_cast<T>(~T(0));
    constexpr T KeepMask = AllOnes >> (sizeof(T) * 8 - Bits);
    T val = 0;
    detail::PackBitsByIndex(val, KeepMask, Bits, std::forward<Args>(args)...);
    return BitsPack<T, Bits>(val);
}

}
