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
#include "../common/CommonBase.hpp"

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
using std::function;
using miniBLAS::AlignBase;
using miniBLAS::vector;
using namespace common;

class oclUtil;
using MessageCallBack = std::function<void(void)>;

namespace inner
{
class _oclPlatform;
class _oclProgram;
class _oclDevice;
class _oclComQue;
class _oclContext;
}


}
