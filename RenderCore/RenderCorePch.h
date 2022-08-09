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
#include "SystemCommon/AsyncManager.h"
#include "ResourcePackager/ResourceUtil.h"
#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/ThreadEx.h"
#include "SystemCommon/StackTrace.h"
#include "SystemCommon/PromiseTaskSTD.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/StringDetect.h"
#include "SystemCommon/Format.h"
#include "common/math/VecSIMD.hpp"
#include "common/math/MatSIMD.hpp"
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


namespace dizz
{
namespace msimd = common::math::simd;
#define FMTSTR2(syntax, ...) common::str::Formatter<char16_t>{}.FormatStatic(FmtString(syntax), __VA_ARGS__)


common::mlog::MiniLogger<false>& dizzLog();
std::string LoadShaderFromDLL(int32_t id);


namespace detail
{

struct JsonConv : xziar::ejson::JsonConvertor
{
    EJSONCOV_TOVAL_BEGIN
    {
        using xziar::ejson::JArray;
        if constexpr (std::is_same_v<PlainType, mbase::Vec2>)
        {
            auto ret = handle.NewArray();
            ret.Push(val.X, val.Y);
            return static_cast<rapidjson::Value>(ret);
        }
        else if constexpr (std::is_same_v<PlainType, mbase::Vec3>)
        {
            auto ret = handle.NewArray();
            ret.Push(val.X, val.Y, val.Z);
            return static_cast<rapidjson::Value>(ret);
        }
        else if constexpr (std::is_same_v<PlainType, mbase::Vec4>)
        {
            auto ret = handle.NewArray();
            ret.Push(val.X, val.Y, val.Z, val.W);
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
        if constexpr (std::is_same_v<T, mbase::Vec2>)
        {
            if (!value.IsArray() || value.Size() != 2) return false;
            JsonConvertor::FromVal(value[0], val.X);
            JsonConvertor::FromVal(value[1], val.Y);
        }
        else if constexpr (std::is_same_v<T, mbase::Vec3>)
        {
            if (!value.IsArray() || value.Size() != 3) return false;
            JsonConvertor::FromVal(value[0], val.X);
            JsonConvertor::FromVal(value[1], val.Y);
            JsonConvertor::FromVal(value[2], val.Z);
        }
        else if constexpr (std::is_same_v<T, mbase::Vec4>)
        {
            if (!value.IsArray() || value.Size() != 4) return false;
            JsonConvertor::FromVal(value[0], val.X);
            JsonConvertor::FromVal(value[1], val.Y);
            JsonConvertor::FromVal(value[2], val.Z);
            JsonConvertor::FromVal(value[3], val.W);
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
forceinline xziar::ejson::JArray ToJArray(T& handle, const mbase::Vec2& vec)
{
    auto ret = handle.NewArray();
    ret.Push(vec.X, vec.Y);
    return ret;
}
template<typename T>
forceinline xziar::ejson::JArray ToJArray(T& handle, const mbase::Vec3& vec)
{
    auto ret = handle.NewArray();
    ret.Push(vec.X, vec.Y, vec.Z);
    return ret;
}
template<typename T>
forceinline xziar::ejson::JArray ToJArray(T& handle, const mbase::Vec4& vec)
{
    auto ret = handle.NewArray();
    ret.Push(vec.X, vec.Y, vec.Z, vec.W);
    return ret;
}
template<typename T>
forceinline void FromJArray(const T& jarray, mbase::Vec2& vec)
{
    jarray.TryGetMany(0, vec.X, vec.Y);
}
template<typename T>
forceinline void FromJArray(const T& jarray, mbase::Vec3& vec)
{
    jarray.TryGetMany(0, vec.X, vec.Y, vec.Z);
}
template<typename T>
forceinline void FromJArray(const T& jarray, mbase::Vec4& vec)
{
    jarray.TryGetMany(0, vec.X, vec.Y, vec.Z, vec.W);
}
}


}

