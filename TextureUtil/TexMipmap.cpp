#include "TexUtilPch.h"
#include "TexMipmap.h"
#include "TexUtilWorker.h"
#include "resource.h"


namespace oglu::texutil
{
using std::string;
using std::u16string;
using std::vector;
using common::PromiseResult;
using namespace xziar::img;
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
        try
        {
            oclu::CLProgConfig config;
            config.Defines["CountX"] = GroupX;
            config.Defines["CountY"] = GroupY;
            auto clProg = oclProgram_::CreateAndBuild(CLContext, getShaderFromDLL(IDR_SHADER_MIPMAP), config);
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
    })->Wait();
}

TexMipmap::~TexMipmap()
{
}

struct Info
{
    uint32_t SrcOffset = 0;
    uint32_t DstOffset = 0;
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
    const auto srcPath = common::fs::temp_directory_path() / u"src.png";
    if (common::fs::exists(srcPath))
        src = ReadImage(srcPath);
    else
    {
        src.SetSize(256, 256);
        for (uint32_t i = 0; i < 256 * 256; ++i)
            src.GetRawPtr<uint32_t>()[i] = i | 0xff000000u;
    }
    auto inBuf = oclBuffer_::Create(CLContext, MemFlag::ReadOnly | MemFlag::HostWriteOnly, src.GetSize());
    inBuf->Write(CmdQue, src.GetRawPtr(), src.GetSize());
    auto midBuf = oclBuffer_::Create(CLContext, MemFlag::ReadWrite | MemFlag::HostNoAccess, src.GetSize() / 2);
    auto mid2Buf = oclBuffer_::Create(CLContext, MemFlag::ReadWrite | MemFlag::HostNoAccess, src.GetSize() / 8);
    auto outBuf = oclBuffer_::Create(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly, src.GetSize() / 4);
    auto infoBuf = oclBuffer_::Create(CLContext, MemFlag::ReadOnly | MemFlag::HostWriteOnly, sizeof(Info) * 2);
    Info info[]{ {src.GetWidth(),src.GetHeight()}, {src.GetWidth() / 2, src.GetHeight() / 2} };
    infoBuf->Write(CmdQue, &info, sizeof(info));
    const auto pms = DownsampleSrc->Call<2>(inBuf, infoBuf, (uint8_t)0, midBuf, outBuf)(CmdQue, { src.GetWidth() / 4,src.GetHeight() / 4 }, { GroupX,GroupY });
    pms->Wait();
    const auto time = pms->ElapseNs();
    Image dst(ImageDataType::RGBA); dst.SetSize(src.GetWidth() / 2, src.GetHeight() / 2);
    outBuf->Read(CmdQue, dst.GetRawPtr(), dst.GetSize());
    WriteImage(dst, common::fs::temp_directory_path() / u"dst.png");



    /*
    const auto pms2 = DownsampleTest->Call<2>(inBuf, infoBuf, (uint8_t)0, midBuf, outBuf)(CmdQue, { src.GetWidth() / 4,src.GetHeight() / 4 }, { GroupX,GroupY });
    pms2->Wait();
    const auto time2 = pms2->ElapseNs();
    Image dst2(ImageDataType::RGBA); dst2.SetSize(src.GetWidth() / 2, src.GetHeight() / 2);
    outBuf->Read(CmdQue, dst2.GetRawPtr(), dst2.GetSize());
    WriteImage(dst2, fs::temp_directory_path() / u"dst2.png");
    */


    const auto pms3 = DownsampleRaw->Call<2>(inBuf, infoBuf, (uint8_t)0, outBuf)(pms, CmdQue, { src.GetWidth() / 4,src.GetHeight() / 4 }, { GroupX,GroupY });
    pms3->Wait();
    const auto time3 = pms3->ElapseNs();
    Image dst3(ImageDataType::RGBA); dst3.SetSize(src.GetWidth() / 2, src.GetHeight() / 2);
    outBuf->Read(CmdQue, dst3.GetRawPtr(), dst3.GetSize());
    WriteImage(dst3, common::fs::temp_directory_path() / u"dst_.png");
    //getchar();
}
void TexMipmap::Test2()
{
    //Test();
    Image src = xziar::img::ReadImage(common::fs::temp_directory_path() / u"src.png");
    const auto pms = GenerateMipmaps(src, false);
    const auto mipmaps = pms->Wait();
    uint32_t i = 1;
    xziar::img::WriteImage(src, common::fs::temp_directory_path() / ("mip_0.jpg"));
    for (const auto& mm : mipmaps)
    {
        xziar::img::WriteImage(mm, common::fs::temp_directory_path() / ("mip_" + std::to_string(i) + ".jpg"));
        i++;
    }
    texLog().debug(u"Mipmap test2 totally cost {} us.\n", pms->ElapseNs() / 1000);
}

static vector<Info> GenerateInfo(uint32_t width, uint32_t height, const uint8_t levels)
{
    vector<Info> infos;
    uint32_t offsetSrc = 0, offsetDst = 0;
    while (width >= 64 && height >= 64 && width % 64 == 0 && height % 64 == 0 && levels > infos.size())
    {
        infos.emplace_back(width, height);
        infos.back().SrcOffset = offsetSrc;
        infos.back().DstOffset = offsetDst;
        width /= 2; height /= 2;
        offsetSrc = offsetDst;
        offsetDst += width * height;
    }
    return infos;
}

PromiseResult<vector<Image>> TexMipmap::GenerateMipmaps(const ImageView& src, const bool isSRGB, const uint8_t levels)
{
    auto infos = GenerateInfo(src.GetWidth(), src.GetHeight(), levels);
    if (isSRGB)
    {
        return Worker->AddTask([this, src, infos = std::move(infos)](const common::asyexe::AsyncAgent& agent)
        {
            const auto bytes = common::linq::FromIterable(infos).Reduce([](uint32_t& sum, const Info& info) { sum += info.SrcWidth * info.SrcHeight; }, 0u);
            common::AlignedBuffer mainBuf(bytes, 4096);
            vector<Image> images;
            auto infoBuf = oclBuffer_::Create(CLContext, MemFlag::ReadOnly | MemFlag::HostNoAccess | MemFlag::HostCopy, sizeof(Info) * infos.size(), infos.data());
            auto inBuf = oclBuffer_::Create(CLContext, MemFlag::ReadWrite | MemFlag::HostNoAccess | MemFlag::HostCopy, src.GetSize(), src.GetRawPtr());
            auto midBuf = oclBuffer_::Create(CLContext, MemFlag::ReadWrite | MemFlag::HostNoAccess, src.GetSize() / 2);
            auto outBuf = oclBuffer_::Create(CLContext, MemFlag::WriteOnly | MemFlag::HostReadOnly | MemFlag::UseHost, mainBuf.GetSize(), mainBuf.GetRawPtr());

            size_t offset = 0;
            vector<PromiseResult<void>> pmss;
            for (uint8_t idx = 0; idx < infos.size(); ++idx)
            {
                const auto& info = infos[idx];
                images.emplace_back(mainBuf.CreateSubBuffer(offset, info.SrcWidth * info.SrcHeight), info.SrcWidth / 2, info.SrcHeight / 2, ImageDataType::RGBA);
                offset += info.SrcWidth * info.SrcHeight;
                PromiseResult<void> pms;
                if (idx == 0)
                {
                    pms = DownsampleSrc->Call<2>(inBuf, infoBuf, idx, midBuf, outBuf)(CmdQue, { (size_t)info.SrcWidth / 4, (size_t)info.SrcHeight / 4 }, { GroupX,GroupY });
                }
                else
                {
                    pms = DownsampleMid->Call<2>((idx & 1) == 0 ? inBuf : midBuf, infoBuf, idx, (idx & 1) == 0 ? midBuf : inBuf, outBuf)
                        (pmss.back(), CmdQue, { (size_t)info.SrcWidth / 4, (size_t)info.SrcHeight / 4 }, { GroupX,GroupY });
                }
                pmss.push_back(pms);
            }
            agent.Await(pmss.back());
            const uint64_t totalTime = common::linq::FromIterable(pmss).Select([](const auto& pms) { return pms->ElapseNs(); }).Sum((uint64_t)0);
            outBuf->Map(CmdQue, oclu::MapFlag::Read);
            texLog().debug(u"Mipmap from [{}x{}] generate [{}] level within {}us.\n", src.GetWidth(), src.GetHeight(), images.size(), totalTime / 1000);
            return images;
        });
    }
    else
    {
        return Worker->AddTask([this, src, infos = std::move(infos)](const common::asyexe::AsyncAgent& agent)
        {
            const auto bytes = common::linq::FromIterable(infos).Reduce([](uint32_t& sum, const Info& info) { sum += info.SrcWidth * info.SrcHeight; }, 0u);
            common::AlignedBuffer mainBuf(bytes, 4096);
            vector<Image> images;
            auto infoBuf = oclBuffer_::Create(CLContext, MemFlag::ReadOnly | MemFlag::HostNoAccess | MemFlag::HostCopy, sizeof(Info) * infos.size(), infos.data());
            auto inBuf = oclBuffer_::Create(CLContext, MemFlag::ReadOnly | MemFlag::HostNoAccess | MemFlag::HostCopy, src.GetSize(), src.GetRawPtr());
            auto outBuf = oclBuffer_::Create(CLContext, MemFlag::ReadWrite | MemFlag::HostReadOnly | MemFlag::UseHost, mainBuf.GetSize(), mainBuf.GetRawPtr());

            size_t offset = 0;
            vector<PromiseResult<void>> pmss;
            for (uint8_t idx = 0; idx < infos.size(); ++idx)
            {
                const auto& info = infos[idx];
                images.emplace_back(mainBuf.CreateSubBuffer(offset, info.SrcWidth * info.SrcHeight), info.SrcWidth / 2, info.SrcHeight / 2, ImageDataType::RGBA);
                offset += info.SrcWidth * info.SrcHeight;
                PromiseResult<void> prev;
                if (!pmss.empty()) 
                    prev = pmss.back();
                const auto pms = DownsampleRaw->Call<2>(idx == 0 ? inBuf : outBuf, infoBuf, idx, outBuf)
                    (prev, CmdQue, { (size_t)infos[idx].SrcWidth / 4, (size_t)infos[idx].SrcHeight / 4 }, { GroupX,GroupY });
                pmss.push_back(pms);
            }
            agent.Await(pmss.back());
            const uint64_t totalTime = common::linq::FromIterable(pmss).Select([](const auto& pms) { return pms->ElapseNs(); }).Sum((uint64_t)0);
            outBuf->Map(CmdQue, oclu::MapFlag::Read);
            texLog().debug(u"Mipmap from [{}x{}] generate [{}] level within {}us.\n", src.GetWidth(), src.GetHeight(), images.size(), totalTime / 1000);
            return images;
        });
    }
}


}