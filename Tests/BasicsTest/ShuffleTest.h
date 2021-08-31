#pragma once
#include "pch.h"
#pragma message("Compiling ShuffleTest with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace shuftest
{
using namespace std::string_literals;
using namespace std::string_view_literals;


static constexpr std::string_view NUM64 = " 0 1 2 3 4 5 6 7 8 9101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263"sv;

template<size_t N>
static std::string GenerateMatchStr(std::string_view op, const std::array<uint8_t, N>& ref)
{
    std::string ret; ret.reserve(256);
    ret.append(op).append(" to [");
    bool isFirst = true;
    for (const auto i : ref)
    {
        if (!isFirst)
            ret.append(", ");
        ret.append(NUM64.substr(i * 2, 2));
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

MATCHER_P(MatchShuffle, ref, GenerateMatchStr("Shuffle", ref))
{
    [[maybe_unused]] const auto& tmp = result_listener;
    constexpr size_t N = std::tuple_size_v<std::decay_t<decltype(ref)>>;
    return CheckMatch(arg, ref, std::make_index_sequence<N>{});
}

template<typename T, size_t N>
forcenoinline void ResultCheck(const T& data, const std::array<uint8_t, N>& poses) noexcept
{
    if (!CheckMatch(data.Val, poses, std::make_index_sequence<N>{}))
    {
        EXPECT_THAT(data.Val, MatchShuffle(poses));
    }
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
        static_assert(AlwaysTrue<T>, "only support 2/4/8 members for now");
    }
    ResultCheck(output, Poses);
}

template<typename T, uint64_t IdxBegin, uint16_t... Vals>
forceinline void TestShuffleBatch(const T& data, std::integer_sequence<uint16_t, Vals...>)
{
    (..., RunShuffle<T, IdxBegin + Vals>(data));
}

template<typename T, uint64_t IdxBegin, uint64_t IdxEnd>
forceinline void TestShuffle(const T& data)
{
    if constexpr (IdxEnd - IdxBegin <= 256)
    {
        TestShuffleBatch<T, IdxBegin>(data, std::make_integer_sequence<uint16_t, IdxEnd - IdxBegin>{});
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
        static_assert(AlwaysTrue<T>, "only support 2/4/8 members for now");
    }
    ResultCheck(output, poses);
}

template<typename T, size_t... Idxes>
static constexpr T GenerateT(std::index_sequence<Idxes...>, size_t offset = 0)
{
    static_assert(sizeof...(Idxes) == T::Count);
    using ArgType = std::decay_t<decltype(std::declval<T&>().Val[0])>;
    return T(static_cast<ArgType>(Idxes + offset)...);
}


template<typename T>
class ShuffleTest : public SIMDFixture
{
public:
    static constexpr auto TestSuite = "Shuffle";
    void TestBody() override
    {
        const auto data = GenerateT<T>(std::make_index_sequence<T::Count>{});
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
        const auto data = GenerateT<T>(std::make_index_sequence<T::Count>{});
        constexpr auto Count = Pow<T::Count, T::Count>();
        for (uint64_t i = 0; i < Count; ++i)
            RunShuffleVar<T>(data, i);
    }
};



MATCHER_P(MatchZip, ref, GenerateMatchStr("Zip", ref))
{
    [[maybe_unused]] const auto& tmp = result_listener;
    constexpr size_t N = std::tuple_size_v<std::decay_t<decltype(ref)>>;
    return CheckMatch(arg, ref, std::make_index_sequence<N>{});
}

template<typename T, bool ZipLane = false>
class ZipTest : public SIMDFixture
{
private:
public:
    static constexpr auto GetRef() noexcept
    {
        constexpr size_t N = T::Count;
        std::array<uint8_t, N * 2> ref = { 0 };
        if constexpr (ZipLane)
        {
            constexpr size_t M = N / 4;
            constexpr size_t offsets[] = { M * 0,  M * 2, M * 1, M * 3 };
            for (size_t i = 0; i < N; ++i)
            {
                const auto j = (i % M) + offsets[i / M];
                ref[i * 2 + 0] = static_cast<uint8_t>(j);
                ref[i * 2 + 1] = static_cast<uint8_t>(j + N);
            }
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                ref[i * 2 + 0] = static_cast<uint8_t>(i);
                ref[i * 2 + 1] = static_cast<uint8_t>(i + N);
            }
        }
        return ref;
    }
public:
    static constexpr auto TestSuite = ZipLane ? "SIMDZipLane" : "SIMDZip";
    void TestBody() override
    {
        using U = typename T::EleType;
        const auto data0 = GenerateT<T>(std::make_index_sequence<T::Count>{}, 0);
        const auto data1 = GenerateT<T>(std::make_index_sequence<T::Count>{}, T::Count);
        U out[T::Count * 2] = { 0 };
        constexpr auto ref = GetRef();
        if constexpr (ZipLane)
            data0.ZipLoLane(data1).Save(out), data0.ZipHiLane(data1).Save(out + T::Count);
        else
            data0.ZipLo(data1).Save(out), data0.ZipHi(data1).Save(out + T::Count);
        EXPECT_THAT(out, MatchZip(ref));
    }
};
#define RegisterSIMDZipTestItem(r, lv, i, type)  RegisterSIMDTest(type, lv, shuftest::ZipTest<type, false>);
#define RegisterSIMDZipTest(lv, ...)  BOOST_PP_SEQ_FOR_EACH_I(RegisterSIMDZipTestItem, lv, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define RegisterSIMDZipLaneTestItem(r, lv, i, type)  RegisterSIMDTest(type, lv, shuftest::ZipTest<type, true>);
#define RegisterSIMDZipLaneTest(lv, ...)  BOOST_PP_SEQ_FOR_EACH_I(RegisterSIMDZipLaneTestItem, lv, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))


}