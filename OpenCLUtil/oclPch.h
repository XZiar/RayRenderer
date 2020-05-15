#pragma once
#include "oclRely.h"

#include "StringCharset/Convert.h"

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