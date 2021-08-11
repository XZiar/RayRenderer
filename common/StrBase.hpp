#pragma once

#include "CommonRely.hpp"
#include <boost/predef/other/endian.h>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

namespace common::str
{
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
using u8string = std::u8string;
using u8string_view = std::u8string_view;
#else
using u8string = std::string;
using u8string_view = std::string_view;
#endif


template<typename T>
[[nodiscard]] inline constexpr auto ToStringView(T&& val) noexcept
{
    using U = std::decay_t<T>;
    if constexpr (std::is_pointer_v<U>)
        return std::basic_string_view(val);
    else if constexpr (common::has_valuetype_v<U>)
    {
        using Char = typename U::value_type;
        if constexpr (std::is_constructible_v<std::basic_string_view<Char>, T>)
            return std::basic_string_view<Char>(std::forward<T>(val));
        else if constexpr (std::is_convertible_v<T, std::basic_string_view<Char>>)
            return (std::basic_string_view<Char>)val;
        else if constexpr (std::is_constructible_v<common::span<const Char>, T>)
        {
            common::span<const Char> space(std::forward<T>(val));
            return std::basic_string_view<Char>(space.data(), space.size());
        }
        else if constexpr (std::is_convertible_v<T, common::span<const Char>>)
        {
            auto space = (common::span<const Char>)val;
            return std::basic_string_view<Char>(space.data(), space.size());
        }
        else
            static_assert(!common::AlwaysTrue<T>, "with value_type, still not able to be converted into string_view");
    }
    else
        static_assert(!common::AlwaysTrue<T>, "unsupported type to be converted into string_view");
}

template<typename Ch>
struct StrVariant
{
private:
    std::basic_string<Ch> Str;
    std::basic_string_view<Ch> View;
public:
    StrVariant() noexcept { }
    template<size_t N>
    StrVariant(const Ch(&str)[N]) noexcept : View(str) { }
    StrVariant(std::basic_string<Ch>&& str) noexcept : Str(std::move(str)), View(Str) { }
    StrVariant(const std::basic_string<Ch>& str) noexcept : View(str) { }
    StrVariant(const std::basic_string_view<Ch> str) noexcept : View(str) { }
    StrVariant(const StrVariant<Ch>& other) noexcept :
        Str(other.Str), View(Str.empty() ? other.View : Str) { }
    StrVariant(StrVariant<Ch>&& other) noexcept : 
        Str(std::move(other.Str)), View(Str.empty() ? other.View : Str) 
    {
        other.View = {};
    }

    StrVariant& operator=(const StrVariant<Ch>& other) noexcept
    {
        View = {};
        Str = other.Str;
        if (!Str.empty())
            View = Str;
        return *this;
    }
    StrVariant& operator=(StrVariant<Ch>&& other) noexcept
    {
        View = {};
        Str = std::move(other.Str);
        if (!Str.empty())
            View = Str;
        other.View = {};
        return *this;
    }

    [[nodiscard]] constexpr std::basic_string_view<Ch> StrView() const noexcept
    {
        return View;
    }
    [[nodiscard]] std::basic_string<Ch> ExtractStr() noexcept
    {
        if (Str.empty())
        {
            std::basic_string<Ch> ret(View);
            View = {};
            return ret;
        }
        else
        {
            View = {};
            return std::move(Str);
        }
    }
    constexpr operator common::span<const Ch>() const noexcept
    {
        return common::span<const Ch>(View.data(), View.size());
    }
    constexpr bool operator==(const std::basic_string_view<Ch> str) const noexcept
    {
        return View == str;
    }

    decltype(auto) begin() const noexcept
    {
        return View.begin();
    }
    decltype(auto) end() const noexcept
    {
        return View.end();
    }
    decltype(auto) data() const noexcept
    {
        return View.data();
    }
    decltype(auto) size() const noexcept
    {
        return View.size();
    }
    constexpr bool empty() const noexcept
    {
        return View.empty();
    }

};


template<typename Hasher = DJBHash>
struct PreHashed
{
    uint64_t Hash;
    constexpr PreHashed(const uint64_t hash) noexcept : Hash(hash) { }
    template<typename T>
    constexpr PreHashed(T&& str) noexcept : Hash(Hasher::Hash(str)) { }
};


template<typename Ch>
struct HashedStrView : public PreHashed<DJBHash>
{
    std::basic_string_view<Ch> View;
    constexpr HashedStrView() noexcept : 
        PreHashed(DJBHash::HashC(std::basic_string_view<Ch>{})), View{} { }
    constexpr HashedStrView(std::basic_string_view<Ch> str) noexcept : 
        PreHashed(DJBHash::HashC(str)), View(str) { }
    constexpr explicit HashedStrView(const uint64_t hash, std::basic_string_view<Ch> str) noexcept :
        PreHashed(hash), View(str) { }
    constexpr operator std::basic_string_view<Ch>() const noexcept 
    { 
        return View;
    }
    constexpr bool operator==(const std::basic_string_view<Ch> other) const noexcept 
    { 
        return View == other;
    }
    constexpr bool operator==(const HashedStrView<Ch>& other) const noexcept 
    { 
        return Hash == other.Hash && View == other;
    }
};


namespace detail
{
struct ShortStrBase
{
    template<typename T, size_t Bits, size_t... Idx>
    static constexpr T Pack(const char(&str)[sizeof...(Idx) + 1], std::index_sequence<Idx...>) noexcept
    {
        constexpr size_t M = sizeof...(Idx);
        T val = 0;
        (..., void(val += static_cast<T>(str[Idx]) << ((M - Idx - 1) * Bits)));
        return val;
    }
    template<typename T, size_t Bits, typename C>
    static constexpr T Pack(const std::basic_string_view<C>& str) noexcept
    {
        constexpr T limit = T(1) << Bits;
        T val = 0;
        const auto N = str.size();
        for (size_t i = 0; i < N; ++i)
        {
            const auto num = static_cast<T>(str[i]);
            if (num >= limit) return std::numeric_limits<T>::max();
            val += num << ((N - i - 1) * Bits);
        }
        return val;
    }
};
}

template<size_t N, bool Compact = false>
struct ShortStrVal
{
    static constexpr size_t EleBits = Compact ? 7 : 8;
    static constexpr size_t TotalBits = EleBits * N;
    using T = std::conditional_t<(TotalBits > 32), uint64_t, uint32_t>;
    T Val;
    constexpr ShortStrVal() noexcept : Val(0) {}
    template<size_t M>
    constexpr ShortStrVal(const char(&str)[M]) noexcept :
        Val(M > N + 1 ? std::numeric_limits<T>::max() :
            detail::ShortStrBase::template Pack<T, EleBits>(str, std::make_index_sequence<M - 1>{}))
    { }
    constexpr ShortStrVal(std::u32string_view str) noexcept :
        Val(str.size() > N ? std::numeric_limits<T>::max() :
            detail::ShortStrBase::template Pack<T, EleBits, char32_t>(str))
    { }
    constexpr bool operator==(const ShortStrVal& other) const noexcept { return Val == other.Val; }
    constexpr bool operator<(const ShortStrVal& other) const noexcept { return Val < other.Val; }
};


enum class Charset 
{ 
    ASCII, 
    UTF7 = ASCII, 
    GB18030, 
    UTF8, 
    UTF16LE, 
    UTF16BE, 
    UTF32LE, 
    UTF32BE,
#if BOOST_ENDIAN_LITTLE_BYTE
    UTF16 = UTF16LE,
    UTF32 = UTF32LE,
#elif BOOST_ENDIAN_BIG_BYTE
    UTF16 = UTF16BE,
    UTF32 = UTF32BE,
#endif
};


namespace detail
{
template<typename T>
struct DefCharset
{
    static constexpr Charset Val = Charset::ASCII;
};
template<> struct DefCharset<char>
{
    static constexpr Charset Val = Charset::ASCII;
};
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template<> struct DefCharset<char8_t>
{
    static constexpr Charset Val = Charset::UTF8;
};
#endif
#if BOOST_ENDIAN_LITTLE_BYTE || BOOST_ENDIAN_BIG_BYTE
template<> struct DefCharset<char16_t>
{
    static constexpr Charset Val = Charset::UTF16;
};
template<> struct DefCharset<char32_t>
{
    static constexpr Charset Val = Charset::UTF32;
};
#endif
#if COMMON_COMPILER_MSVC
template<> struct DefCharset<wchar_t>
{
    static constexpr Charset Val = Charset::UTF16LE;
};
#elif BOOST_ENDIAN_LITTLE_BYTE || BOOST_ENDIAN_BIG_BYTE
template<> struct DefCharset<wchar_t>
{
    static constexpr Charset Val = sizeof(wchar_t) == 4 ? Charset::UTF32 : Charset::UTF16;
};
#endif
}
template<typename T>
inline constexpr Charset DefaultCharset = detail::DefCharset<std::decay_t<T>>::Val;


inline constexpr Charset toCharset(const std::string_view chset) noexcept
{
    switch (DJBHash::HashC(chset))
    {
    case "GB18030"_hash:
        return Charset::GB18030;
    case "UTF-8"_hash:
        return Charset::UTF8;
    case "UTF-16LE"_hash:
        return Charset::UTF16LE;
    case "UTF-16BE"_hash:
        return Charset::UTF16BE;
    case "UTF-32LE"_hash:
        return Charset::UTF32LE;
    case "UTF-32BE"_hash:
        return Charset::UTF32BE;
    case "error"_hash:
        return Charset::ASCII;
    default:
        return Charset::ASCII;
    }
}

inline constexpr std::string_view getCharsetName(const Charset chset) noexcept
{
    using namespace std::string_view_literals;
    switch (chset)
    {
    case Charset::ASCII:
        return "ASCII"sv;
    case Charset::GB18030:
        return "GB18030"sv;
    case Charset::UTF8:
        return "UTF-8"sv;
    case Charset::UTF16BE:
        return "UTF-16BE"sv;
    case Charset::UTF16LE:
        return "UTF-16LE"sv;
    case Charset::UTF32LE:
        return "UTF-32LE"sv;
    case Charset::UTF32BE:
        return "UTF-32BE"sv;
    default:
        return "error"sv;
    }
}



}