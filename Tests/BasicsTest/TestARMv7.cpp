#include "SIMDRely.h"

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 30
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ < COMMON_SIMD_LV
#   error requires SIMDLV >= 30
#endif
#include "common/simd/SIMD128.hpp"

namespace
{
using namespace common;
using namespace common::simd;
using namespace COMMON_SIMD_NAMESPACE;
#include "SIMDBaseTest.h"
#include "ShuffleTest.h"
#include "DotProdTest.h"

using namespace common::simd;


RegisterSIMDBaseTest(F32x4, 30,       Move,      SEL, MSK, Add, Sub,                        Mul, Div, Neg, Abs, Min, Max, FMA, Rnd);
RegisterSIMDBaseTest(I64x2, 30, Load, Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub,                  Neg, Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA, And, Or, Xor, AndNot, Not);
RegisterSIMDBaseTest(U64x2, 30,       Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub,                       Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(I32x4, 30, Load, Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulX,     Neg, Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(U32x4, 30,       Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulX,          Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(I16x8, 30, Load, Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulX,     Neg, Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(U16x8, 30,       Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulX,          Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(I8x16, 30,       Move,      SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulX,     Neg, Abs, Min, Max, SLL,       SRL,       SRA);
RegisterSIMDBaseTest(U8x16, 30,       Move,      SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulX,          Abs, Min, Max, SLL,       SRL,       SRA);


RegisterSIMDCastTest(F16x8, 30, F32x4);
RegisterSIMDCastTest(F32x4, 30, F16x8);
RegisterSIMDCastTest(I64x2, 30, I8x16,        I16x8,        I32x4                                   );
RegisterSIMDCastTest(U64x2, 30,        U8x16,        U16x8,        U32x4                            );
RegisterSIMDCastTest(I32x4, 30, I8x16,                                    I64x2, U64x2,        F32x4);
RegisterSIMDCastTest(U32x4, 30,        U8x16,        U16x8,               I64x2, U64x2,        F32x4);
RegisterSIMDCastTest(I16x8, 30, I8x16,                      I32x4, U32x4, I64x2, U64x2, F16x8, F32x4);
RegisterSIMDCastTest(U16x8, 30,        U8x16,               I32x4, U32x4, I64x2, U64x2, F16x8, F32x4);
RegisterSIMDCastTest(I8x16, 30,               I16x8, U16x8, I32x4, U32x4, I64x2, U64x2,        F32x4);
RegisterSIMDCastTest(U8x16, 30,               I16x8, U16x8, I32x4, U32x4, I64x2, U64x2,        F32x4);


RegisterSIMDCastModeTest(F32x4, 30, RangeSaturate, I32x4, I16x8, I8x16);
RegisterSIMDCastModeTest(I64x2, 30, RangeSaturate, I32x4, U32x4);
RegisterSIMDCastModeTest(U64x2, 30, RangeSaturate, U32x4);
RegisterSIMDCastModeTest(I32x4, 30, RangeSaturate, I16x8, U16x8);
RegisterSIMDCastModeTest(U32x4, 30, RangeSaturate, U16x8);
RegisterSIMDCastModeTest(I16x8, 30, RangeSaturate, I8x16, U8x16);
RegisterSIMDCastModeTest(U16x8, 30, RangeSaturate, U8x16);


RegisterSIMDCmpTest(30, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDZipTest(30, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)
RegisterSIMDZipOETest(30, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDBroadcastTest(30, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDSelectTest(30, F32x4, I64x2, I32x4, I16x8)
RegisterSIMDSelectSlimTest(30, I8x16)


RegisterSIMDTest(U64x2, 30, shuftest::ShuffleTest<U64x2>);
RegisterSIMDTest(U32x4, 30, shuftest::ShuffleTest<U32x4>);


RegisterSIMDTest(U64x2, 30, shuftest::ShuffleVarTest<U64x2>);
RegisterSIMDTest(U32x4, 30, shuftest::ShuffleVarTest<U32x4>);
RegisterSIMDTest(U16x8, 30, shuftest::ShuffleVarTest<U16x8>);


RegisterSIMDTest(F32x4, 30, dottest::DotProdTest<F32x4>);


}
