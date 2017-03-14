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
#include "../3DBasic/3dElement.hpp"
#include "../OpenGLUtil/oglUtil.h"

