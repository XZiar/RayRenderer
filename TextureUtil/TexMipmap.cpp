#include "TexUtilRely.h"
#include "TexMipmap.h"
#include "TexUtilWorker.h"
#include "resource.h"


namespace oglu::texutil
{
using namespace oclu;
using namespace std::literals;

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
            clProg->Build(config);
            Downsample = clProg->GetKernel("Downsample_Src");
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
    uint32_t SrcWidth;
    uint32_t SrcHeight;
};

void TexMipmap::Test()
{
    vector<uint32_t> input(256 * 256);
    for (uint32_t i = 0; i < 256 * 256; ++i)
        input[i] = i | 0xff000000u;
    Image src(ImageDataType::RGBA); src.SetSize(256, 256); memcpy_s(src.GetRawPtr(), src.GetSize(), input.data(), input.size() * sizeof(uint32_t));
    oclBuffer inBuf(CLContext, MemFlag::ReadOnly | MemFlag::HostWriteOnly, input.size() * sizeof(uint32_t));
    inBuf->Write(CmdQue, input);
    oclBuffer midBuf(CLContext, MemFlag::ReadWrite | MemFlag::HostReadOnly, input.size() * sizeof(uint32_t));
    oclBuffer outBuf(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly, input.size() / 4 * sizeof(uint32_t));
    Info info{ 256,256 };
    Downsample->SetArg(0, inBuf);
    Downsample->SetSimpleArg(1, 256);
    Downsample->SetSimpleArg(2, 256);
    Downsample->SetArg(3, midBuf);
    Downsample->SetArg(4, outBuf);
    const auto pms = Downsample->Run<2>(CmdQue, { 64,64 }, { 8,4 }, false);
    pms->wait();
    const auto time = pms->ElapseNs();
    uint32_t output[128][128];
    outBuf->Read(CmdQue, (void*)output, sizeof(output));
    Image dst(ImageDataType::RGBA); dst.SetSize(128, 128); memcpy_s(dst.GetRawPtr(), dst.GetSize(), output, sizeof(output));
    WriteImage(src, fs::temp_directory_path() / u"src.png");
    Image ref(src); ref.Resize(128, 128, true, false);
    WriteImage(ref, fs::temp_directory_path() / u"ref.png");
    WriteImage(dst, fs::temp_directory_path() / u"dst.png");
    //getchar();
}


}