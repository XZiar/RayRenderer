#pragma once
#include "oglRely.h"
#include "oglTexture.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace oglu
{
class oglRenderBuffer_;
using oglRBO = std::shared_ptr<oglRenderBuffer_>;
class oglFrameBuffer_;
using oglFBO = std::shared_ptr<oglFrameBuffer_>;
class oglFrameBuffer2D_;
using oglFBO2D = std::shared_ptr<oglFrameBuffer2D_>;
class oglFrameBuffer3D_;
using oglFBO3D = std::shared_ptr<oglFrameBuffer3D_>;



enum class RBOFormat : uint8_t
{
    TYPE_MASK = 0xf0,
    TYPE_DEPTH = 0x10, TYPE_STENCIL = 0x20, TYPE_DEPTH_STENCIL = 0x30, TYPE_COLOR = 0x40,
    Depth   = TYPE_DEPTH   | 0x0, Depth16  = TYPE_DEPTH   | 0x1, Depth24  = TYPE_DEPTH   | 0x2, Depth32   = TYPE_DEPTH   | 0x3,
    Stencil = TYPE_STENCIL | 0x0, Stencil1 = TYPE_STENCIL | 0x1, Stencil8 = TYPE_STENCIL | 0x2, Stencil16 = TYPE_STENCIL | 0x3,
    RGBA8   = TYPE_COLOR   | 0x0, RGBA8U   = TYPE_COLOR   | 0x1, RGBAf    = TYPE_COLOR   | 0x2, RGB10A2   = TYPE_COLOR   | 0x3,
    Depth24Stencil8 = TYPE_DEPTH_STENCIL | 0x0, Depth32Stencil8 = TYPE_DEPTH_STENCIL | 0x1,
};
MAKE_ENUM_BITFIELD(RBOFormat)

enum class FBOStatus : uint8_t
{
    Complete, Undefined, Unsupported, IncompleteAttachment, MissingAttachment, IncompleteDrawBuffer, IncompleteReadBuffer, IncompleteMultiSample, IncompleteLayerTargets, Unknown
};


class OGLUAPI oglRenderBuffer_ : public common::NonMovable, public detail::oglCtxObject<true>
{
    friend class oglFrameBuffer_;
private:
    MAKE_ENABLER();
    std::u16string Name;
    GLuint RBOId;
    const RBOFormat InnerFormat;
    const uint32_t Width, Height;
    oglRenderBuffer_(const uint32_t width, const uint32_t height, const RBOFormat format);
public:
    ~oglRenderBuffer_();
    
    void SetName(std::u16string name) noexcept;
   
    RBOFormat GetType() const noexcept { return InnerFormat & RBOFormat::TYPE_MASK; }
    std::u16string_view GetName() const noexcept { return Name; }

    static oglRBO Create(const uint32_t width, const uint32_t height, const RBOFormat format);
};


class OGLUAPI oglFrameBuffer_ : public common::NonMovable, public detail::oglCtxObject<false>
{
    friend class oglContext_;
public:
    using FBOAttachment = std::variant<std::monostate, oglRBO, oglTex2D, oglTex3D, oglTex2DArray, std::pair<oglTex2DArray, uint32_t>>;
protected:
    std::u16string Name;
    GLuint FBOId;
    std::vector<FBOAttachment> ColorAttachemnts;
    FBOAttachment DepthAttachment;
    FBOAttachment StencilAttachment;
    oglFrameBuffer_();
    static GLuint GetID(const oglRBO& rbo);
    static GLuint GetID(const oglTexBase& tex);
public:
    ~oglFrameBuffer_();

    void SetName(std::u16string name) noexcept;

    FBOStatus CheckStatus() const;
    void Use() const;
    std::pair<GLuint, GLuint> DebugBinding() const;
    std::u16string_view GetName() const noexcept { return Name; }

    static void UseDefault();
    static std::pair<GLuint, GLuint> DebugBinding(GLuint id);
};


class OGLUAPI oglFrameBuffer2D_ : public oglFrameBuffer_
{
    friend class oglContext_;
private:
    MAKE_ENABLER();
    oglFrameBuffer2D_();
public:
    ~oglFrameBuffer2D_();
    void AttachColorTexture(const oglTex2D& tex, const uint8_t attachment);
    void AttachColorTexture(const oglTex2DArray& tex, const uint32_t layer, const uint8_t attachment);
    void AttachColorTexture(const oglRBO& rbo, const uint8_t attachment);
    void AttachDepthTexture(const oglTex2D& tex);
    void AttachDepthTexture(const oglRBO& rbo);
    void AttachStencilTexture(const oglTex2D& tex);
    void AttachStencilTexture(const oglRBO& rbo);
    void AttachDepthStencilBuffer(const oglRBO& rbo);

    void BlitColorTo(const oglFBO2D& to, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect);
    void BlitColorFrom(const oglFBO2D& from, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect);

    static oglFBO2D Create();
};


class OGLUAPI oglFrameBuffer3D_ : public oglFrameBuffer_
{
    friend class oglContext_;
private:
    MAKE_ENABLER();
    uint32_t LayerCount = 0;
    oglFrameBuffer3D_();
    void CheckLayerMatch(const uint32_t layer);
public:
    ~oglFrameBuffer3D_();
    void AttachColorTexture(const oglTex3D& tex, const uint8_t attachment);
    void AttachColorTexture(const oglTex2DArray& tex, const uint8_t attachment);
    void AttachDepthTexture(const oglTex3D& tex);
    void AttachDepthTexture(const oglTex2DArray& tex);
    void AttachStencilTexture(const oglTex3D& tex);
    void AttachStencilTexture(const oglTex2DArray& tex);

    static oglFBO3D Create();
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
