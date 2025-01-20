#pragma once

#include "ImageUtilRely.h"
#include "SystemCommon/FormatInclude.h"


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace xziar::img
{


struct ImgDType
{
    enum class Channels : uint8_t { R = 0b000, RA = 0b001, RGB = 0b010, RGBA = 0b011, BGR = 0b110, BGRA = 0b111 };
    enum class DataTypes : uint8_t { Composed = 0, Uint8 = 1, Uint16 = 2, Fixed16 = 3, Float16 = 4, Float32 = 5 };

    static constexpr bool IsBGROrder(Channels ch) noexcept { return (::common::enum_cast(ch) & 0b100); }
    static constexpr uint32_t DataTypeToSize(DataTypes type) noexcept
    {
        switch (type)
        {
        case DataTypes::Uint8:   return 1;
        case DataTypes::Uint16:  return 2;
        case DataTypes::Fixed16: return 2;
        case DataTypes::Float16: return 2;
        case DataTypes::Float32: return 4;
        default:                 return 0;
        }
    }
    static constexpr bool IsFloat(DataTypes type) noexcept
    {
        switch (type)
        {
        case DataTypes::Float16: return true;
        case DataTypes::Float32: return true;
        default:                 return false;
        }
    }
    IMGUTILAPI static std::string_view Stringify(Channels ch) noexcept;
    IMGUTILAPI static std::string_view Stringify(DataTypes type) noexcept;
    IMGUTILAPI static std::string Stringify(const ImgDType& type, bool detail) noexcept;

    //[dtype|rev|channel]
    //[6...3|*2*|1.....0]
    uint8_t Value;
    constexpr ImgDType() noexcept : Value(0) {}
    explicit constexpr ImgDType(uint8_t value) noexcept : Value(value) {}
    explicit constexpr ImgDType(Channels ch, DataTypes dt) noexcept : Value{ static_cast<uint8_t>(::common::enum_cast(ch) | (::common::enum_cast(dt) << 3)) } {}
    explicit operator bool() const noexcept { return Value != 0; }
    constexpr bool operator==(const ImgDType& rhs) const noexcept { return Value == rhs.Value; }
    constexpr bool operator!=(const ImgDType& rhs) const noexcept { return Value != rhs.Value; }
    constexpr Channels Channel() const noexcept { return static_cast<Channels>(Value & 0b111); }
    constexpr DataTypes DataType() const noexcept { return static_cast<DataTypes>((Value >> 3) & 0b111); }
    constexpr bool HasAlpha() const noexcept { return (Value & 0b1) == 1; /*ChannelCount() % 2 == 0*/ }
    constexpr uint32_t ChannelCount() const noexcept { return (Value & 0b11) + 1; }
    constexpr uint32_t DataTypeSize() const noexcept { return DataTypeToSize(DataType()); }
    constexpr uint32_t ElementSize() const noexcept
    {
        if (DataType() != DataTypes::Composed) return DataTypeSize() * ChannelCount();
        return 0;
    }
    constexpr bool IsFloat() const noexcept { return IsFloat(DataType()); }
    constexpr bool IsBGROrder() const noexcept { return IsBGROrder(Channel()); }
    constexpr bool Is(Channels ch) const noexcept { return Channel() == ch; }
    constexpr bool Is(DataTypes dt) const noexcept { return DataType() == dt; }
    std::string ToString() const noexcept
    {
        auto ret = Stringify(*this, false);
        if (ret.empty()) return Stringify(*this, true);
        return ret;
    }
    IMGUTILAPI void FormatWith(common::str::FormatterHost& host, common::str::FormatterContext& context, const common::str::FormatSpec* spec) const;

    constexpr ImgDType& SetDatatype(DataTypes dt) noexcept
    {
        *this = ImgDType(Channel(), dt);
        return *this;
    }
    constexpr ImgDType& SetChannels(Channels ch) noexcept
    {
        *this = ImgDType(ch, DataType());
        return *this;
    }
    constexpr ImgDType& SetChannelCount(uint8_t count) noexcept
    {
        Expects(count > 0 && count < 5);
        Value = static_cast<uint8_t>((Value & 0xfc) | (count - 1));
        return *this;
    }
};
namespace ImageDataType
{
static_assert(std::is_trivially_copyable_v<ImgDType>);
#define IMG_DT(name, ch, dt) inline constexpr ImgDType name = ImgDType(ImgDType::Channels::ch, ImgDType::DataTypes::dt)
IMG_DT(RGBA,    RGBA, Uint8);
IMG_DT(BGRA,    BGRA, Uint8);
IMG_DT(RGB,     RGB,  Uint8);
IMG_DT(BGR,     BGR,  Uint8);
IMG_DT(GA,      RA,   Uint8);
IMG_DT(RA,      RA,   Uint8);
IMG_DT(GRAY,    R,    Uint8);
IMG_DT(RED,     R,    Uint8);
IMG_DT(RGBA16,  RGBA, Uint16);
IMG_DT(BGRA16,  BGRA, Uint16);
IMG_DT(RGB16,   RGB,  Uint16);
IMG_DT(BGR16,   BGR,  Uint16);
IMG_DT(GA16,    RA,   Uint16);
IMG_DT(RA16,    RA,   Uint16);
IMG_DT(GRAY16,  R,    Uint16);
IMG_DT(RED16,   R,    Uint16);
IMG_DT(RGBAf,   RGBA, Float32);
IMG_DT(BGRAf,   BGRA, Float32);
IMG_DT(RGBf,    RGB,  Float32);
IMG_DT(BGRf,    BGR,  Float32);
IMG_DT(GAf,     RA,   Float32);
IMG_DT(RAf,     RA,   Float32);
IMG_DT(GRAYf,   R,    Float32);
IMG_DT(REDf,    R,    Float32);
IMG_DT(RGBAh,   RGBA, Float16);
IMG_DT(BGRAh,   BGRA, Float16);
IMG_DT(RGBh,    RGB,  Float16);
IMG_DT(BGRh,    BGR,  Float16);
IMG_DT(GAh,     RA,   Float16);
IMG_DT(RAh,     RA,   Float16);
IMG_DT(GRAYh,   R,    Float16);
IMG_DT(REDh,    R,    Float16);
#undef IMG_DT
}


class ImageView;
/*Custom Image Data Holder, with pixel data alignment promise*/
class IMGUTILAPI Image : protected common::AlignedBuffer
{
    friend class ImageView;
private:
    void ResetSize(const uint32_t width, const uint32_t height);
protected:
    uint32_t Width, Height;
    ImgDType DataType;
    uint8_t ElementSize;
    void CheckSizeLegal() const
    {
        if (static_cast<size_t>(Width) * Height * ElementSize != Size)
            COMMON_THROW(common::BaseException, u"Size not match");
    }
public:
    [[nodiscard]] static constexpr uint8_t GetElementSize(const ImgDType dataType) noexcept { return static_cast<uint8_t>(dataType.ElementSize()); }
    [[nodiscard]] static Image CombineChannels(const ImgDType dataType, common::span<const ImageView> channels);
    [[nodiscard]] static Image CreateViewFromTemp(common::span<std::byte> space, const ImgDType dataType, uint32_t w, uint32_t h);

    Image(const ImgDType dataType = ImageDataType::RGBA) noexcept : Width(0), Height(0), DataType(dataType), ElementSize(GetElementSize(DataType))
    { }
    Image(const common::AlignedBuffer& data, const uint32_t width, const uint32_t height, const ImgDType dataType = ImageDataType::RGBA)
        : common::AlignedBuffer(data), Width(width), Height(height), DataType(dataType), ElementSize(GetElementSize(DataType))
    {
        CheckSizeLegal();
    }
    Image(common::AlignedBuffer&& data, const uint32_t width, const uint32_t height, const ImgDType dataType = ImageDataType::RGBA)
        : common::AlignedBuffer(std::move(data)), Width(width), Height(height), DataType(dataType), ElementSize(GetElementSize(DataType))
    {
        CheckSizeLegal();
    }

    using common::AlignedBuffer::GetSize;
    using common::AlignedBuffer::AsSpan;
    [[nodiscard]] uint32_t GetWidth()       const noexcept { return Width;       }
    [[nodiscard]] uint32_t GetHeight()      const noexcept { return Height;      }
    [[nodiscard]] ImgDType GetDataType()    const noexcept { return DataType;    }
    [[nodiscard]] uint8_t  GetElementSize() const noexcept { return ElementSize; }
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
    [[nodiscard]] constexpr bool IsGray() const noexcept { return DataType.Channel() == ImgDType::Channels::R; }

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
    void PlaceImage(ImageView other, const uint32_t srcX, const uint32_t srcY, const uint32_t destX, const uint32_t destY);
    ///<summary>Resize the image in-place</summary>  
    ///<param name="width">width</param>
    ///<param name="height">height</param>
    void Resize(uint32_t width, uint32_t height, const bool isSRGB = false, const bool mulAlpha = true)
    {
        *this = ResizeTo(width, height, isSRGB, mulAlpha);
    }

    ///<summary>Resize the image</summary>  
    ///<param name="width">width</param>
    ///<param name="height">height</param>
    Image ResizeTo(uint32_t width, uint32_t height, const bool isSRGB = false, const bool mulAlpha = true) const;
    void ResizeTo(Image& dst, const uint32_t srcX, const uint32_t srcY, const uint32_t destX, const uint32_t destY, uint32_t width = 0, uint32_t height = 0, const bool isSRGB = false, const bool mulAlpha = true) const;
    [[nodiscard]] Image Region(const uint32_t x = 0, const uint32_t y = 0, uint32_t w = 0, uint32_t h = 0) const;
    [[nodiscard]] Image ConvertTo(const ImgDType dataType, const uint32_t x = 0, const uint32_t y = 0, uint32_t w = 0, uint32_t h = 0) const;
    [[nodiscard]] Image ConvertFloat(const ImgDType::DataTypes dataType, const float floatRange = 1) const;
    ///<summary>Pick single channel from image</summary>  
    ///<param name="channel">channel</param>
    [[nodiscard]] Image ExtractChannel(uint8_t channel) const;
    [[nodiscard]] std::vector<Image> ExtractChannels() const;
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
    using Image::ConvertFloat;
    using Image::IsGray;
    using Image::ExtractData;
    using Image::ExtractChannel;
    using Image::ExtractChannels;
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

}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif