#pragma once

#include "ImageUtilRely.h"

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
class IMGUTILAPI Image
{
public:
	static constexpr uint8_t GetElementSize(const ImageDataType dataType);
private:
	byte *Data = nullptr;
	void Alloc(const bool zero = false)
	{
		Release();
        const auto size = Size();
		Data = (byte*)malloc_align(size, 32);
        if (zero)
            memset(Data, 0x0, size);
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
        const auto size = Size();
        memcpy_s(Data, size, other.Data, size);
	}
	Image(Image&& other) : Width(other.Width), Height(other.Height), Type(other.Type), DataType(other.DataType), ElementSize(other.ElementSize), Data(other.Data)
	{
		other.Data = nullptr;
	}
	~Image() { Release(); }

	size_t Size() const { return Width * Height * ElementSize; }
    size_t RowSize() const { return Width * ElementSize; }
    size_t PixelCount() const { return Width * Height; }
	void SetSize(const uint32_t width, const uint32_t height, const bool zero = true)
	{
		Width = width, Height = height;
		Alloc(zero);
	}
	template<typename T>
	void SetSize(const tuple<T, T>& size)
	{
		SetSize(std::get<0>(size), std::get<1>(size));
	}

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
	case ImageDataType::GREY:
		return 1 * baseSize;
	default:
		return 0;
	}
}


}