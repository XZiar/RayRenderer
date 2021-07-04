#pragma once
#include "pch.h"
#include "common/simd/SIMD128.hpp"
#pragma message("Compiling DotProdTest with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace dottest
{
using common::simd::DotPos;
using namespace std::string_literals;
using namespace std::string_view_literals;


static std::string GenerateMatchStr(const std::array<float, 4>& ref, DotPos mul, DotPos res)
{
    char tmp[256] = { 0 };
    snprintf(tmp, 240, "Mul[%s%s%s%s] Res[%s%s%s%s] to be [%f,%f,%f,%f]",
        (HAS_FIELD(mul, DotPos::X) ? "X" : ""), (HAS_FIELD(mul, DotPos::Y) ? "Y" : ""),
        (HAS_FIELD(mul, DotPos::Z) ? "Z" : ""), (HAS_FIELD(mul, DotPos::W) ? "W" : ""),
        (HAS_FIELD(res, DotPos::X) ? "X" : ""), (HAS_FIELD(res, DotPos::Y) ? "Y" : ""),
        (HAS_FIELD(res, DotPos::Z) ? "Z" : ""), (HAS_FIELD(res, DotPos::W) ? "W" : ""),
        ref[0], ref[1], ref[2], ref[3]);
    return tmp;
}

MATCHER_P3(MatchDotProd, ref, mul, res, GenerateMatchStr(ref, mul, res))
{
    [[maybe_unused]] const auto& tmp = result_listener;
    return arg[0] == ref[0] && arg[1] == ref[1] && arg[2] == ref[2] && arg[3] == ref[3];
}

template<typename T>
forcenoinline void ResultCheck(const T& data, DotPos mul, DotPos res) noexcept
{
    constexpr float prod[4] = { 3 * 3, 5 * 5, 7 * 7, 13 * 13 };
    float sum = 0.f;
    if (HAS_FIELD(mul, DotPos::X)) sum += prod[0];
    if (HAS_FIELD(mul, DotPos::Y)) sum += prod[1];
    if (HAS_FIELD(mul, DotPos::Z)) sum += prod[2];
    if (HAS_FIELD(mul, DotPos::W)) sum += prod[3];
    std::array<float, 4> ref = { 0 };
    if (HAS_FIELD(res, DotPos::X)) ref[0] = sum;
    if (HAS_FIELD(res, DotPos::Y)) ref[1] = sum;
    if (HAS_FIELD(res, DotPos::Z)) ref[2] = sum;
    if (HAS_FIELD(res, DotPos::W)) ref[3] = sum;
    bool match = true;
    for (size_t i = 0; i < 4; ++i)
        match &= (data.Val[i] == ref[i]);
    if (!match)
    {
        EXPECT_THAT(data.Val, MatchDotProd(ref, mul, res));
        //EXPECT_THAT(data.Val, testing::ElementsAreArray(ref));
    }
}

template<typename T, uint64_t Val>
forceinline void RunDotProd(const T& data)
{
    constexpr DotPos Mul = static_cast<DotPos>(Val & 0b1111), Res = static_cast<DotPos>((Val >> 4) & 0b1111);
    const auto output = data.template Dot<Mul, Res>(data);
    ResultCheck(output, Mul, Res);
}

template<typename T, size_t... Vals>
void TestDotProd(const T& data, std::index_sequence<Vals...>)
{
    (..., RunDotProd<T, Vals>(data));
}

template<typename T>
class DotProdTest : public SIMDFixture
{
public:
    static constexpr auto TestSuite = "DotProd";
    void TestBody() override
    {
        const T data(3, 5, 7, 13);
        TestDotProd(data, std::make_index_sequence<0b11111111 + 1>{});
    }
};


}