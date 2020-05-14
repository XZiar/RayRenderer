#pragma once
#include "SystemCommonRely.h"

namespace common
{


class MiscIntrins
{
    friend uint32_t InnerInject() noexcept;
private:
    static SYSCOMMONAPI uint32_t (*LeadZero32)(const uint32_t) noexcept;
    static SYSCOMMONAPI uint32_t (*LeadZero64)(const uint64_t) noexcept;
    static SYSCOMMONAPI uint32_t (*TailZero32)(const uint32_t) noexcept;
    static SYSCOMMONAPI uint32_t (*TailZero64)(const uint64_t) noexcept;
    static SYSCOMMONAPI uint32_t (*PopCount32)(const uint32_t) noexcept;
    static SYSCOMMONAPI uint32_t (*PopCount64)(const uint64_t) noexcept;
    static void InitIntrins() noexcept;
public:
    [[nodiscard]] static SYSCOMMONAPI common::span<std::pair<std::string_view, std::string_view>> GetIntrinMap() noexcept;

    template<typename T>
    [[nodiscard]] static forceinline uint32_t LeadZero(const T num) noexcept
    {
        static_assert(std::is_integral_v<T>, "only integer allowed");
        if constexpr (sizeof(T) == 1)
            return LeadZero32(static_cast<uint8_t>(num));
        else if constexpr (sizeof(T) == 2)
            return LeadZero32(static_cast<uint16_t>(num));
        else if constexpr (sizeof(T) == 4)
            return LeadZero32(static_cast<uint32_t>(num));
        else if constexpr (sizeof(T) == 8)
            return LeadZero64(static_cast<uint64_t>(num));
        else
            static_assert(AlwaysTrue<T>, "datatype larger than 64 bit is not supported");
    }
    template<typename T>
    [[nodiscard]] static forceinline uint32_t TailZero(const T num) noexcept
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
    [[nodiscard]] static forceinline uint32_t PopCount(const T num) noexcept
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
};


}