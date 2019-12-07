#pragma once

#pragma unmanaged

#define HALF_ENABLE_CPP11_CFENV 0

#include "RenderCore/RenderCore.h"
#include "RenderCore/SceneManager.h"
#include "RenderCore/RenderPass.h"
#include "RenderCore/TextureLoader.h"
#include "RenderCore/ThumbnailManager.h"
#include "ImageUtil/ImageUtil.h"
#include "MiniLogger/MiniLogger.h"
#include "common/Exceptions.hpp"

using std::vector;

#pragma managed


#include "common/CLICommonRely.hpp"
#include "common/CLIViewModel.hpp"
using namespace System;
using namespace Common;


forceinline System::Windows::Media::Color ToColor(const miniBLAS::Vec3& color)
{
    return System::Windows::Media::Color::FromScRgb(1.0f, color.x, color.y, color.z);
}
forceinline System::Windows::Media::Color ToColor(const miniBLAS::Vec4& color)
{
    return System::Windows::Media::Color::FromScRgb(color.w, color.x, color.y, color.z);
}
forceinline System::Windows::Media::Color ToColor(const std::tuple<float, float, float>& vec)
{
    return System::Windows::Media::Color::FromScRgb(1.0f, std::get<0>(vec), std::get<1>(vec), std::get<2>(vec));
}
forceinline System::Windows::Media::Color ToColor(const std::tuple<float, float, float, float>& vec)
{
    return System::Windows::Media::Color::FromScRgb(std::get<3>(vec), std::get<0>(vec), std::get<1>(vec), std::get<2>(vec));
}
forceinline void FromColor(System::Windows::Media::Color value, miniBLAS::Vec4& color)
{
    color.x = value.ScR, color.y = value.ScG, color.z = value.ScB, color.w = value.ScA;
}
forceinline void FromColor(System::Windows::Media::Color value, miniBLAS::Vec3& color)
{
    color.x = value.ScR, color.y = value.ScG, color.z = value.ScB;
}

forceinline System::Guid^ ToGuid(const boost::uuids::uuid& uid)
{
    array<byte>^ raw = gcnew array<byte>(16);
    {
        cli::pin_ptr<byte> ptr = &raw[0];
        memcpy_s(ptr, 16, &uid, 16);
    }
    return gcnew System::Guid(raw);
}
forceinline boost::uuids::uuid FromGuid(System::Guid^ uid)
{
    boost::uuids::uuid ret;
    {
        cli::pin_ptr<byte> ptr = &uid->ToByteArray()[0];
        memcpy_s(&ret, 16, ptr, 16);
    }
    return ret;
}

forceinline String^ ToStr(const fmt::basic_memory_buffer<char16_t>& strBuffer)
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
forceinline Vector3 ToVector3(const std::tuple<float, float, float>& vec)
{
    return Vector3(std::get<0>(vec), std::get<1>(vec), std::get<2>(vec));
}
forceinline void StoreVector3(Vector3% val, miniBLAS::Vec3& vec)
{
    vec.x = val.X, vec.y = val.Y, vec.z = val.Z;
}
forceinline Vector4 ToVector4(const miniBLAS::Vec4& vec)
{
    return Vector4(vec.x, vec.y, vec.z, vec.w);
}
forceinline Vector4 ToVector4(const std::tuple<float, float, float, float>& vec)
{
    return Vector4(std::get<0>(vec), std::get<1>(vec), std::get<2>(vec), std::get<3>(vec));
}
forceinline void StoreVector4(Vector4% val, miniBLAS::Vec4& vec)
{
    vec.x = val.X, vec.y = val.Y, vec.z = val.Z, vec.w = val.W;
}


template<typename T, typename... Args>
forceinline T* TryConstruct(Args&&... args)
{
    try
    {
        return new T(std::forward<Args>(args)...);
    }
    catch (common::BaseException & be)
    {
        throw gcnew Common::CPPException(be);
    }
}
