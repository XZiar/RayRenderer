#include "TexUtilRely.h"
#include "TexMipmap.h"
#include "TexUtilWorker.h"
#include "resource.h"


namespace oglu::texutil
{
using namespace oclu;
using namespace std::literals;

constexpr uint32_t GroupX = 16, GroupY = 4;

TexMipmap::TexMipmap(const std::shared_ptr<TexUtilWorker>& worker) : Worker(worker)
{
    Worker->AddTask([this](const auto&)
    {
        GLContext = Worker->GLContext;
        CLContext = Worker->CLContext;
        CmdQue = Worker->CmdQue;
        oclProgram clProg(CLContext, getShaderFromDLL(IDR_SHADER_MIPMAP));
        try
        {
            oclu::CLProgConfig config;
            config.Defines["CountX"] = GroupX;
            config.Defines["CountY"] = GroupY;
            clProg->Build(config);
            Downsample = clProg->GetKernel("Downsample_SrcH");
            if (!Downsample)
                Downsample = clProg->GetKernel("Downsample_Src");
            Downsample2 = clProg->GetKernel("Downsample_MidH");
            if (!Downsample2)
                Downsample2 = clProg->GetKernel("Downsample_Mid");
            const auto wgInfo = Downsample->GetWorkGroupInfo(CLContext->Devices[0]);
            texLog().info(u"kernel compiled workgroup size [{}x{}x{}], uses [{}] pmem and [{}] smem\n",
                wgInfo.CompiledWorkGroupSize[0], wgInfo.CompiledWorkGroupSize[1], wgInfo.CompiledWorkGroupSize[2], wgInfo.PrivateMemorySize, wgInfo.LocalMemorySize);
        }
        catch (OCLException& cle)
        {
            u16string buildLog;
            if (cle.data.has_value())
                buildLog = std::any_cast<u16string>(cle.data);
            texLog().error(u"Fail to build opencl Program:{}\n{}\n", cle.message, buildLog);
        }
    })->wait();
}

TexMipmap::~TexMipmap()
{
}

struct Info
{
    uint16_t SrcWidth;
    uint16_t SrcHeight;
    uint16_t LimitX;
    uint16_t LimitY;
    Info(const uint32_t width, const uint32_t height)
        : SrcWidth(static_cast<uint16_t>(width)), SrcHeight(static_cast<uint16_t>(height)),
        LimitX(SrcWidth / 4 - 1), LimitY(SrcHeight / 4 - 1) {}
};

void TexMipmap::Test()
{
    Image src(ImageDataType::RGBA);
    const auto srcPath = fs::temp_directory_path() / u"src.png";
    if (fs::exists(srcPath))
        src = ReadImage(srcPath);
    else
    {
        src.SetSize(256, 256);
        for (uint32_t i = 0; i < 256 * 256; ++i)
            src.GetRawPtr<uint32_t>()[i] = i | 0xff000000u;
    }
    oclBuffer inBuf(CLContext, MemFlag::ReadOnly | MemFlag::HostWriteOnly, src.GetSize());
    inBuf->Write(CmdQue, src.GetRawPtr(), src.GetSize());
    oclBuffer midBuf(CLContext, MemFlag::ReadWrite | MemFlag::HostReadOnly, src.GetSize());
    oclBuffer mid2Buf(CLContext, MemFlag::ReadWrite | MemFlag::HostReadOnly, src.GetSize() / 4);
    oclBuffer outBuf(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly, src.GetSize() / 4);
    oclBuffer infoBuf(CLContext, MemFlag::ReadOnly | MemFlag::HostWriteOnly, sizeof(Info) * 2);
    Info info[]{ {src.GetWidth(),src.GetHeight()}, {src.GetWidth()/2, src.GetHeight()/2} };
    infoBuf->Write(CmdQue, &info, sizeof(info));
    Downsample->SetArg(0, inBuf);
    Downsample->SetArg(1, infoBuf);
    Downsample->SetSimpleArg<uint8_t>(2, 0);
    Downsample->SetArg(3, midBuf);
    Downsample->SetArg(4, outBuf);
    const auto pms = Downsample->Run<2>(CmdQue, { src.GetWidth() / 4,src.GetHeight() / 4 }, { GroupX,GroupY }, false);
    pms->wait();
    const auto time = pms->ElapseNs();
    Image dst(ImageDataType::RGBA); dst.SetSize(src.GetWidth() / 2, src.GetHeight() / 2);
    outBuf->Read(CmdQue, dst.GetRawPtr(), dst.GetSize());
    Image ref(src); ref.Resize(ref.GetWidth() / 2, ref.GetHeight() / 2, true, false);
    WriteImage(ref, fs::temp_directory_path() / u"ref.png");
    WriteImage(dst, fs::temp_directory_path() / u"dst.png");
    
    Downsample2->SetArg(0, midBuf);
    Downsample2->SetArg(1, infoBuf);
    Downsample2->SetSimpleArg<uint8_t>(2, 1);
    Downsample2->SetArg(3, mid2Buf);
    Downsample2->SetArg(4, outBuf);
    const auto pms2 = Downsample2->Run<2>(CmdQue, { src.GetWidth() / 8,src.GetHeight() / 8 }, { GroupX,GroupY }, false);
    pms2->wait();
    const auto time2 = pms2->ElapseNs();
    Image dst2(ImageDataType::RGBA); dst2.SetSize(src.GetWidth() / 4, src.GetHeight() / 4);
    outBuf->Read(CmdQue, dst2.GetRawPtr(), dst2.GetSize());
    Image ref2(ref); ref2.Resize(ref2.GetWidth() / 2, ref2.GetHeight() / 2, true, false);
    WriteImage(ref2, fs::temp_directory_path() / u"ref2.png");
    WriteImage(dst2, fs::temp_directory_path() / u"dst2.png");
    //getchar();
}


}