#pragma once

#include "CommonRely.hpp"
#include "Exceptions.hpp"
#include "StrBase.hpp"
#include <cstring>
#include <algorithm>


namespace common::str
{


namespace detail
{
inline constexpr auto InvalidChar = static_cast<char32_t>(-1);
inline constexpr auto InvalidCharPair = std::pair<char32_t, uint32_t>{ InvalidChar, 0 };

//struct ConvertCPBase
//{
//    using ElementType = char;
//    static constexpr std::pair<char32_t, uint32_t> From(const ElementType* __restrict const, const size_t) noexcept
//    {
//        return InvalidCharPair;
//    }
//    static constexpr uint8_t To(const char32_t src, const size_t size, ElementType* __restrict const dest) noexcept
//    {
//        return 0;
//    }
//};
//struct ConvertByteBase
//{
//    static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, const size_t size) noexcept
//    {
//        return InvalidCharPair;
//    }
//    static constexpr uint8_t ToBytes(const char32_t src, const size_t size, uint8_t* __restrict const dest) noexcept
//    {
//        return 0;
//    }
//};

struct ConvertCPBase 
{ };
struct ConvertByteBase 
{ };

struct UTF7 : public ConvertCPBase, public ConvertByteBase
{
    using ElementType = char;
    static constexpr size_t MinBytes = 1;
    static constexpr size_t MaxBytes = 1;
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> From(const ElementType* __restrict const src, const size_t size) noexcept
    {
        if (size >= 1 && src[0] > 0)
            return { src[0], 1 };
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t To(const char32_t src, const size_t size, ElementType* __restrict const dest) noexcept
    {
        if (size >= 1 && src < 128)
        {
            dest[0] = char(src);
            return 1;
        }
        return 0;
    }
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, const size_t size) noexcept
    {
        if (size >= 1 && src[0] > 0)
            return { src[0], 1 };
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t ToBytes(const char32_t src, const size_t size, uint8_t* __restrict const dest) noexcept
    {
        if (size >= 1 && src < 128)
        {
            dest[0] = uint8_t(src);
            return 1;
        }
        return 0;
    }
};

struct UTF32 : public ConvertCPBase
{
    using ElementType = char32_t;
    static constexpr size_t MinBytes = 4;
    static constexpr size_t MaxBytes = 4;
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> From(const ElementType* __restrict const src, const size_t size) noexcept
    {
        if (size >= 1u && src[0] < 0x200000u)
        {
            return { src[0], 1 };
        }
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t To(const char32_t src, const size_t size, ElementType* __restrict const dest) noexcept
    {
        if (size >= 1u && src < 0x200000u)
        {
            dest[0] = src;
            return 1;
        }
        return 0;
    }
};
struct UTF32LE : public UTF32, public ConvertByteBase
{
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, const size_t size) noexcept
    {
        if (size >= 4)
        {
            const uint32_t ch = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
            if (ch < 0x200000u)
                return { ch, 4 };
        }
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t ToBytes(const char32_t src, const size_t size, uint8_t* __restrict const dest) noexcept
    {
        if (size >= 1u && src < 0x200000u)
        {
            dest[0] = static_cast<uint8_t>(src);       dest[1] = static_cast<uint8_t>(src >> 8);
            dest[2] = static_cast<uint8_t>(src >> 16); dest[3] = static_cast<uint8_t>(src >> 24);
            return 4;
        }
        return 0;
    }
};
struct UTF32BE : public UTF32, public ConvertByteBase
{
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, const size_t size) noexcept
    {
        if (size >= 4)
        {
            const uint32_t ch = src[3] | (src[2] << 8) | (src[1] << 16) | (src[0] << 24);
            if (ch < 0x200000u)
                return { ch, 4 };
        }
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t ToBytes(const char32_t src, const size_t size, uint8_t* __restrict const dest) noexcept
    {
        if (size >= 1u && src < 0x200000u)
        {
            dest[3] = static_cast<uint8_t>(src);       dest[2] = static_cast<uint8_t>(src >> 8);
            dest[1] = static_cast<uint8_t>(src >> 16); dest[0] = static_cast<uint8_t>(src >> 24);
            return 4;
        }
        return 0;
    }
};

struct UTF8 : public ConvertCPBase, public ConvertByteBase
{
private:
    template<typename T>
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> InnerFrom(const T* __restrict const src, const size_t size) noexcept
    {
        if (size == 0)
            return InvalidCharPair;
        if ((uint32_t)src[0] < 0x80u)//1 byte
            return { src[0], 1 };
        else if ((src[0] & 0xe0u) == 0xc0u)//2 byte
        {
            if (size >= 2 && (src[1] & 0xc0u) == 0x80u)
                return { ((src[0] & 0x1fu) << 6) | (src[1] & 0x3fu), 2 };
        }
        else if ((src[0] & 0xf0u) == 0xe0u)//3 byte
        {
            if (size >= 3 && ((src[1] & 0xc0u) == 0x80) && ((src[2] & 0xc0u) == 0x80u))
                return { ((src[0] & 0x0fu) << 12) | ((src[1] & 0x3fu) << 6) | (src[2] & 0x3fu), 3 };
        }
        else if ((src[0] & 0xf8u) == 0xf0u)//4 byte
        {
            if (size >= 4 && ((src[1] & 0xc0u) == 0x80u) && ((src[2] & 0xc0u) == 0x80u) && ((src[3] & 0xc0u) == 0x80u))
                return { ((src[0] & 0x0fu) << 18) | ((src[1] & 0x3fu) << 12) | ((src[2] & 0x3fu) << 6) | (src[3] & 0x3fu), 4 };
        }
        return InvalidCharPair;
    }
    template<typename T>
    [[nodiscard]] forceinline static constexpr uint8_t InnerTo(const char32_t src, const size_t size, T* __restrict const dest) noexcept
    {
        if (src < 0x80)//1 byte
        {
            if (size >= 1)
            {
                dest[0] = (T)(src & 0x7f);
                return 1;
            }
        }
        else if (src < 0x800)//2 byte
        {
            if (size >= 2)
            {
                dest[0] = T(0b11000000 | ((src >> 6) & 0x3f)), dest[1] = T(0x80 | (src & 0x3f));
                return 2;
            }
        }
        else if (src < 0x10000)//3 byte
        {
            if (size >= 3)
            {
                dest[0] = T(0b11100000 | (src >> 12)), dest[1] = T(0x80 | ((src >> 6) & 0x3f)), dest[2] = T(0x80 | (src & 0x3f));
                return 3;
            }
        }
        else if (src < 0x200000)//4 byte
        {
            if (size >= 4)
            {
                dest[0] = T(0b11110000 | (src >> 18)), dest[1] = T(0x80 | ((src >> 12) & 0x3f));
                dest[2] = T(0x80 | ((src >> 6) & 0x3f)), dest[3] = T(0x80 | (src & 0x3f));
                return 4;
            }
        }
        return 0;
    }
public:
    using ElementType = char;
    static constexpr size_t MinBytes = 1;
    static constexpr size_t MaxBytes = 4;
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

struct UTF16 : public ConvertCPBase
{
    using ElementType = char32_t;
    static constexpr size_t MinBytes = 2;
    static constexpr size_t MaxBytes = 2;
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> From(const char16_t* __restrict const src, const size_t size) noexcept
    {
        if (size == 0)
            return InvalidCharPair;
        if (src[0] < 0xd800 || src[0] >= 0xe000)//1 unit
            return { src[0], 1 };
        if (src[0] <= 0xdbff)//2 unit
        {
            if (size >= 2 && src[1] >= 0xdc00 && src[1] <= 0xdfff)
                return { (((src[0] & 0x3ff) << 10) | (src[1] & 0x3ff)) + 0x10000, 2 };
        }
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t To(const char32_t src, const size_t size, char16_t* __restrict const dest) noexcept
    {
        if (src < 0xd800)
        {
            dest[0] = (char16_t)src;
            return 1;
        }
        if (src < 0xe000)
            return 0;
        if (src < 0x10000)
        {
            dest[0] = (char16_t)src;
            return 1;
        }
        if (size >= 2 && src < 0x200000u)
        {
            const auto tmp = src - 0x10000;
            dest[0] = char16_t(0xd800 | (tmp >> 10)), dest[1] = char16_t(0xdc00 | (tmp & 0x3ff));
            return 2;
        }
        return 0;
    }
};
struct UTF16BE : public UTF16, public ConvertByteBase
{
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, const size_t size) noexcept
    {
        if (size < 2)
            return InvalidCharPair;
        if (src[0] < 0xd8 || src[0] >= 0xe0)//1 unit
            return { (src[0] << 8) | src[1], 2 };
        if (src[0] < 0xdc)//2 unit
        {
            if (size >= 2 && src[2] >= 0xdc && src[2] <= 0xdf)
                return { (((src[0] & 0x3) << 18) | (src[1] << 10) | ((src[2] & 0x3) << 8) | src[3]) + 0x10000, 4 };
        }
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t ToBytes(const char32_t src, const size_t size, uint8_t* __restrict const dest) noexcept
    {
        if (size < 2)
            return 0;
        if (src < 0xd800)
        {
            dest[0] = uint8_t(src >> 8);
            dest[1] = src & 0xff;
            return 2;
        }
        if (src < 0xe000)
            return 0;
        if (src < 0x10000)
        {
            dest[0] = uint8_t(src >> 8);
            dest[1] = src & 0xff;
            return 2;
        }
        if (size >= 4 && src < 0x200000u)
        {
            const auto tmp = src - 0x10000;
            dest[0] = uint8_t(0xd8 | (tmp >> 18)), dest[1] = ((tmp >> 10) & 0xff);
            dest[2] = 0xdc | ((tmp >> 8) & 0x3), dest[3] = tmp & 0xff;
            return 4;
        }
        return 0;
    }
};
struct UTF16LE : public UTF16, public ConvertByteBase
{
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, const size_t size) noexcept
    {
        if (size < 2)
            return InvalidCharPair;
        if (src[1] < 0xd8 || src[1] >= 0xe0)//1 unit
            return { (src[1] << 8) | src[0], 2 };
        if (src[1] < 0xdc)//2 unit
        {
            if (size >= 4 && src[3] >= 0xdc && src[3] <= 0xdf)
                return { (((src[1] & 0x3) << 18) | (src[0] << 10) | ((src[3] & 0x3) << 8) | src[2]) + 0x10000, 4 };
        }
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t ToBytes(const char32_t src, const size_t size, uint8_t* __restrict const dest) noexcept
    {
        if (size < 2)
            return 0;
        if (src < 0xd800)
        {
            dest[1] = uint8_t(src >> 8);
            dest[0] = src & 0xff;
            return 2;
        }
        if (src < 0xe000)
            return 0;
        if (src < 0x10000)
        {
            dest[1] = uint8_t(src >> 8);
            dest[0] = src & 0xff;
            return 2;
        }
        if (size >= 4 && src < 0x200000u)
        {
            const auto tmp = src - 0x10000;
            dest[1] = uint8_t(0xd8 | (tmp >> 18)), dest[0] = ((tmp >> 10) & 0xff);
            dest[3] = 0xdc | ((tmp >> 8) & 0x3), dest[2] = tmp & 0xff;
            return 4;
        }
        return 0;
    }
};

#include "LUT_gb18030.tab"

struct GB18030 : public ConvertCPBase, public ConvertByteBase
{
private:
    template<typename T>
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> InnerFrom(const T* __restrict const src, const size_t size) noexcept
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
    [[nodiscard]] forceinline static constexpr uint8_t InnerTo(const char32_t src, const size_t size, T* __restrict const dest) noexcept
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
    static constexpr size_t MinBytes = 1;
    static constexpr size_t MaxBytes = 4;
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


template<class From, typename CharT, typename Consumer>
inline void ForEachChar(const std::basic_string_view<CharT> str, Consumer consumer)
{
    static_assert(std::is_base_of_v<ConvertByteBase, From>, "Covertor [From] should support ConvertByteBase");
    const uint8_t* __restrict src = reinterpret_cast<const uint8_t*>(str.data());
    for (size_t bytes = str.size() * sizeof(CharT); bytes > 0;)
    {
        const auto[codepoint, len] = From::FromBytes(src, bytes);
        if (codepoint == InvalidChar)//fail
        {
            //move to next element
            bytes -= sizeof(CharT);
            src += sizeof(CharT);
            continue;
        }
        else
        {
            bytes -= len;
            src += len;
            if (consumer(codepoint))
                return;
        }
    }
}

template<class From1, class From2, typename CharT1, typename CharT2, typename Consumer>
inline void ForEachCharPair(const std::basic_string_view<CharT1> str1, const std::basic_string_view<CharT2> str2, Consumer consumer)
{
    static_assert(std::is_base_of_v<ConvertByteBase, From1>, "Covertor [From1] should support ConvertByteBase");
    static_assert(std::is_base_of_v<ConvertByteBase, From2>, "Covertor [From2] should support ConvertByteBase");
    const uint8_t* __restrict src1 = reinterpret_cast<const uint8_t*>(str1.data());
    const uint8_t* __restrict src2 = reinterpret_cast<const uint8_t*>(str2.data());
    for (size_t srcBytes1 = str1.size() * sizeof(CharT1), srcBytes2 = str2.data() * sizeof(CharT2); srcBytes1 > 0 && srcBytes2 > 0;)
    {
        const auto[cp1, len1] = From1::FromBytes(src1, srcBytes1);
        const auto[cp2, len2] = From2::FromBytes(src2, srcBytes2);
        if (cp1 == InvalidChar)//fail
        {
            //move to next element
            srcBytes1 -= sizeof(CharT1);
            src1 += sizeof(CharT1);
        }
        if (cp2 == InvalidChar)
        {
            //move to next element
            srcBytes2 -= sizeof(CharT2);
            src2 += sizeof(CharT2);
        }
        if(cp1 != InvalidChar && cp2 != InvalidChar)
        {
            srcBytes1 -= len1, srcBytes2 -= len2;
            src1 += len1, src2 += len2;
            if (consumer(cp1, cp2))
                return;
        }
    }
}

template<class From, class To, typename SrcType, typename DestType>
class CharsetConvertor
{
    static_assert(std::is_base_of_v<ConvertByteBase, From>, "Covertor [From] should support ConvertByteBase");
    static_assert(std::is_base_of_v<ConvertByteBase, To>  , "Covertor [To]   should support ConvertByteBase");
private:
    forceinline static constexpr char32_t Dummy(const char32_t in) noexcept { return in; }
    forceinline static constexpr size_t CalcDestCount(const size_t srcCount) noexcept
    {
        const size_t srcSize = sizeof(SrcType) * srcCount;
        return srcSize / sizeof(DestType);
    }
    template<size_t UnitSize>
    forceinline static constexpr std::pair<size_t, size_t> CalcUnitDestCount(const size_t srcCount) noexcept
    {
        Expects(UnitSize % sizeof(SrcType) == 0);
        Expects(UnitSize % sizeof(DestType) == 0);
        Expects(UnitSize >= sizeof(SrcType));
        Expects(UnitSize >= sizeof(DestType));
        const size_t srcSize = sizeof(SrcType) * srcCount;
        const size_t unitCount = srcSize / UnitSize;
        const size_t memSpace = unitCount * UnitSize;
        const size_t dstCount = memSpace / sizeof(DestType);
        return { unitCount, dstCount };
    }
public:
    template<typename TransformFunc>
    static std::basic_string<DestType> Transform(const std::basic_string_view<SrcType> str, TransformFunc transFunc)
    {
        const size_t maxCpCount = sizeof(SrcType) * str.size() / From::MinBytes;
        const size_t maxDstCount = 1 + (maxCpCount * To::MinBytes / sizeof(DestType));

        std::basic_string<DestType> ret(maxDstCount, 0); // reserve space fit for all codepoint
        const uint8_t* __restrict src = reinterpret_cast<const uint8_t*>(str.data());
              uint8_t* __restrict dst = reinterpret_cast<      uint8_t*>(ret.data());
              size_t srcAvalibale = str.size() * sizeof(SrcType);
        const size_t dstAvaliable = ret.size() * sizeof(DestType);
              size_t outBytes = 0;
        while (srcAvalibale > 0 && outBytes < dstAvaliable)
        {
            const auto [codepoint, len1] = From::FromBytes(src, srcAvalibale);
            const auto newCp = transFunc(codepoint);
            if (codepoint == InvalidChar)//fail
            {
                //move to next element
                srcAvalibale -= sizeof(SrcType);
                src += sizeof(SrcType);
                continue;
            }
            else
            {
                srcAvalibale -= len1;
                src += len1;
            }

            const auto len2 = To::ToBytes(newCp, dstAvaliable, dst);
            if (len2 == 0)//fail
            {
                ;//do nothing, skip
            }
            else
            {
                outBytes += len2;
                dst += len2;
            }
        }
        const auto outCount = (outBytes + (sizeof(DestType) - 1)) / sizeof(DestType);
        ret.resize(outCount);
        return ret;
    }
    static std::basic_string<DestType> Convert(const std::basic_string_view<SrcType> str)
    {
        if constexpr (std::is_same_v<From, To>)
        {
            if constexpr (sizeof(SrcType) == sizeof(DestType))
                return std::basic_string<DestType>(str.cbegin(), str.cend());
            else
            {
                const size_t count = CalcDestCount(str.size());
                std::basic_string<DestType> ret(count, DestType(0));
                memcpy_s(ret.data(), ret.size() * sizeof(DestType), str.data(), count * sizeof(DestType));
                return ret;
            }
        }
        /* specialize for UTF16's swap endian */
        else if constexpr (std::is_base_of_v<UTF16, From> && std::is_base_of_v<UTF16, To>)
        {
            //static_assert(sizeof(SrcType) <= 2 && sizeof(DestType) <= 2, "char size too big for UTF16");
            const auto [unitCount, dstCount] = CalcUnitDestCount<2>(str.size());
            std::basic_string<DestType> ret(dstCount, (DestType)0);
            const uint8_t * __restrict srcPtr = reinterpret_cast<const uint8_t*>(str.data());
            uint8_t * __restrict destPtr = reinterpret_cast<uint8_t*>(ret.data());
            for (size_t idx = 0; idx++ < unitCount; srcPtr += 2, destPtr += 2)
                destPtr[0] = srcPtr[1], destPtr[1] = srcPtr[0];
            return ret;
        }
        /* specialize for UTF32's swap endian */
        else if constexpr (std::is_base_of_v<UTF32, From> && std::is_base_of_v<UTF32, To>)
        {
            //static_assert(sizeof(SrcType) <= 4 && sizeof(DestType) <= 4, "char size too big for UTF32");
            const auto [unitCount, dstCount] = CalcUnitDestCount<4>(str.size());
            std::basic_string<DestType> ret(dstCount, (DestType)0);
            const uint8_t* __restrict srcPtr = reinterpret_cast<const uint8_t*>(str.data());
            uint8_t* __restrict destPtr = reinterpret_cast<uint8_t*>(ret.data());
            for (size_t idx = 0; idx++ < unitCount; srcPtr += 4, destPtr += 4)
                destPtr[0] = srcPtr[3], destPtr[1] = srcPtr[2], destPtr[2] = srcPtr[1], destPtr[3] = srcPtr[0];
            return ret;
        }
        else
            return Transform(str, Dummy);
    }
};


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


template<typename Char>
[[nodiscard]] forceinline constexpr bool CheckChsetType(const Charset inchset) noexcept
{
    switch (inchset)
    {
    case Charset::ASCII:
    case Charset::UTF8:
    case Charset::GB18030:
        return (sizeof(Char) <= 1) && (1 % sizeof(Char) == 0);
    case Charset::UTF16LE:
    case Charset::UTF16BE:
        return (sizeof(Char) <= 2) && (2 % sizeof(Char) == 0);
    case Charset::UTF32LE:
    case Charset::UTF32BE:
        return (sizeof(Char) <= 4) && (4 % sizeof(Char) == 0);
    default: // should not enter, unsupported charset
        return false;
    }
}

}


#define CHK_CHAR_SIZE_MOST(SIZE)    \
if constexpr(sizeof(Char) > SIZE)   \
    Expects(sizeof(Char) <= SIZE);  \
else                                \


template<typename T>
[[nodiscard]] forceinline std::string to_string(const T& str_, const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    const auto str = ToStringView(str_);
    using Char = typename decltype(str)::value_type;
    Expects(detail::CheckChsetType<Char>(inchset));
    if (outchset == inchset)//just copy
        return std::string(reinterpret_cast<const char*>(&str.front()), reinterpret_cast<const char*>(&str.back() + 1));
    switch (inchset)
    {
    case Charset::ASCII:
        switch (outchset)
        {
        case Charset::UTF8:
            return detail::CharsetConvertor<detail::UTF7, detail::UTF8,    Char, char>::Convert(str);
        case Charset::UTF16LE:
            return detail::CharsetConvertor<detail::UTF7, detail::UTF16LE, Char, char>::Convert(str);
        case Charset::UTF16BE:
            return detail::CharsetConvertor<detail::UTF7, detail::UTF16BE, Char, char>::Convert(str);
        case Charset::UTF32LE:
            return detail::CharsetConvertor<detail::UTF7, detail::UTF32LE, Char, char>::Convert(str);
        case Charset::UTF32BE:
            return detail::CharsetConvertor<detail::UTF7, detail::UTF32BE, Char, char>::Convert(str);
        case Charset::GB18030:
            return detail::CharsetConvertor<detail::UTF7, detail::GB18030, Char, char>::Convert(str);
        default: // should not enter, to please compiler
            Expects(false);
            return {};
        }
        break;
    case Charset::UTF8:
        switch (outchset)
        {
        case Charset::ASCII:
            return detail::CharsetConvertor<detail::UTF8, detail::UTF7,    Char, char>::Convert(str);
        case Charset::UTF16LE:
            return detail::CharsetConvertor<detail::UTF8, detail::UTF16LE, Char, char>::Convert(str);
        case Charset::UTF16BE:
            return detail::CharsetConvertor<detail::UTF8, detail::UTF16BE, Char, char>::Convert(str);
        case Charset::UTF32LE:
            return detail::CharsetConvertor<detail::UTF8, detail::UTF32LE, Char, char>::Convert(str);
        case Charset::UTF32BE:
            return detail::CharsetConvertor<detail::UTF8, detail::UTF32BE, Char, char>::Convert(str);
        case Charset::GB18030:
            return detail::CharsetConvertor<detail::UTF8, detail::GB18030, Char, char>::Convert(str);
        default: // should not enter, to please compiler
            Expects(false);
            return {};
        }
        break;
    case Charset::UTF16LE:
        switch (outchset)
        {
        case Charset::ASCII:
            return detail::CharsetConvertor<detail::UTF16LE, detail::UTF7,    Char, char>::Convert(str);
        case Charset::UTF8:
            return detail::CharsetConvertor<detail::UTF16LE, detail::UTF8,    Char, char>::Convert(str);
        case Charset::UTF16BE:
            return detail::CharsetConvertor<detail::UTF16LE, detail::UTF16BE, Char, char>::Convert(str);
        case Charset::UTF32LE:
            return detail::CharsetConvertor<detail::UTF16LE, detail::UTF32LE, Char, char>::Convert(str);
        case Charset::UTF32BE:
            return detail::CharsetConvertor<detail::UTF16LE, detail::UTF32BE, Char, char>::Convert(str);
        case Charset::GB18030:
            return detail::CharsetConvertor<detail::UTF16LE, detail::GB18030, Char, char>::Convert(str);
        default: // should not enter, to please compiler
            Expects(false);
            return {};
        }
        break;
    case Charset::UTF16BE:
        switch (outchset)
        {
        case Charset::ASCII:
            return detail::CharsetConvertor<detail::UTF16BE, detail::UTF7,    Char, char>::Convert(str);
        case Charset::UTF8:
            return detail::CharsetConvertor<detail::UTF16BE, detail::UTF8,    Char, char>::Convert(str);
        case Charset::UTF16BE:
            return detail::CharsetConvertor<detail::UTF16BE, detail::UTF16BE, Char, char>::Convert(str);
        case Charset::UTF32LE:
            return detail::CharsetConvertor<detail::UTF16BE, detail::UTF32LE, Char, char>::Convert(str);
        case Charset::UTF32BE:
            return detail::CharsetConvertor<detail::UTF16BE, detail::UTF32BE, Char, char>::Convert(str);
        case Charset::GB18030:
            return detail::CharsetConvertor<detail::UTF16BE, detail::GB18030, Char, char>::Convert(str);
        default: // should not enter, to please compiler
            Expects(false);
            return {};
        }
        break;
    case Charset::UTF32LE:
        switch (outchset)
        {
        case Charset::ASCII:
            return detail::CharsetConvertor<detail::UTF32LE, detail::UTF7,    Char, char>::Convert(str);
        case Charset::UTF8:
            return detail::CharsetConvertor<detail::UTF32LE, detail::UTF8,    Char, char>::Convert(str);
        case Charset::UTF16LE:
            return detail::CharsetConvertor<detail::UTF32LE, detail::UTF16LE, Char, char>::Convert(str);
        case Charset::UTF16BE:
            return detail::CharsetConvertor<detail::UTF32LE, detail::UTF16BE, Char, char>::Convert(str);
        case Charset::UTF32BE:
            return detail::CharsetConvertor<detail::UTF32LE, detail::UTF32BE, Char, char>::Convert(str);
        case Charset::GB18030:
            return detail::CharsetConvertor<detail::UTF32LE, detail::GB18030, Char, char>::Convert(str);
        default: // should not enter, to please compiler
            Expects(false);
            return {};
        }
        break;
    case Charset::UTF32BE:
        switch (outchset)
        {
        case Charset::ASCII:
            return detail::CharsetConvertor<detail::UTF32BE, detail::UTF7,    Char, char>::Convert(str);
        case Charset::UTF8:
            return detail::CharsetConvertor<detail::UTF32BE, detail::UTF8,    Char, char>::Convert(str);
        case Charset::UTF16LE:
            return detail::CharsetConvertor<detail::UTF32BE, detail::UTF16LE, Char, char>::Convert(str);
        case Charset::UTF16BE:
            return detail::CharsetConvertor<detail::UTF32BE, detail::UTF16BE, Char, char>::Convert(str);
        case Charset::UTF32LE:
            return detail::CharsetConvertor<detail::UTF32BE, detail::UTF32LE, Char, char>::Convert(str);
        case Charset::GB18030:
            return detail::CharsetConvertor<detail::UTF32BE, detail::GB18030, Char, char>::Convert(str);
        default: // should not enter, to please compiler
            Expects(false);
            return {};
        }
        break;
    case Charset::GB18030:
        switch (outchset)
        {
        case Charset::ASCII:
            return detail::CharsetConvertor<detail::GB18030, detail::UTF7,    Char, char>::Convert(str);
        case Charset::UTF8:
            return detail::CharsetConvertor<detail::GB18030, detail::UTF8,    Char, char>::Convert(str);
        case Charset::UTF16LE:
            return detail::CharsetConvertor<detail::GB18030, detail::UTF16LE, Char, char>::Convert(str);
        case Charset::UTF16BE:
            return detail::CharsetConvertor<detail::GB18030, detail::UTF16BE, Char, char>::Convert(str);
        case Charset::UTF32LE:
            return detail::CharsetConvertor<detail::GB18030, detail::UTF32LE, Char, char>::Convert(str);
        case Charset::UTF32BE:
            return detail::CharsetConvertor<detail::GB18030, detail::UTF32BE, Char, char>::Convert(str);
        default: // should not enter, to please compiler
            Expects(false);
            return {};
        }
        break;
    default:
        Expects(false);
        COMMON_THROW(BaseException, u"unknown charset", inchset);
    }
}


template<typename T>
[[nodiscard]] forceinline std::string to_u8string(const T& str_, const Charset inchset = Charset::ASCII)
{
    const auto str = ToStringView(str_);
    using Char = typename decltype(str)::value_type;
    Expects(detail::CheckChsetType<Char>(inchset));
    switch (inchset)
    {
    case Charset::ASCII:
        return detail::DirectAssign<char>(str);
    case Charset::UTF8:
        return detail::CharsetConvertor<detail::UTF8,    detail::UTF8, Char, char>::Convert(str);
    case Charset::UTF16LE:
        return detail::CharsetConvertor<detail::UTF16LE, detail::UTF8, Char, char>::Convert(str);
    case Charset::UTF16BE:
        return detail::CharsetConvertor<detail::UTF16BE, detail::UTF8, Char, char>::Convert(str);
    case Charset::UTF32LE:
        return detail::CharsetConvertor<detail::UTF32LE, detail::UTF8, Char, char>::Convert(str);
    case Charset::UTF32BE:
        return detail::CharsetConvertor<detail::UTF32BE, detail::UTF8, Char, char>::Convert(str);
    case Charset::GB18030:
        return detail::CharsetConvertor<detail::GB18030, detail::UTF8, Char, char>::Convert(str);
    default: // should not enter, to please compiler
        Expects(false);
        return {};
    }
}


template<typename T>
[[nodiscard]] forceinline std::u16string to_u16string(const T& str_, const Charset inchset = Charset::ASCII)
{
    const auto str = ToStringView(str_);
    using Char = typename decltype(str)::value_type;
    Expects(detail::CheckChsetType<Char>(inchset));
    switch (inchset)
    {
    case Charset::ASCII:
        return detail::DirectAssign<char16_t>(str);
    case Charset::UTF8:
        return detail::CharsetConvertor<detail::UTF8,    detail::UTF16LE, Char, char16_t>::Convert(str);
    case Charset::UTF16LE:
        return detail::CharsetConvertor<detail::UTF16LE, detail::UTF16LE, Char, char16_t>::Convert(str);
    case Charset::UTF16BE:
        return detail::CharsetConvertor<detail::UTF16BE, detail::UTF16LE, Char, char16_t>::Convert(str);
    case Charset::UTF32LE:
        return detail::CharsetConvertor<detail::UTF32LE, detail::UTF16LE, Char, char16_t>::Convert(str);
    case Charset::UTF32BE:
        return detail::CharsetConvertor<detail::UTF32BE, detail::UTF16LE, Char, char16_t>::Convert(str);
    case Charset::GB18030:
        return detail::CharsetConvertor<detail::GB18030, detail::UTF16LE, Char, char16_t>::Convert(str);
    default: // should not enter, to please compiler
        Expects(false);
        return {};
    }
}


template<typename T>
[[nodiscard]] forceinline std::u32string to_u32string(const T& str_, const Charset inchset = Charset::ASCII)
{
    const auto str = ToStringView(str_);
    using Char = typename decltype(str)::value_type;
    Expects(detail::CheckChsetType<Char>(inchset));
    switch (inchset)
    {
    case Charset::ASCII:
        return detail::DirectAssign<char32_t>(str);
    case Charset::UTF8:
        return detail::CharsetConvertor<detail::UTF8,    detail::UTF32LE, Char, char32_t>::Convert(str);
    case Charset::UTF16LE:
        return detail::CharsetConvertor<detail::UTF16LE, detail::UTF32LE, Char, char32_t>::Convert(str);
    case Charset::UTF16BE:
        return detail::CharsetConvertor<detail::UTF16BE, detail::UTF32LE, Char, char32_t>::Convert(str);
    case Charset::UTF32LE:
        return detail::CharsetConvertor<detail::UTF32LE, detail::UTF32LE, Char, char32_t>::Convert(str);
    case Charset::UTF32BE:
        return detail::CharsetConvertor<detail::UTF32BE, detail::UTF32LE, Char, char32_t>::Convert(str);
    case Charset::GB18030:
        return detail::CharsetConvertor<detail::GB18030, detail::UTF32LE, Char, char32_t>::Convert(str);
    default: // should not enter, to please compiler
        Expects(false);
        return {};
    }
}


template<typename T>
[[nodiscard]] forceinline std::wstring to_wstring(const T& str_, const Charset inchset = Charset::ASCII)
{
    static_assert(sizeof(wchar_t) == sizeof(char32_t) || sizeof(wchar_t) == sizeof(char16_t), "unrecognized wchar_t size");
    using To = std::conditional_t<sizeof(wchar_t) == sizeof(char32_t), detail::UTF32LE, detail::UTF16LE>;
    const auto str = ToStringView(str_);
    using Char = typename decltype(str)::value_type;
    Expects(detail::CheckChsetType<Char>(inchset));
    switch (inchset)
    {
    case Charset::ASCII:
        return detail::DirectAssign<wchar_t>(str);
    case Charset::UTF8:
        return detail::CharsetConvertor<detail::UTF8,    To, Char, wchar_t>::Convert(str);
    case Charset::UTF16LE:
        return detail::CharsetConvertor<detail::UTF16LE, To, Char, wchar_t>::Convert(str);
    case Charset::UTF16BE:
        return detail::CharsetConvertor<detail::UTF16BE, To, Char, wchar_t>::Convert(str);
    case Charset::UTF32LE:
        return detail::CharsetConvertor<detail::UTF32LE, To, Char, wchar_t>::Convert(str);
    case Charset::UTF32BE:
        return detail::CharsetConvertor<detail::UTF32BE, To, Char, wchar_t>::Convert(str);
    case Charset::GB18030:
        return detail::CharsetConvertor<detail::GB18030, To, Char, wchar_t>::Convert(str);
    default: // should not enter, to please compiler
        Expects(false);
        return {};
    }
}


template<typename T>
[[nodiscard]] forceinline std::enable_if_t<std::is_integral_v<T>, std::string> to_string(const T val)
{
    return std::to_string(val);
}

template<typename T>
[[nodiscard]] forceinline std::enable_if_t<std::is_integral_v<T>, std::wstring> to_wstring(const T val)
{
    return std::to_wstring(val);
}


template<typename T, typename Consumer>
inline void ForEachChar(const T& str_, const Consumer& consumer, const Charset inchset = Charset::ASCII)
{
    const auto str = ToStringView(str_);
    using Char = typename decltype(str)::value_type;
    Expects(detail::CheckChsetType<Char>(inchset));
    switch (inchset)
    {
    case Charset::ASCII:
        detail::ForEachChar<detail::UTF7,    Char, Consumer>(str, consumer);
    case Charset::UTF8:
        detail::ForEachChar<detail::UTF8,    Char, Consumer>(str, consumer);
    case Charset::UTF16LE:
        detail::ForEachChar<detail::UTF16LE, Char, Consumer>(str, consumer);
    case Charset::UTF16BE:
        detail::ForEachChar<detail::UTF16BE, Char, Consumer>(str, consumer);
    case Charset::UTF32LE:
        detail::ForEachChar<detail::UTF32LE, Char, Consumer>(str, consumer);
    case Charset::UTF32BE:
        detail::ForEachChar<detail::UTF32BE, Char, Consumer>(str, consumer);
    case Charset::GB18030:
        detail::ForEachChar<detail::GB18030, Char, Consumer>(str, consumer);
    default:
        Expects(false);
        COMMON_THROW(BaseException, u"unsupported charset");
    }
}


namespace detail
{
forceinline constexpr char32_t EngUpper(const char32_t in) noexcept
{
    if (in >= U'a' && in <= U'z')
        return in - U'a' + U'A';
    else 
        return in;
}
forceinline constexpr char32_t EngLower(const char32_t in) noexcept
{
    if (in >= U'A' && in <= U'Z')
        return in - U'A' + U'a';
    else 
        return in;
}

template<typename Char, typename T, typename TransFunc>
std::basic_string<Char> DirectTransform(const std::basic_string_view<T> src, TransFunc&& trans)
{
    static_assert(std::is_invocable_r_v<Char, TransFunc, T>, "TransFunc should accept SrcChar and return DstChar");
    std::basic_string<Char> ret;
    ret.reserve(src.size());
    std::transform(src.cbegin(), src.cend(), std::back_inserter(ret), trans);
    return ret;
}
}


template<typename T>
inline auto ToUpperEng(const T& str_, const Charset inchset = Charset::ASCII)
{
    const auto str = ToStringView(str_);
    using Char = typename decltype(str)::value_type;
    Expects(detail::CheckChsetType<Char>(inchset));
    switch (inchset)
    {
    case Charset::ASCII:
        // each element is determined
        return detail::DirectTransform<Char>(str, [](const Char ch) { return (ch >= 'a' && ch <= 'z') ? (Char)(ch - 'a' + 'A') : ch; });
    case Charset::UTF8:
        return detail::CharsetConvertor<detail::UTF8,    detail::UTF8,    Char, Char>::Transform(str, detail::EngUpper);
    case Charset::UTF16LE:
        return detail::CharsetConvertor<detail::UTF16LE, detail::UTF16LE, Char, Char>::Transform(str, detail::EngUpper);
    case Charset::UTF16BE:
        return detail::CharsetConvertor<detail::UTF16BE, detail::UTF16BE, Char, Char>::Transform(str, detail::EngUpper);
    case Charset::UTF32LE:
        if constexpr (std::is_same_v<Char, char32_t>)
            // each element is determined
            return detail::DirectTransform<Char>(str, detail::EngUpper);
        else
            return detail::CharsetConvertor<detail::UTF32LE, detail::UTF32LE, Char, Char>::Transform(str, detail::EngUpper);
    case Charset::UTF32BE:
        return detail::CharsetConvertor<detail::UTF32BE, detail::UTF32BE, Char, Char>::Transform(str, detail::EngUpper);
    case Charset::GB18030:
        return detail::CharsetConvertor<detail::GB18030, detail::GB18030, Char, Char>::Transform(str, detail::EngUpper);
    default:
        Expects(false);
        COMMON_THROW(BaseException, u"unsupported charset");
    }
}


template<typename T>
inline auto ToLowerEng(const T& str_, const Charset inchset = Charset::ASCII)
{
    const auto str = ToStringView(str_);
    using Char = typename decltype(str)::value_type;
    Expects(detail::CheckChsetType<Char>(inchset));
    switch (inchset)
    {
    case Charset::ASCII:
        // each element is determined
        return detail::DirectTransform<Char>(str, [](const Char ch) { return (ch >= 'A' && ch <= 'Z') ? (Char)(ch - 'A' + 'a') : ch; });
    case Charset::UTF8:
        return detail::CharsetConvertor<detail::UTF8,    detail::UTF8,    Char, Char>::Transform(str, detail::EngLower);
    case Charset::UTF16LE:
        return detail::CharsetConvertor<detail::UTF16LE, detail::UTF16LE, Char, Char>::Transform(str, detail::EngLower);
    case Charset::UTF16BE:
        return detail::CharsetConvertor<detail::UTF16BE, detail::UTF16BE, Char, Char>::Transform(str, detail::EngLower);
    case Charset::UTF32LE:
        if constexpr (std::is_same_v<Char, char32_t>)
            // each element is determined
            return detail::DirectTransform<Char>(str, detail::EngUpper);
        else
            return detail::CharsetConvertor<detail::UTF32LE, detail::UTF32LE, Char, Char>::Transform(str, detail::EngLower);
    case Charset::UTF32BE:
        return detail::CharsetConvertor<detail::UTF32BE, detail::UTF32BE, Char, Char>::Transform(str, detail::EngLower);
    case Charset::GB18030:
        return detail::CharsetConvertor<detail::GB18030, detail::GB18030, Char, Char>::Transform(str, detail::EngLower);
    default:
        Expects(false);
        COMMON_THROW(BaseException, u"unsupported charset");
    }
}



namespace detail
{
template<class From1, class From2, typename CharT1, typename CharT2>
inline bool CaseInsensitiveCompare(const std::basic_string_view<CharT1> str, const std::basic_string_view<CharT2> prefix)
{
    bool isEqual = true;
    ForEachCharPair<From1, From2>(str, prefix, [&](const char32_t ch1, const char32_t ch2) 
    {
        const auto ch1New = (ch1 >= U'a' && ch1 <= U'z') ? (ch1 - U'a' + U'A') : ch1;
        const auto ch2New = (ch2 >= U'a' && ch2 <= U'z') ? (ch2 - U'a' + U'A') : ch2;
        isEqual = (ch1New == ch2New);
        return !isEqual;
    });
    return isEqual;
}
}
template<typename T1, typename T2>
inline bool IsIBeginWith(const T1& str_, const T2& prefix_, const Charset strchset = Charset::ASCII)
{
    const auto str = ToStringView(str_);
    using Char = typename decltype(str)::value_type;
    const auto prefix = ToStringView(prefix_);
    using CharP = typename decltype(prefix)::value_type;
    static_assert(std::is_same_v<Char, CharP>, "str and prefix should be of the same char_type");
    if (prefix.size() > str.size())
        return false;
    Expects(detail::CheckChsetType<Char>(strchset));
    switch (strchset)
    {
    case Charset::ASCII:
        return detail::CaseInsensitiveCompare<detail::UTF7,    detail::UTF7,    Char, Char>(str, prefix);
    case Charset::UTF8:
        return detail::CaseInsensitiveCompare<detail::UTF8,    detail::UTF8,    Char, Char>(str, prefix);
    case Charset::UTF16LE:
        return detail::CaseInsensitiveCompare<detail::UTF16LE, detail::UTF16LE, Char, Char>(str, prefix);
    case Charset::UTF16BE:
        return detail::CaseInsensitiveCompare<detail::UTF16BE, detail::UTF16BE, Char, Char>(str, prefix);
    case Charset::UTF32LE:
        return detail::CaseInsensitiveCompare<detail::UTF32LE, detail::UTF32LE, Char, Char>(str, prefix);
    case Charset::UTF32BE:
        return detail::CaseInsensitiveCompare<detail::UTF32BE, detail::UTF32BE, Char, Char>(str, prefix);
    case Charset::GB18030:
        return detail::CaseInsensitiveCompare<detail::GB18030, detail::GB18030, Char, Char>(str, prefix);
    default:
        Expects(false);
        COMMON_THROW(BaseException, u"unsupported charset");
    }
}


}


