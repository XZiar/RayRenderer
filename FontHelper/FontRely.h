#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef FONTHELPER_EXPORT
#   define FONTHELPAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define FONTHELPAPI _declspec(dllimport)
# endif
#else
# ifdef FONTHELPER_EXPORT
#   define FONTHELPAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define FONTHELPAPI
# endif
#endif


#include <cstdint>
#include <cstdio>
#include <string>
#include <map>
#include "common/Wrapper.hpp"
#include "common/Exceptions.hpp"
#include "common/TimeUtil.hpp"
#include "common/FileEx.hpp"
#include "common/StrBase.hpp"
#include "OpenGLUtil/OpenGLUtil.h"
#include "OpenCLUtil/OpenCLUtil.h"
#include "ImageUtil/ImageUtil.h"


namespace ft
{
class FreeTyper;
}

#ifdef FONTHELPER_EXPORT
#include "common/miniLogger/miniLogger.h"
namespace oglu
{
common::mlog::MiniLogger<false>& fntLog();
string getShaderFromDLL(int32_t id);
}
#endif
