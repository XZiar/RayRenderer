#pragma once
#include "SIMD.hpp"

#if COMMON_ARCH_X86
#   include "SIMD128SSE.hpp"
#else
#   include "SIMD128NEON.hpp"
#endif

static_assert(sizeof(COMMON_SIMD_NAMESPACE::U8x16) == 16);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::I8x16) == 16);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::U16x8) == 16);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::I16x8) == 16);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::U32x4) == 16);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::I32x4) == 16);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::U64x2) == 16);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::I64x2) == 16);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::F32x4) == 16);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::F64x2) == 16);

#define COMMON_SIMD_HAS_128 1