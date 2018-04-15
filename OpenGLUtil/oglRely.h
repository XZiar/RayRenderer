#pragma once

#ifdef OGLU_EXPORT
#   define OGLUAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define OGLUAPI _declspec(dllimport)
#endif

#include "common/CommonRely.hpp"
#include "common/CommonMacro.hpp"
#include "common/AlignedContainer.hpp"
#include "common/CopyEx.hpp"
#include "common/Wrapper.hpp"
#include "common/ContainerEx.hpp"
#include "common/SpinLock.hpp"
#include "common/StringEx.hpp"
#include "common/StrCharset.hpp"
#define USING_CHARDET
#include "common/FileEx.hpp"
#include "common/TimeUtil.hpp"
#include "common/Exceptions.hpp"
#include "common/PromiseTask.hpp"
#include "common/ThreadEx.h"
#include "ImageUtil/ImageUtil.h"
#include "3DElement.hpp"

#define GLEW_STATIC
#include "glew/glew.h"

#include "cpplinq.hpp"

#include <cstddef>
#include <cstdio>
#include <memory>
#include <functional>
#include <type_traits>
#include <string.h>
#include <string>
#include <cstring>
#include <array>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <optional>
#include <variant>
#include <algorithm>
#include <atomic>

namespace oclu
{
namespace detail
{
class _oclGLBuffer;
}
}

namespace oglu
{
namespace str = common::str;
namespace fs = common::file::fs;
using std::string;
using std::string_view;
using std::wstring;
using std::u16string;
using std::byte;
using std::tuple;
using std::pair;
using std::vector;
using std::map;
using std::set;
using std::function;
using std::optional;
using b3d::Vec3;
using b3d::Vec4;
using b3d::Mat3x3;
using b3d::Mat4x4;
using b3d::Camera;
using common::Wrapper;
using common::SimpleTimer;
using common::NonCopyable;
using common::NonMovable;
using common::vectorEx;
using common::PromiseResult;
using common::BaseException;
using common::FileException;


enum class TransformType : uint8_t { RotateXYZ, Rotate, Translate, Scale };
struct OGLUAPI alignas(alignof(Vec4)) TransformOP : public common::AlignBase<alignof(Vec4)>
{
    Vec4 vec;
    TransformType type;
    TransformOP(const Vec4& vec_, const TransformType type_) :vec(vec_), type(type_) { }
};

namespace detail
{
class TextureManager;
class UBOManager;
}
}

#ifdef OGLU_EXPORT
#include "common/miniLogger/miniLogger.h"
namespace oglu
{
common::mlog::MiniLogger<false>& oglLog();
}
#endif