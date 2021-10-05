#pragma once
#include "oclRely.h"

#include "XComputeBase/XCompNailang.h"
#include "XComputeBase/XCompDebug.h"
#include "Nailang/NailangParser.h"
#include "Nailang/NailangRuntime.h"
#include "SystemCommon/StringConvert.h"

#include "common/Linq2.hpp"
#include "common/StringLinq.hpp"
#include "common/StringEx.hpp"
#include "common/StrParsePack.hpp"
#include "common/ContainerHelper.hpp"


namespace oclu
{
common::mlog::MiniLogger<false>& oclLog();
std::pair<uint32_t, uint32_t> ParseVersionString(std::u16string_view str, const size_t verPos = 0);
}