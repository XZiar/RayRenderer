#include "oglPch.h"

#if defined(_WIN32)
#   pragma comment (lib, "opengl32.lib")// link Microsoft OpenGL lib
#   pragma comment (lib, "glu32.lib")// link OpenGL Utility lib
#endif
#pragma message("Compiling OpenGLUtil with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace oglu
{

using namespace common::mlog;
MiniLogger<false>& oglLog()
{
    static MiniLogger<false> ogllog(u"OpenGLUtil", { GetConsoleBackend() });
    return ogllog;
}

std::string ContextCapability::GenerateSupportLog() const
{
    std::string text("Capability:\n");
    auto backer = std::back_inserter(text);

#define CHK_SUPPORT(x) fmt::format_to(backer, FMT_STRING("Support [{:^15}] : \t [{}]\n"), #x, PPCAT(Support,x) ? 'Y' : 'N')
    CHK_SUPPORT(Debug);
    CHK_SUPPORT(SRGB);
    CHK_SUPPORT(ClipControl);
    CHK_SUPPORT(ComputeShader);
    CHK_SUPPORT(TessShader);
    CHK_SUPPORT(BindlessTexture);
    CHK_SUPPORT(ImageLoadStore);
    CHK_SUPPORT(Subroutine);
    CHK_SUPPORT(IndirectDraw);
    CHK_SUPPORT(BaseInstance);
    CHK_SUPPORT(VSMultiLayer);
#undef CHK_SUPPORT

    return text;
}

}