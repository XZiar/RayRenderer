#include "TexUtilRely.h"
#include "GLTexResizer.h"
#include "resource.h"
#include "OpenGLUtil/oglPromise.hpp"
#include "common/PromiseTaskSTD.hpp"
#include <future>
#include <thread>


namespace oglu::texutil
{

GLTexResizer::GLTexResizer(oglContext&& glContext) : Executor(u"GLTexResizer"), GLContext(glContext)
{
    Executor.Start([this]
    {
        common::SetThreadName(u"GLTexResizer");
        texLog().success(u"TexResize thread start running.\n");
        if (!GLContext->UseContext())
        {
            texLog().error(u"GLTexResizer cannot use GL context\n");
            return;
        }
        texLog().info(u"GLTexResizer use GL context with version {}\n", oglUtil::getVersion());
        GLContext->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        GLContext->SetSRGBFBO(true);
        GLResizer.reset(u"GLResizer");
        const string shaderSrc = getShaderFromDLL(IDR_SHADER_GLRESIZER);
        try
        {
            GLResizer->AddExtShaders(shaderSrc);
            GLResizer->Link();
        }
        catch (const OGLException& gle)
        {
            texLog().error(u"GLTexResizer shader fail:\n{}\n", gle.message);
            COMMON_THROW(BaseException, u"GLTexResizer shader fail");
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

        OutputFrame.reset();
        GLContext->SetFBO(OutputFrame);

    }, [&]()
    {
        //exit
        if (!GLContext->UnloadContext())
        {
            texLog().error(u"GLTexResizer cannot terminate GL context\n");
        }
        GLContext.release();
    });
}

GLTexResizer::~GLTexResizer()
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
    if (TexFormatUtil::IsGrayType(matched) != TexFormatUtil::IsGrayType(prefer))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"not support to convert between color and gray");
    return matched;
}

static void FilterFormat(const TextureInnerFormat format)
{
    if (TexFormatUtil::IsCompressType(format))
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"not support to resize to a compressed format");
}

common::PromiseResult<Image> GLTexResizer::ExtractImage(common::PromiseResult<oglTex2DS>&& pmsTex, const ImageDataType format)
{
    const auto pms = std::make_shared<std::promise<Image>>();
    auto ret = std::make_shared<common::PromiseResultSTD<Image, false>>(*pms);

    Executor.AddTask([pms, pmsTex, format](const common::asyexe::AsyncAgent& agent)
    {
        const auto outtex = agent.Await(pmsTex);
        pms->set_value(outtex->GetImage(format, true)); // flip it to correctly represent GL's texture
    });

    return ret;
}

common::PromiseResult<Image> GLTexResizer::ResizeToDat(const Image& img, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY)
{
    auto pmsTex = ResizeToTex(img, width, height, DecideFormat(format), flipY);
    return ExtractImage(std::move(pmsTex), format);
}
common::PromiseResult<Image> GLTexResizer::ResizeToDat(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY)
{
    auto pmsTex = ResizeToTex(tex, width, height, DecideFormat(format), flipY);
    return ExtractImage(std::move(pmsTex), format);
}

common::PromiseResult<Image> GLTexResizer::ResizeToDat(const common::AlignedBuffer<32>& data, const std::pair<uint32_t, uint32_t>& size, const TextureInnerFormat dataFormat, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY)
{
    auto pmsTex = ResizeToTex(data, size, dataFormat, width, height, DecideFormat(format), flipY);
    return ExtractImage(std::move(pmsTex), format);
}

common::PromiseResult<oglTex2DS> GLTexResizer::ResizeToTex(const Image& img, const uint16_t width, const uint16_t height, const TextureInnerFormat format, const bool flipY)
{
    FilterFormat(format);
    const auto pms = std::make_shared<std::promise<oglTex2DS>>();
    auto ret = std::make_shared<common::PromiseResultSTD<oglTex2DS, false>>(*pms);

    Executor.AddTask([this, pms, rawimg = std::make_shared<Image>(img), width, height, format, flipY](const common::asyexe::AsyncAgent& agent)
    {
        const auto middleFormat = DecideFormat(rawimg->GetDataType(), format);
        oglTex2DS tex(rawimg->GetWidth(), rawimg->GetHeight(), middleFormat);
        tex->SetData(*rawimg, true, false);
        tex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);
        const auto outtex = agent.Await(ResizeToTex(tex, width, height, format, flipY));
        pms->set_value(outtex);
    });

    return ret;
}

common::PromiseResult<oglTex2DS> GLTexResizer::ResizeToTex(const oglTex2D& tex, const uint16_t width, const uint16_t height, const TextureInnerFormat format, const bool flipY)
{
    FilterFormat(format);
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
        GLContext->SetViewPort(0, 0, width, height);
        GLContext->ClearFBO();
        string routineName = "PlainCopy";
        if (uint8_t(TexFormatUtil::DecideFormat(format) & TextureDataFormat::RAW_FORMAT_MASK) >= uint8_t(TextureDataFormat::RGB_FORMAT))
        {
            switch (TexFormatUtil::DecideFormat(tex->GetInnerFormat()) & TextureDataFormat::RAW_FORMAT_MASK)
            {
            case TextureDataFormat::R_FORMAT:   routineName = "G2RGBA"; break;
            case TextureDataFormat::RG_FORMAT:  routineName = "GA2RGBA"; break;
                // others just keep default
            }
        }
        GLResizer->Draw()
            .SetTexture(tex, "tex")
            .SetSubroutine("ColorConv",routineName)
            .Draw(vao);
        agent.Await(oglUtil::SyncGL());
        pms->set_value(outtex);
    });

    return ret;
}

common::PromiseResult<oglTex2DS> GLTexResizer::ResizeToTex(const common::AlignedBuffer<32>& data, const std::pair<uint32_t, uint32_t>& size, const TextureInnerFormat dataFormat, const uint16_t width, const uint16_t height, const TextureInnerFormat format, const bool flipY)
{
    FilterFormat(format);
    const auto pms = std::make_shared<std::promise<oglTex2DS>>();
    auto ret = std::make_shared<common::PromiseResultSTD<oglTex2DS, false>>(*pms);

    Executor.AddTask([this, pms, rawdata = std::make_shared<common::AlignedBuffer<32>>(data), size, dataFormat, width, height, format, flipY](const common::asyexe::AsyncAgent& agent)
    {
        oglTex2DS tex(size.first, size.second, dataFormat);

        if (TexFormatUtil::IsCompressType(dataFormat))
            tex->SetCompressedData(rawdata->GetRawPtr(), rawdata->GetSize());
        else
            tex->SetData(TexFormatUtil::DecideFormat(dataFormat), rawdata->GetRawPtr());
        tex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);
        const auto outtex = agent.Await(ResizeToTex(tex, width, height, format, flipY));
        pms->set_value(outtex);
    });
    return ret;
}

}
