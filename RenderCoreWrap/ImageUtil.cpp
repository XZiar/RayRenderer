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

static ImageDataType ConvertFormat(PixelFormat pformat)
{
    if (pformat == PixelFormats::Bgr24)           return ImageDataType::BGR;
    if (pformat == PixelFormats::Pbgra32)         return ImageDataType::BGRA;
    if (pformat == PixelFormats::Bgra32)          return ImageDataType::BGRA;
    if (pformat == PixelFormats::Rgb24)           return ImageDataType::RGB;
    if (pformat == PixelFormats::Gray8)           return ImageDataType::GRAY;
    if (pformat == PixelFormats::Gray32Float)     return ImageDataType::GRAYf;
    if (pformat == PixelFormats::Prgba128Float)   return ImageDataType::RGBAf;
    if (pformat == PixelFormats::Rgba128Float)    return ImageDataType::RGBAf;
    
    return ImageDataType::UNKNOWN_RESERVE;
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
    catch (BaseException& be)
    {
        throw gcnew CPPException(be);
    }
}

BitmapSource^ ImageUtil::Convert(const xziar::img::Image& image)
{
    BitmapSource^ bmp = nullptr;
    if (const auto pf = ConvertFormat(image.DataType); pf != PixelFormats::Default)
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
    const auto pf = image->Format;
    if (const auto dt = ConvertFormat(pf); dt != ImageDataType::UNKNOWN_RESERVE)
    {
        Image ret(dt);
        ret.Resize(image->PixelWidth, image->PixelHeight);
        image->CopyPixels(Windows::Int32Rect::Empty, IntPtr(ret.GetRawPtr()), (int)ret.GetSize(), ret.ElementSize * ret.Width);
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


void ImageSaver::Save(String^ filePath)
{
    ImageUtil::WriteImage(Img.Extract(), filePath);
}

}
}