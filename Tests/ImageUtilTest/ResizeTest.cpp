#include "rely.h"
#include "ImageUtil/ImageUtil.h"
#include "ImageUtil/ImageCore.h"
#include "ImageUtil/ImageSupport.hpp"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/ConsoleEx.h"

using namespace xziar::img;


void WriteBmp(common::mlog::MiniLogger<false>& logger, std::u16string_view writer, std::u16string_view name, const Image& img, common::fs::path fpath)
{
    auto supports = GetImageSupport(u"BMP", img.GetDataType(), false);
    const auto it = std::find_if(supports.begin(), supports.end(), [&](const auto& support) { return support->Name == writer; });
    if (it != supports.end())
        std::rotate(supports.begin(), it, it + 1);
    
    common::file::FileOutputStream stream(common::file::FileObject::OpenThrow(fpath, common::file::OpenFlag::CreateNewBinary));
    logger.Debug(u"Write Image {}\n", fpath.u16string());

    for (const auto& support : supports)
    {
        try
        {
            const auto outer = support->GetWriter(stream, u"BMP");
            outer->Write(img, 100);
            logger.Info(u"writed {} img using {}\n", name, support->Name);
            return;
        }
        catch (const common::BaseException& be)
        {
            logger.Warning(u"Write Image using {} receive error {}\n", support->Name, be.Message());
        }
        catch (...)
        {
            logger.Warning(u"Write Image using {} receive error\n", support->Name);
        }
    }
    logger.Error(u"Write Image failed with no backend support\n");
}


void TestResizeImage(std::string filepath, std::string_view writer_)
{
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

    const std::u16string writer(writer_.begin(), writer_.end());

    auto img1 = img0.ConvertTo(ImageDataType::BGR);
    WriteBmp(logger, writer, u"bgr", img1, folder / (basename + "-bgr.bmp"));

    auto img0half = img0.ResizeTo(w / 2, h / 2, true);
    WriteBmp(logger, writer, u"half", img0half, folder / (basename + "-half.bmp"));

    auto img0sq = img0.ResizeTo(w / 3, w / 3, true);
    WriteBmp(logger, writer, u"square", img0sq, folder / (basename + "-sq.bmp"));

    auto img1half = img1.ResizeTo(w / 2, h / 2, true);
    WriteBmp(logger, writer, u"bgr half", img1half, folder / (basename + "-bgr-half.bmp"));

    auto img1sq = img1.ResizeTo(w / 3, w / 3, true);
    WriteBmp(logger, writer, u"bgr square", img1sq, folder / (basename + "-bgr-sq.bmp"));

    auto img0r = img0.ExtractChannel(0);
    WriteBmp(logger, writer, u"R channel", img0r, folder / (basename + "-r.bmp"));

    auto img0g = img0.ExtractChannel(1);
    WriteBmp(logger, writer, u"G channel", img0g, folder / (basename + "-g.bmp"));

    auto img0b = img0.ExtractChannel(2);
    WriteBmp(logger, writer, u"B channel", img0b, folder / (basename + "-b.bmp"));

}
