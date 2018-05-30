#pragma once

#include "CommonRely.hpp"
#include "CommonMacro.hpp"
#include "Exceptions.hpp"
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <type_traits>
#include <algorithm>


namespace common::str
{


enum class Charset { ASCII, UTF7 = ASCII, GB18030, UTF8, UTF16LE, UTF16BE, UTF32 };

inline Charset toCharset(const std::string& chset)
{
    switch (hash_(chset))
    {
    case "GB18030"_hash:
        return Charset::GB18030;
    case "UTF-8"_hash:
        return Charset::UTF8;
    case "UTF-16LE"_hash:
        return Charset::UTF16LE;
    case "UTF-16BE"_hash:
        return Charset::UTF16BE;
    case "error"_hash:
        return Charset::ASCII;
    default:
        return Charset::ASCII;
    }
}

inline std::wstring getCharsetWName(const Charset chset)
{
    switch (chset)
    {
    case Charset::ASCII:
        return L"ASCII";
    case Charset::GB18030:
        return L"GB18030";
    case Charset::UTF8:
        return L"UTF-8";
    case Charset::UTF16BE:
        return L"UTF-16BE";
    case Charset::UTF16LE:
        return L"UTF-16LE";
    case Charset::UTF32:
        return L"UTF-32";
    default:
        return L"error";
    }
}


namespace detail
{

struct ConvertBase//just for template
{
    static char32_t From(const char* __restrict const, const size_t, uint8_t&)
    {
        return (char32_t)-1;
    }
    static char32_t FromBytes(const char* __restrict const src, const size_t size, [[maybe_unused]] const bool isLE, uint8_t& len)
    {
        return From(src, size, len);
    }
    static uint8_t To(const char32_t, const size_t, char* __restrict const)
    {
        return 0;
    }
    static uint8_t ToBytes(const char32_t src, const size_t size, [[maybe_unused]] const bool isLE, char* __restrict const dest)
    {
        return To(src, size, dest);
    }
};

struct UTF7
{
    static char32_t From(const char* __restrict const src, const size_t size, uint8_t& len)
    {
        if (size >= 1 && src[0] > 0)
        {
            len = 1;
            return src[0];
        }
        return (char32_t)-1;
    }
    static char32_t FromBytes(const char* __restrict const src, const size_t size, [[maybe_unused]] const bool isLE, uint8_t& len)
    {
        return From(src, size, len);
    }
    static uint8_t To(const char32_t src, const size_t size, char* __restrict const dest)
    {
        if (size >= 1 && src < 128)
        {
            dest[0] = char(src);
            return 1;
        }
        return 0;
    }
    static uint8_t ToBytes(const char32_t src, const size_t size, [[maybe_unused]] const bool isLE, char* __restrict const dest)
    {
        return To(src, size, dest);
    }
};

struct UTF32
{
    static char32_t From(const char32_t* __restrict const src, const size_t size, uint8_t& len)
    {
        if (size >= 1 && src[0] < 0x200000)
        {
            len = 1;
            return src[0];
        }
        return (char32_t)-1;
    }
    static char32_t FromBytes(const char* __restrict const src, const size_t size, [[maybe_unused]] const bool isLE, uint8_t& len)
    {
        uint8_t len4;
        const auto ret = From(reinterpret_cast<const char32_t*>(src), size / 4, len4);
        if (ret != (char32_t)-1)
            len = len4 * 4;
        return ret;
    }
    static uint8_t To(const char32_t src, const size_t size, char32_t* __restrict const dest)
    {
        if (size >= 1 && src < 0x200000)
        {
            dest[0] = src;
            return 1;
        }
        return 0;
    }
    static uint8_t ToBytes(const char32_t src, const size_t size, [[maybe_unused]] const bool isLE, char* __restrict const dest)
    {
        return 4 * To(src, size / 4, reinterpret_cast<char32_t*>(dest));
    }
};

struct UTF8
{
    static char32_t From(const char* __restrict const src0, const size_t size, uint8_t& len)
    {
        const uint8_t* __restrict const src = reinterpret_cast<const uint8_t*>(src0);
        if (size == 0)
            return (char32_t)-1;
        if (src[0] < 0x80)//1 byte
        {
            len = 1;
            return src[0];
        }
        if ((src[0] & 0xe0) == 0xc0)//2 byte
        {
            if (size >= 2 && (src[1] & 0xc0) == 0x80)
            {
                len = 2;
                return ((src[0] & 0x1f) << 6) | (src[1] & 0x3f);
            }
            return (char32_t)-1;
        }
        if ((src[0] & 0xf0) == 0xe0)//3 byte
        {
            if (size >= 3 && ((src[1] & 0xc0) == 0x80) && ((src[2] & 0xc0) == 0x80))
            {
                len = 3;
                return ((src[0] & 0x0f) << 12) | ((src[1] & 0x3f) << 6) | (src[2] & 0x3f);
            }
            return (char32_t)-1;
        }
        if ((src[0] & 0xf8) == 0xf0)//4 byte
        {
            if (size >= 4 && ((src[1] & 0xc0) == 0x80) && ((src[2] & 0xc0) == 0x80) && ((src[3] & 0xc0) == 0x80))
            {
                len = 4;
                return ((src[0] & 0x0f) << 18) | ((src[1] & 0x3f) << 12) | ((src[2] & 0x3f) << 6) | (src[3] & 0x3f);
            }
            return (char32_t)-1;
        }
        return (char32_t)-1;
    }
    static char32_t FromBytes(const char* __restrict const src, const size_t size, [[maybe_unused]] const bool isLE, uint8_t& len)
    {
        return From(src, size, len);
    }

    static uint8_t To(const char32_t src, const size_t size, char* __restrict const dest)
    {
        if (src < 0x80)//1 byte
        {
            if (size < 1)
                return 0;
            dest[0] = (char)(src & 0x7f);
            return 1;
        }
        if (src < 0x800)//2 byte
        {
            if (size < 2)
                return 0;
            dest[0] = char(0b11000000 | ((src >> 6) & 0x3f)), dest[1] = char(0x80 | (src & 0x3f));
            return 2;
        }
        if (src < 0x10000)//3 byte
        {
            if (size < 3)
                return 0;
            dest[0] = char(0b11100000 | (src >> 12)), dest[1] = char(0x80 | ((src >> 6) & 0x3f)), dest[2] = char(0x80 | (src & 0x3f));
            return 3;
        }
        if (src < 0x200000)//4 byte
        {
            if (size < 4)
                return 0;
            dest[0] = char(0b11110000 | (src >> 18)), dest[1] = char(0x80 | ((src >> 12) & 0x3f)), dest[2] = char(0x80 | ((src >> 6) & 0x3f)), dest[3] = char(0x80 | (src & 0x3f));
            return 4;
        }
        return 0;
    }
    static uint8_t ToBytes(const char32_t src, const size_t size, [[maybe_unused]] const bool isLE, char* __restrict const dest)
    {
        return To(src, size, dest);
    }
};

struct UTF16
{
private:
    static char32_t FromBE(const char16_t* __restrict const src0, const size_t size, uint8_t& len)
    {
        const uint8_t* __restrict const src = reinterpret_cast<const uint8_t*>(src0);
        if (size == 0)
            return (char32_t)-1;
        if (src[0] < 0xd8 || src[0] >= 0xe0)//1 unit
        {
            len = 1;
            return (src[0] << 8) | src[1];
        }
        if (src[0] < 0xdc)//2 unit
        {
            if (size >= 2 && src[2] >= 0xdc)
            {
                len = 2;
                return (((src[0] & 0x3) << 18) | (src[1] << 10) | ((src[2] & 0x3) << 8) | src[3]) + 0x10000;
            }
            return (char32_t)-1;
        }
        return (char32_t)-1;
    }
    static uint8_t ToBE(const char32_t src, const size_t size, char16_t* __restrict const dest0)
    {
        uint8_t* __restrict const dest = reinterpret_cast<uint8_t*>(dest0);
        if (size < 1)
            return 0;
        if (src < 0xd800)
        {
            dest[0] = uint8_t(src >> 8);
            dest[1] = src & 0xff;
            return 1;
        }
        if (src < 0xe000)
            return 0;
        if (src < 0x10000)
        {
            dest[0] = uint8_t(src >> 8);
            dest[1] = src & 0xff;
            return 1;
        }
        if (size >= 2 && src < 0x200000)
        {
            const auto tmp = src - 0x10000;
            dest[0] = uint8_t(0xd8 | (tmp >> 18)), dest[1] = ((tmp >> 10) & 0xff);
            dest[2] = 0xdc | ((tmp >> 8) & 0xff), dest[3] = tmp & 0xff;
            return 2;
        }
        return 0;
    }
public:
    static char32_t From(const char16_t* __restrict const src, const size_t size, uint8_t& len)
    {
        if (size == 0)
            return (char32_t)-1;
        if (src[0] < 0xd800 || src[0] >= 0xe000)//1 unit
        {
            len = 1;
            return src[0];
        }
        if (src[0] <= 0xdbff)//2 unit
        {
            if (size >= 2 && src[1] >= 0xdc00 && src[1] <= 0xdfff)
            {
                len = 2;
                return (((src[0] & 0x3ff) << 10) | (src[1] & 0x3ff)) + 0x10000;
            }
            return (char32_t)-1;
        }
        return (char32_t)-1;
    }
    static char32_t FromBytes(const char* __restrict const src, const size_t size, const bool isLE, uint8_t& len)
    {
        uint8_t len2 = 0;
        const auto ch = isLE ? From(reinterpret_cast<const char16_t*>(src), size / 2, len2)
            : FromBE(reinterpret_cast<const char16_t*>(src), size / 2, len2);
        if (ch != (char32_t)-1)
            len = len2 * 2;
        return ch;
    }
    static uint8_t To(const char32_t src, const size_t size, char16_t* __restrict const dest)
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
        if (size >= 2 && src < 0x200000)
        {
            const auto tmp = src - 0x10000;
            dest[0] = char16_t(0xd800 | (tmp >> 10)), dest[1] = char16_t(0xdc00 | (tmp & 0x3ff));
            return 2;
        }
        return 0;
    }
    static uint8_t ToBytes(const char32_t src, const size_t size, const bool isLE, char* __restrict const dest)
    {
        const auto len = isLE ? To(src, size / 2, reinterpret_cast<char16_t*>(dest))
            : ToBE(src, size / 2, reinterpret_cast<char16_t*>(dest));
        return len * 2;
    }
};

#include "LUT_gb18030.tab"
struct GB18030
{
    static char32_t From(const char* __restrict const src0, const size_t size, uint8_t& len)
    {
        const uint8_t* __restrict const src = reinterpret_cast<const uint8_t*>(src0);
        if (size == 0)
            return (char32_t)-1;
        if (src[0] < 0x80)//1 byte
        {
            len = 1;
            return src[0];
        }
        if (src[0] >= 0x81 && src[0] <= 0xfe && size >= 2)
        {
            if ((src[1] >= 0x40 && src[1] < 0x7f) || (src[1] >= 0x80 && src[1] < 0xff))//2 byte
            {
                const uint32_t tmp = (src[0] << 8) | src[1];
                const auto ch = LUT_GB18030_2B[tmp - LUT_GB18030_2B_BEGIN];
                if (ch != 0)
                    len = 2;
                return (char32_t)ch;
            }
            if (size >= 4 && src[1] >= 0x30 && src[1] <= 0x39 && src[2] >= 0x81 && src[2] <= 0xfe && src[3] >= 0x30 && src[3] <= 0x39)//4 byte
            {
                const uint32_t tmp = ((src[3] - 0x30) + (src[2] - 0x81) * 10 + (src[1] - 0x30)* (0xff - 0x81) * 10 + (src[0] - 0x81) * (0xff - 0x81) * 10 * 10);
                const auto ch = tmp < LUT_GB18030_4B_SIZE ? LUT_GB18030_4B[tmp] : 0x10000 + (tmp - LUT_GB18030_4B_SIZE);
                if (ch != 0)
                    len = 4;
                return (char32_t)ch;
            }
        }
        return (char32_t)-1;
    }
    static char32_t FromBytes(const char* __restrict const src, const size_t size, [[maybe_unused]] const bool isLE, uint8_t& len)
    {
        return From(src, size, len);
    }
    static uint8_t To(const char32_t src, const size_t size, char* __restrict const dest)
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
                *reinterpret_cast<char16_t*>(dest) = (char16_t)ch;
                return 2;
            }
            else
            {
                if (size < 4)
                    return 0;
                *reinterpret_cast<char32_t*>(dest) = ch;
                return 4;
            }
        }
        if (src < 0x200000)//4 byte
        {
            if (size < 4)
                return 0;
            auto tmp = src - 0x10000 + LUT_GB18030_4B_SIZE;
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
    static uint8_t ToBytes(const char32_t src, const size_t size, [[maybe_unused]] const bool isLE, char* __restrict const dest)
    {
        return To(src, size, dest);
    }
};


template<class From, typename CharT, typename Consumer>
inline void ForEachChar(const CharT* __restrict const str, const size_t size, const bool fromLE, Consumer consumer)
{
    const char* __restrict src = reinterpret_cast<const char*>(str);
    for (size_t srcBytes = size * sizeof(CharT); srcBytes > 0;)
    {
        uint8_t len = 0;
        const char32_t codepoint = From::FromBytes(src, srcBytes, fromLE, len);
        if (codepoint == (char32_t)-1)//fail
        {
            //move to next element
            srcBytes -= sizeof(CharT);
            src += sizeof(CharT);
            continue;
        }
        else
        {
            srcBytes -= len;
            src += len;
        }
        if (consumer(codepoint))
            return;
    }
}

template<class From1, class From2, typename CharT1, typename CharT2, typename Consumer>
inline void ForEachCharPair(const CharT1* __restrict const str1, const size_t size1, const bool fromLE1, const CharT2* __restrict const str2, const size_t size2, const bool fromLE2, Consumer consumer)
{
    const char* __restrict src1 = reinterpret_cast<const char*>(str1);
    const char* __restrict src2 = reinterpret_cast<const char*>(str2);
    for (size_t srcBytes1 = size1 * sizeof(CharT1), srcBytes2 = size2 * sizeof(CharT2); srcBytes1 > 0 && srcBytes2 > 0;)
    {
        uint8_t len1 = 0, len2 = 0;
        const char32_t cp1 = From1::FromBytes(src1, srcBytes1, fromLE1, len1);
        const char32_t cp2 = From2::FromBytes(src2, srcBytes2, fromLE2, len2);
        if (cp1 == (char32_t)-1)//fail
        {
            //move to next element
            srcBytes1 -= sizeof(CharT1);
            src1 += sizeof(CharT1);
        }
        if (cp2 == (char32_t)-1)
        {
            //move to next element
            srcBytes2 -= sizeof(CharT2);
            src2 += sizeof(CharT2);
        }
        if(cp1 != -1 && cp2 != -2)
        {
            srcBytes1 -= len1, srcBytes2 -= len2;
            src1 += len1, src2 += len2;
        }
        if (consumer(cp1, cp2))
            return;
    }
}

template<class From, class To, typename SrcType, typename DestType>
class CharsetConvertor
{
private:
    static char32_t Dummy(const char32_t in) { return in; }
public:
    template<typename TransformFunc>
    static std::basic_string<DestType> Transform(const SrcType* __restrict const str, const size_t size, const bool fromLE, const bool toLE, TransformFunc transFunc)
    {
        std::basic_string<DestType> ret((size * 4) + 3 / sizeof(DestType), 0);//reserve space fit for all codepoint
        const char* __restrict src = reinterpret_cast<const char*>(str);
        size_t cacheidx = 0;
        for (size_t srcBytes = size * sizeof(SrcType); srcBytes > 0;)
        {
            uint8_t len = 0;
            const char32_t codepoint = transFunc(From::FromBytes(src, srcBytes, fromLE, len));
            if (codepoint == (char32_t)-1)//fail
            {
                //move to next element
                srcBytes -= sizeof(SrcType);
                src += sizeof(SrcType);
                continue;
            }
            else
            {
                srcBytes -= len;
                src += len;
            }
            char* __restrict dest = &((char*)ret.data())[cacheidx];
            len = To::ToBytes(codepoint, sizeof(char32_t), toLE, dest);
            if (len == 0)//fail
            {
                ;//do nothing, skip
            }
            else
            {
                cacheidx += len;
            }
        }
        const auto destSize = (cacheidx + sizeof(DestType) - 1) / sizeof(DestType);
        ret.resize(destSize);
        return ret;
    }
    static std::basic_string<DestType> Convert(const SrcType* __restrict const str, const size_t size, const bool fromLE, const bool toLE)
    {
        return Transform(str, size, fromLE, toLE, Dummy);
    }
};

/* partial specialization for UTF16 */
template<typename SrcType, typename DestType>
class CharsetConvertor<UTF16, UTF16, SrcType, DestType>
{
public:
    template<typename TransformFunc>
    static std::basic_string<DestType> Transform(const SrcType* __restrict const str, const size_t size, const bool fromLE, const bool toLE, TransformFunc transFunc)
    {
        std::basic_string<DestType> ret((size * 4) + 3 / sizeof(DestType), 0);//reserve space fit for all codepoint
        const char* __restrict src = reinterpret_cast<const char*>(str);
        size_t cacheidx = 0;
        for (size_t srcBytes = size * sizeof(SrcType); srcBytes > 0;)
        {
            uint8_t len = 0;
            const char32_t codepoint = transFunc(UTF16::FromBytes(src, srcBytes, fromLE, len));
            if (codepoint == (char32_t)-1)//fail
            {
                //move to next element
                srcBytes -= sizeof(SrcType);
                src += sizeof(SrcType);
                continue;
            }
            else
            {
                srcBytes -= len;
                src += len;
            }
            char* __restrict dest = &((char*)ret.data())[cacheidx];
            len = UTF16::ToBytes(codepoint, sizeof(char32_t), toLE, dest);
            if (len == 0)//fail
            {
                ;//do nothing, skip
            }
            else
            {
                cacheidx += len;
            }
        }
        const auto destSize = (cacheidx + sizeof(DestType) - 1) / sizeof(DestType);
        ret.resize(destSize);
        return ret;
    }
    /* partial specialization for UTF16 */
    static std::basic_string<DestType> Convert(const SrcType* __restrict const str, const size_t size, const bool fromLE, const bool toLE)
    {
        static_assert(sizeof(SrcType) <= 2, "source type should be at most 2 byte");
        static_assert(sizeof(DestType) <= 2, "dest type should be at most 2 byte");
        const auto destcount = (size * sizeof(SrcType) + sizeof(DestType) - 1) / sizeof(DestType);
        const auto destcount2 = ((destcount * sizeof(DestType) + 1) / 2) * 2 / sizeof(DestType);
        std::basic_string<DestType> ret(destcount2, (DestType)0);
        const uint8_t * __restrict srcPtr = reinterpret_cast<const uint8_t*>(str);
        uint8_t * __restrict destPtr = reinterpret_cast<uint8_t*>(ret.data());
        if (fromLE == toLE)
        {
            memcpy_s(destPtr, destcount2 * sizeof(DestType), srcPtr, size * sizeof(SrcType));
        }
        else
        {
            for (auto procCnt = size * sizeof(SrcType) / 2; procCnt--; srcPtr += 2, destPtr += 2)
                destPtr[0] = srcPtr[1], destPtr[1] = srcPtr[0];
        }
        return ret;
    }
};

}

#define CONCAT_TYPE_SIZE_STR(TYPE, SIZE) STRINGIZE(TYPE) " string should has type of at most size[" STRINGIZE(SIZE) "]"
#define CHK_CHAR_SIZE_MOST(TYPE, SIZE) \
    if constexpr(sizeof(Char) > SIZE) \
        COMMON_THROW(BaseException, WIDEN(CONCAT_TYPE_SIZE_STR(TYPE, SIZE))); \
    else \
    {
#define CHK_CHAR_SIZE_END \
    }

template<typename Char>
inline std::string to_string(const Char *str, const size_t size, const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    if (outchset == inchset)//just copy
        return std::string(reinterpret_cast<const char*>(str), reinterpret_cast<const char*>(str + size));
    switch (inchset)
    {
    case Charset::ASCII:
        CHK_CHAR_SIZE_MOST(ASCII, 1)
            switch (outchset)
            {
            case Charset::UTF8:
                return detail::CharsetConvertor<detail::UTF7, detail::UTF8, Char, char>::Convert(str, size, true, true);
            case Charset::UTF16LE:
                return detail::CharsetConvertor<detail::UTF7, detail::UTF16, Char, char>::Convert(str, size, true, true);
            case Charset::UTF16BE:
                return detail::CharsetConvertor<detail::UTF7, detail::UTF16, Char, char>::Convert(str, size, true, false);
            case Charset::UTF32:
                return detail::CharsetConvertor<detail::UTF7, detail::UTF32, Char, char>::Convert(str, size, true, true);
            case Charset::GB18030:
                return detail::CharsetConvertor<detail::UTF7, detail::GB18030, Char, char>::Convert(str, size, true, true);
            default: // should not enter, to please compiler
                return {};
            }
        CHK_CHAR_SIZE_END
        break;
    case Charset::UTF8:
        CHK_CHAR_SIZE_MOST(UTF8, 1)
            switch (outchset)
            {
            case Charset::ASCII:
                return detail::CharsetConvertor<detail::UTF8, detail::UTF7, Char, char>::Convert(str, size, true, true);
            case Charset::UTF16LE:
                return detail::CharsetConvertor<detail::UTF8, detail::UTF16, Char, char>::Convert(str, size, true, true);
            case Charset::UTF16BE:
                return detail::CharsetConvertor<detail::UTF8, detail::UTF16, Char, char>::Convert(str, size, true, false);
            case Charset::UTF32:
                return detail::CharsetConvertor<detail::UTF8, detail::UTF32, Char, char>::Convert(str, size, true, true);
            case Charset::GB18030:
                return detail::CharsetConvertor<detail::UTF8, detail::GB18030, Char, char>::Convert(str, size, true, true);
            default: // should not enter, to please compiler
                return {};
            }
        CHK_CHAR_SIZE_END
        break;
    case Charset::UTF16LE:
        CHK_CHAR_SIZE_MOST(UTF16LE, 2)
            switch (outchset)
            {
            case Charset::ASCII:
                return detail::CharsetConvertor<detail::UTF16, detail::UTF7, Char, char>::Convert(str, size, true, true);
            case Charset::UTF8:
                return detail::CharsetConvertor<detail::UTF16, detail::UTF8, Char, char>::Convert(str, size, true, true);
            case Charset::UTF16BE:
                return detail::CharsetConvertor<detail::UTF16, detail::UTF16, Char, char>::Convert(str, size, true, false);
            case Charset::UTF32:
                return detail::CharsetConvertor<detail::UTF16, detail::UTF32, Char, char>::Convert(str, size, true, true);
            case Charset::GB18030:
                return detail::CharsetConvertor<detail::UTF16, detail::GB18030, Char, char>::Convert(str, size, true, true);
            default: // should not enter, to please compiler
                return {};
            }
        CHK_CHAR_SIZE_END
        break;
    case Charset::UTF16BE:
        CHK_CHAR_SIZE_MOST(UTF16BE, 2)
            switch (outchset)
            {
            case Charset::ASCII:
                return detail::CharsetConvertor<detail::UTF16, detail::UTF7, Char, char>::Convert(str, size, false, true);
            case Charset::UTF8:
                return detail::CharsetConvertor<detail::UTF16, detail::UTF8, Char, char>::Convert(str, size, false, true);
            case Charset::UTF16LE:
                return detail::CharsetConvertor<detail::UTF16, detail::UTF16, Char, char>::Convert(str, size, false, true);
            case Charset::UTF32:
                return detail::CharsetConvertor<detail::UTF16, detail::UTF32, Char, char>::Convert(str, size, false, true);
            case Charset::GB18030:
                return detail::CharsetConvertor<detail::UTF16, detail::GB18030, Char, char>::Convert(str, size, false, true);
            default: // should not enter, to please compiler
                return {};
            }
        CHK_CHAR_SIZE_END
        break;
    case Charset::UTF32:
        CHK_CHAR_SIZE_MOST(UTF32, 4)
            switch (outchset)
            {
            case Charset::ASCII:
                return detail::CharsetConvertor<detail::UTF32, detail::UTF7, Char, char>::Convert(str, size, true, true);
            case Charset::UTF8:
                return detail::CharsetConvertor<detail::UTF32, detail::UTF8, Char, char>::Convert(str, size, true, true);
            case Charset::UTF16LE:
                return detail::CharsetConvertor<detail::UTF32, detail::UTF16, Char, char>::Convert(str, size, true, true);
            case Charset::UTF16BE:
                return detail::CharsetConvertor<detail::UTF32, detail::UTF16, Char, char>::Convert(str, size, true, false);
            case Charset::GB18030:
                return detail::CharsetConvertor<detail::UTF32, detail::GB18030, Char, char>::Convert(str, size, true, true);
            default: // should not enter, to please compiler
                return {};
            }
        CHK_CHAR_SIZE_END
        break;
    case Charset::GB18030:
        CHK_CHAR_SIZE_MOST(GB18030, 1)
            switch (outchset)
            {
            case Charset::ASCII:
                return detail::CharsetConvertor<detail::GB18030, detail::UTF7, Char, char>::Convert(str, size, true, true);
            case Charset::UTF8:
                return detail::CharsetConvertor<detail::GB18030, detail::UTF8, Char, char>::Convert(str, size, true, true);
            case Charset::UTF16LE:
                return detail::CharsetConvertor<detail::GB18030, detail::UTF16, Char, char>::Convert(str, size, true, true);
            case Charset::UTF16BE:
                return detail::CharsetConvertor<detail::GB18030, detail::UTF16, Char, char>::Convert(str, size, true, false);
            case Charset::UTF32:
                return detail::CharsetConvertor<detail::GB18030, detail::UTF32, Char, char>::Convert(str, size, true, true);
            default: // should not enter, to please compiler
                return {};
            }
        CHK_CHAR_SIZE_END
        break;
    default:
        COMMON_THROW(BaseException, L"unknow charset", inchset);
    }
    COMMON_THROW(BaseException, L"unknow charset", outchset);
}
template<typename Char, typename Alloc>
forceinline std::string to_string(const std::basic_string<Char, Alloc>& str, const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    return to_string(str.data(), str.size(), outchset, inchset);
}
template<typename Char>
forceinline std::string to_string(const std::basic_string_view<Char>& str, const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    return to_string(str.data(), str.size(), outchset, inchset);
}
template<typename Char, typename Alloc>
forceinline std::string to_string(const std::vector<Char, Alloc>& str, const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    return to_string(str.data(), str.size(), outchset, inchset);
}
template<typename Char, size_t N>
forceinline std::string to_string(const Char(&str)[N], const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    return to_string(str, N - 1, outchset, inchset);
}
template<typename Char>
forceinline std::string to_string(const Char* str, const Charset outchset = Charset::ASCII, const Charset inchset = Charset::ASCII)
{
    return to_string(str, std::char_traits<Char>::length(str), outchset, inchset);
}


template<typename Char>
forceinline std::string to_u8string(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    return to_string(str, size, Charset::UTF8, inchset);
}
template<typename Char, typename Traits, typename Alloc>
forceinline std::string to_u8string(const std::basic_string<Char, Traits, Alloc>& str, const Charset inchset = Charset::ASCII)
{
    return to_string(str.data(), str.size(), Charset::UTF8, inchset);
}
template<typename Char, typename Traits>
forceinline std::string to_u8string(const std::basic_string_view<Char, Traits>& str, const Charset inchset = Charset::ASCII)
{
    return to_string(str.data(), str.size(), Charset::UTF8, inchset);
}
template<typename Char, typename Alloc>
forceinline std::string to_u8string(const std::vector<Char, Alloc>& str, const Charset inchset = Charset::ASCII)
{
    return to_string(str.data(), str.size(), Charset::UTF8, inchset);
}
template<typename Char, size_t N>
forceinline std::string to_u8string(const Char(&str)[N], const Charset inchset = Charset::ASCII)
{
    return to_string(str, N - 1, Charset::UTF8, inchset);
}
template<typename Char>
forceinline std::string to_u8string(const Char* str, const Charset inchset = Charset::ASCII)
{
    return to_string(str, std::char_traits<Char>::length(str), Charset::UTF8, inchset);
}


template<typename Char>
inline std::u16string to_u16string(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    switch (inchset)
    {
    case Charset::ASCII:
        CHK_CHAR_SIZE_MOST(ASCII, 1)
            return std::u16string(str, str + size);
        CHK_CHAR_SIZE_END
    case Charset::UTF8:
        CHK_CHAR_SIZE_MOST(UTF8, 1)
            return detail::CharsetConvertor<detail::UTF8, detail::UTF16, Char, char16_t>::Convert(str, size, true, true);
        CHK_CHAR_SIZE_END
    case Charset::UTF16LE:
        CHK_CHAR_SIZE_MOST(UTF16LE, 2)
            return detail::CharsetConvertor<detail::UTF16, detail::UTF16, Char, char16_t>::Convert(str, size, true, true);
        CHK_CHAR_SIZE_END
    case Charset::UTF16BE:
        CHK_CHAR_SIZE_MOST(UTF16BE, 2)
            return detail::CharsetConvertor<detail::UTF16, detail::UTF16, Char, char16_t>::Convert(str, size, false, true);
        CHK_CHAR_SIZE_END
    case Charset::UTF32:
        CHK_CHAR_SIZE_MOST(UTF32, 4)
            return detail::CharsetConvertor<detail::UTF32, detail::UTF16, Char, char16_t>::Convert(str, size, true, true);
        CHK_CHAR_SIZE_END
    case Charset::GB18030:
        CHK_CHAR_SIZE_MOST(GB18030, 1)
            return detail::CharsetConvertor<detail::GB18030, detail::UTF16, Char, char16_t>::Convert(str, size, true, true);
        CHK_CHAR_SIZE_END
    default:
        return std::u16string();
    }
}
template<typename Char, typename Traits, typename Alloc>
forceinline std::u16string to_u16string(const std::basic_string<Char, Traits, Alloc>& str, const Charset inchset = Charset::ASCII)
{
    return to_u16string(str.data(), str.size(), inchset);
}
template<typename Char, typename Traits>
forceinline std::u16string to_u16string(const std::basic_string_view<Char, Traits>& str, const Charset inchset = Charset::ASCII)
{
    return to_u16string(str.data(), str.size(), inchset);
}
template<typename Char, typename Alloc>
forceinline std::u16string to_u16string(const std::vector<Char, Alloc>& str, const Charset inchset = Charset::ASCII)
{
    return to_u16string(str.data(), str.size(), inchset);
}
template<typename Char, size_t N>
forceinline std::u16string to_u16string(const Char(&str)[N], const Charset inchset = Charset::ASCII)
{
    return to_u16string(str, N - 1, inchset);
}
template<typename Char>
forceinline std::u16string to_u16string(const Char* str, const Charset inchset = Charset::ASCII)
{
    return to_u16string(str, std::char_traits<Char>::length(str), inchset);
}


template<typename Char>
inline std::u32string to_u32string(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    switch (inchset)
    {
    case Charset::ASCII:
        CHK_CHAR_SIZE_MOST(ASCII, 1)
            return std::u32string(str, str + size);
        CHK_CHAR_SIZE_END
    case Charset::UTF8:
        CHK_CHAR_SIZE_MOST(UTF8, 1)
            return detail::CharsetConvertor<detail::UTF8, detail::UTF32, Char, char32_t>::Convert(str, size, true, true);
        CHK_CHAR_SIZE_END
    case Charset::UTF16LE:
        CHK_CHAR_SIZE_MOST(UTF16LE, 2)
            return detail::CharsetConvertor<detail::UTF16, detail::UTF32, Char, char32_t>::Convert(str, size, true, true);
        CHK_CHAR_SIZE_END
    case Charset::UTF16BE:
        CHK_CHAR_SIZE_MOST(UTF16BE, 2)
            return detail::CharsetConvertor<detail::UTF16, detail::UTF32, Char, char32_t>::Convert(str, size, false, true);
        CHK_CHAR_SIZE_END
    case Charset::UTF32:
        CHK_CHAR_SIZE_MOST(UTF32, 4)
            return detail::CharsetConvertor<detail::UTF32, detail::UTF32, Char, char32_t>::Convert(str, size, true, true);
        CHK_CHAR_SIZE_END
    case Charset::GB18030:
        CHK_CHAR_SIZE_MOST(GB18030, 1)
            return detail::CharsetConvertor<detail::GB18030, detail::UTF32, Char, char32_t>::Convert(str, size, true, true);
        CHK_CHAR_SIZE_END
    default:
        return std::u32string();
    }
}
template<typename Char, typename Traits, typename Alloc>
forceinline std::u32string to_u32string(const std::basic_string<Char, Traits, Alloc>& str, const Charset inchset = Charset::ASCII)
{
    return to_u32string(str.data(), str.size(), inchset);
}
template<typename Char, typename Traits>
forceinline std::u32string to_u32string(const std::basic_string_view<Char, Traits>& str, const Charset inchset = Charset::ASCII)
{
    return to_u32string(str.data(), str.size(), inchset);
}
template<typename Char, typename Alloc>
forceinline std::u32string to_u32string(const std::vector<Char, Alloc>& str, const Charset inchset = Charset::ASCII)
{
    return to_u32string(str.data(), str.size(), inchset);
}
template<typename Char, size_t N>
forceinline std::u32string to_u32string(const Char(&str)[N], const Charset inchset = Charset::ASCII)
{
    return to_u32string(str, N - 1, inchset);
}
template<typename Char>
forceinline std::u32string to_u32string(const Char* str, const Charset inchset = Charset::ASCII)
{
    return to_u32string(str, std::char_traits<Char>::length(str), inchset);
}


template<typename Char>
forceinline std::wstring to_wstring(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    if constexpr(sizeof(wchar_t) == sizeof(char16_t))
    {
        const auto wstr = to_u16string(str, size, inchset);
        return *(const std::wstring*)&wstr;
    }    
    else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
    {
        const auto wstr = to_u32string(str, size, inchset);
        return *(const std::wstring*)&wstr;
    }  
    else
        return std::wstring();
}
template<typename Char, typename Traits, typename Alloc>
forceinline std::wstring to_wstring(const std::basic_string<Char, Traits, Alloc>& str, const Charset inchset = Charset::ASCII)
{
    if constexpr(sizeof(wchar_t) == sizeof(char16_t))
    {
        const auto wstr = to_u16string(str.data(), str.size(), inchset);
        return *(const std::wstring*)&wstr;
    }    
    else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
    {
        const auto wstr = to_u32string(str.data(), str.size(), inchset);
        return *(const std::wstring*)&wstr;
    }  
    else
        return std::wstring();
}
template<typename Char, typename Traits>
forceinline std::wstring to_wstring(const std::basic_string_view<Char, Traits>& str, const Charset inchset = Charset::ASCII)
{
    if constexpr(sizeof(wchar_t) == sizeof(char16_t))
    {
        const auto wstr = to_u16string(str.data(), str.size(), inchset);
        return *(const std::wstring*)&wstr;
    }    
    else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
    {
        const auto wstr = to_u32string(str.data(), str.size(), inchset);
        return *(const std::wstring*)&wstr;
    }  
    else
        return std::wstring();
}
template<typename Char, typename Alloc>
forceinline std::wstring to_wstring(const std::vector<Char, Alloc>& str, const Charset inchset = Charset::ASCII)
{
    if constexpr(sizeof(wchar_t) == sizeof(char16_t))
    {
        const auto wstr = to_u16string(str.data(), str.size(), inchset);
        return *(const std::wstring*)&wstr;
    }    
    else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
    {
        const auto wstr = to_u32string(str.data(), str.size(), inchset);
        return *(const std::wstring*)&wstr;
    }    
    else
        return std::wstring();
}
template<typename Char, size_t N>
forceinline std::wstring to_wstring(const Char(&str)[N], const Charset inchset = Charset::ASCII)
{
    if constexpr(sizeof(wchar_t) == sizeof(char16_t))
        return *(std::wstring*)&to_u16string(str, N - 1, inchset);
    else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
        return *(std::wstring*)&to_u32string(str, N - 1, inchset);
    else
        return std::wstring();
}
template<typename Char>
forceinline std::wstring to_wstring(const Char* str, const Charset inchset = Charset::ASCII)
{
    if constexpr(sizeof(wchar_t) == sizeof(char16_t))
    {
        const auto wstr = to_u16string(str, std::char_traits<Char>::length(str), inchset);
        return *(const std::wstring*)&wstr;
    }    
    else if constexpr(sizeof(wchar_t) == sizeof(char32_t))
    {
        const auto wstr = to_u32string(str, std::char_traits<Char>::length(str), inchset);
        return *(const std::wstring*)&wstr;
    }
    else
        return std::wstring();
}


template<class T, class = typename std::enable_if_t<std::is_integral_v<T>>>
forceinline std::string to_string(const T val)
{
    return std::to_string(val);
}

template<class T, class = typename std::enable_if_t<std::is_integral_v<T>>>
forceinline std::wstring to_wstring(const T val)
{
    return std::to_wstring(val);
}


template<typename Char, typename Consumer>
inline void ForEachChar(const Char *str, const size_t size, const Consumer& consumer, const Charset inchset = Charset::ASCII)
{
    switch (inchset)
    {
    case Charset::ASCII:
        CHK_CHAR_SIZE_MOST(ASCII, 1)
            detail::ForEachChar<detail::UTF7, Char, Consumer>(str, size, true, consumer);
        CHK_CHAR_SIZE_END
    case Charset::UTF8:
        CHK_CHAR_SIZE_MOST(UTF8, 1)
            detail::ForEachChar<detail::UTF8, Char, Consumer>(str, size, true, consumer);
        CHK_CHAR_SIZE_END
    case Charset::UTF16LE:
        CHK_CHAR_SIZE_MOST(UTF16LE, 2)
            detail::ForEachChar<detail::UTF16, Char, Consumer>(str, size, true, consumer);
        CHK_CHAR_SIZE_END
    case Charset::UTF16BE:
        CHK_CHAR_SIZE_MOST(UTF16BE, 2)
            detail::ForEachChar<detail::UTF16, Char, Consumer>(str, size, false, consumer);
        CHK_CHAR_SIZE_END
    case Charset::UTF32:
        CHK_CHAR_SIZE_MOST(UTF32, 4)
            detail::ForEachChar<detail::UTF32, Char, Consumer>(str, size, true, consumer);
        CHK_CHAR_SIZE_END
    case Charset::GB18030:
        CHK_CHAR_SIZE_MOST(GB18030, 1)
            detail::ForEachChar<detail::GB18030, Char, Consumer>(str, size, true, consumer);
        CHK_CHAR_SIZE_END
    default:
        COMMON_THROW(BaseException, L"unsupported charset");
    }
}
template<typename Char, typename Traits, typename Alloc, typename Consumer>
forceinline void ForEachChar(const std::basic_string<Char, Traits, Alloc>& str, const Consumer& consumer, const Charset inchset = Charset::ASCII)
{
    return ForEachChar(str.data(), str.size(), consumer, inchset);
}
template<typename Char, typename Traits, typename Consumer>
forceinline void ForEachChar(const std::basic_string_view<Char, Traits>& str, const Consumer& consumer, const Charset inchset = Charset::ASCII)
{
    return ForEachChar(str.data(), str.size(), consumer, inchset);
}
template<typename Char, typename Alloc, typename Consumer>
forceinline void ForEachChar(const std::vector<Char, Alloc>& str, const Consumer& consumer, const Charset inchset = Charset::ASCII)
{
    return ForEachChar(str.data(), str.size(), consumer, inchset);
}
template<typename Char, size_t N, typename Consumer>
forceinline void ForEachChar(const Char(&str)[N], const Consumer& consumer, const Charset inchset = Charset::ASCII)
{
    return ForEachChar(str, N - 1, consumer, inchset);
}
template<typename Char, typename Consumer>
forceinline void ForEachChar(const Char* str, const Consumer& consumer, const Charset inchset = Charset::ASCII)
{
    return ForEachChar(str, std::char_traits<Char>::length(str), consumer, inchset);
}


namespace detail
{

forceinline constexpr char32_t EngUpper(const char32_t in)
{
    if (in >= U'a' && in <= U'z')
        return in - U'a' + U'A';
    else return in;
}
forceinline constexpr char32_t EngLower(const char32_t in)
{
    if (in >= U'A' && in <= U'Z')
        return in - U'A' + U'a';
    else return in;
}

}

template<typename Char>
inline std::basic_string<Char> ToUpperEng(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    switch (inchset)
    {
    case Charset::ASCII:
        CHK_CHAR_SIZE_MOST(ASCII, 1)
            //can only be 1-byte string
            std::basic_string<Char> ret; 
            ret.reserve(size);
            std::transform(str, str + size, std::back_inserter(ret), [](const Char ch) { return (ch >= 'a' && ch <= 'z') ? ch - 'a' + 'A' : ch; });
            return ret;
        CHK_CHAR_SIZE_END
    case Charset::UTF8:
        CHK_CHAR_SIZE_MOST(UTF8, 1)
            return detail::CharsetConvertor<detail::UTF8, detail::UTF8, Char, Char>::Transform(str, size, true, true, detail::EngUpper);
        CHK_CHAR_SIZE_END
    case Charset::UTF16LE:
        CHK_CHAR_SIZE_MOST(UTF16LE, 2)
            return detail::CharsetConvertor<detail::UTF16, detail::UTF16, Char, Char>::Transform(str, size, true, true, detail::EngUpper);
        CHK_CHAR_SIZE_END
    case Charset::UTF16BE:
        CHK_CHAR_SIZE_MOST(UTF16BE, 2)
            return detail::CharsetConvertor<detail::UTF16, detail::UTF16, Char, Char>::Transform(str, size, false, true, detail::EngUpper);
        CHK_CHAR_SIZE_END
    case Charset::UTF32:
        CHK_CHAR_SIZE_MOST(UTF32, 4)
            return detail::CharsetConvertor<detail::UTF32, detail::UTF32, Char, Char>::Transform(str, size, true, true, detail::EngUpper);
        CHK_CHAR_SIZE_END
    case Charset::GB18030:
        CHK_CHAR_SIZE_MOST(GB18030, 1)
            return detail::CharsetConvertor<detail::GB18030, detail::GB18030, Char, Char>::Transform(str, size, true, true, detail::EngUpper);
        CHK_CHAR_SIZE_END
    default:
        COMMON_THROW(BaseException, L"unsupported charset");
    }
}
template<typename Char, typename Traits, typename Alloc>
forceinline std::basic_string<Char> ToUpperEng(const std::basic_string<Char, Traits, Alloc>& str, const Charset inchset = Charset::ASCII)
{
    return ToUpperEng(str.data(), str.length(), inchset);
}
template<typename Char, typename Traits>
forceinline std::basic_string<Char> ToUpperEng(const std::basic_string_view<Char, Traits>& str, const Charset inchset = Charset::ASCII)
{
    return ToUpperEng(str.data(), str.length(), inchset);
}
template<typename Char, typename Alloc>
forceinline std::basic_string<Char> ToUpperEng(const std::vector<Char, Alloc>& str, const Charset inchset = Charset::ASCII)
{
    return ToUpperEng(str.data(), str.size(), inchset);
}
template<typename Char, size_t N>
forceinline std::basic_string<Char> ToUpperEng(const Char(&str)[N], const Charset inchset = Charset::ASCII)
{
    return ToUpperEng(str, N - 1, inchset);
}
template<typename Char>
forceinline std::basic_string<Char> ToUpperEng(const Char* str, const Charset inchset = Charset::ASCII)
{
    return ToUpperEng(str, std::char_traits<Char>::length(str), inchset);
}


template<typename Char>
inline std::basic_string<Char> ToLowerEng(const Char *str, const size_t size, const Charset inchset = Charset::ASCII)
{
    switch (inchset)
    {
    case Charset::ASCII:
        CHK_CHAR_SIZE_MOST(ASCII, 1)
            std::basic_string<Char> ret;
            ret.reserve(size);
            std::transform(str, str + size, std::back_inserter(ret), [](const Char ch) { return (ch >= 'A' && ch <= 'Z') ? ch - 'A' + 'a' : ch; });
            return ret;
        CHK_CHAR_SIZE_END
    case Charset::UTF8:
        CHK_CHAR_SIZE_MOST(UTF8, 1)
            return detail::CharsetConvertor<detail::UTF8, detail::UTF8, Char, Char>::Transform(str, size, true, true, detail::EngLower);
        CHK_CHAR_SIZE_END
    case Charset::UTF16LE:
        CHK_CHAR_SIZE_MOST(UTF16LE, 2)
            return detail::CharsetConvertor<detail::UTF16, detail::UTF16, Char, Char>::Transform(str, size, true, true, detail::EngLower);
        CHK_CHAR_SIZE_END
    case Charset::UTF16BE:
        CHK_CHAR_SIZE_MOST(UTF16BE, 2)
            return detail::CharsetConvertor<detail::UTF16, detail::UTF16, Char, Char>::Transform(str, size, false, true, detail::EngLower);
        CHK_CHAR_SIZE_END
    case Charset::UTF32:
        CHK_CHAR_SIZE_MOST(UTF32, 4)
            std::basic_string<Char> ret;
            ret.reserve(size);
            std::transform(str, str + size, std::back_inserter(ret), detail::EngLower);
            return ret;
        CHK_CHAR_SIZE_END
    case Charset::GB18030:
        CHK_CHAR_SIZE_MOST(GB18030, 1)
            return detail::CharsetConvertor<detail::GB18030, detail::GB18030, Char, Char>::Transform(str, size, true, true, detail::EngLower);
        CHK_CHAR_SIZE_END
    default:
        COMMON_THROW(BaseException, L"unsupported charset");
    }
}
template<typename Char, typename Traits, typename Alloc>
forceinline std::basic_string<Char> ToLowerEng(const std::basic_string<Char, Traits, Alloc>& str, const Charset inchset = Charset::ASCII)
{
    return ToLowerEng(str.data(), str.length(), inchset);
}
template<typename Char, typename Traits>
forceinline std::basic_string<Char> ToLowerEng(const std::basic_string_view<Char, Traits>& str, const Charset inchset = Charset::ASCII)
{
    return ToLowerEng(str.data(), str.length(), inchset);
}
template<typename Char, typename Alloc>
forceinline std::basic_string<Char> ToLowerEng(const std::vector<Char, Alloc>& str, const Charset inchset = Charset::ASCII)
{
    return ToLowerEng(str.data(), str.size(), inchset);
}
template<typename Char, size_t N>
forceinline std::basic_string<Char> ToLowerEng(const Char(&str)[N], const Charset inchset = Charset::ASCII)
{
    return ToLowerEng(str, N - 1, inchset);
}
template<typename Char>
forceinline std::basic_string<Char> ToLowerEng(const Char* str, const Charset inchset = Charset::ASCII)
{
    return ToLowerEng(str, std::char_traits<Char>::length(str), inchset);
}


namespace detail
{
template<class From1, class From2, typename CharT1, typename CharT2>
inline bool CaseInsensitiveCompare(const CharT1 *str, const size_t size1, const bool fromLE1, const CharT2 *prefix, const size_t size2, const bool fromLE2)
{
    bool isEqual = true;
    ForEachCharPair(str, size1, fromLE1, prefix, size2, fromLE2, [&](const char32_t ch1, const char32_t ch2) 
    {
        const auto ch3 = (ch1 >= U'a' && ch1 <= U'z') ? (ch1 - U'a' + U'A') : ch1;
        const auto ch4 = (ch2 >= U'a' && ch2 <= U'z') ? (ch2 - U'a' + U'A') : ch2;
        isEqual = ch3 == ch4;
        return !isEqual;
    });
    return isEqual;
}
}
template<typename Char>
inline bool IsIBeginWith(const Char *str, const size_t size1, const Char *prefix, const size_t size2, const Charset strchset = Charset::ASCII)
{
    if (size2 > size1)
        return false;
    switch (strchset)
    {
    case Charset::ASCII:
        CHK_CHAR_SIZE_MOST(ASCII, 1)
            return detail::CaseInsensitiveCompare<detail::UTF7, detail::UTF7, Char, Char>(str, size1, true, prefix, size2, true);
        CHK_CHAR_SIZE_END
    case Charset::UTF8:
        CHK_CHAR_SIZE_MOST(UTF8, 1)
            return detail::CaseInsensitiveCompare<detail::UTF8, detail::UTF8, Char, Char>(str, size1, true, prefix, size2, true);
        CHK_CHAR_SIZE_END
    case Charset::UTF16LE:
        CHK_CHAR_SIZE_MOST(UTF16LE, 2)
            return detail::CaseInsensitiveCompare<detail::UTF16, detail::UTF16, Char, Char>(str, size1, true, prefix, size2, true);
        CHK_CHAR_SIZE_END
    case Charset::UTF16BE:
        CHK_CHAR_SIZE_MOST(UTF16BE, 2)
            return detail::CaseInsensitiveCompare<detail::UTF16, detail::UTF16, Char, Char>(str, size1, false, prefix, size2, false);
        CHK_CHAR_SIZE_END
    case Charset::UTF32:
        CHK_CHAR_SIZE_MOST(UTF32, 4)
            return detail::CaseInsensitiveCompare<detail::UTF32, detail::UTF32, Char, Char>(str, size1, true, prefix, size2, true);
        CHK_CHAR_SIZE_END
    case Charset::GB18030:
        CHK_CHAR_SIZE_MOST(GB18030, 1)
            return detail::CaseInsensitiveCompare<detail::GB18030, detail::GB18030, Char, Char>(str, size1, true, prefix, size2, true);
        CHK_CHAR_SIZE_END
    default:
        COMMON_THROW(BaseException, L"unsupported charset");
    }
}
template<typename Char, typename Container>
forceinline bool IsIBeginWith(const Char *str, const size_t size, const Char *prefix, const Charset strchset = Charset::ASCII)
{
    return IsIBeginWith(str, size, prefix, std::char_traits<Char>::length(prefix), strchset);
}
template<typename Char, typename Container>
forceinline bool IsIBeginWith(const Char *str, const Char *prefix, const size_t size, const Charset strchset = Charset::ASCII)
{
    return IsIBeginWith(str, std::char_traits<Char>::length(str), prefix, size, strchset);
}
template<typename Char, typename Container>
forceinline bool IsIBeginWith(const Char *str, const Char *prefix, const Charset strchset = Charset::ASCII)
{
    return IsIBeginWith(str, std::char_traits<Char>::length(str), prefix, std::char_traits<Char>::length(prefix), strchset);
}
template<typename Char, typename Container>
forceinline bool IsIBeginWith(const Char *str, const size_t size, const Container& prefix, const Charset strchset = Charset::ASCII)
{
    return IsIBeginWith(str, size, prefix.data(), prefix.data(), strchset);
}
template<typename Char, typename Container>
forceinline bool IsIBeginWith(const Container& str, const Char *prefix, const size_t size, const Charset strchset = Charset::ASCII)
{
    return IsIBeginWith(str.data(), str.size(), prefix, size, strchset);
}
template<typename Char, typename Container1, typename Container2>
forceinline bool IsIBeginWith(const Container1& str, const Container2& prefix, const Charset strchset = Charset::ASCII)
{
    return IsIBeginWith(str.data(), str.size(), prefix.data(), prefix.size(), strchset);
}

}


