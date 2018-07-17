#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef IMGUTIL_EXPORT
#   define IMGUTILAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define IMGUTILAPI _declspec(dllimport)
# endif
#else
# define IMGUTILAPI 
#endif

#include "common/CommonRely.hpp"
#include "common/SIMD.hpp"
#include "common/CopyEx.hpp"
#include "common/Exceptions.hpp"
#include "common/AlignedContainer.hpp"
#include "common/StringEx.hpp"
#include "common/StrCharset.hpp"
#include "common/FileEx.hpp"
#include "common/TimeUtil.hpp"
#include "common/Wrapper.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <array>
#include <tuple>
#include <algorithm>

namespace xziar::img
{
namespace fs = common::fs;
namespace str = common::str;
using std::byte;
using std::string;
using std::wstring;
using std::u16string;
using std::tuple;
}

#if COMMON_SIMD_LV >= 20
#   define IMGU_USE_SIMD
#endif

#ifdef IMGUTIL_EXPORT
#include "common/miniLogger/miniLogger.h"
namespace xziar::img
{
common::mlog::MiniLogger<false>& ImgLog();
}
#endif