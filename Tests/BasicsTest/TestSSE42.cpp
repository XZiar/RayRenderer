#include "pch.h"
#define COMMON_SIMD_LV 42
#include "common/SIMD.hpp"
#if COMMON_SIMD_LV_ >= 42
#   include "CopyTest.h"


namespace copytest
{
using namespace common::copy;

GEN_COPY_TEST(32, 32)

GEN_COPY_TEST(32, 16)

GEN_COPY_TEST(32,  8)

GEN_COPY_TEST(16,  8)

GEN_BCAST_TEST(32)

GEN_BCAST_TEST(16)

GEN_BCAST_TEST(8)

}
#endif

