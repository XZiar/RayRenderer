#include "RenderCoreRely.h"
#include "ModelImage.h"
#include "../Material.h"


namespace rayr
{
using common::container::FindInMap;
using common::asyexe::AsyncAgent;


namespace detail
{

map<u16string, ModelImage> _ModelImage::images;

ModelImage _ModelImage::getImage(fs::path picPath, const fs::path& curPath)
{
    const auto fname = picPath.filename().u16string();
    auto img = getImage(fname);
    if (img)
        return img;

    if (!fs::exists(picPath))
    {
        picPath = curPath / fname;
        if (!fs::exists(picPath))
        {
            basLog().error(u"Fail to open image file\t[{}]\n", fname);
            return ModelImage();
        }
    }
    try
    {
        _ModelImage *mi = new _ModelImage(picPath.u16string());
        ModelImage image(std::move(mi));
        images.insert_or_assign(fname, image);
        return image;
    }
#pragma warning(disable:4101)
    catch (FileException& fe)
    {
        if (fe.reason == FileException::Reason::ReadFail)
            basLog().error(u"Fail to decode image file\t[{}]\n", picPath.u16string());
        else
            basLog().error(u"Cannot find image file\t[{}]\n", picPath.u16string());
        return img;
    }
#pragma warning(default:4101)
}

ModelImage _ModelImage::getImage(const u16string& pname)
{
    if (auto img = FindInMap(images, pname))
        return *img;
    else
        return ModelImage();
}
#pragma warning(disable:4996)
void _ModelImage::shrink()
{
    auto it = images.cbegin();
    while (it != images.end())
    {
        if (it->second.unique()) //deprecated, may lead to bug
            it = images.erase(it);
        else
            ++it;
    }
}
#pragma warning(default:4996)

_ModelImage::_ModelImage(const u16string& pfname) : Image(xziar::img::ReadImage(pfname))
{
    if (Width > UINT16_MAX || Height > UINT16_MAX)
        COMMON_THROW(BaseException, L"image too big");
}

_ModelImage::_ModelImage(const uint16_t w, const uint16_t h, const uint32_t color)
{
    SetSize(w, h);
}

oglu::oglTex2DS _ModelImage::genTexture(const oglu::TextureInnerFormat format)
{
    return rayr::GenTexture(*this, format);
}

oglu::oglTex2DS _ModelImage::genTextureAsync(const oglu::TextureInnerFormat format)
{
    return rayr::GenTextureAsync(*this, format, u"Comp-" + Name);
}


}
}