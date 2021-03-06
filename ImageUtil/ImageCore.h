#pragma once

#include "ImageUtilRely.h"


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace xziar::img
{


//[has-alpha|float|RGB channel]
//[****7****|**6**|1.........0]
enum class ImageDataType : uint8_t
{
    ALPHA_MASK = 0x80, FLOAT_MASK = 0x40, EMPTY_MASK = 0x0, UNKNOWN_RESERVE = 0xff,
    RGB = 0, RGBA = RGB | ALPHA_MASK, RGBf = RGB | FLOAT_MASK, RGBAf = RGBA | FLOAT_MASK,
    BGR = 1, BGRA = BGR | ALPHA_MASK, BGRf = BGR | FLOAT_MASK, BGRAf = BGRA | FLOAT_MASK,
    GRAY = 2, RED = GRAY, GA = GRAY | ALPHA_MASK, RA = GA, GRAYf = GRAY | FLOAT_MASK, REDf = GRAYf, GAf = GA | FLOAT_MASK, RAf = GAf,
};
MAKE_ENUM_BITFIELD(ImageDataType)

class ImageView;
/*Custom Image Data Holder, with pixel data alignment promise*/
class IMGUTILAPI Image : protected common::AlignedBuffer
{
    friend class ImageView;
private:
    void ResetSize(const uint32_t width, const uint32_t height);
protected:
    uint32_t Width, Height;
    ImageDataType DataType;
    uint8_t ElementSize;
    void CheckSizeLegal() const
    {
        if (static_cast<size_t>(Width)* Height* ElementSize != Size)
            COMMON_THROW(common::BaseException, u"Size not match");
    }
public:
    [[nodiscard]] static constexpr uint8_t GetElementSize(const ImageDataType dataType) noexcept;
    Image(const ImageDataType dataType = ImageDataType::RGBA) noexcept : Width(0), Height(0), DataType(dataType), ElementSize(GetElementSize(DataType))
    { }
    Image(const common::AlignedBuffer& data, const uint32_t width, const uint32_t height, const ImageDataType dataType = ImageDataType::RGBA)
        : common::AlignedBuffer(data), Width(width), Height(height), DataType(dataType), ElementSize(GetElementSize(DataType))
    {
        CheckSizeLegal();
    }
    Image(common::AlignedBuffer&& data, const uint32_t width, const uint32_t height, const ImageDataType dataType = ImageDataType::RGBA)
        : common::AlignedBuffer(std::move(data)), Width(width), Height(height), DataType(dataType), ElementSize(GetElementSize(DataType))
    {
        CheckSizeLegal();
    }

    using common::AlignedBuffer::GetSize;
    using common::AlignedBuffer::AsSpan;
    [[nodiscard]] uint32_t      GetWidth()       const noexcept { return Width;       }
    [[nodiscard]] uint32_t      GetHeight()      const noexcept { return Height;      }
    [[nodiscard]] ImageDataType GetDataType()    const noexcept { return DataType;    }
    [[nodiscard]] uint8_t       GetElementSize() const noexcept { return ElementSize; }
    [[nodiscard]] size_t RowSize()    const noexcept { return static_cast<size_t>(Width) * ElementSize; }
    [[nodiscard]] size_t PixelCount() const noexcept { return static_cast<size_t>(Width) * Height; }
    void SetSize(const uint32_t width, const uint32_t height, const bool zero = true)
    {
        ResetSize(width, height);
        if (zero) Fill();
    }
    void SetSize(const uint32_t width, const uint32_t height, const std::byte fill)
    {
        ResetSize(width, height);
        Fill(fill);
    }
    template<typename T>
    void SetSize(const std::tuple<T, T>& size, const bool zero = true)
    {
        SetSize(static_cast<uint32_t>(std::get<0>(size)), static_cast<uint32_t>(std::get<1>(size)), zero);
    }
    [[nodiscard]] bool IsGray() const noexcept { return REMOVE_MASK(DataType, ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK) == ImageDataType::GRAY; }

    template<typename T = std::byte>
    [[nodiscard]] T* GetRawPtr(const uint32_t row = 0, const uint32_t col = 0) noexcept
    {
        return reinterpret_cast<T*>(Data + (static_cast<size_t>(row) * Width + col) * ElementSize);
    }
    template<typename T = std::byte>
    [[nodiscard]] const T* GetRawPtr(const uint32_t row = 0, const uint32_t col = 0) const noexcept
    {
        return reinterpret_cast<T*>(Data + (static_cast<size_t>(row) * Width + col) * ElementSize);
    }
    template<typename T = std::byte>
    [[nodiscard]] std::vector<T*> GetRowPtrs(const size_t offset = 0)
    {
        std::vector<T*> pointers(Height, nullptr);
        T *rawPtr = GetRawPtr<T>();
        const size_t lineStep = RowSize();
        for (auto& ptr : pointers)
            ptr = rawPtr + offset, rawPtr += lineStep;
        return pointers;
    }
    template<typename T = std::byte>
    [[nodiscard]] std::vector<const T*> GetRowPtrs(const size_t offset = 0) const
    {
        std::vector<const T*> pointers(Height, nullptr);
        const T *rawPtr = GetRawPtr<T>();
        const size_t lineStep = RowSize();
        for (auto& ptr : pointers)
            ptr = rawPtr + offset, rawPtr += lineStep;
        return pointers;
    }

    [[nodiscard]] const common::AlignedBuffer& GetData() const noexcept
    {
        return *static_cast<const common::AlignedBuffer*>(this);
    }
    [[nodiscard]] common::AlignedBuffer ExtractData() noexcept
    {
        common::AlignedBuffer data = std::move(*this);
        Width = Height = 0; ElementSize = 0;
        return data;
    }

    void FlipVertical();
    void FlipHorizontal();
    void Rotate180();
    [[nodiscard]] Image FlipToVertical() const;
    [[nodiscard]] Image FlipToHorizontal() const;
    [[nodiscard]] Image RotateTo180() const;
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

    ///<summary>Resize the image</summary>  
    ///<param name="width">width</param>
    ///<param name="height">height</param>
    Image ResizeTo(uint32_t width, uint32_t height, const bool isSRGB = false, const bool mulAlpha = true) const;
    [[nodiscard]] Image Region(const uint32_t x = 0, const uint32_t y = 0, uint32_t w = 0, uint32_t h = 0) const;
    [[nodiscard]] Image ConvertTo(const ImageDataType dataType, const uint32_t x = 0, const uint32_t y = 0, uint32_t w = 0, uint32_t h = 0) const;
    [[nodiscard]] Image ConvertToFloat(const float floatRange = 1) const;
};

class IMGUTILAPI ImageView : protected Image
{
    friend class Image;
    friend struct ::common::AlignBufLessor;
public:
    ImageView(const Image& image) : Image(image.CreateSubBuffer(0, image.Size), image.Width, image.Height, image.DataType) {}
    ImageView(const ImageView& imgview) : Image(imgview.CreateSubBuffer(0, imgview.Size), imgview.Width, imgview.Height, imgview.DataType) {}
    ImageView(Image&& image) noexcept : Image(std::move(image)) {}
    ImageView(ImageView&& imgview) noexcept : Image(std::move(static_cast<Image&&>(imgview))) {}
    ImageView& operator=(const Image& image)
    {
        *static_cast<common::AlignedBuffer*>(this) = image.CreateSubBuffer(0, image.Size);
        Width = image.Width; Height = image.Height; DataType = image.DataType; ElementSize = image.ElementSize;
        return *this;
    }
    ImageView& operator=(const ImageView& imgview)
    {
        *static_cast<common::AlignedBuffer*>(this) = imgview.CreateSubBuffer(0, imgview.Size);
        Width = imgview.Width; Height = imgview.Height; DataType = imgview.DataType; ElementSize = imgview.ElementSize;
        return *this;
    }
    ImageView& operator=(Image&& image) noexcept
    {
        *static_cast<common::AlignedBuffer*>(this) = static_cast<common::AlignedBuffer&&>(image);
        Width = image.Width; Height = image.Height; DataType = image.DataType; ElementSize = image.ElementSize;
        return *this;
    }
    ImageView& operator=(ImageView&& imgview) noexcept
    {
        *static_cast<common::AlignedBuffer*>(this) = static_cast<common::AlignedBuffer&&>(imgview);
        Width = imgview.Width; Height = imgview.Height; DataType = imgview.DataType; ElementSize = imgview.ElementSize;
        return *this;
    }
    [[nodiscard]] const Image& AsRawImage() const noexcept { return *static_cast<const Image*>(this); }
    template<typename T = std::byte>
    [[nodiscard]] constexpr common::span<const T> AsSpan() const noexcept 
    { 
        return common::span<const T>(GetRawPtr<T>(), Size / sizeof(T));
    }

    using Image::GetSize;
    using Image::GetWidth;
    using Image::GetHeight;
    using Image::GetDataType;
    using Image::GetElementSize;
    using Image::RowSize;
    using Image::PixelCount;
    using Image::GetData;
    using Image::FlipToVertical;
    using Image::FlipToHorizontal;
    using Image::RotateTo180;
    using Image::ResizeTo;
    using Image::Region;
    using Image::ConvertTo;
    using Image::ConvertToFloat;
    using Image::IsGray;
    using Image::ExtractData;
    template<typename T = std::byte>
    [[nodiscard]] const T* GetRawPtr(const uint32_t row = 0, const uint32_t col = 0) const noexcept
    {
        return reinterpret_cast<T*>(Data + (row * Width + col) * ElementSize);
    }
    template<typename T = std::byte>
    [[nodiscard]] std::vector<const T*> GetRowPtrs(const size_t offset = 0) const
    {
        std::vector<const T*> pointers(Height, nullptr);
        const T *rawPtr = GetRawPtr<T>();
        const size_t lineStep = RowSize();
        for (auto& ptr : pointers)
            ptr = rawPtr + offset, rawPtr += lineStep;
        return pointers;
    }
};

constexpr inline uint8_t Image::GetElementSize(const ImageDataType dataType) noexcept
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

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif