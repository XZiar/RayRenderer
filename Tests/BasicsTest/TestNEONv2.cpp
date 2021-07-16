#include "pch.h"
#define COMMON_SIMD_LV 200
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#if COMMON_SIMD_LV_ >= 200
#   include "SIMDBaseTest.h"
#   include "ShuffleTest.h"

using namespace common::simd;


RegisterSIMDBaseTest(F64x2, 200, Add, Sub, Mul, Div, Neg, Abs, Min, Max)


RegisterSIMDCastTest(F32x4, 200, F64x2)
RegisterSIMDCastTest(F64x2, 200, F32x4)
RegisterSIMDCastTest(I32x4, 200,                                    I64x2, U64x2, F64x2);
RegisterSIMDCastTest(U32x4, 200, U8x16,        U16x8,               I64x2, U64x2, F64x2);
RegisterSIMDCastTest(I16x8, 200,                      I32x4, U32x4,               F64x2);
RegisterSIMDCastTest(U16x8, 200, U8x16,               I32x4, U32x4,               F64x2);
RegisterSIMDCastTest(I8x16, 200,        I16x8, U16x8,                             F64x2);
RegisterSIMDCastTest(U8x16, 200,        I16x8, U16x8,                             F64x2);


RegisterSIMDTest("F64x2", 200, simdtest::SIMDCompareTest<F64x2>)


RegisterSIMDTest("F64x2", 200, shuftest::ShuffleTest<F64x2>)


RegisterSIMDTest("F64x2", 200, shuftest::ShuffleVarTest<F64x2>)


#endif

