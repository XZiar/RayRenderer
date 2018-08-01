#pragma once

#ifdef RESPAK_EXPORT
#   define RESPAKAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define RESPAKAPI _declspec(dllimport)
#endif


#include "common/CommonRely.hpp"
#include "common/Exceptions.hpp"
#include "common/ContainerEx.hpp"
#include "common/FileEx.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <unordered_map>
#include <variant>

namespace xziar::respak
{
namespace str = common::str;
namespace fs = common::fs;
using common::file::FileObject;
using common::file::OpenFlag;
using common::file::BufferedFileReader;
using common::file::BufferedFileWriter;
using std::byte;
using std::wstring;
using std::string;
using std::u16string;
using std::string_view;
using std::array;
using std::vector;
using std::unordered_map;
using std::variant;
template<size_t N>
using bytearray = std::array<std::byte, N>;
using common::BaseException;
using common::FileException;
using common::NonCopyable;
using common::NonMovable;

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
constexpr inline bytearray<sizeof(T)> ToLEByteArray(T intva)
{
    bytearray<sizeof(T)> output;
    for (size_t i = 0; i < sizeof(T); ++i, intva /= 256)
        output[i] = byte(intva);
    return output;
}

}

}

#ifdef RESPAK_EXPORT
#   include "common/miniLogger/miniLogger.h"
namespace xziar::respak
{
common::mlog::MiniLogger<false>& rpakLog();
}
#endif
