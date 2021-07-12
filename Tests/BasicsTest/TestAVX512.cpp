#include "pch.h"
#define COMMON_SIMD_LV 320
#include "common/simd/SIMD.hpp"
#include "common/simd/SIMD128.hpp"
#include "common/simd/SIMD256.hpp"
#if COMMON_SIMD_LV_ >= 310
#   include "SIMDBaseTest.h"

using namespace common::simd;


RegisterSIMDBaseTest(I64x4,  320, MulLo, Min, Max)
RegisterSIMDBaseTest(U64x4,  320, SatAdd, SatSub, MulLo, Min, Max)
RegisterSIMDBaseTest(I64x2,  320, MulLo, Min, Max)
RegisterSIMDBaseTest(U64x2,  320, SatAdd, SatSub, MulLo, Min, Max)


RegisterSIMDCastTest(U32x4,  320, F64x2)
RegisterSIMDCastTest(U32x8,  320, F64x4)


#endif
