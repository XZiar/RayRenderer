#pragma once
#include "pch.h"
#pragma message("Compiling ShuffleTest with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace shuftest
{
using namespace std::string_literals;
using namespace std::string_view_literals;

static constexpr std::string_view NUM16[] = 
{
    "0"sv, "1"sv,  "2"sv,  "3"sv,  "4"sv,  "5"sv,  "6"sv,  "7"sv, 
    "8"sv, "9"sv, "10"sv, "11"sv, "12"sv, "13"sv, "14"sv, "15"sv,
};
template<size_t N>
static std::string GenerateMatchStr(const std::array<uint8_t, N>& ref)
{
    std::string ret; ret.reserve(256);
    ret.append("Shuffle to [");
    bool isFirst = true;
    for (const auto i : ref)
    {
        if (!isFirst)
            ret.append(", ");
        ret.append(NUM16[i]);
        isFirst = false;
    }
    ret.append("]");
    return ret;
}

template<typename T, size_t N, size_t... Idx>
forceinline bool CheckMatch(const T(&arg)[N], const std::array<uint8_t, N>& ref, std::index_sequence<Idx...>)
{
    return (... && (arg[Idx] == ref[Idx]));
}

MATCHER_P(MatchShuffle, ref, GenerateMatchStr(ref))
{
    [[maybe_unused]] const auto& tmp = result_listener;
    constexpr size_t N = std::tuple_size_v<std::decay_t<decltype(ref)>>;
    return CheckMatch(arg, ref, std::make_index_sequence<N>{});
}

template<typename T, size_t N>
forcenoinline void ResultCheck(const T& data, const std::array<uint8_t, N>& poses) noexcept
{
    EXPECT_THAT(data.Val, MatchShuffle(poses));
}


template<typename T, uint64_t Val>
forceinline void RunShuffle(const T& data)
{
    constexpr auto N = T::Count;
    constexpr auto Poses = GeneratePoses<N>(Val);
    T output;
    if constexpr (N == 2)
    {
        constexpr uint8_t Lo0 = Val % 2, Hi1 = Val / 2;
        output = data.template Shuffle<Lo0, Hi1>();
    }
    else if constexpr (N == 4)
    {
        constexpr uint8_t Lo0 = Val % 4, Lo1 = (Val / 4) % 4, Lo2 = (Val / 16) % 4, Hi3 = (Val / 64) % 4;
        output = data.template Shuffle<Lo0, Lo1, Lo2, Hi3>();
    }
    else if constexpr (N == 8)
    {
        constexpr uint8_t Lo0 = Val % 8, Lo1 = (Val / 8) % 8, Lo2 = (Val / 64) % 8, Lo3 = (Val / 512) % 8,
            Lo4 = (Val / 4096) % 8, Lo5 = (Val / 32768) % 8, Lo6 = (Val / 262144) % 8, Hi7 = (Val / 2097152) % 8;
        output = data.template Shuffle<Lo0, Lo1, Lo2, Lo3, Lo4, Lo5, Lo6, Hi7>();
    }
    else
    {
        static_assert(common::AlwaysTrue<T>, "only support 2/4/8 members for now");
    }
    ResultCheck(output, Poses);
}

template<typename T, uint64_t IdxBegin, uint64_t... Vals>
void TestShuffleBatch(const T& data, std::index_sequence<Vals...>)
{
    (..., RunShuffle<T, IdxBegin + Vals>(data));
}

template<typename T, uint64_t IdxBegin, uint64_t IdxEnd>
void TestShuffle(const T& data)
{
    if constexpr (IdxEnd - IdxBegin <= 256)
    {
        TestShuffleBatch<T, IdxBegin>(data, std::make_index_sequence<IdxEnd - IdxBegin>{});
    }
    else
    {
        TestShuffle<T, IdxBegin, IdxBegin + (IdxEnd - IdxBegin) / 2>(data);
        TestShuffle<T, IdxBegin + (IdxEnd - IdxBegin) / 2 + 1, IdxEnd>(data);
    }
}


template<typename T>
forceinline void RunShuffleVar(const T& data, uint64_t val)
{
    constexpr auto N = T::Count;
    const auto& poses = PosesHolder<N>::Poses[val];
    T output;
    if constexpr (N == 2)
    {
        output = data.Shuffle(poses[0], poses[1]);
    }
    else if constexpr (N == 4)
    {
        output = data.Shuffle(poses[0], poses[1], poses[2], poses[3]);
    }
    else if constexpr (N == 8)
    {
        output = data.Shuffle(poses[0], poses[1], poses[2], poses[3], poses[4], poses[5], poses[6], poses[7]);
    }
    else
    {
        static_assert(common::AlwaysTrue<T>, "only support 2/4/8 members for now");
    }
    ResultCheck(output, poses);
}

template<typename T, size_t... Idxes>
static constexpr T GenerateT()
{
    using ArgType = std::decay_t<decltype(std::declval<T&>().Val[0])>;
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
        constexpr auto Count = Pow<T::Count, T::Count>();
        for (uint64_t i = 0; i < Count; ++i)
            RunShuffleVar<T>(data, i);
    }
};


}