#include "pch.h"
#define COMMON_SIMD_LV 100
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ >= 100
#   include "CopyTest.h"
#   include "ShuffleTest.h"


RegisterSIMDTest("u32", 100, copytest::BroadcastTest<uint32_t>)

RegisterSIMDTest("u16", 100, copytest::BroadcastTest<uint16_t>)

RegisterSIMDTest("u8",  100, copytest::BroadcastTest< uint8_t>)



RegisterSIMDTest("F64x4", 100, shuftest::ShuffleTest<common::simd::F64x4>)
//RegisterSIMDTest("I64x4", 100, shuftest::ShuffleTest<common::simd::I64x4>) // No Intergel xmm in AVX
RegisterSIMDTest("F64x2", 100, shuftest::ShuffleTest<common::simd::F64x2>)
RegisterSIMDTest("I64x2", 100, shuftest::ShuffleTest<common::simd::I64x2>)
RegisterSIMDTest("F32x4", 100, shuftest::ShuffleTest<common::simd::F32x4>)
RegisterSIMDTest("I32x4", 100, shuftest::ShuffleTest<common::simd::I32x4>)


RegisterSIMDTest("F64x4", 100, shuftest::ShuffleVarTest<common::simd::F64x4>)
RegisterSIMDTest("F32x8", 100, shuftest::ShuffleVarTest<common::simd::F32x8>)
//RegisterSIMDTest("I64x4", 100, shuftest::ShuffleVarTest<common::simd::I64x4>) // No Intergel xmm in AVX
RegisterSIMDTest("F64x2", 100, shuftest::ShuffleVarTest<common::simd::F64x2>)
RegisterSIMDTest("I64x2", 100, shuftest::ShuffleVarTest<common::simd::I64x2>)
RegisterSIMDTest("F32x4", 100, shuftest::ShuffleVarTest<common::simd::F32x4>)
RegisterSIMDTest("I32x4", 100, shuftest::ShuffleVarTest<common::simd::I32x4>)


#endif
