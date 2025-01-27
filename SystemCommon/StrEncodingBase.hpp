#pragma once

#include "common/CommonRely.hpp"
#include "common/StrBase.hpp"
#include "common/CompactPack.hpp"


namespace common::str::charset::detail
{

static_assert(common::detail::is_little_endian, "Need Little Endian");

inline constexpr auto InvalidChar = static_cast<char32_t>(-1);
inline constexpr auto InvalidCharPair = std::pair<char32_t, uint32_t>{ InvalidChar, 0 };

// `FromXXX` ensures CP correctness, `ToXXX` expects CP correctness

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

template<typename T, bool IsLE, typename U>
[[nodiscard]] forceinline constexpr T EndianReader(const U* __restrict const src) noexcept
{
#if defined(COMMON_SIMD_LV)
    return ::common::simd::EndianReader<T, IsLE>(src);
#else
    return ::common::EndianReader<T, IsLE>(src);
#endif
}
template<bool IsLE, typename U, typename T>
forceinline constexpr void EndianWriter(U* __restrict const dst, const T val) noexcept
{
#if defined(COMMON_SIMD_LV)
    ::common::simd::EndianWriter<IsLE>(dst, val);
#else
    ::common::EndianWriter<IsLE>(dst, val);
#endif
}


struct ConvertCPBase 
{
    static inline constexpr uint32_t UTF32Max = 0x0010FFFFu;
    static inline constexpr uint32_t UTF32SurrogateHi = 0xD800u;
    static inline constexpr uint32_t UTF32SurrogateLo = 0xDC00u;
    static inline constexpr uint32_t UTF32SurrogateEnd = 0xDFFFu;
    [[nodiscard]] forceinline static constexpr bool CheckCPValid(char32_t cp) noexcept
    {
        const auto tmp = cp ^ UTF32SurrogateHi;
        return tmp >= 0x0800u && cp <= UTF32Max; // not within [d800,dfff]
    }
};
struct ConvertByteBase 
{ };

struct UTF7 : public ConvertCPBase, public ConvertByteBase
{
    using ElementType = char;
    static inline constexpr size_t MinBytes = 1;
    static inline constexpr size_t MaxOutputUnit = 1;
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> From(const ElementType* __restrict const src, [[maybe_unused]] const size_t size) noexcept
    {
        IF_LIKELY(src[0] > 0)
            return { src[0], 1 };
        else
            return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t To(const char32_t src, [[maybe_unused]] const size_t size, ElementType* __restrict const dest) noexcept
    {
        IF_LIKELY(src < 128)
        {
            dest[0] = static_cast<char>(src);
            return 1;
        }
        else
            return 0;
    }
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, [[maybe_unused]] const size_t size) noexcept
    {
        IF_LIKELY(src[0] > 0)
            return { src[0], 1 };
        else
            return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t ToBytes(const char32_t src, [[maybe_unused]] const size_t size, uint8_t* __restrict const dest) noexcept
    {
        IF_LIKELY(src < 128)
        {
            dest[0] = static_cast<uint8_t>(src);
            return 1;
        }
        else
            return 0;
    }
};


struct UTF32 : public ConvertCPBase
{
    using ElementType = char32_t;
    static inline constexpr size_t MinBytes = 4;
    static inline constexpr size_t MaxOutputUnit = 1;
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> From(const ElementType* __restrict const src, [[maybe_unused]] const size_t size) noexcept
    {
        const auto ch = src[0];
        IF_LIKELY(CheckCPValid(ch))
            return { ch, 1 };
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t To(const char32_t src, [[maybe_unused]] const size_t size, ElementType* __restrict const dest) noexcept
    {
        dest[0] = src;
        return 1;
    }
};
struct UTF32LE : public UTF32, public ConvertByteBase
{
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, [[maybe_unused]] const size_t size) noexcept
    {
        const auto ch = EndianReader<uint32_t, true>(src);
        IF_LIKELY(CheckCPValid(ch))
            return { ch, 4 };
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t ToBytes(const char32_t src, [[maybe_unused]] const size_t size, uint8_t* __restrict const dest) noexcept
    {
        EndianWriter<true>(dest, src);
        return 4;
    }
};
struct UTF32BE : public UTF32, public ConvertByteBase
{
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, [[maybe_unused]] const size_t size) noexcept
    {
        const auto ch = EndianReader<uint32_t, false>(src);
        IF_LIKELY(CheckCPValid(ch))
            return { ch, 4 };
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t ToBytes(const char32_t src, [[maybe_unused]] const size_t size, uint8_t* __restrict const dest) noexcept
    {
        EndianWriter<false>(dest, src);
        return 4;
    }
};

struct UTF8 : public ConvertCPBase, public ConvertByteBase
{
    // [0..15] for bits [4,7]
    static inline constexpr auto UTF8MultiLenPack = BitsPackFrom<2>(
        0,0,0,0,0,0,0,0, // 0xxx
        0, // 1000
        0, // 1001
        0, // 1010
        0, // 1011
        1, // 1100
        1, // 1101
        2, // 1110
        3  // 1111
    );
    static inline constexpr auto UTF8MultiFirstMaskPack = BitsPackFrom<8>(
        0x7f, // 0yyyzzzz
        0x1f, // 110xxxyy
        0xf,  // 1110wwww
        0xf   // 11110uvv, keep 0uvv to make sure msb is 0
    );
    static inline constexpr auto UTF8MultiSecondCheckPack = BitsPackFrom<8>(
        0, // invalid
        0x80 >> 0,
        0x800u >> 6,
        0x10000u >> 12
    );

    template<typename T>
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> InnerFrom(const T* __restrict const src, const size_t size) noexcept
    {
        const auto ch0 = static_cast<uint8_t>(src[0]);
        IF_LIKELY(ch0 < 0x80u) // 1 byte
            return { ch0, 1 };

        const auto leftBytes = UTF8MultiLenPack.Get<uint8_t>(ch0 >> 4);
        IF_UNLIKELY(leftBytes == 0 || (1u + leftBytes > size))
            return InvalidCharPair;

        const auto ch0kept = UTF8MultiFirstMaskPack.GetWithoutMask(leftBytes) & ch0; // ch0 is uint8, already ensure mask keep only 8bit
        const auto ch1 = static_cast<uint8_t>(src[1]);
        const auto tmp1 = ch1 ^ 0b10000000u; // flip highest and keep following, valid will be 00xxxxxx, larger is invalid
        IF_UNLIKELY(tmp1 > 0b00111111u)
            return InvalidCharPair;

        const auto first2 = (ch0kept << 6) + tmp1;
        IF_UNLIKELY(first2 < static_cast<uint8_t>(UTF8MultiSecondCheckPack.GetWithoutMask(leftBytes))) // overlong encodings
            return InvalidCharPair;

        IF_LIKELY(leftBytes == 1)
            return { static_cast<char32_t>(first2), 2 };

        const auto ch2 = static_cast<uint8_t>(src[2]);
        const auto tmp2 = ch2 ^ 0b10000000u; // flip highest and keep following, valid will be 00xxxxxx, larger is invalid
        IF_UNLIKELY(tmp2 > 0b00111111u)
            return InvalidCharPair;

        IF_LIKELY(leftBytes == 2) // 0x0800~0xffff
        {
            const auto tmp = first2 ^ (0xd800u >> 6);
            IF_LIKELY(tmp >= (0x0800u >> 6)) // not within [d800,dfff]
                return { static_cast<char32_t>((first2 << 6) + tmp2), 3 };
            return InvalidCharPair;
        }
        else // 3, 0x010000~0x10ffff
        {
            IF_UNLIKELY(first2 > (UTF32Max >> 12))
                return InvalidCharPair;

            const auto ch3 = static_cast<uint8_t>(src[3]);
            const auto tmp3 = ch3 ^ 0b10000000u; // flip highest and keep following, valid will be 00xxxxxx, larger is invalid
            IF_UNLIKELY(tmp3 > 0b00111111u)
                return InvalidCharPair;
            return { static_cast<char32_t>((first2 << 12) + (tmp2 << 6) + tmp3), 4 };
        }
    }
    template<typename T>
    [[nodiscard]] forceinline static constexpr uint8_t InnerTo(const char32_t src, const size_t size, T* __restrict const dest) noexcept
    {
        IF_LIKELY(src < 0x80) // 1 byte
        {
            dest[0] = static_cast<T>(src & 0x7f);
            return 1;
        }
        else if (src < 0x800) // 2 byte
        {
            IF_LIKELY(size >= 2)
            {
                dest[0] = static_cast<T>(0b11000000 | ((src >> 6) & 0x3f)), dest[1] = static_cast<T>(0x80 | (src & 0x3f));
                return 2;
            }
        }
        else if (src < 0x10000) // 3 byte
        {
            IF_LIKELY(size >= 3)
            {
                dest[0] = static_cast<T>(0b11100000 | (src >> 12)), dest[1] = static_cast<T>(0x80 | ((src >> 6) & 0x3f)), dest[2] = static_cast<T>(0x80 | (src & 0x3f));
                return 3;
            }
        }
        else // 4 byte, no need for max check
        {
            IF_LIKELY(size >= 4)
            {
                dest[0] = static_cast<T>(0b11110000 | (src >> 18)), dest[1] = static_cast<T>(0x80 | ((src >> 12) & 0x3f));
                dest[2] = static_cast<T>(0x80 | ((src >> 6) & 0x3f)), dest[3] = static_cast<T>(0x80 | (src & 0x3f));
                return 4;
            }
        }
        return 0;
    }
    using ElementType = u8ch_t;
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
                dest[0] = (T)(src & 0x7f);
                return 1;
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
        else // 4 byte, no need to check max
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
    static inline constexpr size_t MinBytes = 1;
    static inline constexpr size_t MaxOutputUnit = 12;
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
    // d800 = 1101 10yy yyyy yyyy
    // dc00 = 1101 11xx xxxx xxxx
    // 1101 1... .... .... = d800
    // 0010 0... .... .... = 27ff
    using ElementType = char16_t;
    static inline constexpr size_t MinBytes = 2;
    static inline constexpr size_t MaxOutputUnit = 2;
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> From(const char16_t* __restrict const src, const size_t size) noexcept
    {
        const auto ch0 = src[0];
        const uint32_t y = ch0 ^ 0xd800;
        IF_LIKELY(y >= 0x0800u) // not within [d800,dfff], 1 unit
            return { ch0, 1 };
        IF_LIKELY(size >= 2 && ch0 < 0xdc00) // can be 2 unit
        {
            const auto ch1 = src[1];
            const uint32_t x = ch1 ^ 0xdc00;
            IF_LIKELY(x < 0x0400) // within [dc00,dfff], 2 unit
                return { (y << 10) + x + 0x10000u, 2 };
        }
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t To(const char32_t src, const size_t size, char16_t* __restrict const dest) noexcept
    {
        IF_LIKELY(src < 0x10000u)
        {
            dest[0] = static_cast<char16_t>(src);
            return 1;
        }
        else
        {
            IF_LIKELY(size >= 2)
            {
                const auto tmp2 = src - 0x10000u;
                dest[0] = static_cast<char16_t>(0xd800 | (tmp2 >> 10)), dest[1] = static_cast<char16_t>(0xdc00 | (tmp2 & 0x3ff));
                return 2;
            }
        }
        return 0;
    }
};
struct UTF16LE : public UTF16, public ConvertByteBase
{
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, const size_t size) noexcept
    {
        const auto ch0 = EndianReader<uint16_t, true>(src);
        const uint32_t y = ch0 ^ 0xd800;
        IF_LIKELY(y >= 0x0800u) // not within [d800,dfff], 1 unit
            return { ch0, 2 };
        IF_LIKELY(size >= 4 && ch0 < 0xdc00) // can be 2 unit
        {
            const auto ch1 = EndianReader<uint16_t, true>(src + 2);
            const uint32_t x = ch1 ^ 0xdc00;
            IF_LIKELY(x < 0x0400) // within [dc00,dfff], 2 unit
                return { (y << 10) + x + 0x10000u, 4 };
        }
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t ToBytes(const char32_t src, const size_t size, uint8_t* __restrict const dest) noexcept
    {
        IF_LIKELY(src < 0x10000u)
        {
            EndianWriter<true>(dest, static_cast<char16_t>(src));
            return 2;
        }
        else
        {
            IF_LIKELY(size >= 4)
            {
                const auto tmp2 = src - 0x10000u;
                EndianWriter<true>(dest + 0, static_cast<char16_t>(0xd800 | (tmp2 >> 10)));
                EndianWriter<true>(dest + 2, static_cast<char16_t>(0xdc00 | (tmp2 & 0x3ff)));
                return 4;
            }
        }
        return 0;
    }
};
struct UTF16BE : public UTF16, public ConvertByteBase
{
    [[nodiscard]] forceinline static constexpr std::pair<char32_t, uint32_t> FromBytes(const uint8_t* __restrict const src, const size_t size) noexcept
    {
        const auto ch0 = EndianReader<uint16_t, false>(src);
        const uint32_t y = ch0 ^ 0xd800;
        IF_LIKELY(y >= 0x0800u) // not within [d800,dfff], 1 unit
            return { ch0, 2 };
        IF_LIKELY(size >= 4 && ch0 < 0xdc00) // can be 2 unit
        {
            const auto ch1 = EndianReader<uint16_t, false>(src + 2);
            const uint32_t x = ch1 ^ 0xdc00;
            IF_LIKELY(x < 0x0400) // within [dc00,dfff], 2 unit
                return { (y << 10) + x + 0x10000u, 4 };
        }
        return InvalidCharPair;
    }
    [[nodiscard]] forceinline static constexpr uint8_t ToBytes(const char32_t src, const size_t size, uint8_t* __restrict const dest) noexcept
    {
        IF_LIKELY(src < 0x10000u)
        {
            EndianWriter<false>(dest, static_cast<char16_t>(src));
            return 2;
        }
        else
        {
            IF_LIKELY(size >= 4)
            {
                const auto tmp2 = src - 0x10000u;
                EndianWriter<false>(dest + 0, static_cast<char16_t>(0xd800 | (tmp2 >> 10)));
                EndianWriter<false>(dest + 2, static_cast<char16_t>(0xdc00 | (tmp2 & 0x3ff)));
                return 4;
            }
        }
        return 0;
    }
};


}


