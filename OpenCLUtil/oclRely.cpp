#include "oclPch.h"

#pragma message("Compiling miniBLAS with " STRINGIZE(COMMON_SIMD_INTRIN) )

namespace oclu
{

using namespace common::mlog;
MiniLogger<false>& oclLog()
{
    static MiniLogger<false> ocllog(u"OpenCLUtil", { GetConsoleBackend() });
    return ocllog;
}


}