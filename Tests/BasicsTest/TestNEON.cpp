#include "pch.h"
#define COMMON_SIMD_LV 100
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#if COMMON_SIMD_LV_ >= 100
#   include "SIMDBaseTest.h"
#   include "ShuffleTest.h"
#   include "DotProdTest.h"

using namespace common::simd;


RegisterSIMDBaseTest(F32x4, 100, Add, Sub, Mul, Div, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(I64x2, 100, Add, Sub, SatAdd, SatSub, Neg, Abs, And, Or, Xor, AndNot, Not, Min, Max)
RegisterSIMDBaseTest(U64x2, 100, Add, Sub, SatAdd, SatSub, Abs, Min, Max)
RegisterSIMDBaseTest(I32x4, 100, Add, Sub, SatAdd, SatSub, MulLo, MulX, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(U32x4, 100, Add, Sub, SatAdd, SatSub, MulLo, MulX, Abs, Min, Max)
RegisterSIMDBaseTest(I16x8, 100, Add, Sub, SatAdd, SatSub, MulLo, MulX, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(U16x8, 100, Add, Sub, SatAdd, SatSub, MulLo, MulX, Abs, Min, Max)
RegisterSIMDBaseTest(I8x16, 100, Add, Sub, SatAdd, SatSub, MulLo, MulX, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(U8x16, 100, Add, Sub, SatAdd, SatSub, MulLo, MulX, Abs, Min, Max)


RegisterSIMDCastTest(I32x4, 100,                                    I64x2, U64x2, F32x4);
RegisterSIMDCastTest(U32x4, 100, U8x16,        U16x8,               I64x2, U64x2, F32x4);
RegisterSIMDCastTest(I16x8, 100,                      I32x4, U32x4, I64x2, U64x2, F32x4);
RegisterSIMDCastTest(U16x8, 100, U8x16,               I32x4, U32x4, I64x2, U64x2, F32x4);
RegisterSIMDCastTest(I8x16, 100,        I16x8, U16x8, I32x4, U32x4, I64x2, U64x2, F32x4);
RegisterSIMDCastTest(U8x16, 100,        I16x8, U16x8, I32x4, U32x4, I64x2, U64x2, F32x4);


RegisterSIMDCastModeTest(F32x4, 100, RangeSaturate, I32x4, I16x8, I8x16)
RegisterSIMDCastModeTest(I64x2, 100, RangeSaturate, I32x4, U32x4)
RegisterSIMDCastModeTest(U64x2, 100, RangeSaturate, U32x4)
RegisterSIMDCastModeTest(I32x4, 100, RangeSaturate, I16x8, U16x8)
RegisterSIMDCastModeTest(U32x4, 100, RangeSaturate, U16x8)
RegisterSIMDCastModeTest(I16x8, 100, RangeSaturate, I8x16, U8x16)
RegisterSIMDCastModeTest(U16x8, 100, RangeSaturate, U8x16)


RegisterSIMDTest("F32x4", 100, simdtest::SIMDCompareTest<F32x4>)
RegisterSIMDTest("I64x2", 100, simdtest::SIMDCompareTest<I64x2>)
RegisterSIMDTest("U64x2", 100, simdtest::SIMDCompareTest<U64x2>)
RegisterSIMDTest("I32x4", 100, simdtest::SIMDCompareTest<I32x4>)
RegisterSIMDTest("U32x4", 100, simdtest::SIMDCompareTest<U32x4>)
RegisterSIMDTest("I16x8", 100, simdtest::SIMDCompareTest<I16x8>)
RegisterSIMDTest("U16x8", 100, simdtest::SIMDCompareTest<U16x8>)
RegisterSIMDTest("I8x16", 100, simdtest::SIMDCompareTest<I8x16>)
RegisterSIMDTest("U8x16", 100, simdtest::SIMDCompareTest<U8x16>)


RegisterSIMDTest("F32x4", 100, shuftest::ShuffleTest<F32x4>)


RegisterSIMDTest("F32x4", 100, shuftest::ShuffleVarTest<F32x4>)


RegisterSIMDTest("F32x4", 100, dottest::DotProdTest<F32x4>)


#endif

