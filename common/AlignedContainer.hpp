#pragma once

#include "CommonRely.hpp"
#include <atomic>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace common
{

template<size_t Align>
struct AlignBase
{
public:
#if defined(__cpp_lib_gcd_lcm)
    static constexpr size_t ALIGN_SIZE = std::lcm((size_t)Align, (size_t)32);
#else
#  message("C++17 unsupported, AlignSize may be incorrect")
    static constexpr size_t ALIGN_SIZE = std::max((size_t)Align, (size_t)32);
#endif
    static void* operator new(size_t size)
    {
        return malloc_align(size, ALIGN_SIZE);
    };
    static void operator delete(void* p)
    {
        return free_align(p);
    }
    static void* operator new[](size_t size)
    {
        return malloc_align(size, ALIGN_SIZE);
    };
    static void operator delete[](void* p)
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
        if constexpr (IsDerivedFromAlignBase)
            return T::ALIGN_SIZE;
        else
            return AlignBase<alignof(T)>::ALIGN_SIZE;
    }
public:
    static constexpr bool IsDerivedFromAlignBase = decltype(is_derived_from_alignbase_impl(std::declval<T*>()))::value;
    static constexpr size_t Align = GetAlignSize();
};


template <class T>
struct AlignAllocator
{
    using value_type = T;
    static constexpr size_t Align = AlignBaseHelper<T>::Align;

    AlignAllocator() noexcept = default;
    template<class U>
    AlignAllocator(const AlignAllocator<U>&) noexcept { }

    template<class U>
    bool operator==(const AlignAllocator<U>&) const noexcept
    {
        return true;
    }
    template<class U>
    bool operator!=(const AlignAllocator<U>&) const noexcept
    {
        return false;
    }
    T* allocate(const size_t n) const
    {
        T *ptr = nullptr;
        if (n == 0)
            return ptr;
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_array_new_length();
        ptr = (T*)malloc_align(n * sizeof(T), Align);
        if (!ptr)
            throw std::bad_alloc();
        else
            return ptr;
    }
    void deallocate(T* const p, const size_t) const noexcept
    {
        free_align(p);
    }
};


namespace container
{

template<class T>
class vectorEx : public std::vector<T, common::AlignAllocator<T>>
{
public:
    using std::vector<T, common::AlignAllocator<T>>::vector;
    operator const std::vector<T>& () const
    {
        return *(const std::vector<T>*)this;
    }
};

template<class T>
using dequeEx = std::deque<T, common::AlignAllocator<T>>;

template<class K, class V, class C = std::less<K>>
using mapEx = std::map<K, V, C, common::AlignAllocator<std::pair<const K, V>>>;

template<class T, class C = std::less<T>>
using setEx = std::set<T, C, common::AlignAllocator<T>>;

template<class K, class V, class H = std::hash<K>, class E = std::equal_to<K>>
using hashmapEx = std::unordered_map<K, H, E, V, common::AlignAllocator<std::pair<const K, V>>>;

template<class T, class H = std::hash<T>, class E = std::equal_to<T>>
using hashsetEx = std::unordered_set<T, H, E, common::AlignAllocator<T>>;
}

}
