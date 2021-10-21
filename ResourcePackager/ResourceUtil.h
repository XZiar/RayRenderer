#pragma once
#include "ResourcePackagerRely.h"
#include "SystemCommon/MiscIntrins.h"

namespace xziar::respak
{

class RESPAKAPI ResourceUtil
{
public:
    static std::vector<std::byte> Str2Hex(const std::string_view& str);

    template<typename T>
    forceinline static std::string ToBase64(const T& data)
    {
        return ToBase64(data.data(), data.size() * sizeof(typename T::value_type));
    }
    static std::string ToBase64(const void* data, const size_t size);

    static std::vector<std::byte> FromBase64(const std::string_view& str);

    template<typename T>
    forceinline static bytearray<32> SHA256(const T& data)
    {
        return common::DigestFunc.SHA256(common::to_span(data));
    }
    forceinline static bytearray<32> SHA256(const void* data, const size_t size)
    {
        common::span<const std::byte> dat(reinterpret_cast<const std::byte*>(data), size);
        return common::DigestFunc.SHA256(dat);
    }
};

}


