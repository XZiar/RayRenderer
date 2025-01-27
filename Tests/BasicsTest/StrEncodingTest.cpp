#include "pch.h"
#include "common/simd/SIMD.hpp"
#include "SystemCommon/StrEncoding.hpp"

using namespace common::str;
using namespace std::string_view_literals;
using charset::ToLowerEng;
using charset::ToUpperEng;
using charset::detail::UTF7;
using charset::detail::UTF8;
using charset::detail::UTF16LE;
using charset::detail::UTF16BE;
using charset::detail::UTF32LE;
using charset::detail::UTF32BE;
using charset::detail::GB18030;


template<typename Char>
[[nodiscard]] inline std::u32string to_u32string(const Char* str, size_t size, const Encoding inchset)
{
    std::u32string ret;
    switch (inchset)
    {
    case Encoding::ASCII:
        charset::detail::Transform2<UTF7, UTF32LE>(str, size, ret); break;
    case Encoding::UTF8:
        charset::detail::Transform2<UTF8, UTF32LE>(str, size, ret); break;
    case Encoding::UTF16LE:
        charset::detail::Transform2<UTF16LE, UTF32LE>(str, size, ret); break;
    case Encoding::UTF16BE:
        charset::detail::Transform2<UTF16BE, UTF32LE>(str, size, ret); break;
    case Encoding::UTF32LE:
        Ensures(false);
    case Encoding::UTF32BE:
        charset::detail::Transform2<UTF32BE, UTF32LE>(str, size, ret); break;
    case Encoding::GB18030:
        charset::detail::Transform2<GB18030, UTF32LE>(str, size, ret); break;
    default: CM_UNREACHABLE(); break;
    }
    return ret;
}
template<typename Char>
[[nodiscard]] inline std::u32string to_u32string(std::basic_string_view<Char> str, const Encoding inchset)
{
    return to_u32string(str.data(), str.size(), inchset);
}


template<typename Char>
[[nodiscard]] inline std::u16string to_u16string(const Char* str, size_t size, const Encoding inchset)
{
    std::u16string ret;
    switch (inchset)
    {
    case Encoding::ASCII:
        charset::detail::Transform2<UTF7, UTF16LE>(str, size, ret); break;
    case Encoding::UTF8:
        charset::detail::Transform2<UTF8, UTF16LE>(str, size, ret); break;
    case Encoding::UTF16LE:
        Ensures(false);
    case Encoding::UTF16BE:
        charset::detail::Transform2<UTF16BE, UTF16LE>(str, size, ret); break;
    case Encoding::UTF32LE:
        charset::detail::Transform2<UTF32LE, UTF16LE>(str, size, ret); break;
    case Encoding::UTF32BE:
        charset::detail::Transform2<UTF32BE, UTF16LE>(str, size, ret); break;
    case Encoding::GB18030:
        charset::detail::Transform2<GB18030, UTF16LE>(str, size, ret); break;
    default: CM_UNREACHABLE(); break;
    }
    return ret;
}
template<typename Char>
[[nodiscard]] inline std::u16string to_u16string(std::basic_string_view<Char> str, const Encoding inchset)
{
    return to_u16string(str.data(), str.size(), inchset);
}


template<typename Char>
[[nodiscard]] inline std::u8string to_u8string(const Char* str, size_t size, const Encoding inchset)
{
    std::u8string ret;
    switch (inchset)
    {
    case Encoding::ASCII:
    case Encoding::UTF8:
        Ensures(false);
    case Encoding::UTF16LE:
        charset::detail::Transform2<UTF16LE, UTF8>(str, size, ret); break;
    case Encoding::UTF16BE:
        charset::detail::Transform2<UTF16BE, UTF8>(str, size, ret); break;
    case Encoding::UTF32LE:
        charset::detail::Transform2<UTF32LE, UTF8>(str, size, ret); break;
    case Encoding::UTF32BE:
        charset::detail::Transform2<UTF32BE, UTF8>(str, size, ret); break;
    case Encoding::GB18030:
        charset::detail::Transform2<GB18030, UTF8>(str, size, ret); break;
    default: CM_UNREACHABLE(); break;
    }
    return ret;
}
template<typename Char>
[[nodiscard]] inline std::u8string to_u8string(std::basic_string_view<Char> str, const Encoding inchset)
{
    return to_u8string(str.data(), str.size(), inchset);
}



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
static void CompareSpans(const common::span<const T> src, const common::span<const T> ref) noexcept
{
    EXPECT_EQ(src.size(), ref.size());
    for (size_t i = 0; i < src.size(); ++i)
    {
        IF_LIKELY(src[i] == ref[i]) continue;
        EXPECT_EQ(src[i], ref[i]) << "element [" << i << "], mismatch with [" << src[i] << "] and [" << ref[i] << "].\n";
        break;
    }
}

#define CHK_STR_BYTE_EQ(src, ref, scope) do { SCOPED_TRACE(scope); CompareSpans(ToByteSpan(src), ToByteSpan(ref)); } while(0)
#define CHK_STR_UNIT_EQ(src, ref, scope) do { SCOPED_TRACE(scope); CompareSpans( ToIntSpan(src),  ToIntSpan(ref)); } while(0)



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
#define CTSource(str) []() constexpr { return str; };

//constexpr auto u8s = CTSource(u8"hello"sv);
//constexpr auto u8a = u8s();
//constexpr auto u8b = CTConv0<charset::detail::UTF8, charset::detail::UTF16>(u8a);
//constexpr auto u8c = CTConv1<charset::detail::UTF8, charset::detail::UTF16, decltype(u8s)>();
//constexpr auto u8d = CTConv2<charset::detail::UTF8, charset::detail::UTF16, decltype(u8s)>();
//constexpr auto u8e = CTCheck(u8d, u"hello"sv);


#define CTConvCheck(From, To, Src, Dst, str) []() constexpr                 \
{                                                                           \
    constexpr auto srcstr = CTSource(Src ## str ## sv);                     \
    constexpr auto dststr = CTConv2<                                        \
        charset::detail::From, charset::detail::To, decltype(srcstr)>();    \
    return CTCheck(dststr, Dst ## str ## sv);                               \
}()



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


static constexpr uint32_t UTF32Max = 0x0010FFFFu;
static constexpr uint32_t UTF32SurrogateHi = 0xD800u;
static constexpr uint32_t UTF32SurrogateLo = 0xDC00u;
static constexpr uint32_t UTF32SurrogateEnd = 0xDFFFu;

template<typename F>
static forceinline constexpr auto EachUnicode(F&& func) noexcept -> std::invoke_result_t<F, char32_t>
{
    for (uint32_t i = 0; i < UTF32SurrogateHi; ++i)
    {
        const auto ret = func(static_cast<char32_t>(i));
        IF_UNLIKELY(ret)
            return ret;
    }
    for (uint32_t i = UTF32SurrogateEnd + 1; i <= UTF32Max; ++i)
    {
        const auto ret = func(static_cast<char32_t>(i));
        IF_UNLIKELY(ret)
            return ret;
    }
    return {};
}
template<typename F>
static forceinline constexpr auto EachInvalidUnicode(F&& func) noexcept -> std::invoke_result_t<F, char32_t>
{
    for (uint32_t i = UTF32SurrogateHi; i <= UTF32SurrogateEnd; ++i)
    {
        const auto ret = func(static_cast<char32_t>(i));
        IF_UNLIKELY(ret)
            return ret;
    }
    return {};
}

static const auto AllUnicode = []()
{
    std::vector<char32_t> ret;
    ret.reserve(UTF32Max + 1);
    EachUnicode([&](char32_t ch) { ret.push_back(ch); return false; });
    return ret;
}();

template<typename Conv, typename LE, typename BE, uint32_t IVTBase, uint32_t IVTShift, typename F>
static void TestConvert(F&& reffunc)
{
    using Char = typename Conv::ElementType;
    using U = std::make_unsigned_t<Char>;
    constexpr uint32_t N = Conv::MaxOutputUnit;
    constexpr size_t TotalSize = N * sizeof(Char);
    static_assert(TotalSize == 4);
    using T = std::conditional_t<TotalSize == 4, uint32_t, uint64_t>;
    std::vector<T> validSets[N];

    constexpr auto Masks = []() 
    {
        std::array<T, N> masks = { 0 };
        for (uint32_t i = 0; i < N; ++i)
        {
            masks[i] = static_cast<T>(std::numeric_limits<U>::max()) << (i * sizeof(U) * 8);
        }
        for (uint32_t i = 1; i < N; ++i)
        {
            masks[i] += masks[i - 1];
        }
        return masks;
    }();
    
    const auto mismatch = EachUnicode([&](char32_t ch) -> std::optional<std::pair<char32_t, uint32_t>>
    {
        std::array<Char, N> tmp = { 0 }, ref = { 0 };
        reffunc(ch, ref);

        const auto cnt0 = Conv::To(ch, tmp.size(), tmp.data());
        IF_UNLIKELY(!cnt0)
            return std::pair{ ch, 0u };
        IF_UNLIKELY(tmp != ref)
            return std::pair{ ch, 1u };

        const auto [outch, cnt1] = Conv::From(tmp.data(), cnt0);
        IF_UNLIKELY(!cnt1)
            return std::pair{ ch, 2u };
        IF_UNLIKELY(cnt1 != cnt0)
            return std::pair{ ch, 3u };
        IF_UNLIKELY(outch != ch)
            return std::pair{ ch, 4u };

        [[maybe_unused]] std::array<uint8_t, TotalSize> tmp2 = { 0 }, tmp3 = { 0 };
        static_assert(sizeof(tmp) == sizeof(tmp2));
        const auto cnt2 = LE::ToBytes(ch, tmp2.size(), tmp2.data());
        IF_UNLIKELY(!cnt2)
            return std::pair{ ch, 5u };
        IF_UNLIKELY(cnt2 != cnt0 * sizeof(Char))
            return std::pair{ ch, 6u };
        IF_UNLIKELY(memcmp(ref.data(), tmp2.data(), tmp2.size()))
            return std::pair{ ch, 7u };

        const auto [lech, cnt4] = LE::FromBytes(std::launder(reinterpret_cast<const uint8_t*>(ref.data())), TotalSize);
        IF_UNLIKELY(!cnt4)
            return std::pair{ ch, 8u };
        IF_UNLIKELY(cnt4 != cnt0 * sizeof(Char))
            return std::pair{ ch, 9u };
        IF_UNLIKELY(lech != ch)
            return std::pair{ ch, 10u };

        if constexpr (sizeof(Char) > 1)
        {
            std::array<Char, N> refBE = { 0 };
            for (uint32_t i = 0; i < tmp.size(); ++i)
                refBE[i] = common::ByteSwap(ref[i]);

            const auto cnt3 = BE::ToBytes(ch, tmp3.size(), tmp3.data());
            IF_UNLIKELY(!cnt3)
                return std::pair{ ch, 11u };
            IF_UNLIKELY(cnt3 != cnt0 * sizeof(Char))
                return std::pair{ ch, 12u };
            IF_UNLIKELY(memcmp(refBE.data(), tmp3.data(), tmp3.size()))
                return std::pair{ ch, 13u };

            const auto [bech, cnt5] = BE::FromBytes(std::launder(reinterpret_cast<const uint8_t*>(refBE.data())), TotalSize);
            IF_UNLIKELY(!cnt5)
                return std::pair{ ch, 14u };
            IF_UNLIKELY(cnt5 != cnt0 * sizeof(Char))
                return std::pair{ ch, 15u };
            IF_UNLIKELY(bech != ch)
                return std::pair{ ch, 16u };
        }

        validSets[cnt0 - 1].push_back(*reinterpret_cast<const T*>(ref.data()));
        return {};
    });
    EXPECT_FALSE(mismatch) << "when convert [" << static_cast<uint32_t>(mismatch->first) << "] at stage " << mismatch->second;

    {
        TestCout out;
        out << "valid count: ";
        for (auto& vec : validSets)
        {
            out << vec.size() << " ";
            std::sort(vec.begin(), vec.end());
        }
        out << "\n";
    }

    constexpr auto Check = [](const T val)
    {
        bool sucLE = true, sucBE = true;
        const auto [lech, cnt0] = LE::FromBytes(reinterpret_cast<const uint8_t*>(&val), TotalSize);
        sucLE = (cnt0 == 0);
        if constexpr (sizeof(Char) > 1)
        {
            std::array<U, N> beval = { 0 };
            memcpy(beval.data(), &val, TotalSize);
            for (uint32_t i = 0; i < N; ++i)
                beval[i] = common::ByteSwap(beval[i]);
            const auto [bech, cnt1] = BE::FromBytes(reinterpret_cast<const uint8_t*>(beval.data()), TotalSize);
            sucBE = (cnt1 == 0);
        }
        return std::pair{ sucLE, sucBE };
    };
    uint32_t invalidTestCount = 0;
#if CM_DEBUG
    std::mt19937 gen(0);
    constexpr uint32_t invalidTestFull = 0x1000000u;
    for (uint32_t i = 0; i < invalidTestFull; ++i)
    {
        const auto val = gen();
#else
    constexpr uint32_t invalidTestFull = (UINT32_MAX >> 5) + 1;
    for (uint32_t i = 0; i < invalidTestFull; ++i)
    {
        const auto val = IVTBase + (i << IVTShift);
#endif
        bool isValid = false;
        for (uint32_t idx = 0; idx < N; ++idx)
        {
            const auto testVal = val & Masks[idx];
            if (std::binary_search(validSets[idx].begin(), validSets[idx].end(), testVal))
            {
                isValid = true;
                break;
            }
        }
        if (isValid)
            continue;
        invalidTestCount++;
        const auto ret = Check(val);
        constexpr auto suc = std::pair{ true, true };
        EXPECT_EQ(ret, suc) << "when convert [" << val << "]";
        if (!ret.first || !ret.second)
            break;
    }
    TestCout() << "Invalid Test tried [" << invalidTestCount << "/" << invalidTestFull << "]\n";
}

TEST(StrChset, AllUTF32)
{
    using Conv = charset::detail::UTF32;
    using LE = charset::detail::UTF32LE;
    using BE = charset::detail::UTF32BE;
    const auto validtest = EachUnicode([&](char32_t ch) -> std::optional<char32_t>
    {
        IF_LIKELY(charset::detail::ConvertCPBase::CheckCPValid(ch))
            return {};
        else
            return ch;
    });
    EXPECT_EQ(validtest, std::optional<char32_t>{});
    const auto invalidtest = EachInvalidUnicode([&](char32_t ch) -> std::optional<char32_t>
    {
        IF_LIKELY(charset::detail::ConvertCPBase::CheckCPValid(ch))
            return ch;
        else
            return {};
    });
    EXPECT_EQ(invalidtest, std::optional<char32_t>{});
    TestConvert<Conv, LE, BE, 0, 5>([](char32_t ch, std::array<Conv::ElementType, Conv::MaxOutputUnit>& out)
    {
        out[0] = ch;
    });
}

TEST(StrChset, AllUTF16)
{
    using Conv = charset::detail::UTF16;
    using LE = charset::detail::UTF16LE;
    using BE = charset::detail::UTF16BE;
    TestConvert<Conv, LE, BE, 0, 5>([](char32_t ch, std::array<Conv::ElementType, Conv::MaxOutputUnit>& out)
    {
        if (ch <= 0xffffu)
        {
            Expects(!(ch >= UTF32SurrogateHi && ch <= UTF32SurrogateEnd));
            out[0] = static_cast<char16_t>(ch);
        }
        else
        {
            const uint32_t val = ch - 0x10000u;
            out[0] = static_cast<char16_t>((val >> 10) + UTF32SurrogateHi);
            out[1] = static_cast<char16_t>((val & 0x3ffu) + UTF32SurrogateLo);
        }
    });

    const auto allutf16 = to_u16string(AllUnicode.data(), AllUnicode.size(), Encoding::UTF32);
    const auto allutf32 = to_u32string(allutf16.data(), allutf16.size(), Encoding::UTF16);
    CHK_STR_UNIT_EQ(allutf32, AllUnicode, "AllUnicode To UTF16");
}

TEST(StrChset, AllUTF8)
{
    using Conv = charset::detail::UTF8;
    static constexpr std::array<uint8_t, 7> ByteMasks = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
    // shift 0 to ensure byte0 get all permutation
    TestConvert<Conv, Conv, Conv, 0x80000000u, 1>([](char32_t ch, std::array<Conv::ElementType, Conv::MaxOutputUnit>& out)
    {
        uint32_t val = ch;
        uint32_t bytes = 0;
        if (ch < 0x80)
            bytes = 1;
        else if (ch < 0x800)
            bytes = 2;
        else if (ch < 0x10000)
            bytes = 3;
        else
            bytes = 4;
        switch (bytes)
        {
        case 4:
            out[3] = static_cast<Conv::ElementType>((val | 0x80u) & 0xBFu);
            val >>= 6;
            [[fallthrough]];
        case 3:
            out[2] = static_cast<Conv::ElementType>((val | 0x80u) & 0xBFu);
            val >>= 6;
            [[fallthrough]];
        case 2:
            out[1] = static_cast<Conv::ElementType>((val | 0x80u) & 0xBFu);
            val >>= 6;
            [[fallthrough]];
        case 1:
            out[0] = static_cast<Conv::ElementType>(val | ByteMasks[bytes]);
            val >>= 6;
            break;
        default:
            CM_UNREACHABLE();
            break;
        }
    });

    const auto allutf8 = to_u8string(AllUnicode.data(), AllUnicode.size(), Encoding::UTF32);
    const auto allutf32 = to_u32string(allutf8.data(), allutf8.size(), Encoding::UTF8);
    CHK_STR_UNIT_EQ(allutf32, AllUnicode, "AllUnicode To UTF8");
}


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

constexpr std::string_view StrA0 = "A0"sv;

TEST(StrChset, utf16)
{
    EXPECT_EQ(to_u16string(StrA0, Encoding::UTF7), u"A0");
    EXPECT_EQ(to_u16string(U32Str, Encoding::UTF32LE), U16Str);
    EXPECT_EQ(to_u32string(U16Str, Encoding::UTF16LE), U32Str);
}


TEST(StrChset, utf8)
{
    CHK_STR_UNIT_EQ(to_u8string (StrA0, Encoding::UTF7), "A0", "utf8");
    CHK_STR_UNIT_EQ(to_u8string (U32Str, Encoding::UTF32LE),  U8Str, "utf8");
    CHK_STR_UNIT_EQ(to_u32string(U8Str, Encoding::UTF8), U32Str, "utf8");
}


TEST(StrChset, utf32)
{
    EXPECT_EQ(to_u32string(StrA0, Encoding::UTF7), U"A0");
    std::string utf7str;
    charset::detail::Transform2<UTF32LE, UTF7>(U32Str.data(), U32Str.size(), utf7str);
    EXPECT_EQ(utf7str, "aBcD1");
}


