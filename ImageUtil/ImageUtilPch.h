#pragma once
#include "ImageUtilRely.h"
#include "DataConvertor.hpp"
#include "RGB15Converter.hpp"
#include "FloatConvertor.hpp"

#include "MiniLogger/MiniLogger.h"
#include "StringCharset/Convert.h"

#include "common/CopyEx.hpp"
#include "common/Linq.hpp"
#include "common/StringEx.hpp"
#include "common/TimeUtil.hpp"

#include <array>
#include <cmath>
#include <algorithm>
#include <functional>

namespace xziar::img
{
common::mlog::MiniLogger<false>& ImgLog();
}