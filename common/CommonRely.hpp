#pragma once

#if defined(_WIN32)
# ifdef COMMON_EXPORT
#   define COMMONAPI _declspec(dllexport) 
#   define COMMONTPL _declspec(dllexport) 
# else
#   define COMMONAPI _declspec(dllimport) 
#   define COMMONTPL
# endif
#else
# ifdef COMMON_EXPORT
#   define COMMONAPI __attribute__((visibility("default")))
#   define COMMONTPL __attribute__((visibility("default")))
# else
#   define COMMONAPI 
#   define COMMONTPL
# endif
#endif

#define __STDC_WANT_SECURE_LIB__ 1
#define __STDC_WANT_LIB_EXT1__ 1
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <new>
#include <numeric>
#include <type_traits> 
#include <initializer_list>
#if (defined(_HAS_CXX17) && _HAS_CXX17) || (defined(__cplusplus) && (__cplusplus >= 201703L))
#   include<variant>
#endif


#if defined(__APPLE__)
#   include <malloc/malloc.h>
inline void* apple_malloc_align(const size_t size, const size_t align)
{
    void* ptr = nullptr;
    if (posix_memalign(&ptr, align, size))
        return nullptr;
    return ptr;
}
#   define malloc_align(size, align) apple_malloc_align((size), (align))
#   define free_align(ptr) free(ptr)
#elif defined(__GNUC__)
#   include <malloc.h>
#   define malloc_align(size, align) memalign((align), (size))
#   define free_align(ptr) free(ptr)
#elif defined(_MSC_VER)
#   define malloc_align(size, align) _aligned_malloc((size), (align))
#   define free_align(ptr) _aligned_free(ptr)
#endif

#if defined(_MSC_VER)
#   define forceinline      __forceinline
#   define forcenoinline    __declspec(noinline)
#if defined(__STDC_LIB_EXT1__)
#  define KKK 1
# endif
#elif defined(__GNUC__)
#   define forceinline      __inline__ __attribute__((always_inline))
#   define forcenoinline    __attribute__((noinline))
#   if !defined(__STDC_LIB_EXT1__)
#       include <errno.h>
forceinline std::remove_reference<decltype(errno)>::type memcpy_s(void * dest, size_t destsz, const void * src, size_t count)
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
forceinline std::remove_reference<decltype(errno)>::type memmove_s(void * dest, size_t destsz, const void * src, size_t count)
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
#   define _FILE_OFFSET_BITS 64
#   endif
#else
#   define forceinline inline
#   define forcenoinline 
#endif

#if defined(_WIN32) && !defined(_MANAGED) && !defined(_M_CEE)
#   define VECCALL __vectorcall
#else
#   define VECCALL 
#endif

#if defined(__clang__)
#   define COMPILER_CLANG 1
#elif defined(__GNUC__)
#   define COMPILER_GCC 1
#elif defined(_MSC_VER)
#   define COMPILER_MSVC 1
#endif

#ifndef COMPILER_CLANG
#   define COMPILER_CLANG 0
#endif
#ifndef COMPILER_GCC
#   define COMPILER_GCC 0
#endif
#ifndef COMPILER_MSVC
#   define COMPILER_MSVC 0
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


#define ENUM_CLASS_BITFIELD_FUNC(T, U) \
inline constexpr T  operator &  (const T x, const T y) { return static_cast<T>(static_cast<U>(x) & static_cast<U>(y)); } \
inline constexpr T& operator &= (T& x, const T y) { x = x & y; return x; } \
inline constexpr T  operator |  (const T x, const T y) { return static_cast<T>(static_cast<U>(x) | static_cast<U>(y)); } \
inline constexpr T& operator |= (T& x, const T y) { x = x | y; return x; } \
inline constexpr T  operator ^  (const T x, const T y) { return static_cast<T>(static_cast<U>(x) ^ static_cast<U>(y)); } \
inline constexpr T& operator ^= (T& x, const T y) { x = x ^ y; return x; } \
inline constexpr T  operator ~  (const T x) { return static_cast<T>(~static_cast<U>(x)); } \
inline constexpr bool HAS_FIELD(const T x, const T obj) { return static_cast<U>(x & obj) != 0; } \
inline constexpr T REMOVE_MASK(const T x, const T mask) \
{ \
    return x & (~mask); \
} \
inline constexpr T REMOVE_MASK(const T x, const T mask, const T mask2) \
{ \
    return x & (~(mask | mask2)); \
} \
template<typename... Masks> \
inline constexpr T REMOVE_MASK(const T x, const T mask, const T mask2, const Masks... masks) \
{ \
    return REMOVE_MASK(x, mask | mask2, masks...); \
} \
inline constexpr bool MATCH_ANY(const T x, const std::initializer_list<T> objs) \
{ \
    for (const auto obj : objs) \
        if (x == obj) return true; \
    return false; \
}

#define MAKE_ENUM_BITFIELD(T) ENUM_CLASS_BITFIELD_FUNC(T, std::underlying_type_t<T>)


//for std::byte
#if (defined(_HAS_STD_BYTE) && _HAS_STD_BYTE) || (defined(__cplusplus) && (__cplusplus >= 201703L) || defined(__cpp_lib_byte))
inline constexpr bool HAS_FIELD(const std::byte b, const uint8_t bits) { return static_cast<uint8_t>(b & std::byte(bits)) != 0; }
#endif


/**
** @brief calculate simple hash for string, used for switch-string
** @param str std-string_view/string for the text
** @return uint64_t the hash
**/
template<typename T>
inline constexpr uint64_t hash_(const T& str)
{
    uint64_t hash = 0;
    for (size_t a = 0, len = str.length(); a < len; ++a)
        hash = hash * 33 + str[a];
    return hash;
}
/**
** @brief calculate simple hash for string, used for switch-string
** @param str c-string for the text
** @return uint64_t the hash
**/
inline constexpr uint64_t hash_(const char *str)
{
    uint64_t hash = 0;
    for (; *str != '\0'; ++str)
        hash = hash * 33 + *str;
    return hash;
}
/**
** @brief calculate simple hash for string, used for switch-string
** @return uint64_t the hash
**/
inline constexpr uint64_t operator "" _hash(const char *str, size_t)
{
    return hash_(str);
}


namespace common
{

template<typename T>
constexpr bool AlwaysTrue() { return true; }

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

#if defined(__cpp_lib_variant)
template <typename> struct variant_tag { };
template <typename T, typename... Ts>
inline constexpr size_t get_variant_index(variant_tag<std::variant<Ts...>>)
{
    return std::variant<variant_tag<Ts>...>(variant_tag<T>()).index();
}
template <typename T, typename V>
inline constexpr size_t get_variant_index_v()
{
    return get_variant_index<T>(variant_tag<V>());
}
#endif

template<class T, class = typename std::enable_if<std::is_arithmetic<T>::value>::type>
constexpr const T& max(const T& left, const T& right)
{
    return left < right ? right : left;
}
template<class T, class = typename std::enable_if<std::is_arithmetic<T>::value>::type>
constexpr const T& min(const T& left, const T& right)
{
    return left < right ? left : right;
}

struct NonCopyable
{
    NonCopyable() noexcept = default;
    ~NonCopyable() noexcept = default;
    NonCopyable(const NonCopyable&) noexcept = delete;
    NonCopyable& operator= (const NonCopyable&) noexcept = delete;
    NonCopyable(NonCopyable&&) noexcept = default;
    NonCopyable& operator= (NonCopyable&&) noexcept = default;
};

struct NonMovable
{
    NonMovable() noexcept = default;
    ~NonMovable() noexcept = default;
    NonMovable(NonMovable&&) noexcept = delete;
    NonMovable& operator= (NonMovable&&) noexcept = delete;
    NonMovable(const NonMovable&) noexcept = default;
    NonMovable& operator= (const NonMovable&) noexcept = default;
};


#if defined(__cpp_lib_string_view)
#include <string_view>
#  if COMPILER_CLANG
#    define U8STR_CONSTEXPR 
#else
#    define U8STR_CONSTEXPR constexpr
#  endif
class u8StrView
{
private:
    const intptr_t Ptr;
    const size_t Size;
public:
    constexpr u8StrView(const u8StrView& other) noexcept : Ptr(other.Ptr), Size(other.Size) {}
    u8StrView(u8StrView&&) noexcept = delete;
    u8StrView& operator=(const u8StrView&) = delete;
    u8StrView& operator=(u8StrView&&) = delete;
    constexpr size_t Length() const noexcept { return Size; }
   
    U8STR_CONSTEXPR u8StrView(const std::string_view& sv) noexcept :
        Ptr((intptr_t)(sv.data())), Size(sv.length()) { }
    template<size_t N> U8STR_CONSTEXPR u8StrView(const char(&str)[N]) noexcept :
        Ptr((intptr_t)(str)), Size(std::char_traits<char>::length(str)) { }

    U8STR_CONSTEXPR const char* CharData() const noexcept { return (const char*)(Ptr); }
    U8STR_CONSTEXPR operator std::string_view() const noexcept { return { CharData(), Length() }; }

#if defined(__cpp_char8_t) && defined(__cpp_lib_char8_t)
    U8STR_CONSTEXPR u8StrView(const std::u8string_view& sv) noexcept :
        Ptr((intptr_t)(sv.data())), Size(sv.length()) { }
    template<size_t N> U8STR_CONSTEXPR u8StrView(const char8_t(&str)[N]) noexcept :
        Ptr((intptr_t)(str)), Size(std::char_traits<char8_t>::length(str)) { }

    U8STR_CONSTEXPR const char8_t* U8Data() const noexcept { return (const char8_t*)(Ptr); }
    U8STR_CONSTEXPR operator std::u8string_view() const noexcept { return { U8Data(), Length() }; }
#endif
};
#  undef U8STR_CONSTEXPR
#endif

template<size_t Align>
struct AlignBase
{
public:
#if defined(__cpp_lib_gcd_lcm)
    static constexpr size_t ALIGN_SIZE = std::lcm((size_t)Align, (size_t)32);
#else
#   message("C++17 unsupported, AlignSize may be incorrect")
    static constexpr size_t ALIGN_SIZE = std::max((size_t)Align, (size_t)32);
#endif
    static void* operator new(size_t size)
    {
        return malloc_align(size, ALIGN_SIZE);
    };
    static void operator delete(void *p)
    {
        return free_align(p);
    }
    static void* operator new[](size_t size)
    {
        return malloc_align(size, ALIGN_SIZE);
    };
    static void operator delete[](void *p)
    {
        return free_align(p);
    }
};

template<typename T>
struct AlignBaseHelper
{
private:
    template<size_t Align>
    static std::true_type is_derived_from_alignbase_impl(const AlignBase<Align>*);
    static std::false_type is_derived_from_alignbase_impl(const void*);
    static constexpr size_t GetAlignSize()
    {
        if constexpr(IsDerivedFromAlignBase)
            return T::ALIGN_SIZE;
        else
            return AlignBase<alignof(T)>::ALIGN_SIZE;
    }
public:
    static constexpr bool IsDerivedFromAlignBase = decltype(is_derived_from_alignbase_impl(std::declval<T*>()))::value;
    static constexpr size_t Align = GetAlignSize();
};

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

}
