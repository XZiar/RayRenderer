#pragma once
#include "SystemCommonRely.h"


#ifndef SYSCOMMON_CHARCONV_BUILD
namespace common::detail
{
template<typename T>
SYSCOMMONAPI std::pair<bool, const char*> StrToInt(const std::string_view str, T& val, const int32_t base);
template<typename T>
SYSCOMMONAPI std::pair<bool, const char*> StrToFP(const std::string_view str, T& val, [[maybe_unused]] const bool isScientific);
}
#   define SYSCOMMON_CHARCONV_USE 1
#else
#   define SYSCOMMON_CHARCONV_FUNC SYSCOMMONAPI
#endif
#include "common/CharConvs.hpp"

