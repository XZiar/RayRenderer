#include "ImageUtilRely.h"
#include "ImageUtil.h"
#include "common/Linq.hpp"


namespace xziar::img
{
using std::vector;
using namespace common;

static vector<Wrapper<ImgSupport>>& SUPPORT_MAP()
{
    static vector<Wrapper<ImgSupport>> supports;
    return supports;
}

uint32_t RegistImageSupport(const Wrapper<ImgSupport>& support)
{
    SUPPORT_MAP().push_back(support);
    return 0;
}


static vector<Wrapper<ImgSupport>> GenerateSupportList(const u16string& ext, const ImageDataType dataType, const bool isRead, const bool allowDisMatch)
{
    return common::linq::Linq::FromIterable(SUPPORT_MAP())
        .Select([&](const auto& support) { return std::pair(support, support->MatchExtension(ext, dataType, isRead)); })
        .Where([=](const auto& spPair) { return allowDisMatch || spPair.second > 0; })
        .OrderBy([](const auto& l, const auto& r) { return l.second > r.second; })
        .Select([](const auto& spPair) { return spPair.first; })
        .ToVector();
}

static u16string GetExtName(const fs::path& path)
{
    auto ext = path.extension().u16string();
    return ext.empty() ? ext : ext.substr(1);
}

Image ReadImage(const fs::path& path, const ImageDataType dataType)
{
#define USEBUF 1
#if defined(USEBUF)
    const std::unique_ptr<RandomInputStream> stream = std::make_unique<BufferedRandomInputStream>(
        common::file::FileInputStream(file::FileObject::OpenThrow(path, file::OpenFlag::READ | file::OpenFlag::BINARY)),
        65599);
#else
    const std::unique_ptr<RandomInputStream> stream = std::make_unique<common::file::FileInputStream>(
        file::FileObject::OpenThrow(path, file::OpenFlag::READ | file::OpenFlag::BINARY));
#endif
    ImgLog().debug(u"Read Image {}\n", path.u16string());
    return ReadImage(stream, GetExtName(path), dataType);
}

Image ReadImage(const std::unique_ptr<RandomInputStream>& stream, const std::u16string& ext, const ImageDataType dataType)
{
    const auto extName = common::strchset::ToUpperEng(ext, common::str::Charset::UTF16LE);
    auto testList = GenerateSupportList(extName, dataType, true, true);
    for (auto& support : testList)
    {
        try
        {
            auto reader = support->GetReader(stream, extName);
            if (!reader->Validate())
            {
                stream->SetPos(0);
                continue;
            }
            ImgLog().debug(u"Using [{}]\n", support->Name);
            auto img = reader->Read(dataType);
            return img;
        }
        catch (BaseException& be)
        {
            ImgLog().warning(u"Read Image using {} receive error {}\n", support->Name, be.message);
        }
    }
    COMMON_THROW(BaseException, u"cannot read image");
}

void WriteImage(const Image& image, const fs::path & path, const uint8_t quality)
{
    const std::unique_ptr<RandomOutputStream> stream = std::make_unique<common::file::FileOutputStream>(
        file::FileObject::OpenThrow(path, file::OpenFlag::CreatNewBinary));
    ImgLog().debug(u"Write Image {}\n", path.u16string());
    WriteImage(image, stream, GetExtName(path), quality);
}

void WriteImage(const Image& image, const std::unique_ptr<RandomOutputStream>& stream, const std::u16string& ext, const uint8_t quality)
{
    const auto extName = common::strchset::ToUpperEng(ext, common::str::Charset::UTF16LE);
    auto testList = GenerateSupportList(extName, image.GetDataType(), false, false);
    for (auto& support : testList)
    {
        try
        {
            auto writer = support->GetWriter(stream, extName);
            ImgLog().debug(u"Using [{}]\n", support->Name);
            return writer->Write(image, quality);
        }
        catch (BaseException& be)
        {
            ImgLog().warning(u"Write Image using {} receive error {}\n", support->Name, be.message);
        }
    }
    COMMON_THROW(BaseException, u"cannot write image");
}


}

