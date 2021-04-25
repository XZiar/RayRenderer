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
#include "common/Exceptions.hpp"
#include "common/MemoryStream.hpp"
#include "common/SIMD.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <tuple>


#if COMMON_SIMD_LV >= 20
#   define IMGU_USE_SIMD
#endif
