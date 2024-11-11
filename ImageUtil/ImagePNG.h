#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::libpng
{


class PngReader : public ImgReader
{
private:
    common::io::RandomInputStream& Stream;
    void *PngStruct = nullptr;
    void *PngInfo = nullptr;
public:
    PngReader(common::io::RandomInputStream& stream);
    virtual ~PngReader() override;
    [[nodiscard]] virtual bool Validate() override;
    [[nodiscard]] virtual Image Read(ImageDataType dataType) override;
};

class PngWriter : public ImgWriter
{
private:
    common::io::RandomOutputStream& Stream;
    void *PngStruct = nullptr;
    void *PngInfo = nullptr;
public:
    PngWriter(common::io::RandomOutputStream& stream);
    virtual ~PngWriter() override;
    virtual void Write(const Image& image, const uint8_t quality) override;
};

class PngSupport final : public ImgSupport
{
public:
    PngSupport() : ImgSupport(u"Libpng") {}
    ~PngSupport() final {}
    [[nodiscard]] std::unique_ptr<ImgReader> GetReader(common::io::RandomInputStream& stream, std::u16string_view) const final
    {
        return std::make_unique<PngReader>(stream);
    }
    [[nodiscard]] std::unique_ptr<ImgWriter> GetWriter(common::io::RandomOutputStream& stream, std::u16string_view) const final
    {
        return std::make_unique<PngWriter>(stream);
    }
    [[nodiscard]] uint8_t MatchExtension(std::u16string_view ext, ImageDataType, const bool) const final
    { 
        return ext == u"PNG" ? 240 : 0;
    }
};


}