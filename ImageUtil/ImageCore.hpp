#pragma once

#include "ImageUtilRely.h"
#include "DataConvertor.hpp"

namespace xziar::img
{

enum class ImageType : uint8_t
{
	JPEG, PNG, TGA, BMP
};

enum class ImageDataType : uint8_t
{
	ALPHA_MASK = 0x80, FLOAT_MASK = 0x40,
	RGB = 0, RGBA = 0x80, RGBf = 0x40, RGBAf = 0xA0,
	BGR = 1, BGRA = 0x81, BGRf = 0x41, BGRAf = 0xA1,
	GREY = 2, RED = 2, GA = 0x82, RA = 0x82,
	GREYf = 0x22, REDf = 0x22, GAf = 0xA2, RAf = 0xA2
};
MAKE_ENUM_BITFIELD(ImageDataType)


/*Custom Image Data Holder, with pixel data alignment promise*/
class Image
{
public:
	static constexpr uint8_t GetElementSize(const ImageDataType dataType);
private:
	uint8_t *Data = nullptr;
	void Alloc()
	{
		Release();
		Data = (uint8_t*)malloc_align(Width * Height * ElementSize, 32);
	}
	void Release()
	{
		if (Data) free_align(Data);
		Data = nullptr;
	};
public:
	uint32_t Width, Height;
	ImageType Type = ImageType::BMP;
	const ImageDataType DataType;
	const uint8_t ElementSize;
	Image(const ImageDataType dataType = ImageDataType::RGBA) noexcept : DataType(dataType), ElementSize(GetElementSize(DataType)) {}
	Image(const Image& other) : Width(other.Width), Height(other.Height), Type(other.Type), DataType(other.DataType), ElementSize(other.ElementSize)
	{
		Alloc();
		memcpy(Data, other.Data, Size());
	}
	Image(Image&& other) : Width(other.Width), Height(other.Height), Type(other.Type), DataType(other.DataType), ElementSize(other.ElementSize), Data(other.Data)
	{
		other.Data = nullptr;
	}
	~Image() { Release(); }
	size_t Size() const { return Width * Height * ElementSize; }
	void SetSize(const uint32_t width, const uint32_t height)
	{
		Width = width, Height = height;
		Alloc();
	}

	uint8_t* GetRawPtr(const uint32_t row = 0, const uint32_t colum = 0) noexcept
	{
		return Data + (row * Width + colum) * ElementSize;
	}
	const uint8_t* GetRawPtr(const uint32_t row = 0, const uint32_t colum = 0) const noexcept
	{
		return Data + (row * Width + colum) * ElementSize;
	}

	void FlipVertical();
	void FlipHorizontal();
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
	case ImageDataType::GREY:
		return 1 * baseSize;
	default:
		return 0;
	}
}

inline void Image::FlipVertical()
{
	const auto lineStep = Width * ElementSize;
	uint8_t* rowUp = Data, *rowDown = Data + (Height - 1) * lineStep;
	while (rowUp < rowDown)
	{
		convert::Swap2Buffer(rowUp, rowDown, lineStep);
		rowUp += lineStep, rowDown -= lineStep;
	}
}

inline void Image::FlipHorizontal()
{

}

}