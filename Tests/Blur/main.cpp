#include "OpenCLUtil/OpenCLUtil.h"
#include "MiniLogger/MiniLogger.h"
#include "common/FileEx.hpp"
#include "common/MemoryStream.hpp"
#include "common/SpinLock.hpp"
#include <thread>
#include <mutex>
#include <array>
#include <cmath>

using namespace oclu;
using namespace common::mlog;
using namespace xziar::img;
using namespace std::string_view_literals;
using std::string;

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

    Linq::FromIterable(plats)
        .ForEach([i = 0u](const auto& plat) mutable { log().info(u"option[{}] {}\t{}\n", i++, plat->Name, plat->Ver); });
    const auto plat = plats[1];

    auto thedev = Linq::FromIterable(plat->GetDevices())
        .Where([](const auto& dev) { return dev->Type == DeviceType::GPU; })
        .TryGetFirst().value_or(plat->GetDefaultDevice());
    const auto ctx = plat->CreateContext(thedev);
    //ctx->onMessage = [](const auto& str) { log().debug(u"[MSG]{}\n", str); };

    oclCmdQue cmdque(ctx, thedev);
    oclProgram prog(ctx, kernel);
    try
    {
        prog->Build({}, thedev);
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
    oclBuffer rawBuf(ctx, MemFlag::ReadWrite, image.GetSize());
    pms = rawBuf->Write(cmdque, image.GetRawPtr(), image.GetSize(), 0, false);
    pms->Wait();
    const auto time1 = pms->ElapseNs() / 1e6f;
    oclBuffer midBuf1(ctx, MemFlag::ReadWrite, image.GetSize() *sizeof(float));
    oclBuffer midBuf2(ctx, MemFlag::ReadWrite, image.GetSize() *sizeof(float));
    
    const auto width = image.GetWidth(), height = image.GetHeight();
    const auto w4 = width - width % 4, h4 = height - height % 4;
    {
        blurX->SetArg(0, rawBuf);
        blurX->SetArg(1, midBuf1);
        blurX->SetArg(2, midBuf2);
        blurX->SetSimpleArg(3, w4);
        blurX->SetSimpleArg(4, h4);
        blurX->SetSimpleArg(5, width);
        blurX->SetSimpleArg(6, coeff);
        pms = blurX->Run<1>(cmdque, { h4 }, false);
    }
    pms->Wait();
    const auto time2 = pms->ElapseNs() / 1e6f;
    {
        blurY->SetArg(0, midBuf2);
        blurY->SetArg(1, midBuf1);
        blurY->SetArg(2, rawBuf);
        blurY->SetSimpleArg(3, w4);
        blurY->SetSimpleArg(4, h4);
        blurY->SetSimpleArg(5, width);
        blurY->SetSimpleArg(6, coeff);
        pms = blurY->Run<1>(cmdque, { w4 }, false);
    }
    pms->Wait();
    const auto time3 = pms->ElapseNs() / 1e6f;
    xziar::img::Image img2(xziar::img::ImageDataType::RGBA);
    img2.SetSize(image.GetWidth(), image.GetHeight());
    rawBuf->Read(cmdque, img2.GetRawPtr(), width*height*4, false);
    pms->Wait();
    const auto time4 = pms->ElapseNs() / 1e6f;
    log().info(u"WRITE[{}ms], BLURX[{}ms], BLURY[{}ms], READ[{}ms]\n", time1, time2, time3, time4);
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

    auto img = xziar::img::ReadImage("./download.jpg");

    auto data = common::file::ReadAll<std::byte>("./download.jpg");
    common::io::ContainerInputStream<std::vector<std::byte>> stream(data);
    auto img1 = xziar::img::ReadImage(stream, u"JPG");

    xziar::img::WriteImage(img, "./downlaod0.bmp");
    xziar::img::WriteImage(img1, "./downlaod1.bmp");

    auto img2 = ProcessImg(str, img, 2.0f);
    xziar::img::WriteImage(img2, "./downlaod2.jpg");
    data.resize(0);
    common::io::ContainerOutputStream<std::vector<std::byte>> stream2(data);
    xziar::img::WriteImage(img2, stream2, u"JPG");
    common::file::WriteAll("./downlaod3.jpg", data);
    getchar();
}
