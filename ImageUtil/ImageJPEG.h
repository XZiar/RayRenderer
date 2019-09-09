#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"
#include "DataConvertor.hpp"


namespace xziar::img::jpeg
{
using namespace common;
struct JpegHelper;

class StreamReader : public NonCopyable, public NonMovable
{
public:
    using MemSpan = std::pair<const std::byte*, size_t>;
    virtual ~StreamReader() {}
    virtual MemSpan Rewind() = 0;
    virtual MemSpan ReadFromStream() = 0;
    virtual MemSpan SkipStream(size_t len) = 0;
};


class IMGUTILAPI JpegReader : public ImgReader
{
    friend JpegHelper;
private:
    RandomInputStream& Stream;
    std::unique_ptr<StreamReader> Reader;
    void *JpegDecompStruct = nullptr;
    void *JpegSource = nullptr;
    void *JpegErrorHandler = nullptr;
public:
    JpegReader(RandomInputStream& stream);
    virtual ~JpegReader() override;
    virtual bool Validate() override;
    virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI JpegWriter : public ImgWriter
{
    friend JpegHelper;
private:
    RandomOutputStream& Stream;
    common::AlignedBuffer Buffer;
    void *JpegCompStruct = nullptr;
    void *JpegDest = nullptr;
    void *JpegErrorHandler = nullptr;
public:
    JpegWriter(RandomOutputStream& stream);
    virtual ~JpegWriter() override;
    virtual void Write(const Image& image, const uint8_t quality) override;
};

class IMGUTILAPI JpegSupport : public ImgSupport
{
public:
    JpegSupport() : ImgSupport(u"Jpeg") {}
    virtual ~JpegSupport() override {}
    virtual std::unique_ptr<ImgReader> GetReader(RandomInputStream& stream, const u16string&) const override
    {
        return std::make_unique<JpegReader>(stream);
    }
    virtual std::unique_ptr<ImgWriter> GetWriter(RandomOutputStream& stream, const u16string&) const override
    {
        return std::make_unique<JpegWriter>(stream);
    }
    virtual uint8_t MatchExtension(const u16string& ext, const ImageDataType, const bool) const override 
    { 
        return (ext == u"JPEG" || ext == u"JPG") ? 240 : 0;
    }
};

}