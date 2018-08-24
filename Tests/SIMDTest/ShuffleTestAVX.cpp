#define COMMON_SIMD_LV 100
#include "ShuffleTest.h"

using namespace common::simd;

static bool TestF64x4()
{
    F64x4 data(0, 1, 2, 3);
    return TestShuffle<F64x4, 4, 4, 0, 4 * 4 * 4 * 4 - 1>(data);
}
static auto Dummy1 = RegistTest(u"ShuffleTest@AVX F64x4", 100, TestF64x4);

static bool TestF64x4Var()
{
    F64x4 data(0, 1, 2, 3);
    return TestShuffleVar<F64x4, 4, 4>(data, 0, 4 * 4 * 4 * 4 - 1);
}
static auto Dummy2 = RegistTest(u"ShuffleTest@AVX F64x4[Var]", 100, TestF64x4Var);

//static bool TestF32x8()
//{
//    F32x8 data(0, 1, 2, 3, 4, 5, 6, 7);
//    return TestShuffle<F32x8, 8, 8, 0, 8 * 8 * 8 * 8 * 8 * 8 * 8 * 8 - 1>(data);
//}
//static auto Dummy3 = RegistTest(u"ShuffleTest@AVX F32x8", TestF32x8);

static bool TestF32x8Var()
{
    F32x8 data(0, 1, 2, 3, 4, 5, 6, 7);
    return TestShuffleVar<F32x8, 8, 8>(data, 0, 8 * 8 * 8 * 8 * 8 * 8 * 8 * 8 - 1);
}
static auto Dummy3 = RegistTest(u"ShuffleTest@AVX F32x8[Var]", 100, TestF32x8Var);

static bool TestF64x2()
{
    F64x2 data(0, 1);
    return TestShuffle<F64x2, 2, 2, 0, 2 * 2 - 1>(data);
}
static auto Dummy4 = RegistTest(u"ShuffleTest@AVX F64x2", 100, TestF64x2);

static bool TestF64x2Var()
{
    F64x2 data(0, 1);
    return TestShuffleVar<F64x2, 2, 2>(data, 0, 2 * 2 - 1);
}
static auto Dummy5 = RegistTest(u"ShuffleTest@AVX F64x2[Var]", 100, TestF64x2Var);

static bool TestF32x4()
{
    F32x4 data(0, 1, 2, 3);
    return TestShuffle<F32x4, 4, 4, 0, 4 * 4 * 4 * 4 - 1>(data);
}
static auto Dummy6 = RegistTest(u"ShuffleTest@AVX F32x4", 100, TestF32x4);

static bool TestF32x4Var()
{
    F32x4 data(0, 1, 2, 3);
    return TestShuffleVar<F32x4, 4, 4>(data, 0, 4 * 4 * 4 * 4 - 1);
}
static auto Dummy7 = RegistTest(u"ShuffleTest@AVX F32x4[Var]", 100, TestF32x4Var);
