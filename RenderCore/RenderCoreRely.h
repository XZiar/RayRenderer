#pragma once

#ifdef RAYCORE_EXPORT
#   define RAYCOREAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define RAYCOREAPI _declspec(dllimport)
#endif

#define _HAS_AUTO_PTR_ETC 1

#include <cstdint>
#include <cstdio>
#include <string>
#include <tuple>
#include <map>
#include <filesystem>
#include "../3DBasic/3dElement.hpp"
#include "../common/CommonRely.hpp"
#include "../common/Wrapper.hpp"
#include "../common/AlignedContainer.hpp"
#include "../common/Exceptions.hpp"
#include "../common/StringEx.hpp"
#include "../OpenGLUtil/OpenGLUtil.h"
#include "../OpenCLUtil/OpenCLUtil.h"

#include <boost/bimap.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>


namespace rayr
{
using std::wstring;
using std::string;
using std::map;
using std::vector;
using namespace common;
}