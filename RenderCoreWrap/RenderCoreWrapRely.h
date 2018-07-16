#pragma once

#pragma unmanaged

#include "RenderCore/RenderCore.h"
#include "common/Exceptions.hpp"
#include "common/miniLogger/miniLogger.h"

using namespace common;
using std::vector;

#pragma managed

#include "common/CLICommonRely.hpp"
#include "common/CLIViewModel.hpp"

inline System::Windows::Media::Color ToColor(const miniBLAS::Vec4& color)
{
    return System::Windows::Media::Color::FromScRgb(color.w, color.x, color.y, color.z);
}
inline System::Windows::Media::Color ToColor(const miniBLAS::Vec3& color)
{
    return System::Windows::Media::Color::FromScRgb(1.0f, color.x, color.y, color.z);
}
inline void FromColor(System::Windows::Media::Color value, miniBLAS::Vec4& color)
{
    color.x = value.ScR, color.y = value.ScG, color.z = value.ScB, color.w = value.ScA;
}
inline void FromColor(System::Windows::Media::Color value, miniBLAS::Vec3& color)
{
    color.x = value.ScR, color.y = value.ScG, color.z = value.ScB;
}

forceinline System::String^ ToStr(const fmt::internal::basic_buffer<char16_t>& strBuffer)
{
    return gcnew System::String(reinterpret_cast<const wchar_t*>(strBuffer.data()), 0, (int)strBuffer.size());
}
