#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"
#include "DataConvertor.hpp"


namespace xziar::img::tga
{


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
        std::byte ColorMapOffset[2];
        std::byte ColorMapCount[2];
        uint8_t ColorEntryDepth;
    } ColorMapData;
    std::byte OriginHorizontal[2];
    std::byte OriginVertical[2];
    std::byte Width[2];
    std::byte Height[2];
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
    common::io::RandomInputStream& Stream;
    detail::TgaHeader Header;
    int32_t Width, Height;
public:
    TgaReader(common::io::RandomInputStream& stream);
    virtual ~TgaReader() override {};
    virtual bool Validate() override;
    virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI TgaWriter : public ImgWriter
{
private:
    common::io::RandomOutputStream& Stream;
public:
    TgaWriter(common::io::RandomOutputStream& stream);
    virtual ~TgaWriter() override {};
    virtual void Write(const Image& image, const uint8_t quality) override;
};

class IMGUTILAPI TgaSupport : public ImgSupport
{
    friend class TgaReader;
    friend class TgaWriter;
public:
    TgaSupport() : ImgSupport(u"Tga") {}
    virtual ~TgaSupport() override {}
    virtual std::unique_ptr<ImgReader> GetReader(common::io::RandomInputStream& stream, const std::u16string&) const override
    {
        return std::make_unique<TgaReader>(stream);
    }
    virtual std::unique_ptr<ImgWriter> GetWriter(common::io::RandomOutputStream& stream, const std::u16string&) const override
    {
        return std::make_unique<TgaWriter>(stream);
    }
    virtual uint8_t MatchExtension(const std::u16string& ext, const ImageDataType, const bool) const override
    { 
        return ext == u"TGA" ? 240 : 0;
    }
};


}