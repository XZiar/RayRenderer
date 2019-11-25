#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef OCLIOP_EXPORT
#   define OCLIOPAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define OCLIOPAPI _declspec(dllimport)
# endif
#else
# ifdef OCLIOP_EXPORT
#   define OCLIOPAPI [[gnu::visibility("default")]]
#   define COMMON_EXPORT
# else
#   define OCLIOPAPI
# endif
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
#include "common/Exceptions.hpp"


