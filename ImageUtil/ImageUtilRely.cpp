#include "ImageUtilPch.h"
#include "SystemCommon/RuntimeFastPath.h"


#if COMMON_ARCH_X86
DEFINE_FASTPATH_SCOPE(AVX512)
{
    return common::CheckCPUFeature("avx512f");
}
DEFINE_FASTPATH_SCOPE(AVX2)
{
    return common::CheckCPUFeature("avx2");
}
DEFINE_FASTPATH_SCOPE(SSE42)
{
    return true;
}
DECLARE_FASTPATH_PARTIALS(ColorConvertor, AVX512, AVX2, SSE42)
DECLARE_FASTPATH_PARTIALS(STBResize, AVX2, SSE42)
#elif COMMON_ARCH_ARM
#   if COMMON_OSBIT == 64
DEFINE_FASTPATH_SCOPE(A64)
{
    return common::CheckCPUFeature("asimd");
}
DECLARE_FASTPATH_PARTIALS(ColorConvertor, A64)
DECLARE_FASTPATH_PARTIALS(STBResize, A64)
#   else
DEFINE_FASTPATH_SCOPE(A32)
{
    return common::CheckCPUFeature("asimd");
}
DECLARE_FASTPATH_PARTIALS(ColorConvertor, A32)
DECLARE_FASTPATH_PARTIALS(STBResize, A32)
#   endif
#endif


namespace xziar::img
{
using namespace common::mlog;
MiniLogger<false>& ImgLog()
{
    static MiniLogger<false> imglog(u"ImageUtil", { GetConsoleBackend() });
    return imglog;
}


DEFINE_FASTPATH_BASIC(ColorConvertor,
    G8ToGA8, G8ToRGB8, G8ToRGBA8, GA8ToRGB8, GA8ToRGBA8,
    RGB8ToRGBA8, BGR8ToRGBA8, RGBA8ToRGB8, RGBA8ToBGR8, 
    RGB8ToBGR8, RGBA8ToBGRA8, RGBfToBGRf, RGBAfToBGRAf,
    RGBA8ToR8, RGBA8ToG8, RGBA8ToB8, RGBA8ToA8, RGB8ToR8, RGB8ToG8, RGB8ToB8, RGBAfToRf, RGBAfToGf, RGBAfToBf, RGBAfToAf, RGBfToRf, RGBfToGf, RGBfToBf,
    RGBA8ToRA8, RGBA8ToGA8, RGBA8ToBA8,
    Extract8x2, Extract8x3, Extract8x4, Extract32x2, Extract32x3, Extract32x4,
    Combine8x2, Combine8x3, Combine8x4, Combine32x2, Combine32x3, Combine32x4,
    RGB555ToRGB8, BGR555ToRGB8, RGB555ToRGBA8, BGR555ToRGBA8, RGB5551ToRGBA8, BGR5551ToRGBA8,
    RGB565ToRGB8, BGR565ToRGB8, RGB565ToRGBA8, BGR565ToRGBA8,
    RGB10ToRGBf, BGR10ToRGBf, RGB10ToRGBAf, BGR10ToRGBAf, RGB10A2ToRGBAf, BGR10A2ToRGBAf)

const ColorConvertor& ColorConvertor::Get() noexcept
{
    static ColorConvertor convertor;
    return convertor;
}

DEFINE_FASTPATH_BASIC(STBResize, DoResize)

const STBResize& STBResize::Get() noexcept
{
    static STBResize resizer;
    return resizer;
}


}