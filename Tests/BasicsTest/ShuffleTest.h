#pragma once
#include "SIMDRely.h"
#pragma message("Compiling ShuffleTest with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace shuftest
{
using namespace std::string_literals;
using namespace std::string_view_literals;


static constexpr std::string_view NUM64 = " 0 1 2 3 4 5 6 7 8 9101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263"sv;

template<size_t N>
inline std::string GenerateMatchStr(std::string_view op, const std::array<uint8_t, N>& ref)
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

MATCHER_P(FailShuffle, ref, GenerateMatchStr("Shuffle", ref))
{
    [[maybe_unused]] const auto& tmp0 = arg;
    [[maybe_unused]] const auto& tmp1 = result_listener;
    return false;
}

template<typename T, size_t N>
forcenoinline void ShuffleCheck(const T& data, const std::array<uint8_t, N>& poses) noexcept
{
    if (!CheckMatch(data.Val, poses, std::make_index_sequence<N>{}))
    {
        EXPECT_THAT(data.Val, FailShuffle(poses));
    }
}


template<typename T, size_t... Idxes>
inline constexpr T GenerateT(std::index_sequence<Idxes...>, size_t offset = 0)
{
    static_assert(sizeof...(Idxes) == T::Count);
    using ArgType = std::decay_t<decltype(std::declval<T&>().Val[0])>;
    return T(static_cast<ArgType>(Idxes + offset)...);
}

template<size_t N>
struct IdxesHolder
{
    static constexpr uint64_t Mask = (uint64_t(1) << N) - 1;
    uint64_t Idx;
    constexpr uint8_t At(uint16_t offset, size_t i) const noexcept
    {
        return static_cast<uint8_t>(((Idx + offset) >> (N * i)) & Mask);
    }
};

template<typename T>
class ShuffleTest : public SIMDFixture
{
private:
    static constexpr auto N = T::Count;
    static constexpr uint32_t BatchSize = 256;
    static constexpr uint64_t InstCount = Pow<N, N>();
    static_assert(InstCount < BatchSize || InstCount % BatchSize == 0);
    static constexpr uint32_t BatchCount = static_cast<uint32_t>(std::max<uint64_t>(1, InstCount / BatchSize));
    const T Original = GenerateT<T>(std::make_index_sequence<N>{});

    template<uint64_t IdxBegin, uint16_t... Vals>
    void TestShuffle(std::integer_sequence<uint16_t, Vals...>) const
    {
        if constexpr (N == 2)
        {
            constexpr IdxesHolder<1> Holder = { IdxBegin };
            T output[] = { Original.template Shuffle<Holder.At(Vals, 0), Holder.At(Vals, 1)>()... };
            uint64_t idx = IdxBegin;
            for (const auto& out : output)
                ShuffleCheck(out, PosesHolder<N>::Poses[idx++]);
        }
        else if constexpr (N == 4)
        {
            constexpr IdxesHolder<2> Holder = { IdxBegin };
            T output[] = { Original.template Shuffle<Holder.At(Vals, 0), Holder.At(Vals, 1), Holder.At(Vals, 2), Holder.At(Vals, 3)>()... };
            uint64_t idx = IdxBegin;
            for (const auto& out : output)
                ShuffleCheck(out, PosesHolder<N>::Poses[idx++]);
        }
        else if constexpr (N == 8)
        {
            constexpr IdxesHolder<3> Holder = { IdxBegin };
            T output[] = { Original.template Shuffle<Holder.At(Vals, 0), Holder.At(Vals, 1), Holder.At(Vals, 2), Holder.At(Vals, 3),
                Holder.At(Vals, 4), Holder.At(Vals, 5), Holder.At(Vals, 6), Holder.At(Vals, 7)>()... };
            uint64_t idx = IdxBegin;
            for (const auto& out : output)
                ShuffleCheck(out, PosesHolder<N>::Poses[idx++]);
        }
        else
        {
            static_assert(!AlwaysTrue<T>, "only support 2/4/8 members for now");
        }
    }
    template<uint32_t BatchBegin, uint32_t BatchEnd>
    void TestShuffle() const
    {
        if constexpr (BatchBegin == BatchEnd)
        {
            constexpr uint32_t IdxBegin = BatchBegin * BatchSize;
            constexpr uint32_t IdxCount = std::min(static_cast<uint32_t>(InstCount - IdxBegin), BatchSize);
            TestShuffle<IdxBegin>(std::make_integer_sequence<uint16_t, IdxCount>{});
        }
        else
        {
            TestShuffle<BatchBegin, BatchBegin + (BatchEnd - BatchBegin) / 2>();
            TestShuffle<BatchBegin + (BatchEnd - BatchBegin) / 2 + 1, BatchEnd>();
        }
    }
public:
    static constexpr auto TestSuite = "Shuffle";
    void TestBody() override
    {
        TestShuffle<0, BatchCount - 1>();
    }
};


template<typename T>
class ShuffleVarTest : public SIMDFixture
{
private:
    static void RunShuffleVar(const T& data, uint64_t val)
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
        ShuffleCheck(output, poses);
    }
public:
    static constexpr auto TestSuite = "ShuffleVar";
    void TestBody() override
    {
        const auto data = GenerateT<T>(std::make_index_sequence<T::Count>{});
        constexpr auto Count = Pow<T::Count, T::Count>();
        for (uint64_t i = 0; i < Count; ++i)
            RunShuffleVar(data, i);
    }
};


MATCHER_P(MatchZip, ref, GenerateMatchStr("Zip", ref))
{
    [[maybe_unused]] const auto& tmp = result_listener;
    constexpr size_t N = std::tuple_size_v<std::decay_t<decltype(ref)>>;
    return CheckMatch(arg, ref, std::make_index_sequence<N>{});
}

template<typename T>
class ZipLHTest : public SIMDFixture
{
private:
public:
    static constexpr auto GetRef() noexcept
    {
        constexpr size_t N = T::Count;
        std::array<uint8_t, N * 2> ref = { 0 };
        for (size_t i = 0; i < N; ++i)
        {
            ref[i * 2 + 0] = static_cast<uint8_t>(i);
            ref[i * 2 + 1] = static_cast<uint8_t>(i + N);
        }
        return ref;
    }
public:
    static constexpr auto TestSuite = "SIMDZipLH";
    void TestBody() override
    {
        using U = typename T::EleType;
        const auto data0 = GenerateT<T>(std::make_index_sequence<T::Count>{}, 0);
        const auto data1 = GenerateT<T>(std::make_index_sequence<T::Count>{}, T::Count);
        U out[T::Count * 2] = { 0 };
        constexpr auto ref = GetRef();
        data0.ZipLo(data1).Save(out), data0.ZipHi(data1).Save(out + T::Count);
        EXPECT_THAT(out, MatchZip(ref));
    }
};
#define RegisterSIMDZipTestItem(r, lv, i, type)  RegisterSIMDTest(type, lv, shuftest::ZipLHTest<type>);
#define RegisterSIMDZipTest(lv, ...)  BOOST_PP_SEQ_FOR_EACH_I(RegisterSIMDZipTestItem, lv, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
template<typename T>
class ZipOETest : public SIMDFixture
{
private:
public:
    static constexpr auto GetRef() noexcept
    {
        constexpr size_t N = T::Count;
        std::array<uint8_t, N * 2> ref = { 0 };
        for (size_t i = 0; i < N / 2; ++i)
        {
            ref[i * 2 + 0 + 0] = static_cast<uint8_t>(i * 2 + 0 + 0);
            ref[i * 2 + N + 0] = static_cast<uint8_t>(i * 2 + 0 + 1);
            ref[i * 2 + 0 + 1] = static_cast<uint8_t>(i * 2 + N + 0);
            ref[i * 2 + N + 1] = static_cast<uint8_t>(i * 2 + N + 1);
        }
        return ref;
    }
public:
    static constexpr auto TestSuite = "SIMDZipOE";
    void TestBody() override
    {
        using U = typename T::EleType;
        const auto data0 = GenerateT<T>(std::make_index_sequence<T::Count>{}, 0);
        const auto data1 = GenerateT<T>(std::make_index_sequence<T::Count>{}, T::Count);
        U out[T::Count * 2] = { 0 };
        constexpr auto ref = GetRef();
        data0.ZipOdd(data1).Save(out), data0.ZipEven(data1).Save(out + T::Count);
        EXPECT_THAT(out, MatchZip(ref));
    }
};
#define RegisterSIMDZipOETestItem(r, lv, i, type)  RegisterSIMDTest(type, lv, shuftest::ZipOETest<type>);
#define RegisterSIMDZipOETest(lv, ...)  BOOST_PP_SEQ_FOR_EACH_I(RegisterSIMDZipOETestItem, lv, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
template<typename T>
class ZipLaneTest : public SIMDFixture
{
private:
public:
    static constexpr auto GetRef() noexcept
    {
        constexpr size_t N = T::Count;
        std::array<uint8_t, N * 2> ref = { 0 };
        constexpr size_t M = N / 4;
        constexpr size_t offsets[] = { M * 0,  M * 2, M * 1, M * 3 };
        for (size_t i = 0; i < N; ++i)
        {
            const auto j = (i % M) + offsets[i / M];
            ref[i * 2 + 0] = static_cast<uint8_t>(j);
            ref[i * 2 + 1] = static_cast<uint8_t>(j + N);
        }
        return ref;
    }
public:
    static constexpr auto TestSuite = "SIMDZipLane";
    void TestBody() override
    {
        using U = typename T::EleType;
        const auto data0 = GenerateT<T>(std::make_index_sequence<T::Count>{}, 0);
        const auto data1 = GenerateT<T>(std::make_index_sequence<T::Count>{}, T::Count);
        U out[T::Count * 2] = { 0 };
        constexpr auto ref = GetRef();
        data0.ZipLoLane(data1).Save(out), data0.ZipHiLane(data1).Save(out + T::Count);
        EXPECT_THAT(out, MatchZip(ref));
    }
};
#define RegisterSIMDZipLaneTestItem(r, lv, i, type)  RegisterSIMDTest(type, lv, shuftest::ZipLaneTest<type>);
#define RegisterSIMDZipLaneTest(lv, ...)  BOOST_PP_SEQ_FOR_EACH_I(RegisterSIMDZipLaneTestItem, lv, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))


template<typename T, size_t N, size_t... Idx>
forceinline bool CheckMatchSame(const T(&arg)[N], const uint8_t n, std::index_sequence<Idx...>)
{
    return (... && (arg[Idx] == n));
}
template<typename T, size_t N>
forceinline bool CheckMatchSame(const T(&arg)[N], const uint8_t n)
{
    return CheckMatchSame(arg, n, std::make_index_sequence<N>{});
}
template<typename T, size_t N, size_t... Idx>
forceinline bool CheckMatchSame2(const T(&arg)[N], const uint8_t n, std::index_sequence<Idx...>)
{
    return (... && (arg[Idx] == n)) && (... && (arg[N / 2 + Idx] == static_cast<uint8_t>(N / 2 + n)));
}
template<typename T, size_t N>
forceinline bool CheckMatchSame2(const T(&arg)[N], const uint8_t n)
{
    return CheckMatchSame2(arg, n, std::make_index_sequence<N / 2>{});
}
inline std::string GenerateBroadcastToStr(uint8_t idx)
{
    std::string ret = "Broadcast to [";
    ret.append(NUM64.substr(idx * 2, 2)).append("]");
    return ret;
}
MATCHER_P(MatchBroadcast, idx, GenerateBroadcastToStr(idx))
{
    [[maybe_unused]] const auto& tmp = result_listener;
    return CheckMatchSame(arg, idx);
}
MATCHER_P(MatchBroadcastLane, idx, GenerateBroadcastToStr(idx))
{
    [[maybe_unused]] const auto& tmp = result_listener;
    return CheckMatchSame2(arg, idx);
}

template<typename T, bool IsLane = false>
class BroadcastTest : public SIMDFixture
{
private:
public:
    void CheckOnce(const T& data, const uint8_t idx)
    {
        using U = typename T::EleType;
        U out[T::Count] = { 0 };
        data.Save(out);
        if constexpr (IsLane)
            EXPECT_THAT(out, MatchBroadcastLane(idx));
        else
            EXPECT_THAT(out, MatchBroadcast(idx));
    }
    template<size_t... Idxes>
    void CheckAll(const T& in, std::index_sequence<Idxes...>)
    {
        if constexpr (IsLane)
        {
            static_assert(sizeof...(Idxes) * 2 == T::Count);
            (..., CheckOnce(in.template BroadcastLane<Idxes>(), static_cast<uint8_t>(Idxes)));
        }
        else
        {
            static_assert(sizeof...(Idxes) == T::Count);
            (..., CheckOnce(in.template Broadcast<Idxes>(), static_cast<uint8_t>(Idxes)));
        }
    }
public:
    static constexpr auto TestSuite = IsLane ? "SIMDBroadcastLane" : "SIMDBroadcast";
    void TestBody() override
    {
        const auto data = GenerateT<T>(std::make_index_sequence<T::Count>{}, 0);
        CheckAll(data, std::make_index_sequence<IsLane ? T::Count / 2 : T::Count>{});
    }
};
#define RegisterSIMDBroadcastTestItem(r, lv, i, type)  RegisterSIMDTest(type, lv, shuftest::BroadcastTest<type, false>);
#define RegisterSIMDBroadcastTest(lv, ...)  BOOST_PP_SEQ_FOR_EACH_I(RegisterSIMDBroadcastTestItem, lv, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define RegisterSIMDBroadcastLaneTestItem(r, lv, i, type)  RegisterSIMDTest(type, lv, shuftest::BroadcastTest<type, true>);
#define RegisterSIMDBroadcastLaneTest(lv, ...)  BOOST_PP_SEQ_FOR_EACH_I(RegisterSIMDBroadcastLaneTestItem, lv, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))


inline std::string GenerateSelectStr(uint64_t mask, size_t bits)
{
    std::string ret = "Select to [";
    while (bits-- > 0)
    {
        ret.push_back(mask & 0x1 ? '1' : '0');
        mask >>= 1;
    }
    ret.append("]");
    return ret;
}
MATCHER_P2(FailSelect, mask, bits, GenerateSelectStr(mask, bits))
{
    [[maybe_unused]] const auto& tmp0 = arg;
    [[maybe_unused]] const auto& tmp1 = result_listener;
    return false;
}

template<typename T>
class SelectTestBase
{
protected:
    static T InitAll(uint8_t val) noexcept
    {
        T ret;
        using U = typename T::EleType;
        for (size_t i = 0; i < T::Count; ++i)
            ret.Val[i] = static_cast<U>(val);
        return ret;
    }
    const T Zero = InitAll(0), One = InitAll(1);
    static void CheckSelect(const T& obj, uint64_t mask)
    {
        const uint64_t mask_ = mask;
        bool match = true;
        for (size_t idx = 0; idx < T::Count && match; ++idx)
        {
            match &= (obj.Val[idx] == static_cast<typename T::EleType>(mask & 0x1));
            mask >>= 1;
        }
        if (!match)
        {
            EXPECT_THAT(obj.Val, FailSelect(mask_, T::Count));
        }
    }
};
template<typename T>
class SelectTest : public SIMDFixture, public SelectTestBase<T>
{
private:
    using Base = SelectTestBase<T>;
    static constexpr uint32_t BatchSize = 256;
    static constexpr uint64_t InstCount = Pow<2, T::Count>();
    static_assert(InstCount < BatchSize || InstCount % BatchSize == 0);
    static constexpr uint32_t BatchCount = static_cast<uint32_t>(std::max<uint64_t>(1, InstCount / BatchSize));
    template<uint64_t IdxBegin, uint16_t... Vals>
    void TestSelect(std::integer_sequence<uint16_t, Vals...>) const
    {
        T output[] = { Base::Zero.template SelectWith<IdxBegin + Vals>(Base::One)... };
        uint64_t mask = IdxBegin;
        for (const auto& out : output)
            Base::CheckSelect(out, mask++);
    }
    template<uint64_t BatchBegin, uint64_t BatchEnd>
    void TestSelect() const
    {
        if constexpr (BatchBegin == BatchEnd)
        {
            constexpr uint64_t IdxBegin = BatchBegin * BatchSize;
            constexpr uint32_t IdxCount = std::min(static_cast<uint32_t>(InstCount - IdxBegin), BatchSize);
            TestSelect<IdxBegin>(std::make_integer_sequence<uint16_t, IdxCount>{});
        }
        else
        {
            TestSelect<BatchBegin, BatchBegin + (BatchEnd - BatchBegin) / 2>();
            TestSelect<BatchBegin + (BatchEnd - BatchBegin) / 2 + 1, BatchEnd>();
        }
    }
public:
    static constexpr auto TestSuite = "Select";
    void TestBody() override
    {
        TestSelect<0, BatchCount - 1>();
    }
};
template<typename T>
class SelectSlimTest : public SIMDFixture, public SelectTestBase<T>
{
private:
    using Base = SelectTestBase<T>;
    static constexpr size_t N = T::Count;
    static constexpr size_t TestCount = N * 2 + 1;
    static constexpr auto Tests = []() 
    {
        std::array<uint64_t, TestCount> ret = {};
        for (size_t i = 0; i < N; ++i)
            ret[i] = uint64_t(1) << i;
        constexpr uint64_t mask = 0xaaaaaaaaaaaaaaaau >> (64 - N);
        for (size_t i = 0; i < N - 1; ++i)
            ret[N + i] = mask >> i;
        ret[N * 2 - 1] = 0, ret[N * 2] = (uint64_t(1) << N) - 1;
        return ret;
    }();
    template<uint8_t... Vals>
    void TestSelect(std::integer_sequence<uint8_t, Vals...>) const
    {
        T output[] = { Base::Zero.template SelectWith<Tests[Vals]>(Base::One)... };
        size_t idx = 0;
        for (const auto& out : output)
            Base::CheckSelect(out, Tests[idx++]);
    }
public:
    static constexpr auto TestSuite = "Select";
    void TestBody() override
    {
        TestSelect(std::make_integer_sequence<uint8_t, TestCount>{});
    }
};
template<typename T>
class SelectRandTest : public SIMDFixture, public SelectTestBase<T>
{
private:
    using Base = SelectTestBase<T>;
    static constexpr size_t N = T::Count;
    template<uint32_t... Vals>
    void TestSelect(std::integer_sequence<uint32_t, Vals...>) const
    {
        T output[] = { Base::Zero.template SelectWith<StaticRands[Vals] % N>(Base::One)... };
        size_t idx = 0;
        for (const auto& out : output)
            Base::CheckSelect(out, StaticRands[idx++] % N);
    }
public:
    static constexpr auto TestSuite = "Select";
    void TestBody() override
    {
        TestSelect(std::make_integer_sequence<uint32_t, StaticRands.size()>{});
    }
};
#define RegisterSIMDSelectTestItem(r, lv, i, type)  RegisterSIMDTest(type, lv, shuftest::SelectTest<type>);
#define RegisterSIMDSelectTest(lv, ...)  BOOST_PP_SEQ_FOR_EACH_I(RegisterSIMDSelectTestItem, lv, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define RegisterSIMDSelectSlimTestItem(r, lv, i, type)  RegisterSIMDTest(type, lv, shuftest::SelectSlimTest<type>);
#define RegisterSIMDSelectSlimTest(lv, ...)  BOOST_PP_SEQ_FOR_EACH_I(RegisterSIMDSelectSlimTestItem, lv, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define RegisterSIMDSelectRandTestItem(r, lv, i, type)  RegisterSIMDTest(type, lv, shuftest::SelectRandTest<type>);
#define RegisterSIMDSelectRandTest(lv, ...)  BOOST_PP_SEQ_FOR_EACH_I(RegisterSIMDSelectRandTestItem, lv, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

}