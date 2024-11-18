#include "SIMDRely.h"

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 320
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ < COMMON_SIMD_LV
#   error requires SIMDLV >= 320
#endif
#include "common/simd/SIMD128.hpp"
#include "common/simd/SIMD256.hpp"

namespace avx512
{
using namespace common;
using namespace common::simd;
using namespace COMMON_SIMD_NAMESPACE;
#include "SIMDBaseTest.h"
#include "ShuffleTest.h"


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


RegisterSIMDCastTest(U32x8,  320, F32x8, F64x4, U8x32);
RegisterSIMDCastTest(U32x4,  320, F32x4, F64x2);

RegisterSIMDSelectRandTest(320, I16x16, I8x32, I16x8, I8x16)

}
