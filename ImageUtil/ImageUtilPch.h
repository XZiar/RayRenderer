#pragma once
#include "ImageUtilRely.h"
#include "ColorConvert.h"

#include "common/simd/SIMD.hpp"

#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/FormatExtra.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/FileMapperEx.h"
#include "SystemCommon/CopyEx.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/Exceptions.h"
#include "SystemCommon/StackTrace.h"

#include "common/FileBase.hpp"
#include "common/Linq2.hpp"
#include "common/StringEx.hpp"
#include "common/TimeUtil.hpp"
#include "common/StaticLookup.hpp"

#include <array>
#include <cmath>
#include <algorithm>
#include <functional>

namespace xziar::img
{
using common::BaseException;
using common::SimpleTimer;
using namespace std::string_view_literals;
common::mlog::MiniLogger<false>& ImgLog();
}