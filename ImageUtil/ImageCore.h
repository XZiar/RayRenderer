#pragma once

#include "ImageUtilRely.h"

namespace xziar::img
{

using common::BaseException;

enum class ImageDataType : uint8_t
{
    ALPHA_MASK = 0x80, FLOAT_MASK = 0x40, EMPTY_MASK = 0x0, UNKNOWN_RESERVE = 0xff,
    RGB = 0, RGBA = RGB | ALPHA_MASK, RGBf = RGB | FLOAT_MASK, RGBAf = RGBA | FLOAT_MASK,
    BGR = 1, BGRA = BGR | ALPHA_MASK, BGRf = BGR | FLOAT_MASK, BGRAf = BGRA | FLOAT_MASK,
    GRAY = 2, RED = GRAY, GA = GRAY | ALPHA_MASK, RA = GA, GRAYf = GRAY | FLOAT_MASK, REDf = GRAYf, GAf = GA | FLOAT_MASK, RAf = GAf,
};
MAKE_ENUM_BITFIELD(ImageDataType)

/*Custom Image Data Holder, with pixel data alignment promise*/
class IMGUTILAPI Image : protected common::AlignedBuffer<32>
{
public:
    static constexpr uint8_t GetElementSize(const ImageDataType dataType);
    uint32_t Width, Height;
    const ImageDataType DataType;
    const uint8_t ElementSize;
    Image(const ImageDataType dataType = ImageDataType::RGBA) noexcept : Width(0), Height(0), DataType(dataType), ElementSize(GetElementSize(DataType))
    { }
    Image(const Image& other) : common::AlignedBuffer<32>(other), Width(other.Width), Height(other.Height), DataType(other.DataType), ElementSize(other.ElementSize)
    { }
    Image(Image&& other) noexcept : common::AlignedBuffer<32>(other), Width(other.Width), Height(other.Height), DataType(other.DataType), ElementSize(other.ElementSize)
    { }
    Image(const common::AlignedBuffer<32>& data, const uint32_t width, const uint32_t height, const ImageDataType dataType = ImageDataType::RGBA)
        : common::AlignedBuffer<32>(data), Width(width), Height(height), DataType(dataType), ElementSize(GetElementSize(DataType))
    {
        if (Width*Height*ElementSize != Size_)
            COMMON_THROW(BaseException, L"Size not match");
    }
    Image(common::AlignedBuffer<32>&& data, const uint32_t width, const uint32_t height, const ImageDataType dataType = ImageDataType::RGBA)
        : common::AlignedBuffer<32>(data), Width(width), Height(height), DataType(dataType), ElementSize(GetElementSize(DataType))
    {
        if (Width*Height*ElementSize != Size_)
            COMMON_THROW(BaseException, L"Size not match");
    }

    using common::AlignedBuffer<32>::GetSize;
    size_t RowSize() const noexcept { return Width * ElementSize; }
    size_t PixelCount() const noexcept { return Width * Height; }
    void SetSize(const uint32_t width, const uint32_t height, const bool zero = true)
    {
        Width = width, Height = height, Size_ = Width * Height * ElementSize;
        Alloc(zero);
    }
    void SetSize(const uint32_t width, const uint32_t height, const byte fill)
    {
        Width = width, Height = height, Size_ = Width * Height * ElementSize;
        Alloc(false);
        memset(Data, std::to_integer<uint8_t>(fill), Size_);
    }
    template<typename T>
    void SetSize(const tuple<T, T>& size, const bool zero = true)
    {
        SetSize(std::get<0>(size), std::get<1>(size), zero);
    }
    bool isGray() const { return REMOVE_MASK(DataType, ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK) == ImageDataType::GRAY; }

    template<typename T = byte>
    T* GetRawPtr(const uint32_t row = 0, const uint32_t col = 0) noexcept
    {
        return reinterpret_cast<T*>(Data + (row * Width + col) * ElementSize);
    }
    template<typename T = byte>
    const T* GetRawPtr(const uint32_t row = 0, const uint32_t col = 0) const noexcept
    {
        return reinterpret_cast<T*>(Data + (row * Width + col) * ElementSize);
    }
    template<typename T = byte>
    std::vector<T*> GetRowPtrs(const size_t offset = 0)
    {
        std::vector<T*> pointers(Height, nullptr);
        T *rawPtr = GetRawPtr<T>();
        const size_t lineStep = RowSize();
        for (auto& ptr : pointers)
            ptr = rawPtr + offset, rawPtr += lineStep;
        return pointers;
    }
    template<typename T = byte>
    std::vector<const T*> GetRowPtrs(const size_t offset = 0) const
    {
        std::vector<const T*> pointers(Height, nullptr);
        const T *rawPtr = GetRawPtr<T>();
        const size_t lineStep = RowSize();
        for (auto& ptr : pointers)
            ptr = rawPtr + offset, rawPtr += lineStep;
        return pointers;
    }

    common::AlignedBuffer<32> ExtractData()
    {
        common::AlignedBuffer<32> data = std::move(*this);
        Width = Height = 0;
        return data;
    }

    void FlipVertical();
    void FlipHorizontal();
    void Rotate180();
    Image FlipToVertical() const;
    Image FlipToHorizontal() const;
    Image RotateTo180() const;
    ///<summary>Place other image into current image</summary>  
    ///<param name="other">image being putted</param>
    ///<param name="srcX">image source's left-top position</param>
    ///<param name="srcY">image source's left-top position</param>
    ///<param name="destX">image destination's left-top position</param>
    ///<param name="destY">image destination's left-top position</param>
    void PlaceImage(const Image& other, const uint32_t srcX, const uint32_t srcY, const uint32_t destX, const uint32_t destY);
    ///<summary>Resize the image in-place</summary>  
    ///<param name="width">width</param>
    ///<param name="height">height</param>
    void Resize(uint32_t width, uint32_t height, const bool isSRGB = false, const bool mulAlpha = true);

    Image Region(const uint32_t x = 0, const uint32_t y = 0, uint32_t w = 0, uint32_t h = 0) const;
    Image ConvertTo(const ImageDataType dataType, const uint32_t x = 0, const uint32_t y = 0, uint32_t w = 0, uint32_t h = 0) const;
    Image ConvertFloat(const float floatRange = 1) const;
};

constexpr inline uint8_t Image::GetElementSize(const ImageDataType dataType)
{
    const uint8_t baseSize = HAS_FIELD(dataType, ImageDataType::FLOAT_MASK) ? 4 : 1;
    switch (REMOVE_MASK(dataType, ImageDataType::FLOAT_MASK))
    {
    case ImageDataType::BGR:
    case ImageDataType::RGB:
        return 3 * baseSize;
    case ImageDataType::BGRA:
    case ImageDataType::RGBA:
        return 4 * baseSize;
    case ImageDataType::GA:
        return 2 * baseSize;
    case ImageDataType::GRAY:
        return 1 * baseSize;
    default:
        return 0;
    }
}


}