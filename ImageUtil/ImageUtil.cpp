#include "ImageUtilPch.h"
#include "ImageUtil.h"
#include "ImageSupport.hpp"


namespace xziar::img
{
using std::byte;
using std::string;
using std::wstring;
using std::u16string;
using std::vector;

static vector<std::shared_ptr<ImgSupport>>& SUPPORT_MAP()
{
    static vector<std::shared_ptr<ImgSupport>> supports;
    return supports;
}

uint32_t RegistImageSupport(std::shared_ptr<ImgSupport> support)
{
    SUPPORT_MAP().emplace_back(support);
    return 0;
}


static vector<std::reference_wrapper<const ImgSupport>> GenerateSupportList(const u16string& ext, const ImageDataType dataType, const bool isRead, const bool allowDisMatch)
{
    return common::linq::FromIterable(SUPPORT_MAP())
        .Select([&](const auto& support) { return std::pair(std::cref(*support), support->MatchExtension(ext, dataType, isRead)); })
        .Where([=](const auto& spPair) { return allowDisMatch || spPair.second > 0; })
        .OrderBy([](const auto& l, const auto& r) { return l.second > r.second; })
        .Select([](const auto& spPair) { return spPair.first; })
        .ToVector();
}

static u16string GetExtName(const common::fs::path& path)
{
    auto ext = path.extension().u16string();
    return ext.empty() ? ext : ext.substr(1);
}

Image ReadImage(const common::fs::path& path, const ImageDataType dataType)
{
#define USEBUF 1
#if defined(USEBUF)
    BufferedRandomInputStream stream(
        common::file::FileInputStream(common::file::FileObject::OpenThrow(path, common::file::OpenFlag::ReadBinary)),
        65536);
#else
    common::file::FileInputStream stream(common::file::FileObject::OpenThrow(path, common::file::OpenFlag::ReadBinary));
#endif
    ImgLog().debug(u"Read Image {}\n", path.u16string());
    return ReadImage(stream, GetExtName(path), dataType);
}

Image ReadImage(RandomInputStream& stream, const std::u16string& ext, const ImageDataType dataType)
{
    const auto extName = common::strchset::ToUpperEng(ext, common::str::Charset::UTF16LE);
    auto testList = GenerateSupportList(extName, dataType, true, true);
    for (const ImgSupport& support : testList)
    {
        try
        {
            auto reader = support.GetReader(stream, extName);
            if (!reader->Validate())
            {
                stream.SetPos(0);
                continue;
            }
            ImgLog().debug(u"Using [{}]\n", support.Name);
            auto img = reader->Read(dataType);
            return img;
        }
        catch (BaseException& be)
        {
            ImgLog().warning(u"Read Image using {} receive error {}\n", support.Name, be.message);
        }
    }
    COMMON_THROW(BaseException, u"cannot read image");
}

void WriteImage(const Image& image, const common::fs::path & path, const uint8_t quality)
{
    common::file::FileOutputStream stream(common::file::FileObject::OpenThrow(path, common::file::OpenFlag::CreateNewBinary));
    ImgLog().debug(u"Write Image {}\n", path.u16string());
    WriteImage(image, stream, GetExtName(path), quality);
}

void WriteImage(const Image& image, RandomOutputStream& stream, const std::u16string& ext, const uint8_t quality)
{
    const auto extName = common::strchset::ToUpperEng(ext, common::str::Charset::UTF16LE);
    auto testList = GenerateSupportList(extName, image.GetDataType(), false, false);
    for (const ImgSupport& support : testList)
    {
        try
        {
            auto writer = support.GetWriter(stream, extName);
            ImgLog().debug(u"Using [{}]\n", support.Name);
            writer->Write(image, quality);
            stream.Flush();
            return;
        }
        catch (BaseException& be)
        {
            ImgLog().warning(u"Write Image using {} receive error {}\n", support.Name, be.message);
        }
    }
    COMMON_THROW(BaseException, u"cannot write image");
}


}

