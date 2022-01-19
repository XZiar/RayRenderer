#include "pch.h"
namespace
{
using namespace common;
#define COMMON_SIMD_LV 100
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ >= 100
#   include "common/simd/SIMD128.hpp"
using namespace common::simd;
#   include "SIMDBaseTest.h"

using namespace common::simd;


RegisterSIMDCastTest(I64x2, 100, I8x16,        I16x8,        I32x4                            );
RegisterSIMDCastTest(U64x2, 100,        U8x16,        U16x8,        U32x4                     );
RegisterSIMDBaseTest(F32x4, 100, Rnd);


#endif
}
