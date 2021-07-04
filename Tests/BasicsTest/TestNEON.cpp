#include "pch.h"
#define COMMON_SIMD_LV 200
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ >= 200
#   include "ShuffleTest.h"
#   include "DotProdTest.h"



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


RegisterSIMDTest("F64x2", 200, shuftest::ShuffleTest<common::simd::F64x2>)
//RegisterSIMDTest("I64x2", 42, shuftest::ShuffleTest<common::simd::I64x2>)
RegisterSIMDTest("F32x4", 100, shuftest::ShuffleTest<common::simd::F32x4>)
//RegisterSIMDTest("I32x4", 42, shuftest::ShuffleTest<common::simd::I32x4>)


RegisterSIMDTest("F64x2", 200, shuftest::ShuffleVarTest<common::simd::F64x2>)
//RegisterSIMDTest("I64x2", 42, shuftest::ShuffleTest<common::simd::I64x2>)
RegisterSIMDTest("F32x4", 100, shuftest::ShuffleVarTest<common::simd::F32x4>)
//RegisterSIMDTest("I32x4", 42, shuftest::ShuffleTest<common::simd::I32x4>)


RegisterSIMDTest("F32x4", 100, dottest::DotProdTest<common::simd::F32x4>)


#endif

