#pragma once

#include "common/simd/SIMD.hpp"
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
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> InnerFrom(const T* __restrict const src, const size_t size) noexcept
    {
        if ((uint32_t)src[0] < 0x80)//1 byte
        {
            return { src[0], 1 };
        }
        IF_LIKELY(size >= 2 && (uint32_t)src[0] >= 0x81 && (uint32_t)src[0] <= 0xfe)
        {
            if (((uint32_t)src[1] >= 0x40 && (uint32_t)src[1] < 0x7f) || ((uint32_t)src[1] >= 0x80 && (uint32_t)src[1] < 0xff)) // 2 byte
            {
                const uint32_t tmp = ((uint32_t)src[0] << 8) | (uint32_t)src[1];
                const auto ch = LUT_GB18030_2B[tmp - LUT_GB18030_2B_BEGIN];
                return { ch, 2 };
            }
            IF_UNLIKELY(size >= 4
                && (uint32_t)src[1] >= 0x30 && (uint32_t)src[1] <= 0x39 
                && (uint32_t)src[2] >= 0x81 && (uint32_t)src[2] <= 0xfe 
                && (uint32_t)src[3] >= 0x30 && (uint32_t)src[3] <= 0x39) // 4 byte
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
    [[nodiscard]] forceinline static constexpr uint8_t InnerTo(const char32_t src, const size_t size, T* __restrict const dest) noexcept
    {
        IF_LIKELY (src < 0x80) // 1 byte
        {
            dest[0] = (char)(src & 0x7f);
            return 1;
        }
        else if (src < 0x10000) // 2 byte
        {
            const auto tmp = src - LUT_GB18030_R_BEGIN;
            const auto ch = LUT_GB18030_R[tmp];
            if (ch == 0) // invalid
                return 0;
            if (ch < 0xffff)
            {
                IF_UNLIKELY(size < 2)
                    return 0;
                dest[0] = T(ch & 0xff); dest[1] = T(ch >> 8);
                return 2;
            }
            else
            {
                IF_UNLIKELY(size < 4)
                    return 0;
                dest[0] = T(ch & 0xff); dest[1] = T(ch >> 8); dest[2] = T(ch >> 16); dest[3] = T(ch >> 24);
                return 4;
            }
        }
        else // 4 byte
        {
            IF_UNLIKELY(size < 4)
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
    }
public:
    using ElementType = char;
    static inline constexpr size_t MinBytes = 1;
    static inline constexpr size_t MaxOutputUnit = 4;
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



template<typename Dec, typename Enc, typename DChar, typename EChar>
forceinline void Transform(const DChar* __restrict srcPtr, size_t count, std::basic_string<EChar>& dst) noexcept
{
    static_assert(std::is_base_of_v<ConvertByteBase, Dec>, "Conv should be of ConvertByteBase");
    static_assert(std::is_base_of_v<ConvertByteBase, Enc>, "Conv should be of ConvertByteBase");
    constexpr bool DecUseCP = std::is_same_v<typename Dec::ElementType, DChar>;
    constexpr bool EncUseCP = std::is_same_v<typename Enc::ElementType, EChar>;
    static_assert(DecUseCP || sizeof(DChar) == 1);
    static_assert(EncUseCP || sizeof(EChar) == 1);

    EChar temp[EncUseCP ? Enc::MaxOutputUnit : Enc::MaxOutputUnit * sizeof(typename Enc::ElementType)] = { 0 };
    while (true)
    {
        char32_t cp = InvalidChar;
        while (count >= (DecUseCP ? 1u : Dec::MinBytes))
        {
            uint32_t cnt = 0;
            if constexpr (DecUseCP)
                std::tie(cp, cnt) = Dec::From(srcPtr, count);
            else
                std::tie(cp, cnt) = Dec::FromBytes(srcPtr, count);
            IF_LIKELY(cnt)
            {
                srcPtr += cnt;
                count -= cnt;
                CM_ASSUME(cp != InvalidChar);
                break;
            }
            else
            {
                CM_ASSUME(cp == InvalidChar);
                srcPtr += 1;
                count -= 1;
            }
        }
        IF_UNLIKELY(cp == InvalidChar)
            break;

        uint32_t cnt = 0;
        if constexpr (EncUseCP)
            cnt = Enc::To(cp, Enc::MaxOutputUnit, temp);
        else
            cnt = Enc::ToBytes(cp, sizeof(temp), std::launder(reinterpret_cast<uint8_t*>(temp)));
        IF_LIKELY(cnt)
        {
            dst.append(temp, cnt);
        }
    }
    return;
}

template<typename Dec, typename Enc, typename DChar, typename EChar>
forceinline void Transform2(const DChar* __restrict srcPtr, size_t count, std::basic_string<EChar>& dst) noexcept
{
    constexpr bool DecUseCP = std::is_same_v<typename Dec::ElementType, DChar>;
    if constexpr (DecUseCP)
    {
        Transform<Dec, Enc>(srcPtr, count, dst);
    }
    else
    {
        Transform<Dec, Enc>(std::launder(reinterpret_cast<const uint8_t*>(srcPtr)), count * sizeof(DChar), dst);
    }
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
forceinline void DirectCopyStr(const std::basic_string_view<Src> str, std::basic_string<Dst>& dst) noexcept
{
    const auto ptr = reinterpret_cast<const Dst*>(str.data());
    const auto cnt = str.size() * sizeof(Src) / sizeof(Dst);
    dst.assign(ptr, ptr + cnt);
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
        while (Source.size() >= (!UseCodeUnit ? Conv::MinBytes : 1u))
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
        while (Source.size() >= (!UseCodeUnit ? Conv::MinBytes : 1u))
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

template<typename Conv, typename Char>
[[nodiscard]] inline constexpr bool CaseInsensitiveCompare(const Char* str, size_t size1, const Char* prefix, size_t size2) noexcept
{
    static_assert(std::is_base_of_v<ConvertByteBase, Conv>, "Conv should be of ConvertByteBase");
    constexpr bool UseCP = std::is_same_v<typename Conv::ElementType, Char>;
    static_assert(UseCP || sizeof(Char) == 1);
    if (size1 < size2) return false;
    while (size2 >= (UseCP ? 1u : Conv::MinBytes))
    {
        char32_t cp1 = InvalidChar, cp2 = InvalidChar;
        uint32_t cnt1 = 0, cnt2 = 0;
        if constexpr (UseCP)
        {
            std::tie(cp1, cnt1) = Conv::From(str, size1);
            std::tie(cp2, cnt2) = Conv::From(prefix, size2);
        }
        else
        {
            std::tie(cp1, cnt1) = Conv::FromBytes(str, size1);
            std::tie(cp2, cnt2) = Conv::FromBytes(prefix, size2);
        }
        if (cnt1 != cnt2 || !cnt2)
            return false;
        CM_ASSUME(cp1 != InvalidChar);
        CM_ASSUME(cp2 != InvalidChar);
        if (cp1 != cp2)
        {
            const auto ch1 = (cp1 >= U'a' && cp1 <= U'z') ? (cp1 - U'a' + U'A') : cp1;
            const auto ch2 = (cp2 >= U'a' && cp2 <= U'z') ? (cp2 - U'a' + U'A') : cp2;
            if (ch1 != ch2) // not the same, fast return
                return false;
        }
        str += cnt2;
        prefix += cnt2;
        size2 -= cnt2;
    }
    return size2 == 0; // prefix finished then success
}
template<typename Conv, typename Char>
[[nodiscard]] inline constexpr bool CaseInsensitiveCompare2(const Char* str, size_t size1, const Char* prefix, size_t size2) noexcept
{
    constexpr bool UseCP = std::is_same_v<typename Conv::ElementType, Char>;
    if constexpr (UseCP)
    {
        return CaseInsensitiveCompare<Conv>(str, size1, prefix, size2);
    }
    else
    {
        return CaseInsensitiveCompare<Conv>(std::launder(reinterpret_cast<const uint8_t*>(str)), size1 * sizeof(Char),
            std::launder(reinterpret_cast<const uint8_t*>(prefix)), size2 * sizeof(Char));
    }
}


template<typename Char>
[[nodiscard]] inline bool IsIBeginWith_impl(const std::basic_string_view<Char> str, const std::basic_string_view<Char> prefix, const Encoding strchset)
{
    switch (strchset)
    {
    case Encoding::ASCII:   return CaseInsensitiveCompare2<UTF7   >(str.data(), str.size(), prefix.data(), prefix.size());
    case Encoding::UTF8:    return CaseInsensitiveCompare2<UTF8   >(str.data(), str.size(), prefix.data(), prefix.size());
    case Encoding::UTF16LE: return CaseInsensitiveCompare2<UTF16LE>(str.data(), str.size(), prefix.data(), prefix.size());
    case Encoding::UTF16BE: return CaseInsensitiveCompare2<UTF16BE>(str.data(), str.size(), prefix.data(), prefix.size());
    case Encoding::UTF32LE: return CaseInsensitiveCompare2<UTF32LE>(str.data(), str.size(), prefix.data(), prefix.size());
    case Encoding::UTF32BE: return CaseInsensitiveCompare2<UTF32BE>(str.data(), str.size(), prefix.data(), prefix.size());
    case Encoding::GB18030: return CaseInsensitiveCompare2<GB18030>(str.data(), str.size(), prefix.data(), prefix.size());
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


