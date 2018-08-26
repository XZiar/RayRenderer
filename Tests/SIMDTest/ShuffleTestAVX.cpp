#define COMMON_SIMD_LV 100
#include "ShuffleTest.h"

using namespace common::simd;

static bool TestF64x4AVX()
{
    F64x4 data(0, 1, 2, 3);
    return TestShuffle<F64x4, 0, Pow<4, 4>() - 1>(data);
}
static auto Dummy1 = RegistTest(u"ShuffleTest@AVX F64x4", 100, TestF64x4AVX);

static bool TestF64x4VarAVX()
{
    F64x4 data(0, 1, 2, 3);
    return TestShuffleVar<F64x4>(data, 0, Pow<4, 4>() - 1);
}
static auto Dummy2 = RegistTest(u"ShuffleTest@AVX F64x4[Var]", 100, TestF64x4VarAVX);

//static bool TestF32x8AVX()
//{
//    F32x8 data(0, 1, 2, 3, 4, 5, 6, 7);
//    return TestShuffle<F32x8, 0, Pow<8, 8>() - 1>(data);
//}
//static auto Dummy3 = RegistTest(u"ShuffleTest@AVX F32x8", TestF32x8);

static bool TestF32x8VarAVX()
{
    F32x8 data(0, 1, 2, 3, 4, 5, 6, 7);
    return TestShuffleVar<F32x8>(data, 0, Pow<8, 8>() - 1);
}
static auto Dummy3 = RegistTest(u"ShuffleTest@AVX F32x8[Var]", 100, TestF32x8VarAVX);

static bool TestF64x2AVX()
{
    F64x2 data(0, 1);
    return TestShuffle<F64x2, 0, Pow<2, 2>() - 1>(data);
}
static auto Dummy4 = RegistTest(u"ShuffleTest@AVX F64x2", 100, TestF64x2AVX);

static bool TestF64x2VarAVX()
{
    F64x2 data(0, 1);
    return TestShuffleVar<F64x2>(data, 0, Pow<2, 2>() - 1);
}
static auto Dummy5 = RegistTest(u"ShuffleTest@AVX F64x2[Var]", 100, TestF64x2VarAVX);

static bool TestF32x4AVX()
{
    F32x4 data(0, 1, 2, 3);
    return TestShuffle<F32x4, 0, Pow<4, 4>() - 1>(data);
}
static auto Dummy6 = RegistTest(u"ShuffleTest@AVX F32x4", 100, TestF32x4AVX);

static bool TestF32x4VarAVX()
{
    F32x4 data(0, 1, 2, 3);
    return TestShuffleVar<F32x4>(data, 0, Pow<4, 4>() - 1);
}
static auto Dummy7 = RegistTest(u"ShuffleTest@AVX F32x4[Var]", 100, TestF32x4VarAVX);

static bool TestI32x4AVX()
{
    I32x4 data(0, 1, 2, 3);
    return TestShuffle<I32x4, 0, Pow<4, 4>() - 1>(data);
}
static auto Dummy8 = RegistTest(u"ShuffleTest@AVX I32x4", 100, TestI32x4AVX);

static bool TestI32x4VarAVX()
{
    I32x4 data(0, 1, 2, 3);
    return TestShuffleVar<I32x4>(data, 0, Pow<4, 4>() - 1);
}
static auto Dummy9 = RegistTest(u"ShuffleTest@AVX I32x4[Var]", 100, TestI32x4VarAVX);

static bool TestI16x8VarAVX()
{
    I16x8 data(0, 1, 2, 3, 4, 5, 6, 7);
    return TestShuffleVar<I16x8>(data, 0, Pow<8, 8>() - 1);
}
static auto Dummy10 = RegistTest(u"ShuffleTest@AVX I16x8[Var]", 31, TestI16x8VarAVX);
