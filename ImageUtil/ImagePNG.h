#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::png
{

using namespace common;

class IMGUTILAPI PngReader : public ImgReader
{
private:
    FileObject& ImgFile;
    void *PngStruct = nullptr;
    void *PngInfo = nullptr;
public:
    PngReader(FileObject& file);
    virtual ~PngReader() override;
    virtual bool Validate() override;
    virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI PngWriter : public ImgWriter
{
private:
    FileObject& ImgFile;
    void *PngStruct = nullptr;
    void *PngInfo = nullptr;
public:
    PngWriter(FileObject& file);
    virtual ~PngWriter() override;
    virtual void Write(const Image& image, const uint8_t quality) override;
};

class IMGUTILAPI PngSupport : public ImgSupport
{
public:
    PngSupport() : ImgSupport(u"Png") {}
    virtual ~PngSupport() override {}
    virtual Wrapper<ImgReader> GetReader(FileObject& file, const u16string&) const override { return Wrapper<PngReader>(file).cast_dynamic<ImgReader>(); }
    virtual Wrapper<ImgWriter> GetWriter(FileObject& file, const u16string&) const override { return Wrapper<PngWriter>(file).cast_dynamic<ImgWriter>(); }
    virtual uint8_t MatchExtension(const u16string& ext, const ImageDataType, const bool) const override { return ext == u".PNG" ? 240 : 0; }
};


}