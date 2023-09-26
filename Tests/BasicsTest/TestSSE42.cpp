#include "pch.h"

#define COMMON_SIMD_LV 42
#include "common/simd/SIMD.hpp"

namespace sse42
{
using namespace common;
using namespace common::simd;
#include "common/simd/SIMD128.hpp"
using namespace common::simd;
#include "SIMDBaseTest.h"
#include "ShuffleTest.h"
#include "DotProdTest.h"


RegisterSIMDBaseTest(F64x2, 42,            SEL, Add, Sub,                               Mul, Div, Neg, Abs, Min, Max, FMA, Rnd);
RegisterSIMDBaseTest(F32x4, 42,            SEL, Add, Sub,                               Mul, Div, Neg, Abs, Min, Max, FMA, Rnd);
RegisterSIMDBaseTest(I64x2, 42, Load, SWE, SEL, Add, Sub, SatAdd, SatSub, MulLo,                  Neg, Abs, Min, Max, SLL, SLLV, SRL, SRA, And, Or, Xor, AndNot, Not);
RegisterSIMDBaseTest(U64x2, 42,       SWE, SEL, Add, Sub, SatAdd, SatSub, MulLo,                       Abs, Min, Max, SLL, SLLV, SRL, SRA);
RegisterSIMDBaseTest(I32x4, 42, Load, SWE, SEL, Add, Sub, SatAdd, SatSub, MulLo,        MulX,     Neg, Abs, Min, Max, SLL, SLLV, SRL, SRA);
RegisterSIMDBaseTest(U32x4, 42,       SWE, SEL, Add, Sub, SatAdd, SatSub, MulLo,        MulX,          Abs, Min, Max, SLL, SLLV, SRL, SRA);
RegisterSIMDBaseTest(I16x8, 42, Load, SWE, SEL, Add, Sub, SatAdd, SatSub, MulLo, MulHi, MulX,     Neg, Abs, Min, Max, SLL, SLLV, SRL, SRA);
RegisterSIMDBaseTest(U16x8, 42,       SWE, SEL, Add, Sub, SatAdd, SatSub, MulLo, MulHi, MulX,          Abs, Min, Max, SLL, SLLV, SRL, SRA);
RegisterSIMDBaseTest(I8x16, 42,            SEL, Add, Sub, SatAdd, SatSub, MulLo, MulHi, MulX,     Neg, Abs, Min, Max, SLL,       SRL, SRA);
RegisterSIMDBaseTest(U8x16, 42,            SEL, Add, Sub, SatAdd, SatSub, MulLo, MulHi, MulX,          Abs, Min, Max, SLL,       SRL, SRA);


RegisterSIMDCastTest(F32x4, 42, F64x2);
RegisterSIMDCastTest(F64x2, 42, F32x4);
RegisterSIMDCastTest(I64x2, 42, I8x16,        I16x8,        I32x4                                   );
RegisterSIMDCastTest(U64x2, 42,        U8x16,        U16x8,        U32x4                            );
RegisterSIMDCastTest(I32x4, 42, I8x16,        I16x8,                      I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(U32x4, 42,        U8x16,        U16x8,               I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(I16x8, 42, I8x16,                      I32x4, U32x4, I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(U16x8, 42,        U8x16,               I32x4, U32x4, I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(I8x16, 42,               I16x8, U16x8, I32x4, U32x4, I64x2, U64x2, F32x4, F64x2);
RegisterSIMDCastTest(U8x16, 42,               I16x8, U16x8, I32x4, U32x4, I64x2, U64x2, F32x4, F64x2);


RegisterSIMDCastModeTest(F32x4, 42, RangeSaturate, I32x4, I16x8, U16x8, I8x16, U8x16);
RegisterSIMDCastModeTest(I32x4, 42, RangeSaturate, I16x8, U16x8);
RegisterSIMDCastModeTest(U32x4, 42, RangeSaturate, U16x8);
RegisterSIMDCastModeTest(I16x8, 42, RangeSaturate, I8x16, U8x16);
RegisterSIMDCastModeTest(U16x8, 42, RangeSaturate, U8x16);


RegisterSIMDCmpTest(42, F64x2, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDZipTest(42, F64x2, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDBroadcastTest(42, F64x2, F32x4, I64x2, U64x2, I32x4, U32x4, I16x8, U16x8, I8x16, U8x16)


RegisterSIMDSelectTest(42, F64x2, F32x4, I64x2, I32x4, I16x8)
RegisterSIMDSelectSlimTest(42, I8x16)


RegisterSIMDTest(F64x2, 42, shuftest::ShuffleTest<F64x2>);
RegisterSIMDTest(I64x2, 42, shuftest::ShuffleTest<I64x2>);
RegisterSIMDTest(F32x4, 42, shuftest::ShuffleTest<F32x4>);
RegisterSIMDTest(I32x4, 42, shuftest::ShuffleTest<I32x4>);


RegisterSIMDTest(F32x4, 41, dottest::DotProdTest<F32x4>);


}
