#pragma once
#include "SIMD.hpp"

#if COMMON_ARCH_X86
#   include "SIMD256AVX.hpp"
#else
#endif

static_assert(sizeof(COMMON_SIMD_NAMESPACE::U8x32 ) == 32);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::I8x32 ) == 32);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::U16x16) == 32);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::I16x16) == 32);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::U32x8 ) == 32);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::I32x8 ) == 32);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::U64x4 ) == 32);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::I64x4 ) == 32);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::F32x8 ) == 32);
static_assert(sizeof(COMMON_SIMD_NAMESPACE::F64x4 ) == 32);

#define COMMON_SIMD_HAS_256 1
