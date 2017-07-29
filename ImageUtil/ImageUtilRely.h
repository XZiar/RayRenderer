#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <filesystem>
#include "common/CommonRely.hpp"

namespace xziar::img
{

namespace fs = std::experimental::filesystem;
using std::string;
using std::wstring;

enum class ImageType : uint8_t
{
	JPEG, PNG, TGA, BMP
};

enum class ImageDataType : uint8_t
{
	RGB, RGBA, BGR, GREY
};


class Image
{
public:
	static constexpr uint8_t GetElementSize(const ImageDataType dataType);
private:
	std::vector<uint8_t> Data;
public:
	uint32_t Width, Height;
	ImageType Type = ImageType::BMP;
	const ImageDataType DataType;
	const uint8_t ElementSize;
	Image(const ImageDataType dataType = ImageDataType::RGBA) noexcept : DataType(dataType), ElementSize(GetElementSize(DataType)) {}
	void SetSize(const uint32_t width, const uint32_t height)
	{
		Width = width, Height = height;
		Data.resize(width * height * ElementSize);
	}

	std::vector<uint8_t>& GetRaw() noexcept { return Data; }
	const std::vector<uint8_t>& GetRaw() const noexcept { return Data; }
	uint8_t* GetRawPtr(const uint32_t row = 0, const uint32_t colum = 0) noexcept 
	{ 
		return Data.data() + (row * Width + colum) * ElementSize;
	}
	const uint8_t* GetRawPtr(const uint32_t row = 0, const uint32_t colum = 0) const noexcept
	{
		return Data.data() + (row * Width + colum) * ElementSize;
	}
};

constexpr uint8_t Image::GetElementSize(const ImageDataType dataType)
{
	switch (dataType)
	{
	case ImageDataType::BGR:
	case ImageDataType::RGB:
		return 3;
	case ImageDataType::RGBA:
		return 4;
	case ImageDataType::GREY:
		return 1;
	default:
		return 0;
	}
}

}