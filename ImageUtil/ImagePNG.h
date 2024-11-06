#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::png
{


class IMGUTILAPI PngReader : public ImgReader
{
private:
    common::io::RandomInputStream& Stream;
    void *PngStruct = nullptr;
    void *PngInfo = nullptr;
public:
    PngReader(common::io::RandomInputStream& stream);
    virtual ~PngReader() override;
    [[nodiscard]] virtual bool Validate() override;
    [[nodiscard]] virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI PngWriter : public ImgWriter
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

class IMGUTILAPI PngSupport : public ImgSupport
{
public:
    PngSupport() : ImgSupport(u"Libpng") {}
    virtual ~PngSupport() override {}
    [[nodiscard]] virtual std::unique_ptr<ImgReader> GetReader(common::io::RandomInputStream& stream, const std::u16string&) const override
    {
        return std::make_unique<PngReader>(stream);
    }
    [[nodiscard]] virtual std::unique_ptr<ImgWriter> GetWriter(common::io::RandomOutputStream& stream, const std::u16string&) const override
    {
        return std::make_unique<PngWriter>(stream);
    }
    [[nodiscard]] virtual uint8_t MatchExtension(const std::u16string& ext, const ImageDataType, const bool) const override
    { 
        return ext == u"PNG" ? 240 : 0;
    }
};


}