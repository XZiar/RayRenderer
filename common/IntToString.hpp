#pragma once
#include <type_traits> 
#if defined(__SSE2__) || defined(__SSE3__) || defined(__SSSE3__) || defined(__SSE4_1__) || defined(__SSE4_2__) || defined(__AVX__) || defined(__AVX2__) || defined(__FMA__)
#   include "3rdParty/itoa_sse2.hpp"
#   define ITOA_U32 common::itoa::u32toa_sse2
#   define ITOA_U64 common::itoa::u64toa_sse2
#else
#   include "3rdParty/itoa_branchlut2.hpp"
#   define ITOA_U32 common::itoa::u32toa_branchlut2
#   define ITOA_U64 common::itoa::u64toa_branchlut2
#endif
#include <string>

namespace common
{

namespace str
{

namespace detail
{
template<typename Char>
inline std::basic_string<Char> I32_Str(const int32_t num)
{
    char tmp[12];
    const char* end = nullptr;
    if (num < 0)
    {
        tmp[0] = '-';
        end = ITOA_U32(~static_cast<uint32_t>(num) + 1, &tmp[1]);
    }
    else
        end = ITOA_U32(static_cast<uint32_t>(num), &tmp[0]);
    return std::basic_string<Char>((const char*)tmp, end);
}
template<typename Char>
inline std::basic_string<Char> U32_Str(const uint32_t num)
{
    char tmp[11];
    const char* end = nullptr;
    end = ITOA_U32(num, &tmp[0]);
    return std::basic_string<Char>((const char*)tmp, end);
}
template<typename Char>
inline std::basic_string<Char> I64_Str(const int64_t num)
{
    char tmp[21];
    const char* end = nullptr;
    if (num < 0)
    {
        tmp[0] = '-';
        end = ITOA_U64(~static_cast<uint64_t>(num) + 1, &tmp[1]);
    }
    else
        end = ITOA_U64(static_cast<uint64_t>(num), &tmp[0]);
    return std::basic_string<Char>((const char*)tmp, end);
}
template<typename Char>
inline std::basic_string<Char> U64_Str(const uint64_t num)
{
    char tmp[20];
    const char* end = nullptr;
    end = ITOA_U64(num, &tmp[0]);
    return std::basic_string<Char>((const char*)tmp, end);
}
}

inline std::string to_string(const int32_t num)
{
    return detail::I32_Str<char>(num);
}
inline std::string to_string(const uint32_t num)
{
    return detail::U32_Str<char>(num);
}
inline std::string to_string(const int64_t num)
{
    return detail::I64_Str<char>(num);
}
inline std::string to_string(const uint64_t num)
{
    return detail::U64_Str<char>(num);
}
inline std::u16string to_u16string(const int32_t num)
{
    return detail::I32_Str<char16_t>(num);
}
inline std::u16string to_u16string(const uint32_t num)
{
    return detail::U32_Str<char16_t>(num);
}
inline std::u16string to_u16string(const int64_t num)
{
    return detail::I64_Str<char16_t>(num);
}
inline std::u16string to_u16string(const uint64_t num)
{
    return detail::U64_Str<char16_t>(num);
}
inline std::u32string to_u32string(const int32_t num)
{
    return detail::I32_Str<char32_t>(num);
}
inline std::u32string to_u32string(const uint32_t num)
{
    return detail::U32_Str<char32_t>(num);
}
inline std::u32string to_u32string(const int64_t num)
{
    return detail::I64_Str<char32_t>(num);
}
inline std::u32string to_u32string(const uint64_t num)
{
    return detail::U64_Str<char32_t>(num);
}

}


}