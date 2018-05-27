#pragma once

#ifdef RAYCORE_EXPORT
#   define RAYCOREAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define RAYCOREAPI _declspec(dllimport)
#endif

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <tuple>
#include <map>
#include <unordered_map>
#include <set>
#include <deque>
#include <variant>
#include <typeindex>
#include <filesystem>
#include "common/CommonRely.hpp"
#include "common/Wrapper.hpp"
#include "common/AlignedContainer.hpp"
#include "common/ContainerEx.hpp"
#include "common/Exceptions.hpp"
#include "common/FileEx.hpp"
#include "common/ThreadEx.h"
#include "common/StringEx.hpp"
#include "common/StrCharset.hpp"
#include "common/PromiseTask.hpp"
#include "OpenGLUtil/OpenGLUtil.h"
#include "OpenCLUtil/OpenCLUtil.h"
#include "ImageUtil/ImageUtil.h"

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING 1
#pragma warning(disable:4996)
#include <boost/bimap.hpp>
#pragma warning(default:4996)
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace rayr
{
namespace str = common::str;
namespace fs = common::fs;
using std::byte;
using std::wstring;
using std::string;
using std::u16string;
using std::map;
using std::set;
using std::deque;
using std::vector;
using std::variant;
using common::min;
using common::max;
using common::Wrapper;
using common::SimpleTimer;
using common::NonCopyable;
using common::NonMovable;
using common::vectorEx;
using common::PromiseResult;
using common::BaseException;
using common::FileException;
using str::Charset;

template<class T, class... Args>
using CallbackInvoke = std::function<void(std::function<T(Args...)>)>;
}

#ifdef RAYCORE_EXPORT
#include "common/miniLogger/miniLogger.h"
namespace rayr
{
common::mlog::MiniLogger<false>& basLog();
string getShaderFromDLL(int32_t id);
}
#endif
