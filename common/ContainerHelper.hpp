#pragma once
#include "CommonRely.hpp"
#include "AlignedBuffer.hpp"
#include <array>
#include <string>
#include <vector>


namespace common
{
namespace container
{

namespace detail
{
struct NonContiguous
{
    constexpr static bool IsContiguous = false;
};
struct DoContiguous
{
    constexpr static bool IsContiguous = true;
};
template<typename T>
struct StdContiguous : DoContiguous
{
    static auto Data(T& container) { return std::data(container); }
    static auto Data(const T& container) { return std::data(container); }
    constexpr static size_t Count(const T& container) { return std::size(container); }
    constexpr static size_t EleSize = sizeof(*std::data(std::declval<T&>()));
};
}
template<typename T>
struct ContiguousHelper : detail::NonContiguous {};
template<typename T, typename A>
struct ContiguousHelper<std::vector<T, A>> : detail::StdContiguous<std::vector<T, A>> {};
template<typename A>
struct ContiguousHelper<std::vector<bool, A>> : detail::NonContiguous {};
template<typename T, size_t N>
struct ContiguousHelper<std::array<T, N>> : detail::StdContiguous<std::array<T, N>> {};
template<typename T, size_t N>
struct ContiguousHelper<T[N]> : detail::StdContiguous<T[N]> {};
template<typename Ch, typename T, typename A>
struct ContiguousHelper<std::basic_string<Ch, T, A>> : detail::StdContiguous<std::basic_string<Ch, T, A>> {};
template<>
struct ContiguousHelper<common::AlignedBuffer> : detail::DoContiguous
{
    static auto Data(common::AlignedBuffer& container) { return container.GetRawPtr(); }
    static auto Data(const common::AlignedBuffer& container) { return container.GetRawPtr(); }
    constexpr static size_t Count(const common::AlignedBuffer& container) { return container.GetSize(); }
    constexpr static size_t EleSize = 1;
};

}
}
