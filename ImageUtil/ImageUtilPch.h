#pragma once
#include "ImageUtilRely.h"
#include "DataConvertor.hpp"
#include "RGB15Converter.hpp"
#include "FloatConvertor.hpp"

#include "MiniLogger/MiniLogger.h"
#include "StringCharset/Convert.h"

#include "common/StringEx.hpp"
#include "common/TimeUtil.hpp"
#include "common/Linq.hpp"

#include <cmath>
#include <tuple>
#include <algorithm>
#include <functional>

namespace xziar::img
{
common::mlog::MiniLogger<false>& ImgLog();
}