#include "pch.h"
#define COMMON_SIMD_LV 42
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#if COMMON_SIMD_LV_ >= 42
#   include "CopyTest.h"
#   include "SIMDBaseTest.h"
#   include "ShuffleTest.h"
#   include "DotProdTest.h"


RegisterSIMDTest("u32_u32", 42, copytest::CopyTest<uint32_t, uint32_t>)

RegisterSIMDTest("u32_u16", 42, copytest::CopyTest<uint32_t, uint16_t>)

RegisterSIMDTest("u32_u8",  42, copytest::CopyTest<uint32_t, uint8_t>)

RegisterSIMDTest("u16_u8",  42, copytest::CopyTest<uint16_t, uint8_t>)


RegisterSIMDTest("u32", 42, copytest::BroadcastTest<uint32_t>)

RegisterSIMDTest("u16", 42, copytest::BroadcastTest<uint16_t>)

RegisterSIMDTest("u8",  42, copytest::BroadcastTest< uint8_t>)


RegisterSIMDBaseTest(F64x2, 42, Add, Sub, Mul, Div)
RegisterSIMDBaseTest(F32x4, 42, Add, Sub, Mul, Div)
RegisterSIMDBaseTest(I64x2, 42, Add, Sub, And, Or, Xor, AndNot, Not)
RegisterSIMDBaseTest(I32x4, 42, Add, Sub, MulLo)
RegisterSIMDBaseTest(U32x4, 42, Add, Sub, MulLo)
RegisterSIMDBaseTest(I16x8, 42, Add, Sub, MulLo, MulHi)
RegisterSIMDBaseTest(U16x8, 42, Add, Sub, MulLo, MulHi)
RegisterSIMDBaseTest(I8x16, 42, Add, Sub, MulLo, MulHi)
RegisterSIMDBaseTest(U8x16, 42, Add, Sub, MulLo, MulHi)


RegisterSIMDTest("F64x2", 42, shuftest::ShuffleTest<common::simd::F64x2>)
RegisterSIMDTest("I64x2", 42, shuftest::ShuffleTest<common::simd::I64x2>)
RegisterSIMDTest("F32x4", 42, shuftest::ShuffleTest<common::simd::F32x4>)
RegisterSIMDTest("I32x4", 42, shuftest::ShuffleTest<common::simd::I32x4>)


RegisterSIMDTest("F32x4", 42, dottest::DotProdTest<common::simd::F32x4>)


#endif

