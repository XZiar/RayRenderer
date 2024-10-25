#pragma once
#include "CommonRely.hpp"

// has charconv
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
# define CM_HAS_CHARCONV 1
#elif COMMON_COMPILER_MSVC // in case MSVC
# if _MSC_VER >= 1914 // VS15.7
#   define CM_HAS_CHARCONV 1
# endif
#endif

// decide int/fp support
#if defined(CM_HAS_CHARCONV) && CM_HAS_CHARCONV
# if COMMON_STL_LIBCPP
#   if COMMON_LIBCPP_VER >= 7000
#     define CM_CHARCONV_INT 1
#     define CM_CHARCONV_FP  0
#   else
#     define CM_CHARCONV_INT 0
#     define CM_CHARCONV_FP  0
#   endif
# elif COMMON_STL_LIBSTDCPP
#   if COMMON_LIBSTDCPP_VER >= 11
#     define CM_CHARCONV_INT 1
#     define CM_CHARCONV_FP  1
#   elif COMMON_LIBSTDCPP_VER >= 8
#     define CM_CHARCONV_INT 1
#     define CM_CHARCONV_FP  0
#   else
#     define CM_CHARCONV_INT 0
#     define CM_CHARCONV_FP  0
#   endif
# elif COMMON_COMPILER_MSVC
#   if _MSC_VER >= 1915 // VS15.8
#     define CM_CHARCONV_INT 1
#     define CM_CHARCONV_FP  1
#   elif _MSC_VER >= 1914 // VS15.7
#     define CM_CHARCONV_INT 1
#     define CM_CHARCONV_FP  0
#   else
#     define CM_CHARCONV_INT 0
#     define CM_CHARCONV_FP  0
#   endif
# else
#   pragma message("Use CharConv unknown")
#  define CM_CHARCONV_INT 0
#  define CM_CHARCONV_FP  0
# endif
#endif


// use std
#if CM_CHARCONV_INT || CM_CHARCONV_FP
# include <charconv>
#endif


// try boost
#if !CM_CHARCONV_INT || !CM_CHARCONV_FP
# include <boost/version.hpp>
# if BOOST_VERSION >= 108500 // introduced in 1.85
#   include <boost/charconv.hpp>
#   if !CM_CHARCONV_INT
#     undef CM_CHARCONV_INT
#     define CM_CHARCONV_INT 2
#   endif
#   if !CM_CHARCONV_FP && defined(SYSCOMMON_CHARCONV_BUILD) // boost's str2fp is not header-only
#     undef CM_CHARCONV_FP
#     define CM_CHARCONV_FP 2
#   endif
# endif
#endif


// still not available, use stdlib
#if !CM_CHARCONV_INT || !CM_CHARCONV_FP
# include <cstdlib>
#endif


#ifdef SYSCOMMON_CHARCONV_BUILD
namespace common::detail
#else
namespace common
# define SYSCOMMON_CHARCONV_FUNC forceinline
#endif
{


template<typename T>
SYSCOMMON_CHARCONV_FUNC std::pair<bool, const char*> StrToInt(const std::string_view str, T& val, const int32_t base = 10)
{
    static_assert(std::is_integral_v<T>, "Need intergal type");
#if CM_CHARCONV_INT
# if CM_CHARCONV_INT == 1
    const auto result = std::from_chars(str.data(), str.data() + str.size(), val, base);
# elif CM_CHARCONV_INT == 2
    const auto result = boost::charconv::from_chars(str.data(), str.data() + str.size(), val, base);
# endif
    return { result.ptr == str.data() + str.size(), result.ptr };
#elif defined(SYSCOMMON_CHARCONV_USE)
    return detail::StrToInt(str, val, base);
#else
    auto ptrEnd = str.data() + str.size();
    char** end = const_cast<char**>(&ptrEnd);

    if constexpr (std::is_signed_v<T>)
    {
        const auto result = strtoll(str.data(), end, base);
        if (result > std::numeric_limits<T>::max() || result < std::numeric_limits<T>::min())
            return { false, str.data() };
        val = static_cast<T>(result);
        return { true, *end };
    }
    else
    {
        const auto result = strtoull(str.data(), end, base);
        if (result > std::numeric_limits<T>::max())
            return { false, str.data() };
        val = static_cast<T>(result);
        return { true, *end };
    }
#endif
}


template<typename T>
SYSCOMMON_CHARCONV_FUNC std::pair<bool, const char*> StrToFP(const std::string_view str, T& val, [[maybe_unused]] const bool isScientific = false)
{
    static_assert(std::is_floating_point_v<T>, "Need float point type");
#if CM_CHARCONV_FP
# if CM_CHARCONV_FP == 1
    const auto result = std::from_chars(str.data(), str.data() + str.size(), val,
        isScientific ? std::chars_format::scientific : std::chars_format::general);
# elif CM_CHARCONV_FP == 2
    const auto result = boost::charconv::from_chars(str.data(), str.data() + str.size(), val,
        isScientific ? boost::charconv::chars_format::scientific : boost::charconv::chars_format::general);
# endif
    return { result.ptr == str.data() + str.size(), result.ptr };
#elif defined(SYSCOMMON_CHARCONV_USE)
    return detail::StrToFP(str, val, isScientific);
#else
    auto ptrEnd = str.data() + str.size();
    char** end = const_cast<char**>(&ptrEnd);

    const auto result = strtod(str.data(), end);
    if (std::abs(result) > std::numeric_limits<T>::max())
        return { false, str.data() };
    val = static_cast<T>(result);
    return { true, *end };
#endif
}


}

#undef CM_HAS_CHARCONV