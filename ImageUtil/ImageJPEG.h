#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"
#include "DataConvertor.hpp"


namespace xziar::img::jpeg
{
using namespace common;
struct JpegHelper;
class StreamReader;


class IMGUTILAPI JpegReader : public ImgReader
{
    friend JpegHelper;
private:
    common::io::RandomInputStream& Stream;
    std::unique_ptr<StreamReader> Reader;
    void *JpegDecompStruct = nullptr;
    void *JpegSource = nullptr;
    void *JpegErrorHandler = nullptr;
public:
    JpegReader(common::io::RandomInputStream& stream);
    virtual ~JpegReader() override;
    [[nodiscard]] virtual bool Validate() override;
    [[nodiscard]] virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI JpegWriter : public ImgWriter
{
    friend JpegHelper;
private:
    common::io::RandomOutputStream& Stream;
    common::AlignedBuffer Buffer;
    void *JpegCompStruct = nullptr;
    void *JpegDest = nullptr;
    void *JpegErrorHandler = nullptr;
public:
    JpegWriter(common::io::RandomOutputStream& stream);
    virtual ~JpegWriter() override;
    virtual void Write(const Image& image, const uint8_t quality) override;
};

class IMGUTILAPI JpegSupport : public ImgSupport
{
public:
    JpegSupport() : ImgSupport(u"Jpeg") {}
    virtual ~JpegSupport() override {}
    [[nodiscard]] virtual std::unique_ptr<ImgReader> GetReader(common::io::RandomInputStream& stream, const std::u16string&) const override
    {
        return std::make_unique<JpegReader>(stream);
    }
    [[nodiscard]] virtual std::unique_ptr<ImgWriter> GetWriter(common::io::RandomOutputStream& stream, const std::u16string&) const override
    {
        return std::make_unique<JpegWriter>(stream);
    }
    [[nodiscard]] virtual uint8_t MatchExtension(const std::u16string& ext, const ImageDataType, const bool) const override
    { 
        return (ext == u"JPEG" || ext == u"JPG") ? 240 : 0;
    }
};

}