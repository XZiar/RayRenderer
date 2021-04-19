#pragma once
#include "SystemCommonRely.h"
#include <cstdlib>

namespace common
{

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class SYSCOMMONAPI MiscIntrins
{
private:
    using VarItem = std::pair<std::string_view, std::string_view>;
    uint32_t (*LeadZero32)(const uint32_t) noexcept = nullptr;
    uint32_t (*LeadZero64)(const uint64_t) noexcept = nullptr;
    uint32_t (*TailZero32)(const uint32_t) noexcept = nullptr;
    uint32_t (*TailZero64)(const uint64_t) noexcept = nullptr;
    uint32_t (*PopCount32)(const uint32_t) noexcept = nullptr;
    uint32_t (*PopCount64)(const uint64_t) noexcept = nullptr;
    std::vector<VarItem> VariantMap;
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
    [[nodiscard]] static common::span<const VarItem> GetSupportMap() noexcept;
    MiscIntrins(common::span<const VarItem> requests) noexcept;
    MiscIntrins() noexcept : MiscIntrins(GetSupportMap()) { }
    [[nodiscard]] common::span<const VarItem> GetIntrinMap() const noexcept
    {
        return VariantMap;
    }
    [[nodiscard]] bool IsComplete() const noexcept
    {
        return LeadZero32 && LeadZero64 && TailZero32 && TailZero64 && PopCount32 && PopCount64;
    }

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


class SYSCOMMONAPI DigestFuncs
{
public:
    using VarItem = std::pair<std::string_view, std::string_view>;
    template<size_t N>
    using bytearray = std::array<std::byte, N>;
private:
    bytearray<32>(*Sha256)(const std::byte*, const size_t) noexcept = nullptr;
    std::vector<VarItem> VariantMap;
public:
    [[nodiscard]] static common::span<const VarItem> GetSupportMap() noexcept;
    DigestFuncs(common::span<const VarItem> requests) noexcept;
    DigestFuncs() noexcept : DigestFuncs(GetSupportMap()) { }
    [[nodiscard]] common::span<const VarItem> GetIntrinMap() const noexcept
    {
        return VariantMap;
    }
    [[nodiscard]] bool IsComplete() const noexcept
    {
        return Sha256;
    }

    template<typename T>
    bytearray<32> SHA256(const common::span<T> data) const noexcept
    {
        const common::span<const std::byte> bytes = common::as_bytes(data);
        return Sha256(bytes.data(), bytes.size());
    }
};

SYSCOMMONAPI extern const DigestFuncs DigestFunc;


#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

}