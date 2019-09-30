#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef IMGUTIL_EXPORT
#   define IMGUTILAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define IMGUTILAPI _declspec(dllimport)
# endif
#else
# ifdef IMGUTIL_EXPORT
#   define IMGUTILAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define IMGUTILAPI
# endif
#endif

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "common/SIMD.hpp"
#include "common/CopyEx.hpp"
#include "common/Exceptions.hpp"
#include "common/AlignedBuffer.hpp"
#include "SystemCommon/FileEx.h"
#include "common/MemoryStream.hpp"
#include "common/TimeUtil.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <array>

namespace xziar::img
{
namespace fs = common::fs;
using std::byte;
using std::string;
using std::wstring;
using std::u16string;
using std::tuple;
}

#if COMMON_SIMD_LV >= 20
#   define IMGU_USE_SIMD
#endif
