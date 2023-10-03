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
#elif COMMON_ARCH_ARM
#   if COMMON_OSBIT == 64
DEFINE_FASTPATH_SCOPE(A64)
{
    return common::CheckCPUFeature("asimd");
}
DECLARE_FASTPATH_PARTIALS(ColorConvertor, A64)
#   else
DEFINE_FASTPATH_SCOPE(A32)
{
    return common::CheckCPUFeature("asimd");
}
DECLARE_FASTPATH_PARTIALS(ColorConvertor, A32)
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

DEFINE_FASTPATH_BASIC(ColorConvertor, G8ToGA8, G8ToRGB8, G8ToRGBA8)

const ColorConvertor& ColorConvertor::Get() noexcept
{
    static ColorConvertor convertor;
    return convertor;
}


}