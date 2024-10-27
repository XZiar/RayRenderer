#include "common/CommonRely.hpp"

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 42
#if COMMON_COMPILER_MSVC
#   define __SSE4_2__ 1
#   define __SSE4_1__ 1
#   define __SSSE3__  1
#   define __SSE3__   1
#endif
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ < COMMON_SIMD_LV
#   error requires SIMDLV >= 42
#endif
#include "common/simd/SIMD128.hpp"

#include "CopyExIntrin.inl"
#include "MiscIntrins.inl"


DEFINE_FASTPATH_PARTIAL(CopyManager, SSE42)
{
    REGISTER_FASTPATH_VARIANTS(Broadcast2, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(Broadcast4, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(ZExtCopy12, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(ZExtCopy14, SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(ZExtCopy24, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(ZExtCopy28, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(ZExtCopy48, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(SExtCopy12, SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(SExtCopy14, SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(SExtCopy24, SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(SExtCopy28, SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(SExtCopy48, SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(TruncCopy21, SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(TruncCopy41, SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(TruncCopy42, SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(TruncCopy81, SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(TruncCopy82, SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(TruncCopy84, SSSE3, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtI32F32, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtI16F32, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtI8F32,  SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtU32F32, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtU16F32, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtU8F32,  SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtF32I32, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtF32I16, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtF32I8,  SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtF32U16, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtF32U8,  SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtF16F32, SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtF32F16, SSE41, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtF32F64, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtF64F32, SIMD128, LOOP);
}

DEFINE_FASTPATH_PARTIAL(MiscIntrins, SSE42)
{
    REGISTER_FASTPATH_VARIANTS(LeadZero32, LZCNT, COMPILER);
    REGISTER_FASTPATH_VARIANTS(LeadZero64, LZCNT, COMPILER);
    REGISTER_FASTPATH_VARIANTS(TailZero32, TZCNT, COMPILER);
    REGISTER_FASTPATH_VARIANTS(TailZero64, TZCNT, COMPILER);
    REGISTER_FASTPATH_VARIANTS(PopCount32, POPCNT, COMPILER, NAIVE);
    REGISTER_FASTPATH_VARIANTS(PopCount64, POPCNT, COMPILER, NAIVE);
    REGISTER_FASTPATH_VARIANTS(PopCounts,  POPCNT, COMPILER, NAIVE);
    REGISTER_FASTPATH_VARIANTS(Hex2Str, SSSE3, SIMD128, NAIVE);
    REGISTER_FASTPATH_VARIANTS(PauseCycles, WAITPKG, SSE2, COMPILER);
}

DEFINE_FASTPATH_PARTIAL(DigestFuncs, SSE42)
{
    REGISTER_FASTPATH_VARIANTS(Sha256, SHANI, NAIVE);
}
