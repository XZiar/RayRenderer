#pragma once

#include "ImageUtilRely.h"
#include "ImageSupport.hpp"


namespace xziar::img::ndk
{
class NdkSupport;

class NdkReader : public ImgReader
{
private:
    std::shared_ptr<const NdkSupport> Support;
    common::AlignedBuffer Source;
    void* Decoder;
    uint32_t Width = 0, Height = 0;
    int32_t Format = 0;
public:
    NdkReader(std::shared_ptr<const NdkSupport>&& support, common::AlignedBuffer&& src, void* decoder) noexcept;
    virtual ~NdkReader() override;
    [[nodiscard]] virtual bool Validate() override;
    [[nodiscard]] virtual Image Read(ImgDType dataType) override;
};

class NdkWriter : public ImgWriter
{
private:
    std::shared_ptr<const NdkSupport> Support;
    common::io::RandomOutputStream& Stream;
    int32_t Format = 0;
public:
    NdkWriter(std::shared_ptr<const NdkSupport>&& support, common::io::RandomOutputStream& stream, int32_t format) noexcept;
    virtual ~NdkWriter() override;
    virtual void Write(const Image& image, const uint8_t quality) override;
};

class NdkSupport final : public ImgSupport, public std::enable_shared_from_this<NdkSupport>
{
    friend NdkReader;
    friend NdkWriter;
    struct ImgDecoder;
    std::unique_ptr<ImgDecoder> Host;
public:
    NdkSupport();
    ~NdkSupport() final;
    [[nodiscard]] std::unique_ptr<ImgReader> GetReader(common::io::RandomInputStream& stream, std::u16string_view) const final;
    [[nodiscard]] std::unique_ptr<ImgWriter> GetWriter(common::io::RandomOutputStream& stream, std::u16string_view ext) const final;
    [[nodiscard]] uint8_t MatchExtension(std::u16string_view ext, ImgDType dataType, const bool isRead) const final;
};


}

