#pragma once

#ifdef RAYCORE_EXPORT
#   define RAYCOREAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define RAYCOREAPI _declspec(dllimport)
#endif

#ifndef _HAS_AUTO_PTR_ETC
#   define _HAS_AUTO_PTR_ETC 1
#elif !(_HAS_AUTO_PTR_ETC)
#   error "RenderCore need to be compiled with _HAS_AUTO_PTR_ETC defined to 1"
#endif

#include <cstdint>
#include <cstdio>
#include <string>
#include <tuple>
#include <map>
#include <unordered_map>
#include <set>
#include <filesystem>
#include "../3DBasic/3dElement.hpp"
#include "../common/CommonRely.hpp"
#include "../common/Wrapper.hpp"
#include "../common/AlignedContainer.hpp"
#include "../common/ContainerEx.hpp"
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
template<class T, class... ARGS>
using CallbackInvoke = std::function<void(std::function<T(ARGS...)>)>;
}

#ifdef RAYCORE_EXPORT
#include "../common/miniLogger/miniLogger.h"
namespace rayr
{
common::mlog::logger& basLog();
string getShaderFromDLL(int32_t id);
}
#endif
