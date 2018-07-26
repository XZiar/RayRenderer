#include "RenderCoreWrapRely.h"
#include "ImageUtil.h"

using System::Windows::Media::PixelFormat;
using System::Windows::Media::PixelFormats;
using namespace System::Windows::Media::Imaging;
using namespace xziar::img;

namespace XZiar
{
namespace Img
{

static PixelFormat ConvertFormat(ImageDataType dtype)
{
    switch (dtype)
    {
    case ImageDataType::BGR:    return PixelFormats::Bgr24;
    case ImageDataType::BGRA:   return PixelFormats::Bgra32;
    case ImageDataType::RGB:    return PixelFormats::Rgb24;
    case ImageDataType::GRAY:   return PixelFormats::Gray8;
    case ImageDataType::GRAYf:  return PixelFormats::Gray32Float;
    case ImageDataType::RGBAf:  return PixelFormats::Rgba128Float;
    default:                    return PixelFormats::Default;
    }
}

BitmapSource^ ImageUtil::ReadImage(String^ filePath)
{
    return Convert(xziar::img::ReadImage(ToU16Str(filePath)));
}

void ImageUtil::WriteImage(BitmapSource^ image, String^ filePath)
{
    xziar::img::WriteImage(Convert(image), ToU16Str(filePath));
}

BitmapSource^ ImageUtil::Convert(const xziar::img::Image& image)
{
    BitmapSource^ bmp = nullptr;
    const auto pf = ConvertFormat(image.DataType);
    if (pf != PixelFormats::Default)
        return BitmapSource::Create(image.Width, image.Height, 96, 96, pf, nullptr,
            IntPtr(const_cast<std::byte*>(image.GetRawPtr())), (int)image.GetSize(), image.ElementSize * image.Width);
    else
    {
        std::unique_ptr<Image> newimg;
        switch (image.DataType)
        {
        case ImageDataType::RGBA:   newimg = std::make_unique<Image>(image.ConvertTo(ImageDataType::BGRA)); break;
        case ImageDataType::GA:     newimg = std::make_unique<Image>(image.ConvertTo(ImageDataType::BGRA)); break;
            //others currently unsupported
        }
        if (!newimg)
            return nullptr;
        return BitmapSource::Create(newimg->Width, newimg->Height, 96, 96, ConvertFormat(newimg->DataType), nullptr,
            IntPtr(newimg->GetRawPtr()), (int)newimg->GetSize(), newimg->ElementSize * newimg->Width);
    }
}

Image ImageUtil::Convert(BitmapSource^ image)
{
    return Image();
}


}
}