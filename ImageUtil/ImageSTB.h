#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::stb
{

using namespace common;

class IMGUTILAPI StbReader : public ImgReader
{
private:
    FileObject& ImgFile;
    void *StbCallback = nullptr;
    void *StbContext = nullptr;
    void *StbResult = nullptr;
public:
    StbReader(FileObject& file);
    virtual ~StbReader() override;
    virtual bool Validate() override;
    virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI StbWriter : public ImgWriter
{
private:
    FileObject& ImgFile;
public:
    StbWriter(FileObject& file);
    virtual ~StbWriter() override;
    virtual void Write(const Image& image) override;
};

class IMGUTILAPI StbSupport : public ImgSupport
{
public:
    StbSupport() : ImgSupport(u"Stb") {}
    virtual ~StbSupport() override {}
    virtual Wrapper<ImgReader> GetReader(FileObject& file) const override { return Wrapper<StbReader>(file).cast_dynamic<ImgReader>(); }
    virtual Wrapper<ImgWriter> GetWriter(FileObject& file) const override { return Wrapper<StbWriter>(file).cast_dynamic<ImgWriter>(); }
    virtual bool MatchExtension(const u16string& ext, const ImageDataType, const bool IsRead) const override 
    { return (ext == u".PPM" || ext == u".PGM") && IsRead; }
};


}