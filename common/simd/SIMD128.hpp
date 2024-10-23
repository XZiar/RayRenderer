#pragma once
#include "SIMD.hpp"

#if COMMON_ARCH_X86
#   include "SIMD128SSE.hpp"
#else
#   include "SIMD128NEON.hpp"
#endif

#define COMMON_SIMD_HAS_128 1