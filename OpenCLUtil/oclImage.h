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
    common::span<std::byte> MapObject(const cl_command_queue& que, const MapFlag mapFlag) override;
    [[nodiscard]] size_t CalculateSize() const;
public:
    virtual ~oclImage_();

    [[nodiscard]] common::PromiseResult<void> ReadSpan(const common::PromiseStub& pmss, const oclCmdQue& que, common::span<std::byte> buf) const;
    [[nodiscard]] common::PromiseResult<xziar::img::Image> Read(const common::PromiseStub& pmss, const oclCmdQue& que) const;
    [[nodiscard]] common::PromiseResult<common::AlignedBuffer> ReadRaw(const common::PromiseStub& pmss, const oclCmdQue& que) const;
    [[nodiscard]] common::PromiseResult<void> ReadSpan(const oclCmdQue& que, common::span<std::byte> buf) const
    {
        return ReadSpan({}, que, buf);
    }
    [[nodiscard]] common::PromiseResult<xziar::img::Image> Read(const oclCmdQue& que) const
    {
        return Read({}, que);
    }
    [[nodiscard]] common::PromiseResult<common::AlignedBuffer> ReadRaw(const oclCmdQue& que) const
    {
        return ReadRaw({}, que);
    }

    [[nodiscard]] common::PromiseResult<void> WriteSpan(const common::PromiseStub& pmss, const oclCmdQue& que, common::span<const std::byte> buf) const;
    [[nodiscard]] common::PromiseResult<void> Write(const common::PromiseStub& pmss, const oclCmdQue& que, const xziar::img::ImageView image) const;
    [[nodiscard]] common::PromiseResult<void> WriteSpan(const oclCmdQue& que, common::span<const std::byte> buf) const
    {
        return WriteSpan({}, que, buf);
    }
    [[nodiscard]] common::PromiseResult<void> Write(const oclCmdQue& que, const xziar::img::ImageView image) const
    {
        return Write({}, que, image);
    }

    [[nodiscard]] std::tuple<uint32_t, uint32_t, uint32_t> GetSize() const { return { Width, Height, Depth }; }
    [[nodiscard]] xziar::img::TextureFormat GetFormat() const { return Format; }

    [[nodiscard]] static bool CheckFormatCompatible(xziar::img::TextureFormat format);
};

class OCLUAPI oclImage2D_ : public oclImage_ 
{
protected:
    MAKE_ENABLER();
    using oclImage_::oclImage_;
    oclImage2D_(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const xziar::img::TextureFormat format, const void* ptr);
public:
    using oclImage_::Width; using oclImage_::Height;

    [[nodiscard]] static oclImg2D Create(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const xziar::img::TextureFormat format, const void* ptr = nullptr);
    [[nodiscard]] static oclImg2D Create(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const xziar::img::ImageDataType dtype, const bool isNormalized = true)
    {
        return Create(ctx, flag, width, height, xziar::img::TexFormatUtil::FromImageDType(dtype, isNormalized));
    }
    [[nodiscard]] static oclImg2D Create(const oclContext& ctx, const MemFlag flag, const xziar::img::Image& image, const bool isNormalized = true)
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
    
    [[nodiscard]] static oclImg3D Create(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const xziar::img::TextureFormat format, const void* ptr = nullptr);
    [[nodiscard]] static oclImg3D Create(const oclContext& ctx, const MemFlag flag, const uint32_t width, const uint32_t height, const uint32_t depth, const xziar::img::ImageDataType dtype, const bool isNormalized = true)
    {
        return Create(ctx, flag, width, height, depth, xziar::img::TexFormatUtil::FromImageDType(dtype, isNormalized));
    }
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
