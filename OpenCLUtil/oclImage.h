#pragma once

#include "oclRely.h"
#include "oclCmdQue.h"
#include "oclMem.h"
#include "oclContext.h"
#include "oclException.h"

namespace oclu
{

class OCLWrongFormatException : public OCLException
{
public:
    EXCEPTION_CLONE_EX(OCLWrongFormatException);
    std::variant<oglu::TextureDataFormat, oglu::TextureInnerFormat> Format;
    OCLWrongFormatException(const std::u16string_view& msg, const oglu::TextureDataFormat format, const std::any& data_ = std::any())
        : OCLException(TYPENAME, CLComponent::OCLU, msg, data_), Format(format)
    { }
    OCLWrongFormatException(const std::u16string_view& msg, const oglu::TextureInnerFormat format, const std::any& data_ = std::any())
        : OCLException(TYPENAME, CLComponent::OCLU, msg, data_), Format(format)
    { }
    virtual ~OCLWrongFormatException() {}
};

namespace detail
{
using xziar::img::Image;

class OCLUAPI _oclImage : public _oclMem
{
    friend class _oclKernel;
    friend class _oclContext;
protected:
    const uint32_t Width, Height, Depth;
    const oglu::TextureDataFormat Format;
    _oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const oglu::TextureDataFormat dformat, const cl_mem id);
    _oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const oglu::TextureDataFormat dformat, cl_mem_object_type type);
    _oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const oglu::TextureDataFormat dformat, cl_mem_object_type type, const void* ptr);
    virtual void* MapObject(const oclCmdQue& que, const MapFlag mapFlag) override;
public:
    virtual ~_oclImage();
    PromiseResult<void> Write(const oclCmdQue que, const void *data, const size_t size, const bool shouldBlock = true) const;
    PromiseResult<void> Write(const oclCmdQue que, const common::AlignedBuffer& data, const bool shouldBlock = true) const
    { return Write(que, data.GetRawPtr(), data.GetSize(), shouldBlock); }
    PromiseResult<void> Write(const oclCmdQue que, const Image& image, const bool shouldBlock = true) const;

    PromiseResult<void> Read(const oclCmdQue que, void *data, const bool shouldBlock = true) const;
    PromiseResult<void> Read(const oclCmdQue que, Image& image, const bool shouldBlock = true) const;
    PromiseResult<Image> Read(const oclCmdQue que) const;
    PromiseResult<common::AlignedBuffer> ReadRaw(const oclCmdQue que) const;

    std::tuple<uint32_t, uint32_t, uint32_t> GetSize() const { return { Width, Height, Depth }; }
    oglu::TextureDataFormat GetFormat() const { return Format; }

    static bool CheckFormatCompatible(oglu::TextureDataFormat format);
    static bool CheckFormatCompatible(oglu::TextureInnerFormat format);
};

class OCLUAPI _oclImage2D : public _oclImage 
{
protected:
    using _oclImage::_oclImage;
public:
    using _oclImage::Width; using _oclImage::Height;
    _oclImage2D(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const oglu::TextureDataFormat dformat);
    _oclImage2D(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const xziar::img::ImageDataType dtype, const bool isNormalized = true)
        : _oclImage2D(ctx, flag, width, height, oglu::TexFormatUtil::ConvertDtypeFrom(dtype, isNormalized)) { }
    _oclImage2D(const oclContext& ctx, const MemFlag flag, const Image& image, const bool isNormalized = true);
    _oclImage2D(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const oglu::TextureDataFormat dformat, const void* ptr);
};

class OCLUAPI _oclImage3D : public _oclImage
{
protected:
    using _oclImage::_oclImage;
public:
    using _oclImage::Width; using _oclImage::Height; using _oclImage::Depth;
    _oclImage3D(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const oglu::TextureDataFormat dformat);
    _oclImage3D(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const xziar::img::ImageDataType dtype, const bool isNormalized = true)
        : _oclImage3D(ctx, flag, width, height, depth, oglu::TexFormatUtil::ConvertDtypeFrom(dtype, isNormalized)) { }
};


class OCLUAPI _oclGLImage2D : public _oclImage2D
{
    template<typename> friend class _oclGLObject;
private:
    _oclGLImage2D(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D& tex);
public:
    const oglu::oglTex2D GLTex;
    virtual ~_oclGLImage2D() override {}
};
class OCLUAPI _oclGLImage3D : public _oclImage3D
{
    template<typename> friend class _oclGLObject;
private:
    _oclGLImage3D(const oclContext& ctx, const MemFlag flag, const oglu::oglTex3D& tex);
public:
    const oglu::oglTex3D GLTex;
    virtual ~_oclGLImage3D() override {}
};

class OCLUAPI _oclGLInterImg2D : public _oclGLObject<_oclGLImage2D>
{
public:
    _oclGLInterImg2D(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D& tex);
};

class OCLUAPI _oclGLInterImg3D : public _oclGLObject<_oclGLImage3D>
{
public:
    _oclGLInterImg3D(const oclContext& ctx, const MemFlag flag, const oglu::oglTex3D& tex);
};


}
using oclImage = Wrapper<detail::_oclImage>;
using oclImg2D = Wrapper<detail::_oclImage2D>;
using oclImg3D = Wrapper<detail::_oclImage3D>;
//using oclGLImage = Wrapper<detail::_oclGLImage>;
using oclGLImg2D = Wrapper<detail::_oclGLImage2D>;
using oclGLImg3D = Wrapper<detail::_oclGLImage3D>;
using oclGLInterImg2D = Wrapper<detail::_oclGLInterImg2D>;
using oclGLInterImg3D = Wrapper<detail::_oclGLInterImg3D>;

}