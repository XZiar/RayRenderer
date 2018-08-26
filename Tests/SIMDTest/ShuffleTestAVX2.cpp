#define COMMON_SIMD_LV 200
#include "ShuffleTest.h"

using namespace common::simd;

static bool TestF64x4AVX2()
{
    F64x4 data(0, 1, 2, 3);
    return TestShuffle<F64x4, 0, Pow<4, 4>() - 1>(data);
}
static auto Dummy1 = RegistTest(u"ShuffleTest@AVX2 F64x4", 200, TestF64x4AVX2);

static bool TestF64x4VarAVX2()
{
    F64x4 data(0, 1, 2, 3);
    return TestShuffleVar<F64x4>(data, 0, Pow<4, 4>() - 1);
}
static auto Dummy2 = RegistTest(u"ShuffleTest@AVX2 F64x4[Var]", 200, TestF64x4VarAVX2);

//static bool TestF32x8()
//{
//    F32x8 data(0, 1, 2, 3, 4, 5, 6, 7);
//    return TestShuffle<F32x8, 0, Pow<8, 8>() - 1>(data);
//}
//static auto Dummy3 = RegistTest(u"ShuffleTest@AVX F32x8", TestF32x8);

static bool TestF32x8VarAVX2()
{
    F32x8 data(0, 1, 2, 3, 4, 5, 6, 7);
    return TestShuffleVar<F32x8>(data, 0, Pow<8, 8>() - 1);
}
static auto Dummy3 = RegistTest(u"ShuffleTest@AVX2 F32x8[Var]", 200, TestF32x8VarAVX2);

static bool TestI64x4AVX2()
{
    I64x4 data(0, 1, 2, 3);
    return TestShuffle<I64x4, 0, Pow<4, 4>() - 1>(data);
}
static auto Dummy4 = RegistTest(u"ShuffleTest@AVX2 I64x4", 200, TestI64x4AVX2);

static bool TestI64x4VarAVX2()
{
    I64x4 data(0, 1, 2, 3);
    return TestShuffleVar<I64x4>(data, 0, Pow<4, 4>() - 1);
}
static auto Dummy5 = RegistTest(u"ShuffleTest@AVX2 I64x4[Var]", 200, TestI64x4VarAVX2);

//static bool TestI32x8AVX2()
//{
//    I32x8 data(0, 1, 2, 3, 4, 5, 6, 7);
//    return TestShuffle<I32x8, 0, Pow<8, 8>() - 1>(data);
//}
//static auto Dummy6 = RegistTest(u"ShuffleTest@AVX I32x8", TestI32x8);

static bool TestI32x8VarAVX2()
{
    I32x8 data(0, 1, 2, 3, 4, 5, 6, 7);
    return TestShuffleVar<I32x8>(data, 0, Pow<8, 8>() - 1);
}
static auto Dummy7 = RegistTest(u"ShuffleTest@AVX2 I32x8[Var]", 200, TestI32x8VarAVX2);