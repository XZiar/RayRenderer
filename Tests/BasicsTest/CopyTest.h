#include "common/CopyEx.hpp"


namespace copytest
{
using namespace common::copy;


struct CopyTestHelper
{
    static uint32_t GetARand();
    static const std::vector<uint32_t>& GetCopyRanges();
    template<typename T>
    static const std::vector<T>& GetRandomSources();
    template<typename Src, typename Dst>
    static const std::vector<Dst>& GetConvertData();
};

#define GEN_BASE_NAME(FROM, TO) u##FROM##_u##TO##_
#define GEN_TEST_NAME(FROM, TO) PPCAT(GEN_BASE_NAME(FROM, TO), COMMON_SIMD_INTRIN)

#define TEST_CONV(FROM, TO)                                                             \
TEST(Copy, GEN_TEST_NAME(FROM, TO))                                                     \
{                                                                                       \
    using TFrom = uint##FROM##_t;                                                       \
    using TTo   = uint##TO##_t  ;                                                       \
    const auto& orig = CopyTestHelper::GetRandomSources<TFrom>();                       \
    const auto& real = CopyTestHelper::GetConvertData<TFrom, TTo>();                    \
    const auto total = orig.size();                                                     \
    for (const auto len : CopyTestHelper::GetCopyRanges())                              \
    {                                                                                   \
        for (uint8_t i = 4; i--;)                                                       \
        {                                                                               \
            const auto start = CopyTestHelper::GetARand() % (total - len);              \
            std::vector<TTo> dest(len);                                                 \
            CopyLittleEndian(dest.data(), dest.size(), orig.data() + start, len);       \
            std::vector<TTo> truth(real.data() + start, real.data() + start + len);     \
            EXPECT_THAT(dest, testing::ContainerEq(truth));                             \
        }                                                                               \
    }                                                                                   \
}                                                                                       \


//
//TEST(Copy, u32_u32)
//{
//    const auto& orig = GetRandom32();
//    const auto total = orig.size();
//    for (const auto len : TestSizes)
//    {
//        for (uint8_t i = 4; i--;)
//        {
//            const auto start = GetRanEng()() % (total - len);
//            const auto src = FromIterable(orig).Skip(start).Take(len).ToVector();
//            std::vector<uint32_t> dest(src.size());
//            CopyLittleEndian(dest.data(), dest.size(), src.data(), src.size());
//            EXPECT_THAT(dest, testing::ContainerEq(src));
//        }
//    }
//}

//class Copy : public testing::TestWithParam<std::string>
//{
//};

}
//
//TEST_P(Copy, u32_u32)
//{
//    const auto& orig = GetRandom32();
//    const auto total = orig.size();
//    for (const auto len : TestSizes)
//    {
//        for (uint8_t i = 4; i--;)
//        {
//            const auto start = GetRanEng()() % (total - len);
//            const auto src = FromIterable(orig).Skip(start).Take(len).ToVector();
//            std::vector<uint32_t> dest(src.size());
//            CopyLittleEndian(dest.data(), dest.size(), src.data(), src.size());
//            EXPECT_THAT(dest, testing::ContainerEq(src));
//        }
//    }
//}
//
//TEST_P(Copy, u32_u16)
//{
//    const auto& orig = GetRandom32();
//    const auto total = orig.size();
//    for (const auto len : TestSizes)
//    {
//        for (uint8_t i = 4; i--;)
//        {
//            const auto start = GetRanEng()() % (total - len);
//            const auto src = FromIterable(orig).Skip(start).Take(len).ToVector();
//            const auto src2 = FromIterable(src)
//                .Select([](const auto i) { return static_cast<uint16_t>(i); }).ToVector();
//            std::vector<uint16_t> dest(src.size());
//            CopyLittleEndian(dest.data(), dest.size(), src.data(), src.size());
//            EXPECT_THAT(dest, testing::ContainerEq(src2));
//        }
//    }
//}
//
//TEST_P(Copy, u32_u8)
//{
//    const auto& orig = GetRandom32();
//    const auto total = orig.size();
//    for (const auto len : TestSizes)
//    {
//        for (uint8_t i = 4; i--;)
//        {
//            const auto start = GetRanEng()() % (total - len);
//            const auto src = FromIterable(orig).Skip(start).Take(len).ToVector();
//            const auto src2 = FromIterable(src)
//                .Select([](const auto i) { return static_cast<uint8_t>(i); }).ToVector();
//            std::vector<uint8_t> dest(src.size());
//            CopyLittleEndian(dest.data(), dest.size(), src.data(), src.size());
//            EXPECT_THAT(dest, testing::ContainerEq(src2));
//        }
//    }
//}
//
//TEST_P(Copy, u16_u8)
//{
//    const auto& orig = GetRandom16();
//    const auto total = orig.size();
//    for (const auto len : TestSizes)
//    {
//        for (uint8_t i = 4; i--;)
//        {
//            const auto start = GetRanEng()() % (total - len);
//            const auto src = FromIterable(orig).Skip(start).Take(len).ToVector();
//            const auto src2 = FromIterable(src)
//                .Select([](const auto i) { return static_cast<uint8_t>(i); }).ToVector();
//            std::vector<uint8_t> dest(src.size());
//            CopyLittleEndian(dest.data(), dest.size(), src.data(), src.size());
//            EXPECT_THAT(dest, testing::ContainerEq(src2));
//        }
//    }
//}

