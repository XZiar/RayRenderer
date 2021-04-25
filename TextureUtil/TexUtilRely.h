#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef TEXUTIL_EXPORT
#   define TEXUTILAPI _declspec(dllexport)
# else
#   define TEXUTILAPI _declspec(dllimport)
# endif
#else
# define TEXUTILAPI [[gnu::visibility("default")]]
#endif



#include "OpenGLUtil/OpenGLUtil.h"
#include "OpenCLUtil/oclCmdQue.h"
#include "OpenCLUtil/oclProgram.h"
#include "OpenCLUtil/oclImage.h"
#include "OpenCLUtil/oclBuffer.h"
#include "ImageUtil/ImageCore.h"
#include "SystemCommon/PromiseTask.h"
#include "common/Exceptions.hpp"
#include <cstdint>
#include <cstdio>
#include <string>


namespace oglu::texutil
{

class TexUtilWorker;

}


