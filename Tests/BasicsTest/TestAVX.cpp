#include "pch.h"
#define COMMON_SIMD_LV 100
#include "common/SIMD.hpp"
#if COMMON_SIMD_LV_ >= 100
#   include "CopyTest.h"
#   include "ShuffleTest.h"


namespace copytest
{
using namespace common::copy;

GEN_BCAST_TEST(32)

GEN_BCAST_TEST(16)

GEN_BCAST_TEST( 8)

}

namespace shuftest
{
using namespace common::simd;

GEN_SHUF_TEST(F64x4)

GEN_SHUF_TEST(F64x2)

GEN_SHUF_TEST(I64x2)

GEN_SHUF_TEST(F32x4)

GEN_SHUF_TEST(I32x4)

GEN_SHUF_VAR_TEST(F64x4)

GEN_SHUF_VAR_TEST(F64x2)

GEN_SHUF_VAR_TEST(F32x8)

GEN_SHUF_VAR_TEST(F32x4)

GEN_SHUF_VAR_TEST(I32x4)

}


#endif
