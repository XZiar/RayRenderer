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

#define USING_INTEL
//#define USING_NVIDIA

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL\opencl.h>

namespace oclu
{
using std::string;
using std::wstring;
using std::tuple;
using std::map;
using std::vector;
using std::function;
using namespace common;

class oclUtil;
using MessageCallBack = std::function<void(void)>;


}
