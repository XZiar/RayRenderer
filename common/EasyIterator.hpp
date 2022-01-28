#pragma once

#include "CommonRely.hpp"
#include <cstdint>
#include <type_traits>
#include <utility>

namespace common::container
{


template<typename T, typename V, auto F>
class IndirectIterator
{
    friend T;
    /*static_assert(std::is_invocable_r_v<V, decltype(F), T, size_t>, 
        "F need to be member function of T, acceptting size_t and returning V");*/
private:
    T* Host;
    size_t Idx;
    constexpr IndirectIterator(T* host, size_t idx) noexcept : Host(host), Idx(idx) {}
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = V;
    using difference_type = std::ptrdiff_t;

    [[nodiscard]] value_type operator*() const noexcept
    {
        return (Host->*F)(Idx);
    }
    [[nodiscard]] constexpr bool operator!=(const IndirectIterator& other) const noexcept
    {
        return Host != other.Host || Idx != other.Idx;
    }
    [[nodiscard]] constexpr bool operator==(const IndirectIterator& other) const noexcept
    {
        return Host == other.Host && Idx == other.Idx;
    }
    constexpr IndirectIterator& operator++() noexcept
    {
        Idx++;
        return *this;
    }
    constexpr IndirectIterator& operator--() noexcept
    {
        Expects(Idx >= 1);
        Idx--;
        return *this;
    }
    constexpr IndirectIterator& operator+=(size_t n) noexcept
    {
        Idx += n;
        return *this;
    }
    constexpr IndirectIterator& operator-=(size_t n) noexcept
    {
        Expects(Idx >= n);
        Idx -= n;
        return *this;
    }
};

#define COMMON_EASY_ITER_(name, clz, clztype, type, func, vbegin, vend)                 \
    using name##_ = ::common::container::IndirectIterator<clztype, type, &clz::func>;   \
    friend name##_;                                                                     \
    class name                                                                          \
    {                                                                                   \
        friend clz;                                                                     \
        clztype* Host;                                                                  \
        constexpr name(clztype* host) noexcept : Host(host) { }                         \
    public:                                                                             \
        constexpr name##_ begin() const noexcept { return { Host, vbegin }; }           \
                  name##_ end()   const noexcept { return { Host, vend }; }             \
                  size_t  size()  const noexcept { return vend - vbegin; }              \
                  bool    empty() const noexcept { return vend == vbegin; }             \
    }

#define COMMON_EASY_ITER(name, clz, type, func, vbegin, vend)       COMMON_EASY_ITER_(name, clz,       clz, type, func, vbegin, vend)
#define COMMON_EASY_CONST_ITER(name, clz, type, func, vbegin, vend) COMMON_EASY_ITER_(name, clz, const clz, type, func, vbegin, vend)



}