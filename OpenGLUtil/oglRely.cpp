#include "oglPch.h"

#pragma message("Compiling OpenGLUtil with " STRINGIZE(COMMON_SIMD_INTRIN) )


namespace oglu
{
using namespace common::mlog;
using namespace std::string_view_literals;

COMMON_EXCEPTION_IMPL(OGLException)

MiniLogger<false>& oglLog()
{
    static MiniLogger<false> ogllog(u"OpenGLUtil", { GetConsoleBackend() });
    return ogllog;
}


std::string ContextCapability::GenerateSupportLog() const
{
    std::string text("Capability:\n");

    const auto& syntax = FmtString("Support [{:^15}] : \t [{}]\n"sv);
    common::str::Formatter<char> fmter{};
#define CHK_SUPPORT(x) fmter.FormatToStatic(text, syntax, #x, PPCAT(Support,x) ? 'Y' : 'N')
    CHK_SUPPORT(Debug);
    CHK_SUPPORT(SRGB);
    CHK_SUPPORT(SRGBFrameBuffer);
    CHK_SUPPORT(ClipControl);
    CHK_SUPPORT(GeometryShader);
    CHK_SUPPORT(ComputeShader);
    CHK_SUPPORT(TessShader);
    CHK_SUPPORT(BindlessTexture);
    CHK_SUPPORT(ImageLoadStore);
    CHK_SUPPORT(Subroutine);
    CHK_SUPPORT(IndirectDraw);
    CHK_SUPPORT(InstanceDraw);
    CHK_SUPPORT(BaseInstance);
    CHK_SUPPORT(LayeredRender);
    CHK_SUPPORT(VSMultiLayer);
#undef CHK_SUPPORT

    return text;
}

}