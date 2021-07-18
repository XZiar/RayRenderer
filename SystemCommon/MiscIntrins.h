#pragma once
#include "SystemCommonRely.h"
#include <cstdlib>

namespace common
{


class MiscIntrins : public RuntimeFastPath<MiscIntrins>
{
    friend RuntimeFastPath<MiscIntrins>;
public:
    using TLeadZero32 = uint32_t(const uint32_t) noexcept;
    using TLeadZero64 = uint32_t(const uint64_t) noexcept;
    using TTailZero32 = uint32_t(const uint32_t) noexcept;
    using TTailZero64 = uint32_t(const uint64_t) noexcept;
    using TPopCount32 = uint32_t(const uint32_t) noexcept;
    using TPopCount64 = uint32_t(const uint64_t) noexcept;
private:
    TLeadZero32* LeadZero32 = nullptr;
    TLeadZero64* LeadZero64 = nullptr;
    TTailZero32* TailZero32 = nullptr;
    TTailZero64* TailZero64 = nullptr;
    TPopCount32* PopCount32 = nullptr;
    TPopCount64* PopCount64 = nullptr;
#if COMMON_COMPILER_MSVC
    [[nodiscard]] forceinline uint16_t ByteSwap16(const uint16_t num) const noexcept
    { 
        return _byteswap_ushort(num);
    }
    [[nodiscard]] forceinline uint32_t ByteSwap32(const uint32_t num) const noexcept
    { 
        return _byteswap_ulong(num);
    }
    [[nodiscard]] forceinline uint64_t ByteSwap64(const uint64_t num) const noexcept
    { 
        return _byteswap_uint64(num);
    }
#elif COMMON_COMPILER_GCC || COMMON_COMPILER_CLANG
    [[nodiscard]] forceinline uint16_t ByteSwap16(const uint16_t num) const noexcept
    { 
        return __builtin_bswap16(num);
    }
    [[nodiscard]] forceinline uint32_t ByteSwap32(const uint32_t num) const noexcept
    { 
        return __builtin_bswap32(num);
    }
    [[nodiscard]] forceinline uint64_t ByteSwap64(const uint64_t num) const noexcept
    { 
        return __builtin_bswap64(num);
    }
#else
#endif
public:
    SYSCOMMONAPI [[nodiscard]] static common::span<const PathInfo> GetSupportMap() noexcept;
    SYSCOMMONAPI MiscIntrins(common::span<const VarItem> requests = {}) noexcept;
    SYSCOMMONAPI ~MiscIntrins();
    SYSCOMMONAPI [[nodiscard]] bool IsComplete() const noexcept override;

    template<typename T>
    [[nodiscard]] forceinline uint32_t LeadZero(const T num) const noexcept
    {
        static_assert(std::is_integral_v<T>, "only integer allowed");
        if constexpr (sizeof(T) == 1)
            return std::min(LeadZero32(static_cast<uint8_t>(num)), 8u);
        else if constexpr (sizeof(T) == 2)
            return std::min(LeadZero32(static_cast<uint16_t>(num)), 16u);
        else if constexpr (sizeof(T) == 4)
            return LeadZero32(static_cast<uint32_t>(num));
        else if constexpr (sizeof(T) == 8)
            return LeadZero64(static_cast<uint64_t>(num));
        else
            static_assert(AlwaysTrue<T>, "datatype larger than 64 bit is not supported");
    }
    template<typename T>
    [[nodiscard]] forceinline uint32_t TailZero(const T num) const noexcept
    {
        static_assert(std::is_integral_v<T>, "only integer allowed");
        if constexpr (sizeof(T) == 1)
            return TailZero32(static_cast<uint8_t>(num));
        else if constexpr (sizeof(T) == 2)
            return TailZero32(static_cast<uint16_t>(num));
        else if constexpr (sizeof(T) == 4)
            return TailZero32(static_cast<uint32_t>(num));
        else if constexpr (sizeof(T) == 8)
            return TailZero64(static_cast<uint64_t>(num));
        else
            static_assert(AlwaysTrue<T>, "datatype larger than 64 bit is not supported");
    }
    template<typename T>
    [[nodiscard]] forceinline uint32_t PopCount(const T num) const noexcept
    {
        static_assert(std::is_integral_v<T>, "only integer allowed");
        if constexpr (sizeof(T) == 1)
            return PopCount32(static_cast<uint8_t>(num));
        else if constexpr (sizeof(T) == 2)
            return PopCount32(static_cast<uint16_t>(num));
        else if constexpr (sizeof(T) == 4)
            return PopCount32(static_cast<uint32_t>(num));
        else if constexpr (sizeof(T) == 8)
            return PopCount64(static_cast<uint64_t>(num));
        else
            static_assert(AlwaysTrue<T>, "datatype larger than 64 bit is not supported");
    }
    template<typename T>
    [[nodiscard]] forceinline T ByteSwap(const T num) const noexcept
    {
        if constexpr (sizeof(T) == 2)
            return ByteSwap16(static_cast<uint16_t>(num));
        else if constexpr (sizeof(T) == 4)
            return ByteSwap32(static_cast<uint32_t>(num));
        else if constexpr (sizeof(T) == 8)
            return ByteSwap64(static_cast<uint64_t>(num));
        else
            static_assert(AlwaysTrue<T>, "datatype larger than 64 bit is not supported");
    }
};

SYSCOMMONAPI extern const MiscIntrins MiscIntrin;


class DigestFuncs : public RuntimeFastPath<DigestFuncs>
{
    friend RuntimeFastPath<DigestFuncs>;
public:
    template<size_t N>
    using bytearray = std::array<std::byte, N>;
    using TSha256 = bytearray<32>(const std::byte*, const size_t) noexcept;
private:
    TSha256* Sha256 = nullptr;
public:
    SYSCOMMONAPI [[nodiscard]] static common::span<const PathInfo> GetSupportMap() noexcept;
    SYSCOMMONAPI DigestFuncs(common::span<const VarItem> requests = {}) noexcept;
    SYSCOMMONAPI ~DigestFuncs();
    SYSCOMMONAPI [[nodiscard]] bool IsComplete() const noexcept override;

    template<typename T>
    bytearray<32> SHA256(const common::span<T> data) const noexcept
    {
        const common::span<const std::byte> bytes = common::as_bytes(data);
        return Sha256(bytes.data(), bytes.size());
    }
};

SYSCOMMONAPI extern const DigestFuncs DigestFunc;


}