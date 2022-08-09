#pragma once
#include "oclRely.h"

#include "XComputeBase/XCompNailang.h"
#include "XComputeBase/XCompDebug.h"
#include "Nailang/NailangParser.h"
#include "Nailang/NailangRuntime.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/StringFormat.h"
#include "SystemCommon/Format.h"

#include "common/Linq2.hpp"
#include "common/StringLinq.hpp"
#include "common/StringEx.hpp"
#include "common/StrParsePack.hpp"
#include "common/ContainerHelper.hpp"

#include "oclInternal.h"

#define FMTSTR2(syntax, ...) common::str::Formatter<char16_t>{}.FormatStatic(FmtString(syntax), __VA_ARGS__)

namespace oclu
{

common::mlog::MiniLogger<false>& oclLog();
std::pair<uint32_t, uint32_t> ParseVersionString(std::u16string_view str, const size_t verPos = 0);

}