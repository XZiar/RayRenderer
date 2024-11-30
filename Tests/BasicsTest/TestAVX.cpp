#include "SIMDRely.h"

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 100
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ < COMMON_SIMD_LV
#   error requires SIMDLV >= 100
#endif
#include "common/simd/SIMD128.hpp"
#include "common/simd/SIMD256.hpp"

namespace avx
{
using namespace common;
using namespace common::simd;
using namespace COMMON_SIMD_NAMESPACE;
#include "SIMDBaseTest.h"
#include "ShuffleTest.h"


RegisterSIMDBaseTest(F64x4, 100, SEL, Add, Sub, Mul, Div, Neg, Abs, Min, Max, FMA, Rnd);
RegisterSIMDBaseTest(F32x8, 100, SEL, Add, Sub, Mul, Div, Neg, Abs, Min, Max, FMA, Rnd);

RegisterSIMDBaseTest(I64x4,  100, Load);
RegisterSIMDBaseTest(I32x8,  100, Load);
RegisterSIMDBaseTest(I16x16, 100, Load);


RegisterSIMDCastTest(F16x8,  100, F32x4);
RegisterSIMDCastTest(F32x4,  100, F16x8);
RegisterSIMDCastTest(I32x8,  100,               F32x8, F64x4);
RegisterSIMDCastTest(U32x8,  100, I64x4, U64x4);


RegisterSIMDCastModeTest(F32x8, 100, RangeSaturate, I32x8);


RegisterSIMDCmpTest(100, F64x4, F32x8, F64x2, F32x4)


RegisterSIMDBroadcastTest(100, F64x4, F32x8, F64x2, F32x4)
RegisterSIMDBroadcastLaneTest(100, F64x4, F32x8)


RegisterSIMDSelectTest(100, F64x4, F32x8)


RegisterSIMDTest(F64x4, 100, shuftest::ShuffleTest<F64x4>);
RegisterSIMDTest(F64x2, 100, shuftest::ShuffleTest<F64x2>);
RegisterSIMDTest(I64x2, 100, shuftest::ShuffleTest<I64x2>);
RegisterSIMDTest(F32x4, 100, shuftest::ShuffleTest<F32x4>);
RegisterSIMDTest(I32x4, 100, shuftest::ShuffleTest<I32x4>);


RegisterSIMDTest(F64x4, 100, shuftest::ShuffleVarTest<F64x4>);
RegisterSIMDTest(F32x8, 100, shuftest::ShuffleVarTest<F32x8>);
RegisterSIMDTest(F64x2, 100, shuftest::ShuffleVarTest<F64x2>);
RegisterSIMDTest(I64x2, 100, shuftest::ShuffleVarTest<I64x2>);
RegisterSIMDTest(F32x4, 100, shuftest::ShuffleVarTest<F32x4>);
RegisterSIMDTest(I32x4, 100, shuftest::ShuffleVarTest<I32x4>);


}
