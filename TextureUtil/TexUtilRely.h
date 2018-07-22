#pragma once

#ifdef TEXUTIL_EXPORT
#   define TEXUTILAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define TEXUTILAPI _declspec(dllimport)
#endif


#include <cstdint>
#include <cstdio>
#include <string>
#include <map>
#include <filesystem>
#include "common/Wrapper.hpp"
#include "common/Exceptions.hpp"
#include "common/TimeUtil.hpp"
#include "common/PromiseTask.hpp"
#include "common/ThreadEx.h"
#include "OpenGLUtil/OpenGLUtil.h"
#include "OpenCLUtil/OpenCLUtil.h"
#include "ImageUtil/ImageUtil.h"


namespace oglu::texutil
{

using namespace xziar::img;

}

#ifdef TEXUTIL_EXPORT
#include "common/miniLogger/miniLogger.h"
namespace oglu::texutil
{
common::mlog::MiniLogger<false>& texLog();
string getShaderFromDLL(int32_t id);
}
#endif
