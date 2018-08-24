#define COMMON_SIMD_LV 200
#include "ShuffleTest.h"

using namespace common::simd;

static bool TestF64x4()
{
    F64x4 data(0, 1, 2, 3);
    return TestShuffle<F64x4, 4, 4, 0, 4 * 4 * 4 * 4 - 1>(data);
}
static auto Dummy1 = RegistTest(u"ShuffleTest@AVX2 F64x4", TestF64x4);

static bool TestF64x4Var()
{
    F64x4 data(0, 1, 2, 3);
    return TestShuffleVar<F64x4, 4, 4>(data, 0, 4 * 4 * 4 * 4 - 1);
}
static auto Dummy2 = RegistTest(u"ShuffleTest@AVX2 F64x4[Var]", TestF64x4Var);

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
static auto Dummy3 = RegistTest(u"ShuffleTest@AVX2 F32x8[Var]", TestF32x8Var);
