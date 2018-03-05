#pragma once

#ifdef FONTHELPER_EXPORT
#   define FONTHELPAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define FONTHELPAPI _declspec(dllimport)
#endif


#include <cstdint>
#include <cstdio>
#include <string>
#include <map>
#include <filesystem>
#include "3DBasic/3dElement.hpp"
#include "common/Wrapper.hpp"
#include "common/Exceptions.hpp"
#include "common/TimeUtil.hpp"
#include "OpenGLUtil/OpenGLUtil.h"
#include "OpenCLUtil/OpenCLUtil.h"
#include "freetype2/freetype2.h"
#include "ImageUtil/ImageUtil.h"


#ifdef FONTHELPER_EXPORT
#include "common/miniLogger/miniLogger.h"
namespace oglu
{
common::mlog::MiniLogger<false>& fntLog();
string getShaderFromDLL(int32_t id);
}
#endif
