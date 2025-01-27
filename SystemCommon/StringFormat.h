#pragma once

#ifdef FMT_FORMAT_H_
#  error "Don't include format.h before Format.h"
#endif
#define FMT_USE_FULL_CACHE_DRAGONBOX 1
#ifdef SYSCOMMON_EXPORT
#  define FMT_LIB_EXPORT
#else
#  define FMT_SHARED
#endif

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4127)
#endif

#include "3rdParty/fmt/include/fmt/xchar.h"
#include "3rdParty/fmt/include/fmt/args.h"
#include "3rdParty/fmt/include/fmt/ranges.h"
#include "3rdParty/fmt/include/fmt/chrono.h"
#include "3rdParty/fmt/include/fmt/format.h"
#include "3rdParty/fmt/include/fmt/compile.h"

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

#if FMT_VERSION < 110101
#   error("Require fmt 11.1.1")
#endif

namespace common::str::detail
{
struct ForceFill4
{
    char Dummy = '\0';
};
}

FMT_BEGIN_NAMESPACE

// utf fill hacks
template<>
forceinline constexpr auto basic_specs::fill() const -> const char16_t*
{
    if (detail::is_constant_evaluated())
        return nullptr;
    else
        return reinterpret_cast<const char16_t*>(fill_data_);
}
template<>
forceinline FMT_CONSTEXPR void basic_specs::set_fill(basic_string_view<char16_t> s)
{
    auto size = s.size();
    set_fill_size(size);
    FMT_ASSERT(size <= max_fill_size / 2, "invalid fill");
    for (size_t i = 0; i < size; ++i)
    {
        unsigned uchar = static_cast<detail::unsigned_char<char16_t>>(s[i]);
        fill_data_[(i * 2 + 0) & 3] = static_cast<char>(uchar);
        fill_data_[(i * 2 + 1) & 3] = static_cast<char>(uchar >> 8);
    }
}
template<>
forceinline FMT_CONSTEXPR void basic_specs::set_fill(basic_string_view<common::str::detail::ForceFill4> s)
{
    const auto size = s.size();
    CM_ASSUME(size <= 4);
    set_fill_size(size);
    static_assert(4 <= max_fill_size);
    if (detail::is_constant_evaluated())
    {
        for (size_t i = 0; i < 4; ++i)
            fill_data_[i] = s[i].Dummy;
    }
    else
        memcpy(fill_data_, s.data(), 4);
}


FMT_BEGIN_EXPORT


FMT_END_EXPORT
FMT_END_NAMESPACE

