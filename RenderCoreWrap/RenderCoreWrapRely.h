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
using namespace System;

template<typename T>
inline T ExchangeNullptr(T% pointer)
{
    return reinterpret_cast<T>(Threading::Interlocked::Exchange((IntPtr%)pointer, IntPtr::Zero).ToPointer());
}

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

forceinline String^ ToStr(const fmt::internal::basic_buffer<char16_t>& strBuffer)
{
    return gcnew String(reinterpret_cast<const wchar_t*>(strBuffer.data()), 0, (int)strBuffer.size());
}


using namespace System::Numerics;
forceinline Vector2 ToVector2(const std::pair<float, float>& pair)
{
    return Vector2(pair.first, pair.second);
}
forceinline void StoreVector2(Vector3% val, std::pair<float, float>& pair)
{
    pair.first = val.X, pair.second = val.Y;
}
forceinline Vector3 ToVector3(const miniBLAS::Vec3& vec)
{
    return Vector3(vec.x, vec.y, vec.z);
}
forceinline void StoreVector3(Vector3% val, miniBLAS::Vec3& vec)
{
    vec.x = val.X, vec.y = val.Y, vec.z = val.Z;
}
forceinline Vector4 ToVector4(const miniBLAS::Vec4& vec)
{
    return Vector4(vec.x, vec.y, vec.z, vec.w);
}
forceinline void StoreVector4(Vector4% val, miniBLAS::Vec4& vec)
{
    vec.x = val.X, vec.y = val.Y, vec.z = val.Z, vec.w = val.W;
}
