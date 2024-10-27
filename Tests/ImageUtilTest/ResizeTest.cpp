#include "rely.h"
#include "ImageUtil/ImageUtil.h"
#include "ImageUtil/ImageCore.h"
#include "ImageUtil/ColorConvert.h"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/ConsoleEx.h"


void TestResizeImage(std::string filepath)
{
    using namespace xziar::img;
    common::mlog::MiniLogger<false> logger(u"ImgTest", { common::mlog::GetConsoleBackend() });
    while (filepath.empty())
    {
        common::mlog::SyncConsoleBackend();
        filepath = common::console::ConsoleEx::ReadLine("image file to test:");
    }

    common::fs::path fpath(filepath);
    auto img0 = ReadImage(fpath, ImageDataType::RGBA);
    const auto folder = fpath.parent_path();
    const auto basename = fpath.stem().string();
    const auto w = img0.GetWidth(), h = img0.GetHeight();
    logger.Info(u"read img [{}]: {}x{}\n", basename, w, h);

    auto img1 = img0.ConvertTo(ImageDataType::RGB);
    WriteImage(img1, folder / (basename + "-rgb.bmp"));
    logger.Info(u"write rgb img\n");

    auto img0half = img0.ResizeTo(w / 2, h / 2, true);
    WriteImage(img0half, folder / (basename + "-half.bmp"));
    logger.Info(u"write half img\n");

    auto img0sq = img0.ResizeTo(w / 3, w / 3, true);
    WriteImage(img0sq, folder / (basename + "-sq.bmp"));
    logger.Info(u"write square img\n");

    auto img1half = img1.ResizeTo(w / 2, h / 2, true);
    WriteImage(img1half, folder / (basename + "-rgb-half.bmp"));
    logger.Info(u"write rgb half img\n");

    auto img1sq = img1.ResizeTo(w / 3, w / 3, true);
    WriteImage(img1sq, folder / (basename + "-rgb-sq.bmp"));
    logger.Info(u"write rgb square img\n");

    auto img1r = img1.ExtractChannel(0);
    WriteImage(img1r, folder / (basename + "-r.bmp"));
    logger.Info(u"write R channel\n");
    auto img1g = img1.ExtractChannel(1);
    WriteImage(img1g, folder / (basename + "-g.bmp"));
    logger.Info(u"write G channel\n");
    auto img1b = img1.ExtractChannel(2);
    WriteImage(img1b, folder / (basename + "-b.bmp"));
    logger.Info(u"write B channel\n");

}
