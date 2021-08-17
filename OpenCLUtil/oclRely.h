#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef OCLU_EXPORT
#   define OCLUAPI _declspec(dllexport)
# else
#   define OCLUAPI _declspec(dllimport)
# endif
#else
# define OCLUAPI [[gnu::visibility("default")]]
#endif

#include "SystemCommon/MiniLogger.h"
#include "ImageUtil/ImageCore.h"
#include "ImageUtil/TexFormat.h"
#include "SystemCommon/PromiseTask.h"

#include "common/CommonRely.hpp"
#include "common/ContainerEx.hpp"
#include "common/FrozenDenseSet.hpp"
#include "common/Delegate.hpp"
#include "common/EnumEx.hpp"
#include "common/Exceptions.hpp"

#include <cstdio>
#include <memory>
#include <functional>
#include <type_traits>
#include <assert.h>
#include <string>
#include <string_view>
#include <cstring>
#include <vector>
#include <set>
#include <map>
#include <tuple>
#include <optional>
#include <variant>


#define CL_TARGET_OPENCL_VERSION 300
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_2_APIS
#include "3rdParty/CL/opencl.h"
#include "3rdParty/CL/cl_ext.h"

namespace oclu
{

class oclUtil;
class oclMapPtr_;
class GLInterop;
class NLCLExecutor;
class NLCLRawExecutor;
class NLCLRuntime;
struct KernelContext;
class NLCLContext;
class NLCLProcessor;

class oclCmdQue_;
using oclCmdQue = std::shared_ptr<const oclCmdQue_>;

enum class Vendors { Other = 0, NVIDIA, Intel, AMD, ARM, Qualcomm };


}

