#pragma once

#include "AlignedBase.hpp"
#include "CommonRely.hpp"
#include <cstdlib>
#include <atomic>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace common
{

template <class T>
struct AlignAllocator
{
    using value_type = T;
    static constexpr size_t Align = AlignBaseHelper<T>::Align;
    static constexpr auto IsMultiple = sizeof(T) % Align == 0;

    constexpr AlignAllocator() noexcept = default;
    template<class U>
    constexpr AlignAllocator(const AlignAllocator<U>&) noexcept { }

    template<class U>
    constexpr bool operator==(const AlignAllocator<U>&) const noexcept
    {
        return true;
    }
    template<class U>
    constexpr bool operator!=(const AlignAllocator<U>&) const noexcept
    {
        return false;
    }
    [[nodiscard]] T* allocate(const size_t n) const
    {
        T *ptr = nullptr;
        if (n == 0)
            return ptr;
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_array_new_length();

        if constexpr (IsMultiple && false)
            ptr = reinterpret_cast<T*>(aligned_alloc(Align, n * sizeof(T)));
        else
            ptr = reinterpret_cast<T*>(malloc_align(n * sizeof(T), Align));

        if (!ptr)
            throw std::bad_alloc();
        else
            return ptr;
    }
    void deallocate(T* const p, const size_t) const noexcept
    {
        if (p == nullptr) return;
        if constexpr (IsMultiple && false)
            std::free(p);
        else
            free_align(p);
    }
};


namespace container
{

template<class T>
using vectorEx = std::vector<T, common::AlignAllocator<T>>;

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
