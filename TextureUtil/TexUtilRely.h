#pragma once

#ifdef TEXCOMP_EXPORT
#   define TEXCOMPAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define TEXCOMPAPI _declspec(dllimport)
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
#include "OpenGLUtil/OpenGLUtil.h"
#include "OpenCLUtil/OpenCLUtil.h"
#include "ImageUtil/ImageUtil.h"


namespace oglu::texcomp
{

using namespace xziar::img;

}

#ifdef TEXCOMP_EXPORT
#include "common/miniLogger/miniLogger.h"
namespace oglu::texcomp
{
common::mlog::MiniLogger<false>& texcLog();
string getShaderFromDLL(int32_t id);
}
#endif
