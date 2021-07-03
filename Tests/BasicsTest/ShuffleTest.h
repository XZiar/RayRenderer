#pragma once
#include "pch.h"
#include "common/simd/SIMD128.hpp"
#if COMMON_ARCH_X86 && COMMON_SIMD_LV >= 100
#   include "common/simd/SIMD256.hpp"
#endif
#pragma message("Compiling ShuffleTest with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace shuftest
{

template<typename T, size_t N, typename U = std::decay_t<decltype(std::declval<T>().Val[0])>>
forcenoinline void ResultCheck(const T& data, const std::array<uint8_t, N>& poses) noexcept
{
    std::array<U, N> ref = {};
    for (size_t i = 0; i < N; ++i)
        ref[i] = poses[i];
    bool match = true;
    for (size_t i = 0; i < N; ++i)
        match &= (data.Val[i] == ref[i]);
    if (!match)
    {
        EXPECT_THAT(data.Val, testing::ElementsAreArray(ref));
    }
}

template<size_t N, typename T, uint64_t Val>
forceinline auto GenerateShuffle(const T& data)
{
    if constexpr (N == 2)
    {
        constexpr uint8_t Lo0 = Val % 2, Hi1 = Val / 2;
        return data.template Shuffle<Lo0, Hi1>();
    }
    else if constexpr (N == 4)
    {
        constexpr uint8_t Lo0 = Val % 4, Lo1 = (Val / 4) % 4, Lo2 = (Val / 16) % 4, Hi3 = (Val / 64) % 4;
        return data.template Shuffle<Lo0, Lo1, Lo2, Hi3>();
    }
    else if constexpr (N == 8)
    {
        constexpr uint8_t Lo0 = Val % 8, Lo1 = (Val / 8) % 8, Lo2 = (Val / 64) % 8, Lo3 = (Val / 512) % 8,
            Lo4 = (Val / 4096) % 8, Lo5 = (Val / 32768) % 8, Lo6 = (Val / 262144) % 8, Hi7 = (Val / 2097152) % 8;
        return data.template Shuffle<Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Hi7>();
    }
    else
    {
        static_assert(common::AlwaysTrue<T>, "only support 2/4/8 members for now");
    }
}

template<typename T, uint64_t Val>
forceinline void RunShuffle(const T& data)
{
    constexpr auto N = T::Count;
    constexpr auto Poses = GeneratePoses<N>(Val);
    const auto output = GenerateShuffle<N, T, Val>(data);
    ResultCheck(output, Poses);
}

template<typename T, uint64_t IdxBegin, uint64_t IdxEnd>
void TestShuffle(const T& data)
{
    if constexpr (IdxBegin == IdxEnd)
        RunShuffle<T, IdxBegin>(data);
    else
    {
        TestShuffle<T, IdxBegin, IdxBegin + (IdxEnd - IdxBegin) / 2>(data);
        TestShuffle<T, IdxBegin + (IdxEnd - IdxBegin) / 2 + 1, IdxEnd>(data);
    }
}


template<size_t N, typename T>
forceinline auto GenerateShuffleVar(const T& data, const std::array<uint8_t, N>& poses)
{
    if constexpr (N == 2)
    {
        return data.Shuffle(poses[0], poses[1]);
    }
    else if constexpr (N == 4)
    {
        return data.Shuffle(poses[0], poses[1], poses[2], poses[3]);
    }
    else if constexpr (N == 8)
    {
        return data.Shuffle(poses[0], poses[1], poses[2], poses[3], poses[4], poses[5], poses[6], poses[7]);
    }
    else
    {
        static_assert(common::AlwaysTrue<T>, "only support 2/4/8 members for now");
    }
}

template<typename T>
forceinline void RunShuffleVar(const T& data, uint64_t val)
{
    constexpr auto N = T::Count;
    const auto& poses = PosesHolder<N>::Poses[val];
    const auto output = GenerateShuffleVar<N>(data, poses);
    ResultCheck(output, poses);
}

template<typename T>
void TestShuffleVar(const T& data, uint64_t idxBegin, uint64_t idxEnd)
{
    if (idxBegin == idxEnd)
        RunShuffleVar<T>(data, idxBegin);
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