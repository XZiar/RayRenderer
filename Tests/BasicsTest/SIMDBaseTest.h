#pragma once
#include "pch.h"
#pragma message("Compiling SIMDBaseTest with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace simdtest
{
using namespace std::string_literals;
using namespace std::string_view_literals;


template<typename T>
struct Upcast;
template<> struct Upcast<uint8_t>  { using U = uint16_t; };
template<> struct Upcast<int8_t>   { using U = int16_t;  };
template<> struct Upcast<uint16_t> { using U = uint32_t; };
template<> struct Upcast<int16_t>  { using U = int32_t;  };
template<> struct Upcast<uint32_t> { using U = uint64_t; };
template<> struct Upcast<int32_t>  { using U = int64_t;  };

template<typename T, size_t Begin, size_t... Idxes>
static constexpr T GenerateTSeq(std::index_sequence<Idxes...>)
{
    static_assert(T::Count == sizeof...(Idxes), "Not having same number of arg");
    using ArgType = std::decay_t<decltype(std::declval<T&>().Val[0])>;
    return T(static_cast<ArgType>(Begin + Idxes)...);
}

alignas(32) static const std::array<uint8_t, 64> RandVals = []() 
{
    std::array<uint8_t, 64> vals = {};
    for (auto& val : vals)
        val = static_cast<uint8_t>(GetARand());
    return vals;
}();

alignas(32) static const std::array<float, 16> RandValsF32 = []()
{
    std::array<float, 16> vals = {};
    for (auto& val : vals)
        val = static_cast<float>(static_cast<int32_t>(GetARand()));
    return vals;
}();

alignas(32) static const std::array<double, 8> RandValsF64 = []()
{
    std::array<double, 8> vals = {};
    for (auto& val : vals)
        val = static_cast<double>(static_cast<int32_t>(GetARand()));
    return vals;
}();

template<typename T>
static const T* GetRandPtr() noexcept
{
    using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
    if constexpr (std::is_same_v<U, double>)
        return reinterpret_cast<const T*>(RandValsF64.data());
    else if constexpr (std::is_same_v<U, float>)
        return reinterpret_cast<const T*>(RandValsF32.data());
    else
        return reinterpret_cast<const T*>(RandVals.data());
}

template<typename T>
static void TestAdd()
{
    const auto ptr = GetRandPtr<T>();
    const auto data1 = ptr[0], data2 = ptr[1];
    using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
    const auto output = data1.Add(data2);
    U ref[T::Count] = { 0 };
    for (uint8_t i = 0; i < T::Count; ++i)
        ref[i] = data1.Val[i] + data2.Val[i];
    EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
}

template<typename T>
static void TestSub()
{
    const auto ptr = GetRandPtr<T>();
    const auto data1 = ptr[0], data2 = ptr[1];
    using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
    const auto output = data1.Sub(data2);
    U ref[T::Count] = { 0 };
    for (uint8_t i = 0; i < T::Count; ++i)
        ref[i] = data1.Val[i] - data2.Val[i];
    EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
}

template<typename T>
static void TestMul()
{
    const auto ptr = GetRandPtr<T>();
    const auto data1 = ptr[0], data2 = ptr[1];
    using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
    const auto output = data1.Mul(data2);
    U ref[T::Count] = { 0 };
    for (uint8_t i = 0; i < T::Count; ++i)
        ref[i] = data1.Val[i] * data2.Val[i];
    EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
}

template<typename T>
static void TestMulLo()
{
    const auto ptr = GetRandPtr<T>();
    const auto data1 = ptr[0], data2 = ptr[1];
    using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
    using V = Upcast<U>::U;
    const auto output = data1.MulLo(data2);
    U ref[T::Count] = { 0 };
    for (uint8_t i = 0; i < T::Count; ++i)
        ref[i] = static_cast<U>(static_cast<V>(data1.Val[i]) * static_cast<V>(data2.Val[i]));
    EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
}

template<typename T>
static void TestMulHi()
{
    const auto ptr = GetRandPtr<T>();
    const auto data1 = ptr[0], data2 = ptr[1];
    using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
    using V = Upcast<U>::U;
    const auto output = data1.MulHi(data2);
    U ref[T::Count] = { 0 };
    for (uint8_t i = 0; i < T::Count; ++i)
        ref[i] = static_cast<U>((static_cast<V>(data1.Val[i]) * static_cast<V>(data2.Val[i])) >> (sizeof(U) * 8));
    EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
}

template<typename T>
static void TestDiv()
{
    const auto ptr = GetRandPtr<T>();
    const auto data1 = ptr[0], data2 = ptr[1];
    using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
    const auto output = data1.Div(data2);
    U ref[T::Count] = { 0 };
    for (uint8_t i = 0; i < T::Count; ++i)
        ref[i] = data1.Val[i] / data2.Val[i];
    EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
}

template<typename T>
static void TestAnd()
{
    const auto ptr = GetRandPtr<T>();
    const auto data1 = ptr[0], data2 = ptr[1];
    using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
    const auto output = data1.And(data2);
    U ref[T::Count] = { 0 };
    for (uint8_t i = 0; i < T::Count; ++i)
        ref[i] = data1.Val[i] & data2.Val[i];
    EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
}

template<typename T>
static void TestOr()
{
    const auto ptr = GetRandPtr<T>();
    const auto data1 = ptr[0], data2 = ptr[1];
    using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
    const auto output = data1.Or(data2);
    U ref[T::Count] = { 0 };
    for (uint8_t i = 0; i < T::Count; ++i)
        ref[i] = data1.Val[i] | data2.Val[i];
    EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
}

template<typename T>
static void TestXor()
{
    const auto ptr = GetRandPtr<T>();
    const auto data1 = ptr[0], data2 = ptr[1];
    using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
    const auto output = data1.Xor(data2);
    U ref[T::Count] = { 0 };
    for (uint8_t i = 0; i < T::Count; ++i)
        ref[i] = data1.Val[i] ^ data2.Val[i];
    EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
}

template<typename T>
static void TestAndNot()
{
    const auto ptr = GetRandPtr<T>();
    const auto data1 = ptr[0], data2 = ptr[1];
    using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
    const auto output = data1.AndNot(data2);
    U ref[T::Count] = { 0 };
    for (uint8_t i = 0; i < T::Count; ++i)
        ref[i] = (~data1.Val[i]) & data2.Val[i];
    EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
}

template<typename T>
static void TestNot()
{
    const auto ptr = GetRandPtr<T>();
    const auto data = ptr[0];
    using U = std::decay_t<decltype(std::declval<T>().Val[0])>;
    const auto output = data.Not();
    U ref[T::Count] = { 0 };
    for (uint8_t i = 0; i < T::Count; ++i)
        ref[i] = ~data.Val[i];
    EXPECT_THAT(output.Val, testing::ElementsAreArray(ref));
}


enum class TestItem : uint32_t
{
    Add = 0x1, Sub = 0x2, Mul = 0x4, MulLo = 0x8, MulHi = 0x10, Div = 0x20,
    And = 0x100, Or = 0x200, Xor = 0x400, AndNot = 0x800, Not = 0x1000,
};
MAKE_ENUM_BITFIELD(TestItem)

template<typename T, TestItem Items>
class SIMDBaseTest : public SIMDFixture
{
public:
    static constexpr auto TestSuite = "SIMDBase";
    void TestBody() override
    {
#define AddItem(r, data, x) if constexpr (HAS_FIELD(Items, TestItem::x)) BOOST_PP_CAT(Test,x)<T>();
#define AddItems(...) BOOST_PP_SEQ_FOR_EACH(AddItem, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
        AddItems(Add, Sub, Mul, MulLo, MulHi, Div, And, Or, Xor, AndNot, Not)
#undef AddItems
#undef AddItem
    }
};


#define SIMDBaseItem(r, data, i, x) BOOST_PP_IF(i, |, ) simdtest::TestItem::x
#define RegisterSIMDBaseTest(type, lv, ...)  RegisterSIMDTest(#type, lv, \
simdtest::SIMDBaseTest<common::simd::type, BOOST_PP_SEQ_FOR_EACH_I(SIMDBaseItem, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))>)


}