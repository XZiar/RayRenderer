#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::libjpeg
{
using namespace common;
struct JpegHelper;


class JpegReader : public ImgReader
{
    friend JpegHelper;
private:
    struct StreamBlock;
    std::unique_ptr<StreamBlock> Block;
    void *JpegDecompStruct = nullptr;
    void *JpegSource = nullptr;
    void *JpegErrorHandler = nullptr;
public:
    JpegReader(common::io::RandomInputStream& stream);
    virtual ~JpegReader() override;
    [[nodiscard]] virtual bool Validate() override;
    [[nodiscard]] virtual Image Read(ImgDType dataType) override;
};

class JpegWriter : public ImgWriter
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
    virtual void Write(ImageView image, const uint8_t quality) override;
};

class JpegSupport final : public ImgSupport
{
public:
    JpegSupport() : ImgSupport(u"LibjpegTurbo") {}
    ~JpegSupport() final {}
    [[nodiscard]] std::unique_ptr<ImgReader> GetReader(common::io::RandomInputStream& stream, std::u16string_view) const final
    {
        return std::make_unique<JpegReader>(stream);
    }
    [[nodiscard]] std::unique_ptr<ImgWriter> GetWriter(common::io::RandomOutputStream& stream, std::u16string_view) const final
    {
        return std::make_unique<JpegWriter>(stream);
    }
    [[nodiscard]] uint8_t MatchExtension(std::u16string_view ext, ImgDType, const bool) const final
    { 
        return (ext == u"JPEG" || ext == u"JPG") ? 240 : 0;
    }
};

}