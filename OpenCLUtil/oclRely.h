#pragma once

#ifdef OCLU_EXPORT
#   define OCLUAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define OCLUAPI _declspec(dllimport)
#endif

#include "3DBasic/miniBLAS.hpp"
#include "common/CommonRely.hpp"
#include "common/CommonMacro.hpp"
#include "common/Wrapper.hpp"
#include "common/Exceptions.hpp"
#include "common/ContainerEx.hpp"
#include "common/StringEx.hpp"
#include "common/StrCharset.hpp"
#include "common/AlignedContainer.hpp"
#include "common/PromiseTask.h"

#include "OpenGLUtil/oglBuffer.h"
#include "OpenGLUtil/oglTexture.h"

#include <cstdio>
#include <memory>
#include <functional>
#include <type_traits>
#include <assert.h>
#include <string>
#include <string_view>
#include <cstring>
#include <vector>
#include <map>
#include <tuple>
#include <optional>

#define USING_INTEL
//#define USING_NVIDIA

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL\opencl.h>

namespace oclu
{
namespace str = common::str;
namespace fs = common::file::fs;
using std::string;
using std::wstring;
using std::string_view;
using std::wstring_view;
using std::tuple;
using std::map;
using std::vector;
using std::function;
using std::optional;
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

class oclUtil;
using MessageCallBack = std::function<void(const wstring&)>;

enum class Vendor { Other = 0, NVIDIA, Intel, AMD };

namespace detail
{
class _oclContext;
class _oclBuffer;
class _oclKernel;
}
}

#ifdef OCLU_EXPORT
#include "common/miniLogger/miniLogger.h"
namespace oclu
{
common::mlog::logger& oclLog();
}
#endif
