#pragma once

#include "CommonRely.hpp"



/* aligned allocation */

#include <new>
#include <cstdlib>
#if COMMON_OS_MACOS
#   include <malloc/malloc.h>
#else
#   include <malloc.h>
#endif



namespace common
{


#if COMMON_OS_LINUX || COMMON_OS_FREEBSD
[[nodiscard]] forceinline void* malloc_align(const size_t size, const size_t align) noexcept
{
    return memalign(align, size);
}
forceinline void free_align(void* ptr) noexcept
{
    free(ptr);
}
#elif COMMON_OS_WIN && COMPILER_MSVC
[[nodiscard]] forceinline void* malloc_align(const size_t size, const size_t align) noexcept
{
    return _aligned_malloc(size, align);
}
forceinline void free_align(void* ptr) noexcept
{
    _aligned_free(ptr);
}
#else
[[nodiscard]] forceinline void* malloc_align(const size_t size, const size_t align) noexcept
{
    static_assert(alignof(std::max_align_t) >= alignof(size_t), "malloc should at least satisfy alignment of size_t");
    constexpr size_t infoPadding = sizeof(size_t);
    constexpr size_t infoPaddingMask = ~(sizeof(size_t) - 1);

    const size_t extraPadding = align - 1 + infoPadding;
    if (align == 0 || !common::IsPower2(align) || SIZE_MAX - size < extraPadding)
        return nullptr;

    const auto ptr = malloc(size + extraPadding);
    if (ptr == nullptr)
        return nullptr;
    auto offset = uintptr_t(ptr) % align;
    if (offset < infoPadding) offset += align;

    const auto newptrval = uintptr_t(ptr) + offset;
    const auto offsetptrval = (newptrval & infoPaddingMask) - infoPadding;
    *reinterpret_cast<size_t*>(offsetptrval) = offset;

    return reinterpret_cast<void*>(newptrval);
}
forceinline void free_align(void* ptr) noexcept
{
    constexpr size_t infoPadding = sizeof(size_t);
    constexpr size_t infoPaddingMask = ~(sizeof(size_t) - 1);

    if (ptr == nullptr) return;

    const auto newptrval = uintptr_t(ptr);
    const auto offsetptrval = (newptrval & infoPaddingMask) - infoPadding;
    const size_t offset = *reinterpret_cast<const size_t*>(offsetptrval);

    free(reinterpret_cast<void*>(newptrval - offset));
}
#endif


[[nodiscard]] forceinline void* mallocn_align(const size_t size, const size_t align) noexcept
{
    if (align == 0 || !common::IsPower2(align) || size % align != 0)
        return nullptr;
#if COMMON_OS_UNIX
    void* ptr = nullptr;
    if (posix_memalign(&ptr, align, size))
        return nullptr;
    return ptr;
    //#elif COMMON_OS_WIN && COMPILER_MSVC
#else
    return malloc_align(size, align);
#endif
}
forceinline void freen_align(void* ptr) noexcept
{
#if COMMON_OS_UNIX
    free(ptr);
    //#elif COMMON_OS_WIN && COMPILER_MSVC
#else
    free_align(ptr);
#endif
}



template<size_t Align>
struct AlignBase
{
    static_assert(Align != 0 && common::IsPower2(Align), "Alignment should be non-zero and power-of-2");
    template<typename> friend struct AlignBaseHelper;
private:
    static constexpr size_t ALIGN_SIZE = Align;
public:
    static void* operator new(size_t size) noexcept
    {
        return malloc_align(size, ALIGN_SIZE);
    };
    static void operator delete(void* p) noexcept
    {
        return free_align(p);
    }
    static void* operator new[](size_t size) noexcept
    {
        return malloc_align(size, ALIGN_SIZE);
    };
    static void operator delete[](void* p) noexcept
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
    static constexpr size_t GetAlignSize() noexcept
    {
        if constexpr (IsDerivedFromAlignBase)
            return T::ALIGN_SIZE;
        else
            return alignof(T);
    }
public:
    static constexpr bool IsDerivedFromAlignBase = decltype(is_derived_from_alignbase_impl(std::declval<T*>()))::value;
    static constexpr size_t Align = GetAlignSize();
};


}