#include "pch.h"

#if COMMON_COMPILER_GCC
#   pragma GCC push_options
#   pragma GCC target("avx512f", "avx512vl", "avx512bw", "avx512dq")
#elif COMMON_COMPILER_CLANG
#   pragma clang attribute push (__attribute__((target("avx512f,avx512vl,avx512bw,avx512dq"))), apply_to=function)
#endif

namespace avx512
{
using namespace common;
#define COMMON_SIMD_LV 320
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ >= 310
#   include "common/simd/SIMD128.hpp"
#   include "common/simd/SIMD256.hpp"
using namespace common::simd;
#   include "SIMDBaseTest.h"


RegisterSIMDBaseTest(I64x4,  320, SEL,                 MulLo, Min, Max, SRA);
RegisterSIMDBaseTest(U64x4,  320, SEL, SatAdd, SatSub, MulLo, Min, Max);
RegisterSIMDBaseTest(I64x2,  320, SEL,                 MulLo, Min, Max, SRA);
RegisterSIMDBaseTest(U64x2,  320, SEL, SatAdd, SatSub, MulLo, Min, Max);


RegisterSIMDCastTest(U32x4,  320, F64x2);
RegisterSIMDCastTest(U32x8,  320, F64x4, U8x32);

#endif
}

#if COMMON_COMPILER_GCC
#   pragma GCC pop_options
#elif COMMON_COMPILER_CLANG
#   pragma clang attribute pop
#endif
