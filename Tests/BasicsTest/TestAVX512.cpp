#include "pch.h"
namespace
{
using namespace common;
#define COMMON_SIMD_LV 320
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ >= 310
#   include "common/simd/SIMD128.hpp"
#   include "common/simd/SIMD256.hpp"
using namespace common::simd;
#   include "SIMDBaseTest.h"


RegisterSIMDBaseTest(I64x4,  320, MulLo, Min, Max)
RegisterSIMDBaseTest(U64x4,  320, SatAdd, SatSub, MulLo, Min, Max)
RegisterSIMDBaseTest(I64x2,  320, MulLo, Min, Max)
RegisterSIMDBaseTest(U64x2,  320, SatAdd, SatSub, MulLo, Min, Max)


RegisterSIMDCastTest(U32x4,  320, F64x2)
RegisterSIMDCastTest(U32x8,  320, F64x4, U8x32)

#endif
}
