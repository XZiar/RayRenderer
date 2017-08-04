#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"
#include "DataConvertor.hpp"


namespace xziar::img::tga
{
using namespace common;

namespace detail
{
enum class TGAImgType : uint8_t
{
	RLE_MASK = 0x8, EMPTY = 0,
	COLOR_MAP = 1, COLOR = 2, GREY = 3
};
MAKE_ENUM_BITFIELD(TGAImgType)

struct TgaHeader
{
	uint8_t IdLength;
	uint8_t ColorMapType;
	TGAImgType ImageType;
	union
	{
		uint8_t ColorMapSpec[5];
		struct
		{
			uint8_t ColorMapOffset[2];
			uint8_t ColorMapCount[2];
			uint8_t ColorEntryDepth;
		};
	};
	uint8_t OriginHorizontal[2];
	uint8_t OriginVertical[2];
	uint8_t Width[2];
	uint8_t Height[2];
	uint8_t PixelDepth;
	uint8_t ImageDescriptor;
};
constexpr size_t TGA_HEADER_SIZE = sizeof(TgaHeader);
struct ColorMapInfo
{
	uint16_t Offset;
	uint16_t Size;
	uint8_t ColorDepth;
	ColorMapInfo(const TgaHeader& header)
	{
		Offset = img::convert::ParseWordLE(header.ColorMapOffset);
		Size = img::convert::ParseWordLE(header.ColorMapCount);
		ColorDepth = header.ColorEntryDepth;
	}
};
}

class IMGUTILAPI TgaReader : public ImgReader
{
private:
	FileObject& ImgFile;
	detail::TgaHeader Header;
	int32_t Width, Height;
	void ReadGrey(Image& image) {};
public:
	TgaReader(FileObject& file);
	virtual ~TgaReader() override {};
	virtual bool Validate() override;
	virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI TgaWriter : public ImgWriter
{
private:
	FileObject& ImgFile;
public:
	TgaWriter(FileObject& file);
	virtual ~TgaWriter() override {};
	virtual void Write(const Image& image) override;
};

class IMGUTILAPI TgaSupport : public ImgSupport
{
	friend class TgaReader;
	friend class TgaWriter;
public:
	TgaSupport();
	virtual Wrapper<ImgReader> GetReader(FileObject& file) const override { return Wrapper<TgaReader>(file).cast_static<ImgReader>(); }
	virtual Wrapper<ImgWriter> GetWriter(FileObject& file) const override { return Wrapper<TgaWriter>(file).cast_static<ImgWriter>(); }
	virtual bool MatchExtension(const wstring& ext) const override { return ext == L".TGA"; }
	virtual bool MatchType(const wstring& type) const override { return type == L"TGA"; }
};


}