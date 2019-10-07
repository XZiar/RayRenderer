#include "pch.h"
#define COMMON_SIMD_LV 200
#include "common/SIMD.hpp"
#if COMMON_SIMD_LV_ >= 200
#   include "CopyTest.h"


namespace copytest
{
using namespace common::copy;


TEST_CONV(32, 32)

TEST_CONV(32, 16)

TEST_CONV(32,  8)

TEST_CONV(16,  8)                                                                                


}
#endif

