#include "ImageUtilPch.h"
#include "ImageCore.h"
#include "ColorConvert.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"


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
        if (diff == ImageDataType::ALPHA_MASK)//remove/add alpha only
        {
            switch (src.ElementSize)
            {
            case 4://remove alpha, 4->3
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    convert::RGBAsToRGBs(destPtr, srcPtr, pixcnt);
                break;
            case 3://add alpha, 3->4
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    convert::RGBsToRGBAs(destPtr, srcPtr, pixcnt);
                break;
            case 2://remove alpha, 2->1
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    ColorConvertor::Get().GrayAToGray(reinterpret_cast<uint8_t*>(destPtr), reinterpret_cast<const uint16_t*>(srcPtr), pixcnt);
                    //convert::GrayAsToGrays(destPtr, srcPtr, pixcnt);
                break;
            case 1://add alpha, 1->2
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    ColorConvertor::Get().GrayToGrayA(reinterpret_cast<uint16_t*>(destPtr), reinterpret_cast<const uint8_t*>(srcPtr), pixcnt);
                    //convert::GraysToGrayAs(destPtr, srcPtr, pixcnt);
                break;
            }
        }
        else if (ElementSize == 4)
        {
            switch (src.ElementSize)
            {
            case 1:
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    ColorConvertor::Get().GrayToRGBA(reinterpret_cast<uint32_t*>(destPtr), reinterpret_cast<const uint8_t*>(srcPtr), pixcnt);
                    //convert::GraysToRGBAs(destPtr, srcPtr, pixcnt);
                break;
            case 2:
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    ColorConvertor::Get().GrayAToRGBA(reinterpret_cast<uint32_t*>(destPtr), reinterpret_cast<const uint16_t*>(srcPtr), pixcnt);
                    //convert::GrayAsToRGBAs(destPtr, srcPtr, pixcnt);
                break;
            case 3://change byte-order and add alpha(plain-add, see above) 
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    convert::RGBsToBGRAs(destPtr, srcPtr, pixcnt);
                break;
            case 4://change byte-order only(plain copy, see above*2)
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    convert::BGRAsToRGBAs(destPtr, srcPtr, pixcnt);
                break;
            }
        }
        else if (ElementSize == 3)
        {
            switch (src.ElementSize)
            {
            case 1:
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    ColorConvertor::Get().GrayToRGB(reinterpret_cast<uint8_t*>(destPtr), reinterpret_cast<const uint8_t*>(srcPtr), pixcnt);
                    //convert::GraysToRGBs(destPtr, srcPtr, pixcnt);
                break;
            case 2:
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    convert::GrayAsToRGBs(destPtr, srcPtr, pixcnt);
                break;
            case 3://change byte-order only(plain copy, see above*2)
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    convert::BGRsToRGBs(destPtr, srcPtr, pixcnt);
                break;
            case 4://change byte-order and remove alpha(plain-add, see above) 
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    convert::RGBAsToBGRs(destPtr, srcPtr, pixcnt);
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
    width = width == 0 ? (uint32_t)((uint64_t)height * Width / Height) : width;
    height = height == 0 ? (uint32_t)((uint64_t)width * Height / Width) : height;
    Image output(DataType);
    output.SetSize(width, height);

    const auto datatype = HAS_FIELD(DataType, ImageDataType::FLOAT_MASK) ? STBIR_TYPE_FLOAT : STBIR_TYPE_UINT8;
    const int32_t channel = HAS_FIELD(DataType, ImageDataType::FLOAT_MASK) ? ElementSize / sizeof(float) : ElementSize;
    int flag = 0;
    if (HAS_FIELD(DataType, ImageDataType::ALPHA_MASK))
    {
        if (!mulAlpha)
            flag |= STBIR_FLAG_ALPHA_PREMULTIPLIED;
    }
    else
        flag |= STBIR_ALPHA_CHANNEL_NONE;
    stbir_resize(Data, (int32_t)Width, (int32_t)Height, 0, output.GetRawPtr(), (int32_t)width, (int32_t)height, 0,
        datatype, channel, ElementSize - 1, flag,
        STBIR_EDGE_REFLECT, STBIR_EDGE_REFLECT, STBIR_FILTER_TRIANGLE, STBIR_FILTER_TRIANGLE,
        isSRGB ? STBIR_COLORSPACE_SRGB : STBIR_COLORSPACE_LINEAR, nullptr);

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
        // convert::U8sToFloat1s(reinterpret_cast<float*>(newimg.Data), Data, Size, floatRange);
        return newimg;
    }
    else
    {
        Image newimg(REMOVE_MASK(DataType, ImageDataType::FLOAT_MASK));
        newimg.SetSize(Width, Height);
        const auto floatCount = Size / sizeof(float);
        common::CopyEx.CopyFromFloat(reinterpret_cast<uint8_t*>(newimg.Data), reinterpret_cast<const float*>(Data),
            Size, floatRange);
        //convert::Float1sToU8s(newimg.Data, reinterpret_cast<const float*>(Data), floatCount, floatRange);
        return newimg;
    }
}

}

