#include "pch.h"

#if COMMON_COMPILER_GCC
#   pragma GCC push_options
#   pragma GCC target("avx512f,avx512vl,avx512bw,avx512dq,avx2,fma,avx,sse4.2,sse2")
#elif COMMON_COMPILER_CLANG
#   pragma clang attribute push (__attribute__((target("avx512f,avx512vl,avx512bw,avx512dq,avx2,fma,avx,sse4.2,sse2"))), apply_to=function)
#endif

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 320
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#include "common/simd/SIMD256.hpp"

namespace avx512
{
using namespace common;
using namespace common::simd;
using namespace COMMON_SIMD_NAMESPACE;
#include "SIMDBaseTest.h"


RegisterSIMDBaseTest(I64x4,  320, SatAdd, SatSub, MulLo, Min, Max, SRA);
RegisterSIMDBaseTest(U64x4,  320, SatAdd, SatSub, MulLo, Min, Max);
RegisterSIMDBaseTest(I64x2,  320, SatAdd, SatSub, MulLo, Min, Max, SRA);
RegisterSIMDBaseTest(U64x2,  320, SatAdd, SatSub, MulLo, Min, Max);
RegisterSIMDBaseTest(I32x8,  320, SatAdd, SatSub);
RegisterSIMDBaseTest(I32x4,  320, SatAdd, SatSub);
RegisterSIMDBaseTest(I16x16, 320, SLLV, SRLV);
RegisterSIMDBaseTest(U16x16, 320, SLLV, SRLV);
RegisterSIMDBaseTest(I16x8,  320, SLLV, SRLV);
RegisterSIMDBaseTest(U16x8,  320, SLLV, SRLV);


RegisterSIMDCastTest(U32x8,  320, F64x4, U8x32);
RegisterSIMDCastTest(U32x4,  320, F64x2);

}

#if COMMON_COMPILER_GCC
#   pragma GCC pop_options
#elif COMMON_COMPILER_CLANG
#   pragma clang attribute pop
#endif
