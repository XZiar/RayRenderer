#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"
#include "DataConvertor.hpp"


namespace xziar::img::jpeg
{
using namespace common;
struct JpegHelper;
namespace detail
{

}

class IMGUTILAPI JpegReader : public ImgReader
{
    friend JpegHelper;
private:
    const std::unique_ptr<RandomInputStream>& Stream;
    common::AlignedBuffer Buffer;
    void *JpegDecompStruct = nullptr;
    void *JpegSource = nullptr;
    void *JpegErrorHandler = nullptr;
public:
    JpegReader(const std::unique_ptr<RandomInputStream>& stream);
    virtual ~JpegReader() override;
    virtual bool Validate() override;
    virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI JpegWriter : public ImgWriter
{
    friend JpegHelper;
private:
    const std::unique_ptr<RandomOutputStream>& Stream;
    common::AlignedBuffer Buffer;
    void *JpegCompStruct = nullptr;
    void *JpegDest = nullptr;
    void *JpegErrorHandler = nullptr;
public:
    JpegWriter(const std::unique_ptr<RandomOutputStream>& stream);
    virtual ~JpegWriter() override;
    virtual void Write(const Image& image, const uint8_t quality) override;
};

class IMGUTILAPI JpegSupport : public ImgSupport
{
public:
    JpegSupport() : ImgSupport(u"Jpeg") {}
    virtual ~JpegSupport() override {}
    virtual std::unique_ptr<ImgReader> GetReader(const std::unique_ptr<RandomInputStream>& stream, const u16string&) const override
    {
        return std::make_unique<JpegReader>(stream);
    }
    virtual std::unique_ptr<ImgWriter> GetWriter(const std::unique_ptr<RandomOutputStream>& stream, const u16string&) const override
    {
        return std::make_unique<JpegWriter>(stream);
    }
    virtual uint8_t MatchExtension(const u16string& ext, const ImageDataType, const bool) const override 
    { 
        return (ext == u"JPEG" || ext == u"JPG") ? 240 : 0;
    }
};

}