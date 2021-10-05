#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef IMGUTIL_EXPORT
#   define IMGUTILAPI _declspec(dllexport)
# else
#   define IMGUTILAPI _declspec(dllimport)
# endif
#else
# define IMGUTILAPI [[gnu::visibility("default")]]
#endif


#include "common/CommonRely.hpp"
#include "common/AlignedBuffer.hpp"
#include "common/EnumEx.hpp"
#include "SystemCommon/Exceptions.h"
#include "common/MemoryStream.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <tuple>

