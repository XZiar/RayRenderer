#include "pch.h"
#define COMMON_SIMD_LV 200
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#include "common/simd/SIMD256.hpp"
#if COMMON_SIMD_LV_ >= 200
#   include "SIMDBaseTest.h"
#   include "ShuffleTest.h"

using namespace common::simd;


RegisterSIMDBaseTest(I64x4,  200, Add, Sub, MulLo, Neg, Abs, And, Or, Xor, AndNot, Not)
RegisterSIMDBaseTest(U64x4,  200, Add, Sub, SatAdd, SatSub, MulLo, Abs, Min, Max)
RegisterSIMDBaseTest(I32x8,  200, Add, Sub, MulLo, MulX, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(U32x8,  200, Add, Sub, SatAdd, SatSub, MulLo, MulX, Abs, Min, Max)
RegisterSIMDBaseTest(I16x16, 200, Add, Sub, SatAdd, SatSub, MulLo, MulHi, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(U16x16, 200, Add, Sub, SatAdd, SatSub, MulLo, MulHi, Abs, Min, Max)
RegisterSIMDBaseTest(I8x32,  200, Add, Sub, SatAdd, SatSub, MulLo, MulHi, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(U8x32,  200, Add, Sub, SatAdd, SatSub, MulLo, MulHi, Abs, Min, Max)


RegisterSIMDCastTest(I8x32,  200, I16x16, U16x16, I32x8, U32x8, I64x4, U64x4, F32x8, F64x4)
RegisterSIMDCastTest(U8x32,  200, I16x16, U16x16, I32x8, U32x8, I64x4, U64x4, F32x8, F64x4)
RegisterSIMDCastTest(I16x16, 200,                 I32x8, U32x8, I64x4, U64x4, F32x8, F64x4)
RegisterSIMDCastTest(U16x16, 200,                 I32x8, U32x8, I64x4, U64x4, F32x8, F64x4)
RegisterSIMDCastTest(I32x8,  200,                               I64x4, U64x4)
RegisterSIMDCastTest(U32x8,  200,                               I64x4, U64x4, F32x8, F64x4)


RegisterSIMDTest("F64x4", 200, shuftest::ShuffleTest<F64x4>)
RegisterSIMDTest("I64x4", 200, shuftest::ShuffleTest<I64x4>)
RegisterSIMDTest("F64x2", 200, shuftest::ShuffleTest<F64x2>)
RegisterSIMDTest("I64x2", 200, shuftest::ShuffleTest<I64x2>)
RegisterSIMDTest("F32x4", 200, shuftest::ShuffleTest<F32x4>)
RegisterSIMDTest("I32x4", 200, shuftest::ShuffleTest<I32x4>)


RegisterSIMDTest("F64x4", 200, shuftest::ShuffleVarTest<F64x4>)
RegisterSIMDTest("I64x4", 200, shuftest::ShuffleVarTest<I64x4>)
RegisterSIMDTest("F32x8", 200, shuftest::ShuffleVarTest<F32x8>)
RegisterSIMDTest("I32x8", 200, shuftest::ShuffleVarTest<I32x8>)
RegisterSIMDTest("F64x2", 200, shuftest::ShuffleVarTest<F64x2>)
RegisterSIMDTest("I64x2", 200, shuftest::ShuffleVarTest<I64x2>)
RegisterSIMDTest("F32x4", 200, shuftest::ShuffleVarTest<F32x4>)
RegisterSIMDTest("I32x4", 200, shuftest::ShuffleVarTest<I32x4>)


#endif
