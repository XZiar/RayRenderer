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
#include "../common/StringEx.hpp"
#include "../common/TimeUtil.h"
#include "../common/Exceptions.hpp"
#include "../common/PromiseTask.h"

#include <GL/glew.h>

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

#include <boost/circular_buffer.hpp>

namespace oclu
{
namespace detail
{
class _oclMem;
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
using b3d::Vec3;
using b3d::Vec4;
using b3d::Mat3x3;
using b3d::Mat4x4;
using b3d::Camera;
using namespace common;
}
