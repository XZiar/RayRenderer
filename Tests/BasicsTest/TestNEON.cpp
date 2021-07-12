#include "pch.h"
#define COMMON_SIMD_LV 100
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#if COMMON_SIMD_LV_ >= 100
#   include "SIMDBaseTest.h"
#   include "ShuffleTest.h"
#   include "DotProdTest.h"

using namespace common::simd;


//RegisterSIMDTest("u32_u32", 42, copytest::CopyTest<uint32_t, uint32_t>)
//
//RegisterSIMDTest("u32_u16", 42, copytest::CopyTest<uint32_t, uint16_t>)
//
//RegisterSIMDTest("u32_u8",  42, copytest::CopyTest<uint32_t, uint8_t>)
//
//RegisterSIMDTest("u16_u8",  42, copytest::CopyTest<uint16_t, uint8_t>)


//RegisterSIMDTest("u32", 42, copytest::BroadcastTest<uint32_t>)
//
//RegisterSIMDTest("u16", 42, copytest::BroadcastTest<uint16_t>)
//
//RegisterSIMDTest("u8",  42, copytest::BroadcastTest< uint8_t>)


RegisterSIMDBaseTest(F32x4, 100, Add, Sub, Mul, Div, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(I64x2, 100, Add, Sub, SatAdd, SatSub, Neg, Abs, And, Or, Xor, AndNot, Not, Min, Max)
RegisterSIMDBaseTest(U64x2, 100, Add, Sub, SatAdd, SatSub, Abs, Min, Max)
RegisterSIMDBaseTest(I32x4, 100, Add, Sub, SatAdd, SatSub, MulLo, MulX, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(U32x4, 100, Add, Sub, SatAdd, SatSub, MulLo, MulX, Abs, Min, Max)
RegisterSIMDBaseTest(I16x8, 100, Add, Sub, SatAdd, SatSub, MulLo, MulX, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(U16x8, 100, Add, Sub, SatAdd, SatSub, MulLo, MulX, Abs, Min, Max)
RegisterSIMDBaseTest(I8x16, 100, Add, Sub, SatAdd, SatSub, MulLo, MulX, Neg, Abs, Min, Max)
RegisterSIMDBaseTest(U8x16, 100, Add, Sub, SatAdd, SatSub, MulLo, MulX, Abs, Min, Max)


RegisterSIMDCastTest(I8x16, 100, I16x8, U16x8, I32x4, U32x4, I64x2, U64x2);
RegisterSIMDCastTest(U8x16, 100, I16x8, U16x8, I32x4, U32x4, I64x2, U64x2);
RegisterSIMDCastTest(I16x8, 100,               I32x4, U32x4, I64x2, U64x2);
RegisterSIMDCastTest(U16x8, 100,               I32x4, U32x4, I64x2, U64x2);
RegisterSIMDCastTest(I32x4, 100,                             I64x2, U64x2);
RegisterSIMDCastTest(U32x4, 100,                             I64x2, U64x2);

RegisterSIMDTest("F32x4", 100, shuftest::ShuffleTest<F32x4>)


RegisterSIMDTest("F32x4", 100, shuftest::ShuffleVarTest<F32x4>)


RegisterSIMDTest("F32x4", 100, dottest::DotProdTest<F32x4>)


#endif

