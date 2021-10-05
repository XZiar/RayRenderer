#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef OCLIOP_EXPORT
#   define OCLIOPAPI _declspec(dllexport)
# else
#   define OCLIOPAPI _declspec(dllimport)
# endif
#else
# define OCLIOPAPI [[gnu::visibility("default")]]
#endif

#include "ImageUtil/ImageCore.h"
#include "OpenCLUtil/oclMem.h"
#include "OpenCLUtil/oclBuffer.h"
#include "OpenCLUtil/oclImage.h"
#include "OpenCLUtil/oclDevice.h"
#include "OpenCLUtil/oclPlatform.h"
#include "OpenCLUtil/oclUtil.h"

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "SystemCommon/Exceptions.h"


