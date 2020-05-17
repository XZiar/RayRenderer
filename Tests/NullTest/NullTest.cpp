#include "common/CommonRely.hpp"
#include "common/Linq2.hpp"
#include "common/AlignedBuffer.hpp"
#include "common/StrParsePack.hpp"
#include "common/SIMD.hpp"
#define FMT_STRING_ALIAS 1
#include "fmt/utfext.h"
#include <vector>
#include <cstdio>
#include <iostream>

using namespace std::string_view_literals;


constexpr auto txy = PARSE_PACK(U"abc"sv, 101, U"dev", 202, U"axy", 303, U"hellothere", 404, U"dontknowwho", 505);
constexpr auto t01 = txy(U"abc");
constexpr auto t02 = txy(U"dev");
constexpr auto t03 = txy(U"axy");
constexpr auto t04 = txy(U"hellothere");
constexpr auto t05 = txy(U"dontknowwho");
constexpr auto t00 = txy(U"notexists");


constexpr auto fkl = SWITCH_PACK(ConstEval, (U"abc", 101), (U"dev", 202), (U"axy", 303), (U"hellothere", 404), (U"dontknowwho", 505));
constexpr auto f01 = fkl(U"abc");
constexpr auto f02 = fkl(U"dev");
constexpr auto f03 = fkl(U"axy");
constexpr auto f04 = fkl(U"hellothere");
constexpr auto f05 = fkl(U"dontknowwho");
constexpr auto f00 = fkl(U"notexists");


//void TestStacktrace()
//{
//    fmt::basic_memory_buffer<char16_t> out;
//    ConsoleHelper console;
//    try
//    {
//        COMMON_THROWEX(BaseException, u"hey");
//    }
//    catch (const BaseException&)
//    {
//        try
//        {
//            COMMON_THROWEX(BaseException, u"hey");
//        }
//        catch (const BaseException & be2)
//        {
//            for (const auto kk : be2.Stack())
//            {
//                out.clear();
//                fmt::format_to(out, FMT_STRING(u"[{}]:[{:d}]\t{}\n"), kk.File, kk.Line, kk.Func);
//                console.Print(std::u16string_view(out.data(), out.size()));
//            }
//        }
//    }
//}


static __m128i CopyX(const uint8_t(&src)[16], const size_t size)
{
    if (size == 0)
        return _mm_setzero_si128();
    auto val = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src));
    if (size < 16)
    {
        const __m128i IdxMask = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
        const __m128i SizeMask = _mm_set1_epi8(static_cast<char>(size - 1)); // x,x,x,x
        const __m128i KeepMask = _mm_cmpgt_epi8(IdxMask, SizeMask); // 00,00,ff,ff
        const __m128i SignMask = _mm_set1_epi8(static_cast<char>(0x80)); // 00,00,80,80
        const __m128i ShufMask = _mm_or_si128(_mm_and_si128(SignMask, KeepMask), IdxMask); // x,x,8x,8x
        val = _mm_shuffle_epi8(val, ShufMask); // 1,2,0,0
    }
    return val;
}

void TestSSERead()
{
    for (uint8_t i = 0; i <= 16; ++i)
    {
        uint8_t src[16] = { 0 };
        uint8_t dst[16] = { 0 };
        for (uint8_t j = 0; j < i; ++j)
            src[j] = j;
        for (uint8_t j = i; j < 16; ++j)
            src[j] = 0xcc;
        for (uint8_t j = 0; j < 16; ++j)
            dst[j] = 0xff;
        const auto val = CopyX(src, i);
        _mm_store_si128(reinterpret_cast<__m128i*>(dst), val);
        printf("Test idx[%d]:\nsrc:\t", i);
        for (uint8_t j = 0; j < 16; ++j)
            printf(" %02x,", src[j]);
        printf("\ndst:\t");
        for (uint8_t j = 0; j < 16; ++j)
            printf(" %02x,", dst[j]);
        printf("\n\n");
    }
}


int main()
{

    printf("\n\n");

    //TestStacktrace();
    TestSSERead();
    printf("\n\n");

    getchar();
}

