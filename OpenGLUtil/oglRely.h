#pragma once

#ifdef OGLU_EXPORT
#   define OGLUAPI _declspec(dllexport)
#   define OGLUTPL _declspec(dllexport) 
#   define COMMON_EXPORT
#else
#   define OGLUAPI _declspec(dllimport)
#   define OGLUTPL
#endif

#include "../3dBasic/3dElement.hpp"
#include "../common/AlignedContainer.hpp"
#include "../common/Wrapper.hpp"
#include "../common/ContainerEx.hpp"
#include "../common/StringEx.hpp"
#include "../common/FileEx.hpp"
#include "../common/TimeUtil.hpp"
#include "../common/Exceptions.hpp"
#include "../common/PromiseTask.h"

#define GLEW_STATIC
#include "../3rdParty/glew/glew.h"

#include "../3rdParty/cpplinq.hpp"

#include <cstdio>
#include <memory>
#include <functional>
#include <type_traits>
#include <assert.h>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <tuple>
#include <optional>
#include <algorithm>

#include <boost/circular_buffer.hpp>

namespace oclu
{
namespace detail
{
class _oclGLBuffer;
}
}

namespace oglu
{
using std::string;
using std::wstring;
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
using namespace common;
}

#ifdef OGLU_EXPORT
#include "../common/miniLogger/miniLogger.h"
namespace oglu
{
common::mlog::logger& oglLog();
}
#endif