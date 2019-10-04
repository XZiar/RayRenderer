#include "TexUtilRely.h"
#include "CLTexResizer.h"
#include "OpenCLInterop/GLInterop.h"
#include "resource.h"
#include <future>
#include <thread>


namespace oglu::texutil
{
using namespace oclu;
using xziar::img::TexFormatUtil;

CLTexResizer::CLTexResizer(oglContext&& glContext, const oclContext& clContext) : Executor(u"CLTexResizer"), GLContext(glContext), CLContext(clContext)
{
    Executor.Start([this]
    {
        common::SetThreadName(u"CLTexResizer");
        texLog().success(u"TexResize thread start running.\n");
        if (!GLContext->UseContext())
        {
            texLog().error(u"CLTexResizer cannot use GL context\n");
            return;
        }
        texLog().info(u"CLTexResizer use GL context with version {}\n", oglUtil::GetVersionStr());
        GLContext->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        if (!CLContext)
        {
            texLog().error(u"CLTexResizer cannot create shared CL context from given OGL context\n");
            return;
        }
        CmdQue = oclCmdQue_::Create(CLContext, CLContext->GetGPUDevice());
        if (!CmdQue)
            COMMON_THROW(BaseException, u"clQueue initialized failed!");

        auto clProg = oclProgram_::Create(CLContext, getShaderFromDLL(IDR_SHADER_CLRESIZER));
        try
        {
            oclu::CLProgConfig config;
            clProg->Build(config);
        }
        catch (OCLException& cle)
        {
            u16string buildLog;
            if (cle.data.has_value()) 
                buildLog = std::any_cast<u16string>(cle.data);
            texLog().error(u"Fail to build opencl Program:{}\n{}\n", cle.message, buildLog);
            COMMON_THROW(BaseException, u"build Program error");
        }
        ResizeToImg = clProg->GetKernel("ResizeToImg");
        ResizeToDat3 = clProg->GetKernel("ResizeToDat3");
        ResizeToDat4 = clProg->GetKernel("ResizeToDat4");
        const auto wgInfo = ResizeToImg->GetWorkGroupInfo(CLContext->Devices[0]);
        texLog().info(u"kernel compiled workgroup size [{}x{}x{}], uses [{}] private mem\n", wgInfo.CompiledWorkGroupSize[0], wgInfo.CompiledWorkGroupSize[1], wgInfo.CompiledWorkGroupSize[2], wgInfo.PrivateMemorySize);
    }, [this] 
    {
        ResizeToImg.reset(); ResizeToDat3.reset(); ResizeToDat4.reset();
        CmdQue.reset();
        CLContext.reset();
        GLContext->UnloadContext();
        GLContext.release();
    });
}

CLTexResizer::~CLTexResizer()
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

static TextureFormat FixFormat(const TextureFormat dformat)
{
    switch (dformat)
    {
    case TextureFormat::RGB8:       return TextureFormat::RGBA8;
    case TextureFormat::BGR8:       return TextureFormat::BGRA8;
    default:                            return dformat;
    }
}

common::PromiseResult<Image> CLTexResizer::ResizeToDat(const oclu::oclImg2D& input, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        //const auto wantFormat = OGLTexUtil::ConvertDtypeFrom(format, true);
        //oclImage output(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly, width, height, FixFormat(wantFormat));
        auto output = oclBuffer_::Create(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly, width*height*Image::GetElementSize(format));
        ImageInfo info{ input->Width, input->Height, width, height, 1.0f / width, 1.0f / height };
        const auto& ker = HAS_FIELD(format, ImageDataType::ALPHA_MASK) ? ResizeToDat4 : ResizeToDat3;

        const size_t worksize[] = { width, height };
        auto pms1 = ker->Call<2>(input, output, 1, info)(CmdQue, worksize);
        agent.Await(common::PromiseResult<void>(pms1));
        texLog().success(u"CLTexResizer Kernel runs {}us.\n", pms1->ElapseNs() / 1000);

        Image result(format);
        result.SetSize(width, height);
        auto pms2 = output->Read(CmdQue, result.GetRawPtr(), result.GetSize(), 0, false);
        //auto pms2 = output->Read(CmdQue, result, false);
        agent.Await(common::PromiseResult<void>(pms2));
        //if (result.GetDataType() != format)
        //    result = result.ConvertTo(format);
        return result;
    });
}
common::PromiseResult<Image> CLTexResizer::ResizeToDat(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY)
{
    if (CLContext->GetVendor() == Vendors::Intel) //seems nvidia not handle cl_gl_share correctly.
    {
        return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
        {
            auto interimg = oclGLInterImg2D_::Create(CLContext, MemFlag::ReadOnly, tex);
            auto lockimg = interimg->Lock(CmdQue);
            auto img = agent.Await(ResizeToDat(lockimg.Get(), width, height, format, flipY));
            return img;
        });
    }
    else
    {
        return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
        {
            const auto img = tex->GetImage(ImageDataType::RGBA);
            auto input = oclImage2D_::Create(CLContext, MemFlag::ReadOnly | MemFlag::HostWriteOnly, img.GetWidth(), img.GetHeight(), TextureFormat::RGBA8);
            auto pms1 = input->Write(CmdQue, img, false);
            agent.Await(common::PromiseResult<void>(pms1));
            return agent.Await(ResizeToDat(input, width, height, format, flipY));
        });
    }
}
common::PromiseResult<Image> CLTexResizer::ResizeToDat(const common::AlignedBuffer& data, const std::pair<uint32_t, uint32_t>& size, const TextureFormat dataFormat, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY)
{
    return Executor.AddTask([=, &data](const common::asyexe::AsyncAgent& agent)
    {
        if (TexFormatUtil::IsCompressType(dataFormat))
            COMMON_THROW(OCLException, OCLException::CLComponent::OCLU, u"OpenCL doesnot support compressed texture yet.");
        else
        {
            auto input = oclImage2D_::Create(CLContext, MemFlag::ReadOnly | MemFlag::HostWriteOnly, size.first, size.second, dataFormat);
            auto pms1 = input->Write(CmdQue, data, false);
            agent.Await(common::PromiseResult<void>(pms1));
            return agent.Await(ResizeToDat(input, width, height, format, flipY));
        }
    });
}

}
