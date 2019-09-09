#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::bmp
{
using namespace common;

namespace detail
{
#pragma pack(push, 1)
struct BmpHeader
{
    char Sig[2];
    byte Size[4];
    union
    {
        uint8_t Dummy[4];
        uint32_t Reserved;
    };
    byte Offset[4];
};
constexpr size_t BMP_HEADER_SIZE = sizeof(BmpHeader);
struct BmpInfo
{
    uint32_t Size;
    byte Width[4];
    byte Height[4];
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
    const std::unique_ptr<RandomInputStream>& Stream;
    detail::BmpHeader Header;
    detail::BmpInfo Info;
public:
    BmpReader(const std::unique_ptr<RandomInputStream>& stream);
    virtual ~BmpReader() override {};
    virtual bool Validate() override;
    virtual Image Read(const ImageDataType dataType) override;
};


class IMGUTILAPI BmpWriter : public ImgWriter
{
private:
    const std::unique_ptr<RandomOutputStream>& Stream;
    detail::BmpHeader Header;
    detail::BmpInfo Info;
public:
    BmpWriter(const std::unique_ptr<RandomOutputStream>& stream);
    virtual ~BmpWriter() override {};
    virtual void Write(const Image& image, const uint8_t quality) override;
};


class IMGUTILAPI BmpSupport : public ImgSupport {
public:
    BmpSupport();
    virtual ~BmpSupport() override {}
    virtual std::unique_ptr<ImgReader> GetReader(const std::unique_ptr<RandomInputStream>& stream, const u16string&) const override
    {
        return std::make_unique<BmpReader>(stream);
    }
    virtual std::unique_ptr<ImgWriter> GetWriter(const std::unique_ptr<RandomOutputStream>& stream, const u16string&) const override
    {
        return std::make_unique<BmpWriter>(stream);
    }
    virtual uint8_t MatchExtension(const u16string& ext, const ImageDataType, const bool) const override 
    { 
        return ext == u"BMP" ? 240 : 0;
    }
};


}