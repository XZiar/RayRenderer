#pragma once

#include "oclRely.h"
#include "oclPromiseTask.h"
#include "oclCmdQue.h"
#include "oclBuffer.h"
#include "oclContext.h"

namespace oclu
{

namespace detail
{

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
    _oclImage(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const xziar::img::ImageDataType dtype, const bool isNormalized = true);
    virtual ~_oclImage();
    oclPromise Read(const oclCmdQue que, xziar::img::Image& image, const bool shouldBlock = true) const;
    oclPromise Write(const oclCmdQue que, const xziar::img::Image& image, const bool shouldBlock = true) const;
};

class OCLUAPI _oclGLImage : public _oclImage, public GLShared<_oclGLBuffer>
{
private:
    const oglu::oglTex2D tex;
public:
    _oclGLImage(const oclContext& ctx, const MemFlag flag, const oglu::TextureDataFormat dformat, const oglu::oglBuffer buf_);
    _oclGLImage(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D tex_);
    virtual ~_oclGLImage() override;
};

}
using oclImage = Wrapper<detail::_oclImage>;
using oclGLImage = Wrapper<detail::_oclGLImage>;

}