#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"
#include "SystemCommon/ComHelper.h"

namespace xziar::img::wic
{

struct WICFactory;
struct WICDecoder;
struct WICEncoder;

class WicSupport;

enum class ImgType : uint8_t { None = 0, JPG, PNG, BMP, TIF, JXL, HEIF, WEBP };

class WicReader : public ImgReader
{
private:
    std::shared_ptr<const WicSupport> Support;
    common::com::PtrProxy<WICDecoder> Decoder;
public:
    WicReader(std::shared_ptr<const WicSupport>&& support, common::com::PtrProxy<WICDecoder>&& decoder);
    virtual ~WicReader() override;
    [[nodiscard]] virtual bool Validate() override;
    [[nodiscard]] virtual Image Read(ImgDType dataType) override;
};

class WicWriter : public ImgWriter
{
private:
    std::shared_ptr<const WicSupport> Support;
    common::com::PtrProxy<WICEncoder> Encoder;
    ImgType Type;
public:
    WicWriter(std::shared_ptr<const WicSupport>&& support, common::com::PtrProxy<WICEncoder>&& encoder, ImgType type);
    virtual ~WicWriter() override;
    virtual void Write(ImageView image, const uint8_t quality) override;
};

class WicSupport final : public ImgSupport, public std::enable_shared_from_this<WicSupport>
{
    friend WicReader;
    friend WicWriter;
    common::com::PtrProxy<WICFactory> Factory;
public:
    WicSupport();
    ~WicSupport() final;
    [[nodiscard]] std::unique_ptr<ImgReader> GetReader(common::io::RandomInputStream& stream, std::u16string_view) const final;
    [[nodiscard]] std::unique_ptr<ImgWriter> GetWriter(common::io::RandomOutputStream& stream, std::u16string_view ext) const final;
    [[nodiscard]] uint8_t MatchExtension(std::u16string_view ext, ImgDType dataType, const bool IsRead) const final;
};


}