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



#include "OpenGLUtil/oglProgram.h"
#include "OpenGLUtil/oglTexture.h"
#include "OpenGLUtil/oglVAO.h"
#include "OpenCLUtil/oclProgram.h"
#include "OpenCLUtil/oclContext.h"
#include "OpenCLUtil/oclCmdQue.h"
#include "OpenCLUtil/oclBuffer.h"
#include "ImageUtil/ImageCore.h"
#include "SystemCommon/FileEx.h"
#include "common/Exceptions.hpp"
#include "common/TimeUtil.hpp"
#include "common/Controllable.hpp"
#include <cstdint>
#include <cstdio>
#include <string>
#include <map>


namespace ft
{
class FreeTyper;
}

