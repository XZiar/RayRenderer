#pragma once
#include "SIMD.hpp"

#if COMMON_ARCH_X86
#   include "SIMD256AVX.hpp"
#else
#endif

#define COMMON_SIMD_HAS_256 1
