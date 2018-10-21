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

Image ReadImage(const fs::path& path, const ImageDataType dataType)
{
    auto imgFile = file::FileObject::OpenThrow(path, file::OpenFlag::READ | file::OpenFlag::BINARY);
    ImgLog().debug(u"Read Image {}\n", path.u16string());
    const auto ext = str::ToUpperEng(path.extension().u16string(), common::str::Charset::UTF16LE);
    auto testList = GenerateSupportList(ext, dataType, true, true);
    for (auto& support : testList)
    {
        try
        {
            auto reader = support->GetReader(imgFile);
            if (!reader->Validate())
            {
                reader->Release();
                imgFile.Rewind();
                continue;
            }
            ImgLog().debug(u"Using [{}]\n", support->Name);
            auto img = reader->Read(dataType);
            reader->Release();
            return img;
        }
        catch (BaseException& be)
        {
            ImgLog().warning(u"Read Image using {} receive error {}\n", support->Name, be.message);
        }
    }
    COMMON_THROW(BaseException, u"cannot read image", path);
}

void WriteImage(const Image& image, const fs::path & path, const uint8_t quality)
{
    auto imgFile = file::FileObject::OpenThrow(path, file::OpenFlag::CreatNewBinary);
    ImgLog().debug(u"Write Image {}\n", path.u16string());
    const auto ext = str::ToUpperEng(path.extension().u16string(), common::str::Charset::UTF16LE);
    auto testList = GenerateSupportList(ext, image.GetDataType(), false, false);
    for (auto& support : testList)
    {
        try
        {
            auto writer = support->GetWriter(imgFile);
            ImgLog().debug(u"Using [{}]\n", support->Name);
            return writer->Write(image, quality);
        }
        catch (BaseException& be)
        {
            ImgLog().warning(u"Write Image using {} receive error {}\n", support->Name, be.message);
        }
    }
    return;
}


}

