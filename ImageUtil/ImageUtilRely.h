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
#include "common/SIMD.hpp"
#include "common/CopyEx.hpp"
#include "common/Exceptions.hpp"
#include "common/AlignedContainer.hpp"
#include "common/StringEx.hpp"
#include "common/FileEx.hpp"
#include "common/MemoryStream.hpp"
#include "common/TimeUtil.hpp"
#include "common/Wrapper.hpp"
#include "StringCharset/Convert.h"
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
class ImgSupport;
IMGUTILAPI uint32_t RegistImageSupport(const common::Wrapper<ImgSupport>& support);
template<typename T>
uint32_t RegistImageSupport()
{
    return RegistImageSupport(common::Wrapper<T>(std::in_place).template cast_static<ImgSupport>());
}
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