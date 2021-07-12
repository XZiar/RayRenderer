#include "pch.h"
#define COMMON_SIMD_LV 42
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#if COMMON_SIMD_LV_ >= 42
#   include "SIMDBaseTest.h"
#   include "ShuffleTest.h"
#   include "DotProdTest.h"

using namespace common::simd;


RegisterSIMDBaseTest(F64x2, 42, Add, Sub, Mul, Div, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(F32x4, 42, Add, Sub, Mul, Div, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(I64x2, 42, Add, Sub, MulLo, Neg, Abs, And, Or, Xor, AndNot, Not, Min, Max)
RegisterSIMDBaseTest(U64x2, 42, Add, Sub, SatAdd, SatSub, MulLo, Abs, Min, Max)
RegisterSIMDBaseTest(I32x4, 42, Add, Sub, MulLo, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(U32x4, 42, Add, Sub, SatAdd, SatSub, MulLo, Abs, Min, Max)
RegisterSIMDBaseTest(I16x8, 42, Add, Sub, SatAdd, SatSub, MulLo, MulHi, MulX, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(U16x8, 42, Add, Sub, SatAdd, SatSub, MulLo, MulHi, MulX, Abs, Min, Max)
RegisterSIMDBaseTest(I8x16, 42, Add, Sub, SatAdd, SatSub, MulLo, MulHi, MulX, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(U8x16, 42, Add, Sub, SatAdd, SatSub, MulLo, MulHi, MulX, Abs, Min, Max)


RegisterSIMDCastTest(I8x16, 42, I16x8, U16x8, I32x4, U32x4, I64x2, U64x2)
RegisterSIMDCastTest(U8x16, 42, I16x8, U16x8, I32x4, U32x4, I64x2, U64x2)
RegisterSIMDCastTest(I16x8, 42,               I32x4, U32x4, I64x2, U64x2)
RegisterSIMDCastTest(U16x8, 42,               I32x4, U32x4, I64x2, U64x2)
RegisterSIMDCastTest(I32x4, 42,                             I64x2, U64x2)
RegisterSIMDCastTest(U32x4, 42,                             I64x2, U64x2)


RegisterSIMDTest("F64x2", 42, shuftest::ShuffleTest<F64x2>)
RegisterSIMDTest("I64x2", 42, shuftest::ShuffleTest<I64x2>)
RegisterSIMDTest("F32x4", 42, shuftest::ShuffleTest<F32x4>)
RegisterSIMDTest("I32x4", 42, shuftest::ShuffleTest<I32x4>)


RegisterSIMDTest("F32x4", 42, dottest::DotProdTest<F32x4>)


#endif

