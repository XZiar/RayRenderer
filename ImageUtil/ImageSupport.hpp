#pragma once

#include "ImageUtilRely.h"
#include "ImageCore.h"

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace xziar::img
{

class IMGUTILAPI ImgReader : public common::NonCopyable
{
protected:
public:
    virtual ~ImgReader() {}
    [[nodiscard]] virtual bool Validate() = 0;
    [[nodiscard]] virtual Image Read(ImgDType dataType) = 0;
};

class IMGUTILAPI ImgWriter : public common::NonCopyable
{
public:
    virtual ~ImgWriter() {};
    virtual void Write(const Image& image, const uint8_t quality) = 0;
};

class IMGUTILAPI ImgSupport
{
protected:
    ImgSupport(const std::u16string& name) : Name(name) {}
    virtual ~ImgSupport() {}
public:
    const std::u16string Name;
    [[nodiscard]] virtual std::unique_ptr<ImgReader> GetReader(common::io::RandomInputStream& stream, std::u16string_view ext) const = 0;
    [[nodiscard]] virtual std::unique_ptr<ImgWriter> GetWriter(common::io::RandomOutputStream& stream, std::u16string_view ext) const = 0;
    [[nodiscard]] virtual uint8_t MatchExtension(std::u16string_view ext, ImgDType dataType, const bool IsRead) const = 0;
};


IMGUTILAPI uint32_t RegistImageSupport(std::shared_ptr<ImgSupport> support) noexcept;
IMGUTILAPI bool UnRegistImageSupport(const std::shared_ptr<ImgSupport>& support) noexcept;
template<typename T>
inline uint32_t RegistImageSupport()
{
    return RegistImageSupport(std::make_shared<T>());
}

IMGUTILAPI std::vector<std::shared_ptr<const ImgSupport>> GetImageSupport(std::u16string_view ext, ImgDType dataType, const bool isRead) noexcept;

}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif