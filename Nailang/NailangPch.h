#pragma once

#include "NailangRely.h"

#include "SystemCommon/StackTrace.h"
#include "SystemCommon/MiscIntrins.h"
#include "SystemCommon/Format.h"
#include "SystemCommon/StringConvert.h"

#include "common/parser/ParserBase.hpp"
#include "common/StringEx.hpp"
#include "common/Linq2.hpp"
#include "common/StaticLookup.hpp"
#include "common/ContainerEx.hpp"
#include "common/StrParsePack.hpp"
#include "common/CharConvs.hpp"
#include "common/AlignedBase.hpp"

#include <boost/container/small_vector.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include <optional>
#include <any>
#include <cmath>
#include <vector>

#define FMTSTR2(syntax, ...) common::str::Formatter<char16_t>{}.FormatStatic(FmtString(syntax), __VA_ARGS__)

namespace xziar::nailang
{


}
