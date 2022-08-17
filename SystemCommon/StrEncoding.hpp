#pragma once

#include "StrEncodingBase.hpp"
#include <cstring>
#include <algorithm>


namespace common::str::charset
{


namespace detail
{


#include "LUT_gb18030.tab"

struct GB18030 : public ConvertCPBase, public ConvertByteBase
{
private:
    template<typename T>
    [[nodiscard]] inline static constexpr std::pair<char32_t, uint32_t> InnerFrom(const T* __restrict const src, const size_t size) noexcept
    {
        if (size == 0)
            return InvalidCharPair;
        if ((uint32_t)src[0] < 0x80)//1 byte
        {
            return { src[0], 1 };
        }
        if (size >= 2 && (uint32_t)src[0] >= 0x81 && (uint32_t)src[0] <= 0xfe)
        {
            if (((uint32_t)src[1] >= 0x40 && (uint32_t)src[1] < 0x7f) || ((uint32_t)src[1] >= 0x80 && (uint32_t)src[1] < 0xff))//2 byte
            {
                const uint32_t tmp = ((uint32_t)src[0] << 8) | (uint32_t)src[1];
                const auto ch = LUT_GB18030_2B[tmp - LUT_GB18030_2B_BEGIN];
                return { ch, 2 };
            }
            if (size >= 4 
                && (uint32_t)src[1] >= 0x30 && (uint32_t)src[1] <= 0x39 
                && (uint32_t)src[2] >= 0x81 && (uint32_t)src[2] <= 0xfe 
                && (uint32_t)src[3] >= 0x30 && (uint32_t)src[3] <= 0x39)//4 byte
            {
                const uint32_t tmp = (src[3] - 0x30u) 
                                   + (src[2] - 0x81u) * 10 
                                   + (src[1] - 0x30u) * (0xff - 0x81) * 10 
                                   + (src[0] - 0x81u) * (0xff - 0x81) * 10 * 10;
                const auto ch = tmp < LUT_GB18030_4B_SIZE ? LUT_GB18030_4B[tmp] : 0x10000 + (tmp - LUT_GB18030_4B_SIZE);
                return { ch, 4 };
            }
        }
        return InvalidCharPair;
    }
    template<typename T>
    [[nodiscard]] inline static constexpr uint8_t InnerTo(const char32_t src, const size_t size, T* __restrict const dest) noexcept
    {
        if (src < 0x80)//1 byte
        {
            if (size < 1)
                return 0;
            dest[0] = (char)(src & 0x7f);
            return 1;
        }
        if (src < 0x10000)//2 byte
        {
            const auto tmp = src - LUT_GB18030_R_BEGIN;
            const auto ch = LUT_GB18030_R[tmp];
            if (ch == 0)//invalid
                return 0;
            if (ch < 0xffff)
            {
                if (size < 2)
                    return 0;
                dest[0] = T(ch & 0xff); dest[1] = T(ch >> 8);
                return 2;
            }
            else
            {
                if (size < 4)
                    return 0;
                dest[0] = T(ch & 0xff); dest[1] = T(ch >> 8); dest[2] = T(ch >> 16); dest[3] = T(ch >> 24);
                return 4;
            }
        }
        if (src < 0x200000)//4 byte
        {
            if (size < 4)
                return 0;
            auto tmp = LUT_GB18030_4B_SIZE + src - 0x10000;
            dest[0] = char(tmp / ((0xff - 0x81) * 10 * 10) + 0x81);
            tmp = tmp % ((0xff - 0x81) * 10 * 10);
            dest[1] = char(tmp / ((0xff - 0x81) * 10) + 0x30);
            tmp = tmp % ((0xff - 0x81) * 10);
            dest[2] = char(tmp / 10 + 0x81);
            dest[3] = char((tmp % 10) + 0x30);
            return 4;
        }
        return 0;
    }
public:
    using ElementType = char;
    static constexpr size_t MaxOutputUnit = 4;
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> From(const ElementType* __restrict const src, const size_t size) noexcept
    {
        return InnerFrom(src, size);
    }
    [[nodiscard]] forceinline static constexpr uint8_t To(const char32_t src, const size_t size, ElementType* __restrict const dest) noexcept
    {
        return InnerTo(src, size, dest);
    }
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, const size_t size) noexcept
    {
        return InnerFrom(src, size);
    }
    [[nodiscard]] forceinline static constexpr uint8_t ToBytes(const char32_t src, const size_t size, uint8_t* __restrict const dest) noexcept
    {
        return InnerTo(src, size, dest);
    }
};



template<typename Conv, typename SrcType>
struct CommonDecoder
{
    static_assert(std::is_base_of_v<ConvertByteBase, Conv>, "Conv should be of ConvertByteBase");
protected:
    SrcType Source;
    [[nodiscard]] forceinline constexpr std::pair<char32_t, uint32_t> DecodeOnce() noexcept
    {
        if constexpr (std::is_same_v<typename Conv::ElementType, typename SrcType::value_type>)
            return Conv::From(Source.data(), Source.size());
        else
            return Conv::FromBytes(Source.data(), Source.size());
    }
public:
    constexpr CommonDecoder(const SrcType src) : Source(src)
    { }

    [[nodiscard]] inline constexpr char32_t Decode() noexcept
    {
        while (Source.size() > 0)
        {
            const auto [cp, cnt] = DecodeOnce();
            if (cp != InvalidChar)
            {
                Source = { Source.data() + cnt, Source.size() - cnt };
                return cp;
            }
            else
            {
                Source = { Source.data() +   1, Source.size() -   1 };
            }
        }
        return InvalidChar;
    }
};
template<typename Conv, typename Char>
[[nodiscard]] inline constexpr auto GetDecoder(std::basic_string_view<Char> src)
{
    if constexpr (std::is_same_v<typename Conv::ElementType, Char> && std::is_base_of_v<ConvertCPBase, Conv>)
    {
        return CommonDecoder<Conv, std::basic_string_view<Char>>(src);
    }
    else
    {
        static_assert(std::is_base_of_v<ConvertByteBase, Conv>, "Conv should be of ConvertByteBase");
        common::span<const uint8_t> src2(reinterpret_cast<const uint8_t*>(src.data()), src.size() * sizeof(Char));
        return CommonDecoder<Conv, common::span<const uint8_t>>(src2);
    }
}



template<typename Conv, typename Char>
struct CommonEncoder
{
    static_assert(std::is_base_of_v<ConvertByteBase, Conv>, "Conv should be of ConvertByteBase");
public:
    std::basic_string<Char> Result;
private:
    Char Temp[Conv::MaxOutputUnit * sizeof(typename Conv::ElementType) / sizeof(Char)] = { 0 };
    uint8_t* __restrict TmpPtr;
    [[nodiscard]] forceinline constexpr size_t EncodeOnce(const char32_t cp) noexcept
    {
        if constexpr (std::is_same_v<typename Conv::ElementType, Char>)
            return Conv::To(cp, Conv::MaxOutputUnit, Temp);
        else
        {
            const auto bytes = Conv::ToBytes(cp, sizeof(Temp), TmpPtr);
            return bytes / sizeof(Char);
        }
    }
public:
    CommonEncoder(const size_t hint) : TmpPtr(reinterpret_cast<uint8_t*>(Temp))
    {
        Result.reserve(hint);
    }

    inline constexpr void Encode(const char32_t cp) noexcept
    {
        const auto cnt = EncodeOnce(cp);
        if (cnt > 0)
        {
            Result.append(Temp, cnt);
        }
    }
};
template<typename Conv, typename Char>
[[nodiscard]] inline constexpr auto GetEncoder(const size_t hint = 0)
{
    if constexpr (std::is_same_v<typename Conv::ElementType, Char> && std::is_base_of_v<ConvertCPBase, Conv>)
    {
        return CommonEncoder<Conv, Char>(hint);
    }
    else
    {
        static_assert(std::is_base_of_v<ConvertByteBase, Conv>, "Conv should be of ConvertByteBase");
        static_assert(sizeof(typename Conv::ElementType) % sizeof(Char) == 0, "Conv's minimal output should be multiple of Char type");
        return CommonEncoder<Conv, Char>(hint);
    }
}



//template<typename Dec = Decoder<UTF8, char>, typename Enc = Encoder<UTF16LE, char16_t>>
template<typename Dec, typename Enc>
[[nodiscard]] inline auto Transform(Dec decoder, Enc encoder)
{
    while (true)
    {
        const auto codepoint = decoder.Decode();
        if (codepoint == InvalidChar) 
            break;
        encoder.Encode(codepoint);
    }
    return std::move(encoder.Result);
}
template<typename Dec, typename Enc, typename TransFunc>
[[nodiscard]] inline auto Transform(Dec decoder, Enc encoder, TransFunc trans)
{
    static_assert(std::is_invocable_r_v<char32_t, TransFunc, char32_t>, "TransFunc should accept an char32_t and return another");
    while (true)
    {
        auto codepoint = decoder.Decode();
        if (codepoint == InvalidChar)
            break;
        codepoint = trans(codepoint);
        if (codepoint == InvalidChar)
            break; 
        encoder.Encode(codepoint);
    }
    return std::move(encoder.Result);
}



#define DST_CONSTRAINT(Conv, Char)                                      \
if constexpr(sizeof(typename Conv::ElementType) % sizeof(Char) != 0)    \
    /*Conv's minimal output should be multiple of Char type*/           \
    Expects(false);                                                     \
else                                                                    \



template<typename Dest, typename Src>
[[nodiscard]] forceinline std::basic_string<Dest> DirectAssign(const std::basic_string_view<Src> str) noexcept
{
    if constexpr (sizeof(Src) <= sizeof(Dest))
        return std::basic_string<Dest>(str.cbegin(), str.cend());
    else
    {
        Expects(sizeof(Src) <= sizeof(Dest));
        return {};
    }
}
template<typename Dst, typename Src>
[[nodiscard]] forceinline std::basic_string<Dst> DirectCopyStr(const std::basic_string_view<Src> str) noexcept
{
    return std::basic_string<Dst>(reinterpret_cast<const Dst*>(str.data()), str.size() * sizeof(Src) / sizeof(Dst));
}


template<typename Char>
[[nodiscard]] inline u8string to_u8string_impl(const std::basic_string_view<Char> str, const Encoding inchset)
{
    switch (inchset)
    {
    case Encoding::ASCII:
    case Encoding::UTF8:
        return DirectCopyStr<u8ch_t>(str);
    case Encoding::UTF16LE:
        return Transform(GetDecoder<UTF16LE>(str), GetEncoder<UTF8, u8ch_t>());
    case Encoding::UTF16BE:
        return Transform(GetDecoder<UTF16BE>(str), GetEncoder<UTF8, u8ch_t>());
    case Encoding::UTF32LE:
        return Transform(GetDecoder<UTF32LE>(str), GetEncoder<UTF8, u8ch_t>());
    case Encoding::UTF32BE:
        return Transform(GetDecoder<UTF32BE>(str), GetEncoder<UTF8, u8ch_t>());
    case Encoding::GB18030:
        return Transform(GetDecoder<GB18030>(str), GetEncoder<UTF8, u8ch_t>());
    default: // should not enter, to please compiler
        Expects(false);
        return {};
    }
}


template<typename Char>
[[nodiscard]] inline std::u16string to_u16string_impl(const std::basic_string_view<Char> str, const Encoding inchset)
{
    switch (inchset)
    {
    case Encoding::ASCII:
        return Transform(GetDecoder<UTF7>   (str), GetEncoder<UTF16LE, char16_t>());
    case Encoding::UTF8:
        return Transform(GetDecoder<UTF8>   (str), GetEncoder<UTF16LE, char16_t>());
    case Encoding::UTF16LE:
        return DirectCopyStr<char16_t>(str);
    case Encoding::UTF16BE:
        return Transform(GetDecoder<UTF16BE>(str), GetEncoder<UTF16LE, char16_t>());
    case Encoding::UTF32LE:
        return Transform(GetDecoder<UTF32LE>(str), GetEncoder<UTF16LE, char16_t>());
    case Encoding::UTF32BE:
        return Transform(GetDecoder<UTF32BE>(str), GetEncoder<UTF16LE, char16_t>());
    case Encoding::GB18030:
        return Transform(GetDecoder<GB18030>(str), GetEncoder<UTF16LE, char16_t>());
    default: // should not enter, to please compiler
        Expects(false);
        return {};
    }
}


template<typename Char>
[[nodiscard]] inline std::u32string to_u32string_impl(const std::basic_string_view<Char> str, const Encoding inchset)
{
    switch (inchset)
    {
    case Encoding::ASCII:
        return Transform(GetDecoder<UTF7>   (str), GetEncoder<UTF32LE, char32_t>());
    case Encoding::UTF8:
        return Transform(GetDecoder<UTF8>   (str), GetEncoder<UTF32LE, char32_t>());
    case Encoding::UTF16LE:
        return Transform(GetDecoder<UTF16LE>(str), GetEncoder<UTF32LE, char32_t>());
    case Encoding::UTF16BE:
        return Transform(GetDecoder<UTF16BE>(str), GetEncoder<UTF32LE, char32_t>());
    case Encoding::UTF32LE:
        return DirectCopyStr<char32_t>(str);
    case Encoding::UTF32BE:
        return Transform(GetDecoder<UTF32BE>(str), GetEncoder<UTF32LE, char32_t>());
    case Encoding::GB18030:
        return Transform(GetDecoder<GB18030>(str), GetEncoder<UTF32LE, char32_t>());
    default: // should not enter, to please compiler
        Expects(false);
        return {};
    }
}


template<typename Decoder>
[[nodiscard]] inline std::string to_string_impl(Decoder decoder, const Encoding outchset)
{
    switch (outchset)
    {
    case Encoding::ASCII:
        return Transform(std::move(decoder), GetEncoder<UTF7,    char>());
    case Encoding::UTF8:
        return Transform(std::move(decoder), GetEncoder<UTF8,    char>());
    case Encoding::UTF16LE:
        return Transform(std::move(decoder), GetEncoder<UTF16LE, char>());
    case Encoding::UTF16BE:
        return Transform(std::move(decoder), GetEncoder<UTF16BE, char>());
    case Encoding::UTF32LE:
        return Transform(std::move(decoder), GetEncoder<UTF32LE, char>());
    case Encoding::UTF32BE:
        return Transform(std::move(decoder), GetEncoder<UTF32BE, char>());
    case Encoding::GB18030:
        return Transform(std::move(decoder), GetEncoder<GB18030, char>());
    default: // should not enter, to please compiler
        Expects(false);
        return {};
    }
}
template<typename Char>
[[nodiscard]] inline std::string to_string_impl(const std::basic_string_view<Char> str, const Encoding inchset, const Encoding outchset)
{
    if (inchset == outchset)
        return DirectCopyStr<char>(str);
    switch (inchset)
    {
    case Encoding::ASCII:
        return to_string_impl(GetDecoder<UTF7>   (str), outchset);
    case Encoding::UTF8:
        return to_string_impl(GetDecoder<UTF8>   (str), outchset);
    case Encoding::UTF16LE:
        return to_string_impl(GetDecoder<UTF16LE>(str), outchset);
    case Encoding::UTF16BE:
        return to_string_impl(GetDecoder<UTF16BE>(str), outchset);
    case Encoding::UTF32LE:
        return to_string_impl(GetDecoder<UTF32LE>(str), outchset);
    case Encoding::UTF32BE:
        return to_string_impl(GetDecoder<UTF32BE>(str), outchset);
    case Encoding::GB18030:
        return to_string_impl(GetDecoder<GB18030>(str), outchset);
    default: // should not enter, to please compiler
        Expects(false);
        return {};
    }
}


}


template<typename T>
[[nodiscard]] forceinline std::string to_string(const T& str_, const Encoding outchset = Encoding::ASCII, const Encoding inchset = Encoding::ASCII)
{
    if constexpr (common::is_specialization<T, std::basic_string_view>::value)
    {
        using Char = typename T::value_type;
        return detail::to_string_impl<Char>(str_, inchset, outchset);

    }
    else
    {
        const auto str = ToStringView(str_);
        using Char = typename decltype(str)::value_type;
        return detail::to_string_impl<Char>(str_, inchset, outchset);
    }
}


template<typename T>
[[nodiscard]] forceinline u8string to_u8string(const T& str_, const Encoding inchset = Encoding::ASCII)
{
    if constexpr (common::is_specialization<T, std::basic_string_view>::value)
    {
        using Char = typename T::value_type;
        return detail::to_u8string_impl<Char>(str_, inchset);

    }
    else
    {
        const auto str = ToStringView(str_);
        using Char = typename decltype(str)::value_type;
        return detail::to_u8string_impl<Char>(str_, inchset);
    }
}


template<typename T>
[[nodiscard]] forceinline std::u16string to_u16string(const T& str_, const Encoding inchset = Encoding::ASCII)
{
    if constexpr (common::is_specialization<T, std::basic_string_view>::value)
    {
        using Char = typename T::value_type;
        return detail::to_u16string_impl<Char>(str_, inchset);

    }
    else
    {
        const auto str = ToStringView(str_);
        using Char = typename decltype(str)::value_type;
        return detail::to_u16string_impl<Char>(str_, inchset);
    }
}


template<typename T>
[[nodiscard]] forceinline std::u32string to_u32string(const T& str_, const Encoding inchset = Encoding::ASCII)
{
    if constexpr (common::is_specialization<T, std::basic_string_view>::value)
    {
        using Char = typename T::value_type;
        return detail::to_u32string_impl<Char>(str_, inchset);

    }
    else
    {
        const auto str = ToStringView(str_);
        using Char = typename decltype(str)::value_type;
        return detail::to_u32string_impl<Char>(str_, inchset);
    }
}


template<typename T>
[[nodiscard]] forceinline std::wstring to_wstring(const T& str_, const Encoding inchset = Encoding::ASCII)
{
    if constexpr (sizeof(wchar_t) == sizeof(char16_t))
    {
        auto ret = to_u16string(str_, inchset);
        return *reinterpret_cast<std::wstring*>(&ret);
    }
    else if constexpr (sizeof(wchar_t) == sizeof(char32_t))
    {
        auto ret = to_u32string(str_, inchset);
        return *reinterpret_cast<std::wstring*>(&ret);
    }
    else
    {
        static_assert(!common::AlwaysTrue<T>, "wchar_t size unrecognized");
    }
}



template<typename T>
[[nodiscard]] forceinline std::enable_if_t<std::is_integral_v<T>, std::string> to_string (const T val)
{
    return std::to_string(val);
}

template<typename T>
[[nodiscard]] forceinline std::enable_if_t<std::is_integral_v<T>, std::wstring> to_wstring(const T val)
{
    return std::to_wstring(val);
}



namespace detail
{


[[nodiscard]] forceinline constexpr char32_t EngUpper(const char32_t in) noexcept
{
    if (in >= U'a' && in <= U'z')
        return in - U'a' + U'A';
    else 
        return in;
}
[[nodiscard]] forceinline constexpr char32_t EngLower(const char32_t in) noexcept
{
    if (in >= U'A' && in <= U'Z')
        return in - U'A' + U'a';
    else 
        return in;
}

template<typename Char, typename T, typename TransFunc>
[[nodiscard]] inline std::basic_string<Char> DirectTransform(const std::basic_string_view<T> src, TransFunc&& trans)
{
    static_assert(std::is_invocable_r_v<Char, TransFunc, T>, "TransFunc should accept SrcChar and return DstChar");
    std::basic_string<Char> ret;
    ret.reserve(src.size());
    std::transform(src.cbegin(), src.cend(), std::back_inserter(ret), trans);
    return ret;
}



template<typename Conv, typename Char>
struct EngConver
{
private:
    static constexpr bool UseCodeUnit = std::is_same_v<typename Conv::ElementType, Char> && std::is_base_of_v<ConvertCPBase, Conv>;
    using SrcType = std::conditional_t<UseCodeUnit, std::basic_string_view<Char>, common::span<const uint8_t>>;
    
    SrcType Source;
    std::basic_string<Char> Result;
    size_t OutIdx = 0;

    [[nodiscard]] static constexpr SrcType GenerateSource(std::basic_string_view<Char> src) noexcept
    {
        if constexpr (UseCodeUnit)
            return src;
        else
            return common::span<const uint8_t>(reinterpret_cast<const uint8_t*>(src.data()), src.size() * sizeof(Char));
    }
    [[nodiscard]] forceinline constexpr std::pair<char32_t, uint32_t> DecodeOnce() noexcept
    {
        if constexpr (UseCodeUnit)
            return Conv::From     (Source.data(), Source.size());
        else
            return Conv::FromBytes(Source.data(), Source.size());
    }
    [[nodiscard]] forceinline constexpr size_t EncodeOnce(const char32_t cp, const size_t cnt) noexcept
    {
        if constexpr (UseCodeUnit)
            return Conv::To     (cp, cnt, &Result[OutIdx]);
        else
            return Conv::ToBytes(cp, cnt, reinterpret_cast<uint8_t*>(&Result[OutIdx]));
    }
    forceinline constexpr void MoveForward(const size_t cnt) noexcept
    {
        if constexpr (UseCodeUnit)
            OutIdx += cnt;
        else
            OutIdx += cnt / sizeof(Char);
    }
public:
    EngConver(const std::basic_string_view<Char> src) : 
        Source(GenerateSource(src)), Result(src)
    { }

    template<typename TransFunc>
    void Transform(TransFunc&& trans) noexcept
    {
        bool canSkip = true;
        while (Source.size() > 0)
        {
            const auto [cp, cnt] = DecodeOnce();
            if (cp == InvalidChar)
            {
                Source = { Source.data() +   1, Source.size() -   1 };
                canSkip = false;
                continue;
            }
            else
            {
                Source = { Source.data() + cnt, Source.size() - cnt };
                const auto newcp = (cp <= 0xff) ? trans(cp) : cp;
                if (newcp != cp || !canSkip)
                {
                    const auto outcnt = EncodeOnce(newcp, cnt);
                    MoveForward(outcnt);
                    canSkip &= outcnt == cnt;
                }
                else
                    MoveForward(cnt);
            }
        }
        Result.resize(OutIdx);
    }

    template<bool IsUpper>
    void Transform2() noexcept
    {
        bool canSkip = true;
        while (Source.size() > 0)
        {
            const auto [cp, cnt] = DecodeOnce();
            if (cp == InvalidChar)
            {
                Source = { Source.data() + 1, Source.size() - 1 };
                canSkip = false;
                continue;
            }
            else
            {
                Source = { Source.data() + cnt, Source.size() - cnt };
                auto newcp = cp;
                if constexpr (IsUpper)
                {
                    if (cp >= U'a' && cp <= U'z')
                        newcp = cp - U'a' + U'A';
                }
                else
                {
                    if (cp >= U'A' && cp <= U'Z')
                        newcp = cp - U'A' + U'a';
                }
                if (newcp != cp || !canSkip)
                {
                    const auto outcnt = EncodeOnce(newcp, cnt);
                    MoveForward(outcnt);
                    canSkip &= outcnt == cnt;
                }
                else
                    MoveForward(cnt);
            }
        }
        Result.resize(OutIdx);
    }

    template<typename TransFunc>
    static std::basic_string<Char> Convert([[maybe_unused]] const std::basic_string_view<Char> src, [[maybe_unused]] TransFunc&& trans) noexcept
    {
        if constexpr (sizeof(typename Conv::ElementType) % sizeof(Char) != 0)
        {   /*Conv's minimal output should be multiple of Char type*/
            Expects(false); return {};
        }
        else
        {
            EngConver<Conv, Char> conv(src);
            conv.Transform<TransFunc>(std::forward<TransFunc>(trans));
            return std::move(conv.Result);
        }
    }
};



// work around for MSVC's Release build 
template<typename Char, typename TransFunc>
[[nodiscard]] forceinline std::basic_string<Char> DirectConv2(const std::basic_string_view<Char> str, const Encoding inchset, TransFunc&& trans)
{
    //static_assert(std::is_invocable_r_v<char32_t, TransFunc, char32_t>, "TransFunc should accept char32_t and return char32_t");
    switch (inchset)
    {
    case Encoding::ASCII:
        return EngConver<UTF7,    Char>::Convert(str, std::forward<TransFunc>(trans));
    case Encoding::UTF8:
        return EngConver<UTF8,    Char>::Convert(str, std::forward<TransFunc>(trans));
    case Encoding::GB18030:
        return EngConver<GB18030, Char>::Convert(str, std::forward<TransFunc>(trans));
    default: // should not enter, to please compiler
        Expects(false);
        return {};
    }
}
template<typename Char, typename TransFunc>
[[nodiscard]] forceinline std::basic_string<Char> DirectConv(const std::basic_string_view<Char> str, const Encoding inchset, TransFunc&& trans)
{
    //static_assert(std::is_invocable_r_v<char32_t, TransFunc, char32_t>, "TransFunc should accept char32_t and return char32_t");
    switch (inchset)
    {
    case Encoding::UTF16LE:
        return EngConver<UTF16LE, Char>::Convert(str, std::forward<TransFunc>(trans));
    case Encoding::UTF16BE:
        return EngConver<UTF16BE, Char>::Convert(str, std::forward<TransFunc>(trans));
    case Encoding::UTF32LE:
        return EngConver<UTF32LE, Char>::Convert(str, std::forward<TransFunc>(trans));
    case Encoding::UTF32BE:
        return EngConver<UTF32BE, Char>::Convert(str, std::forward<TransFunc>(trans));
    default: // should not enter, to please compiler
        return DirectConv2<Char, TransFunc>(str, inchset, std::forward<TransFunc>(trans));
    }
}

}



template<typename T>
[[nodiscard]] forceinline auto ToUpperEng(const T& str_, const Encoding inchset = Encoding::ASCII)
{
    if constexpr (common::is_specialization<T, std::basic_string_view>::value)
    {
        using Char = typename T::value_type;
        return detail::DirectConv<Char>(str_, inchset, detail::EngUpper);
    }
    else
    {
        const auto str = ToStringView(str_);
        using Char = typename decltype(str)::value_type;
        return detail::DirectConv<Char>(str,  inchset, detail::EngUpper);
    }
}


template<typename T>
[[nodiscard]] forceinline auto ToLowerEng(const T& str_, const Encoding inchset = Encoding::ASCII)
{
    if constexpr (common::is_specialization<T, std::basic_string_view>::value)
    {
        using Char = typename T::value_type;
        return detail::DirectConv<Char>(str_, inchset, detail::EngLower);

    }
    else
    {
        const auto str = ToStringView(str_);
        using Char = typename decltype(str)::value_type;
        return detail::DirectConv<Char>(str,  inchset, detail::EngLower);
    }
}



namespace detail
{
template<typename Decoder>
[[nodiscard]] inline constexpr bool CaseInsensitiveCompare(Decoder str, Decoder prefix) noexcept
{
    while (true)
    {
        const auto strCp    =    str.Decode();
        const auto prefixCp = prefix.Decode();
        if (prefixCp == InvalidChar) // prefix finish
            break;
        if (strCp == InvalidChar) // str finish before prefix
            return false;
        if (strCp != prefixCp) // may need convert
        {
            const auto ch1 = (strCp    >= U'a' && strCp    <= U'z') ? (strCp    - U'a' + U'A') : strCp;
            const auto ch2 = (prefixCp >= U'a' && prefixCp <= U'z') ? (prefixCp - U'a' + U'A') : prefixCp;
            if (ch1 != ch2) // not the same, fast return
                return false;
        }
    }
    return true;
}


template<typename Char>
[[nodiscard]] inline bool IsIBeginWith_impl(const std::basic_string_view<Char> str, const std::basic_string_view<Char> prefix, const Encoding strchset)
{
    switch (strchset)
    {
    case Encoding::ASCII:
        return CaseInsensitiveCompare(GetDecoder<UTF7>   (str), GetDecoder<UTF7>   (prefix));
    case Encoding::UTF8:
        return CaseInsensitiveCompare(GetDecoder<UTF8>   (str), GetDecoder<UTF8>   (prefix));
    case Encoding::UTF16LE:
        return CaseInsensitiveCompare(GetDecoder<UTF16LE>(str), GetDecoder<UTF16LE>(prefix));
    case Encoding::UTF16BE:
        return CaseInsensitiveCompare(GetDecoder<UTF16BE>(str), GetDecoder<UTF16BE>(prefix));
    case Encoding::UTF32LE:
        return CaseInsensitiveCompare(GetDecoder<UTF32LE>(str), GetDecoder<UTF32LE>(prefix));
    case Encoding::UTF32BE:
        return CaseInsensitiveCompare(GetDecoder<UTF32BE>(str), GetDecoder<UTF32BE>(prefix));
    case Encoding::GB18030:
        return CaseInsensitiveCompare(GetDecoder<GB18030>(str), GetDecoder<GB18030>(prefix));
    default: // should not enter, to please compiler
        Expects(false);
        return false;
    }
}


}
template<typename T1, typename T2>
[[nodiscard]] forceinline bool IsIBeginWith(const T1& str_, const T2& prefix_, const Encoding strchset = Encoding::ASCII)
{
    const auto str = ToStringView(str_);
    using Char = typename decltype(str)::value_type;
    const auto prefix = ToStringView(prefix_);
    using CharP = typename decltype(prefix)::value_type;
    static_assert(std::is_same_v<Char, CharP>, "str and prefix should be of the same char_type");
    return detail::IsIBeginWith_impl(str, prefix, strchset);
}


}


