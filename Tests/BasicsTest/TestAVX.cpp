#include "pch.h"
namespace
{
using namespace common;
#define COMMON_SIMD_LV 100
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ >= 100
#   include "common/simd/SIMD128.hpp"
#   include "common/simd/SIMD256.hpp"
using namespace common::simd;
#   include "SIMDBaseTest.h"
#   include "ShuffleTest.h"


RegisterSIMDBaseTest(F64x4, 100, Add, Sub, Mul, Div, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(F32x8, 100, Add, Sub, Mul, Div, Neg, Abs, Min, Max)


RegisterSIMDCastTest(I32x8,  100,               F32x8, F64x4)
RegisterSIMDCastTest(U32x8,  100, I64x4, U64x4)


RegisterSIMDCastModeTest(F32x8, 100, RangeSaturate, I32x8)


//RegisterSIMDTest("F64x4", 100, simdtest::SIMDCompareTest<F64x4>)
//RegisterSIMDTest("F32x8", 100, simdtest::SIMDCompareTest<F32x8>)
//RegisterSIMDTest("F64x2", 100, simdtest::SIMDCompareTest<F64x2>)
//RegisterSIMDTest("F32x4", 100, simdtest::SIMDCompareTest<F32x4>)

RegisterSIMDCmpTest(100, F64x4, F32x8, F64x2, F32x4)


RegisterSIMDTest(F64x4, 100, shuftest::ShuffleTest<F64x4>)
RegisterSIMDTest(F64x2, 100, shuftest::ShuffleTest<F64x2>)
RegisterSIMDTest(I64x2, 100, shuftest::ShuffleTest<I64x2>)
RegisterSIMDTest(F32x4, 100, shuftest::ShuffleTest<F32x4>)
RegisterSIMDTest(I32x4, 100, shuftest::ShuffleTest<I32x4>)


RegisterSIMDTest(F64x4, 100, shuftest::ShuffleVarTest<F64x4>)
RegisterSIMDTest(F32x8, 100, shuftest::ShuffleVarTest<F32x8>)
RegisterSIMDTest(F64x2, 100, shuftest::ShuffleVarTest<F64x2>)
RegisterSIMDTest(I64x2, 100, shuftest::ShuffleVarTest<I64x2>)
RegisterSIMDTest(F32x4, 100, shuftest::ShuffleVarTest<F32x4>)
RegisterSIMDTest(I32x4, 100, shuftest::ShuffleVarTest<I32x4>)


#endif
}
