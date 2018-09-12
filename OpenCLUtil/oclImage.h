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
public:
    const uint32_t Width, Height;
    const MemFlag Flags;
    const oglu::TextureDataFormat Format;
protected:
    const cl_mem memID;
    _oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const oglu::TextureDataFormat dformat, const cl_mem id);
public:
    _oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const oglu::TextureDataFormat dformat);
    _oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const xziar::img::ImageDataType dtype, const bool isNormalized = true)
        : _oclImage(ctx, flag, width, height, oglu::TexFormatUtil::ConvertDtypeFrom(dtype, isNormalized)) { }
    virtual ~_oclImage();
    oclPromise Read(const oclCmdQue que, Image& image, const bool shouldBlock = true) const;
    oclPromise Write(const oclCmdQue que, const Image& image, const bool shouldBlock = true) const;
    oclPromise Write(const oclCmdQue que, const common::AlignedBuffer<32>& data, const bool shouldBlock = true) const;
};

class OCLUAPI _oclGLImage : public _oclImage, public GLShared<_oclGLImage>
{
    friend class GLShared<_oclGLImage>;
private:
    const oglu::oglTex2D GlTex;
public:
    _oclGLImage(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D tex);
    virtual ~_oclGLImage() override;
};

}
using oclImage = Wrapper<detail::_oclImage>;
using oclGLImage = Wrapper<detail::_oclGLImage>;

}