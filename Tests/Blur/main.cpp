#include "OpenCLUtil/OpenCLUtil.h"
#include "ImageUtil/ImageUtil.h"
#include "MiniLogger/MiniLogger.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/RawFileEx.h"
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


Image ProcessImg(const string kernel, const Image& image, float sigma) try
{
    const auto& plats = oclUtil::GetPlatforms();
    if (plats.size() == 0)
        return {};

    common::linq::FromIterable(plats)
        .ForEach([i = 0u](const auto& plat) mutable 
        { log().info(u"option[{}] {}\t{}\n", i++, plat->Name, plat->Ver); });
    const auto plat = plats[1];

    auto thedev = common::linq::FromContainer(plat->GetDevices())
        .Where([](const auto& dev) { return dev->Type == DeviceType::GPU; })
        .TryGetFirst().value_or(plat->GetDefaultDevice());
    const auto ctx = plat->CreateContext(thedev);
    //ctx->onMessage = [](const auto& str) { log().debug(u"[MSG]{}\n", str); };

    auto cmdque = oclCmdQue_::Create(ctx, thedev);
    oclProgram prog;
    try
    {
        prog = oclProgram_::CreateAndBuild(ctx, kernel, {}, { thedev });
    }
    catch (OCLException& cle)
    {
        u16string buildLog;
        if (cle.data.has_value())
            buildLog = std::any_cast<u16string>(cle.data);
        log().error(u"Fail to build opencl Program:{}\n{}\n", cle.message, buildLog);
        return {};
    }
    oclKernel blurX = prog->GetKernel("blurX");
    oclKernel blurY = prog->GetKernel("blurY");

    common::PromiseResult<void> pms;

    const auto coeff = ComputeCoeff(sigma);
    auto rawBuf = oclBuffer_::Create(ctx, MemFlag::ReadWrite, image.GetSize());
    pms = rawBuf->Write(cmdque, image.GetRawPtr(), image.GetSize(), 0, false);
    pms->Wait();
    const auto time1 = pms->ElapseNs() / 1e6f;
    auto midBuf1 = oclBuffer_::Create(ctx, MemFlag::ReadWrite, image.GetSize() *sizeof(float));
    auto midBuf2 = oclBuffer_::Create(ctx, MemFlag::ReadWrite, image.GetSize() *sizeof(float));
    
    const auto width = image.GetWidth(), height = image.GetHeight();
    const auto w4 = width - width % 4, h4 = height - height % 4;
    auto pmsX = blurX->Call<1>(rawBuf, midBuf1, midBuf2, w4, h4, width, coeff)(cmdque, { h4 });
    auto pmsY = blurY->Call<1>(midBuf2, midBuf1, rawBuf, w4, h4, width, coeff)(pmsX, cmdque, { w4 });
    pmsY->Wait();
    const auto time2 = pmsX->ElapseNs() / 1e6f;
    const auto time3 = pmsY->ElapseNs() / 1e6f;
    xziar::img::Image img2(xziar::img::ImageDataType::RGBA);
    img2.SetSize(image.GetWidth(), image.GetHeight());
    rawBuf->Read(cmdque, img2.GetRawPtr(), width*height*4, false);
    pms->Wait();
    const auto time4 = pms->ElapseNs() / 1e6f;
    log().info(u"WRITE[{:.5}ms], BLURX[{:.5}ms], BLURY[{:.5}ms], READ[{:.5}ms]\n", time1, time2, time3, time4);
    return img2;
}
catch (common::BaseException& be)
{
    log().error(u"Error when process image: {}\n", be.message);
    return {};
}


int main()
{
    static const common::fs::path basepath(UTF16ER(__FILE__));
    const auto kernelPath = common::fs::path(basepath).replace_filename("iirblur.cl");
    log().verbose(u"cl path:{}\n", kernelPath.u16string());
    const auto str = common::file::ReadAllText(kernelPath);

    string fname;
    std::getline(std::cin, fname);
    common::fs::path fpath = fname;

    {
        auto rf = common::file::RawFileObject::OpenThrow(fpath, common::file::OpenFlag::ReadBinary);
        common::file::RawFileInputStream stream(rf);
        stream.Skip(10);
        common::io::BufferedRandomInputStream bs(std::move(stream), 65536);
        bs.Skip(20);
    }
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

    auto img2 = ProcessImg(str, img1, 2.0f);
    auto fpath3 = fpath;
    fpath3.replace_extension(".blur.jpg");
    xziar::img::WriteImage(img2, fpath3);
    
    /*auto rf2 = common::file::RawFileObject::OpenThrow(fpath3, common::file::OpenFlag::CreateNewBinary);
    common::file::RawFileOutputStream stream2(rf2);
    xziar::img::WriteImage(img2, stream2, u"jpg");*/

    getchar();
}
