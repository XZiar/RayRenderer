#pragma once

#include "CommonRely.hpp"
#include <cstdint>
#include <type_traits>
#include <utility>

namespace common::container
{


#define DEFINE_EASY_ITER(name, H, V, ...)                                       \
class name                                                                      \
{                                                                               \
    friend H;                                                                   \
private:                                                                        \
    H* Host;                                                                    \
    size_t Idx;                                                                 \
    constexpr name(H* host, size_t idx) noexcept : Host(host), Idx(idx) {}      \
public:                                                                         \
    using iterator_category = std::random_access_iterator_tag;                  \
    using value_type = V;                                                       \
    using difference_type = std::ptrdiff_t;                                     \
    [[nodiscard]] value_type operator*() const noexcept                         \
    {                                                                           \
        return __VA_ARGS__;                                                     \
    }                                                                           \
    [[nodiscard]] constexpr bool operator!=(const name& other) const noexcept   \
    {                                                                           \
        return Host != other.Host || Idx != other.Idx;                          \
    }                                                                           \
    [[nodiscard]] constexpr bool operator==(const name& other) const noexcept   \
    {                                                                           \
        return Host == other.Host && Idx == other.Idx;                          \
    }                                                                           \
    constexpr name& operator++() noexcept                                       \
    {                                                                           \
        Idx++;                                                                  \
        return *this;                                                           \
    }                                                                           \
    constexpr name& operator--() noexcept                                       \
    {                                                                           \
        Expects(Idx >= 1);                                                      \
        Idx--;                                                                  \
        return *this;                                                           \
    }                                                                           \
    constexpr name& operator+=(size_t n) noexcept                               \
    {                                                                           \
        Idx += n;                                                               \
        return *this;                                                           \
    }                                                                           \
    constexpr name& operator-=(size_t n) noexcept                               \
    {                                                                           \
        Expects(Idx >= n);                                                      \
        Idx -= n;                                                               \
        return *this;                                                           \
    }                                                                           \
}


template<typename T, typename V, auto F>
DEFINE_EASY_ITER(IndirectIterator, T, V, (Host->*F)(Idx));


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