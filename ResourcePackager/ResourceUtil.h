#pragma once
#include "ResourcePackagerRely.h"

namespace xziar::respak
{

class RESPAKAPI ResourceUtil
{
public:
    template<typename T>
    static std::string Hex2Str(const T& data)
    {
        return Hex2Str(data.data(), data.size() * sizeof(typename T::value_type));
    }
    static std::string Hex2Str(const void* data, const size_t size);

    static std::vector<std::byte> Str2Hex(const std::string_view& str);

    template<typename T>
    static std::string ToBase64(const T& data)
    {
        return ToBase64(data.data(), data.size() * sizeof(typename T::value_type));
    }
    static std::string ToBase64(const void* data, const size_t size);

    static std::vector<std::byte> FromBase64(const std::string_view& str);

    template<typename T>
    static bytearray<32> SHA256(const T& data)
    {
        return SHA256(data.data(), data.size() * sizeof(typename T::value_type));
    }
    static bytearray<32> SHA256(const void* data, const size_t size);
};

}


