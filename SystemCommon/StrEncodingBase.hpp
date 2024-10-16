#pragma once

#include "common/CommonRely.hpp"
#include "common/StrBase.hpp"


namespace common::str::charset::detail
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
    static constexpr size_t MaxOutputUnit = 1;
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
    static constexpr size_t MaxOutputUnit = 1;
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
        if (size >= 4u)
        {
            const uint32_t ch = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
            if (ch < 0x200000u)
                return { ch, 4 };
        }
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t ToBytes(const char32_t src, const size_t size, uint8_t* __restrict const dest) noexcept
    {
        if (size >= 4u && src < 0x200000u)
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
        if (size >= 4u)
        {
            const uint32_t ch = src[3] | (src[2] << 8) | (src[1] << 16) | (src[0] << 24);
            if (ch < 0x200000u)
                return { ch, 4 };
        }
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t ToBytes(const char32_t src, const size_t size, uint8_t* __restrict const dest) noexcept
    {
        if (size >= 4u && src < 0x200000u)
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
            if (size >= 3 && ((src[1] & 0xc0u) == 0x80u) && ((src[2] & 0xc0u) == 0x80u))
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
    using ElementType = u8ch_t;
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

struct URI : public ConvertCPBase, public ConvertByteBase
{
private:
    inline static constexpr uint8_t ParseChar(uint8_t ch) noexcept
    {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        return 0xff;
    }
    template<typename T>
    [[nodiscard]] inline static constexpr std::pair<char32_t, uint32_t> InnerFrom(const T* __restrict const src, const size_t size) noexcept
    {
        if (size == 0)
            return InvalidCharPair;
        const auto ch0 = static_cast<uint8_t>(src[0]);
        if (ch0 >= 0x80u || ch0 <= 0x20u) // should be encoded
            return InvalidCharPair;
        if (src[0] != '%') // direct output
            return { ch0, 1 };
        // need decode
        if (size < 3)
            return InvalidCharPair;
        const auto val0Hi = ParseChar(src[1]), val0Lo = ParseChar(src[2]);
        if (val0Hi == 0xff || val0Lo == 0xff)
            return InvalidCharPair;
        const uint32_t val0 = (val0Hi << 4) + val0Lo;
        if (val0 < 0x80u) // 1 byte
            return { val0, 3 };
        else if ((val0 & 0xe0u) == 0xc0u) // 2 byte
        {
            if (size >= 6)
            {
                const auto val1Hi = ParseChar(src[4]), val1Lo = ParseChar(src[5]);
                if (src[3] == '%' && val1Hi != 0xff && val1Lo != 0xff)
                {
                    const uint32_t val1 = (val1Hi << 4) + val1Lo;
                    if ((val1 & 0xc0u) == 0x80u)
                        return { ((val0 & 0x1fu) << 6) | (val1 & 0x3fu), 6 };
                }
            }
        }
        else if ((val0 & 0xf0u) == 0xe0u) // 3 byte
        {
            if (size >= 9)
            {
                const auto val1Hi = ParseChar(src[4]), val1Lo = ParseChar(src[5]);
                const auto val2Hi = ParseChar(src[7]), val2Lo = ParseChar(src[8]);
                if (src[3] == '%' && src[6] == '%' && val1Hi != 0xff && val1Lo != 0xff && val2Hi != 0xff && val2Lo != 0xff)
                {
                    const uint32_t val1 = (val1Hi << 4) + val1Lo;
                    const uint32_t val2 = (val2Hi << 4) + val2Lo;
                    if ((val1 & 0xc0u) == 0x80u && (val2 & 0xc0u) == 0x80u)
                        return { ((val0 & 0x0fu) << 12) | ((val1 & 0x3fu) << 6) | (val2 & 0x3fu), 9 };
                }
            }
        }
        else if ((val0 & 0xf8u) == 0xf0u) // 4 byte
        {
            if (size >= 12)
            {
                const auto val1Hi = ParseChar(src[ 4]), val1Lo = ParseChar(src[ 5]);
                const auto val2Hi = ParseChar(src[ 7]), val2Lo = ParseChar(src[ 8]);
                const auto val3Hi = ParseChar(src[10]), val3Lo = ParseChar(src[11]);
                if (src[3] == '%' && src[6] == '%' && src[9] == '%' && val1Hi != 0xff && val1Lo != 0xff && 
                    val2Hi != 0xff && val2Lo != 0xff && val3Hi != 0xff && val3Lo != 0xff)
                {
                    const uint32_t val1 = (val1Hi << 4) + val1Lo;
                    const uint32_t val2 = (val2Hi << 4) + val2Lo;
                    const uint32_t val3 = (val3Hi << 4) + val3Lo;
                    if ((val1 & 0xc0u) == 0x80u && (val2 & 0xc0u) == 0x80u && (val3 & 0xc0u) == 0x80u)
                        return { ((val0 & 0x0fu) << 18) | ((val1 & 0x3fu) << 12) | ((val2 & 0x3fu) << 6) | (val3 & 0x3fu), 12 };
                }
            }
        }
        return InvalidCharPair;
    }
    template<typename T>
    [[nodiscard]] inline static constexpr uint8_t InnerTo(const char32_t src, const size_t size, T* __restrict const dest) noexcept
    {
        constexpr char Hex16[] = "0123456789ABCDEF";
        if (src < 0x80)
        {
            if (src > 0x20u && src != '%') // direct output
            {
                if (size >= 1)
                {
                    dest[0] = (T)(src & 0x7f);
                    return 1;
                }
            }
            else // 1 byte
            {
                if (size >= 3)
                {
                    dest[0] = '%'; dest[1] = Hex16[src >> 4]; dest[2] = Hex16[src & 0xf];
                    return 3;
                }
            }
        }
        else if (src < 0x800) // 2 byte
        {
            if (size >= 6)
            {
                const auto val0 = T(0b11000000 | ((src >> 6) & 0x3f)), val1 = T(0x80 | (src & 0x3f));
                dest[0] = '%'; dest[1] = Hex16[val0 >> 4]; dest[2] = Hex16[val0 & 0xf];
                dest[3] = '%'; dest[4] = Hex16[val1 >> 4]; dest[5] = Hex16[val1 & 0xf];
                return 6;
            }
        }
        else if (src < 0x10000) // 3 byte
        {
            if (size >= 9)
            {
                const auto val0 = T(0b11100000 | (src >> 12)), val1 = T(0x80 | ((src >> 6) & 0x3f)), val2 = T(0x80 | (src & 0x3f));
                dest[0] = '%'; dest[1] = Hex16[val0 >> 4]; dest[2] = Hex16[val0 & 0xf];
                dest[3] = '%'; dest[4] = Hex16[val1 >> 4]; dest[5] = Hex16[val1 & 0xf];
                dest[6] = '%'; dest[7] = Hex16[val2 >> 4]; dest[8] = Hex16[val2 & 0xf];
                return 9;
            }
        }
        else if (src < 0x200000) // 4 byte
        {
            if (size >= 12)
            {
                const auto val0 = T(0b11110000 | (src >> 18)), val1 = T(0x80 | ((src >> 12) & 0x3f));
                const auto val2 = T(0x80 | ((src >> 6) & 0x3f)), val3 = T(0x80 | (src & 0x3f));
                dest[0] = '%'; dest[ 1] = Hex16[val0 >> 4]; dest[ 2] = Hex16[val0 & 0xf];
                dest[3] = '%'; dest[ 4] = Hex16[val1 >> 4]; dest[ 5] = Hex16[val1 & 0xf];
                dest[6] = '%'; dest[ 7] = Hex16[val2 >> 4]; dest[ 8] = Hex16[val2 & 0xf];
                dest[9] = '%'; dest[10] = Hex16[val3 >> 4]; dest[11] = Hex16[val3 & 0xf];
                return 12;
            }
        }
        return 0;
    }
public:
    using ElementType = char;
    static constexpr size_t MaxOutputUnit = 12;
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
    using ElementType = char16_t;
    static constexpr size_t MaxOutputUnit = 2;
    [[nodiscard]] inline static constexpr std::pair<char32_t, uint32_t> From(const char16_t* __restrict const src, const size_t size) noexcept
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
    [[nodiscard]] inline static constexpr uint8_t To(const char32_t src, const size_t size, char16_t* __restrict const dest) noexcept
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
    [[nodiscard]] inline static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, const size_t size) noexcept
    {
        if (size < 2)
            return InvalidCharPair;
        if (src[0] < 0xd8 || src[0] >= 0xe0)//1 unit
            return { (src[0] << 8) | src[1], 2 };
        if (src[0] < 0xdc)//2 unit
        {
            if (size >= 4 && src[2] >= 0xdc && src[2] <= 0xdf)
                return { (((src[0] & 0x3) << 18) | (src[1] << 10) | ((src[2] & 0x3) << 8) | src[3]) + 0x10000, 4 };
        }
        return InvalidCharPair;
    }
    [[nodiscard]] inline static constexpr uint8_t ToBytes(const char32_t src, const size_t size, uint8_t* __restrict const dest) noexcept
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
    [[nodiscard]] inline static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, const size_t size) noexcept
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
    [[nodiscard]] inline static constexpr uint8_t ToBytes(const char32_t src, const size_t size, uint8_t* __restrict const dest) noexcept
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


}


