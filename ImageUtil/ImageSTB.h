#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::stb
{


enum class ImgType : uint8_t { None = 0, PNM, JPG, PNG, BMP, GIF, PSD, PIC, TGA };

class IMGUTILAPI StbReader : public ImgReader
{
private:
    common::io::RandomInputStream& Stream;
    void *StbContext = nullptr;
    ImgType TestedType = ImgType::None;
public:
    StbReader(common::io::RandomInputStream& stream);
    virtual ~StbReader() override;
    virtual bool Validate() override;
    virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI StbWriter : public ImgWriter
{
private:
    common::io::RandomOutputStream& Stream;
    ImgType TargetType = ImgType::None;
public:
    StbWriter(common::io::RandomOutputStream& stream, const std::u16string& ext);
    virtual ~StbWriter() override;
    virtual void Write(const Image& image, const uint8_t quality) override;
};

class IMGUTILAPI StbSupport : public ImgSupport
{
public:
    StbSupport() : ImgSupport(u"Stb") {}
    virtual ~StbSupport() override {}
    virtual std::unique_ptr<ImgReader> GetReader(common::io::RandomInputStream& stream, const std::u16string&) const override
    {
        return std::make_unique<StbReader>(stream);
    }
    virtual std::unique_ptr<ImgWriter> GetWriter(common::io::RandomOutputStream& stream, const std::u16string& ext) const override
    {
        return std::make_unique<StbWriter>(stream, ext);
    }
    virtual uint8_t MatchExtension(const std::u16string& ext, const ImageDataType, const bool IsRead) const override;
};


}