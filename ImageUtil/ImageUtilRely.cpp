#include "ImageUtilPch.h"

#pragma message("Compiling ImageUtil with " STRINGIZE(COMMON_SIMD_INTRIN) )

namespace xziar::img
{
using namespace common::mlog;
MiniLogger<false>& ImgLog()
{
    static MiniLogger<false> imglog(u"ImageUtil", { GetConsoleBackend() });
    return imglog;
}
}