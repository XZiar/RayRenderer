#pragma once

#include "ImageUtilRely.h"
#include "ImageInternal.h"
#include "common/StringEx.hpp"
#include "libpng/png.h"
#include "zlib/zlib.h"


#pragma message("Compile ImagePNG with libpng[" STRINGIZE(PNG_LIBPNG_VER_STRING) "] AND zlib[" STRINGIZE(ZLIB_VERSION) "]")

namespace xziar::img::png
{

using namespace common;

class PngReader : public _ImgReader
{
private:
	constexpr static size_t PNG_BYTES_TO_CHECK = 8;
	FileObject& ImgFile;
	png_structp PngStruct = nullptr;
	png_infop PngInfo = nullptr;
	static png_structp CreateStruct()
	{
		auto handle = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, OnError, OnWarn);
		if (!handle)
			COMMON_THROW(BaseException, L"Cannot alloc space for png struct");
		return handle;
	}
	static png_infop CreateInfo(png_structp png_structp)
	{
		auto handle = png_create_info_struct(png_structp);
		if (!handle)
			COMMON_THROW(BaseException, L"Cannot alloc space for png struct");
		return handle;
	}

	static std::vector<uint8_t*> GetRowPtrs(Image& image)
	{
		std::vector<uint8_t*> pointers(image.Height, nullptr);
		uint8_t *rawPtr = image.GetRawPtr();
		size_t lineStep = image.ElementSize * image.Width;
		for (auto& ptr : pointers)
			ptr = rawPtr, rawPtr += lineStep;
		return pointers;
	}

	static void OnReadFile(png_structp pngStruct, png_bytep data, png_size_t length)
	{
		FileObject& file = *(FileObject*)png_get_io_ptr(pngStruct);
		file.Read(length, data);
	}
	static void OnError(png_structrp pngStruct, png_const_charp message)
	{
		ImgLog().error(L"LIBPNG report an error: {}\n", to_wstring(message));
		COMMON_THROW(BaseException, L"Libpng report an error");
	}
	static void OnWarn(png_structrp pngStruct, png_const_charp message)
	{
		ImgLog().warning(L"LIBPNG warns: {}\n", to_wstring(message));
	}

public:
	PngReader(FileObject& file) : ImgFile(file), PngStruct(CreateStruct()), PngInfo(CreateInfo(PngStruct)) { }
	virtual ~PngReader() override
	{
		if (PngStruct)
		{
			if (PngInfo)
				png_destroy_read_struct(&PngStruct, &PngInfo, nullptr);
			else
				png_destroy_read_struct(&PngStruct, nullptr, nullptr);
		}
	}
	virtual bool Validate() override 
	{
		ImgFile.Rewind();
		uint8_t buf[PNG_BYTES_TO_CHECK];
		if (ImgFile.Read(buf) != PNG_BYTES_TO_CHECK)
			return false;
		return !png_sig_cmp(buf, 0, PNG_BYTES_TO_CHECK);
	}
	virtual Image Read() override 
	{
		ImgFile.Rewind();
		png_set_read_fn(PngStruct, &ImgFile, OnReadFile);

		png_read_info(PngStruct, PngInfo);
		uint32_t width = 0, height = 0;
		int32_t bitDepth = -1, colorType = -1, interlaceType = -1;
		png_get_IHDR(PngStruct, PngInfo, &width, &height, &bitDepth, &colorType, &interlaceType, nullptr, nullptr);

		const auto imgType = (colorType & PNG_COLOR_MASK_ALPHA) ? ImageDataType::RGBA : ImageDataType::RGB;
		Image image(imgType/*ImageDataType::RGB*/);
		image.Type = ImageType::PNG;
		image.SetSize(width, height);
		
		if (bitDepth == 16)
		{
#ifdef PNG_READ_SCALE_16_TO_8_SUPPORTED
			png_set_scale_16(PngStruct);
#else
			png_set_strip_16(PngStruct);
#endif
		}
		/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
		* byte into separate bytes (useful for paletted and grayscale images).
		*/
		png_set_packing(PngStruct);
		switch (colorType)
		{
		case PNG_COLOR_TYPE_PALETTE:
			/* Expand paletted colors into true RGB triplets */
			png_set_palette_to_rgb(PngStruct); 
			break;
		case PNG_COLOR_TYPE_GRAY:
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			if (bitDepth < 8)
			{	
				if(bitDepth == 1)
					png_set_invert_mono(PngStruct);
				/* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
				png_set_expand_gray_1_2_4_to_8(PngStruct);
			}
			png_set_gray_to_rgb(PngStruct);
			break;
		}
		/* Expand paletted or RGB images with transparency to full alpha channels
		* so the data will be available as RGBA quartets.
		*/
		if (png_get_valid(PngStruct, PngInfo, PNG_INFO_tRNS) != 0)
			png_set_tRNS_to_alpha(PngStruct);
		/* Strip alpha bytes from the input data without combining with the background */
		//png_set_strip_alpha(PngStruct);
		/* Swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR) */
		//png_set_swap_alpha(PngStruct);

		//handle interlace
		uint8_t passes = (interlaceType == PNG_INTERLACE_NONE) ? 1 : png_set_interlace_handling(PngStruct);

		png_start_read_image(PngStruct);
		auto ptrs = GetRowPtrs(image);
		while (passes--)
		{
			//Sparkle, read all rows at a time
			png_read_rows(PngStruct, ptrs.data(), NULL, height);
		}
		png_read_end(PngStruct, PngInfo);
		return image;
	}
};

class PngWriter : public _ImgWriter
{

};

class _PngSupport : public _ImgSupport
{
public:
	_PngSupport() : _ImgSupport(L"Png") {}
	virtual ImgReader GetReader(FileObject& file) const override { return Wrapper<PngReader>(file).cast_static<_ImgReader>(); }
	virtual ImgWriter GetWriter() const override { return (ImgWriter)Wrapper<PngWriter>().cast_static<_ImgWriter>(); }
	virtual bool MatchExtension(const wstring& ext) const override { return ext == L"PNG"; }
};
using PngSupport = Wrapper<_PngSupport>;

}