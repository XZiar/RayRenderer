#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef STRCHSET_EXPORT
#   define STRCHSETAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define STRCHSETAPI _declspec(dllimport)
# endif
#else
# ifdef STRCHSET_EXPORT
#   define STRCHSETAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define STRCHSETAPI
# endif
#endif

#include "common/CommonRely.hpp"
#include "common/Exceptions.hpp"
#include "common/StrBase.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <memory>
#include <string_view>


namespace common::strchset
{

namespace detail
{
template<typename T>
struct IsSpan : std::false_type 
{};

template <typename ElementType, std::ptrdiff_t Extent>
struct IsSpan<common::span<ElementType, Extent>> : std::true_type
{ };

template<typename T>
using HasValueType = typename T::value_type;

template <typename T>
common::span<const std::byte> ToByteSpan(T&& arg)
{
    using U = common::remove_cvref_t<T>;
    if constexpr (IsSpan<U>::value)
        return common::as_bytes(arg);
    else if constexpr (common::is_detected_v<HasValueType, U>)
        return ToByteSpan(common::span<std::add_const_t<typename U::value_type>>(arg));
    else if constexpr (std::is_convertible_v<T, common::span<const std::byte>>)
        return arg;
    else if constexpr (std::is_pointer_v<U>)
        return ToByteSpan(std::basic_string_view(arg));
    else
        static_assert(!common::AlwaysTrue<T>(), "unsupported");
}

template <typename T>
auto ToStringView(T&& arg)
{
    using U = common::remove_cvref_t<T>;
    if constexpr (common::is_detected_v<HasValueType, U>)
        return std::basic_string_view(arg.data(), arg.size());
    else if constexpr (std::is_pointer_v<U>)
        return ToByteSpan(std::basic_string_view(arg));
    else
        static_assert(!common::AlwaysTrue<T>(), "unsupported");
}


}

}
