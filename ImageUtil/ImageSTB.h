#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::stb
{


enum class ImgType : uint8_t { None = 0, PNM, JPG, PNG, BMP, GIF, PSD, PIC, TGA };

class StbReader : public ImgReader
{
private:
    common::io::RandomInputStream& Stream;
    void *StbContext = nullptr;
    ImgType TestedType = ImgType::None;
public:
    StbReader(common::io::RandomInputStream& stream);
    virtual ~StbReader() override;
    [[nodiscard]] virtual bool Validate() override;
    [[nodiscard]] virtual Image Read(ImageDataType dataType) override;
};

class StbWriter : public ImgWriter
{
private:
    common::io::RandomOutputStream& Stream;
    ImgType TargetType = ImgType::None;
public:
    StbWriter(common::io::RandomOutputStream& stream, std::u16string_view ext);
    virtual ~StbWriter() override;
    virtual void Write(const Image& image, const uint8_t quality) override;
};

class StbSupport : public ImgSupport
{
public:
    StbSupport() : ImgSupport(u"Stb") {}
    ~StbSupport() final {}
    [[nodiscard]] std::unique_ptr<ImgReader> GetReader(common::io::RandomInputStream& stream, std::u16string_view) const final
    {
        return std::make_unique<StbReader>(stream);
    }
    [[nodiscard]] std::unique_ptr<ImgWriter> GetWriter(common::io::RandomOutputStream& stream, std::u16string_view ext) const final
    {
        return std::make_unique<StbWriter>(stream, ext);
    }
    [[nodiscard]] uint8_t MatchExtension(std::u16string_view ext, ImageDataType, const bool IsRead) const final;
};


}