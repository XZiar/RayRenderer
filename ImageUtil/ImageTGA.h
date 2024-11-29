#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::zex
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
        Offset = util::ParseWordLE(header.ColorMapData.ColorMapOffset);
        Size = util::ParseWordLE(header.ColorMapData.ColorMapCount);
        ColorDepth = header.ColorMapData.ColorEntryDepth;
    }
};
}

class TgaReader : public ImgReader
{
private:
    common::io::RandomInputStream& Stream;
    detail::TgaHeader Header;
    int32_t Width = 0, Height = 0;
public:
    TgaReader(common::io::RandomInputStream& stream);
    virtual ~TgaReader() override {};
    [[nodiscard]] virtual bool Validate() override;
    [[nodiscard]] virtual Image Read(ImgDType dataType) override;
};

class TgaWriter : public ImgWriter
{
private:
    common::io::RandomOutputStream& Stream;
public:
    TgaWriter(common::io::RandomOutputStream& stream);
    virtual ~TgaWriter() override {};
    virtual void Write(const Image& image, const uint8_t quality) override;
};

class TgaSupport final : public ImgSupport
{
    friend class TgaReader;
    friend class TgaWriter;
public:
    TgaSupport() : ImgSupport(u"ZexTga") {}
    ~TgaSupport() final {}
    [[nodiscard]] std::unique_ptr<ImgReader> GetReader(common::io::RandomInputStream& stream, std::u16string_view) const final
    {
        return std::make_unique<TgaReader>(stream);
    }
    [[nodiscard]] std::unique_ptr<ImgWriter> GetWriter(common::io::RandomOutputStream& stream, std::u16string_view) const final
    {
        return std::make_unique<TgaWriter>(stream);
    }
    [[nodiscard]] uint8_t MatchExtension(std::u16string_view ext, ImgDType, const bool) const final
    { 
        return ext == u"TGA" ? 240 : 0;
    }
};


}