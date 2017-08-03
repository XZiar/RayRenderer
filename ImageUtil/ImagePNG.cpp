#include "ImageUtilRely.h"
#include "ImagePNG.h"
#include "DataConvertor.hpp"

#include "libpng/png.h"
#include "zlib/zlib.h"
#include "common/StringEx.hpp"
#include "common/TimeUtil.hpp"


#pragma message("Compile ImagePNG with libpng[" STRINGIZE(PNG_LIBPNG_VER_STRING) "] AND zlib[" STRINGIZE(ZLIB_VERSION) "]")

namespace xziar::img::png
{
constexpr static size_t PNG_BYTES_TO_CHECK = 8;


static void OnReadFile(png_structp pngStruct, uint8_t *data, size_t length)
{
	FileObject& file = *(FileObject*)png_get_io_ptr(pngStruct);
	file.Read(length, data);
}
static void OnWriteFile(png_structp pngStruct, uint8_t *data, size_t length)
{
	FileObject& file = *(FileObject*)png_get_io_ptr(pngStruct);
	file.Write(length, data);
}
static void OnFlushFile(png_structp pngStruct) {}

static void OnError(png_structrp pngStruct, const char *message)
{
	ImgLog().error(L"LIBPNG report an error: {}\n", to_wstring(message));
	COMMON_THROW(BaseException, L"Libpng report an error");
}
static void OnWarn(png_structrp pngStruct, const char *message)
{
	ImgLog().warning(L"LIBPNG warns: {}\n", to_wstring(message));
}

static std::vector<uint8_t*> GetRowPtrs(Image& image, const size_t offset = 0)
{
	std::vector<uint8_t*> pointers(image.Height, nullptr);
	uint8_t *rawPtr = image.GetRawPtr();
	size_t lineStep = image.ElementSize * image.Width;
	for (auto& ptr : pointers)
		ptr = rawPtr + offset, rawPtr += lineStep;
	return pointers;
}
static std::vector<const uint8_t*> GetRowPtrs(const Image& image, const size_t offset = 0)
{
	std::vector<const uint8_t*> pointers(image.Height, nullptr);
	const uint8_t *rawPtr = image.GetRawPtr();
	size_t lineStep = image.ElementSize * image.Width;
	for (auto& ptr : pointers)
		ptr = rawPtr + offset, rawPtr += lineStep;
	return pointers;
}

static png_structp CreateReadStruct()
{
	auto handle = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, OnError, OnWarn);
	if (!handle)
		COMMON_THROW(BaseException, L"Cannot alloc space for png struct");
	return handle;
}
static png_structp CreateWriteStruct()
{
	auto handle = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, OnError, OnWarn);
	if (!handle)
		COMMON_THROW(BaseException, L"Cannot alloc space for png struct");
	return handle;
}
static png_infop CreateInfo(png_structp pngStruct)
{
	auto handle = png_create_info_struct(pngStruct);
	if (!handle)
		COMMON_THROW(BaseException, L"Cannot alloc space for png info");
	return handle;
}


void PngReader::ReadThrough(uint8_t passes, Image& image)
{
	auto ptrs = GetRowPtrs(image);
	while (passes--)
	{
		//Sparkle, read all rows at a time
		png_read_rows((png_structp)PngStruct, ptrs.data(), NULL, image.Height);
	}
}

void PngReader::ReadColorToColorAlpha(uint8_t passes, Image& image)
{
	auto ptrs = GetRowPtrs(image, image.Width);
	common::SimpleTimer timer;
	timer.Start();
	while (passes--)
	{
		//Sparkle, read all rows at a time
		png_read_rows((png_structp)PngStruct, ptrs.data(), NULL, image.Height);
	}
	timer.Stop();
	ImgLog().debug(L"[libpng]decode cost {} ms\n", timer.ElapseMs());
	//post process, 3-comp ---> 4-comp
	uint8_t *rowPtr = image.GetRawPtr();
	const size_t lineStep = image.ElementSize * image.Width;
	timer.Start();
	for (uint32_t row = 0; row < image.Height; row++, rowPtr += lineStep)
	{
		uint8_t * __restrict destPtr = rowPtr;
		uint8_t * __restrict srcPtr = rowPtr + image.Width;
		img::convert::RGBsToRGBAs(destPtr, srcPtr, image.Width);
	}
	timer.Stop();
	ImgLog().debug(L"[png]post 3->4comp cost {} ms\n", timer.ElapseMs());
}
#undef LOOP_RGB_RGBA


PngReader::PngReader(FileObject& file) : ImgFile(file), PngStruct(CreateReadStruct()), PngInfo(CreateInfo((png_structp)PngStruct))
{
	png_set_read_fn((png_structp)PngStruct, &ImgFile, OnReadFile);
}

PngReader::~PngReader()
{
	if (PngStruct)
	{
		if (PngInfo)
			png_destroy_read_struct((png_structpp)&PngStruct, (png_infopp)&PngInfo, nullptr);
		else
			png_destroy_read_struct((png_structpp)&PngStruct, nullptr, nullptr);
	}
}

bool PngReader::Validate()
{
	ImgFile.Rewind();
	uint8_t buf[PNG_BYTES_TO_CHECK];
	if (ImgFile.Read(buf) != PNG_BYTES_TO_CHECK)
		return false;
	return !png_sig_cmp(buf, 0, PNG_BYTES_TO_CHECK);
}

Image PngReader::Read(const ImageDataType dataType)
{
	auto pngStruct = (png_structp)PngStruct;
	auto pngInfo = (png_infop)PngInfo;
	if (REMOVE_MASK(dataType, { ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK }) == ImageDataType::GREY || HAS_FIELD(dataType, ImageDataType::FLOAT_MASK))
		//NotSupported Yet
		return Image(dataType);
	ImgFile.Rewind();

	png_read_info(pngStruct, pngInfo);
	uint32_t width = 0, height = 0;
	int32_t bitDepth = -1, colorType = -1, interlaceType = -1;
	png_get_IHDR((png_structp)PngStruct, pngInfo, &width, &height, &bitDepth, &colorType, &interlaceType, nullptr, nullptr);

	Image image(dataType);
	image.Type = ImageType::PNG;
	image.SetSize(width, height);

	if (bitDepth == 16)
	{
#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
		png_set_scale_16(pngStruct);
#else
		png_set_strip_16(PngStruct);
#endif
	}
	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
	* byte into separate bytes (useful for paletted and grayscale images).
	*/
	png_set_packing(pngStruct);
	switch (colorType)
	{
	case PNG_COLOR_TYPE_PALETTE:
		/* Expand paletted colors into true RGB triplets */
		png_set_palette_to_rgb(pngStruct);
		break;
	case PNG_COLOR_TYPE_GRAY:
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		if (bitDepth < 8)
		{
			if (bitDepth == 1)
				png_set_invert_mono(pngStruct);
			/* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
			png_set_expand_gray_1_2_4_to_8(pngStruct);
		}
		png_set_gray_to_rgb(pngStruct);
		break;
	}
	/* Expand paletted or RGB images with transparency to full alpha channels
	* so the data will be available as RGBA quartets.
	*/
	if (png_get_valid(pngStruct, pngInfo, PNG_INFO_tRNS) != 0)
		png_set_tRNS_to_alpha(pngStruct);
	/* Strip alpha bytes from the input data */
	//if (!HAS_FIELD(dataType, ImageDataType::ALPHA_MASK))
		png_set_strip_alpha(pngStruct);
	/* Swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR) */
	if (REMOVE_MASK(dataType, { ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK }) == ImageDataType::BGR)
		png_set_swap_alpha(pngStruct);

	//handle interlace
	const uint8_t passes = (interlaceType == PNG_INTERLACE_NONE) ? 1 : png_set_interlace_handling(pngStruct);
	png_start_read_image(pngStruct);
	if (HAS_FIELD(dataType, ImageDataType::ALPHA_MASK) && (colorType & PNG_COLOR_MASK_ALPHA) != 0)//3comp->4comp
		ReadColorToColorAlpha(passes, image);
	else
		ReadThrough(passes, image);
	png_read_end(pngStruct, pngInfo);
	return image;
}


PngWriter::PngWriter(FileObject& file) : ImgFile(file), PngStruct(CreateWriteStruct()), PngInfo(CreateInfo((png_structp)PngStruct))
{
	png_set_write_fn((png_structp)PngStruct, &ImgFile, OnWriteFile, OnFlushFile);
}

PngWriter::~PngWriter()
{
	if (PngStruct)
	{
		if (PngInfo)
			png_destroy_write_struct((png_structpp)&PngStruct, (png_infopp)&PngInfo);
		else
			png_destroy_write_struct((png_structpp)&PngStruct, nullptr);
	}
}

void PngWriter::Write(const Image& image)
{
	auto pngStruct = (png_structp)PngStruct;
	auto pngInfo = (png_infop)PngInfo;
	if (REMOVE_MASK(image.DataType, { ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK }) == ImageDataType::GREY || HAS_FIELD(image.DataType, ImageDataType::FLOAT_MASK))
		//NotSupported Yet
		return;
	ImgFile.Rewind();

	const auto colorType = HAS_FIELD(image.DataType, ImageDataType::ALPHA_MASK) ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB;
	png_set_IHDR(pngStruct, pngInfo, image.Width, image.Height, 8, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_set_compression_level(pngStruct, 3);
	png_write_info(pngStruct, pngInfo);

	if (REMOVE_MASK(image.DataType, { ImageDataType::ALPHA_MASK, ImageDataType::FLOAT_MASK }) == ImageDataType::BGR)
		png_set_swap_alpha(pngStruct);

	auto ptrs = GetRowPtrs(image);
	png_write_image(pngStruct, (png_bytepp)ptrs.data());
	png_write_end(pngStruct, pngInfo);
}



}