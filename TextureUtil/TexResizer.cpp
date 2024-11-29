#include "TexUtilPch.h"
#include "TexResizer.h"
#include "TexUtilWorker.h"
#include "resource.h"


namespace oglu::texutil
{
using std::string;
using std::string_view;
using std::u16string;
using common::BaseException;
using common::PromiseResult;
using namespace xziar::img;
using namespace oclu;
using namespace std::literals;

TexResizer::TexResizer(const std::shared_ptr<TexUtilWorker>& worker) : Worker(worker)
{
    Worker->AddTask([this](const auto&)
    {
        GLContext = Worker->GLContext;
        CLContext = Worker->CLContext;
        CmdQue = Worker->CmdQue;
        GLContext->SetSRGBFBO(true);
        GLContext->SetDepthTest(DepthTestType::OFF);
        const auto shaderTxt = LoadShaderFromDLL(IDR_SHADER_GLRESIZER);
        try
        {
            GLResizer = oglDrawProgram_::Create(u"GLResizer", shaderTxt);
        }
        catch (const OGLException& gle)
        {
            texLog().Error(u"GLTexResizer shader fail:{}\n{}\n", gle.Message(), gle.GetDetailMessage());
            COMMON_THROW(BaseException, u"GLTexResizer shader fail");
        }
        if (oglComputeProgram_::CheckSupport())
        {
            try
            {
                GLResizer2 = oglComputeProgram_::Create(u"GLResizer2", shaderTxt);
            }
            catch (const OGLException& gle)
            {
                texLog().Error(u"GLResizer2 shader fail:{}\n{}\n", gle.Message(), gle.GetDetailMessage());
                texLog().Warning(u"Compute Shader is disabled");
            }
        }
        ScreenBox = oglArrayBuffer_::Create();
        const mbase::Vec4 pa(-1.0f, -1.0f, 0.0f, 0.0f), pb(1.0f, -1.0f, 1.0f, 0.0f), pc(-1.0f, 1.0f, 0.0f, 1.0f), pd(1.0f, 1.0f, 1.0f, 1.0f);
        const mbase::Vec4 paf(-1.0f, -1.0f, 0.0f, 1.0f), pbf(1.0f, -1.0f, 1.0f, 1.0f), pcf(-1.0f, 1.0f, 0.0f, 0.0f), pdf(1.0f, 1.0f, 1.0f, 0.0f);
        const mbase::Vec4 DatVert[] = { pa,pb,pc, pd,pc,pb, paf,pbf,pcf, pdf,pcf,pbf };
        ScreenBox->SetName(u"TexResizerScreenBox");
        ScreenBox->WriteSpan(DatVert);

        NormalVAO = oglVAO_::Create(VAODrawMode::Triangles);
        NormalVAO->SetName(u"TexResizerNormalVAO");
        NormalVAO->Prepare(GLResizer)
            .SetFloat(ScreenBox, "@VertPos",  sizeof(mbase::Vec4), 2, 0)
            .SetFloat(ScreenBox, "@VertTexc", sizeof(mbase::Vec4), 2, sizeof(float) * 2)
            .SetDrawSize(0, 6);
        FlipYVAO = oglVAO_::Create(VAODrawMode::Triangles);
        FlipYVAO->SetName(u"TexResizerFlipYVAO");
        FlipYVAO->Prepare(GLResizer)
            .SetFloat(ScreenBox, "@VertPos",  sizeof(mbase::Vec4), 2, 0)
            .SetFloat(ScreenBox, "@VertTexc", sizeof(mbase::Vec4), 2, sizeof(float) * 2)
            .SetDrawSize(6, 6);

        if (CLContext)
        {
            try
            {
                oclu::CLProgConfig config;
                auto clProg = oclProgram_::CreateAndBuild(CLContext, LoadShaderFromDLL(IDR_SHADER_CLRESIZER), config, CLContext->Devices[0]);
                KerToImg = clProg->GetKernel("ResizeToImg");
                KerToDat3 = clProg->GetKernel("ResizeToDat3");
                KerToDat4 = clProg->GetKernel("ResizeToDat4");
                const auto& wgInfo = KerToImg->WgInfo;
                texLog().Info(u"kernel compiled workgroup size [{}x{}x{}], uses [{}] pmem and [{}] smem\n",
                    wgInfo.CompiledWorkGroupSize[0], wgInfo.CompiledWorkGroupSize[1], wgInfo.CompiledWorkGroupSize[2], wgInfo.PrivateMemorySize, wgInfo.LocalMemorySize);
            }
            catch (const OCLException& cle)
            {
                texLog().Error(u"Fail to build opencl Program:{}\n{}\n", cle.Message(), cle.GetDetailMessage());
            }
        }
        else
            texLog().Warning(u"TexResizer has no shared CL context attached\n");
        if (!KerToImg)
        {
            texLog().Warning(u"OpenCL Kernel is disabled.\n");
        }
    })->Get();
}

TexResizer::~TexResizer()
{
    Worker->AddTask([this](const auto&)
    {
        KerToImg.reset(); KerToDat3.reset(); KerToDat4.reset();
        //exit
        GLResizer.reset();
        GLResizer2.reset();
        ScreenBox.reset();
        NormalVAO.reset();
        FlipYVAO.reset();
    })->Get();
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

static TextureFormat FixFormat(const TextureFormat dformat) // for Compute Shader
{
    switch (dformat)
    {
    case TextureFormat::RGB8:       return TextureFormat::RGBA8;
    case TextureFormat::BGR8:       return TextureFormat::BGRA8;
    default:                            return dformat;
    }
}

static void FilterFormat(const TextureFormat format)
{
    if (TexFormatUtil::IsCompressType(format))
        COMMON_THROW(OGLException, OGLException::GLComponent::Tex, u"not support to resize to a compressed format");
}
static string_view GetSubroutine(const TextureFormat, const TextureFormat output)
{
    //if ((output & TextureFormat::MASK_CHANNEL) == (input & TextureFormat::MASK_CHANNEL))
    //    return "PlainCopy"sv;
    if ((output & TextureFormat::MASK_CHANNEL_RAW) == TextureFormat::CHANNEL_RGBA)
    {
        if ((output & TextureFormat::MASK_CHANNEL_RAW) == TextureFormat::CHANNEL_RG)
            return "GA2RGBA"sv; // need more channel
        if ((output & TextureFormat::MASK_CHANNEL_RAW) == TextureFormat::CHANNEL_R)
            return "G2RGBA"sv; // need more channel
    }
    return "PlainCopy"sv;
}


PromiseResult<Image> TexResizer::ExtractImage(common::PromiseResult<oglTex2DS>&& pmsTex, const ImgDType format)
{
    return Worker->AddTask([pmsTex, format](const common::asyexe::AsyncAgent& agent)
    {
        const auto outtex = agent.Await(pmsTex);
        return outtex->GetImage(format, true); // flip it to correctly represent GL's texture
    });
}

static oglTex2D ConvertCLToTex(const oclImg2D& img, const oclCmdQue& que, const common::asyexe::AsyncAgent& agent, const bool isSRGB)
{
    if (const auto glImg = std::dynamic_pointer_cast<oclu::oclGLImage2D_>(img); glImg)
    {
        COMMON_THROW(OGLException, OGLException::GLComponent::Tex, u"oclImg2D comes from GL Tex and being locked, can not be used with OpenGL.");
    }
    else
    {
        auto ptr = img->Map(que, oclu::MapFlag::Read);
        auto format = img->GetFormat();
        if (isSRGB)
            format |= TextureFormat::MASK_SRGB;
        auto tex = oglTex2DStatic_::Create(img->Width, img->Height, format);
        tex->SetData(img->GetFormat(), ptr.Get());
        tex->SetProperty(TextureFilterVal::BothLinear, TextureWrapVal::ClampEdge);
        agent.Await(oglUtil::SyncGL());
        return (oglTex2D)tex;
    }
}
static oglTex2DS ConvertDataToTex(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const TextureFormat innerFormat)
{
    auto tex = oglTex2DStatic_::Create(size.first, size.second, innerFormat);
    if (TexFormatUtil::IsCompressType(innerFormat))
        tex->SetCompressedData(data.AsSpan());
    else
        tex->SetData(innerFormat, data.AsSpan());
    tex->SetProperty(TextureFilterVal::BothLinear, TextureWrapVal::ClampEdge);
    return tex;
}
template<>
TEXUTILAPI PromiseResult<oglTex2DS> TexResizer::ResizeToTex<ResizeMethod::OpenGL>(const oglTex2D& tex, const uint16_t width, const uint16_t height, const TextureFormat output, const bool flipY)
{
    return Worker->AddTask([=, this](const common::asyexe::AsyncAgent& agent)
    {
        const auto vao = flipY ? FlipYVAO : NormalVAO;
        tex->CheckCurrent();
        auto outtex = oglTex2DStatic_::Create(width, height, output);
        outtex->SetProperty(TextureFilterVal::BothLinear, TextureWrapVal::Repeat);

        const auto frame = oglFrameBuffer2D_::Create();
        frame->SetName(u"TexResizerFBO");
        frame->AttachColorTexture(outtex);
        texLog().Info(u"FBO resize to [{}x{}], status:{}\n", width, height, frame->CheckStatus() == oglu::FBOStatus::Complete ? u"complete" : u"not complete");
        frame->Use();
        GLResizer->Draw()
            .SetTexture(tex, "tex")
            .SetSubroutine("ColorConv", GetSubroutine(tex->GetInnerFormat(), output))
            .Draw(vao);
        agent.Await(oglUtil::SyncGL());
        return outtex;
    });
}
template<>
TEXUTILAPI PromiseResult<oglTex2DS> TexResizer::ResizeToTex<ResizeMethod::Compute>(const oglTex2D& tex, const uint16_t width, const uint16_t height, const TextureFormat output, const bool flipY)
{
    return Worker->AddTask([=, this](const common::asyexe::AsyncAgent& agent)
    {
        const auto outformat = REMOVE_MASK(output, TextureFormat::MASK_SRGB);
        tex->CheckCurrent();
        auto outtex = oglTex2DStatic_::Create(width, height, outformat);
        outtex->SetProperty(TextureFilterVal::BothLinear, TextureWrapVal::Repeat);
        auto outimg = oglImg2D_::Create(outtex, TexImgUsage::WriteOnly);
        mbase::Vec2 coordStep(1.0f / width, 1.0f / height);
        const auto& localSize = GLResizer2->GetLocalSize();
        GLResizer2->SetVec("coordStep", coordStep);
        GLResizer2->SetVal("isSrgbDst", HAS_FIELD(output, TextureFormat::MASK_SRGB));
        GLResizer2->SetVal("isFlipY", flipY);
        GLResizer2->State()
            .SetTexture(tex, "tex")
            .SetImage(outimg, "result")
            .SetSubroutine("ColorConv", GetSubroutine(tex->GetInnerFormat(), outformat));
        GLResizer2->Run(width / localSize[0], height / localSize[1]);
        GLResizer2->State()
            .SetTexture({}, "tex")
            .SetImage({}, "result");
        agent.Await(oglUtil::SyncGL());
        return outtex;
    });
}

template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenGL>(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImgDType output, const bool flipY)
{
    return ExtractImage(ResizeToTex<ResizeMethod::OpenGL>(tex, width, height, TexFormatUtil::FromImageDType(output, true), flipY), output);
}

template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::Compute>(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImgDType output, const bool flipY)
{
    return ExtractImage(ResizeToTex<ResizeMethod::Compute>(tex, width, height, TexFormatUtil::FromImageDType(output, true), flipY), output);
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenGL>(const oclu::oclImg2D& img, const bool isSRGB, const uint16_t width, const uint16_t height, const ImgDType output, const bool flipY)
{
    return Worker->AddTask([=, this](const common::asyexe::AsyncAgent& agent)
    {
        const auto tex = ConvertCLToTex(img, CmdQue, agent, isSRGB);
        auto pms = ResizeToImg<ResizeMethod::OpenGL>(tex, width, height, output, flipY);
        return agent.Await(pms);
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::Compute>(const oclu::oclImg2D& img, const bool isSRGB, const uint16_t width, const uint16_t height, const ImgDType output, const bool flipY)
{
    return Worker->AddTask([=, this](const common::asyexe::AsyncAgent& agent)
    {
        const auto tex = ConvertCLToTex(img, CmdQue, agent, isSRGB);
        auto pms = ResizeToImg<ResizeMethod::Compute>(tex, width, height, output, flipY);
        return agent.Await(pms);
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenCL>(const oclImg2D& img, const bool isSRGB, const uint16_t width, const uint16_t height, const ImgDType output, const bool flipY)
{
    return Worker->AddTask([=, this](const common::asyexe::AsyncAgent& agent)
    {
        if (isSRGB)
            COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"sRGB texture is not supported on OpenCL");
        const auto& ker = output.HasAlpha() ? KerToDat4 : KerToDat3;
        const auto eleSize = output.HasAlpha() ? 4 : 3;
        common::AlignedBuffer buffer(width*height*eleSize, 4096);
        auto outBuf = oclBuffer_::Create(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly | MemFlag::UseHost, buffer.GetSize(), buffer.GetRawPtr());
        ImageInfo info{ img->Width, img->Height, width, height, 1.0f / width, 1.0f / height };

        const size_t worksize[] = { width, height };
        auto pms = ker->Call<2>(img, outBuf, 1u, info)(CmdQue, worksize);
        agent.Await(pms);
        texLog().Success(u"CLTexResizer Kernel runs {}us.\n", pms->ElapseNs() / 1000);
        outBuf->Flush(CmdQue);
        outBuf.reset();
        Image result(std::move(buffer), width, height, output.HasAlpha() ? ImageDataType::RGBA : ImageDataType::RGB);
        if (flipY)
            result.FlipVertical();
        if (result.GetDataType() != output)
            result = result.ConvertTo(output);
        return result;
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenCL>(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImgDType output, const bool flipY)
{
    return Worker->AddTask([=, this](const common::asyexe::AsyncAgent& agent) 
    {
        if (TexFormatUtil::IsSRGBType(tex->GetInnerFormat()))
            COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"sRGB texture is not supported on OpenCL");
        auto img = oclGLInterImg2D_::Create(CLContext, MemFlag::ReadOnly | MemFlag::HostNoAccess, tex);
        auto lockimg = img->Lock(CmdQue);
        auto pms = ResizeToImg<ResizeMethod::OpenCL>(lockimg.Get(), false, width, height, output, flipY);
        return agent.Await(pms);
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenGL>(const ImageView& img, const uint16_t width, const uint16_t height, const ImgDType output, const bool flipY)
{
    return Worker->AddTask([=, this](const common::asyexe::AsyncAgent& agent)
    {
            auto tex = oglTex2DStatic_::Create(img.GetWidth(), img.GetHeight(), TexFormatUtil::FromImageDType(img.GetDataType(), true));
        tex->SetData(img.AsRawImage(), true);
        agent.Await(oglUtil::SyncGL());
        auto pms = ResizeToImg<ResizeMethod::OpenGL>(tex, width, height, output, flipY);
        return agent.Await(pms);
    });
}
template<>
TEXUTILAPI PromiseResult<oglTex2DS> TexResizer::ResizeToTex<ResizeMethod::OpenCL>(const oglTex2D& tex, const uint16_t width, const uint16_t height, const TextureFormat output, const bool flipY)
{
    auto pms = ResizeToImg<ResizeMethod::OpenCL>(tex, width, height, TexFormatUtil::ToImageDType(output, true), flipY);
    return Worker->AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        const auto img = agent.Await(pms);
        auto tex = oglTex2DStatic_::Create(width, height, output);
        tex->SetData(xziar::img::TexFormatUtil::FromImageDType(img.GetDataType(), true), img.AsSpan());
        agent.Await(oglUtil::SyncGL());
        return tex;
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::Compute>(const ImageView& img, const uint16_t width, const uint16_t height, const ImgDType output, const bool flipY)
{
    return Worker->AddTask([=, this](const common::asyexe::AsyncAgent& agent)
    {
        auto tex = oglTex2DStatic_::Create(img.GetWidth(), img.GetHeight(), TexFormatUtil::FromImageDType(img.GetDataType(), true));
        tex->SetData(img.AsRawImage(), true);
        agent.Await(oglUtil::SyncGL());
        auto pms = ResizeToImg<ResizeMethod::Compute>(tex, width, height, output, flipY);
        return agent.Await(pms);
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenCL>(const ImageView& img, const uint16_t width, const uint16_t height, const ImgDType output, const bool flipY)
{
    return Worker->AddTask([=, this](const common::asyexe::AsyncAgent& agent)
    {
        COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"sRGB texture is not supported on OpenCL");
        auto climg = oclImage2D_::Create(CLContext, MemFlag::ReadOnly | MemFlag::HostWriteOnly | MemFlag::HostCopy, img.AsRawImage(), true);
        auto pms = ResizeToImg<ResizeMethod::OpenCL>(climg, false, width, height, output, flipY);
        return agent.Await(pms);
    });
}

template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenGL>(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const TextureFormat innerFormat, const uint16_t width, const uint16_t height, const ImgDType output, const bool flipY)
{
    return Worker->AddTask([=, this](const common::asyexe::AsyncAgent& agent)
    {
        oglTex2DS tex = ConvertDataToTex(data, size, innerFormat);
        agent.Await(oglUtil::SyncGL());
        auto pms = ResizeToImg<ResizeMethod::OpenGL>(tex, width, height, output, flipY);
        return agent.Await(pms);
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::Compute>(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const TextureFormat innerFormat, const uint16_t width, const uint16_t height, const ImgDType output, const bool flipY)
{
    return Worker->AddTask([=, this](const common::asyexe::AsyncAgent& agent)
    {
        oglTex2DS tex = ConvertDataToTex(data, size, innerFormat);
        agent.Await(oglUtil::SyncGL());
        auto pms = ResizeToImg<ResizeMethod::Compute>(tex, width, height, output, flipY);
        return agent.Await(pms);
    });
}
template<>
TEXUTILAPI PromiseResult<Image> TexResizer::ResizeToImg<ResizeMethod::OpenCL>(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const TextureFormat innerFormat, const uint16_t width, const uint16_t height, const ImgDType output, const bool flipY)
{
    return Worker->AddTask([=, this](const common::asyexe::AsyncAgent& agent)
    {
        if (TexFormatUtil::IsSRGBType(innerFormat))
            COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"sRGB texture is not supported on OpenCL");
        auto img = oclImage2D_::Create(CLContext, MemFlag::ReadOnly | MemFlag::HostWriteOnly | MemFlag::HostCopy, size.first, size.second, innerFormat, data.GetRawPtr());
        auto pms = ResizeToImg<ResizeMethod::OpenCL>(img, false, width, height, output, flipY);
        return agent.Await(pms);
    });
}







}
