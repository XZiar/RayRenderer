#include "TexUtilRely.h"
#include "TexResizer.h"
#include "resource.h"
#include <future>
#include <thread>
#include "common/PromiseTaskSTD.hpp"


namespace oglu::texutil
{
using namespace oclu;
using namespace std::literals;

TexResizer::TexResizer(oglContext&& glContext, const oclContext& clContext) : Executor(u"CLTexResizer"), GLContext(glContext), CLContext(clContext)
{
    Executor.Start([this]
    {
        common::SetThreadName(u"TexResizer");
        texLog().success(u"TexResize thread start running.\n");
        if (!GLContext->UseContext())
        {
            texLog().error(u"TexResizer cannot use GL context\n");
            return;
        }
        texLog().info(u"TexResizer use GL context with version {}\n", oglUtil::GetVersionStr());
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
        try
        {
            GLResizer2.reset(u"GLResizer2", shaderSrc);
        }
        catch (const OGLException& gle)
        {
            texLog().error(u"GLResizer2 shader fail:\n{}\n", gle.message);
            texLog().warning(u"Compute Shader is disabled");
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

        if (CLContext)
        {
            CmdQue.reset(CLContext, CLContext->GetGPUDevice());
            if (!CmdQue)
                COMMON_THROW(BaseException, u"clQueue initialized failed!");
            oclProgram clProg(CLContext, getShaderFromDLL(IDR_SHADER_CLRESIZER));
            try
            {
                oclu::CLProgConfig config;
                clProg->Build(config); 
                KerToImg = clProg->GetKernel("ResizeToImg");
                KerToDat3 = clProg->GetKernel("ResizeToDat3");
                KerToDat4 = clProg->GetKernel("ResizeToDat4");
                const auto wgInfo = KerToImg->GetWorkGroupInfo(CLContext->Devices[0]);
                texLog().info(u"kernel compiled workgroup size [{}x{}x{}], uses [{}] private mem\n", wgInfo.CompiledWorkGroupSize[0], wgInfo.CompiledWorkGroupSize[1], wgInfo.CompiledWorkGroupSize[2], wgInfo.PrivateMemorySize);
            }
            catch (OCLException& cle)
            {
                u16string buildLog;
                if (cle.data.has_value())
                    buildLog = std::any_cast<u16string>(cle.data);
                texLog().error(u"Fail to build opencl Program:{}\n{}\n", cle.message, buildLog);
            }
        }
        else
            texLog().warning(u"TexResizer has no shared CL context attached\n");
        if (!KerToImg)
        {
            texLog().warning(u"OpenCL Kernel is disabled");
        }
    }, [this]
    {
        KerToImg.release(); KerToDat3.release(); KerToDat4.release();
        CmdQue.release();
        CLContext.release();
        //exit
        GLResizer.release();
        GLResizer2.release();
        ScreenBox.release();
        NormalVAO.release();
        FlipYVAO.release();
        OutputFrame.release();
        if (!GLContext->UnloadContext())
        {
            texLog().error(u"TexResizer cannot terminate GL context\n");
        }
        GLContext.release();
    });
}

TexResizer::~TexResizer()
{
    Executor.Stop();
}

struct ImageInfo
{
    uint32_t SrcWidth;
    uint32_t SrcHeight;
    uint32_t DestWidth;
    uint32_t DestHeight;
    float WidthStep;
    float HeightStep;
};

static TextureDataFormat FixFormat(const TextureDataFormat dformat)
{
    switch (dformat)
    {
    case TextureDataFormat::RGB8:       return TextureDataFormat::RGBA8;
    case TextureDataFormat::BGR8:       return TextureDataFormat::BGRA8;
    default:                            return dformat;
    }
}

static void FilterFormat(const TextureInnerFormat format)
{
    if (TexFormatUtil::IsCompressType(format))
        COMMON_THROW(OGLException, OGLException::GLComponent::Tex, u"not support to resize to a compressed format");
}
static string_view GetSubroutine(const TextureInnerFormat input, const TextureInnerFormat output)
{
    if (static_cast<uint16_t>(output & TextureInnerFormat::CHANNEL_MASK) > static_cast<uint16_t>(input & TextureInnerFormat::CHANNEL_MASK))
    { // need more channel
        switch (input & TextureInnerFormat::CHANNEL_MASK)
        {
        case TextureInnerFormat::CHANNEL_R:     return "G2RGBA"sv;
        case TextureInnerFormat::CHANNEL_RG:    return "GA2RGBA"sv;
        default:                                break; // others just keep default
        }
    }
    return "PlainCopy"sv;
}


PromiseResult<oglTex2D> TexResizer::ConvertToTex(const oclImg2D& img)
{
    const auto glImg = img.cast_dynamic<oclu::detail::_oclGLImage2D>();
    if (glImg)
        return common::GetFinishedResult<oglTex2D>(glImg->GlTex);
    else
        return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
        {
            auto rawimg = agent.Await(img->ReadRaw(CmdQue));
            oglTex2DS tex(img->Width, img->Height, TexFormatUtil::ConvertFrom(img->GetFormat()));
            tex->SetData(img->GetFormat(), rawimg.GetRawPtr());
            agent.Await(oglUtil::SyncGL());
            return (oglTex2D)tex;
        });
}
PromiseResult<Image> TexResizer::ExtractImage(common::PromiseResult<oglTex2DS>&& pmsTex, const ImageDataType format)
{
    return Executor.AddTask([pmsTex, format](const common::asyexe::AsyncAgent& agent)
    {
        const auto outtex = agent.Await(pmsTex);
        return outtex->GetImage(format, true); // flip it to correctly represent GL's texture
    });
}

static oglTex2D ConvertCLToTex(const oclImg2D& img, const oclCmdQue& que, const common::asyexe::AsyncAgent& agent)
{
    const auto glImg = img.cast_dynamic<oclu::detail::_oclGLImage2D>();
    if (glImg)
        return glImg->GlTex;
    else
    {
        auto rawimg = agent.Await(img->ReadRaw(que));
        oglTex2DS tex(img->Width, img->Height, TexFormatUtil::ConvertFrom(img->GetFormat()));
        tex->SetData(img->GetFormat(), rawimg.GetRawPtr());
        tex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::ClampEdge);
        agent.Await(oglUtil::SyncGL());
        return (oglTex2D)tex;
    }
}
static oglTex2DS ConvertDataToTex(const common::AlignedBuffer<32>& data, const std::pair<uint32_t, uint32_t>& size, const TextureInnerFormat innerFormat)
{
    oglTex2DS tex(size.first, size.second, innerFormat);
    if (TexFormatUtil::IsCompressType(innerFormat))
        tex->SetCompressedData(data.GetRawPtr(), data.GetSize());
    else
        tex->SetData(TexFormatUtil::ConvertDtypeFrom(innerFormat), data.GetRawPtr());
    tex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::ClampEdge);
    return tex;
}
template<>
TEXUTILAPI PromiseResult<oglTex2DS> TexResizer::ResizeToTex<ResizeMethod::OpenGL>(const oglTex2D& tex, const uint16_t width, const uint16_t height, const TextureInnerFormat output, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        const auto vao = flipY ? FlipYVAO : NormalVAO;
        tex->CheckCurrent();
        oglTex2DS outtex(width, height, output);
        outtex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);

        OutputFrame->AttachColorTexture(outtex, 0);
        oglRBO mainRBO(width, height, oglu::RBOFormat::Depth);
        OutputFrame->AttachDepthTexture(mainRBO);
        texLog().info(u"FBO resize to [{}x{}], status:{}\n", width, height, OutputFrame->CheckStatus() == oglu::FBOStatus::Complete ? u"complete" : u"not complete");
        GLContext->SetViewPort(0, 0, width, height);
        GLContext->ClearFBO();
        GLResizer->Draw()
            .SetTexture(tex, "tex")
            .SetSubroutine("ColorConv", GetSubroutine(tex->GetInnerFormat(), output))
            .Draw(vao);
        agent.Await(oglUtil::SyncGL());
        return outtex;
    });
}
template<>
TEXUTILAPI PromiseResult<oglTex2DS> TexResizer::ResizeToTex<ResizeMethod::Compute>(const oglTex2D& tex, const uint16_t width, const uint16_t height, const TextureInnerFormat output, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        const auto outformat = REMOVE_MASK(output, TextureInnerFormat::FLAG_SRGB);
        tex->CheckCurrent();
        oglTex2DS outtex(width, height, outformat);
        outtex->SetProperty(TextureFilterVal::Linear, TextureWrapVal::Repeat);
        oglImg2D outimg(outtex, TexImgUsage::WriteOnly);
        b3d::Coord2D coordStep(1.0f / width, 1.0f / height);
        GLResizer2->SetVec("coordStep", coordStep);
        GLResizer2->SetUniform("isSrgbDst", true);
        GLResizer2->State()
            .SetTexture(tex, "tex")
            .SetImage(outimg, "result")
            .SetSubroutine("ColorConv", GetSubroutine(tex->GetInnerFormat(), outformat));
        GLResizer2->Run(width, height);
        GLResizer2->State()
            .SetTexture({}, "tex")
            .SetImage({}, "result");
        agent.Await(oglUtil::SyncGL());
        return outtex;
    });
}

template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenGL>(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY)
{
    return ExtractImage(ResizeToTex<ResizeMethod::OpenGL>(tex, width, height, TexFormatUtil::ConvertFrom(output, true), flipY), output);
}

template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::Compute>(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY)
{
    return ExtractImage(ResizeToTex<ResizeMethod::Compute>(tex, width, height, TexFormatUtil::ConvertFrom(output, true), flipY), output);
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenGL>(const oclu::oclImg2D& img, const bool isSRGB, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        const auto tex = ConvertCLToTex(img, CmdQue, agent);
        auto pms = ResizeToImg<ResizeMethod::OpenGL>(tex, width, height, output, flipY);
        return agent.Await(pms);
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::Compute>(const oclu::oclImg2D& img, const bool isSRGB, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        const auto tex = ConvertCLToTex(img, CmdQue, agent);
        auto pms = ResizeToImg<ResizeMethod::Compute>(tex, width, height, output, flipY);
        return agent.Await(pms);
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenCL>(const oclu::oclImg2D& img, const bool isSRGB, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        if (isSRGB)
            COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"sRGB texture is not supported on OpenCL");
        const auto& ker = HAS_FIELD(output, ImageDataType::ALPHA_MASK) ? KerToDat4 : KerToDat3;
        const auto eleSize = HAS_FIELD(output, ImageDataType::ALPHA_MASK) ? 4 : 3;
        oclBuffer outBuf(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly, width*height*eleSize);
        ImageInfo info{ img->Width, img->Height, width, height, 1.0f / width, 1.0f / height };
        ker->SetArg(0, img);
        ker->SetArg(1, outBuf);
        ker->SetSimpleArg(2, 1u);
        ker->SetSimpleArg(3, info);

        const size_t worksize[] = { width, height };
        auto pms = ker->Run<2>(CmdQue, worksize, false);
        agent.Await(common::PromiseResult<void>(pms));
        texLog().success(u"CLTexResizer Kernel runs {}us.\n", pms->ElapseNs() / 1000);

        Image result(HAS_FIELD(output, ImageDataType::ALPHA_MASK) ? ImageDataType::RGBA : ImageDataType::RGB);
        result.SetSize(width, height);
        agent.Await(outBuf->Read(CmdQue, result.GetRawPtr(), result.GetSize(), 0, false));
        if (result.GetDataType() != output)
            result = result.ConvertTo(output);
        return result;
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenCL>(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent) 
    {
        if (TexFormatUtil::IsSRGBType(tex->GetInnerFormat()))
            COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"sRGB texture is not supported on OpenCL");
        oclGLImg2D img(CLContext, MemFlag::ReadOnly | MemFlag::HostNoAccess, tex);
        auto pms = ResizeToImg<ResizeMethod::OpenCL>(img, false, width, height, output, flipY);
        return agent.Await(pms);
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenGL>(const Image& img, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        oglTex2DS tex(img.GetWidth(), img.GetHeight(), TexFormatUtil::ConvertFrom(img.GetDataType(), true));
        tex->SetData(img, true);
        agent.Await(oglUtil::SyncGL());
        auto pms = ResizeToImg<ResizeMethod::OpenGL>(tex, width, height, output, flipY);
        return agent.Await(pms);
    });
}
template<>
TEXUTILAPI PromiseResult<oglTex2DS> TexResizer::ResizeToTex<ResizeMethod::OpenCL>(const oglTex2D& tex, const uint16_t width, const uint16_t height, const TextureInnerFormat output, const bool flipY)
{
    auto pms = ResizeToImg<ResizeMethod::OpenCL>(tex, width, height, TexFormatUtil::ConvertToImgType(output, true), flipY);
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        const auto img = agent.Await(pms);
        oglTex2DS tex(width, height, output);
        tex->SetData(TexFormatUtil::ConvertDtypeFrom(img.GetDataType(), true), img.GetRawPtr());
        agent.Await(oglUtil::SyncGL());
        return tex;
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::Compute>(const Image& img, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        oglTex2DS tex(img.GetWidth(), img.GetHeight(), TexFormatUtil::ConvertFrom(img.GetDataType(), true));
        tex->SetData(img, true);
        agent.Await(oglUtil::SyncGL());
        auto pms = ResizeToImg<ResizeMethod::Compute>(tex, width, height, output, flipY);
        return agent.Await(pms);
    });
}
//template<>
//TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenCL>(const Image& img, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY)
//{
//    return {};
//}

template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenGL>(const common::AlignedBuffer<32>& data, const std::pair<uint32_t, uint32_t>& size, const TextureInnerFormat innerFormat, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        oglTex2DS tex = ConvertDataToTex(data, size, innerFormat);
        agent.Await(oglUtil::SyncGL());
        auto pms = ResizeToImg<ResizeMethod::OpenGL>(tex, width, height, output, flipY);
        return agent.Await(pms);
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::Compute>(const common::AlignedBuffer<32>& data, const std::pair<uint32_t, uint32_t>& size, const TextureInnerFormat innerFormat, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        oglTex2DS tex = ConvertDataToTex(data, size, innerFormat);
        agent.Await(oglUtil::SyncGL());
        auto pms = ResizeToImg<ResizeMethod::Compute>(tex, width, height, output, flipY);
        return agent.Await(pms);
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenCL>(const common::AlignedBuffer<32>& data, const std::pair<uint32_t, uint32_t>& size, const TextureInnerFormat innerFormat, const uint16_t width, const uint16_t height, const ImageDataType output, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        if (TexFormatUtil::IsSRGBType(innerFormat))
            COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"sRGB texture is not supported on OpenCL");
        oclImg2D img(CLContext, MemFlag::ReadOnly | MemFlag::HostWriteOnly, size.first, size.second, TexFormatUtil::ConvertDtypeFrom(innerFormat));
        agent.Await(img->Write(CmdQue, data, false));
        auto pms = ResizeToImg<ResizeMethod::OpenCL>(img, false, width, height, output, flipY);
        return agent.Await(pms);
    });
}







}
