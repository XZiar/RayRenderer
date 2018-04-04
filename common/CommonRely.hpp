#pragma once

#ifdef COMMON_EXPORT
#   define COMMONAPI _declspec(dllexport) 
#   define COMMONTPL _declspec(dllexport) 
#else
#   define COMMONAPI _declspec(dllimport) 
#   define COMMONTPL
#endif

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <numeric>
#include <type_traits> 

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
#define forceinline __forceinline
#elif defined(__GNUC__)
#define forceinline __inline__ __attribute__((always_inline))
#else
#define forceinline inline
#endif

/**
** @brief calculate simple hash for string, used for switch-string
** @param str std-string_view/string for the text
** @return uint64_t the hash
**/
template<typename T>
inline uint64_t hash_(const T& str)
{
    static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>, "only string/string_view supported for hash_");
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
constexpr inline uint64_t hash_(const char *str)
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
constexpr inline uint64_t operator "" _hash(const char *str, size_t)
{
    return hash_(str);
}


namespace common
{


template<template<typename...> class Base, typename...Ts>
std::true_type is_base_of_template_impl(const Base<Ts...>*);

template<template<typename...> class Base>
std::false_type is_base_of_template_impl(...);

template<template<typename...> class Base, typename Derived>
using is_base_of_template = decltype(is_base_of_template_impl<Base>(std::declval<Derived*>()));


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


template<size_t Align>
struct COMMONTPL AlignBase
{
public:
    static constexpr size_t ALIGN_SIZE = std::lcm((size_t)Align, (size_t)32);
    static void* __cdecl operator new(size_t size)
    {
        return malloc_align(size, ALIGN_SIZE);
    };
    static void __cdecl operator delete(void *p)
    {
        return free_align(p);
    }
    static void* __cdecl operator new[](size_t size)
    {
        return malloc_align(size, ALIGN_SIZE);
    };
    static void __cdecl operator delete[](void *p)
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


}
