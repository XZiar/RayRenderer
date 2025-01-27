#pragma once
#include "SystemCommonRely.h"
#include <cstdlib>

namespace common
{


class MiscIntrins final : public RuntimeFastPath<MiscIntrins>
{
    friend ::common::fastpath::PathHack;
private:
    uint32_t(*LeadZero32)(const uint32_t) noexcept = nullptr;
    uint32_t(*LeadZero64)(const uint64_t) noexcept = nullptr;
    uint32_t(*TailZero32)(const uint32_t) noexcept = nullptr;
    uint32_t(*TailZero64)(const uint64_t) noexcept = nullptr;
    uint32_t(*PopCount32)(const uint32_t) noexcept = nullptr;
    uint32_t(*PopCount64)(const uint64_t) noexcept = nullptr;
    uint32_t(*PopCounts )(const std::byte* data, size_t count) noexcept = nullptr;
    std::string(*Hex2Str)(const uint8_t* data, size_t size, bool isCapital) noexcept = nullptr;
    bool(*PauseCycles)(uint32_t) noexcept = nullptr;
public:
    SYSCOMMONAPI [[nodiscard]] static common::span<const PathInfo> GetSupportMap() noexcept;
    SYSCOMMONAPI MiscIntrins(common::span<const VarItem> requests = {}) noexcept;
    SYSCOMMONAPI ~MiscIntrins();
    SYSCOMMONAPI [[nodiscard]] bool IsComplete() const noexcept final;

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
            static_assert(!AlwaysTrue<T>, "datatype larger than 64 bit is not supported");
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
            static_assert(!AlwaysTrue<T>, "datatype larger than 64 bit is not supported");
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
            static_assert(!AlwaysTrue<T>, "datatype larger than 64 bit is not supported");
    }
    template<typename T>
    [[nodiscard]] forceinline uint32_t PopCountRange(span<const T> space) const noexcept
    {
        const auto byteCount = space.size_bytes();
        Expects(byteCount < UINT32_MAX / 8);
        if (byteCount == 8 && PopCount64)
            return PopCount64(*reinterpret_cast<const uint64_t*>(space.data()));
        else if (byteCount == 4 && PopCount32)
            return PopCount32(*reinterpret_cast<const uint32_t*>(space.data()));
        else
            return PopCounts(reinterpret_cast<const std::byte*>(space.data()), byteCount);
    }
    [[nodiscard]] forceinline std::string HexToStr(const void* data, const size_t size, bool isCapital = false) const noexcept
    {
        return Hex2Str(reinterpret_cast<const uint8_t*>(data), size, isCapital);
    }
    [[nodiscard]] forceinline std::string HexToStr(common::span<const std::byte> data, bool isCapital = false) const noexcept
    {
        return Hex2Str(reinterpret_cast<const uint8_t*>(data.data()), data.size(), isCapital);
    }
    forceinline bool Pause(uint32_t cycles) const noexcept
    {
        return PauseCycles(cycles);
    }
};

SYSCOMMONAPI extern const MiscIntrins MiscIntrin;


class DigestFuncs final : public RuntimeFastPath<DigestFuncs>
{
    friend ::common::fastpath::PathHack;
public:
    template<size_t N>
    using bytearray = std::array<std::byte, N>;
private:
    bytearray<32>(*Sha256)(const std::byte*, const size_t) noexcept = nullptr;
public:
    SYSCOMMONAPI [[nodiscard]] static common::span<const PathInfo> GetSupportMap() noexcept;
    SYSCOMMONAPI DigestFuncs(common::span<const VarItem> requests = {}) noexcept;
    SYSCOMMONAPI ~DigestFuncs();
    SYSCOMMONAPI [[nodiscard]] bool IsComplete() const noexcept final;

    template<typename T>
    bytearray<32> SHA256(const common::span<T> data) const noexcept
    {
        const common::span<const std::byte> bytes = common::as_bytes(data);
        return Sha256(bytes.data(), bytes.size());
    }
};

SYSCOMMONAPI extern const DigestFuncs DigestFunc;


}