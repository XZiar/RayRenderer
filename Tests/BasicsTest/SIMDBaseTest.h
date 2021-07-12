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
        if (std::abs(diff) <= std::numeric_limits<T>::epsilon())
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
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        const auto output = data1.Add(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = data1.Val[i] + data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestSub(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        const auto output = data1.Sub(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = data1.Val[i] - data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestSatAdd(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        using V = UpcastST<U>;
        const auto output = data1.SatAdd(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
        {
            const auto tmp = static_cast<V>(static_cast<V>(data1.Val[i]) + static_cast<V>(data2.Val[i]));
            ref[i] = static_cast<U>(std::clamp<V>(tmp, std::numeric_limits<U>::min(), std::numeric_limits<U>::max()));
        }
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestSatSub(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        using V = UpcastST<U>;
        const auto output = data1.SatSub(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
        {
            const auto tmp = static_cast<V>(static_cast<V>(data1.Val[i]) - static_cast<V>(data2.Val[i]));
            ref[i] = static_cast<U>(std::clamp<V>(tmp, std::numeric_limits<U>::min(), std::numeric_limits<U>::max()));
        }
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestMul(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        const auto output = data1.Mul(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = data1.Val[i] * data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestMulLo(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        const auto output = data1.MulLo(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = static_cast<U>(data1.Val[i] * data2.Val[i]); // c++ keeps only low part natively
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestMulHi(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
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
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestMulX(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        using V = UpcastT<U>;
        const auto output = data1.MulX(data2);
        V out[T::Count] = { 0 };
        V ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
        {
            out[i] = output[i * 2 / T::Count].Val[i % (T::Count / 2)];
            ref[i] = static_cast<V>(static_cast<V>(data1.Val[i]) * static_cast<V>(data2.Val[i]));
        }
        EXPECT_THAT(out, testing::ElementsAreArray(ref));
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
            using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
            const auto output = data1.Div(data2);
            std::array<U, T::Count> ref = { 0 };
            for (uint8_t i = 0; i < T::Count; ++i)
                ref[i] = data1.Val[i] / data2.Val[i];
            //EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
            EXPECT_THAT(output.Val, MatchVec(ref));
        }
    }
    else
    {
        ForKItem(1)
        {
            const auto data = ptr[k + 0];
            using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
            const auto output = data.Div(data.Val[0]);
            std::array<U, T::Count> ref = { 0 };
            for (uint8_t i = 0; i < T::Count; ++i)
                ref[i] = data.Val[i] / data.Val[0];
            //EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
            EXPECT_THAT(output.Val, MatchVec(ref));
        }
    }
}

template<typename T>
static void TestNeg(const T* ptr)
{
    ForKItem(1)
    {
        const auto data = ptr[k + 0];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        const auto output = data.Neg();
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
        {
            ref[i] = static_cast<U>(-data.Val[i]);
        }
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestAbs(const T* ptr)
{
    ForKItem(1)
    {
        const auto data = ptr[k + 0];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        const auto output = data.Abs();
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
        {
            if constexpr (std::is_unsigned_v<U>)
                ref[i] = data.Val[i];
            else
                ref[i] = static_cast<U>(data.Val[i] < 0 ? -data.Val[i] : data.Val[i]);
        }
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestAnd(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        const auto output = data1.And(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = data1.Val[i] & data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestOr(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        const auto output = data1.Or(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = data1.Val[i] | data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestXor(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        const auto output = data1.Xor(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = data1.Val[i] ^ data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestAndNot(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        const auto output = data1.AndNot(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = (~data1.Val[i]) & data2.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestNot(const T* ptr)
{
    ForKItem(1)
    {
        const auto data = ptr[k + 0];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        const auto output = data.Not();
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = ~data.Val[i];
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestMin(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        const auto output = data1.Min(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = std::min(data1.Val[i], data2.Val[i]);
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T>
static void TestMax(const T* ptr)
{
    ForKItem(2)
    {
        const auto data1 = ptr[k + 0], data2 = ptr[k + 1];
        using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
        const auto output = data1.Max(data2);
        U ref[T::Count] = { 0 };
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = std::max(data1.Val[i], data2.Val[i]);
        EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T, typename U, size_t... I>
forceinline void CopyEles(const T& src, U* dst, std::index_sequence<I...>)
{
    using V = std::decay_t<decltype(src[0])>;
    constexpr auto N = V::Count;
    (..., src[I].Save(dst + N * I));
}
template<typename T, typename U>
static void TestCast(const T* ptr)
{
    ForKItem(1)
    {
        const auto data = ptr[k];
        using V = std::decay_t<decltype(std::declval<U>().Val[0])>;
        const auto output = data.template Cast<U>();
        using X = std::decay_t<decltype(output)>;
        std::array<V, T::Count> ref = { 0 };
        V out[T::Count] = { 0 };
        if constexpr (std::is_same_v<X, U>)
        {
            output.Save(out);
        }
        else
        {
            CopyEles(output, out, std::make_index_sequence<X::EleCount>{});
        }
        for (uint8_t i = 0; i < T::Count; ++i)
            ref[i] = static_cast<V>(data.Val[i]);
        EXPECT_THAT(out, MatchVec(ref));
    }
}

#undef ForKItem

enum class TestItem : uint32_t
{
    Add = 0x1, Sub = 0x2, SatAdd = 0x4, SatSub = 0x8, Mul = 0x10, MulLo = 0x20, MulHi = 0x40, MulX = 0x80, 
    Div = 0x100, Neg = 0x200, Abs = 0x400,
    And = 0x1000, Or = 0x2000, Xor = 0x4000, AndNot = 0x8000, Not = 0x10000, Min = 0x20000, Max = 0x40000,
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
    BOOST_PP_CAT(Test,x)<T>(GetRandPtr<T, std::decay_t<decltype(std::declval<T>().Val[0])>>());
#define AddItems(...) BOOST_PP_SEQ_FOR_EACH(AddItem, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
        AddItems(Add, Sub, SatAdd, SatSub, Mul, MulLo, MulHi, Mul, Div, Neg, Abs, And, Or, Xor, AndNot, Not, Min, Max)
#undef AddItems
#undef AddItem
    }
};


#define SIMDBaseItem(r, data, i, x) BOOST_PP_IF(i, |, ) simdtest::TestItem::x
#define RegisterSIMDBaseTest(type, lv, ...)  RegisterSIMDTest(#type, lv, \
simdtest::SIMDBaseTest<common::simd::type, BOOST_PP_SEQ_FOR_EACH_I(SIMDBaseItem, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))>)


template<typename T, typename... U>
class SIMDCastTest : public SIMDFixture
{
public:
    static constexpr auto TestSuite = "SIMDCast";
    static constexpr auto Idxes = std::make_index_sequence<sizeof...(U)>{};
    void TestBody() override
    {
        const auto ptr = GetRandPtr<T, std::decay_t<decltype(std::declval<T>().Val[0])>>();
        (..., TestCast<T, U>(ptr));
    }
};


#define RegisterSIMDCastTest(type, lv, ...)  RegisterSIMDTest(#type, lv, simdtest::SIMDCastTest<common::simd::type, __VA_ARGS__>)


}