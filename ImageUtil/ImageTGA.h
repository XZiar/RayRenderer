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
    COLOR_MAP = 1, COLOR = 2, GRAY = 3
};
MAKE_ENUM_BITFIELD(TGAImgType)

struct TgaHeader
{
    uint8_t IdLength;
    uint8_t ColorMapType;
    TGAImgType ImageType;
    struct ColorMapSpec
    {
        byte ColorMapOffset[2];
        byte ColorMapCount[2];
        uint8_t ColorEntryDepth;
    } ColorMapData;
    byte OriginHorizontal[2];
    byte OriginVertical[2];
    byte Width[2];
    byte Height[2];
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
        Offset = img::convert::ParseWordLE(header.ColorMapData.ColorMapOffset);
        Size = img::convert::ParseWordLE(header.ColorMapData.ColorMapCount);
        ColorDepth = header.ColorMapData.ColorEntryDepth;
    }
};
}

class IMGUTILAPI TgaReader : public ImgReader
{
private:
    FileObject & OriginalFile;
    BufferedFileReader ImgFile;
    detail::TgaHeader Header;
    int32_t Width, Height;
public:
    TgaReader(FileObject& file);
    virtual ~TgaReader() override {};
    virtual bool Validate() override;
    virtual Image Read(const ImageDataType dataType) override;
    virtual void Release() override;
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
    TgaSupport() : ImgSupport(u"Tga") {}
    virtual ~TgaSupport() override {}
    virtual Wrapper<ImgReader> GetReader(FileObject& file) const override { return Wrapper<TgaReader>(file).cast_dynamic<ImgReader>(); }
    virtual Wrapper<ImgWriter> GetWriter(FileObject& file) const override { return Wrapper<TgaWriter>(file).cast_dynamic<ImgWriter>(); }
    virtual bool MatchExtension(const u16string& ext, const ImageDataType, const bool) const override { return ext == u".TGA"; }
};


}