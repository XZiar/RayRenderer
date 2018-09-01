#include "TexUtilRely.h"
#include "CLTexResizer.h"
#include "resource.h"
#include <future>
#include <thread>


namespace oglu::texutil
{
using namespace oclu;

CLTexResizer::CLTexResizer(const oglContext& glContext) : Executor(u"CLTexResizer"), GLContext(glContext)
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
        CLContext = oclUtil::CreateGLSharedContext(GLContext);
        if (!CLContext)
        {
            texLog().error(u"CLTexResizer cannot create shared CL context from given OGL context\n");
            return;
        }
        CLContext->onMessage = [](const u16string& errtxt)
        {
            texLog().error(u"Error from context:\t{}\n", errtxt);
        };
        ComQue.reset(CLContext, CLContext->Devices[0]);
        if (!ComQue)
            COMMON_THROW(BaseException, u"clQueue initialized failed!");

        oclProgram clProg(CLContext, getShaderFromDLL(IDR_SHADER_CLRESIZER));
        try
        {
            const string options = CLContext->vendor == Vendor::NVIDIA ? "-cl-kernel-arg-info -cl-fast-relaxed-math -cl-nv-verbose -DNVIDIA" : "-cl-fast-relaxed-math";
            clProg->Build(options);
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
        ResizeToImg.release();
        ComQue.release();
        CLContext.release();
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

static TextureDataFormat FixFormat(const TextureDataFormat dformat)
{
    switch (dformat)
    {
    case TextureDataFormat::RGB8:      return TextureDataFormat::RGBA8;
    case TextureDataFormat::BGR8:      return TextureDataFormat::BGRA8;
    default:                            return dformat;
    }
}

common::PromiseResult<Image> CLTexResizer::ResizeToDat(const oclu::oclImage& input, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        const auto wantFormat = TexFormatUtil::ConvertFormat(format, true);
        //oclImage output(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly, width, height, FixFormat(wantFormat));
        oclBuffer output(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly, width*height*Image::GetElementSize(format));
        ImageInfo info{ input->Width, input->Height, width, height, 1.0f / width, 1.0f / height };
        const auto& ker = HAS_FIELD(format, ImageDataType::ALPHA_MASK) ? ResizeToDat4 : ResizeToDat3;
        ker->SetArg(0, input);
        ker->SetArg(1, output);
        ker->SetSimpleArg(2, 1);
        ker->SetSimpleArg(3, info);

        const size_t worksize[] = { width, height };
        auto pms1 = ker->Run<2>(ComQue, worksize, false);
        agent.Await(common::PromiseResult<void>(pms1));
        texLog().success(u"CLTexResizer Kernel runs {}us.\n", pms1->ElapseNs() / 1000);

        Image result(format);
        result.SetSize(width, height);
        auto pms2 = output->Read(ComQue, result.GetRawPtr(), result.GetSize(), 0, false);
        //auto pms2 = output->Read(ComQue, result, false);
        agent.Await(common::PromiseResult<void>(pms2));
        //if (result.GetDataType() != format)
        //    result = result.ConvertTo(format);
        return result;
    });
}
common::PromiseResult<Image> CLTexResizer::ResizeToDat(const oglTex2D& tex, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY)
{
    return Executor.AddTask([=](const common::asyexe::AsyncAgent& agent)
    {
        auto glimg = oclGLImage(CLContext, MemFlag::ReadOnly, tex);
        glimg->Lock(ComQue);
        auto img = agent.Await(ResizeToDat(glimg, width, height, format, flipY));
        glimg->Unlock(ComQue);
        return img;
    });
}
common::PromiseResult<Image> CLTexResizer::ResizeToDat(const common::AlignedBuffer<32>& data, const std::pair<uint32_t, uint32_t>& size, const TextureInnerFormat dataFormat, const uint16_t width, const uint16_t height, const ImageDataType format, const bool flipY)
{
    return Executor.AddTask([=, &data](const common::asyexe::AsyncAgent& agent)
    {
        if (TexFormatUtil::IsCompressType(dataFormat))
        {
            oglTex2DS tex(size.first, size.second, dataFormat);
            tex->SetCompressedData(data.GetRawPtr(), data.GetSize());
            auto glimg = oclGLImage(CLContext, MemFlag::ReadOnly, tex);
            glimg->Lock(ComQue);
            auto img = agent.Await(ResizeToDat(glimg, width, height, format, flipY));
            glimg->Unlock(ComQue);
            return img;
        }
        else
        {
            oclImage input (CLContext, MemFlag::ReadOnly | MemFlag::HostWriteOnly, size.first, size.second, TexFormatUtil::DecideFormat(dataFormat));
            auto pms1 = input->Write(ComQue, data, false);
            agent.Await(common::PromiseResult<void>(pms1));
            return agent.Await(ResizeToDat(input, width, height, format, flipY));
        }
    });
}

}
