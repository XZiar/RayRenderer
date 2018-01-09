#pragma once

#include "ImageUtilRely.h"

namespace xziar::img
{

using common::BaseException;

enum class ImageType : uint8_t
{
	JPEG, PNG, TGA, BMP
};

enum class ImageDataType : uint8_t
{
	ALPHA_MASK = 0x80, FLOAT_MASK = 0x40, EMPTY_MASK = 0x0,
	RGB = 0, RGBA = 0x80, RGBf = 0x40, RGBAf = 0xA0,
	BGR = 1, BGRA = 0x81, BGRf = 0x41, BGRAf = 0xA1,
	GRAY = 2, RED = 2, GA = 0x82, RA = 0x82,
	GRAYf = 0x22, REDf = 0x22, GAf = 0xA2, RAf = 0xA2
};
MAKE_ENUM_BITFIELD(ImageDataType)

/*Custom Image Data Holder, with pixel data alignment promise*/
class IMGUTILAPI Image : protected common::AlignedBuffer<32>
{
public:
    static constexpr uint8_t GetElementSize(const ImageDataType dataType);
    uint32_t Width, Height;
	ImageType Type = ImageType::BMP;
	const ImageDataType DataType;
	const uint8_t ElementSize;
    Image(const ImageDataType dataType = ImageDataType::RGBA) noexcept : DataType(dataType), ElementSize(GetElementSize(DataType))
    { }
	Image(const Image& other) : common::AlignedBuffer<32>(other), Width(other.Width), Height(other.Height), Type(other.Type), DataType(other.DataType), ElementSize(other.ElementSize)
	{ }
	Image(Image&& other) : common::AlignedBuffer<32>(other), Width(other.Width), Height(other.Height), Type(other.Type), DataType(other.DataType), ElementSize(other.ElementSize)
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
    //bool isGray() const { return REMOVE_MASK(DataType, { ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK }) == ImageDataType::GRAY; }
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

	void FlipVertical();
	void FlipHorizontal();
    void Rotate180();
    void PlaceImage(const Image& other, const uint32_t srcX, const uint32_t srcY, const uint32_t destX, const uint32_t destY);
    void Resize(const uint32_t width, const uint32_t height);

    Image Region(const uint32_t x = 0, const uint32_t y = 0, uint32_t w = 0, uint32_t h = 0) const;
    Image ConvertTo(const ImageDataType dataType, const uint32_t x = 0, const uint32_t y = 0, uint32_t w = 0, uint32_t h = 0) const;
};

constexpr inline uint8_t Image::GetElementSize(const ImageDataType dataType)
{
	const uint8_t baseSize = HAS_FIELD(dataType, ImageDataType::FLOAT_MASK) ? 4 : 1;
	switch (REMOVE_MASK(dataType, { ImageDataType::FLOAT_MASK }))
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