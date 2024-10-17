#pragma once

/* Compier Test */

#if defined(__INTEL_COMPILER)
#   define COMMON_COMPILER_ICC 1
#   define COMMON_ICC_VER (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__clang__)
#   define COMMON_COMPILER_CLANG 1
#   define COMMON_CLANG_VER (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
#   define COMMON_COMPILER_GCC 1
#   define COMMON_GCC_VER (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(_MSC_VER)
#   define COMMON_COMPILER_MSVC 1
#   define COMMON_MSVC_VER (_MSC_VER * 100)
#elif defined(__MINGW32__) || defined(__MINGW64__)
#   define COMMON_COMPILER_MINGW 1
#   define COMMON_MINGW_VER (__MINGW64_VERSION_MAJOR * 10000 + __MINGW64_VERSION_MINOR * 100)
#endif

#ifndef COMMON_COMPILER_ICC
#   define COMMON_COMPILER_ICC 0
#   define COMMON_ICC_VER 0
#endif
#ifndef COMMON_COMPILER_CLANG
#   define COMMON_COMPILER_CLANG 0
#   define COMMON_CLANG_VER 0
#endif
#ifndef COMMON_COMPILER_GCC
#   define COMMON_COMPILER_GCC 0
#   define COMMON_GCC_VER 0
#endif
#ifndef COMMON_COMPILER_MSVC
#   define COMMON_COMPILER_MSVC 0
#   define COMMON_MSVC_VER 0
#endif
#ifndef COMMON_COMPILER_MINGW
#   define COMMON_COMPILER_MINGW 0
#   define COMMON_MINGW_VER 0
#endif



/* STL Test */

#if defined(_LIBCPP_VERSION)
#   define COMMON_STL_LIBCPP 1
#   define COMMON_LIBCPP_VER _LIBCPP_VERSION
#elif defined(_GLIBCXX_RELEASE)
#   define COMMON_STL_LIBSTDCPP 1
#   define COMMON_LIBSTDCPP_VER _GLIBCXX_RELEASE
#endif

#ifndef COMMON_STL_LIBCPP
#   define COMMON_STL_LIBCPP 0
#endif
#ifndef COMMON_STL_LIBSTDCPP
#   define COMMON_STL_LIBSTDCPP 0
#endif



/* OS Test */

#if defined(_WIN32)
#   define COMMON_OS_WIN 1
#elif defined(__APPLE__)
#   include <TargetConditionals.h>
#   if (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) || (defined(TARGET_OS_IOS) && TARGET_OS_IOS)
#       define COMMON_OS_IOS 1
#   elif TARGET_OS_OSX
#       define COMMON_OS_MACOS 1
#   endif
#   define COMMON_OS_DARWIN 1
#elif defined(__ANDROID__)
#   define COMMON_OS_ANDROID 1
#elif defined(__linux__)
#   define COMMON_OS_LINUX 1
#elif defined(__FreeBSD__)
#   define COMMON_OS_FREEBSD 1
#endif

#ifndef COMMON_OS_WIN
#   define COMMON_OS_WIN 0
#endif
#ifndef COMMON_OS_IOS
#   define COMMON_OS_IOS 0
#endif
#ifndef COMMON_OS_MACOS
#   define COMMON_OS_MACOS 0
#endif
#ifndef COMMON_OS_DARWIN
#   define COMMON_OS_DARWIN 0
#endif
#ifndef COMMON_OS_ANDROID
#   define COMMON_OS_ANDROID 0
#endif
#ifndef COMMON_OS_LINUX
#   if COMMON_OS_ANDROID
#       define COMMON_OS_LINUX 1
#   else
#       define COMMON_OS_LINUX 0
#   endif
#endif
#ifndef COMMON_OS_FREEBSD
#   if COMMON_OS_DARWIN
#       define COMMON_OS_FREEBSD 1
#   else
#       define COMMON_OS_FREEBSD 0
#   endif
#endif
#ifndef COMMON_OS_UNIX
#   if COMMON_OS_LINUX || COMMON_OS_FREEBSD || defined(__unix__)
#       define COMMON_OS_UNIX 1
#   else
#       define COMMON_OS_UNIX 0
#   endif
#endif



/* Arch Test */

#if COMMON_COMPILER_MSVC
#   if defined(_M_ARM) || defined(_M_ARM64)
#       define COMMON_ARCH_ARM 1
#   elif defined(_M_IX86) || defined(_M_X64)
#       define COMMON_ARCH_X86 1
#   endif
#else
#   if defined(__arm__) || defined(__aarch64__)
#       define COMMON_ARCH_ARM 1
#   elif defined(__i386__) || defined(__amd64__) || defined(__x86_64__)
#       define COMMON_ARCH_X86 1
#   endif
#endif
#ifndef COMMON_ARCH_ARM
#   define COMMON_ARCH_ARM 0
#endif
#ifndef COMMON_ARCH_X86
#   define COMMON_ARCH_X86 0
#endif



/* C++ language Test */

#if COMMON_COMPILER_MSVC
#   if defined(_MSVC_LANG)
#       define COMMON_CPP_TIME _MSVC_LANG
#   else
#       define COMMON_CPP_TIME __cplusplus
#   endif
#else
#   define COMMON_CPP_TIME __cplusplus
#endif

#if COMMON_CPP_TIME >= 202002L
#   include <version>
#else
#   include <ciso646>
#endif

#if COMMON_CPP_TIME >= 202002L 
#   define COMMON_CPP_20 1
#endif
#if COMMON_CPP_TIME >= 201703L 
#   define COMMON_CPP_17 1
#endif
#if COMMON_CPP_TIME >= 201402L 
#   define COMMON_CPP_14 1
#endif
#if COMMON_CPP_TIME >= 201103L 
#   define COMMON_CPP_11 1
#endif
#if COMMON_CPP_TIME >= 199711L
#   define COMMON_CPP_03 1
#endif

#ifndef COMMON_CPP_20
#   define COMMON_CPP_20 0
#endif
#ifndef COMMON_CPP_17
#   define COMMON_CPP_17 0
#endif
#ifndef COMMON_CPP_14
#   define COMMON_CPP_14 0
#endif
#ifndef COMMON_CPP_11
#   define COMMON_CPP_11 0
#endif
#ifndef COMMON_CPP_03
#   define COMMON_CPP_03 0
#endif



/* debug detection */

#if COMMON_COMPILER_MSVC
#   if defined(_DEBUG)
#       define CM_DEBUG 1
#   else
#       define CM_DEBUG 0
#   endif
#else
#   if defined(NDEBUG)
#       define CM_DEBUG 0
#   else
#       define CM_DEBUG 1
#   endif
#endif



/* builtin check */

#if defined(__has_builtin)
#   define CM_HAS_BUILTIN(x) __has_builtin(x)
#else
#   define CM_HAS_BUILTIN(x) 0
#endif



/* over-aligned support */

#if COMMON_CPP_17 && defined(__cpp_aligned_new) && __cpp_aligned_new >= 201606L
#   define COMMON_OVER_ALIGNED 1
#else
#   define COMMON_OVER_ALIGNED 0
#endif



/* empty base fix */

#if COMMON_COMPILER_MSVC && _MSC_VER >= 1900 && _MSC_FULL_VER >= 190023918 && _MSC_VER < 2000
#   define COMMON_EMPTY_BASES __declspec(empty_bases)
#else
#   define COMMON_EMPTY_BASES 
#endif


/* vectorcall fix */

#if (COMMON_COMPILER_MSVC /*|| COMMON_COMPILER_CLANG*/) && !defined(_MANAGED) && !defined(_M_CEE)
#   define VECCALL __vectorcall
#else
#   define VECCALL 
#endif



/* char8_t fix */

#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
using u8ch_t = char8_t;
#else
using u8ch_t = char;
#endif



/* endian detection */

#if defined(__cpp_lib_endian) && __cpp_lib_endian >= 201907L
#   include <bit>
namespace common::detail
{
inline constexpr bool is_big_endian = std::endian::native == std::endian::big;
inline constexpr bool is_little_endian = std::endian::native == std::endian::little;
}
#elif COMMON_COMPILER_GCC || COMMON_COMPILER_CLANG
namespace common::detail
{
#   if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
inline constexpr bool is_little_endian = true;
#   else
inline constexpr bool is_little_endian = false;
#   endif
#   if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
inline constexpr bool is_big_endian = true;
#   else
inline constexpr bool is_big_endian = false;
#   endif
}
#else
#   include <boost/predef/other/endian.h>
namespace common::detail
{
#   if BOOST_ENDIAN_LITTLE_BYTE
inline constexpr bool is_little_endian = true;
#   else
inline constexpr bool is_little_endian = false;
#   endif
#   if BOOST_ENDIAN_BIG_BYTE
inline constexpr bool is_big_endian = true;
#   else
inline constexpr bool is_big_endian = false;
#   endif
}
#endif
namespace common::detail
{
static_assert(is_big_endian || is_little_endian);
static_assert(is_big_endian != is_little_endian);
}



/* c++ version compatible defines */




/* likely-unlikely */

#if defined(__has_cpp_attribute) && __has_cpp_attribute(likely)
#   define IF_LIKELY(x)         if (x) [[likely]]
#   define IF_UNLIKELY(x)       if (x) [[unlikely]]
#   define ELSE_LIKELY          else [[likely]]
#   define ELSE_UNLIKELY        else [[unlikely]]
#   define SWITCH_LIKELY(x, y)  switch (x)
#   define CASE_LIKELY(x)       [[likely]] case x
#elif COMMON_COMPILER_GCC || COMMON_COMPILER_CLANG
#   define IF_LIKELY(x)         if (__builtin_expect(!!(x), 1))
#   define IF_UNLIKELY(x)       if (__builtin_expect(!!(x), 0))
#   define ELSE_LIKELY          else
#   define ELSE_UNLIKELY        else
#   define SWITCH_LIKELY(x, y)  switch (static_cast<decltype(x)>(__builtin_expect(static_cast<long>(x), static_cast<long>(y))))
#   define CASE_LIKELY(x)       case x
#else
#   define IF_LIKELY(x)         if (x)
#   define IF_UNLIKELY(x)       if (x)
#   define ELSE_LIKELY          else
#   define ELSE_UNLIKELY        else
#   define SWITCH_LIKELY(x, y)  switch (x)
#   define CASE_LIKELY(x)       case x
#endif
#if COMMON_COMPILER_MSVC || COMMON_COMPILER_ICC
#   define CM_ASSUME(x) __assume(!!(x))
#elif CM_HAS_BUILTIN(__builtin_assume)
#   define CM_ASSUME(x) __builtin_assume(!!(x))
#elif COMMON_COMPILER_GCC || CM_HAS_BUILTIN(__builtin_unreachable)
#   define CM_ASSUME(x) ((x) ? void(0) : __builtin_unreachable())
#else
#   define CM_ASSUME(x) void(0)
#endif
#if CM_HAS_BUILTIN(__builtin_unpredictable)
#   define CM_UNPREDICT(x)      static_cast<decltype(x)>(__builtin_unpredictable(static_cast<long>(x)))
#   define CM_UNPREDICT_BOOL(x) static_cast<bool>       (__builtin_unpredictable(!!(x)))
#elif CM_HAS_BUILTIN(__builtin_expect_with_probability)
#   define CM_UNPREDICT(x)      x
#   define CM_UNPREDICT_BOOL(x) static_cast<bool>       (__builtin_expect_with_probability(!!(x), 1, 0.5))
#else
#   define CM_UNPREDICT(x)      x
#   define CM_UNPREDICT_BOOL(x) static_cast<bool>(x)
#endif
#if COMMON_COMPILER_GCC || CM_HAS_BUILTIN(__builtin_unreachable)
#   define CM_UNREACHABLE() __builtin_unreachable()
#else
#   define CM_UNREACHABLE() CM_ASSUME(0)
#endif



/* inline-related */

#if defined(_MSC_VER)
#   define forceinline      __forceinline
#   define forcenoinline    __declspec(noinline)
#elif defined(__GNUC__)
#   define forceinline      __inline__ __attribute__((always_inline))
#   define forcenoinline    __attribute__((noinline))
#   define _FILE_OFFSET_BITS 64
#else
#   define forceinline inline
#   define forcenoinline 
#endif



/* basic include */

#define __STDC_WANT_SECURE_LIB__ 1
#define __STDC_WANT_LIB_EXT1__ 1
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits> 
#include "3rdParty/gsl/assert"
#include "3rdParty/gsl/util"

#if UINTPTR_MAX == UINT64_MAX
#   define COMMON_OSBIT 64
#else
#   define COMMON_OSBIT 32
#endif



/* const-eval-init */

#if defined(__cpp_constinit) && __cpp_constinit >= 201907L
#   define COMMON_CONSTINIT constinit
#else
#   define COMMON_CONSTINIT
#endif

#if defined(__cpp_consteval) && __cpp_consteval >= 201811L
#   define COMMON_CONSTEVAL consteval
#else
#   define COMMON_CONSTEVAL constexpr
#endif

namespace common
{
[[nodiscard]] inline constexpr bool is_constant_evaluated([[maybe_unused]] bool defVal = false) noexcept
{
#if COMMON_COMPILER_CLANG && __clang_major__ == 14 && \
    COMMON_STL_LIBSTDCPP && COMMON_LIBSTDCPP_VER >= 12 && COMMON_CPP_TIME >= 202002L
    // Workaround for incompatibility between libstdc++ and clang-14, see https://github.com/fmtlib/fmt/issues/3247.
    return __builtin_is_constant_evaluated();
#elif defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811L
    return std::is_constant_evaluated();
#else
    return defVal;
#endif
}
}



/* *_s fix */

#if defined(__GNUC__)
#   if !defined(__STDC_LIB_EXT1__)
#       include <errno.h>
forceinline std::remove_reference<decltype(errno)>::type memcpy_s(void* dest, size_t destsz, const void* src, size_t count)
{
    if (count == 0)
        return 0;
    if (src == nullptr || dest == nullptr || destsz < count)
    {
        memset(dest, 0, destsz);
        return EINVAL;
    }
    memcpy(dest, src, count);
    return 0;
}
forceinline std::remove_reference<decltype(errno)>::type memmove_s(void* dest, size_t destsz, const void* src, size_t count)
{
    if (count == 0)
        return 0;
    if (src == nullptr || dest == nullptr)
        return EINVAL;
    if (destsz < count)
        return ERANGE;
    memmove(dest, src, count);
    return 0;
}
#   endif
#endif



/*
* Concatenate preprocessor tokens A and B without expanding macro definitions
* (however, if invoked from a macro, macro arguments are expanded).
*/
#define PPCAT_NX(A, B) A ## B

/*
* Concatenate preprocessor tokens A and B after macro-expanding them.
*/
#define PPCAT(A, B) PPCAT_NX(A, B)

/*
* Turn A into a string literal without expanding macro definitions
* (however, if invoked from a macro, macro arguments are expanded).
*/
#define STRINGIZE_NX(A) #A

/*
* Turn A into a string literal after macro-expanding it.
*/
#define STRINGIZE(A) STRINGIZE_NX(A)

#define WIDEN_NX(X) L ## X
#define WIDEN(X) WIDEN_NX(X)

#define UTF16ER_NX(X) u ## X
#define UTF16ER(X) UTF16ER_NX(X)



/* hacker for std::make_shared/std::make_unique */

#include <memory>
#define MAKE_ENABLER() struct make_enabler

#define MAKE_ENABLER_IMPL(clz)                  \
struct clz::make_enabler : public clz           \
{                                               \
    template<typename... Args>                  \
    make_enabler(Args... args) :                \
        clz(std::forward<Args>(args)...) { }    \
};                                              \


#define MAKE_ENABLER_SHARED(clz, arg)       std::static_pointer_cast<clz>(std::make_shared<std::decay_t<clz>::make_enabler>arg)
#define MAKE_ENABLER_UNIQUE(clz, arg)       std::unique_ptr<clz>(std::make_unique<std::decay_t<clz>::make_enabler>arg)



/* copy/move assignment */

#define COMMON_NO_COPY(clz)                         \
    clz(const clz&) noexcept = delete;              \
    clz& operator= (const clz&) noexcept = delete;  \

#define COMMON_DEF_COPY(clz)                        \
    clz(const clz&) noexcept = default;             \
    clz& operator= (const clz&) noexcept = default; \

#define COMMON_NO_MOVE(clz)                         \
    clz(clz&&) noexcept = delete;                   \
    clz& operator= (clz&&) noexcept = delete;       \

#define COMMON_DEF_MOVE(clz)                        \
    clz(clz&&) noexcept = default;                  \
    clz& operator= (clz&&) noexcept = default;      \



namespace common
{


template<typename T>
using remove_cvref_t = typename std::remove_cv_t<std::remove_reference_t<T>>;


template<typename T>
inline constexpr bool AlwaysTrue = true;
//constexpr inline bool AlwaysTrue() noexcept { return true; }
template<auto T>
inline constexpr bool AlwaysTrue2 = true;
//constexpr inline bool AlwaysTrue2() noexcept { return true; }


template<typename T, typename... Args>
[[nodiscard]] forceinline constexpr bool MatchAny(const T& obj, Args... args)
{
    return (... || (obj == args));
}


template<typename T>
[[nodiscard]] forceinline constexpr bool IsPower2(const T num)
{
    static_assert(std::is_unsigned_v<T>, "only for unsigned type");
    return (num & (num - 1)) == 0; 
}



template <typename, template <typename...> typename Op, typename... T>
struct is_detected_impl : std::false_type {};
template <template <typename...> typename Op, typename... T>
struct is_detected_impl<std::void_t<Op<T...>>, Op, T...> : std::true_type {};

template <template <typename...> typename Op, typename... T>
using is_detected = is_detected_impl<void, Op, T...>;
template <template <typename...> typename Op, typename... T>
inline constexpr bool is_detected_v = is_detected_impl<void, Op, T...>::value;


template<template<typename...> class Base, typename...Ts>
std::true_type is_base_of_template_impl(const Base<Ts...>*);
template<template<typename...> class Base>
std::false_type is_base_of_template_impl(...);
template<template<typename...> class Base, typename Derived>
using is_base_of_template = decltype(is_base_of_template_impl<Base>(std::declval<Derived*>()));

template <class T, template <typename...> class Template>
struct is_specialization : std::false_type {};
template <template <typename...> class Template, typename... Ts>
struct is_specialization<Template<Ts...>, Template> : std::true_type {};
#if defined(__cpp_nontype_template_parameter_auto) || (defined(__cplusplus) && (__cplusplus >= 201703L))
template <class T, template <auto...> class Template>
struct is_specialization2 : std::false_type {};
template <template <auto...> class Template, auto... Vs>
struct is_specialization2<Template<Vs...>, Template> : std::true_type {};
#endif


namespace detail
{
template<typename T>
using HasValueTypeCheck = typename T::value_type;
template<typename A, typename B>
using EqualCompareCheck = decltype(std::declval<const A&>() == std::declval<const B&>());
template<typename A, typename B>
using NotEqualCompareCheck = decltype(std::declval<const A&>() != std::declval<const B&>());
}
template<typename T>
inline constexpr bool has_valuetype_v = is_detected_v<detail::HasValueTypeCheck, T>;
template<typename A, typename B>
inline constexpr bool is_equal_comparable_v = common::is_detected_v<detail::EqualCompareCheck, A, B>;
template<typename A, typename B>
inline constexpr bool is_notequal_comparable_v = common::is_detected_v<detail::NotEqualCompareCheck, A, B>;


template<typename T, template <typename...> class Base>
struct TemplateDerivedHelper
{
private:
    template<typename... Ts>
    static std::true_type is_derived_from_base_impl(const Base<Ts...>*);
    static std::false_type is_derived_from_base_impl(const void*);
public:
    static constexpr bool IsDerivedFromBase = decltype(is_derived_from_base_impl(std::declval<T*>()))::value;
};



template<typename T>
[[nodiscard]] forceinline constexpr const T& max(const T& left, const T& right)
{
    static_assert(std::is_arithmetic_v<T>, "only support arithmetic type");
    return left < right ? right : left;
}
template<typename T>
[[nodiscard]] forceinline constexpr const T& min(const T& left, const T& right)
{
    static_assert(std::is_arithmetic_v<T>, "only support arithmetic type");
    return left < right ? left : right;
}
template<typename T, typename U>
[[nodiscard]] forceinline constexpr T saturate_cast(const U val)
{
    static_assert(std::is_arithmetic_v<T> && std::is_arithmetic_v<U>, "only support arithmetic type");
    constexpr bool MaxFit = static_cast<std::make_unsigned_t<U>>(std::numeric_limits<U>::max())
        <= static_cast<std::make_unsigned_t<T>>(std::numeric_limits<T>::max());
    constexpr bool MinFit = static_cast<std::make_signed_t<U>>(std::numeric_limits<U>::min())
        >= static_cast<std::make_signed_t<T>>(std::numeric_limits<T>::min());
    if constexpr (MaxFit)
    {
        if constexpr (MinFit)
            return val;
        else
            return static_cast<T>(max(static_cast<U>(std::numeric_limits<T>::min()), val));
    }
    else
    {
        if constexpr (MinFit)
            return static_cast<T>(min(static_cast<U>(std::numeric_limits<T>::max()), val));
        else
            return static_cast<T>(min(
                static_cast<U>(std::numeric_limits<T>::max()),
                max(static_cast<U>(std::numeric_limits<T>::min()), val)
            ));
    }
}


struct NonCopyable
{
    constexpr NonCopyable() noexcept = default;
    constexpr ~NonCopyable() noexcept = default;
    constexpr NonCopyable(const NonCopyable&) noexcept = delete;
    constexpr NonCopyable& operator= (const NonCopyable&) noexcept = delete;
    constexpr NonCopyable(NonCopyable&&) noexcept = default;
    constexpr NonCopyable& operator= (NonCopyable&&) noexcept = default;
};

struct NonMovable
{
    constexpr NonMovable() noexcept = default;
    constexpr ~NonMovable() noexcept = default;
    constexpr NonMovable(NonMovable&&) noexcept = delete;
    constexpr NonMovable& operator= (NonMovable&&) noexcept = delete;
    constexpr NonMovable(const NonMovable&) noexcept = default;
    constexpr NonMovable& operator= (const NonMovable&) noexcept = default;
};


forceinline void ZeroRegion(void* ptr, size_t size)
{
#if COMMON_COMPILER_GCC && __GNUC__ >= 8
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
    memset(ptr, 0x0, size);
#if COMMON_COMPILER_GCC && __GNUC__ >= 8
#pragma GCC diagnostic pop
#endif
}

}



/* compile-time str hash support */

namespace common
{
struct DJBHash // DJB Hash
{
    template<typename T>
    static constexpr uint64_t HashC(const T& str) noexcept
    {
        static_assert(std::is_class_v<T> || std::is_union_v<T>, "only accept class");
        uint64_t hash = 5381;
        for (const auto ch : str)
            hash = hash * 33 + ch;
        return hash;
    }
    template<typename Ch>
    static constexpr uint64_t HashP(const Ch* str) noexcept
    {
        uint64_t hash = 5381;
        for (; *str != static_cast<Ch>('\0'); ++str)
            hash = hash * 33 + *str;
        return hash;
    }
    template<typename T>
    static constexpr uint64_t Hash(const T* str) noexcept
    {
        return HashP(str);
    }
    template<typename T>
    static constexpr uint64_t Hash(const T& str) noexcept
    {
        if constexpr (std::is_array_v<remove_cvref_t<T>>)
            return HashP(str);
        else
            return HashC(str);
    }
    template<typename Ch>
    constexpr uint64_t operator()(std::basic_string_view<Ch> str) const noexcept
    {
        return HashC(str);
    }
    template<typename Ch>
    constexpr uint64_t operator()(const std::basic_string<Ch>& str) const noexcept
    {
        return HashC(str);
    }
};
}

/**
** @brief calculate simple hash for string, used for switch-string
** @return uint64_t the hash
**/
[[nodiscard]] inline constexpr uint64_t operator "" _hash(const char* str, size_t) noexcept
{
    return common::DJBHash::HashP(str);
}
[[nodiscard]] inline constexpr uint64_t operator "" _hash(const char16_t* str, size_t) noexcept
{
    return common::DJBHash::HashP(str);
}
[[nodiscard]] inline constexpr uint64_t operator "" _hash(const char32_t* str, size_t) noexcept
{
    return common::DJBHash::HashP(str);
}

#define HashCase(target, cstr) case ::common::DJBHash::Hash(cstr): if (target != cstr) break;



/* span compatible include */

#if (defined(__cpp_lib_span) && (__cpp_lib_span >= 201902L))
#   include <span>
namespace common
{
template <class ElementType, size_t Extent = std::dynamic_extent>
using span = std::span<ElementType, Extent>;
using std::as_bytes;
using std::as_writable_bytes;
namespace detail
{
template<typename T>
struct is_span : std::false_type { };
template <typename ElementType, size_t Extent>
struct is_span<std::span<ElementType, Extent>> : std::true_type { };
}
template<typename T>
using is_span = detail::is_span<common::remove_cvref_t<T>>;
template<typename T>
inline constexpr bool is_span_v = detail::is_span<common::remove_cvref_t<T>>::value;
}
#else
#   include "3rdParty/gsl/span"
namespace common
{
template <class ElementType, size_t Extent = gsl::dynamic_extent>
using span = gsl::span<ElementType, Extent>;
using gsl::as_bytes;
using gsl::as_writable_bytes;
template<typename T>
using is_span = gsl::details::is_span<T>;
template<typename T>
inline constexpr bool is_span_v = gsl::details::is_span<T>::value;
}
#endif
namespace common
{
namespace detail
{
template<typename T>
using CanAsSpan = decltype(std::declval<T&>().AsSpan());
template<typename T>
using HasData = decltype(std::declval<T&>().data());
template<typename T>
using HasSize = decltype(std::declval<T&>().size());
template<typename T>
inline constexpr bool IsContainerLike = is_detected_v<HasData, T> && is_detected_v<HasSize, T>;
struct CannotToSpan
{
    template<typename T>
    constexpr auto operator()(T&& arg) noexcept
    {
        static_assert(!common::AlwaysTrue<T>, "unsupported");
    }
};
struct SkipToSpan
{
    template<typename T>
    constexpr auto operator()(T&&) noexcept
    { }
};
}
template <typename T, size_t N>
[[nodiscard]] forceinline constexpr auto to_span(T(&arr)[N]) noexcept
{
    return common::span<T>(arr, N);
}
template <typename T, typename Fallback = detail::CannotToSpan>
[[nodiscard]] forceinline constexpr auto to_span(T&& arg) noexcept
{
    using U = common::remove_cvref_t<T>;
    if constexpr (common::is_span_v<U>)
        return arg;
    else if constexpr (common::has_valuetype_v<U>)
    {
        constexpr auto IsConst = std::is_const_v<std::remove_reference_t<T>>;
        using EleType = std::conditional_t<IsConst, std::add_const_t<typename U::value_type>, typename U::value_type>;
        if constexpr (std::is_constructible_v<common::span<EleType>, T>)
            return common::span<EleType>(std::forward<T>(arg));
        else if constexpr (std::is_convertible_v<T, common::span<EleType>>)
            return (common::span<EleType>)arg;
        else if constexpr (detail::IsContainerLike<T>)
            return common::span<std::remove_reference_t<decltype(*arg.data())>>(arg.data(), arg.size());
        else
            return Fallback()(std::forward<T>(arg));
    }
    else if constexpr (common::is_detected_v<detail::CanAsSpan, T>)
    {
        return to_span(arg.AsSpan());
    }
    else 
        return Fallback()(std::forward<T>(arg));
}
template<typename X>
inline constexpr bool CanToSpan = !std::is_same_v<decltype(to_span<X&, detail::SkipToSpan>(std::declval<X&>())), void>;
}


