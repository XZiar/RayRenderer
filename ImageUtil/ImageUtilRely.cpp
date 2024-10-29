#include "ImageUtilPch.h"
#include "SystemCommon/RuntimeFastPath.h"

#pragma message("Compiling ImageUtil with " STRINGIZE(COMMON_SIMD_INTRIN) )


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

DEFINE_FASTPATH_BASIC(ColorConvertor, G8ToGA8, G8ToRGB8, G8ToRGBA8, GA8ToRGBA8, RGB8ToRGBA8, BGR8ToRGBA8, RGBA8ToRGB8, RGBA8ToBGR8, RGB8ToBGR8, RGBA8ToBGRA8, RGBA8ToR8, RGBA8ToG8, RGBA8ToB8, RGBA8ToA8, RGB8ToR8, RGB8ToG8, RGB8ToB8, RGBA8ToRA8, RGBA8ToGA8, RGBA8ToBA8)

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