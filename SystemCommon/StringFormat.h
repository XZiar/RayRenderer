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

#if FMT_VERSION < 110000
#   error("Require fmt 11.x")
#endif

FMT_BEGIN_NAMESPACE

//namespace detail 
//{
//template <typename T, FMT_ENABLE_IF(is_compile_string<T>::value)>
//constexpr auto to_string_view(const T& s)
//-> basic_string_view<typename T::char_type> {
//    return (basic_string_view<typename T::char_type>)s;
//}
//}

template <typename S, typename... T, FMT_ENABLE_IF(detail::is_compile_string<S>::value)>
auto format_utf(const S& format_str, T&&... args) -> std::basic_string<typename S::char_type> 
{
    using Char = typename S::char_type;
    return vformat(static_cast<basic_string_view<typename S::char_type>>(format_str),
        fmt::make_format_args<buffered_context<Char>>(args...));
}


FMT_BEGIN_EXPORT


FMT_END_EXPORT
FMT_END_NAMESPACE

#define FMTSTR(syntax, ...) fmt::format_utf(FMT_STRING(syntax), __VA_ARGS__)
