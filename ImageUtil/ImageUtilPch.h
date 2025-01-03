#pragma once
#include "ImageUtilRely.h"
#include "ColorConvert.h"

#include "SystemCommon/MiniLogger.h"
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

#include <array>
#include <cmath>
#include <algorithm>
#include <functional>

namespace xziar::img
{
common::mlog::MiniLogger<false>& ImgLog();
using common::BaseException;
}