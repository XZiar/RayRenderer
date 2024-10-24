#include "common/CommonRely.hpp"

#define COMMON_SIMD_LV_NAMESPACE 1
#define COMMON_SIMD_LV 200
#include "common/simd/SIMD.hpp"
#if COMMON_SIMD_LV_ < COMMON_SIMD_LV
#   error requires SIMDLV >= 200
#endif
#include "common/simd/SIMD128.hpp"

#define IMGU_FASTPATH_STB 1
#define STBIR_USE_FMA 1

#include "ColorConvert.inl"


DEFINE_FASTPATH_PARTIAL(ColorConvertor, A64)
{
    REGISTER_FASTPATH_VARIANTS(G8ToGA8,     SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(G8ToRGB8,    SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(G8ToRGBA8,   SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(GA8ToRGBA8,  SIMD128, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGB8ToRGBA8, NEON, LOOP);
    REGISTER_FASTPATH_VARIANTS(RGBA8ToRGB8, NEON, LOOP);
}


DEFINE_FASTPATH_PARTIAL(STBResize, A64)
{
    REGISTER_FASTPATH_VARIANTS(DoResize, NEON);
}
