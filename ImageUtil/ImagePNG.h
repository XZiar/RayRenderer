#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::png
{

using namespace common;

class IMGUTILAPI PngReader : public ImgReader
{
private:
    RandomInputStream& Stream;
    void *PngStruct = nullptr;
    void *PngInfo = nullptr;
public:
    PngReader(RandomInputStream& stream);
    virtual ~PngReader() override;
    virtual bool Validate() override;
    virtual Image Read(const ImageDataType dataType) override;
};

class IMGUTILAPI PngWriter : public ImgWriter
{
private:
    RandomOutputStream& Stream;
    void *PngStruct = nullptr;
    void *PngInfo = nullptr;
public:
    PngWriter(RandomOutputStream& stream);
    virtual ~PngWriter() override;
    virtual void Write(const Image& image, const uint8_t quality) override;
};

class IMGUTILAPI PngSupport : public ImgSupport
{
public:
    PngSupport() : ImgSupport(u"Png") {}
    virtual ~PngSupport() override {}
    virtual std::unique_ptr<ImgReader> GetReader(RandomInputStream& stream, const u16string&) const override
    {
        return std::make_unique<PngReader>(stream);
    }
    virtual std::unique_ptr<ImgWriter> GetWriter(RandomOutputStream& stream, const u16string&) const override
    {
        return std::make_unique<PngWriter>(stream);
    }
    virtual uint8_t MatchExtension(const u16string& ext, const ImageDataType, const bool) const override 
    { 
        return ext == u"PNG" ? 240 : 0;
    }
};


}