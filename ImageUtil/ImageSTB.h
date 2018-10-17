#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::stb
{

using namespace common;
enum class ImgType : uint8_t { None = 0, PNM, JPG, PNG, BMP, GIF, PSD, PIC, TGA };

class IMGUTILAPI StbReader : public ImgReader
{
private:
    FileObject& ImgFile;
    void *StbContext = nullptr;
    ImgType TestedType = ImgType::None;
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
    ImgType TargetType = ImgType::None;
public:
    StbWriter(FileObject& file);
    virtual ~StbWriter() override;
    virtual void Write(const Image& image, const uint8_t quality) override;
};

class IMGUTILAPI StbSupport : public ImgSupport
{
public:
    StbSupport() : ImgSupport(u"Stb") {}
    virtual ~StbSupport() override {}
    virtual Wrapper<ImgReader> GetReader(FileObject& file) const override { return Wrapper<StbReader>(file).cast_dynamic<ImgReader>(); }
    virtual Wrapper<ImgWriter> GetWriter(FileObject& file) const override { return Wrapper<StbWriter>(file).cast_dynamic<ImgWriter>(); }
    virtual uint8_t MatchExtension(const u16string& ext, const ImageDataType, const bool IsRead) const override;
};


}