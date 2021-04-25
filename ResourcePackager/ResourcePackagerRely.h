#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef RESPAK_EXPORT
#   define RESPAKAPI _declspec(dllexport)
# else
#   define RESPAKAPI _declspec(dllimport)
# endif
#else
# define RESPAKAPI [[gnu::visibility("default")]]
#endif


#include "common/CommonRely.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <array>
#include <vector>

namespace xziar::respak
{
template<size_t N>
using bytearray = std::array<std::byte, N>;


namespace detail
{


struct SHA256Hash
{
public:
    constexpr size_t operator()(const bytearray<32>& sha256) const noexcept
    {
        size_t hash = 0;
        for (size_t i = 0; i < 32; ++i)
            hash = hash * 31 + std::to_integer<size_t>(sha256[i]);
        return hash;
    }
};

template<typename T>
constexpr inline void ToLE(T intva, uint8_t* output)
{
    for (size_t i = 0; i < sizeof(T); ++i, intva /= 256)
        output[i] = static_cast<uint8_t>(intva);
}

template<typename T>
constexpr inline bytearray<sizeof(T)> ToLEByteArray(const T raw)
{
    auto intval = static_cast<uint64_t>(raw);
    bytearray<sizeof(T)> output{};
    for (size_t i = 0; i < sizeof(T); ++i, intval >>= 8)
        output[i] = std::byte(intval);
    return output;
}
template<typename T>
constexpr inline T FromLEByteArray(const bytearray<sizeof(T)>& input)
{
    uint64_t tmp = 0;
    for (size_t i = 0; i < sizeof(T); ++i)
        tmp += std::to_integer<uint32_t>(input[i]) << (i * 8);
    return static_cast<T>(tmp);
}

}

}


