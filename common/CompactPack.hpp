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
    static_assert(std::is_integral_v<S> && std::is_unsigned_v<S>);
    static_assert(Bits < sizeof(S) * 8);
    static constexpr S KeepMask = (S(1) << Bits) - S(1);
    S Storage;
    constexpr BitsPack(S val) noexcept : Storage(val) {}
    template<typename T = S>
    [[nodiscard]] forceinline constexpr T Get(uint32_t index) const noexcept
    {
        constexpr auto OutBits = sizeof(T) * 8;
        static_assert(Bits <= OutBits);
        if constexpr (std::is_same_v<T, bool>)
        {
            const auto mask = static_cast<S>(KeepMask << (index * Bits));
            return Storage & mask;
        }
        else
        {
#if defined(COMMON_SIMD_LV) && COMMON_ARCH_X86 && (COMMON_COMPILER_MSVC || defined(__BMI__)) // enforce BMI support
            if (!is_constant_evaluated(true))
            {
                const uint32_t control = (index * Bits) + (Bits << 8);
                if constexpr (sizeof(S) > sizeof(uint32_t))
                    return static_cast<T>(_bextr2_u64(Storage, control));
                else
                    return static_cast<T>(_bextr2_u32(Storage, control));
            }
#endif
            const auto val = Storage >> (index * Bits);
            return static_cast<T>(val & KeepMask);
        }
    }
    [[nodiscard]] forceinline constexpr S GetWithoutMask(uint32_t index) const noexcept
    {
        const auto val = Storage >> (index * Bits);
        return static_cast<S>(val);
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

template<size_t Count, typename... Args>
inline constexpr auto BitsPackFromIndexes(Args&&... args) noexcept
{
    using T = decltype(detail::ChooseUIntTypeByBits<Count>());
    T val = 0;
    (..., void(val = static_cast<T>((T(1) << args) | val)));
    return BitsPack<T, 1>(val);
}

template<size_t Count, typename U>
inline constexpr auto BitsPackFromIndexesArray(const U* ptr, const size_t count, int64_t offset = 0) noexcept
{
    using T = decltype(detail::ChooseUIntTypeByBits<Count>());
    using V = decltype(detail::ChooseUIntTypeByBits<sizeof(U)>());
    static_assert(sizeof(U) < sizeof(int64_t));
    T val = 0;
    for (size_t i = 0; i < count; ++i)
    {
        const int64_t idx = static_cast<V>(ptr[i]) + offset;
        if (idx > 0)
        {
            val = static_cast<T>((T(1) << idx) | val);
        }
    }
    return BitsPack<T, 1>(val);
}

}
