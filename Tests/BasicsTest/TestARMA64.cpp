#include "SIMDRely.h"

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 200
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ < COMMON_SIMD_LV
#   error requires SIMDLV >= 200
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


RegisterSIMDBaseTest(F64x2, 200,       Move,      SEL, MSK, Add, Sub,                        Mul, Div, Neg, Abs, Min, Max, FMA, Rnd);
RegisterSIMDBaseTest(F32x4, 200,       Move,      SEL, MSK, Add, Sub,                        Mul, Div, Neg, Abs, Min, Max, FMA, Rnd);
RegisterSIMDBaseTest(I64x2, 200, Load, Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub,                  Neg, Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA, And, Or, Xor, AndNot, Not);
RegisterSIMDBaseTest(U64x2, 200,       Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub,                       Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(I32x4, 200, Load, Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulX,     Neg, Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(U32x4, 200,       Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulX,          Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(I16x8, 200, Load, Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulX,     Neg, Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(U16x8, 200,       Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulX,          Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(I8x16, 200,       Move,      SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulX,     Neg, Abs, Min, Max, SLL,       SRL,       SRA);
RegisterSIMDBaseTest(U8x16, 200,       Move,      SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulX,          Abs, Min, Max, SLL,       SRL,       SRA);


RegisterSIMDCastTest(F16x8, 200, F32x4);
RegisterSIMDCastTest(F32x4, 200, F16x8, F64x2);
RegisterSIMDCastTest(F64x2, 200, F32x4);
RegisterSIMDCastTest(I64x2, 200, I8x16,        I16x8,        I32x4                                   );
RegisterSIMDCastTest(U64x2, 200,        U8x16,        U16x8,        U32x4                            );
RegisterSIMDCastTest(I32x4, 200, I8x16,                                    I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(U32x4, 200,        U8x16,        U16x8,               I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(I16x8, 200, I8x16,                      I32x4, U32x4, I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(U16x8, 200,        U8x16,               I32x4, U32x4, I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(I8x16, 200,               I16x8, U16x8, I32x4, U32x4, I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(U8x16, 200,               I16x8, U16x8, I32x4, U32x4, I64x2, U64x2, F32x4, F64x2);


RegisterSIMDCastModeTest(F32x4, 200, RangeSaturate, I32x4, I16x8, I8x16);
RegisterSIMDCastModeTest(I64x2, 200, RangeSaturate, I32x4, U32x4);
RegisterSIMDCastModeTest(U64x2, 200, RangeSaturate, U32x4);
RegisterSIMDCastModeTest(I32x4, 200, RangeSaturate, I16x8, U16x8);
RegisterSIMDCastModeTest(U32x4, 200, RangeSaturate, U16x8);
RegisterSIMDCastModeTest(I16x8, 200, RangeSaturate, I8x16, U8x16);
RegisterSIMDCastModeTest(U16x8, 200, RangeSaturate, U8x16);


RegisterSIMDCmpTest(200, F64x2, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDZipTest(200, F64x2, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)
RegisterSIMDZipOETest(200, F64x2, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDBroadcastTest(200, F64x2, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDSelectTest(200, F64x2, F32x4, I64x2, I32x4, I16x8)
RegisterSIMDSelectSlimTest(200, I8x16)


RegisterSIMDTest(F64x2, 200, shuftest::ShuffleTest<F64x2>);
RegisterSIMDTest(U64x2, 200, shuftest::ShuffleTest<U64x2>);
RegisterSIMDTest(U32x4, 200, shuftest::ShuffleTest<U32x4>);


RegisterSIMDTest(F64x2, 200, shuftest::ShuffleVarTest<F64x2>);
RegisterSIMDTest(U64x2, 200, shuftest::ShuffleVarTest<U64x2>);
RegisterSIMDTest(U32x4, 200, shuftest::ShuffleVarTest<U32x4>);
RegisterSIMDTest(U16x8, 200, shuftest::ShuffleVarTest<U16x8>);


RegisterSIMDTest(F32x4, 200, dottest::DotProdTest<F32x4>);


}
