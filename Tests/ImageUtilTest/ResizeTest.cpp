#include "rely.h"
#include "ImageUtil/ImageUtil.h"
#include "ImageUtil/ImageCore.h"
#include "ImageUtil/ImageSupport.hpp"
#include "SystemCommon/FileEx.h"
#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/ConsoleEx.h"
#include "SystemCommon/FileMapperEx.h"

using namespace xziar::img;


std::vector<std::shared_ptr<const ImgSupport>> GetSupport(common::mlog::MiniLogger<false>& logger, std::u16string ext, ImgDType dtype, bool isRead, std::u16string_view prefer)
{
    auto supports = GetImageSupport(ext, dtype, isRead);
    const auto it = std::find_if(supports.begin(), supports.end(), [&](const auto& support) { return support->Name == prefer; });
    if (it != supports.end())
        std::rotate(supports.begin(), it, it + 1);
    std::u16string txt;
    for (const auto& support : supports)
    {
        txt.push_back(u' ');
        txt.append(support->Name);
    }
    logger.Debug(u"Support for [{}][{}][{}]: {}\n", ext, isRead ? 'R' : 'W', dtype.ToString(), txt);
    return supports;
}

void WriteBmp(common::mlog::MiniLogger<false>& logger, std::u16string_view writer, std::u16string_view name, const Image& img, common::fs::path fpath)
{
    common::file::FileOutputStream stream(common::file::FileObject::OpenThrow(fpath, common::file::OpenFlag::CreateNewBinary));
    logger.Debug(u"Write Image {}\n", fpath.u16string());

    const auto supports = GetSupport(logger, u"BMP", img.GetDataType(), false, writer);
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


void TestResizeImage(std::string filepath, std::string_view reader_, std::string_view writer_)
{
    common::mlog::MiniLogger<false> logger(u"ImgTest", { common::mlog::GetConsoleBackend() });
    while (filepath.empty())
    {
        common::mlog::SyncConsoleBackend();
        filepath = common::console::ConsoleEx::ReadLine("image file to test:");
    }

    common::fs::path fpath(filepath);
    fpath = common::fs::absolute(fpath);
    Image img0;
    {
        auto stream = common::file::MapFileForRead(fpath);
        logger.Debug(u"Read Image {}\n", fpath.u16string());
        auto ext = fpath.extension().u16string();
        if (!ext.empty()) 
            ext = common::str::ToUpperEng(std::u16string_view(ext).substr(1), common::str::Encoding::UTF16LE);

        const std::u16string reader(reader_.begin(), reader_.end());
        const auto supports = GetSupport(logger, ext, ImageDataType::RGBA, true, reader);
        for (const auto& support : supports)
        {
            try
            {
                const auto inputer = support->GetReader(stream, ext);
                if (!inputer->Validate())
                {
                    stream.SetPos(0);
                    continue;
                }
                img0 = inputer->Read(ImageDataType::RGBA);
                logger.Info(u"readed img using {}\n", support->Name);
            }
            catch (const common::BaseException& be)
            {
                logger.Warning(u"Read Image using {} receive error {}\n", support->Name, be.Message());
            }
            catch (...)
            {
                logger.Warning(u"Read Image using {} receive error\n", support->Name);
            }
        }
        if (!img0.GetSize())
        {
            logger.Error(u"Read Image failed with no backend support\n");
            return;
        }
    }
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

    const auto channels = img0.ExtractChannels();
    Ensures(channels.size() == 4);
    WriteBmp(logger, writer, u"R channel", channels[0], folder / (basename + "-r.bmp"));
    WriteBmp(logger, writer, u"G channel", channels[1], folder / (basename + "-g.bmp"));
    WriteBmp(logger, writer, u"B channel", channels[2], folder / (basename + "-b.bmp"));

    Image combine3(ImageDataType::GRAY);
    combine3.SetSize(w * 3 / 2, h / 2);
    channels[0].ResizeTo(combine3, 0, 0, 0, 0, w / 2, h / 2, true);
    channels[1].ResizeTo(combine3, 0, 0, w / 2, 0, w / 2, h, true);
    channels[2].ResizeTo(combine3, w / 4, 0, w, 0, w, h / 2, true);
    WriteBmp(logger, writer, u"Gray Combine", combine3, folder / (basename + "-c3-half.bmp"));
}
