#pragma once

#pragma unmanaged

#include "RenderCore/RenderCore.h"
#include "common/Exceptions.hpp"
#include "common/miniLogger/miniLogger.h"

using namespace common;
using std::vector;

template<typename T>
forceinline void SetControllable(rayr::Controllable& control, const std::string& id, T&& arg)
{
    control.ControllableSet(id, std::forward<T>(arg));
}
forceinline void SetControllableVec4(rayr::Controllable& control, const std::string& id, const float x, const float y, const float z, const float w)
{
    control.ControllableSet(id, miniBLAS::Vec4(x, y, z, w));
}
forceinline void SetControllableVec3(rayr::Controllable& control, const std::string& id, const float x, const float y, const float z)
{
    control.ControllableSet(id, miniBLAS::Vec3(x, y, z));
}

#pragma managed


forceinline void SetControllableVec4(rayr::Controllable& control, const std::string& id, System::Windows::Media::Color color)
{
    SetControllableVec4(control, id, color.ScR, color.ScG, color.ScB, color.ScA);
}
forceinline void SetControllableVec3(rayr::Controllable& control, const std::string& id, System::Windows::Media::Color color)
{
    SetControllableVec3(control, id, color.ScR, color.ScG, color.ScB);
}


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
