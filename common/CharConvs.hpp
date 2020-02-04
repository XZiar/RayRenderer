#pragma once

#include "CommonRely.hpp"

#if COMPILER_MSVC

# if _MSC_VER >= 1914 // VS15.7
#   define CHARCONV_INT 1
#   if _MSC_VER >= 1915 // VS15.8
#     define CHARCONV_FP 1
#   else
#     define CHARCONV_FP 0
#   endif
# else 
#   define CHARCONV_INT 0
#   define CHARCONV_FP  0
# endif

#elif COMPILER_GCC

# if __GNUC__ >= 8
#   define CHARCONV_INT 1
#   define CHARCONV_FP  0
# else
#   define CHARCONV_INT 0
#   define CHARCONV_FP  0
# endif

#elif COMPILER_CLANG

//# if __clang_major__ >= 7
//#   define CHARCONV_INT 1
//#   define CHARCONV_FP  0
//# else
//#   define CHARCONV_INT 0
//#   define CHARCONV_FP  0
//# endif

//libstdc++ may not support it
# define CHARCONV_INT 0
# define CHARCONV_FP  0

#else

# define CHARCONV_INT 0
# define CHARCONV_FP  0

#endif

#if CHARCONV_INT || CHARCONV_FP
#   include<charconv>
#endif

#if !CHARCONV_INT || !CHARCONV_FP
#   include<cstdlib>
#endif


namespace common
{


#if CHARCONV_INT

template<typename T>
inline std::pair<bool, const char*> StrToInt(const std::string_view str, T& val, const int32_t base = 10)
{
    static_assert(std::is_integral_v<T>, "Need intergal type");
    const auto result = std::from_chars(str.data(), str.data() + str.size(), val, base);
    return { result.ptr == str.data() + str.size(), result.ptr };
}

#else

template<typename T>
inline std::pair<bool, const char*> StrToInt(const std::string_view str, T& val, const int32_t base = 10)
{
    static_assert(std::is_integral_v<T>, "Need intergal type");

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
}

#endif


#if CHARCONV_FP

template<typename T>
inline std::pair<bool, const char*> StrToFP(const std::string_view str, T& val, const bool isScientific = false)
{
    static_assert(std::is_floating_point_v<T>, "Need float point type");
    const auto result = std::from_chars(str.data(), str.data() + str.size(), val,
        isScientific ? std::chars_format::scientific : std::chars_format::general);
    return { result.ptr == str.data() + str.size(), result.ptr };
}

#else

template<typename T>
inline std::pair<bool, const char*> StrToFP(const std::string_view str, T& val, [[maybe_unused]] const bool isScientific = false)
{
    static_assert(std::is_floating_point_v<T>, "Need float point type");

    auto ptrEnd = str.data() + str.size();
    char** end = const_cast<char**>(&ptrEnd);

    const auto result = strtod(str.data(), end);
    if (std::abs(result) > std::numeric_limits<T>::max())
        return { false, str.data() };
    val = static_cast<T>(result);
    return { true, *end };
}

#endif


}

#undef CHARCONV_INT
#undef CHARCONV_FP
