#pragma once

#include "oglRely.h"
#include "3DElement.hpp"

namespace oglu
{
namespace convert
{

void ConvertToHalf(const float* __restrict src, b3d::half* __restrict dst, size_t size);
forceinline common::vectorEx<b3d::half> ConvertToHalf(const float* __restrict src, const size_t size)
{
    common::vectorEx<b3d::half> ret(size);
    ConvertToHalf(src, ret.data(), size);
    return ret;
}
template<typename A>
forceinline common::vectorEx<b3d::half> ConvertToHalf(const std::vector<float, A>& src)
{
    return ConvertToHalf(src.data(), src.size());
}
template<typename A>
forceinline void ConvertToHalf(const std::vector<float, A>& src, b3d::half* __restrict dst, const size_t size)
{
    ConvertToHalf(src.data(), dst, std::min(src.size(), size));
}

void ConvertFromHalf(const b3d::half* __restrict src, float* __restrict dst, size_t size);
forceinline common::vectorEx<float> ConvertFromHalf(const b3d::half* __restrict src, const size_t size)
{
    common::vectorEx<float> ret(size);
    ConvertFromHalf(src, ret.data(), size);
    return ret;
}
template<typename A>
forceinline common::vectorEx<float> ConvertFromHalf(const std::vector<b3d::half, A>& src)
{
    return ConvertFromHalf(src.data(), src.size());
}
template<typename A>
forceinline void ConvertFromHalf(const std::vector<b3d::half, A>& src, float* __restrict dst, const size_t size)
{
    ConvertFromHalf(src.data(), dst, std::min(src.size(), size));
}


}
}
