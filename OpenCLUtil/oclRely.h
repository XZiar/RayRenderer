#pragma once

#ifdef OCLU_EXPORT
#   define OCLUAPI _declspec(dllexport)
#   define OCLUTPL _declspec(dllexport) 
#   define COMMON_EXPORT
#else
#   define OCLUAPI _declspec(dllimport)
#   define OCLUTPL
#endif

#include "../3dBasic/miniBLAS.hpp"
#include "../common/CommonRely.hpp"
#include "../common/Wrapper.hpp"
#include "../common/Exceptions.hpp"
#include "../common/StringEx.hpp"
#include "../common/AlignedContainer.hpp"
#include "../common/PromiseTask.h"

#include "../OpenGLUtil/oglBuffer.h"
#include "../OpenGLUtil/oglTexture.h"

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
using std::string;
using std::wstring;
using std::string_view;
using std::wstring_view;
using std::tuple;
using std::map;
using std::vector;
using std::function;
using std::optional;
using namespace common;

class oclUtil;
using MessageCallBack = std::function<void(void)>;

namespace detail
{
class _oclContext;
class _oclBuffer;
class _oclKernel;
}
}
