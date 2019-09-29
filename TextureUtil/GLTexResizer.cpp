#include "TexUtilRely.h"
#include "GLTexResizer.h"
#include "resource.h"
#include "OpenGLUtil/oglPromise.hpp"
#include "OpenGLUtil/PointEnhance.hpp"
#include "common/PromiseTaskSTD.hpp"
#include <future>
#include <thread>


namespace oglu::texutil
{
using namespace std::literals;
using xziar::img::TexFormatUtil;

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
        texLog().info(u"GLTexResizer use GL context with version {}\n", oglUtil::GetVersionStr());
        GLContext->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        GLContext->SetSRGBFBO(true);
        GLContext->SetDepthTest(DepthTestType::OFF);
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
        GLResizer2.reset(u"GLResizer2");
        try
        {
            GLResizer2->AddExtShaders(shaderSrc);
            GLResizer2->Link();
        }
        catch (const OGLException& gle)
        {
            texLog().error(u"GLResizer2 shader fail:\n{}\n", gle.message);
            //COMMON_THROW(BaseException, u"GLResizer2 shader fail");
        }

        ScreenBox.reset();
        const Vec4 pa(-1.0f, -1.0f, 0.0f, 0.0f), pb(1.0f, -1.0f, 1.0f, 0.0f), pc(-1.0f, 1.0f, 0.0f, 1.0f), pd(1.0f, 1.0f, 1.0f, 1.0f);
        const Vec4 paf(-1.0f, -1.0f, 0.0f, 1.0f), pbf(1.0f, -1.0f, 1.0f, 1.0f), pcf(-1.0f, 1.0f, 0.0f, 0.0f), pdf(1.0f, 1.0f, 1.0f, 0.0f);
        const Vec4 DatVert[] = { pa,pb,pc, pd,pc,pb, paf,pbf,pcf, pdf,pcf,pbf };
        ScreenBox->Write(DatVert, sizeof(DatVert));

        NormalVAO.reset(VAODrawMode::Triangles);
        NormalVAO->Prepare()
            .SetFloat(ScreenBox, GLResizer->GetLoc("@VertPos"), sizeof(Vec4), 2, 0)
            .SetFloat(ScreenBox, GLResizer->GetLoc("@VertTexc"), sizeof(Vec4), 2, sizeof(float) * 2)
            .SetDrawSize(0, 6);
        FlipYVAO.reset(VAODrawMode::Triangles);
        FlipYVAO->Prepare()
            .SetFloat(ScreenBox, GLResizer->GetLoc("@VertPos"), sizeof(Vec4), 2, 0)
            .SetFloat(ScreenBox, GLResizer->GetLoc("@VertTexc"), sizeof(Vec4), 2, sizeof(float) * 2)
            .SetDrawSize(6, 6);

        OutputFrame.reset();
        OutputFrame->Use();

    }, [&]()
    {
        //exit
        GLResizer.release();
        GLResizer2.release();
        ScreenBox.release();
        NormalVAO.release();
        FlipYVAO.release();
        OutputFrame.release();
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

static TextureFormat DecideFormat(const ImageDataType& type)
{
    switch (type)
    {
    case ImageDataType::GRAY:       return TextureFormat::R8;
    case ImageDataType::GA:         return TextureFormat::RG8;
    case ImageDataType::BGR:        [[fallthrough]];
    case ImageDataType::RGB:        return TextureFormat::SRGB8;
    case ImageDataType::BGRA:       [[fallthrough]];
    case ImageDataType::RGBA:       return TextureFormat::SRGBA8;

    case ImageDataType::GRAYf:       return TextureFormat::Rf;
    case ImageDataType::GAf:         return TextureFormat::RGf;
    case ImageDataType::BGRf:        [[fallthrough]];
    case ImageDataType::RGBf:        return TextureFormat::RGBf;
    case ImageDataType::BGRAf:       [[fallthrough]];
    case ImageDataType::RGBAf:       return TextureFormat::RGBAf;
    default:                         return TextureFormat::SRGBA8;
    }
}

static TextureFormat DecideFormat(ImageDataType type, const TextureFormat prefer)
{
    if (!TexFormatUtil::HasAlpha(prefer))
        type = REMOVE_MASK(type, ImageDataType::ALPHA_MASK);
    const TextureFormat matched = DecideFormat(type);
    if (TexFormatUtil::IsMonoColor(matched) != TexFormatUtil::IsMonoColor(prefer))
        COMMON_THROW(OGLException, OGLException::GLComponent::Tex, u"not support to convert between color and gray");
    return matched;
}

static void FilterFormat(const TextureFormat format)
{
    if (TexFormatUtil::IsCompressType(format))
        COMMON_THROW(OGLException, OGLException::GLComponent::Tex, u"not support to resize to a compressed format");
}

common::PromiseResult<Image> GLTexResizer::ExtractImage(common::PromiseResult<oglTex2DS>&& pmsTex, const ImageDataType format)
{
    return Executor.AddTask([pmsTex, format](const common::asyexe::AsyncAgent& agent)
    {
        const auto outtex = agent.Await(pmsTex);
        return outtex->GetImage(format, true); // flip it to correctly represent GL's texture
    });
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

common::PromiseResult<Image> GLTexResizer::ResizeToDat(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const TextureFormat dataFormat, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY)
{
    auto pmsTex = ResizeToTex(data, size, dataFormat, width, height, DecideFormat(format), flipY);
    return ExtractImage(std::move(pmsTex), format);
}

common::PromiseResult<oglTex2DS> GLTexResizer::ResizeToTex(const Image& img, const uint16_t width, const uint16_t height, const TextureFormat format, const bool flipY)
{
    FilterFormat(format);

    return Executor.AddTask([this, rawimg = std::make_shared<Image>(img), width, height, format, flipY](const common::asyexe::AsyncAgent& agent)
    {
        const auto middleFormat = DecideFormat(rawimg->GetDataType(), format);
        oglTex2DS tex(rawimg->GetWidth(), rawimg->GetHeight(), middleFormat);
        tex->SetData(*rawimg, true, false);
        tex->SetProperty(TextureFilterVal::BothLinear, TextureWrapVal::Repeat);
        return agent.Await(ResizeToTex(tex, width, height, format, flipY));
    });
}

common::PromiseResult<oglTex2DS> GLTexResizer::ResizeToTex(const oglTex2D& tex, const uint16_t width, const uint16_t height, const TextureFormat format, const bool flipY)
{
    FilterFormat(format);
    const auto vao = flipY ? FlipYVAO : NormalVAO;
    const auto inFormat = tex->GetInnerFormat();
    //if ((output & TextureFormat::MASK_CHANNEL) == (input & TextureFormat::MASK_CHANNEL))
    //    return "PlainCopy"sv;
    string_view routineName = "PlainCopy"sv;
    if ((format & TextureFormat::MASK_CHANNEL_RAW) == TextureFormat::CHANNEL_RGBA)
    {
        if ((inFormat & TextureFormat::MASK_CHANNEL_RAW) == TextureFormat::CHANNEL_RG)
            routineName = "GA2RGBA"sv; // need more channel
        if ((inFormat & TextureFormat::MASK_CHANNEL_RAW) == TextureFormat::CHANNEL_R)
            routineName = "G2RGBA"sv; // need more channel
    }

    if (true)
        return Executor.AddTask([this, tex, width, height, format, vao, rt = routineName](const common::asyexe::AsyncAgent& agent)
        {
            tex->CheckCurrent();
            oglTex2DS outtex(width, height, format);
            outtex->SetProperty(TextureFilterVal::BothLinear, TextureWrapVal::Repeat);
        
            OutputFrame->AttachColorTexture(outtex, 0);
            oglRBO mainRBO(width, height, oglu::RBOFormat::Depth);
            OutputFrame->AttachDepthTexture(mainRBO);
            texLog().info(u"FBO resize to [{}x{}], status:{}\n", width, height, OutputFrame->CheckStatus() == oglu::FBOStatus::Complete ? u"complete" : u"not complete");
            GLContext->SetViewPort(0, 0, width, height);
            GLContext->ClearFBO();
            GLResizer->Draw()
                .SetTexture(tex, "tex")
                .SetSubroutine("ColorConv", rt)
                .Draw(vao);
            agent.Await(oglUtil::SyncGL());
            return outtex;
        });
    else
        return Executor.AddTask([this, tex, width, height, format, rt = routineName](const common::asyexe::AsyncAgent& agent)
        {
            tex->CheckCurrent();
            oglTex2DS outtex(width, height, REMOVE_MASK(format, TextureFormat::MASK_SRGB) | TextureFormat::CHANNEL_A);
            outtex->SetProperty(TextureFilterVal::BothLinear, TextureWrapVal::Repeat);
            oglImg2D outimg(outtex, TexImgUsage::WriteOnly);
            b3d::Coord2D coordStep(1.0f / width, 1.0f / height);
            GLResizer2->SetVec("coordStep", coordStep);
            GLResizer2->SetUniform("isSrgbDst", TexFormatUtil::IsSRGBType(format));
            GLResizer2->State()
                .SetTexture(tex, "tex")
                .SetImage(outimg, "result")
                .SetSubroutine("ColorConv", rt);
            GLResizer2->Run(width, height);
            GLResizer2->State()
                .SetTexture({}, "tex")
                .SetImage({}, "result");
            agent.Await(oglUtil::SyncGL());
            return outtex;
        });
}

common::PromiseResult<oglTex2DS> GLTexResizer::ResizeToTex(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const TextureFormat dataFormat, const uint16_t width, const uint16_t height, const TextureFormat format, const bool flipY)
{
    FilterFormat(format);

    return Executor.AddTask([this, rawdata = std::make_shared<common::AlignedBuffer>(data), size, dataFormat, width, height, format, flipY](const common::asyexe::AsyncAgent& agent)
    {
        oglTex2DS tex(size.first, size.second, dataFormat);

        if (TexFormatUtil::IsCompressType(dataFormat))
            tex->SetCompressedData(rawdata->GetRawPtr(), rawdata->GetSize());
        else
            tex->SetData(dataFormat, rawdata->GetRawPtr());
        tex->SetProperty(TextureFilterVal::BothLinear, TextureWrapVal::Repeat);
        return agent.Await(ResizeToTex(tex, width, height, format, flipY));
    });
}

}
