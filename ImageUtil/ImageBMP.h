#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::bmp
{


namespace detail
{
#pragma pack(push, 1)
struct BmpHeader
{
    char Sig[2];
    std::byte Size[4];
    union
    {
        uint8_t Dummy[4];
        uint32_t Reserved;
    };
    std::byte Offset[4];
};
constexpr size_t BMP_HEADER_SIZE = sizeof(BmpHeader);
struct BmpInfo
{
    uint32_t Size;
    std::byte Width[4];
    std::byte Height[4];
    uint16_t Planes;
    uint16_t BitCount;
    uint32_t Compression;
    uint32_t ImageSize;
    int32_t XPPM;
    int32_t YPPM;
    uint32_t PaletteUsed;
    uint32_t PaletteImportant;
};
constexpr size_t BMP_INFO_SIZE = sizeof(BmpInfo);
#pragma pack(pop)
}

class IMGUTILAPI BmpReader : public ImgReader
{
private:
    common::io::RandomInputStream& Stream;
    detail::BmpHeader Header;
    detail::BmpInfo Info;
public:
    BmpReader(common::io::RandomInputStream& stream);
    virtual ~BmpReader() override {};
    [[nodiscard]] virtual bool Validate() override;
    [[nodiscard]] virtual Image Read(const ImageDataType dataType) override;
};


class IMGUTILAPI BmpWriter : public ImgWriter
{
private:
    common::io::RandomOutputStream& Stream;
    detail::BmpHeader Header;
    detail::BmpInfo Info;
public:
    BmpWriter(common::io::RandomOutputStream& stream);
    virtual ~BmpWriter() override {};
    virtual void Write(const Image& image, const uint8_t quality) override;
};


class IMGUTILAPI BmpSupport : public ImgSupport {
public:
    BmpSupport();
    virtual ~BmpSupport() override {}
    [[nodiscard]] virtual std::unique_ptr<ImgReader> GetReader(common::io::RandomInputStream& stream, const std::u16string&) const override
    {
        return std::make_unique<BmpReader>(stream);
    }
    [[nodiscard]] virtual std::unique_ptr<ImgWriter> GetWriter(common::io::RandomOutputStream& stream, const std::u16string&) const override
    {
        return std::make_unique<BmpWriter>(stream);
    }
    [[nodiscard]] virtual uint8_t MatchExtension(const std::u16string& ext, const ImageDataType, const bool) const override
    { 
        return ext == u"BMP" ? 240 : 0;
    }
};


}