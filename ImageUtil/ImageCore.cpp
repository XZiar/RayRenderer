#include "ImageUtilRely.h"
#include "ImageCore.h"
#include "DataConvertor.hpp"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "3rdParty/stblib/stb_image_resize.h"

namespace xziar::img
{


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
        //not supported yet
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
        //not supported yet
    }
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
            return;
        if (REMOVE_MASK(DataType, { ImageDataType::ALPHA_MASK,ImageDataType::FLOAT_MASK }) == ImageDataType::GREY)//not supported yet
            return;

        const auto diff = DataType ^ src.DataType;
        auto pixcnt = copypix;
        if (isCopyWholeRow)
            pixcnt *= rowcnt, rowcnt = 1;
        if (diff == ImageDataType::ALPHA_MASK)//remove/add alpha only
        {
            if (ElementSize == 4)//add alpha, 3->4
            {
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    convert::RGBsToRGBAs(destPtr, srcPtr, pixcnt);
            }
            else if (ElementSize == 3)//remove alpha, 4->3
            {
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    convert::RGBAsToRGBs(destPtr, srcPtr, pixcnt);
            }
        }
        else if (ElementSize == 4)
        {
            switch (src.ElementSize)
            {
            case 1:
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    convert::GraysToRGBAs(destPtr, srcPtr, pixcnt);
                break;
            case 2:
                for (; rowcnt--; destPtr += destStep, srcPtr += srcStep)
                    convert::GrayAsToRGBAs(destPtr, srcPtr, pixcnt);
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
                    convert::GraysToRGBs(destPtr, srcPtr, pixcnt);
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

void Image::Resize(const uint32_t width, const uint32_t height)
{
    common::AlignedBuffer<32> output(width*height*ElementSize);

    const auto datatype = HAS_FIELD(DataType, ImageDataType::FLOAT_MASK) ? STBIR_TYPE_FLOAT : STBIR_TYPE_UINT8;
    const int32_t channel = HAS_FIELD(DataType, ImageDataType::FLOAT_MASK) ? ElementSize / sizeof(float) : ElementSize;
    const auto flag = HAS_FIELD(DataType, ImageDataType::ALPHA_MASK) ? 0 : STBIR_ALPHA_CHANNEL_NONE;
    stbir_resize(Data, (int32_t)Width, (int32_t)Height, 0, output.GetRawPtr(), (int32_t)width, (int32_t)height, 0,
        datatype, channel, ElementSize - 1, flag,
        STBIR_EDGE_REFLECT, STBIR_EDGE_REFLECT, STBIR_FILTER_TRIANGLE, STBIR_FILTER_TRIANGLE, STBIR_COLORSPACE_LINEAR, nullptr);

    *reinterpret_cast<common::AlignedBuffer<32>*>(this) = output;
    Width = width, Height = height;
}


Image Image::Region(const uint32_t x, const uint32_t y, uint32_t w, uint32_t h) const
{
    if (w == 0) w = Width;
    if (h == 0) h = Height;
    if (x == y == 0 && w == Width && h == Height)
        return *this;

    auto newimg = Image(DataType);
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

    auto newimg = Image(dataType);
    newimg.SetSize(w, h);
    newimg.PlaceImage(*this, x, y, 0, 0);
    return newimg;
}


}

