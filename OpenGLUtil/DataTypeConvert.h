#pragma once

#include "oglRely.h"
#include "3DElement.hpp"

namespace oglu
{
namespace convert
{

OGLUAPI void ConvertToHalf(const float* __restrict src, b3d::half* __restrict dst, size_t size);
forceinline std::vector<b3d::half> ConvertToHalf(const float* __restrict src, const size_t size)
{
    std::vector<b3d::half> ret(size);
    ConvertToHalf(src, ret.data(), size);
    return ret;
}
forceinline std::vector<b3d::half> ConvertToHalf(const common::span<const float> src)
{
    return ConvertToHalf(src.data(), src.size());
}
forceinline void ConvertToHalf(const common::span<const float> src, b3d::half* __restrict dst, const size_t size)
{
    ConvertToHalf(src.data(), dst, std::min<size_t>(src.size(), size));
}

OGLUAPI void ConvertFromHalf(const b3d::half* __restrict src, float* __restrict dst, size_t size);
forceinline std::vector<float> ConvertFromHalf(const b3d::half* __restrict src, const size_t size)
{
    std::vector<float> ret(size);
    ConvertFromHalf(src, ret.data(), size);
    return ret;
}
forceinline std::vector<float> ConvertFromHalf(const common::span<const b3d::half> src)
{
    return ConvertFromHalf(src.data(), src.size());
}
forceinline void ConvertFromHalf(const common::span<const b3d::half> src, float* __restrict dst, const size_t size)
{
    ConvertFromHalf(src.data(), dst, std::min<size_t>(src.size(), size));
}


}
}
