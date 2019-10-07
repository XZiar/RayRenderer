#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::stb
{

using namespace common;
enum class ImgType : uint8_t { None = 0, PNM, JPG, PNG, BMP, GIF, PSD, PIC, TGA };

class IMGUTILAPI StbReader : public ImgReader
{
private:
    RandomInputStream& Stream;
    void *StbContext = nullptr;
    ImgType TestedType = ImgType::None;
public:
    StbReader(RandomInputStream& stream);
    virtual ~StbReader() override;
    virtual bool Validate() override;
    virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI StbWriter : public ImgWriter
{
private:
    RandomOutputStream& Stream;
    ImgType TargetType = ImgType::None;
public:
    StbWriter(RandomOutputStream& stream, const std::u16string& ext);
    virtual ~StbWriter() override;
    virtual void Write(const Image& image, const uint8_t quality) override;
};

class IMGUTILAPI StbSupport : public ImgSupport
{
public:
    StbSupport() : ImgSupport(u"Stb") {}
    virtual ~StbSupport() override {}
    virtual std::unique_ptr<ImgReader> GetReader(RandomInputStream& stream, const std::u16string&) const override
    {
        return std::make_unique<StbReader>(stream);
    }
    virtual std::unique_ptr<ImgWriter> GetWriter(RandomOutputStream& stream, const std::u16string& ext) const override
    {
        return std::make_unique<StbWriter>(stream, ext);
    }
    virtual uint8_t MatchExtension(const std::u16string& ext, const ImageDataType, const bool IsRead) const override;
};


}