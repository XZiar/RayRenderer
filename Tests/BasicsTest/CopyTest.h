#pragma once
#include "common/CopyEx.hpp"
#pragma message("Compiling CopyTest with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace copytest
{
using namespace common::copy;


struct CopyTestHelper
{
    static const std::vector<uint32_t>& GetCopyRanges();
    static const std::vector<uint32_t>& GetMemSetRanges();
    template<typename T>
    static const std::vector<T>& GetRandomSources();
    template<typename Src, typename Dst>
    static const std::vector<Dst>& GetConvertData();
};

#define COPY_GEN_BASE_NAME(FROM, TO) u##FROM##_u##TO##_
#define COPY_GEN_TEST_NAME(FROM, TO) PPCAT(COPY_GEN_BASE_NAME(FROM, TO), COMMON_SIMD_INTRIN)

#define GEN_COPY_TEST(FROM, TO)                                                         \
TEST(Copy, COPY_GEN_TEST_NAME(FROM, TO))                                                \
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
            const auto start = GetARand() % (total - len);                              \
            std::vector<TTo> dest(len);                                                 \
            CopyLittleEndian(dest.data(), dest.size(), orig.data() + start, len);       \
            std::vector<TTo> truth(real.data() + start, real.data() + start + len);     \
            EXPECT_THAT(dest, testing::ContainerEq(truth));                             \
        }                                                                               \
    }                                                                                   \
}                                                                                       \


#define BCAST_GEN_BASE_NAME(TYPE) u##TYPE##_
#define BCAST_GEN_TEST_NAME(TYPE) PPCAT(BCAST_GEN_BASE_NAME(TYPE), COMMON_SIMD_INTRIN)

#define GEN_BCAST_TEST(TYPE)                                                            \
TEST(Broadcast, BCAST_GEN_TEST_NAME(TYPE))                                              \
{                                                                                       \
    using T = uint##TYPE##_t;                                                           \
    for (const auto len : CopyTestHelper::GetMemSetRanges())                            \
    {                                                                                   \
        for (uint8_t i = 4; i--;)                                                       \
        {                                                                               \
            const T val = static_cast<T>(GetARand());                                   \
            const std::vector<T> truth(len, val);                                       \
                  std::vector<T> raw(len, 0);                                           \
            BroadcastMany(raw.data(), raw.size(), val, len);                            \
            EXPECT_THAT(raw, testing::ContainerEq(truth));                              \
        }                                                                               \
    }                                                                                   \
}                                                                                       \


}
