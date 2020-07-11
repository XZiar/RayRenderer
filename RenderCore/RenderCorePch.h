#pragma once
#include "RenderCoreRely.h"
#include "TextureUtil/TexCompressor.h"
#include "TextureUtil/TexMipmap.h"
#include "TextureUtil/TexResizer.h"
#include "TextureUtil/TexUtilWorker.h"
#include "OpenCLInterop/GLInterop.h"
#include "OpenGLUtil/PointEnhance.hpp"
#include "OpenGLUtil/oglWorker.h"
#include "ImageUtil/ImageUtil.h"
#include "AsyncExecutor/AsyncManager.h"
#include "ResourcePackager/ResourceUtil.h"
#include "MiniLogger/MiniLogger.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/ThreadEx.h"
#include "StringUtil/Convert.h"
#include "StringUtil/Detect.h"
#include "SystemCommon/PromiseTaskSTD.h"
#include "3rdParty/boost.stacktrace/stacktrace.h"
#include "common/Linq2.hpp"
#include "common/StringEx.hpp"
#include "common/StringLinq.hpp"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <tuple>
#include <unordered_map>
#include <deque>


namespace rayr
{


common::mlog::MiniLogger<false>& dizzLog();
std::string LoadShaderFromDLL(int32_t id);


namespace detail
{

struct JsonConv : xziar::ejson::JsonConvertor
{
    EJSONCOV_TOVAL_BEGIN
    {
        using xziar::ejson::JArray;
        if constexpr (std::is_same_v<PlainType, b3d::Coord2D>)
        {
            auto ret = handle.NewArray();
            ret.Push(val.u, val.v);
            return static_cast<rapidjson::Value>(ret);
        }
        else if constexpr (std::is_same_v<PlainType, miniBLAS::Vec3> || std::is_same_v<PlainType, b3d::Vec3>)
        {
            auto ret = handle.NewArray();
            ret.Push(val.x, val.y, val.z);
            return static_cast<rapidjson::Value>(ret);
        }
        else if constexpr (std::is_same_v<PlainType, miniBLAS::Vec4> || std::is_same_v<PlainType, b3d::Vec4>)
        {
            auto ret = handle.NewArray();
            ret.Push(val.x, val.y, val.z, val.w);
            return static_cast<rapidjson::Value>(ret);
        }
        else if constexpr (std::is_same_v<PlainType, boost::uuids::uuid>)
            return JsonConvertor::ToVal(boost::uuids::to_string(val), handle);
        else
            return JsonConvertor::ToVal(std::forward<T>(val), handle);
    }
    EJSONCOV_TOVAL_END

    EJSONCOV_FROMVAL
    {
        if constexpr (std::is_same_v<T, b3d::Coord2D>)
        {
            if (!value.IsArray() || value.Size() != 2) return false;
            JsonConvertor::FromVal(value[0], val.u);
            JsonConvertor::FromVal(value[1], val.v);
        }
        else if constexpr (std::is_same_v<T, miniBLAS::Vec3> || std::is_same_v<T, b3d::Vec3>)
        {
            if (!value.IsArray() || value.Size() != 3) return false;
            JsonConvertor::FromVal(value[0], val.x);
            JsonConvertor::FromVal(value[1], val.y);
            JsonConvertor::FromVal(value[2], val.z);
        }
        else if constexpr (std::is_same_v<T, miniBLAS::Vec4> || std::is_same_v<T, b3d::Vec4>)
        {
            if (!value.IsArray() || value.Size() != 4) return false;
            JsonConvertor::FromVal(value[0], val.x);
            JsonConvertor::FromVal(value[1], val.y);
            JsonConvertor::FromVal(value[2], val.z);
            JsonConvertor::FromVal(value[3], val.w);
        }
    /*else if constexpr (std::is_same_v<T, boost::uuids::uuid>)
    {
        static const boost::uuids::string_generator Generator;
        const auto ptr = value.GetString();
        return Generator(ptr, ptr + value.GetStringLength());
    }*/
    else
        return JsonConvertor::FromVal(value, val);
    return true;
    }
};

template<typename T>
forceinline xziar::ejson::JArray ToJArray(T& handle, const b3d::Coord2D& vec)
{
    auto ret = handle.NewArray();
    ret.Push(vec.u, vec.v);
    return ret;
}
template<typename T>
forceinline xziar::ejson::JArray ToJArray(T& handle, const miniBLAS::Vec3& vec)
{
    auto ret = handle.NewArray();
    ret.Push(vec.x, vec.y, vec.z);
    return ret;
}
template<typename T>
forceinline xziar::ejson::JArray ToJArray(T& handle, const miniBLAS::Vec4& vec)
{
    auto ret = handle.NewArray();
    ret.Push(vec.x, vec.y, vec.z, vec.w);
    return ret;
}
template<typename T>
forceinline void FromJArray(const T& jarray, b3d::Coord2D& vec)
{
    jarray.TryGetMany(0, vec.u, vec.v);
}
template<typename T>
forceinline void FromJArray(const T& jarray, miniBLAS::Vec3& vec)
{
    jarray.TryGetMany(0, vec.x, vec.y, vec.z);
}
template<typename T>
forceinline void FromJArray(const T& jarray, miniBLAS::Vec4& vec)
{
    jarray.TryGetMany(0, vec.x, vec.y, vec.z, vec.w);
}
}


}

