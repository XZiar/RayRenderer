#include "ImageUtilRely.h"
#include "ImageTGA.h"
#include "DataConvertor.hpp"

namespace xziar::img::tga
{


using BGR16ToRGBAMap = std::vector<uint32_t>;
using RGB16ToRGBAMap = BGR16ToRGBAMap;
static BGR16ToRGBAMap GenerateBGR16ToRGBAMap()
{
	BGR16ToRGBAMap map(1 << 16);
	constexpr uint32_t COUNT = 1 << 5, STEP = 256 / COUNT, HALF_SIZE = 1 << 15;
	constexpr uint32_t RED_STEP = STEP, GREEN_STEP = STEP << 8, BLUE_STEP = STEP << 16;
	uint32_t idx = 0;
	uint32_t color = 0;
	for (uint32_t red = COUNT, colorR = color; red--; colorR += RED_STEP)
	{
		for (uint32_t green = COUNT, colorRG = colorR; green--; colorRG += GREEN_STEP)
		{
			for (uint32_t blue = COUNT, colorRGB = colorRG; blue--; colorRGB += BLUE_STEP)
			{
				map[idx++] = colorRGB;
			}
		}
	}
	for (uint32_t count = 0; count < HALF_SIZE;)//protential 4k-alignment issue
		map[idx++] = map[count++] | 0xff000000;
	return map;
}
static const BGR16ToRGBAMap& GetBGR16ToRGBAMap()
{
	static const auto map = GenerateBGR16ToRGBAMap();
	return map;
}
static RGB16ToRGBAMap GenerateRGB16ToRGBAMap()
{
	RGB16ToRGBAMap map = GetBGR16ToRGBAMap();
	convert::BGRAsToRGBAs(reinterpret_cast<uint8_t*>(map.data()), map.size());
	return map;
}
static const BGR16ToRGBAMap& GetRGB16ToRGBAMap()
{
	static const auto map = GenerateRGB16ToRGBAMap();
	return map;
}


TgaReader::TgaReader(FileObject& file) : ImgFile(file)
{
}

bool TgaReader::Validate()
{
	using detail::TGAImgType;
	ImgFile.Rewind();
	if (!ImgFile.Read(Header))
		return false;
	switch (Header.ColorMapType)
	{
	case 1://paletted image
		if (REMOVE_MASK(Header.ImageType, { TGAImgType::RLE_MASK }) != TGAImgType::COLOR_MAP)
			return false;
		if (Header.PixelDepth != 8 && Header.PixelDepth != 16)
			return false;
		break;
	case 0://color image
		if (!MATCH_ANY(REMOVE_MASK(Header.ImageType, { TGAImgType::RLE_MASK }), { TGAImgType::COLOR, TGAImgType::GREY }))
			return false;
		if (Header.PixelDepth != 8 && Header.PixelDepth != 15 && Header.PixelDepth != 16 && Header.PixelDepth != 24 && Header.PixelDepth != 32)
			return false;
		if (Header.PixelDepth == 8)//not support
			return false;
		break;
	default:
		return false;
	}
	Width = (int16_t)convert::ParseWordLE(Header.Width);
	Height = (int16_t)convert::ParseWordLE(Header.Height);
	if (Width < 1 || Height < 1)
		return false;
	return true;
}


class TgaHelper
{
public:
	template<typename ReadFunc>
	static void ReadColorData4(const uint8_t colorDepth, const uint64_t count, Image& output, const bool isOutputRGB, ReadFunc& reader)
	{
		if (output.ElementSize != 4)
			return;
		auto& color16Map = isOutputRGB ? GetBGR16ToRGBAMap() : GetRGB16ToRGBAMap();
		switch (colorDepth)
		{
		case 15:
			{
				std::vector<uint16_t> tmp(count);
				reader.Read(count, tmp);
				uint32_t * __restrict destPtr = reinterpret_cast<uint32_t*>(output.GetRawPtr());
				for (auto bgr15 : tmp)
					*destPtr++ = color16Map[bgr15 + (1 << 15)];
			}break;
		case 16:
			{
				std::vector<uint16_t> tmp(count);
				reader.Read(count, tmp);
				uint32_t * __restrict destPtr = reinterpret_cast<uint32_t*>(output.GetRawPtr());
				for (auto bgr16 : tmp)
					*destPtr++ = color16Map[bgr16];
			}break;
		case 24://BGR
			{
				uint8_t * __restrict destPtr = output.GetRawPtr();
				uint8_t * __restrict srcPtr = destPtr + count;
				reader.Read(3 * count, srcPtr);
				if (isOutputRGB)
					convert::BGRsToRGBAs(destPtr, srcPtr, count);
				else
					convert::RGBsToRGBAs(destPtr, srcPtr, count);
			}break;
		case 32:
			{//BGRA
				reader.Read(4 * count, output.GetRawPtr());
				if (isOutputRGB)
					convert::BGRAsToRGBAs(output.GetRawPtr(), count);
			}break;
		}
	}

	template<typename ReadFunc>
	static void ReadColorData3(const uint8_t colorDepth, const uint64_t count, Image& output, const bool isOutputRGB, ReadFunc& reader)
	{
		if (output.ElementSize != 3)
			return;
		auto& color16Map = isOutputRGB ? GetBGR16ToRGBAMap() : GetRGB16ToRGBAMap();
		switch (colorDepth)
		{
		case 15:
		case 16:
			{
				std::vector<uint16_t> tmp(count);
				reader.Read(count, tmp);
				uint8_t * __restrict destPtr = output.GetRawPtr();
				for (auto bgr15 : tmp)
				{
					const uint32_t color = color16Map[bgr15 + (1 << 15)];//ignore alpha
					const uint8_t* __restrict colorPtr = reinterpret_cast<const uint8_t*>(&color);
					*destPtr++ = colorPtr[0];
					*destPtr++ = colorPtr[1];
					*destPtr++ = colorPtr[2];
				}
			}break;
		case 24://BGR
			{
				reader.Read(3 * count, output.GetRawPtr());
				if (isOutputRGB)
					convert::BGRsToRGBs(output.GetRawPtr(), count);
			}break;
		case 32://BGRA
			{
				Image tmp(ImageDataType::RGBA);
				tmp.SetSize(output.Width, 1);
				for (uint32_t row = 0; row < output.Height; ++row)
				{
					reader.Read(4 * output.Width, tmp.GetRawPtr());
					if (isOutputRGB)
						convert::BGRAsToRGBs(output.GetRawPtr(row), tmp.GetRawPtr(), count);
					else
						convert::RGBAsToRGBs(output.GetRawPtr(row), tmp.GetRawPtr(), count);
				}
			}break;
		}
	}

	template<typename MapperReader, typename IndexReader>
	static void ReadFromColorMapped(const detail::TgaHeader& header, Image& image, MapperReader& mapperReader, IndexReader& reader)
	{
		const uint64_t count = (uint64_t)image.Width * image.Height;
		const bool isOutputRGB = REMOVE_MASK(image.DataType, { ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK }) == ImageDataType::RGB;
		const bool needAlpha = HAS_FIELD(image.DataType, ImageDataType::ALPHA_MASK);

		const detail::ColorMapInfo mapInfo(header);
		mapperReader.Skip(mapInfo.Offset);
		Image mapper(needAlpha ? ImageDataType::RGBA : ImageDataType::RGB);
		if (needAlpha)
			ReadColorData4(mapInfo.ColorDepth, mapInfo.Size, mapper, isOutputRGB, mapperReader);
		else
			ReadColorData3(mapInfo.ColorDepth, mapInfo.Size, mapper, isOutputRGB, mapperReader);

		if (needAlpha)
		{
			const uint32_t * __restrict const mapPtr = reinterpret_cast<uint32_t*>(mapper.GetRawPtr());
			uint32_t * __restrict destPtr = reinterpret_cast<uint32_t*>(image.GetRawPtr());
			if (header.PixelDepth == 8)
			{
				std::vector<uint8_t> idxes;
				reader.Read(count, idxes);
				for (auto idx : idxes)
					*destPtr++ = mapPtr[idx];
			}
			else if (header.PixelDepth == 16)
			{
				std::vector<uint16_t> idxes;
				reader.Read(count, idxes);
				for (auto idx : idxes)
					*destPtr++ = mapPtr[idx];
			}
		}
		else
		{
			const uint8_t * __restrict const mapPtr = mapper.GetRawPtr();
			uint8_t * __restrict destPtr = image.GetRawPtr();
			if (header.PixelDepth == 8)
			{
				std::vector<uint8_t> idxes;
				reader.Read(count, idxes);
				for (auto idx : idxes)
				{
					const size_t idx3 = idx * 3;
					*destPtr++ = mapPtr[idx3 + 0];
					*destPtr++ = mapPtr[idx3 + 1];
					*destPtr++ = mapPtr[idx3 + 2];
				}
			}
			else if (header.PixelDepth == 16)
			{
				std::vector<uint16_t> idxes;
				reader.Read(count, idxes);
				for (auto idx : idxes)
				{
					const size_t idx3 = idx * 3;
					*destPtr++ = mapPtr[idx3 + 0];
					*destPtr++ = mapPtr[idx3 + 1];
					*destPtr++ = mapPtr[idx3 + 2];
				}
			}
		}
	}

	template<typename ReadFunc>
	static void ReadFromColor(const detail::TgaHeader& header, Image& image, ReadFunc& reader)
	{
		const uint64_t count = (uint64_t)image.Width * image.Height;
		const bool isOutputRGB = REMOVE_MASK(image.DataType, { ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK }) == ImageDataType::RGB;
		const bool needAlpha = HAS_FIELD(image.DataType, ImageDataType::ALPHA_MASK);
		if (needAlpha)
			ReadColorData4(header.PixelDepth, count, image, isOutputRGB, reader);
		else
			ReadColorData3(header.PixelDepth, count, image, isOutputRGB, reader);
	}
};

//implementation promise each read should be at least a line
class RLEFileDecoder
{
private:
	FileObject& ImgFile;
	const uint8_t ElementSize;
	bool Read1(size_t limit, uint8_t * __restrict output)
	{
		while (limit)
		{
			const uint8_t info = ImgFile.ReadByte();
			const uint8_t size = (info & 0x7f) + 1;
			if (size > limit)
				return false;
			limit -= size;
			if (info > 127)
			{
				const uint8_t obj = ImgFile.ReadByte();
				memset(output, obj, size);
			}
			else
			{
				if (!ImgFile.Read(size, output))
					return false;
			}
			output += size;
		}
		return true;
	}
	bool Read2(size_t limit, uint16_t * __restrict output)
	{
		while (limit)
		{
			const uint8_t info = ImgFile.ReadByte();
			const uint8_t size = (info & 0x7f) + 1;
			if (size > limit)
				return false;
			limit -= size;
			if (info > 127)
			{
				uint16_t obj;
				if (!ImgFile.Read(obj))
					return false;
				for (auto count = size; count--;)
					*output++ = obj;
			}
			else
			{
				if (!ImgFile.Read(size * 2, output))
					return false;
				output += size;
			}
		}
		return true;
	}
	bool Read3(size_t limit, uint8_t * __restrict output)
	{
		while (limit)
		{
			const uint8_t info = ImgFile.ReadByte();
			const uint8_t size = (info & 0x7f) + 1;
			if (size > limit)
				return false;
			limit -= size;
			if (info > 127)
			{
				uint8_t obj[3];
				if (ImgFile.Read(obj) != 3)
					return false;
				for (auto count = size; count--;)
				{
					*output++ = obj[0];
					*output++ = obj[1];
					*output++ = obj[2];
				}
			}
			else
			{
				if (!ImgFile.Read(size * 3, output))
					return false;
				output += size * 3;
			}
		}
		return true;
	}
	bool Read4(size_t limit, uint32_t * __restrict output)
	{
		while (limit)
		{
			const uint8_t info = ImgFile.ReadByte();
			const uint8_t size = (info & 0x7f) + 1;
			if (size > limit)
				return false;
			limit -= size;
			if (info > 127)
			{
				uint32_t obj;
				if (!ImgFile.Read(obj))
					return false;
				for (auto count = size; count--;)
					*output++ = obj;
			}
			else
			{
				if (!ImgFile.Read(size * 4, output))
					return false;
				output += size;
			}
		}
		return true;
	}
public:
	RLEFileDecoder(FileObject& file, const uint8_t elementDepth) : ImgFile(file), ElementSize(elementDepth == 15 ? 2 : (elementDepth / 8)) {}
	void Skip(const size_t offset = 0) { ImgFile.Skip(offset); }

	template<class T, typename = typename std::enable_if<std::is_class<T>::value>::type>
	size_t Read(const size_t count, T& output)
	{
		return Read(count * sizeof(T::value_type), output.data()) ? count : 0;
	}

	bool Read(const size_t len, void *ptr)
	{
		switch (ElementSize)
		{
		case 1:
			return Read1(len, (uint8_t*)ptr);
		case 2:
			return Read2(len / 2, (uint16_t*)ptr);
		case 3:
			return Read3(len / 3, (uint8_t*)ptr);
		case 4:
			return Read4(len / 4, (uint32_t*)ptr);
		default:
			return false;
		}
	}
};

Image TgaReader::Read(const ImageDataType dataType)
{
	common::SimpleTimer timer;
	timer.Start();
	if (HAS_FIELD(dataType, ImageDataType::FLOAT_MASK) || REMOVE_MASK(dataType, { ImageDataType::ALPHA_MASK }) == ImageDataType::GREY)
		//NotSuported
		return Image();
	ImgFile.Rewind(detail::TGA_HEADER_SIZE + Header.IdLength);//Next ColorMap(optional)
	Image image(dataType);
	image.SetSize(Width, Height);
	if (Header.ColorMapType)
	{
		if (HAS_FIELD(Header.ImageType, detail::TGAImgType::RLE_MASK))
		{
			RLEFileDecoder decoder(ImgFile, Header.PixelDepth);
			TgaHelper::ReadFromColorMapped(Header, image, ImgFile, decoder);
		}
		else
			TgaHelper::ReadFromColorMapped(Header, image, ImgFile, ImgFile);
	}
	else
	{
		if (HAS_FIELD(Header.ImageType, detail::TGAImgType::RLE_MASK))
		{
			RLEFileDecoder decoder(ImgFile, Header.PixelDepth);
			TgaHelper::ReadFromColor(Header, image, decoder);
		}
		else
			TgaHelper::ReadFromColor(Header, image, ImgFile);
	}
	timer.Stop();
	ImgLog().debug(L"[zextga] read cost {} ms\n", timer.ElapseMs());
	timer.Start();
	switch ((Header.ImageDescriptor & 0x30) >> 4)
	{
	case 0://left-down
		image.FlipVertical(); break;
	case 1://right-down
		break;
	case 2://left-up
		break;
	case 3://right-up
		break;
	}
	timer.Stop();
	ImgLog().debug(L"[zextga] flip cost {} ms\n", timer.ElapseMs()); 
	return image;
}

TgaWriter::TgaWriter(FileObject& file) : ImgFile(file)
{
}

void TgaWriter::Write(const Image& image)
{
}


TgaSupport::TgaSupport() : ImgSupport(L"Tga") 
{
	common::SimpleTimer timer;
	timer.Start();
	auto& map1 = GetBGR16ToRGBAMap();
	auto& map2 = GetRGB16ToRGBAMap();
	timer.Stop();
	const auto volatile time = timer.ElapseMs();
}

}