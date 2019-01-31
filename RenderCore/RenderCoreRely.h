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
#include "common/Wrapper.hpp"
#include "common/AlignedContainer.hpp"
#include "common/Controllable.hpp"
#include "common/ContainerEx.hpp"
#include "common/Exceptions.hpp"
#include "common/FileEx.hpp"
#include "common/Linq.hpp"
#include "common/ThreadEx.h"
#include "common/StringEx.hpp"
#include "common/PromiseTask.hpp"
#include "OpenGLUtil/OpenGLUtil.h"
#include "OpenCLUtil/OpenCLUtil.h"
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
using common::vectorEx;
using common::PromiseResult;
using common::BaseException;
using common::FileException;
using common::linq::Linq;
using str::Charset;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;

template<class T, class... Args>
using CallbackInvoke = std::function<void(std::function<T(Args...)>)>;

namespace detail
{

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
#include "common/miniLogger/miniLogger.h"
namespace rayr
{
common::mlog::MiniLogger<false>& dizzLog();
string getShaderFromDLL(int32_t id);
}
#endif
