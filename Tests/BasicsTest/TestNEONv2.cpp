#include "pch.h"
#define COMMON_SIMD_LV 200
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#if COMMON_SIMD_LV_ >= 200
#   include "SIMDBaseTest.h"
#   include "ShuffleTest.h"

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


RegisterSIMDBaseTest(F64x2, 200, Add, Sub, Mul, Div, Neg, Abs, Min, Max)


RegisterSIMDCastTest(I8x16, 200, I16x8, U16x8                            );
RegisterSIMDCastTest(U8x16, 200, I16x8, U16x8                            );
RegisterSIMDCastTest(I16x8, 200,               I32x4, U32x4              );
RegisterSIMDCastTest(U16x8, 200,               I32x4, U32x4              );
RegisterSIMDCastTest(I32x4, 200,                             I64x2, U64x2);
RegisterSIMDCastTest(U32x4, 200,                             I64x2, U64x2);

RegisterSIMDTest("F64x2", 200, shuftest::ShuffleTest<F64x2>)


RegisterSIMDTest("F64x2", 200, shuftest::ShuffleVarTest<F64x2>)


#endif

