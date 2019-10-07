#pragma once

#include "ImageUtilRely.h"
#include "ImageCore.h"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace xziar::img
{
using common::io::RandomInputStream;
using common::io::RandomOutputStream;
using common::io::BufferedRandomInputStream;
//using common::io::BufferedRandomOutputStream;


class IMGUTILAPI ImgReader : public common::NonCopyable
{
protected:
public:
    virtual ~ImgReader() {}
    virtual bool Validate() = 0;
    virtual Image Read(const ImageDataType dataType) = 0;
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
    virtual std::unique_ptr<ImgReader> GetReader(RandomInputStream& stream, const std::u16string& ext) const = 0;
    virtual std::unique_ptr<ImgWriter> GetWriter(RandomOutputStream& stream, const std::u16string& ext) const = 0;
    virtual uint8_t MatchExtension(const std::u16string& ext, const ImageDataType dataType, const bool IsRead) const = 0;
};


IMGUTILAPI uint32_t RegistImageSupport(std::shared_ptr<ImgSupport> support);
template<typename T>
inline uint32_t RegistImageSupport()
{
    return RegistImageSupport(std::make_shared<T>());
}

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif