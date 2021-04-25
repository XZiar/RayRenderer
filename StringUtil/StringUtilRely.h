#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef STRCHSET_EXPORT
#   define STRCHSETAPI _declspec(dllexport)
# else
#   define STRCHSETAPI _declspec(dllimport)
# endif
#else
# define STRCHSETAPI [[gnu::visibility("default")]]
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


namespace common::str
{

namespace detail
{

template <typename T>
inline common::span<const std::byte> ToByteSpan(T&& arg)
{
    using U = common::remove_cvref_t<T>;
    if constexpr (common::is_span_v<T>)
        return common::as_bytes(arg);
    else if constexpr (common::has_valuetype_v<U>)
        return ToByteSpan(common::span<std::add_const_t<typename U::value_type>>(arg));
    else if constexpr (std::is_convertible_v<T, common::span<const std::byte>>)
        return arg;
    else if constexpr (std::is_pointer_v<U>)
        return ToByteSpan(std::basic_string_view(arg));
    else
        static_assert(!common::AlwaysTrue<T>, "unsupported");
}
template <typename T>
inline constexpr common::str::Charset GetDefaultCharset() noexcept
{
    using U = common::remove_cvref_t<T>;
    if constexpr (common::is_span_v<T>)
        return common::str::DefaultCharset<typename T::value_type>;
    else if constexpr (common::has_valuetype_v<U>)
        return common::str::DefaultCharset<typename U::value_type>;
    else if constexpr (std::is_pointer_v<U>)
        return common::str::DefaultCharset<std::remove_pointer_t<U>>;
    else if constexpr (std::is_convertible_v<T, common::span<const std::byte>>)
        return common::str::DefaultCharset<std::byte>;
    else
        return common::str::DefaultCharset<T>;
}


}

}
