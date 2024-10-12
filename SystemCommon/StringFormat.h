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

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

#if FMT_VERSION < 100000 || FMT_VERSION > 110000
#   error("Require fmt 10.x")
#endif

FMT_BEGIN_NAMESPACE

using u16memory_buffer = basic_memory_buffer<char16_t>;
using u32memory_buffer = basic_memory_buffer<char32_t>;
using u16format_context = buffer_context<char16_t>;
using u32format_context = buffer_context<char32_t>;
using u16format_args = basic_format_args<u16format_context>;
using u32format_args = basic_format_args<u32format_context>;

FMT_END_NAMESPACE

#define FMTSTR(syntax, ...) fmt::format(FMT_STRING(syntax), __VA_ARGS__)
