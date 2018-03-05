#pragma once

#ifdef IMGUTIL_EXPORT
#   define IMGUTILAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define IMGUTILAPI _declspec(dllimport)
#endif

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <array>
#include <tuple>
#include <algorithm>
#include <filesystem>
#include "common/CommonRely.hpp"
#include "common/CommonMacro.hpp"
#include "common/Exceptions.hpp"
#include "common/AlignedContainer.hpp"
#include "common/StringEx.hpp"
#include "common/StrCharset.hpp"
#include "common/FileEx.hpp"
#include "common/TimeUtil.hpp"
#include "common/Wrapper.hpp"

namespace xziar::img
{
namespace fs = common::file::fs;
namespace str = common::str;
using std::byte;
using std::string;
using std::wstring;
using std::u16string;
using std::tuple;
}

#ifdef IMGUTIL_EXPORT
#include "common/miniLogger/miniLogger.h"
namespace xziar::img
{
common::mlog::MiniLogger<false>& ImgLog();
}
#endif