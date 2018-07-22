#include "TexResizer.h"
#include "resource.h"
#include "OpenGLUtil/oglPromise.hpp"
#include "common/PromiseTaskSTD.hpp"
#include <future>
#include <thread>


namespace oglu::texutil
{

TexResizer::TexResizer(const oglContext& baseContext) : Executor(u"TexResizer")
{
    Executor.Start([this, baseContext]
    {
        common::SetThreadName(u"TexResizer");
        GLContext = oglContext::NewContext(baseContext, true);
        if (!GLContext->UseContext())
        {
            texLog().error(u"TexResizer cannot use GL context\n");
        }
        texLog().info(u"TexResizer use GL context with version {}\n", oglUtil::getVersion());
        GLContext->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        GLContext->SetSRGBFBO(true);
        Init();
        texLog().success(u"TexResize thread start running.\n");
    }, [&]()
    {
        //exit
        if (!GLContext->UnloadContext())
        {
            texLog().error(u"TexResizer cannot terminate GL context\n");
        }
        GLContext.release();
    });
}

void TexResizer::Init()
{
    GLResizer.reset(u"GLResizer");
    const string shaderSrc = getShaderFromDLL(IDR_SHADER_GLRESIZER);
    try
    {
        GLResizer->AddExtShaders(shaderSrc);
        GLResizer->Link();
    }
    catch (const OGLException& gle)
    {
        texLog().error(u"GLResizer shader fail:\n{}\n", gle.message);
        COMMON_THROW(BaseException, L"GLResizer shader fail");
    }

    ScreenBox.reset();
    const Vec4 pa(-1.0f, -1.0f, 0.0f, 0.0f), pb(1.0f, -1.0f, 1.0f, 0.0f), pc(-1.0f, 1.0f, 0.0f, 1.0f), pd(1.0f, 1.0f, 1.0f, 1.0f);
    const Vec4 paf(-1.0f, -1.0f, 0.0f, 1.0f), pbf(1.0f, -1.0f, 1.0f, 1.0f), pcf(-1.0f, 1.0f, 0.0f, 0.0f), pdf(1.0f, 1.0f, 1.0f, 0.0f);
    const Vec4 DatVert[] = { pa,pb,pc, pd,pc,pb, paf,pbf,pcf, pdf,pcf,pbf };
    ScreenBox->Write(DatVert, sizeof(DatVert));

    NormalVAO.reset(VAODrawMode::Triangles);
    NormalVAO->Prepare()
        .SetFloat(ScreenBox, GLResizer->Attr_Vert_Pos, sizeof(Vec4), 2, 0)
        .SetFloat(ScreenBox, GLResizer->Attr_Vert_Texc, sizeof(Vec4), 2, sizeof(float) * 2)
        .SetDrawSize(0, 6);
    FlipYVAO.reset(VAODrawMode::Triangles);
    FlipYVAO->Prepare()
        .SetFloat(ScreenBox, GLResizer->Attr_Vert_Pos, sizeof(Vec4), 2, 0)
        .SetFloat(ScreenBox, GLResizer->Attr_Vert_Texc, sizeof(Vec4), 2, sizeof(float) * 2)
        .SetDrawSize(6, 6);
}

TexResizer::~TexResizer()
{
    Executor.Stop();
}

static TextureInnerFormat DecideFormat(const ImageDataType& type)
{
    switch (type)
    {
    case ImageDataType::GRAY:       return TextureInnerFormat::R8;
    case ImageDataType::GA:         return TextureInnerFormat::RG8;
    case ImageDataType::BGR:        [[fallthrough]];
    case ImageDataType::RGB:        return TextureInnerFormat::SRGB8;
    case ImageDataType::BGRA:       [[fallthrough]];
    case ImageDataType::RGBA:       return TextureInnerFormat::SRGBA8;

    case ImageDataType::GRAYf:       return TextureInnerFormat::Rf;
    case ImageDataType::GAf:         return TextureInnerFormat::RGf;
    case ImageDataType::BGRf:        [[fallthrough]];
    case ImageDataType::RGBf:        return TextureInnerFormat::RGBf;
    case ImageDataType::BGRAf:       [[fallthrough]];
    case ImageDataType::RGBAf:       return TextureInnerFormat::RGBAf;
    default:                         return TextureInnerFormat::SRGBA8;
    }
}

static TextureInnerFormat DecideFormat(ImageDataType type, const TextureInnerFormat prefer)
{
    if (!TexFormatUtil::HasAlphaType(prefer))
        type = REMOVE_MASK(type, ImageDataType::ALPHA_MASK);
    const TextureInnerFormat matched = DecideFormat(type);
    if(TexFormatUtil::IsGrayType(matched) != TexFormatUtil::IsGrayType(prefer))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"not support to convert between color and gray");
    return matched;
}

static Image ExtractTexData(const oglTex2DS& tex)
{

}

static void FilterFormat(const TextureInnerFormat format)
{
    if (TexFormatUtil::IsCompressType(format))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, L"not support to resize to a compressed format");
}

common::PromiseResult<Image> TexResizer::ResizeToDat(const Image& img, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY)
{
    const auto middleFormat = DecideFormat(img.DataType, DecideFormat(format));
    oglTex2DS tex(img.Width, img.Height, middleFormat);
    tex->SetData(img, true, false);
    tex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);
    return ResizeToDat(tex, width, height, format, flipY);
}
common::PromiseResult<Image> TexResizer::ResizeToDat(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY)
{
    const auto pms = std::make_shared<std::promise<std::unique_ptr<Image>>>();
    auto ret = std::make_shared<common::PromiseWrappedResultSTD<Image, false>>(*pms);

    const auto pmsTex = ResizeToTex(tex, width, height, DecideFormat(format), flipY);
    Executor.AddTask([pms, pmsTex, format](const common::asyexe::AsyncAgent& agent)
    {
        const auto outtex = agent.Await(pmsTex);
        pms->set_value(std::make_unique<Image>(outtex->GetImage(format, false)));
    });

    return ret;
}

common::PromiseResult<oglTex2DS> TexResizer::ResizeToTex(const Image& img, const uint16_t width, const uint16_t height, const TextureInnerFormat format, const bool flipY)
{
    const auto middleFormat = DecideFormat(img.DataType, format);
    oglTex2DS tex(img.Width, img.Height, middleFormat);
    tex->SetData(img, true, false);
    tex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);
    return ResizeToTex(tex, width, height, format, flipY);
}

common::PromiseResult<oglTex2DS> TexResizer::ResizeToTex(const oglTex2D& tex, const uint16_t width, const uint16_t height, const TextureInnerFormat format, const bool flipY)
{
    const auto vao = flipY ? FlipYVAO : NormalVAO;

    const auto pms = std::make_shared<std::promise<oglTex2DS>>();
    auto ret = std::make_shared<common::PromiseResultSTD<oglTex2DS, false>>(*pms);

    Executor.AddTask([this, pms, tex, width, height, format, vao](const common::asyexe::AsyncAgent& agent)
    {
        oglTex2DS outtex(width, height, format);
        outtex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);
        OutputFrame->AttachColorTexture(outtex, 0);
        oglRBO mainRBO(width, height, oglu::RBOFormat::Depth24Stencil8);
        OutputFrame->AttachDepthStencilBuffer(mainRBO);
        texLog().info(u"FBO resize to [{}x{}], status:{}\n", width, height, OutputFrame->CheckStatus() == oglu::FBOStatus::Complete ? u"complete" : u"not complete");
        GLResizer->State().SetTexture(tex, "tex");
        GLResizer->Draw().Draw(vao);
        agent.Await(oglUtil::SyncGL());
        pms->set_value(outtex);
    });

    return ret;
}


}
