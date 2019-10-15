#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef RAYCORE_EXPORT
#   define RAYCOREAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define RAYCOREAPI _declspec(dllimport)
# endif
#else
# ifdef RAYCORE_EXPORT
#   define RAYCOREAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define RAYCOREAPI
# endif
#endif


#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "common/Wrapper.hpp"
#include "common/AlignedContainer.hpp"
#include "common/Controllable.hpp"
#include "common/ContainerEx.hpp"
#include "common/Exceptions.hpp"
#include "SystemCommon/FileEx.h"
#include "common/Linq2.hpp"
#include "common/StringEx.hpp"
#include "common/PromiseTask.hpp"
#include "SystemCommon/ThreadEx.h"
#include "OpenGLUtil/OpenGLUtil.h"
#include "OpenCLUtil/OpenCLUtil.h"
#include "OpenCLInterop/GLInterop.h"
#include "ImageUtil/ImageUtil.h"
#include "ResourcePackager/ResourceUtil.h"
#include "ResourcePackager/SerializeUtil.h"
#include "TextureUtil/TexUtilRely.h"
#include "3rdParty/boost.stacktrace/stacktrace.h"
#include "StringCharset/Convert.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <tuple>
#include <array>
#include <map>
#include <unordered_map>
#include <set>
#include <deque>
#include <variant>
#include <typeindex>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace rayr
{
namespace str = common::str;
namespace fs = common::fs;
namespace ejson = xziar::ejson;
namespace strchset = common::strchset;
using std::byte;
using std::wstring;
using std::string;
using std::u16string;
using std::string_view;
using std::u16string_view;
using std::map;
using std::set;
using std::deque;
using std::vector;
using std::array;
using std::variant;
using common::min;
using common::max;
using common::Controllable;
using common::Wrapper;
using common::SimpleTimer;
using common::NonCopyable;
using common::NonMovable;
using common::PromiseResult;
using common::BaseException;
using common::container::vectorEx;
using common::file::FileException;
using str::Charset;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;

template<class T, class... Args>
using CallbackInvoke = std::function<void(std::function<T(Args...)>)>;

namespace detail
{

struct JsonConv : ejson::JsonConvertor
{
    EJSONCOV_TOVAL_BEGIN
    {
        using ejson::JArray;
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
forceinline ejson::JArray ToJArray(T& handle, const b3d::Coord2D& vec)
{
    auto ret = handle.NewArray();
    ret.Push(vec.u, vec.v);
    return ret;
}
template<typename T>
forceinline ejson::JArray ToJArray(T& handle, const miniBLAS::Vec3& vec)
{
    auto ret = handle.NewArray();
    ret.Push(vec.x, vec.y, vec.z);
    return ret;
}
template<typename T>
forceinline ejson::JArray ToJArray(T& handle, const miniBLAS::Vec4& vec)
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

#ifdef RAYCORE_EXPORT
#include "MiniLogger/MiniLogger.h"
namespace rayr
{
common::mlog::MiniLogger<false>& dizzLog();
string getShaderFromDLL(int32_t id);
}
#endif
