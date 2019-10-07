#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef TEXUTIL_EXPORT
#   define TEXUTILAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define TEXUTILAPI _declspec(dllimport)
# endif
#else
# ifdef TEXUTIL_EXPORT
#   define TEXUTILAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define TEXUTILAPI
# endif
#endif



#include "OpenGLUtil/OpenGLUtil.h"
#include "OpenCLUtil/OpenCLUtil.h"
#include "ImageUtil/ImageUtil.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/ThreadEx.h"
#include "common/Exceptions.hpp"
#include "common/TimeUtil.hpp"
#include "common/PromiseTask.hpp"
#include <cstdint>
#include <cstdio>
#include <string>
#include <map>


namespace oglu::texutil
{

using namespace xziar::img;
class TexUtilWorker;

}

#ifdef TEXUTIL_EXPORT
#include "MiniLogger/MiniLogger.h"
namespace oglu::texutil
{
common::mlog::MiniLogger<false>& texLog();
string getShaderFromDLL(int32_t id);
}
#endif
