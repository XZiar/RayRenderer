#pragma once

#include "oclRely.h"
#include "oclPromiseTask.h"
#include "oclCmdQue.h"
#include "oclBuffer.h"
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

class OCLUAPI _oclImage : public NonCopyable, public NonMovable
{
    friend class _oclKernel;
    friend class _oclContext;
protected:
    const oclContext Context;
    const uint32_t Width, Height, Depth;
    const MemFlag Flags;
    const oglu::TextureDataFormat Format;
    const cl_mem memID;
    _oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const oglu::TextureDataFormat dformat, const cl_mem id);
    _oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const oglu::TextureDataFormat dformat, cl_mem_object_type type);
    static oclPromise ReturnPromise(cl_event e, const oclCmdQue que);
    oclPromise Write(const oclCmdQue que, const void *data, const bool shouldBlock = true) const;
    oclPromise Read(const oclCmdQue que, void *data, const bool shouldBlock = true) const;
public:
    virtual ~_oclImage();
    oclPromise Write(const oclCmdQue que, const common::AlignedBuffer<32>& data, const bool shouldBlock = true) const
    {
        return Write(que, data.GetRawPtr(), shouldBlock);
    }
    std::tuple<uint32_t, uint32_t, uint32_t> GetSize() const { return { Width,Height, Depth }; }
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
    oclPromise Read(const oclCmdQue que, Image& image, const bool shouldBlock = true) const;
    oclPromise Write(const oclCmdQue que, const Image& image, const bool shouldBlock = true) const;
    using _oclImage::Write;
};


class OCLUAPI _oclGLImage2D : public _oclImage2D, public GLShared<_oclGLImage2D>
{
    friend class GLShared<_oclGLImage2D>;
private:
    const oglu::oglTex2D GlTex;
public:
    _oclGLImage2D(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D& tex);
    virtual ~_oclGLImage2D() override {}
};

}
using oclImage = Wrapper<detail::_oclImage>;
using oclImg2D = Wrapper<detail::_oclImage2D>;
//using oclGLImage = Wrapper<detail::_oclGLImage>;
using oclGLImg2D = Wrapper<detail::_oclGLImage2D>;

}