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
class oclImage_;
using oclImage = std::shared_ptr<oclImage_>;
class oclImage2D_;
using oclImg2D = std::shared_ptr<oclImage2D_>;
class oclImage3D_;
using oclImg3D = std::shared_ptr<oclImage3D_>;


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


class OCLUAPI oclImage_ : public oclMem_
{
    friend class oclKernel_;
    friend class oclContext_;
protected:
    const uint32_t Width, Height, Depth;
    const xziar::img::TextureFormat Format;
    oclImage_(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const xziar::img::TextureFormat format, const cl_mem id);
    oclImage_(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const xziar::img::TextureFormat format, cl_mem_object_type type, const void* ptr = nullptr);
    virtual common::span<std::byte> MapObject(const cl_command_queue& que, const MapFlag mapFlag) override;
public:
    virtual ~oclImage_();
    common::PromiseResult<void> Write(const oclCmdQue que, const void *data, const size_t size, const bool shouldBlock = true) const;
    common::PromiseResult<void> Write(const oclCmdQue que, const common::AlignedBuffer& data, const bool shouldBlock = true) const
    { return Write(que, data.GetRawPtr(), data.GetSize(), shouldBlock); }
    common::PromiseResult<void> Write(const oclCmdQue que, const xziar::img::Image& image, const bool shouldBlock = true) const;

    common::PromiseResult<void> Read(const oclCmdQue que, void *data, const bool shouldBlock = true) const;
    common::PromiseResult<void> Read(const oclCmdQue que, xziar::img::Image& image, const bool shouldBlock = true) const;
    common::PromiseResult<xziar::img::Image> Read(const oclCmdQue que) const;
    common::PromiseResult<common::AlignedBuffer> ReadRaw(const oclCmdQue que) const;

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
    static oclImg2D Create(const oclContext& ctx, const MemFlag flag, const xziar::img::Image& image, const bool isNormalized = true)
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


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
