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
            DownsampleSrc = clProg->GetKernel("Downsample_SrcH");
            if (!DownsampleSrc)
                DownsampleSrc = clProg->GetKernel("Downsample_Src");
            DownsampleMid = clProg->GetKernel("Downsample_MidH");
            if (!DownsampleMid)
                DownsampleMid = clProg->GetKernel("Downsample_Mid");
            DownsampleRaw = clProg->GetKernel("Downsample_RawH");
            if (!DownsampleRaw)
                DownsampleRaw = clProg->GetKernel("Downsample_Raw");
            DownsampleTest = clProg->GetKernel("Downsample_Src2");
            /*const auto wgInfo = DownsampleSrc->GetWorkGroupInfo(CLContext->Devices[0]);
            texLog().info(u"kernel compiled workgroup size [{}x{}x{}], uses [{}] pmem and [{}] smem\n",
                wgInfo.CompiledWorkGroupSize[0], wgInfo.CompiledWorkGroupSize[1], wgInfo.CompiledWorkGroupSize[2], wgInfo.PrivateMemorySize, wgInfo.LocalMemorySize);*/
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
    oclBuffer midBuf(CLContext, MemFlag::ReadWrite | MemFlag::HostNoAccess, src.GetSize() / 2);
    oclBuffer mid2Buf(CLContext, MemFlag::ReadWrite | MemFlag::HostNoAccess, src.GetSize() / 8);
    oclBuffer outBuf(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly, src.GetSize() / 4);
    oclBuffer infoBuf(CLContext, MemFlag::ReadOnly | MemFlag::HostWriteOnly, sizeof(Info) * 2);
    Info info[]{ {src.GetWidth(),src.GetHeight()}, {src.GetWidth()/2, src.GetHeight()/2} };
    infoBuf->Write(CmdQue, &info, sizeof(info));
    DownsampleSrc->SetArg(0, inBuf);
    DownsampleSrc->SetArg(1, infoBuf);
    DownsampleSrc->SetSimpleArg<uint8_t>(2, 0);
    DownsampleSrc->SetArg(3, midBuf);
    DownsampleSrc->SetArg(4, outBuf);
    const auto pms = DownsampleSrc->Run<2>(CmdQue, { src.GetWidth() / 4,src.GetHeight() / 4 }, { GroupX,GroupY }, false);
    pms->wait();
    const auto time = pms->ElapseNs();
    Image dst(ImageDataType::RGBA); dst.SetSize(src.GetWidth() / 2, src.GetHeight() / 2);
    outBuf->Read(CmdQue, dst.GetRawPtr(), dst.GetSize());
    WriteImage(dst, fs::temp_directory_path() / u"dst.png");

    /*DownsampleTest->SetArg(0, inBuf);
    DownsampleTest->SetArg(1, infoBuf);
    DownsampleTest->SetSimpleArg<uint8_t>(2, 0);
    DownsampleTest->SetArg(3, midBuf);
    DownsampleTest->SetArg(4, outBuf);
    const auto pms2 = DownsampleTest->Run<2>(CmdQue, { src.GetWidth() / 4,src.GetHeight() / 4 }, { GroupX,GroupY }, false);
    pms2->wait();
    const auto time2 = pms2->ElapseNs();
    Image dst2(ImageDataType::RGBA); dst2.SetSize(src.GetWidth() / 2, src.GetHeight() / 2);
    outBuf->Read(CmdQue, dst2.GetRawPtr(), dst2.GetSize());
    WriteImage(dst2, fs::temp_directory_path() / u"dst2.png");*/
    

    DownsampleRaw->SetArg(0, inBuf);
    DownsampleRaw->SetArg(1, infoBuf);
    DownsampleRaw->SetSimpleArg<uint8_t>(2, 0);
    DownsampleRaw->SetArg(3, outBuf);
    const auto pms3 = DownsampleRaw->Run<2>(CmdQue, { src.GetWidth() / 4,src.GetHeight() / 4 }, { GroupX,GroupY }, false);
    pms3->wait();
    const auto time3 = pms3->ElapseNs();
    Image dst3(ImageDataType::RGBA); dst3.SetSize(src.GetWidth() / 2, src.GetHeight() / 2);
    outBuf->Read(CmdQue, dst3.GetRawPtr(), dst3.GetSize());
    WriteImage(dst3, fs::temp_directory_path() / u"dst_.png");
    //getchar();
}
void TexMipmap::Test2()
{
    Test();
    Image src = xziar::img::ReadImage(fs::temp_directory_path() / u"src.png");
    const auto pms = GenerateMipmaps(src, false);
    const auto mipmaps = pms->wait();
    uint32_t i = 1;
    xziar::img::WriteImage(src, fs::temp_directory_path() / ("mip_0.jpg"));
    for (const auto& mm : mipmaps)
    {
        xziar::img::WriteImage(mm, fs::temp_directory_path() / ("mip_" + std::to_string(i) + ".jpg"));
        i++;
    }
    texLog().debug(u"Mipmap test2 totally cost {} us.\n", pms->ElapseNs() / 1000);
}

static vector<Info> GenerateInfo(uint32_t width, uint32_t height, const uint8_t levels)
{
    vector<Info> infos;
    while (width >= 64 && height >= 64 && width % 64 == 0 && height % 64 == 0 && levels > infos.size())
    {
        infos.emplace_back(width, height);
        width /= 2; height /= 2;
    }
    return infos;
}

PromiseResult<vector<Image>> TexMipmap::GenerateMipmaps(const Image& raw, const bool isSRGB, const uint8_t levels)
{
    auto infos = GenerateInfo(raw.GetWidth(), raw.GetHeight(), levels);
    if (isSRGB)
    {
        return Worker->AddTask([this, src = ImageView(raw), infos = std::move(infos)] (const common::asyexe::AsyncAgent& agent)
        {
            uint64_t totalTime = 0;
            const auto bytes = Linq::FromIterable(infos).Reduce([](uint32_t& sum, const Info& info) { sum += info.SrcWidth * info.SrcHeight; }, 0u);
            common::AlignedBuffer mainBuf(bytes, 4096);
            vector<Image> images;
            oclBuffer infoBuf(CLContext, MemFlag::ReadOnly | MemFlag::HostNoAccess | MemFlag::HostCopy, sizeof(Info) * infos.size(), infos.data());
            oclBuffer inBuf(CLContext, MemFlag::ReadWrite | MemFlag::HostNoAccess | MemFlag::HostCopy, src.GetSize(), src.GetRawPtr());
            oclBuffer midBuf(CLContext, MemFlag::ReadWrite | MemFlag::HostNoAccess, src.GetSize() / 2);

            size_t offset = 0;
            for (uint8_t idx = 0; idx < infos.size(); ++idx)
            {
                const auto& info = infos[idx];
                images.emplace_back(mainBuf.CreateSubBuffer(offset, info.SrcWidth * info.SrcHeight), info.SrcWidth / 2, info.SrcHeight / 2, ImageDataType::RGBA);
                offset += info.SrcWidth * info.SrcHeight;
                oclBuffer outBuf(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly | MemFlag::UseHost, images.back().GetSize(), images.back().GetRawPtr());
                PromiseResult<void> pms;
                if (idx == 0)
                {
                    DownsampleSrc->SetArg(0, inBuf);
                    DownsampleSrc->SetArg(1, infoBuf);
                    DownsampleSrc->SetSimpleArg(2, idx);
                    DownsampleSrc->SetArg(3, midBuf);
                    DownsampleSrc->SetArg(4, outBuf);
                    pms = DownsampleSrc->Run<2>(CmdQue, { (size_t)info.SrcWidth / 4, (size_t)info.SrcHeight / 4 }, { GroupX,GroupY }, false);
                }
                else
                {
                    DownsampleMid->SetArg(0, (idx & 1) == 0 ? inBuf : midBuf);
                    DownsampleMid->SetArg(1, infoBuf);
                    DownsampleMid->SetSimpleArg(2, idx);
                    DownsampleMid->SetArg(3, (idx & 1) == 0 ? midBuf : inBuf);
                    DownsampleMid->SetArg(4, outBuf);
                    pms = DownsampleMid->Run<2>(CmdQue, { (size_t)info.SrcWidth / 4, (size_t)info.SrcHeight / 4 }, { GroupX,GroupY }, false);
                }
                agent.Await(pms);
                outBuf->Map(CmdQue, MapFlag::Read);
                totalTime += pms->ElapseNs();
            }
            texLog().debug(u"Mipmap from [{}x{}] generate [{}] level within {}us.\n", src.GetWidth(), src.GetHeight(), images.size(), totalTime / 1000);
            return images;
        });
    }
    else
    {
        return Worker->AddTask([this, src = ImageView(raw), infos = std::move(infos)](const common::asyexe::AsyncAgent& agent)
        {
            uint64_t totalTime = 0;
            const auto bytes = Linq::FromIterable(infos).Reduce([](uint32_t& sum, const Info& info) { sum += info.SrcWidth * info.SrcHeight; }, 0u);
            common::AlignedBuffer mainBuf(bytes, 4096);
            vector<Image> images;
            oclBuffer infoBuf(CLContext, MemFlag::ReadOnly | MemFlag::HostNoAccess | MemFlag::HostCopy, sizeof(Info) * infos.size(), infos.data());
            oclBuffer inBuf;
            oclBuffer outBuf;

            size_t offset = 0;
            for (uint8_t idx = 0; idx < infos.size(); ++idx)
            {
                const auto& info = infos[idx];
                images.emplace_back(mainBuf.CreateSubBuffer(offset, info.SrcWidth * info.SrcHeight), info.SrcWidth / 2, info.SrcHeight / 2, ImageDataType::RGBA);
                offset += info.SrcWidth * info.SrcHeight;
                if (idx == 0)
                    inBuf.reset(CLContext, MemFlag::ReadOnly | MemFlag::HostNoAccess | MemFlag::HostCopy, src.GetSize(), src.GetRawPtr());
                else
                    inBuf = outBuf;
                outBuf.reset(CLContext, MemFlag::ReadWrite | MemFlag::HostReadOnly | MemFlag::UseHost, images.back().GetSize(), images.back().GetRawPtr());
                DownsampleRaw->SetArg(0, inBuf);
                DownsampleRaw->SetArg(1, infoBuf);
                DownsampleRaw->SetSimpleArg(2, idx);
                DownsampleRaw->SetArg(3, outBuf);
                const auto pms = DownsampleRaw->Run<2>(CmdQue, { (size_t)infos[idx].SrcWidth / 4, (size_t)infos[idx].SrcHeight / 4 }, { GroupX,GroupY }, false);
                agent.Await(pms);
                outBuf->Map(CmdQue, MapFlag::Read);
                totalTime += pms->ElapseNs();
            }
            texLog().debug(u"Mipmap from [{}x{}] generate [{}] level within {}us.\n", src.GetWidth(), src.GetHeight(), images.size(), totalTime / 1000);
            return images;
        });
    }
}


}