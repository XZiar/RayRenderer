#pragma once
#include "common/SIMD128.hpp"
#if COMMON_SIMD_LV >= 100
#   include "common/SIMD256.hpp"
#endif
#pragma message("Compiling ShuffleTest with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace shuftest
{

template<typename T, typename U, size_t... Indexes, size_t N>
bool RealCheck(const T& data, std::index_sequence<Indexes...>, const U(&poses)[N])
{
    return (... && (data.Val[Indexes] == poses[Indexes]));
}

template<typename T, uint32_t PosCount, uint64_t N, uint8_t... Poses>
void GenerateShuffle(const T& data)
{
    if constexpr (PosCount == 0)
    {
        using ArgType = std::remove_reference_t<decltype(*data.Val)>;
        const auto output = data.template Shuffle<Poses...>();
        const ArgType poses[sizeof...(Poses)]{ Poses... };
        if (!RealCheck(output, std::make_index_sequence<sizeof...(Poses)>{}, poses))
        {
            EXPECT_THAT(output.Val, testing::ContainerEq(poses));
        }
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
        if (!RealCheck(output, std::make_index_sequence<sizeof...(Poses)>{}, posArray))
        {
            EXPECT_THAT(output.Val, testing::ContainerEq(posArray));
        }
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


template<typename T>
class ShuffleTest : public SIMDFixture
{
public:
    static constexpr auto TestSuite = "Shuffle";
    void TestBody() override
    {
        const auto data = GenerateT<T>();
        TestShuffle<T, 0, Pow<T::Count, T::Count>() - 1>(data);
    }
};


template<typename T>
class ShuffleVarTest : public SIMDFixture
{
public:
    static constexpr auto TestSuite = "ShuffleVar";
    void TestBody() override
    {
        const auto data = GenerateT<T>();
        TestShuffleVar<T>(data, 0, Pow<T::Count, T::Count>() - 1);
    }
};


}