#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef IMGUTIL_EXPORT
#   define IMGUTILAPI _declspec(dllexport)
# else
#   define IMGUTILAPI _declspec(dllimport)
# endif
#else
# define IMGUTILAPI [[gnu::visibility("default")]]
#endif


#include "common/CommonRely.hpp"
#include "common/AlignedBuffer.hpp"
#include "common/EnumEx.hpp"
#include "SystemCommon/Exceptions.h"
#include "common/MemoryStream.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <tuple>

namespace xziar::img::util
{

forceinline constexpr uint16_t ParseWordLE(const std::byte* __restrict data) noexcept
{
    return static_cast<uint16_t>((std::to_integer<uint16_t>(data[1]) << 8) + std::to_integer<uint16_t>(data[0]));
}
forceinline constexpr uint16_t ParseWordBE(const std::byte* __restrict data) noexcept
{
    return static_cast<uint16_t>(std::to_integer<uint16_t>(data[1]) + std::to_integer<uint16_t>(data[0] << 8));
}
forceinline constexpr void WordToLE(std::byte* __restrict output, const uint16_t data) noexcept
{
    output[0] = std::byte(data & 0xff); output[1] = std::byte(data >> 8);
}
forceinline constexpr void WordToBE(std::byte* __restrict output, const uint16_t data) noexcept
{
    output[1] = std::byte(data & 0xff); output[0] = std::byte(data >> 8);
}
forceinline constexpr uint32_t ParseDWordLE(const std::byte* __restrict data) noexcept
{
    return static_cast<uint32_t>(
        (std::to_integer<uint32_t>(data[3]) << 24) +
        (std::to_integer<uint32_t>(data[2]) << 16) +
        (std::to_integer<uint32_t>(data[1]) << 8) +
        (std::to_integer<uint32_t>(data[0])));
}
forceinline constexpr uint32_t ParseDWordBE(const std::byte* __restrict data) noexcept
{
    return static_cast<uint32_t>(
        (std::to_integer<uint32_t>(data[3])) +
        (std::to_integer<uint32_t>(data[2]) << 8) +
        (std::to_integer<uint32_t>(data[1]) << 16) +
        (std::to_integer<uint32_t>(data[0]) << 24));
}
forceinline constexpr void DWordToLE(std::byte* __restrict output, const uint32_t data) noexcept
{
    output[0] = std::byte(data & 0xff); output[1] = std::byte(data >> 8); output[2] = std::byte(data >> 16); output[3] = std::byte(data >> 24);
}
forceinline constexpr void DWordToBE(std::byte* __restrict output, const uint32_t data) noexcept
{
    output[3] = std::byte(data & 0xff); output[2] = std::byte(data >> 8); output[1] = std::byte(data >> 16); output[0] = std::byte(data >> 24);
}

template<typename T>
forceinline T EmptyStruct() noexcept
{
    T obj;
    memset(&obj, 0, sizeof(T));
    return obj;
}

forceinline constexpr void FixAlpha(size_t count, uint32_t* __restrict destPtr) noexcept
{
    while (count--)
        (*destPtr++) |= 0xff000000u;
}

}
