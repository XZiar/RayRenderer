#pragma once

#include "CommonRely.hpp"



/* aligned allocation */

#include <new>
#if COMMON_OS_MACOS && false // posix_memalign doesnot allow arbitrary size
#   include <malloc/malloc.h>
[[nodiscard]] forceinline void* malloc_align(const size_t size, const size_t align) noexcept
{
    void* ptr = nullptr;
    if (posix_memalign(&ptr, align, size))
        return nullptr;
    return ptr;
}
forceinline void free_align(void* ptr) noexcept
{
    free(ptr);
}
#elif COMMON_OS_LINUX || COMMON_OS_FREEBSD
#   include <malloc.h>
[[nodiscard]] forceinline void* malloc_align(const size_t size, const size_t align) noexcept
{
    return memalign(align, size);
}
forceinline void free_align(void* ptr) noexcept
{
    free(ptr);
}
#elif COMMON_OS_WIN
#   include <malloc.h>
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
    if (align == 0 || !common::IsPower2(align) || SIZE_MAX - size < 2 * align)
        return nullptr;

    const auto ptr = malloc(size + 2 * align);
    if (ptr == nullptr)
        return nullptr;
    auto offset = uintptr_t(ptr) % align;
    if (offset == 0) offset = align;
    
    const auto newptrval = uintptr_t(ptr) + offset;
    uint8_t* const baseoffset = reinterpret_cast<uint8_t*>(newptrval - 1);
    if (offset <= 255)
    {
        *baseoffset = static_cast<uint8_t>(offset);
    }
    else
    {
        *baseoffset = 0;
        *reinterpret_cast<size_t*>(newptrval - 2 * sizeof(size_t)) = offset;
    }

    return reinterpret_cast<void*>(newptrval);
}
forceinline void free_align(void* ptr) noexcept
{
    if (ptr == nullptr) return;
    const auto ptrval = uintptr_t(ptr);
    const auto baseoffset = *reinterpret_cast<const uint8_t*>(ptrval - 1);
    const size_t offset = baseoffset == 0 ?
        *reinterpret_cast<const size_t*>(ptrval - 2 * sizeof(size_t)) :
        baseoffset;
    free(reinterpret_cast<void*>(ptrval - offset));
}
#endif


namespace common
{

template<size_t Align>
struct AlignBase
{
    static_assert(Align != 0 && common::IsPower2(Align), "Alignment should be non-zero and power-of-2");
public:
#if defined(__cpp_lib_gcd_lcm)
    static constexpr size_t ALIGN_SIZE = std::lcm(Align, (size_t)32);
#else
    static constexpr size_t ALIGN_SIZE = std::max(Align, (size_t)32);
#endif
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
            return AlignBase<alignof(T)>::ALIGN_SIZE;
    }
public:
    static constexpr bool IsDerivedFromAlignBase = decltype(is_derived_from_alignbase_impl(std::declval<T*>()))::value;
    static constexpr size_t Align = GetAlignSize();
};


}