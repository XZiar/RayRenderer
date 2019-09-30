#pragma once

#include "oclRely.h"
#include "oclCmdQue.h"
#include "oclMem.h"
#include "oclContext.h"
#include "oclException.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{

class OCLWrongFormatException : public OCLException
{
public:
    EXCEPTION_CLONE_EX(OCLWrongFormatException);
    xziar::img::TextureFormat Format;
    OCLWrongFormatException(const std::u16string_view& msg, const xziar::img::TextureFormat format, const std::any& data_ = std::any())
        : OCLException(TYPENAME, CLComponent::OCLU, msg, data_), Format(format)
    { }
    virtual ~OCLWrongFormatException() {}
};

using xziar::img::Image;

class OCLUAPI oclImage_ : public oclMem_
{
    friend class oclKernel_;
    friend class oclContext_;
protected:
    const uint32_t Width, Height, Depth;
    const xziar::img::TextureFormat Format;
    oclImage_(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const xziar::img::TextureFormat format, const cl_mem id);
    oclImage_(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const xziar::img::TextureFormat format, cl_mem_object_type type, const void* ptr = nullptr);
    virtual void* MapObject(const oclCmdQue& que, const MapFlag mapFlag) override;
public:
    virtual ~oclImage_();
    PromiseResult<void> Write(const oclCmdQue que, const void *data, const size_t size, const bool shouldBlock = true) const;
    PromiseResult<void> Write(const oclCmdQue que, const common::AlignedBuffer& data, const bool shouldBlock = true) const
    { return Write(que, data.GetRawPtr(), data.GetSize(), shouldBlock); }
    PromiseResult<void> Write(const oclCmdQue que, const Image& image, const bool shouldBlock = true) const;

    PromiseResult<void> Read(const oclCmdQue que, void *data, const bool shouldBlock = true) const;
    PromiseResult<void> Read(const oclCmdQue que, Image& image, const bool shouldBlock = true) const;
    PromiseResult<Image> Read(const oclCmdQue que) const;
    PromiseResult<common::AlignedBuffer> ReadRaw(const oclCmdQue que) const;

    std::tuple<uint32_t, uint32_t, uint32_t> GetSize() const { return { Width, Height, Depth }; }
    xziar::img::TextureFormat GetFormat() const { return Format; }

    static bool CheckFormatCompatible(xziar::img::TextureFormat format);
};

class OCLUAPI oclImage2D_ : public oclImage_ 
{
protected:
    MAKE_ENABLER();
    using oclImage_::oclImage_;
    oclImage2D_(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const xziar::img::TextureFormat format, const void* ptr);
public:
    using oclImage_::Width; using oclImage_::Height;

    static oclImg2D Create(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const xziar::img::TextureFormat format, const void* ptr = nullptr);
    static oclImg2D Create(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const xziar::img::ImageDataType dtype, const bool isNormalized = true)
    {
        return Create(ctx, flag, width, height, xziar::img::TexFormatUtil::FromImageDType(dtype, isNormalized));
    }
    static oclImg2D Create(const oclContext& ctx, const MemFlag flag, const Image& image, const bool isNormalized = true)
    {
        return Create(ctx, flag, image.GetWidth(), image.GetHeight(), xziar::img::TexFormatUtil::FromImageDType(image.GetDataType(), isNormalized), image.GetRawPtr());
    }
};

class OCLUAPI oclImage3D_ : public oclImage_
{
protected:
    MAKE_ENABLER();
    using oclImage_::oclImage_;
    oclImage3D_(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const xziar::img::TextureFormat format, const void* ptr);
public:
    using oclImage_::Width; using oclImage_::Height; using oclImage_::Depth;
    
    static oclImg3D Create(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const xziar::img::TextureFormat format, const void* ptr = nullptr);
    static oclImg3D Create(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const xziar::img::ImageDataType dtype, const bool isNormalized = true)
    {
        return Create(ctx, flag, width, height, depth, xziar::img::TexFormatUtil::FromImageDType(dtype, isNormalized));
    }
};


class OCLUAPI oclGLImage2D_ : public oclImage2D_
{
    friend class oclGLInterImg2D_;
private:
    MAKE_ENABLER();
    oclGLImage2D_(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D& tex);
public:
    const oglu::oglTex2D GLTex;
    virtual ~oclGLImage2D_() override;
};
MAKE_ENABLER_IMPL(oclGLImage2D_)
class OCLUAPI oclGLImage3D_ : public oclImage3D_
{
    friend class oclGLInterImg3D_;
private:
    MAKE_ENABLER();
    oclGLImage3D_(const oclContext& ctx, const MemFlag flag, const oglu::oglTex3D& tex);
public:
    const oglu::oglTex3D GLTex;
    virtual ~oclGLImage3D_() override;
};
MAKE_ENABLER_IMPL(oclGLImage3D_)

class OCLUAPI oclGLInterImg2D_ : public oclGLObject_<oclGLImage2D_>
{
private:
    MAKE_ENABLER();
    oclGLInterImg2D_(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D& tex);
public:
    oglu::oglTex2D GetGLTex() const { return CLObject->GLTex; }
    
    static oclGLInterImg2D Create(const oclContext& ctx, const MemFlag flag, const oglu::oglTex2D& tex);
};

class OCLUAPI oclGLInterImg3D_ : public oclGLObject_<oclGLImage3D_>
{
private:
    MAKE_ENABLER();
    oclGLInterImg3D_(const oclContext& ctx, const MemFlag flag, const oglu::oglTex3D& tex);
public:
    oglu::oglTex3D GetGLTex() const { return CLObject->GLTex; }
    
    static oclGLInterImg3D Create(const oclContext& ctx, const MemFlag flag, const oglu::oglTex3D& tex);
};


//using oclGLImage      = std::shared_ptr<_oclGLImage>;
using oclGLImg2D        = std::shared_ptr<oclGLImage2D_>;
using oclGLImg3D        = std::shared_ptr<oclGLImage3D_>;

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
