#include "ImageUtilPch.h"
#include "ImageCore.h"
#include "ColorConvert.h"
#include "SystemCommon/FormatExtra.h"

// #define STB_IMAGE_RESIZE2_IMPLEMENTATION
// #define STB_IMAGE_RESIZE_STATIC
#include "stb/stb_image_resize2.h"


namespace xziar::img
{
using std::byte;
using std::string;
using std::wstring;
using std::u16string;
using common::BaseException;



void Image::ResetSize(const uint32_t width, const uint32_t height)
{
    ReAlloc(static_cast<size_t>(width)* height* ElementSize, 64);
    Width = width, Height = height;
}

void Image::FlipVertical()
{
    const auto lineStep = RowSize();
    auto rowUp = Data, rowDown = Data + (Height - 1) * lineStep;
    while (rowUp < rowDown)
    {
        convert::Swap2Buffer(rowUp, rowDown, lineStep);
        rowUp += lineStep, rowDown -= lineStep;
    }
}

void Image::FlipHorizontal()
{
    const auto lineStep = RowSize();
    auto ptr = Data;
    if (ElementSize == 4)
    {
        for (uint32_t row = 0; row < Height; ++row)
        {
            convert::ReverseBuffer4(ptr, Width);
            ptr += lineStep;
        }
    }
    else if (ElementSize == 3)
    {
        for (uint32_t row = 0; row < Height; ++row)
        {
            convert::ReverseBuffer3(ptr, Width);
            ptr += lineStep;
        }
    }
    else
    {
        COMMON_THROW(BaseException, u"unsupported operation");
    }
}

void Image::Rotate180()
{
    const auto count = Width * Height;
    if (ElementSize == 4)
    {
        convert::ReverseBuffer4(Data, count);
    }
    else if (ElementSize == 3)
    {
        convert::ReverseBuffer3(Data, count);
    }
    else
    {
        COMMON_THROW(BaseException, u"unsupported operation");
    }
}


Image Image::FlipToVertical() const
{
    Image img(DataType);
    img.SetSize(Width, Height);

    const auto lineStep = RowSize();
    const auto ptrFrom = Data + (Height - 1) * lineStep;
    auto* const ptrTo = img.Data;
    for (auto i = Height; i--;)
        memcpy_s(ptrTo, lineStep, ptrFrom, lineStep);
    return img;
}

Image Image::FlipToHorizontal() const
{
    Image img(*this);
    img.FlipHorizontal();
    return img;
}

Image Image::RotateTo180() const
{

    Image img(*this);
    img.Rotate180();
    return img;
}

void Image::PlaceImage(const Image& src, const uint32_t srcX, const uint32_t srcY, const uint32_t destX, const uint32_t destY)
{
    if (src.Data == Data)
        return;//place self,should throw
    if (srcX >= src.Width || srcY >= src.Height || destX >= Width || destY >= Height)
        return;

    const byte* __restrict srcPtr = src.GetRawPtr(srcY, srcX);
    byte* __restrict destPtr = GetRawPtr(destY, destX);
    const auto srcStep = src.RowSize(), destStep = RowSize();
    const auto copypix = std::min(Width - destX, src.Width - srcX);
    auto rowcnt = std::min(Height - destY, src.Height - srcY);
    const bool isCopyWholeRow = copypix == Width && Width == src.Width;
    if (src.DataType == DataType)
    {
        auto copysize = copypix * ElementSize;
        if (isCopyWholeRow)
            copysize *= rowcnt, rowcnt = 1;//treat as one row
        for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
            memcpy_s(destPtr, copysize, srcPtr, copysize);
    }
    else
    {
        if (HAS_FIELD(src.DataType, ImageDataType::FLOAT_MASK))//not supported yet
            COMMON_THROW(BaseException, u"float source not supported");
        if (IsGray() && !src.IsGray())//not supported yet
            COMMON_THROW(BaseException, u"need explicit conversion between color & gray image");

        const auto diff = DataType ^ src.DataType;
        auto pixcnt = copypix;
        if (isCopyWholeRow)
            pixcnt *= rowcnt, rowcnt = 1;
        const auto& cvter = ColorConvertor::Get();
        if (diff == ImageDataType::ALPHA_MASK)//remove/add alpha only
        {
            switch (src.ElementSize)
            {
            case 4://remove alpha, 4->3
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    cvter.RGBAToRGB(reinterpret_cast<uint8_t*>(destPtr), reinterpret_cast<const uint32_t*>(srcPtr), pixcnt);
                break;
            case 3://add alpha, 3->4
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    cvter.RGBToRGBA(reinterpret_cast<uint32_t*>(destPtr), reinterpret_cast<const uint8_t*>(srcPtr), pixcnt);
                break;
            case 2://remove alpha, 2->1
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    cvter.GrayAToGray(reinterpret_cast<uint8_t*>(destPtr), reinterpret_cast<const uint16_t*>(srcPtr), pixcnt);
                break;
            case 1://add alpha, 1->2
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    cvter.GrayToGrayA(reinterpret_cast<uint16_t*>(destPtr), reinterpret_cast<const uint8_t*>(srcPtr), pixcnt);
                break;
            }
        }
        else if (ElementSize == 4)
        {
            switch (src.ElementSize)
            {
            case 1:
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    cvter.GrayToRGBA(reinterpret_cast<uint32_t*>(destPtr), reinterpret_cast<const uint8_t*>(srcPtr), pixcnt);
                break;
            case 2:
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    cvter.GrayAToRGBA(reinterpret_cast<uint32_t*>(destPtr), reinterpret_cast<const uint16_t*>(srcPtr), pixcnt);
                break;
            case 3://change byte-order and add alpha(plain-add, see above) 
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    cvter.BGRToRGBA(reinterpret_cast<uint32_t*>(destPtr), reinterpret_cast<const uint8_t*>(srcPtr), pixcnt);
                break;
            case 4://change byte-order only(plain copy, see above*2)
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    cvter.RGBAToBGRA(reinterpret_cast<uint32_t*>(destPtr), reinterpret_cast<const uint32_t*>(srcPtr), pixcnt);
                break;
            }
        }
        else if (ElementSize == 3)
        {
            switch (src.ElementSize)
            {
            case 1:
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    cvter.GrayToRGB(reinterpret_cast<uint8_t*>(destPtr), reinterpret_cast<const uint8_t*>(srcPtr), pixcnt);
                break;
            case 2:
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    convert::GrayAsToRGBs(destPtr, srcPtr, pixcnt);
                break;
            case 3://change byte-order only(plain copy, see above*2)
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    cvter.RGBToBGR(reinterpret_cast<uint8_t*>(destPtr), reinterpret_cast<const uint8_t*>(srcPtr), pixcnt);
                break;
            case 4://change byte-order and remove alpha(plain-add, see above) 
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    cvter.RGBAToBGR(reinterpret_cast<uint8_t*>(destPtr), reinterpret_cast<const uint32_t*>(srcPtr), pixcnt);
                break;
            }
        }
    }
    //put finish
}

void Image::Resize(uint32_t width, uint32_t height, const bool isSRGB, const bool mulAlpha)
{
    *this = ResizeTo(width, height, isSRGB, mulAlpha);
}

Image Image::ResizeTo(uint32_t width, uint32_t height, const bool isSRGB, const bool mulAlpha) const
{
    if (width == 0 && height == 0)
        COMMON_THROW(BaseException, u"image size cannot be all zero!");
    /*if (mulAlpha && !HAS_FIELD(DataType, ImageDataType::ALPHA_MASK))
        ImgLog().Warning(u"Asks for premultiplied alpha for non-alpha source, ignored");*/
    if (isSRGB && HAS_FIELD(DataType, ImageDataType::FLOAT_MASK))
        ImgLog().Warning(u"Asks for sRGB handling for float type, ignored");

    width = width == 0 ? (uint32_t)((uint64_t)height * Width / Height) : width;
    height = height == 0 ? (uint32_t)((uint64_t)width * Height / Width) : height;
    Image output(DataType);
    output.SetSize(width, height);

    const auto datatype = HAS_FIELD(DataType, ImageDataType::FLOAT_MASK) ? STBIR_TYPE_FLOAT : 
        (isSRGB ? STBIR_TYPE_UINT8_SRGB : STBIR_TYPE_UINT8);
    stbir_pixel_layout pixLayout = STBIR_1CHANNEL;
    switch (REMOVE_MASK(DataType, ImageDataType::FLOAT_MASK))
    {
    case ImageDataType::GRAY: pixLayout = STBIR_1CHANNEL; break;
    case ImageDataType::RGB:  pixLayout = STBIR_RGB; break;
    case ImageDataType::BGR:  pixLayout = STBIR_BGR; break;
    case ImageDataType::RGBA: pixLayout = mulAlpha ? STBIR_RGBA_PM : STBIR_RGBA; break;
    case ImageDataType::BGRA: pixLayout = mulAlpha ? STBIR_BGRA_PM : STBIR_BGRA; break;
    case ImageDataType::GA:   pixLayout = mulAlpha ? STBIR_RA_PM   : STBIR_RA; break;
    default:
        COMMON_THROW(BaseException, u"unsupported datatype!");
    }

    static const STBResize& resizer = []() -> const STBResize&
    {
        const auto& stbResizer = STBResize::Get();
        std::string pathtxt;
        common::str::Formatter<char> fmter;
        const auto selections = stbResizer.GetIntrinMap();
        for (const auto& pathinfo : STBResize::GetSupportMap())
        {
            fmter.FormatToStatic(pathtxt, FmtString("\n--[{}] get paths:"), pathinfo.FuncName.View);
            std::string_view uses;
            for (const auto& item : selections)
                if (item.first == pathinfo.FuncName.View)
                {
                    uses = item.second;
                    break;
                }
            const auto& syntax = common::str::FormatterCombiner::Combine(FmtString(" {}"), FmtString(" [{}]"));
            for (const auto& methods : pathinfo.Variants)
                syntax(methods.MethodName.View == uses ? 1 : 0).To(pathtxt, fmter, methods.MethodName.View);
        }
        ImgLog().Verbose(u"Loaded STBResizer:{}\n", pathtxt);
        return stbResizer;
    }();

    STBResize::ResizeInfo info
    {
        .Input = Data,
        .Output = output.GetRawPtr(),
        .InputSizes = { Width, Height },
        .OutputSizes = { width, height },
        .Layout = static_cast<uint8_t>(pixLayout),
        .Datatype = static_cast<uint8_t>(datatype),
        .Edge = static_cast<uint8_t>(STBIR_EDGE_REFLECT),
        .Filter = static_cast<uint8_t>(STBIR_FILTER_TRIANGLE),
    };
    resizer.Resize(info);
    
    return output;
}


Image Image::Region(const uint32_t x, const uint32_t y, uint32_t w, uint32_t h) const
{
    if (w == 0) w = Width;
    if (h == 0) h = Height;
    if (x == y && y == 0 && w == Width && h == Height)
        return *this;

    Image newimg(DataType);
    newimg.SetSize(w, h);
    if (x >= Width || y >= Height)
        return newimg;
    newimg.PlaceImage(*this, x, y, 0, 0);
    return newimg;
}

Image Image::ConvertTo(const ImageDataType dataType, const uint32_t x, const uint32_t y, uint32_t w, uint32_t h) const
{
    if (w == 0) w = Width;
    if (h == 0) h = Height;
    if (dataType == DataType)
        return Region(x, y, w, h);

    Image newimg(dataType);
    newimg.SetSize(w, h);
    newimg.PlaceImage(*this, x, y, 0, 0);
    return newimg;
}

Image Image::ConvertToFloat(const float floatRange) const
{
    if (!HAS_FIELD(DataType, ImageDataType::FLOAT_MASK))
    {
        Image newimg(DataType | ImageDataType::FLOAT_MASK);
        newimg.SetSize(Width, Height);
        common::CopyEx.CopyToFloat(reinterpret_cast<float*>(newimg.Data), reinterpret_cast<const uint8_t*>(Data),
            Size, floatRange);
        return newimg;
    }
    else
    {
        Image newimg(REMOVE_MASK(DataType, ImageDataType::FLOAT_MASK));
        newimg.SetSize(Width, Height);
        common::CopyEx.CopyFromFloat(reinterpret_cast<uint8_t*>(newimg.Data), reinterpret_cast<const float*>(Data),
            Size, floatRange);
        return newimg;
    }
}

Image Image::ExtractChannel(uint8_t channel, bool keepAlpha) const
{
    if (HAS_FIELD(DataType, ImageDataType::FLOAT_MASK))
        COMMON_THROW(BaseException, u"not support extract channel from float image");

    uint32_t chLimit = 0;
    switch (DataType)
    {
    case ImageDataType::GRAY: chLimit = 1; break;
    case ImageDataType::RGB:  [[fallthrough]];
    case ImageDataType::BGR:  chLimit = 3; break;
    case ImageDataType::RGBA: [[fallthrough]];
    case ImageDataType::BGRA: chLimit = 4; break;
    case ImageDataType::GA:   chLimit = 2; break;
    default:
        COMMON_THROW(BaseException, u"unsupported datatype!");
    }
    if (channel >= chLimit)
        COMMON_THROW(BaseException, u"invalid channel index when extract channel");

    if (HAS_FIELD(DataType, ImageDataType::ALPHA_MASK) && keepAlpha)
    {
        if (channel == chLimit - 1) // alpha channel
        {
            if (DataType == ImageDataType::GA)
                COMMON_THROW(BaseException, u"unsupported extracting alpha from GA!");
            keepAlpha = false;
        }
    }

    if (REMOVE_MASK(DataType, ImageDataType::ALPHA_MASK) == ImageDataType::BGR)
    {
        if (channel == 0) channel = 2;
        else if (channel == 2) channel = 0;
    }

    if (DataType == ImageDataType::GRAY) return *this;
    Ensures(chLimit == 2 || chLimit == 3 || chLimit == 4);

    Image newimg(ImageDataType::GRAY | (keepAlpha ? ImageDataType::ALPHA_MASK : ImageDataType::EMPTY_MASK));
    newimg.SetSize(Width, Height);
    const auto& cvter = ColorConvertor::Get();
    switch (chLimit)
    {
    case 2:
        Ensures(channel == 0u);
        cvter.GrayAToGray(newimg.GetRawPtr<uint8_t>(), GetRawPtr<uint16_t>(), PixelCount());
        break;
    case 3:
        Ensures(channel < 3u);
        cvter.RGBGetChannel(newimg.GetRawPtr<uint8_t>(), GetRawPtr<uint8_t>(), PixelCount(), channel);
        break;
    case 4:
        Ensures(channel < 4u);
        cvter.RGBAGetChannel(newimg.GetRawPtr<uint8_t>(), GetRawPtr<uint32_t>(), PixelCount(), channel);
        break;
    default:
        break;
    }

    return newimg;
}


}

