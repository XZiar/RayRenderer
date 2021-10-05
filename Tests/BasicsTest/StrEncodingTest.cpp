#include "pch.h"
#include "SystemCommon/StrEncoding.hpp"

using namespace common::str;
using namespace std::string_view_literals;
using charset::ToLowerEng;
using charset::ToUpperEng;
using charset::to_string;
using charset::to_u8string;
using charset::to_u16string;
using charset::to_u32string;


static constexpr auto U8Str  =       u8string_view(u8"aBcD1\U00000707\U0000A5EE\U00010086");
static constexpr auto U32Str = std::u32string_view( U"aBcD1\U00000707\U0000A5EE\U00010086");
static constexpr auto U16Str = std::u16string_view( u"aBcD1\U00000707\U0000A5EE\U00010086");



template<typename T>
static auto ToIntSpan(T&& str) noexcept
{
    auto sv = ToStringView(str);
    constexpr auto CharSize = sizeof(typename decltype(sv)::value_type);
    if constexpr (CharSize == 1)
        return common::span<const uint8_t >(reinterpret_cast<const uint8_t *>(sv.data()), sv.size());
    else if constexpr (CharSize == 2)
        return common::span<const uint16_t>(reinterpret_cast<const uint16_t*>(sv.data()), sv.size());
    else if constexpr (CharSize == 4)
        return common::span<const uint32_t>(reinterpret_cast<const uint32_t*>(sv.data()), sv.size());
    else
        static_assert(!common::AlwaysTrue<T>);
}
template<typename T>
static auto ToByteSpan(T&& str) noexcept
{
    auto sv = ToStringView(str);
    using Char = typename decltype(sv)::value_type;
    return common::as_bytes(common::span<const Char>(sv.data(), sv.size()));
}

template<typename T>
static void CompareSpans(const common::span<T> l, const common::span<T> r) noexcept
{
    EXPECT_EQ(l.size(), r.size());
    for (size_t i = 0; i < l.size(); ++i)
        EXPECT_EQ(l[i], r[i]) << "element [" << i << "], mismatch with [" << l[i] << "] and [" << r[i] << "].\n";
}

#define CHK_STR_BYTE_EQ(src, ref) CompareSpans(ToByteSpan(src), ToByteSpan(ref))
#define CHK_STR_UNIT_EQ(src, ref) CompareSpans( ToIntSpan(src),  ToIntSpan(ref))



#if COMMON_COMPILER_MSVC
inline namespace
{
template<typename From, typename To, typename CharIn>
static constexpr auto CTConv0(std::basic_string_view<CharIn> input) noexcept
{
    static_assert(std::is_same_v<CharIn, typename From::ElementType>, "CharIn  mismatch");
    using CharOut = typename To::ElementType;
    std::array<CharOut, 100> output = { 0 };
    size_t idx = 0;
    while (input.size() > 0)
    {
        const auto [cp, cnt1] = From::From(input.data(), input.size());
        if (cnt1 > 0)
        {
            input.remove_prefix(cnt1);
            const auto cnt2 = To::To(cp, output.size(), &output[idx]);
            idx += cnt2;
        }
        else
            input.remove_prefix(1);
    }
    return std::pair{ output, idx };
}
template<typename From, typename To, typename Src>
static constexpr auto CTConv1() noexcept
{
    constexpr auto input = Src()();
    //static_assert(common::is_specialization<decltype(input), std::basic_string_view>::value, "Source should be string_view");
    return CTConv0<From, To>(input);
}
template<typename From, typename To, typename Src>
static constexpr auto CTConv2() noexcept
{
    constexpr auto ret = CTConv1<From, To, Src>();
    constexpr auto ar = ret.first;
    constexpr auto cnt = ret.second;
    std::array<typename decltype(ar)::value_type, cnt> str = { 0 };
    for (size_t i = 0; i < cnt; ++i)
        str[i] = ar[i];
    return str;
}

template<typename Char, size_t N>
constexpr bool CTCheck(const std::array<Char, N> src, const std::basic_string_view<Char> ref)
{
    for (size_t i = 0; i < N; ++i)
        if (src[i] != ref[i])
            return false;
    return true;
}
}
#define CTSource(str) []() constexpr { return str; };

//constexpr auto u8s = CTSource(u"hello"sv);
//constexpr auto u8a = u8s();
//constexpr auto u8b = CTConv0<detail::UTF16, detail::UTF8>(u8a);
//constexpr auto u8c = CTConv1<detail::UTF16, detail::UTF8, decltype(u8s)>();
//constexpr auto u8d = CTConv2<detail::UTF16, detail::UTF8, decltype(u8s)>();
//constexpr auto u8e = CTCheck(u8d, u8"hello"sv);

#define CTConvCheck(From, To, Src, Dst, str) []() constexpr                 \
{                                                                           \
    constexpr auto srcstr = CTSource(Src ## str ## sv);                     \
    constexpr auto dststr = CTConv2<                                        \
        charset::detail::From, charset::detail::To, decltype(srcstr)>();    \
    return CTCheck(dststr, Dst ## str ## sv);                               \
}()                                                                         \


TEST(StrChset, CompileTime)
{
    static_assert(CTConvCheck(UTF8,  UTF8,  u8, u8, "aBcD1\U00000707\U0000A5EE\U00010086"),  "utf8->utf8" );
    static_assert(CTConvCheck(UTF16, UTF8,   u, u8, "aBcD1\U00000707\U0000A5EE\U00010086"), "utf16->utf8" );
    static_assert(CTConvCheck(UTF32, UTF8,   U, u8, "aBcD1\U00000707\U0000A5EE\U00010086"), "utf32->utf8" );
    static_assert(CTConvCheck(UTF8,  UTF16, u8,  u, "aBcD1\U00000707\U0000A5EE\U00010086"),  "utf8->utf16");
    static_assert(CTConvCheck(UTF16, UTF16,  u,  u, "aBcD1\U00000707\U0000A5EE\U00010086"), "utf16->utf16");
    static_assert(CTConvCheck(UTF32, UTF16,  U,  u, "aBcD1\U00000707\U0000A5EE\U00010086"), "utf32->utf16");
    static_assert(CTConvCheck(UTF8,  UTF32, u8,  U, "aBcD1\U00000707\U0000A5EE\U00010086"),  "utf8->utf32");
    static_assert(CTConvCheck(UTF16, UTF32,  u,  U, "aBcD1\U00000707\U0000A5EE\U00010086"), "utf16->utf32");
    static_assert(CTConvCheck(UTF32, UTF32,  U,  U, "aBcD1\U00000707\U0000A5EE\U00010086"), "utf32->utf32");

}
#endif


TEST(StrChset, Upper)
{
    EXPECT_EQ(ToUpperEng(""), "");
    EXPECT_EQ(ToUpperEng("aBcD1\0/"), "ABCD1\0/");
    EXPECT_EQ(ToUpperEng(u"aBcD1\0/\u0444", Encoding::UTF16LE), u"ABCD1\0/\u0444");
    EXPECT_EQ(ToUpperEng(U"aBcD1\0/\u0444", Encoding::UTF32LE), U"ABCD1\0/\u0444");
}


TEST(StrChset, Lower)
{
    EXPECT_EQ(ToLowerEng(""), "");
    EXPECT_EQ(ToLowerEng("aBcD1\0/"), "abcd1\0/");
    EXPECT_EQ(ToLowerEng(u"aBcD1\0/\u0444", Encoding::UTF16LE), u"abcd1\0/\u0444");
    EXPECT_EQ(ToLowerEng(U"aBcD1\0/\u0444", Encoding::UTF32LE), U"abcd1\0/\u0444");
}


TEST(StrChset, utf16)
{
    EXPECT_EQ(to_u16string("A0"), u"A0");
    EXPECT_EQ(to_u16string(U32Str, Encoding::UTF32LE), U16Str);
    EXPECT_EQ(to_u32string(U16Str, Encoding::UTF16LE), U32Str);
}


TEST(StrChset, utf8)
{
    CHK_STR_UNIT_EQ(to_u8string ("A0"), "A0");
    CHK_STR_UNIT_EQ(to_u8string (U32Str, Encoding::UTF32LE),  U8Str);
    CHK_STR_UNIT_EQ(to_u32string(U8Str , Encoding::UTF8   ), U32Str);
}


TEST(StrChset, utf32)
{
    EXPECT_EQ(to_u32string("A0"), U"A0");
    EXPECT_EQ(to_string(U32Str, Encoding::ASCII, Encoding::UTF32LE), "aBcD1");
}


