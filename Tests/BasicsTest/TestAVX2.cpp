#include "pch.h"
#define COMMON_SIMD_LV 200
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#include "common/simd/SIMD256.hpp"
#if COMMON_SIMD_LV_ >= 200
#   include "CopyTest.h"
#   include "SIMDBaseTest.h"
#   include "ShuffleTest.h"


RegisterSIMDTest("u32_u32", 200, copytest::CopyTest<uint32_t, uint32_t>)

RegisterSIMDTest("u32_u16", 200, copytest::CopyTest<uint32_t, uint16_t>)

RegisterSIMDTest("u32_u8",  200, copytest::CopyTest<uint32_t, uint8_t>)

RegisterSIMDTest("u16_u8",  200, copytest::CopyTest<uint16_t, uint8_t>)


RegisterSIMDBaseTest(I64x4,  200, Add, Sub, And, Or, Xor, AndNot, Not)
RegisterSIMDBaseTest(I32x8,  200, Add, Sub, MulLo)
RegisterSIMDBaseTest(U32x8,  200, Add, Sub, MulLo)
RegisterSIMDBaseTest(I16x16, 200, Add, Sub, MulLo, MulHi)
RegisterSIMDBaseTest(U16x16, 200, Add, Sub, MulLo, MulHi)
RegisterSIMDBaseTest(I8x32,  200, Add, Sub, MulLo, MulHi)
RegisterSIMDBaseTest(U8x32,  200, Add, Sub, MulLo, MulHi)


RegisterSIMDTest("F64x4", 200, shuftest::ShuffleTest<common::simd::F64x4>)
RegisterSIMDTest("I64x4", 200, shuftest::ShuffleTest<common::simd::I64x4>)
RegisterSIMDTest("F64x2", 200, shuftest::ShuffleTest<common::simd::F64x2>)
RegisterSIMDTest("I64x2", 200, shuftest::ShuffleTest<common::simd::I64x2>)
RegisterSIMDTest("F32x4", 200, shuftest::ShuffleTest<common::simd::F32x4>)
RegisterSIMDTest("I32x4", 200, shuftest::ShuffleTest<common::simd::I32x4>)


RegisterSIMDTest("F64x4", 200, shuftest::ShuffleVarTest<common::simd::F64x4>)
RegisterSIMDTest("I64x4", 200, shuftest::ShuffleVarTest<common::simd::I64x4>)
RegisterSIMDTest("F32x8", 200, shuftest::ShuffleVarTest<common::simd::F32x8>)
RegisterSIMDTest("I32x8", 200, shuftest::ShuffleVarTest<common::simd::I32x8>)
RegisterSIMDTest("F64x2", 200, shuftest::ShuffleVarTest<common::simd::F64x2>)
RegisterSIMDTest("I64x2", 200, shuftest::ShuffleVarTest<common::simd::I64x2>)
RegisterSIMDTest("F32x4", 200, shuftest::ShuffleVarTest<common::simd::F32x4>)
RegisterSIMDTest("I32x4", 200, shuftest::ShuffleVarTest<common::simd::I32x4>)


#endif
