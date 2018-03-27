#pragma once

#ifdef OGLU_EXPORT
#   define OGLUAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define OGLUAPI _declspec(dllimport)
#endif

#include "3DBasic/3dElement.hpp"
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
#include "ImageUtil/ImageUtil.h"

#define GLEW_STATIC
#include "glew/glew.h"

#include "cpplinq.hpp"

#include <cstddef>
#include <cstdio>
#include <memory>
#include <functional>
#include <type_traits>
#include <assert.h>
#include <string>
#include <cstring>
#include <array>
#include <vector>
#include <map>
#include <tuple>
#include <optional>
#include <algorithm>

#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING 1
#pragma warning(disable:4996)
#include <boost/circular_buffer.hpp>
#pragma warning(default:4996)

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
using std::wstring;
using std::u16string;
using std::byte;
using std::tuple;
using std::pair;
using std::vector;
using std::map;
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