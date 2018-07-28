#pragma once
#include "ResourcePackagerRely.h"

namespace xziar::respak
{

class RESPAKAPI ResourceUtil
{
public:
    template<typename T>
    static string Hex2Str(const T& data)
    {
        return Hex2Str(data.data(), data.size() * sizeof(T::value_type));
    }
    static string Hex2Str(const void* data, const size_t size);
    template<typename T>
    static string TestSHA256(const T& data)
    {
        return TestSHA256(data.data(), data.size() * sizeof(T::value_type));
    }
    static string TestSHA256(const void* data, const size_t size);
};

}


