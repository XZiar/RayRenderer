#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef XCOMPBAS_EXPORT
#   define XCOMPBASAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define XCOMPBASAPI _declspec(dllimport)
# endif
#else
# ifdef XCOMPBAS_EXPORT
#   define XCOMPBASAPI [[gnu::visibility("default")]]
#   define COMMON_EXPORT
# else
#   define XCOMPBASAPI
# endif
#endif

#include "MiniLogger/MiniLogger.h"

#include "common/CommonRely.hpp"
#include "common/EnumEx.hpp"
#include "common/Exceptions.hpp"
#include "common/SIMD.hpp"
#include "gsl/gsl_assert"

#include <cstdio>
#include <type_traits>
#include <assert.h>
#include <string>
#include <string_view>
#include <cstring>
#include <vector>
#include <tuple>
#include <optional>
#include <variant>



namespace xcomp
{

XCOMPBASAPI std::pair<common::simd::VecDataInfo, bool> ParseVDataType(const std::u32string_view type) noexcept;
XCOMPBASAPI std::u32string_view StringifyVDataType(const common::simd::VecDataInfo vtype) noexcept;

}