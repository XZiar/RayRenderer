#pragma once
#include "common/SIMD128.hpp"
#include "common/SIMD256.hpp"
#pragma message("Compiling ShuffleTest with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace shuftest
{


template<typename T, uint32_t PosCount, uint64_t N, uint8_t... Poses>
void GenerateShuffle(const T& data)
{
    if constexpr (PosCount == 0)
    {
        using ArgType = std::remove_reference_t<decltype(*data.Val)>;
        const auto output = data.template Shuffle<Poses...>();
        const ArgType poses[sizeof...(Poses)]{ Poses... };
        EXPECT_THAT(output.Val, testing::ContainerEq(poses));
    }
    else
    {
        constexpr uint8_t PosRange = T::Count;
        GenerateShuffle<T, PosCount - 1, N / PosRange, Poses..., (uint8_t)(N % PosRange)>(data);
    }
}

template<typename T, size_t IdxBegin, size_t IdxEnd>
void TestShuffle(const T& data)
{
    if constexpr (IdxBegin == IdxEnd)
        GenerateShuffle<T, T::Count, IdxBegin>(data);
    else
    {
        TestShuffle<T, IdxBegin, IdxBegin + (IdxEnd - IdxBegin) / 2>(data);
        TestShuffle<T, IdxBegin + (IdxEnd - IdxBegin) / 2 + 1, IdxEnd>(data);
    }
}

template<typename T, uint32_t PosCount, typename... Poses>
void GenerateShuffleVar(const T& data, [[maybe_unused]] const uint64_t N, const Poses... poses)
{
    if constexpr (PosCount == 0)
    {
        using ArgType = std::remove_reference_t<decltype(*data.Val)>;
        const auto output = data.Shuffle(poses...);
        const ArgType posArray[sizeof...(Poses)]{ static_cast<ArgType>(poses)... };
        EXPECT_THAT(output.Val, testing::ContainerEq(posArray));
    }
    else
    {
        constexpr uint8_t PosRange = T::Count;
        GenerateShuffleVar<T, PosCount - 1>(data, N / PosRange, poses..., (uint8_t)(N % PosRange));
    }
}

template<typename T>
void TestShuffleVar(const T& data, const uint64_t idxBegin, const uint64_t idxEnd)
{
    if (idxBegin == idxEnd)
        GenerateShuffleVar<T, T::Count>(data, idxBegin);
    else
    {
        TestShuffleVar<T>(data, idxBegin, idxBegin + (idxEnd - idxBegin) / 2);
        TestShuffleVar<T>(data, idxBegin + (idxEnd - idxBegin) / 2 + 1, idxEnd);
    }
}

template<typename T, size_t... Idxes>
constexpr T GenerateT()
{
    using ArgType = std::remove_reference_t<decltype(*std::declval<T&>().Val)>;
    constexpr size_t ArgCount = sizeof...(Idxes);
    if constexpr (ArgCount == T::Count)
        return T(static_cast<ArgType>(Idxes)...);
    else
        return GenerateT<T, Idxes..., ArgCount>();
}


#define SHUF_GEN_BASE_NAME(TYPE) TYPE##_
#define SHUF_GEN_TEST_NAME(TYPE) PPCAT(SHUF_GEN_BASE_NAME(TYPE), COMMON_SIMD_INTRIN)

#define GEN_SHUF_TEST(TYPE)                                                             \
TEST(Shuffle, SHUF_GEN_TEST_NAME(TYPE))                                                 \
{                                                                                       \
    auto data = GenerateT<TYPE>();                                                      \
    TestShuffle<TYPE, 0, Pow<TYPE::Count, TYPE::Count>() - 1>(data);                    \
}                                                                                       \

#define SHUF_GEN_BASE_NAME2(TYPE) TYPE##_Var_
#define SHUF_GEN_TEST_NAME2(TYPE) PPCAT(SHUF_GEN_BASE_NAME2(TYPE), COMMON_SIMD_INTRIN)

#define GEN_SHUF_VAR_TEST(TYPE)                                                         \
TEST(Shuffle, SHUF_GEN_TEST_NAME2(TYPE))                                                \
{                                                                                       \
    auto data = GenerateT<TYPE>();                                                      \
    TestShuffleVar<TYPE>(data, 0, Pow<TYPE::Count, TYPE::Count>() - 1);                 \
}                                                                                       \


}