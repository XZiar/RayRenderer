#pragma once
#include "oglRely.h"
#include "oglTexture.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace oglu
{
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

namespace detail
{
class OGLUAPI _oglRenderBuffer : public NonMovable, public oglCtxObject<true>
{
    friend class _oglFrameBuffer;
public:
private:
    GLuint RBOId;
    const RBOFormat InnerFormat;
    const uint32_t Width, Height;
public:
    _oglRenderBuffer(const uint32_t width, const uint32_t height, const RBOFormat format);
    ~_oglRenderBuffer();
    RBOFormat GetType() const { return InnerFormat & RBOFormat::TYPE_MASK; }
};
}
using oglRBO = Wrapper<detail::_oglRenderBuffer>;

enum class FBOStatus : uint8_t
{
    Complete, Undefined, Unsupported, IncompleteAttachment, MissingAttachment, IncompleteDrawBuffer, IncompleteReadBuffer, IncompleteMultiSample, IncompleteLayerTargets, Unknown
};

class oglFBO;
namespace detail
{
class OGLUAPI _oglFrameBuffer : public NonMovable, public oglCtxObject<false>
{
    friend class _oglContext;
public:
    using FBOAttachment = variant<std::monostate, Wrapper<detail::_oglRenderBuffer>, oglTex2D, pair<oglTex2DArray, uint32_t>>;
private:
    GLuint FBOId;
    vector<FBOAttachment> ColorAttachemnts;
    FBOAttachment DepthAttachment;
    FBOAttachment StencilAttachment;
public:
    _oglFrameBuffer();
    ~_oglFrameBuffer();
    FBOStatus CheckStatus() const;
    void AttachColorTexture(const oglTex2D& tex, const uint8_t attachment);
    void AttachColorTexture(const oglTex2DArray& tex, const uint32_t layer, const uint8_t attachment);
    void AttachColorTexture(const oglRBO& rbo, const uint8_t attachment);
    void AttachDepthTexture(const oglTex2D& tex);
    void AttachDepthTexture(const oglRBO& rbo);
    void AttachStencilTexture(const oglTex2D& tex);
    void AttachStencilTexture(const oglRBO& rbo);
    void AttachDepthStencilBuffer(const oglRBO& rbo);

    void BlitColorTo(const oglFBO& to, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect);
    void BlitColorFrom(const oglFBO& from, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect);
    void Use() const;
    std::pair<GLuint, GLuint> DebugBinding() const;
};
}

class OGLUAPI oglFBO : public common::Wrapper<detail::_oglFrameBuffer>
{
public:
    using common::Wrapper<detail::_oglFrameBuffer>::Wrapper;
    static void UseDefault();
    static std::pair<GLuint, GLuint> DebugBinding(GLuint id);
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
