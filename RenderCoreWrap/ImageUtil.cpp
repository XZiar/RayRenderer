#include "RenderCoreWrapRely.h"
#include "ImageUtil.h"
#include "common/CLIException.hpp"

using System::Windows::Media::PixelFormat;
using System::Windows::Media::PixelFormats;
using namespace System::Windows::Media::Imaging;
using namespace xziar::img;

namespace XZiar
{
namespace Img
{

static PixelFormat ConvertFormat(ImgDType dtype)
{
    switch (dtype.Value)
    {
    case ImageDataType::BGR  .Value: return PixelFormats::Bgr24;
    case ImageDataType::BGRA .Value: return PixelFormats::Bgra32;
    case ImageDataType::RGB  .Value: return PixelFormats::Rgb24;
    case ImageDataType::GRAY .Value: return PixelFormats::Gray8;
    case ImageDataType::GRAYf.Value: return PixelFormats::Gray32Float;
    case ImageDataType::RGBAf.Value: return PixelFormats::Rgba128Float;
    default:                         return PixelFormats::Default;
    }
}

static ImgDType ConvertFormat(PixelFormat pformat)
{
    if (pformat == PixelFormats::Bgr24)           return ImageDataType::BGR;
    if (pformat == PixelFormats::Pbgra32)         return ImageDataType::BGRA;
    if (pformat == PixelFormats::Bgra32)          return ImageDataType::BGRA;
    if (pformat == PixelFormats::Rgb24)           return ImageDataType::RGB;
    if (pformat == PixelFormats::Gray8)           return ImageDataType::GRAY;
    if (pformat == PixelFormats::Gray32Float)     return ImageDataType::GRAYf;
    if (pformat == PixelFormats::Prgba128Float)   return ImageDataType::RGBAf;
    if (pformat == PixelFormats::Rgba128Float)    return ImageDataType::RGBAf;
    
    return {};
}

BitmapSource^ ImageUtil::ReadImage(String^ filePath)
{
    return Convert(xziar::img::ReadImage(ToU16Str(filePath)));
}

void ImageUtil::WriteImage(BitmapSource^ image, String^ filePath)
{
    WriteImage(Convert(image), filePath);
}

void ImageUtil::WriteImage(const xziar::img::Image& image, String^ filePath)
{
    try
    {
        xziar::img::WriteImage(image, ToU16Str(filePath));
    }
    catch (const common::BaseException& be)
    {
        throw CPPException::FromException(be);
    }
}

BitmapSource^ ImageUtil::Convert(const xziar::img::Image& image)
{
    BitmapSource^ bmp = nullptr;
    if (const auto pf = ConvertFormat(image.GetDataType()); pf != PixelFormats::Default)
        return BitmapSource::Create(image.GetWidth(), image.GetHeight(), 96, 96, pf, nullptr,
            IntPtr(const_cast<std::byte*>(image.GetRawPtr())), (int)image.GetSize(), (int)image.RowSize());
    else
    {
        std::unique_ptr<Image> newimg;
        switch (image.GetDataType().Value)
        {
        case ImageDataType::RGBA.Value: newimg = std::make_unique<Image>(image.ConvertTo(ImageDataType::BGRA)); break;
        case ImageDataType::GA  .Value: newimg = std::make_unique<Image>(image.ConvertTo(ImageDataType::BGRA)); break;
            //others currently unsupported
        }
        if (!newimg)
            return nullptr;
        return BitmapSource::Create(newimg->GetWidth(), newimg->GetHeight(), 96, 96, ConvertFormat(newimg->GetDataType()), nullptr,
            IntPtr(newimg->GetRawPtr()), (int)newimg->GetSize(), (int)newimg->RowSize());
    }
}

Image ImageUtil::Convert(BitmapSource^ image)
{
    const auto pf = image->Format;
    if (const auto dt = ConvertFormat(pf); dt)
    {
        Image ret(dt);
        ret.SetSize(image->PixelWidth, image->PixelHeight);
        image->CopyPixels(Windows::Int32Rect::Empty, IntPtr(ret.GetRawPtr()), (int)ret.GetSize(), (int)ret.RowSize());
        return ret;
    }

    PixelFormat newpf;
    if (pf == PixelFormats::Bgr101010 || pf == PixelFormats::Bgr555 || pf == PixelFormats::Bgr565
        || pf == PixelFormats::Indexed8 || pf == PixelFormats::Indexed4 || pf == PixelFormats::Indexed2 || pf == PixelFormats::Indexed1
        || pf == PixelFormats::Cmyk32 || pf == PixelFormats::Bgr32)
        newpf = PixelFormats::Rgb24;
    else if (pf == PixelFormats::BlackWhite || pf == PixelFormats::Gray2 || pf == PixelFormats::Gray4)
        newpf = PixelFormats::Gray8;
    else if (pf == PixelFormats::Gray16)
        newpf = PixelFormats::Gray32Float;
    else if (pf == PixelFormats::Rgb48 || pf == PixelFormats::Rgb128Float)
        newpf = PixelFormats::Rgba128Float;
    else if (pf == PixelFormats::Prgba64)
        newpf = PixelFormats::Prgba128Float;

    const auto newbmp = gcnew FormatConvertedBitmap();
    newbmp->BeginInit();
    newbmp->Source = image;
    newbmp->DestinationFormat = newpf;
    newbmp->EndInit();
    return Convert(newbmp);
}


ImageHolder::ImageHolder(xziar::img::Image&& img)
{
    Img.Construct(std::move(img));
}

ImageHolder::ImageHolder(const xziar::img::Image& img)
{
    Img.Construct(img);
}

ImageHolder::!ImageHolder()
{
    Img.Destruct();
}

void ImageHolder::Save(String^ filePath)
{
    WRAPPER_NATIVE_PTR_DO(Img, rawimg, AsRawImage());
    ImageUtil::WriteImage(rawimg, filePath);
}

}
}