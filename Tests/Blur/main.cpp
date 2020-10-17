#include "OpenCLUtil/OpenCLUtil.h"
#include "OpenCLUtil/oclKernelDebug.h"
#include "OpenCLUtil/oclNLCL.h"
#include "OpenCLUtil/oclPromise.h"
#include "ImageUtil/ImageUtil.h"
#include "MiniLogger/MiniLogger.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/RawFileEx.h"
#include "SystemCommon/ConsoleEx.h"
#include "common/MemoryStream.hpp"
#include "common/Linq2.hpp"
#include <thread>
#include <mutex>
#include <array>
#include <cmath>
#include <iostream>

using namespace oclu;
using namespace common::mlog;
using namespace xziar::img;
using namespace std::string_view_literals;
using std::string;
using std::u16string;


static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"cl", { GetConsoleBackend() });
    return logger;
}

std::array<float, 4> ComputeCoeff(const float sigma)
{
    double q1, q2, q3;
    if (sigma >= 2.5)
        q1 = 0.98711 * sigma - 0.96330;
    else if (sigma >= 0.5)
        q1 = 3.97156 - 4.14554 * std::sqrt(1 - 0.26891 * sigma);
    else
        q1 = 0.1147705018520355224609375;
    q2 = q1 * q1;
    q3 = q1 * q2;
    const double b0 = 1 / (1.57825 + (2.44413*q1) + (1.4281 * q2) + (0.422205 * q3));
    std::array<float, 4> coeff;
    coeff[1] = (float)(((2.44413 * q1) + (2.85619 * q2) + (1.26661 * q3)) * b0);
    coeff[2] = (float)((                 -(1.4281 * q2) - (1.26661 * q3)) * b0);
    coeff[3] = (float)((                                 (0.422205 * q3)) * b0);
    coeff[0] = (float) (1 - coeff[1] - coeff[2] - coeff[3]);
    return coeff;
}

Image ProcessImg(const oclProgram& prog, const oclContext& ctx, const oclCmdQue& cmdque, const Image& image, float sigma) try
{
    oclKernel blurX = prog->GetKernel("blurX");
    oclKernel blurY = prog->GetKernel("blurY");

    common::PromiseResult<void> pms;

    const auto coeff = ComputeCoeff(sigma);
    auto rawBuf = oclBuffer_::Create(ctx, MemFlag::ReadWrite, image.GetSize());
    pms = rawBuf->WriteSpan(cmdque, image.AsSpan<uint32_t>());
    pms->WaitFinish();
    const auto time1 = pms->ElapseNs() / 1e6f;
    auto midBuf1 = oclBuffer_::Create(ctx, MemFlag::ReadWrite, image.GetSize() *sizeof(float));
    auto midBuf2 = oclBuffer_::Create(ctx, MemFlag::ReadWrite, image.GetSize() *sizeof(float));
    
    const auto width = image.GetWidth(), height = image.GetHeight();
    const auto w4 = width - width % 4, h4 = height - height % 4;
    auto pmsX = blurX->Call<1>(rawBuf, midBuf1, midBuf2, w4, h4, width, coeff)(cmdque, { h4 });
    auto pmsY = blurY->Call<1>(midBuf2, midBuf1, rawBuf, w4, h4, width, coeff)(pmsX, cmdque, { w4 });
    pmsY->WaitFinish();
    const auto time2 = pmsX->ElapseNs() / 1e6f;
    const auto time3 = pmsY->ElapseNs() / 1e6f;
    xziar::img::Image img2(xziar::img::ImageDataType::RGBA);
    img2.SetSize(image.GetWidth(), image.GetHeight());
    pms = rawBuf->ReadSpan(cmdque, img2.AsSpan());
    pms->WaitFinish();
    const auto time4 = pms->ElapseNs() / 1e6f;
    log().info(u"WRITE[{:.5}ms], BLURX[{:.5}ms], BLURY[{:.5}ms], READ[{:.5}ms]\n", time1, time2, time3, time4);
    
    ;
    const auto& pcX = dynamic_cast<const oclPromiseCore&>(pmsX->GetPromise());
    log().info(u"BLURX:\nQueued at [{}]\nSubmit at [{}]\nStart  at [{}]\nEnd    at [{}]\n", 
        pcX.QueryTime(oclu::oclPromiseCore::TimeType::Queued),
        pcX.QueryTime(oclu::oclPromiseCore::TimeType::Submit),
        pcX.QueryTime(oclu::oclPromiseCore::TimeType::Start),
        pcX.QueryTime(oclu::oclPromiseCore::TimeType::End)); 
    const auto& pcY = dynamic_cast<const oclPromiseCore&>(pmsY->GetPromise());
    log().info(u"BLURY:\nQueued at [{}]\nSubmit at [{}]\nStart  at [{}]\nEnd    at [{}]\n",
        pcY.QueryTime(oclu::oclPromiseCore::TimeType::Queued),
        pcY.QueryTime(oclu::oclPromiseCore::TimeType::Submit),
        pcY.QueryTime(oclu::oclPromiseCore::TimeType::Start),
        pcY.QueryTime(oclu::oclPromiseCore::TimeType::End));

    const auto debugPack = pmsX->Get().GetDebugData(true);
    auto data = debugPack->GetCachedData();

    auto& logger = log();
    const bool hasSgInfo = oclu::debug::HasSubgroupInfo(debugPack->InfoMan());
    for (auto item : data)
    {
        const auto& tinfo = item.Info();
        if (hasSgInfo)
        {
            const auto& sginfo = static_cast<const oclu::debug::oclThreadInfo&>(tinfo);
            logger.verbose(FMT_STRING(u"tid[{:7}]({},{},{}), gid[{},{},{}], lid[{},{},{}], sg[{},{}]:\n{}\n"),
                item.ThreadId(), tinfo.GlobalId[0], tinfo.GlobalId[1], tinfo.GlobalId[2],
                tinfo.GroupId[0], tinfo.GroupId[1], tinfo.GroupId[2],
                tinfo.LocalId[0], tinfo.LocalId[1], tinfo.LocalId[2],
                sginfo.SubgroupId, sginfo.SubgroupLocalId,
                item.Str());
        }
        else
        {
            logger.verbose(FMT_STRING(u"tid[{:7}]({},{},{}), gid[{},{},{}], lid[{},{},{}]:\n{}\n"),
                item.ThreadId(), tinfo.GlobalId[0], tinfo.GlobalId[1], tinfo.GlobalId[2],
                tinfo.GroupId[0], tinfo.GroupId[1], tinfo.GroupId[2],
                tinfo.LocalId[0], tinfo.LocalId[1], tinfo.LocalId[2],
                item.Str());
        }
    }
    return img2;
}
catch (const common::BaseException& be)
{
    log().error(u"Error when process image: {}\n", be.Message());
    return {};
}

template<typename T>
uint32_t SelectIdx(const T& container, std::u16string_view name)
{
    if (container.size() <= 1)
        return 0;
    log().info(u"Select {} to use:\n", name);
    uint32_t idx = UINT32_MAX;
    do
    {
        const auto ch = common::console::ConsoleEx::ReadCharImmediate(false);
        if (ch >= '0' && ch <= '9')
            idx = ch - '0';
    } while (idx >= container.size());
    return idx;
}

int main() try
{
    static const common::fs::path basepath(UTF16ER(__FILE__));
    auto kernelPath = common::fs::path(basepath).replace_filename("iirblur.nlcl");
    if (!common::fs::exists(kernelPath))
    {
        kernelPath = common::fs::path("iirblur.nlcl");
    }
    log().verbose(u"nlcl path:{}\n", kernelPath.u16string());
    const auto str = common::file::ReadAllText(kernelPath);

    const auto& plats = oclUtil::GetPlatforms();
    if (plats.size() == 0)
        return {};

    common::linq::FromIterable(plats)
        .ForEach([](const auto& plat, size_t idx) mutable
            { log().info(u"option[{}] {}\t{}\n", idx, plat->Name, plat->Ver); });
    const auto platidx = SelectIdx(plats, u"platform");
    const auto plat = plats[platidx];

    auto thedev = common::linq::FromContainer(plat->GetDevices())
        .Where([](const auto& dev) { return dev->Type == DeviceType::GPU; })
        .TryGetFirst().value_or(plat->GetDefaultDevice());
    const auto ctx = plat->CreateContext(thedev);
    //ctx->onMessage = [](const auto& str) { log().debug(u"[MSG]{}\n", str); };

    auto cmdque = oclCmdQue_::Create(ctx, thedev);
    oclProgram prog;
    try
    {
        NLCLProcessor proc;
        auto stub = proc.Parse(common::as_bytes(common::to_span(str)));
        prog = proc.CompileProgram(stub, ctx, thedev)->GetProgram();
    }
    catch (const OCLException& cle)
    {
        log().error(u"Fail to build opencl Program:{}\n{}\n", cle.Message(), cle.GetDetailMessage());
        return 0;
    }

    
    common::mlog::SyncConsoleBackend();
    const string fname = common::console::ConsoleEx::ReadLine("image path:");
    common::fs::path fpath = fname;

    /*{
        auto rf = common::file::RawFileObject::OpenThrow(fpath, common::file::OpenFlag::ReadBinary);
        common::file::RawFileInputStream stream(rf);
        stream.Skip(10);
        common::io::BufferedRandomInputStream bs(std::move(stream), 65536);
        bs.Skip(20);
    }*/
    /*auto data = common::file::ReadAll<std::byte>(fpath);
    common::io::ContainerInputStream<std::vector<std::byte>> stream(data);
    auto img1 = xziar::img::ReadImage(stream, u"JPG");*/
    /*auto rf = common::file::RawFileObject::OpenThrow(fpath, common::file::OpenFlag::ReadBinary);
    common::file::RawFileInputStream stream(rf);
    auto img1 = xziar::img::ReadImage(stream, u"JPG");*/
    auto img1 = xziar::img::ReadImage(fpath);

    auto fpath2 = fpath;
    fpath2.replace_extension(".bmp");
    xziar::img::WriteImage(img1, fpath2);

    auto img2 = ProcessImg(prog, ctx, cmdque, img1, 2.0f);



    auto fpath3 = fpath;
    fpath3.replace_extension(".blur.jpg");
    xziar::img::WriteImage(img2, fpath3);
    
    /*auto rf2 = common::file::RawFileObject::OpenThrow(fpath3, common::file::OpenFlag::CreateNewBinary);
    common::file::RawFileOutputStream stream2(rf2);
    xziar::img::WriteImage(img2, stream2, u"jpg");*/

    getchar();
}
catch (const common::BaseException& be)
{
    log().error(u"{}\n", be.Message());
}
