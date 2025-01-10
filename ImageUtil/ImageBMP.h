#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::zex
{


namespace detail
{
#pragma pack(push, 1)
struct BmpHeader
{
    char Sig[2] = { 0 };
    std::byte Size[4] = { std::byte{0} };
    uint32_t Reserved = 0;
    std::byte Offset[4] = { std::byte{0} };
};
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
struct BmpInfoV2 : public BmpInfo
{
    uint32_t MaskRed;
    uint32_t MaskGreen;
    uint32_t MaskBlue;
};
struct BmpInfoV3 : public BmpInfoV2
{
    uint32_t MaskAlpha;
};
struct BmpInfoV4 : public BmpInfoV3
{
    uint32_t ColorSpace;
    uint32_t CIEEndPoint[3][3];
    uint16_t GammaRed[2];
    uint16_t GammaGreen[2];
    uint16_t GammaBlue[2];
};
struct BmpInfoV5 : public BmpInfoV4
{
    uint32_t Intent;
    uint32_t ProfileData;
    uint32_t ProfileSize;
    uint32_t Reserved;
};
#pragma pack(pop)
}

class BmpReader : public ImgReader
{
private:
    common::io::RandomInputStream& Stream;
    detail::BmpInfoV5 Info;
    size_t PixelOffset = 0;
    size_t InfoOffset = 0;
    uint8_t Format = 0;
public:
    BmpReader(common::io::RandomInputStream& stream);
    virtual ~BmpReader() override {};
    [[nodiscard]] virtual bool Validate() override;
    IMGUTILAPI [[nodiscard]] bool ValidateNoHeader(uint32_t pixOffset);
    [[nodiscard]] virtual Image Read(ImgDType dataType) override;
};


class BmpWriter : public ImgWriter
{
private:
    common::io::RandomOutputStream& Stream;
public:
    BmpWriter(common::io::RandomOutputStream& stream);
    virtual ~BmpWriter() override {};
    virtual void Write(ImageView image, const uint8_t quality) override;
};


class IMGUTILAPI BmpSupport final : public ImgSupport 
{
public:
    BmpSupport() : ImgSupport(u"ZexBmp") {}
    ~BmpSupport() final {}
    [[nodiscard]] std::unique_ptr<ImgReader> GetReader(common::io::RandomInputStream& stream, std::u16string_view) const final
    {
        return std::make_unique<BmpReader>(stream);
    }
    [[nodiscard]] std::unique_ptr<ImgWriter> GetWriter(common::io::RandomOutputStream& stream, std::u16string_view) const final
    {
        return std::make_unique<BmpWriter>(stream);
    }
    [[nodiscard]] uint8_t MatchExtension(std::u16string_view ext, ImgDType, const bool isRead) const final;
};


}