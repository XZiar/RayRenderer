#include "pch.h"
namespace
{
using namespace common;
#define COMMON_SIMD_LV 200
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ >= 200
#   include "common/simd/SIMD128.hpp"
using namespace common::simd;
#   include "SIMDBaseTest.h"
#   include "ShuffleTest.h"
#   include "DotProdTest.h"


RegisterSIMDBaseTest(F64x2, 200, SEL, Add, Sub, Mul, Div, Neg, Abs, Min, Max);
RegisterSIMDBaseTest(F32x4, 200, SEL,                Div);
RegisterSIMDBaseTest(I64x2, 200,                          Abs, Min, Max);
RegisterSIMDBaseTest(U64x2, 200,                          Abs, Min, Max);
RegisterSIMDBaseTest(I32x4, 200, MulX);
RegisterSIMDBaseTest(U32x4, 200, MulX);
RegisterSIMDBaseTest(I16x8, 200, MulX);
RegisterSIMDBaseTest(U16x8, 200, MulX);
RegisterSIMDBaseTest(I8x16, 200, MulX);
RegisterSIMDBaseTest(U8x16, 200, MulX);


RegisterSIMDCastTest(F32x4, 200, F64x2);
RegisterSIMDCastTest(F64x2, 200, F32x4);
RegisterSIMDCastTest(I64x2, 200, I8x16,        I16x8,        I32x4                            );
RegisterSIMDCastTest(U64x2, 200,        U8x16,        U16x8,        U32x4                     );
RegisterSIMDCastTest(I32x4, 200, I8x16,                                    I64x2, U64x2, F64x2);
RegisterSIMDCastTest(U32x4, 200,        U8x16,        U16x8,               I64x2, U64x2, F64x2);
RegisterSIMDCastTest(I16x8, 200, I8x16,                      I32x4, U32x4,               F64x2);
RegisterSIMDCastTest(U16x8, 200,        U8x16,               I32x4, U32x4,               F64x2);
RegisterSIMDCastTest(I8x16, 200,               I16x8, U16x8,                             F64x2);
RegisterSIMDCastTest(U8x16, 200,               I16x8, U16x8,                             F64x2);


RegisterSIMDCastModeTest(F32x4, 100, RangeSaturate, I32x4, I16x8, I8x16);
RegisterSIMDCastModeTest(I64x2, 100, RangeSaturate, I32x4, U32x4);
RegisterSIMDCastModeTest(U64x2, 100, RangeSaturate, U32x4);
RegisterSIMDCastModeTest(I32x4, 100, RangeSaturate, I16x8, U16x8);
RegisterSIMDCastModeTest(U32x4, 100, RangeSaturate, U16x8);
RegisterSIMDCastModeTest(I16x8, 100, RangeSaturate, I8x16, U8x16);
RegisterSIMDCastModeTest(U16x8, 100, RangeSaturate, U8x16);


RegisterSIMDCmpTest(200, F64x2, I64x2, U64x2)


RegisterSIMDZipTest(200, F64x2, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDBroadcastTest(200, F64x2, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDTest(F64x2, 200, shuftest::ShuffleTest<F64x2>);
RegisterSIMDTest(U32x4, 200, shuftest::ShuffleTest<U32x4>);


RegisterSIMDTest(F64x2, 200, shuftest::ShuffleVarTest<F64x2>);
RegisterSIMDTest(U32x4, 200, shuftest::ShuffleVarTest<U32x4>);
RegisterSIMDTest(U16x8, 200, shuftest::ShuffleVarTest<U16x8>);


RegisterSIMDTest(F32x4, 200, dottest::DotProdTest<F32x4>);


#endif
}
