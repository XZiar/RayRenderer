#pragma once
#include "pch.h"
#pragma message("Compiling SIMDBaseTest with " STRINGIZE(COMMON_SIMD_INTRIN) )

namespace simdtest
{
using namespace std::string_literals;
using namespace std::string_view_literals;

template<typename T, size_t N>
static std::string GenerateMatchStr(const std::array<T, N>& ref)
{
    std::string ret; ret.reserve(256);
    ret.append("[");
    bool isFirst = true;
    for (const auto val : ref)
    {
        if (!isFirst)
            ret.append(", ");
        ret.append(std::to_string(val));
        isFirst = false;
    }
    ret.append("]");
    return ret;
}
template<typename T>
static bool CheckSingleMatch(const T arg, const T ref)
{
    if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
    {
        const bool argNan = std::isnan(arg), refNan = std::isnan(ref);
        if (argNan != refNan)
            return false;
        if (argNan)
            return true;
        if (arg == ref)
            return true;
        const T diff = arg - ref;
        if (std::abs(diff) / std::abs(ref) <= std::numeric_limits<T>::epsilon())
            return true;
        //if (listener->IsInterested())
        //    *listener << "which is " << diff << " from " << ref;
        return false;
    }
    else
    {
        if (arg == ref)
            return true;
        /*if (listener->IsInterested())
            *listener << "which is " << (arg - ref) << " from " << ref;*/
        return false;
    }
}

template<typename T, size_t N, size_t... Idx>
forceinline bool CheckMatch(const T(&arg)[N], const std::array<T, N>& ref, std::index_sequence<Idx...>)
{
    return (... && CheckSingleMatch(arg[Idx], ref[Idx]));
}

MATCHER_P(MatchVec, ref, GenerateMatchStr(ref))
{
    [[maybe_unused]] const auto& tmp = result_listener;
    constexpr size_t N = std::tuple_size_v<std::decay_t<decltype(ref)>>;
    return CheckMatch(arg, ref, std::make_index_sequence<N>{});
}

template<typename T>
struct Upcast { using U = void; using S = void; };
template<> struct Upcast<uint8_t>  { using U = uint16_t; using S = int16_t; };
template<> struct Upcast<int8_t>   { using U =  int16_t; using S = int16_t; };
template<> struct Upcast<uint16_t> { using U = uint32_t; using S = int32_t; };
template<> struct Upcast<int16_t>  { using U =  int32_t; using S = int32_t; };
template<> struct Upcast<uint32_t> { using U = uint64_t; using S = int64_t; };
template<> struct Upcast<int32_t>  { using U =  int64_t; using S = int64_t; };
#ifdef __SIZEOF_INT128__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wpedantic"
template<> struct Upcast<uint64_t> { using U = unsigned __int128; using S = __int128; };
template<> struct Upcast<int64_t>  { using U =          __int128; using S = __int128; };
#   pragma GCC diagnostic pop
#else
struct UINT128
{
    uint64_t Low, High;
    constexpr UINT128() noexcept : Low(0), High(0) {}
    constexpr UINT128(uint64_t val) noexcept : Low(val), High(0) {}
    constexpr UINT128(uint64_t low, uint64_t hi) noexcept : Low(low), High(hi) {}
    constexpr UINT128 operator+(UINT128 rhs) const noexcept
    {
        auto low = Low + rhs.Low, high = High + rhs.High;
        if (low < Low) ++high;
        return { low, high };
    }
    constexpr UINT128 operator-(UINT128 rhs) const noexcept
    {
        auto low = Low - rhs.Low, high = High - rhs.High;
        if (Low < rhs.Low) --high;
        return { low, high };
    }
    constexpr bool operator<(UINT128 rhs) const noexcept
    {
        return (High == rhs.High) ? (Low < rhs.Low) : (High < rhs.High);
    }
    constexpr operator uint64_t() const noexcept { return Low; }
};
struct INT128
{
    uint64_t Low; int64_t High;
    constexpr INT128() noexcept : Low(0), High(0) {}
    constexpr INT128(int64_t val) noexcept : Low(val), High(val < 0 ? -1 : 0) {}
    constexpr INT128(uint64_t val) noexcept : Low(val), High(0) {}
    constexpr INT128(uint64_t low, int64_t hi) noexcept : Low(low), High(hi) {}
    constexpr INT128 operator+(INT128 rhs) const noexcept
    {
        auto low = Low + rhs.Low; auto high = High + rhs.High;
        if (low < Low) ++high;
        return { low, high };
    }
    constexpr INT128 operator-(INT128 rhs) const noexcept
    {
        auto low = Low - rhs.Low; auto high = High - rhs.High;
        if (Low < rhs.Low) --high;
        return { low, high };
    }
    constexpr bool operator<(INT128 rhs) const noexcept
    {
        return (High == rhs.High) ? (Low < rhs.Low) : (High < rhs.High);
    }
    constexpr operator int64_t() const noexcept { return static_cast<int64_t>(Low); }
};
template<> struct Upcast<uint64_t> { using U = UINT128; using S = INT128; };
template<> struct Upcast<int64_t>  { using U =  INT128; using S = INT128; };
#endif
template<typename T> using UpcastT = typename Upcast<T>::U;
template<typename T> using UpcastST = typename Upcast<T>::S;

template<typename T, size_t Begin, size_t... Idxes>
static constexpr T GenerateTSeq(std::index_sequence<Idxes...>)
{
    static_assert(T::Count == sizeof...(Idxes), "Not having same number of arg");
    using ArgType = std::decay_t<decltype(std::declval<T&>().Val[0])>;
    return T(static_cast<ArgType>(Begin + Idxes)...);
}


#define ForKItem(n) for (size_t k = 0; k * sizeof(T) * n < RandValBytes; k += n)

template<typename T>
static void TestAdd(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        const auto output = data1.Add(data2);
        typename T::EleType ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = data1.Val[i] + data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing Add";
    }
}

template<typename T>
static void TestSub(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        const auto output = data1.Sub(data2);
        typename T::EleType ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = data1.Val[i] - data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing Sub";
    }
}

template<typename T>
static void TestSatAdd(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = typename T::EleType;
        using V = UpcastST<U>;
        const auto output = data1.SatAdd(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
        {
            const auto tmp = static_cast<V>(static_cast<V>(data1.Val[i]) + static_cast<V>(data2.Val[i]));
            ref[i] = static_cast<U>(std::clamp<V>(tmp, std::numeric_limits<U>::min(), std::numeric_limits<U>::max()));
        }
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing SatAdd";
    }
}

template<typename T>
static void TestSatSub(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = typename T::EleType;
        using V = UpcastST<U>;
        const auto output = data1.SatSub(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
        {
            const auto tmp = static_cast<V>(static_cast<V>(data1.Val[i]) - static_cast<V>(data2.Val[i]));
            ref[i] = static_cast<U>(std::clamp<V>(tmp, std::numeric_limits<U>::min(), std::numeric_limits<U>::max()));
        }
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing SatSub";
    }
}

template<typename T>
static void TestMul(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        const auto output = data1.Mul(data2);
        typename T::EleType ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = data1.Val[i] * data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing Mul";
    }
}

template<typename T>
static void TestMulLo(const T* ptr)
{
    ForKItem(2)
    {
        using U = typename T::EleType;
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        const auto output = data1.MulLo(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = static_cast<U>(data1.Val[i] * data2.Val[i]); // c++ keeps only low part natively
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing MulLo";
    }
}

template<typename T>
static void TestMulHi(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = typename T::EleType;
        using V = UpcastT<U>;
        const auto output = data1.MulHi(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
        {
#if COMMON_COMPILER_MSVC
            if constexpr (std::is_same_v<U, uint64_t>)
            {
                ref[i] = __umulh(data1.Val[i], data2.Val[i]);
            }
            else if constexpr (std::is_same_v<U, int64_t>)
            {
                ref[i] = __mulh(data1.Val[i], data2.Val[i]);
            }
            else
            {
#endif
                ref[i] = static_cast<U>((static_cast<V>(data1.Val[i]) * static_cast<V>(data2.Val[i])) >> (sizeof(U) * 8));
#if COMMON_COMPILER_MSVC
            }
#endif
        }
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing MulHi";
    }
}

template<typename T>
static void TestMulX(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = typename T::EleType;
        using V = UpcastT<U>;
        const auto output = data1.MulX(data2);
        V out[T::Count] = { 0 };
        V ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
        {
            out[i] = output[i * 2 / T::Count].Val[i % (T::Count / 2)];
            ref[i] = static_cast<V>(static_cast<V>(data1.Val[i]) * static_cast<V>(data2.Val[i]));
        }
        EXPECT_THAT(out, testing::ElementsAreArray(ref)) << "when testing MulX";
    }
}

template<typename T>
static constexpr auto CheckDivVec(const T*) -> typename std::is_same<decltype(std::declval<T>().Div(std::declval<T>())), T>::type;
template<typename T>
static constexpr std::false_type CheckDivVec(...);
template<typename T>
static void TestDiv(const T* ptr)
{
    if constexpr(decltype(CheckDivVec<T>(0))::value)
    {
        ForKItem(2)
        {
            const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
            const auto output = data1.Div(data2);
            std::array<typename T::EleType, T::Count> ref = { 0 };
            for (uint8_t i = 0; i < T::Count; ++i)
                ref[i] = data1.Val[i] / data2.Val[i];
            //EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
            EXPECT_THAT(output.Val, MatchVec(ref)) << "when testing Div";
        }
    }
    else
    {
        ForKItem(1)
        {
            const auto data = ptr[k + 0];
            const auto output = data.Div(data.Val[0]);
            std::array<typename T::EleType, T::Count> ref = { 0 };
            for (uint8_t i = 0; i < T::Count; ++i)
                ref[i] = data.Val[i] / data.Val[0];
            //EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
            EXPECT_THAT(output.Val, MatchVec(ref)) << "when testing Div";
        }
    }
}

template<typename T>
static void TestNeg(const T* ptr)
{
    ForKItem(1)
    {
        const auto data = ptr[k + 0];
        using U = typename T::EleType;
        const auto output = data.Neg();
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
        {
            ref[i] = static_cast<U>(-data.Val[i]);
        }
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing Neg";
    }
}

template<typename T>
static void TestAbs(const T* ptr)
{
    ForKItem(1)
    {
        const auto data = ptr[k + 0];
        using U = typename T::EleType;
        const auto output = data.Abs();
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
        {
            if constexpr (std::is_unsigned_v<U>)
                ref[i] = data.Val[i];
            else
                ref[i] = static_cast<U>(data.Val[i] < 0 ? -data.Val[i] : data.Val[i]);
        }
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing Abs";
    }
}

template<typename T>
static void TestAnd(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = typename T::EleType;
        const auto output = data1.And(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = data1.Val[i] & data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing And";
    }
}

template<typename T>
static void TestOr(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        const auto output = data1.Or(data2);
        typename T::EleType ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = data1.Val[i] | data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing Or";
    }
}

template<typename T>
static void TestXor(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        const auto output = data1.Xor(data2);
        typename T::EleType ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = data1.Val[i] ^ data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing Xor";
    }
}

template<typename T>
static void TestAndNot(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        const auto output = data1.AndNot(data2);
        typename T::EleType ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = (~data1.Val[i]) & data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing AndNot";
    }
}

template<typename T>
static void TestNot(const T* ptr)
{
    ForKItem(1)
    {
        const auto data = ptr[k + 0];
        const auto output = data.Not();
        typename T::EleType ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = ~data.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing Not";
    }
}

template<typename T>
static void TestMin(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        const auto output = data1.Min(data2);
        typename T::EleType ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = std::min(data1.Val[i], data2.Val[i]);
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing Min";
    }
}

template<typename T>
static void TestMax(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        const auto output = data1.Max(data2);
        typename T::EleType ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = std::max(data1.Val[i], data2.Val[i]);
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref)) << "when testing Max";
    }
}

template<typename T, size_t N, size_t... Idx>
static void FillFMALane(const T& base, const T& muler, const T& adder, T (&out)[N][4], std::index_sequence<Idx...>)
{
    (..., void(out[Idx][0] = base.template MulAdd <Idx>(muler, adder)));
    (..., void(out[Idx][1] = base.template MulSub <Idx>(muler, adder)));
    (..., void(out[Idx][2] = base.template NMulAdd<Idx>(muler, adder)));
    (..., void(out[Idx][3] = base.template NMulSub<Idx>(muler, adder)));
}
template<typename T>
static void TestFMA(const T* ptr)
{
    using U = typename T::EleType;
    constexpr size_t N = T::Count;
    ForKItem(3)
    {
        const auto base = ptr[k + 0], muler = ptr[k + 1], adder = ptr[k + 2];
        const T out1[4] = { base.MulAdd(muler, adder), base.MulSub(muler, adder), base.NMulAdd(muler, adder), base.NMulSub(muler, adder) };
        std::array<U, N> ref1[4] = { {0}, {0}, {0}, {0} };
        for (uint8_t i = 0; i < N; ++i)
        {
            ref1[0][i] = base.Val[i] * muler.Val[i] + adder.Val[i];
            ref1[1][i] = base.Val[i] * muler.Val[i] - adder.Val[i];
            ref1[2][i] = adder.Val[i] - base.Val[i] * muler.Val[i];
            ref1[3][i] = -ref1[0][i];
        }
        EXPECT_THAT(out1[0].Val, MatchVec(ref1[0])) << "when testing FMA-MulAdd";
        EXPECT_THAT(out1[1].Val, MatchVec(ref1[1])) << "when testing FMA-MulSub";
        EXPECT_THAT(out1[2].Val, MatchVec(ref1[2])) << "when testing FMA-NMulAdd";
        EXPECT_THAT(out1[3].Val, MatchVec(ref1[3])) << "when testing FMA-NMulSub";
        T out2[N][4];
        FillFMALane(base, muler, adder, out2, std::make_index_sequence<N>{});
        for (size_t i = 0; i < N; ++i)
        {
            std::array<U, N> ref2[4] = { {0}, {0}, {0}, {0} };
            const auto mul = muler.Val[i];
            for (size_t j = 0; j < N; ++j)
            {
                ref2[0][j] = base.Val[j] * mul + adder.Val[j];
                ref2[1][j] = base.Val[j] * mul - adder.Val[j];
                ref2[2][j] = adder.Val[j] - base.Val[j] * mul;
                ref2[3][j] = -ref2[0][j];
            }
            EXPECT_THAT(out2[i][0].Val, MatchVec(ref2[0])) << "when testing FMA-MulAdd<" << i << ">";
            EXPECT_THAT(out2[i][1].Val, MatchVec(ref2[1])) << "when testing FMA-MulSub<" << i << ">";
            EXPECT_THAT(out2[i][2].Val, MatchVec(ref2[2])) << "when testing FMA-NMulAdd<" << i << ">";
            EXPECT_THAT(out2[i][3].Val, MatchVec(ref2[3])) << "when testing FMA-NMulSub<" << i << ">";
        }
    }
}

template<typename T>
static void TestSLL(const T* ptr)
{
    using U = typename T::EleType;
    constexpr uint8_t S = sizeof(U);
    ForKItem(1)
    {
        constexpr uint8_t Bits[] = { 0, S * 2, S * 4, S * 6, S * 8 };
        const auto data = ptr[k + 0];
        const T output0[5] = 
        { 
            data.template ShiftLeftLogic<Bits[0]>(), data.template ShiftLeftLogic<Bits[1]>(),
            data.template ShiftLeftLogic<Bits[2]>(), data.template ShiftLeftLogic<Bits[3]>(),
            data.template ShiftLeftLogic<Bits[4]>(),
        };
        const T output1[5] = { data.ShiftLeftLogic(Bits[0]), data.ShiftLeftLogic(Bits[1]),
            data.ShiftLeftLogic(Bits[2]), data.ShiftLeftLogic(Bits[3]), data.ShiftLeftLogic(Bits[4]) };
        for (uint8_t j = 0; j < 4; ++j)
        {
            U ref[T::Count] = { 0 };
            for (uint8_t i = 0; i < T::Count; ++i)
                ref[i] = static_cast<U>(data.Val[i] << Bits[j]);
            EXPECT_THAT(output0[j].Val, testing::ElementsAreArray(ref)) << "when shift-left [" << (int)Bits[j] << "] bits";
            EXPECT_THAT(output1[j].Val, testing::ElementsAreArray(ref)) << "when shift-left [" << (int)Bits[j] << "] bits";
        }
        EXPECT_THAT(output0[4].Val, testing::Each(U(0))) << "when shift-left [" << (int)Bits[4] << "] bits";
        EXPECT_THAT(output1[4].Val, testing::Each(U(0))) << "when shift-left [" << (int)Bits[4] << "] bits";
    }
}
template<typename T>
static void TestSRL(const T* ptr)
{
    using U = typename T::EleType;
    using V = std::make_unsigned_t<U>;
    constexpr uint8_t S = sizeof(U);
    ForKItem(1)
    {
        constexpr uint8_t Bits[] = { 0, S * 2, S * 4, S * 6, S * 8 };
        const auto data = ptr[k + 0];
        const T output0[5] = 
        { 
            data.template ShiftRightLogic<Bits[0]>(), 
            data.template ShiftRightLogic<Bits[1]>(), data.template ShiftRightLogic<Bits[2]>(), 
            data.template ShiftRightLogic<Bits[3]>(), data.template ShiftRightLogic<Bits[4]>() 
        };
        const T output1[5] = { data.ShiftRightLogic(Bits[0]), data.ShiftRightLogic(Bits[1]),
            data.ShiftRightLogic(Bits[2]), data.ShiftRightLogic(Bits[3]), data.ShiftRightLogic(Bits[4]) };
        for (uint8_t j = 0; j < 4; ++j)
        {
            U ref[T::Count] = { 0 };
            for (uint8_t i = 0; i < T::Count; ++i)
                ref[i] = static_cast<U>(static_cast<V>(data.Val[i]) >> Bits[j]);
            EXPECT_THAT(output0[j].Val, testing::ElementsAreArray(ref)) << "when shift-right-logic [" << (int)Bits[j] << "] bits";
            EXPECT_THAT(output1[j].Val, testing::ElementsAreArray(ref)) << "when shift-right-logic [" << (int)Bits[j] << "] bits";
        }
        EXPECT_THAT(output0[4].Val, testing::Each(U(0))) << "when shift-right-logic [" << (int)Bits[4] << "] bits";
        EXPECT_THAT(output1[4].Val, testing::Each(U(0))) << "when shift-right-logic [" << (int)Bits[4] << "] bits";
    }
}
template<typename T>
static void TestSRA(const T* ptr)
{
    using U = typename T::EleType;
    constexpr uint8_t S = sizeof(U);
    ForKItem(1)
    {
        constexpr uint8_t Bits[5] = { 0, S * 2, S * 4, S * 6, S * 8 };
        const auto data = ptr[k + 0];
        const T output0[5] = 
        { 
            data.template ShiftRightArith<Bits[0]>(),
            data.template ShiftRightArith<Bits[1]>(), data.template ShiftRightArith<Bits[2]>(),
            data.template ShiftRightArith<Bits[3]>(), data.template ShiftRightArith<Bits[4]>() 
        };
        const T output1[5] = { data.ShiftRightArith(Bits[0]), data.ShiftRightArith(Bits[1]),
            data.ShiftRightArith(Bits[2]), data.ShiftRightArith(Bits[3]), data.ShiftRightArith(Bits[4]) };
        for (uint8_t j = 0; j < 4; ++j)
        {
            U ref[T::Count] = { 0 };
            for (uint8_t i = 0; i < T::Count; ++i)
                ref[i] = static_cast<U>(data.Val[i] >> Bits[j]);
            EXPECT_THAT(output0[j].Val, testing::ElementsAreArray(ref)) << "when shift-right-arith [" << (int)Bits[j] << "] bits";
            EXPECT_THAT(output1[j].Val, testing::ElementsAreArray(ref)) << "when shift-right-arith [" << (int)Bits[j] << "] bits";
        }
        if constexpr (std::is_unsigned_v<U>)
        {
            EXPECT_THAT(output0[4].Val, testing::Each(U(0))) << "when shift-right-arith [" << (int)Bits[4] << "] bits";
            EXPECT_THAT(output1[4].Val, testing::Each(U(0))) << "when shift-right-arith [" << (int)Bits[4] << "] bits";
        }
        else
        {
            U ref[T::Count] = { 0 };
            for (uint8_t i = 0; i < T::Count; ++i)
                ref[i] = static_cast<U>(data.Val[i] >= 0 ? 0ull : UINT64_MAX);
            EXPECT_THAT(output0[4].Val, testing::ElementsAreArray(ref)) << "when shift-right-arith [" << (int)Bits[4] << "] bits";
            EXPECT_THAT(output1[4].Val, testing::ElementsAreArray(ref)) << "when shift-right-arith [" << (int)Bits[4] << "] bits";
        }
    }
}

template<typename T, typename U, size_t... I>
forceinline void CopyEles(const T& src, U* dst, std::index_sequence<I...>)
{
    using V = std::decay_t<decltype(src[0])>;
    constexpr auto N = V::Count;
    (..., src[I].Save(dst + N * I));
}
template<typename U, CastMode Mode, typename T>
forceinline U CastSingle(T val) noexcept
{
    if constexpr (Mode == CastMode::RangeSaturate)
    {
        constexpr auto dstMin = std::numeric_limits<U>::min(), dstMax = std::numeric_limits<U>::max();
        constexpr auto minVal = static_cast<T>(dstMin), maxVal = static_cast<T>(dstMax);
        if (val <= minVal) return dstMin;
        if (val >= maxVal) return dstMax;
    }
    return static_cast<U>(val);
}
template<typename T, typename U, CastMode Mode>
static void TestCast(const T* ptr)
{
    using V = typename U::EleType;
    if constexpr (T::Count >= U::Count)
    {
        ForKItem(1)
        {
            const auto data = ptr[k];
            const auto output = data.template Cast<U, Mode>();
            std::array<V, T::Count> ref = { 0 };
            V out[T::Count] = { 0 };
            if constexpr (T::Count == U::Count)
                output.Save(out);
            else
                CopyEles(output, out, std::make_index_sequence<T::Count / U::Count>{});
            for (uint8_t i = 0; i < T::Count; ++i)
                ref[i] = CastSingle<V, Mode>(data.Val[i]);
            EXPECT_THAT(out, MatchVec(ref)) << "when testing UpCast";
        }
    }
    else
    {
        constexpr auto K = U::Count / T::Count;
        ForKItem(K)
        {
            U output;
            if constexpr (K == 2)
                output = ptr[k * 2].template Cast<U, Mode>(ptr[k * 2 + 1]);
            else if constexpr (K == 4)
                output = ptr[k * 4].template Cast<U, Mode>(ptr[k * 4 + 1], ptr[k * 4 + 2], ptr[k * 4 + 3]);
            else if constexpr (K == 8)
                output = ptr[k * 8].template Cast<U, Mode>(ptr[k * 8 + 1], ptr[k * 8 + 2], ptr[k * 8 + 3], 
                    ptr[k * 8 + 4], ptr[k * 8 + 5], ptr[k * 8 + 6], ptr[k * 8 + 7]);
            else
                static_assert(!::common::AlwaysTrue<T>, "not supported");
            std::array<V, U::Count> ref = { 0 };
            for (size_t j = 0; j < K; ++j)
            {
                const auto& data = ptr[k * K + j];
                for (uint8_t i = 0; i < T::Count; ++i)
                    ref[j * T::Count + i] = CastSingle<V, Mode>(data.Val[i]);
            }
            EXPECT_THAT(output.Val, MatchVec(ref)) << "when testing DownCast";
        }
    }
}

template<typename T>
inline T GetAllOnes() noexcept
{
    if constexpr (std::is_same_v<T, float>)
    {
        constexpr uint32_t X = UINT32_MAX;
        T ret;
        memcpy(&ret, &X, sizeof(X));
        return ret;
    }
    else if constexpr (std::is_same_v<T, double>)
    {
        constexpr uint64_t X = UINT64_MAX;
        T ret;
        memcpy(&ret, &X, sizeof(X));
        return ret;
    }
    else if constexpr (std::is_unsigned_v<T>)
        return std::numeric_limits<T>::max();
    else
        return static_cast<T>(-1);
}
template<typename T>
static void TestCompare(const T* ptr)
{
    using U = typename T::EleType;
    const U ValTrue = GetAllOnes<U>(), ValFalse = 0;
    ForKItem(2)
    {
        const auto data0 = ptr[k + 0], data1 = ptr[k + 1];
#define TestRel(cmp, op) do {                                                                       \
        const auto output = data0. template Compare<CompareType::cmp, MaskType::FullEle>(data1);    \
        std::array<U, T::Count> ref = { 0 };                                                        \
        for (uint8_t i = 0; i < T::Count; ++i)                                                      \
            ref[i] = data0.Val[i] op data1.Val[i] ? ValTrue : ValFalse;                             \
        EXPECT_THAT(output.Val, MatchVec(ref)) << "when testing Compare " #cmp;                     \
        } while(false)
        TestRel(LessThan,       < );
        TestRel(LessEqual,      <=);
        TestRel(Equal,          ==);
        TestRel(NotEqual,       !=);
        TestRel(GreaterEqual,   >=);
        TestRel(GreaterThan,    > );
#undef TestRel
        /*const auto output = data0.Compare<CompareType::LessThan, MaskType::FullEle>(data1);
        std::array<U, T::Count> ref = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = data0.Val[i] < data1.Val[i] ? ValTrue : ValFalse;
        EXPECT_THAT(output.Val, MatchVec(ref));*/
    }
}

template<typename T>
inline bool CheckMSB(T val) noexcept
{
    if constexpr (std::is_same_v<T, float>)
    {
        uint32_t newVal;
        memcpy(&newVal, &val, sizeof(T));
        return CheckMSB(newVal);
    }
    else if constexpr (std::is_same_v<T, double>)
    {
        uint64_t newVal;
        memcpy(&newVal, &val, sizeof(T));
        return CheckMSB(newVal);
    }
    else if constexpr (std::is_unsigned_v<T>)
    {
        constexpr auto MSB = static_cast<T>(1) << (sizeof(T) * 8 - 1);
        return val & MSB;
    }
    else
        return val < 0;
}
template<typename T>
static void TestSEL(const T* ptr)
{
    using U = typename T::EleType;
    const U ValTrue = GetAllOnes<U>(), ValFalse = 0, Val1 = 1;
    T data1(ValFalse), data2(Val1);
    ForKItem(1)
    {
        const auto mask0 = ptr[k + 0];
        T mask1, ref;
        for (uint32_t i = 0; i < T::Count; ++i)
        {
            const auto msb = CheckMSB(mask0.Val[i]);
            mask1.Val[i] = msb ? ValTrue : ValFalse;
            ref  .Val[i] = msb ? Val1    : ValFalse;
        }
        const auto output0 = data1.template SelectWith<MaskType::SigBit >(data2, mask0);
        const auto output1 = data1.template SelectWith<MaskType::FullEle>(data2, mask1);
        EXPECT_THAT(output0.Val, testing::ElementsAreArray(ref.Val)) << "when testing SelectWith SigBit";
        EXPECT_THAT(output1.Val, testing::ElementsAreArray(ref.Val)) << "when testing SelectWith FullEle";
    }
}


template<typename T>
T SwapEndian(const T val)
{
    constexpr size_t N = sizeof(T);
#if COMMON_COMPILER_MSVC
    if constexpr (N == 2)
        return static_cast<T>(_byteswap_ushort(static_cast<uint16_t>(val)));
    else if constexpr (N == 4)
        return static_cast<T>(_byteswap_ulong(static_cast<uint32_t>(val)));
    else if constexpr (N == 8)
        return static_cast<T>(_byteswap_uint64(static_cast<uint64_t>(val)));
#else
    if constexpr (N == 2)
        return static_cast<T>(__builtin_bswap16(static_cast<uint16_t>(val)));
    else if constexpr (N == 4)
        return static_cast<T>(__builtin_bswap32(static_cast<uint32_t>(val)));
    else if constexpr (N == 8)
        return static_cast<T>(__builtin_bswap64(static_cast<uint64_t>(val)));
#endif
    else
        static_assert(!::common::AlwaysTrue<T>, "not supported");
}
template<typename T>
static void TestSWE(const T* ptr)
{
    using U = typename T::EleType;
    ForKItem(1)
    {
        const auto data = ptr[k];
        const auto output = data.SwapEndian();
        std::array<U, T::Count> ref = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = SwapEndian(data.Val[i]);
        EXPECT_THAT(output.Val, MatchVec(ref)) << "when testing SwapEndian";
    }
}


#undef ForKItem

enum class TestItem : uint32_t
{
    Add = 0x1, Sub = 0x2, SatAdd = 0x4, SatSub = 0x8, Mul = 0x10, MulLo = 0x20, MulHi = 0x40, MulX = 0x80, 
    Div = 0x100, Neg = 0x200, Abs = 0x400, Min = 0x800, Max = 0x1000, SLL = 0x2000, SRL = 0x4000, SRA = 0x8000,
    And = 0x10000, Or = 0x20000, Xor = 0x40000, AndNot = 0x80000, Not = 0x100000, FMA = 0x200000,
    SWE = 0x10000000, SEL = 0x20000000
};
MAKE_ENUM_BITFIELD(TestItem)

template<typename T, TestItem Items>
class SIMDBaseTest : public SIMDFixture
{
public:
    static constexpr auto TestSuite = "SIMDBase";
    void TestBody() override
    {
#define AddItem(r, data, x) if constexpr (HAS_FIELD(Items, TestItem::x)) \
    BOOST_PP_CAT(Test,x)<T>(GetRandPtr<T, typename T::EleType>());
#define AddItems(...) BOOST_PP_SEQ_FOR_EACH(AddItem, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
        AddItems(Add, Sub, SatAdd, SatSub, Mul, MulLo, MulHi, MulX, Div, Neg, Abs, Min, SLL, SRL, SRA, Max, And, Or, Xor, AndNot, Not, SWE, SEL, FMA)
#undef AddItems
#undef AddItem
    }
};


#define SIMDBaseItem(r, data, i, x) BOOST_PP_IF(i, |, ) simdtest::TestItem::x
#define RegisterSIMDBaseTest(type, lv, ...)  RegisterSIMDTest(type, lv, \
simdtest::SIMDBaseTest<type, BOOST_PP_SEQ_FOR_EACH_I(SIMDBaseItem, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))>)


template<CastMode Mode, typename T, typename... U>
class SIMDCastTest : public SIMDFixture
{
public:
    static constexpr auto TestSuite = "SIMDCast";
    void TestBody() override
    {
        [[maybe_unused]] const std::string_view XX = STRINGIZE(COMMON_SIMD_INTRIN);
        const auto ptr = GetRandPtr<T, typename T::EleType>();
        if constexpr (Mode == CastMode::RangeUndef)
            (..., TestCast<T, U, detail::CstMode<T, U>()>(ptr));
        else
            (..., TestCast<T, U, Mode>(ptr));
    }
};


#define RegisterSIMDCastTest(type, lv, ...)  RegisterSIMDTest(type, lv, simdtest::SIMDCastTest<CastMode::RangeUndef, type, __VA_ARGS__>)
#define RegisterSIMDCastModeTest(type, lv, mode, ...)  RegisterSIMDTest(type, lv, simdtest::SIMDCastTest<CastMode::mode, type, __VA_ARGS__>)


template<typename T>
class SIMDCompareTest : public SIMDFixture
{
public:
    static constexpr auto TestSuite = "SIMDCompare";
    void TestBody() override
    {
        const auto ptr = GetRandPtr<T, typename T::EleType>();
        TestCompare(ptr);
    }
};

#define RegisterSIMDCmpTestItem(r, lv, i, type)  RegisterSIMDTest(type, lv, simdtest::SIMDCompareTest<type>);
#define RegisterSIMDCmpTest(lv, ...)  BOOST_PP_SEQ_FOR_EACH_I(RegisterSIMDCmpTestItem, lv, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))



}