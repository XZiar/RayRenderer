#include "pch.h"

#if COMMON_COMPILER_GCC
#   pragma GCC push_options
#   pragma GCC target("arch=armv8-a+crc+simd+crypto")
#elif COMMON_COMPILER_CLANG
#   pragma clang attribute push (__attribute__((target("arch=armv8-a+crc+simd+crypto"))), apply_to=function)
#endif

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 100
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"

namespace a32
{
using namespace common;
using namespace common::simd;
using namespace COMMON_SIMD_NAMESPACE;
#include "SIMDBaseTest.h"


RegisterSIMDCastTest(I64x2, 100, I8x16,        I16x8,        I32x4                            );
RegisterSIMDCastTest(U64x2, 100,        U8x16,        U16x8,        U32x4                     );
RegisterSIMDBaseTest(F32x4, 100, Rnd);


}

#if COMMON_COMPILER_GCC
#   pragma GCC pop_options
#elif COMMON_COMPILER_CLANG
#   pragma clang attribute pop
#endif
