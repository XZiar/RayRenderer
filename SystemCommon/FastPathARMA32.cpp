#include "common/CommonRely.hpp"
#if COMMON_COMPILER_GCC
#   pragma GCC push_options
#   pragma GCC target("arch=armv8-a+simd+crypto")
#elif COMMON_COMPILER_CLANG
#   pragma clang attribute push (__attribute__((target("arch=armv8-a+simd+crypto"))), apply_to=function)
#endif

#define COMMON_SIMD_LV 100
#include "common/simd/SIMD.hpp"

#include "CopyExIntrin.inl"
#include "MiscIntrins.inl"


DEFINE_FASTPATH_PARTIAL(CopyManager, A32)
{
    REGISTER_FASTPATH_VARIANTS(Broadcast2, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(Broadcast4, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(ZExtCopy12, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(ZExtCopy14, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(ZExtCopy24, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(ZExtCopy28, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(ZExtCopy48, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(SExtCopy12, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(SExtCopy14, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(SExtCopy24, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(SExtCopy28, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(SExtCopy48, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(TruncCopy21, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(TruncCopy41, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(TruncCopy42, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(TruncCopy81, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(TruncCopy82, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(TruncCopy84, SIMD128, LOOP);
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
    REGISTER_FASTPATH_VARIANTS(CvtF16F32, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtF32F16, SIMD128, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtF32F64, LOOP);
    REGISTER_FASTPATH_VARIANTS(CvtF64F32, LOOP);
}

DEFINE_FASTPATH_PARTIAL(MiscIntrins, A32)
{
    REGISTER_FASTPATH_VARIANTS(LeadZero32, COMPILER);
    REGISTER_FASTPATH_VARIANTS(LeadZero64, COMPILER);
    REGISTER_FASTPATH_VARIANTS(TailZero32, COMPILER);
    REGISTER_FASTPATH_VARIANTS(TailZero64, COMPILER);
    REGISTER_FASTPATH_VARIANTS(PopCount32, COMPILER, NAIVE);
    REGISTER_FASTPATH_VARIANTS(PopCount64, COMPILER, NAIVE);
    REGISTER_FASTPATH_VARIANTS(Hex2Str, SIMD128, NAIVE);
    REGISTER_FASTPATH_VARIANTS(PauseCycles, COMPILER);
}

DEFINE_FASTPATH_PARTIAL(DigestFuncs, A32)
{
    REGISTER_FASTPATH_VARIANTS(Sha256, SHA2, NAIVE);
}


#if COMMON_COMPILER_GCC
#   pragma GCC pop_options
#elif COMMON_COMPILER_CLANG
#   pragma clang attribute pop
#endif