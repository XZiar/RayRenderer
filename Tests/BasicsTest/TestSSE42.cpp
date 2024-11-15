#include "SIMDRely.h"

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 42
#if COMMON_COMPILER_MSVC
#   define __SSE4_2__ 1
#   define __SSE4_1__ 1
#   define __SSSE3__  1
#   define __SSE3__   1
#endif
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ < COMMON_SIMD_LV
#   error requires SIMDLV >= 42
#endif
#include "common/simd/SIMD128.hpp"

namespace sse42
{
using namespace common;
using namespace common::simd;
using namespace COMMON_SIMD_NAMESPACE;
#include "SIMDBaseTest.h"
#include "ShuffleTest.h"
#include "DotProdTest.h"


RegisterSIMDBaseTest(F64x2, 41,       Move,      SEL, MSK, Add, Sub,                               Mul, Div, Neg, Abs, Min, Max, FMA, Rnd);
RegisterSIMDBaseTest(F32x4, 41,       Move,      SEL, MSK, Add, Sub,                               Mul, Div, Neg, Abs, Min, Max, FMA, Rnd);
RegisterSIMDBaseTest(I64x2, 41, Load, Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo,                  Neg, Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA, And, Or, Xor, AndNot, Not);
RegisterSIMDBaseTest(U64x2, 41,       Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo,                       Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(I32x4, 41, Load, Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo,        MulX,     Neg, Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(U32x4, 41,       Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo,        MulX,          Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(I16x8, 41, Load, Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulHi, MulX,     Neg, Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(U16x8, 41,       Move, SWE, SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulHi, MulX,          Abs, Min, Max, SLL, SLLV, SRL, SRLV, SRA);
RegisterSIMDBaseTest(I8x16, 41,       Move,      SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulHi, MulX,     Neg, Abs, Min, Max, SLL,       SRL,       SRA);
RegisterSIMDBaseTest(U8x16, 41,       Move,      SEL, MSK, Add, Sub, SatAdd, SatSub, MulLo, MulHi, MulX,          Abs, Min, Max, SLL,       SRL,       SRA);


RegisterSIMDCastTest(F32x4, 41, F64x2);
RegisterSIMDCastTest(F64x2, 41, F32x4);
RegisterSIMDCastTest(I64x2, 41, I8x16,        I16x8,        I32x4                                   );
RegisterSIMDCastTest(U64x2, 41,        U8x16,        U16x8,        U32x4                            );
RegisterSIMDCastTest(I32x4, 41, I8x16,        I16x8,                      I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(U32x4, 41,        U8x16,        U16x8,               I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(I16x8, 41, I8x16,                      I32x4, U32x4, I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(U16x8, 41,        U8x16,               I32x4, U32x4, I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(I8x16, 41,               I16x8, U16x8, I32x4, U32x4, I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(U8x16, 41,               I16x8, U16x8, I32x4, U32x4, I64x2, U64x2, F32x4, F64x2);


RegisterSIMDCastModeTest(F32x4, 41, RangeSaturate, I32x4, I16x8, U16x8, I8x16, U8x16);
RegisterSIMDCastModeTest(I32x4, 41, RangeSaturate, I16x8, U16x8);
RegisterSIMDCastModeTest(U32x4, 41, RangeSaturate, U16x8);
RegisterSIMDCastModeTest(I16x8, 41, RangeSaturate, I8x16, U8x16);
RegisterSIMDCastModeTest(U16x8, 41, RangeSaturate, U8x16);


RegisterSIMDCmpTest(41, F64x2, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDZipTest(41, F64x2, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDBroadcastTest(41, F64x2, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDSelectTest(41, F64x2, F32x4, I64x2, I32x4, I16x8)
RegisterSIMDSelectSlimTest(41, I8x16)
RegisterSIMDSelectRandTest(41, I8x16)


RegisterSIMDTest(F64x2, 41, shuftest::ShuffleTest<F64x2>);
RegisterSIMDTest(I64x2, 41, shuftest::ShuffleTest<I64x2>);
RegisterSIMDTest(F32x4, 41, shuftest::ShuffleTest<F32x4>);
RegisterSIMDTest(I32x4, 41, shuftest::ShuffleTest<I32x4>);


RegisterSIMDTest(F32x4, 41, dottest::DotProdTest<F32x4>);


}
