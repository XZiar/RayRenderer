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
    FileObject & ImgFile;
    common::AlignedBuffer Buffer;
    void *JpegDecompStruct = nullptr;
    void *JpegSource = nullptr;
    void *JpegErrorHandler = nullptr;
public:
    JpegReader(FileObject& file);
    virtual ~JpegReader() override;
    virtual bool Validate() override;
    virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI JpegWriter : public ImgWriter
{
    friend JpegHelper;
private:
    FileObject& ImgFile;
    common::AlignedBuffer Buffer;
    void *JpegCompStruct = nullptr;
    void *JpegDest = nullptr;
    void *JpegErrorHandler = nullptr;
public:
    JpegWriter(FileObject& file);
    virtual ~JpegWriter() override;
    virtual void Write(const Image& image) override;
};

class IMGUTILAPI JpegSupport : public ImgSupport
{
public:
    JpegSupport() : ImgSupport(u"Jpeg") {}
    virtual ~JpegSupport() override {}
    virtual Wrapper<ImgReader> GetReader(FileObject& file) const override { return Wrapper<JpegReader>(file).cast_dynamic<ImgReader>(); }
    virtual Wrapper<ImgWriter> GetWriter(FileObject& file) const override { return Wrapper<JpegWriter>(file).cast_dynamic<ImgWriter>(); }
    virtual uint8_t MatchExtension(const u16string& ext, const ImageDataType, const bool) const override 
    { return (ext == u".JPEG" || ext == u".JPG") ? 240 : 0; }
};

}