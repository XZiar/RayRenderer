#pragma once
#include "oglRely.h"
#include "oglTexture.h"


namespace oglu
{
enum class RBOFormat : GLenum
{
    Depth = GL_DEPTH_COMPONENT, Depth16 = GL_DEPTH_COMPONENT16, Depth24 = GL_DEPTH_COMPONENT24, Depth32 = GL_DEPTH_COMPONENT32,
    Stencil = GL_STENCIL_INDEX, Stencil1 = GL_STENCIL_INDEX1, Stencil8 = GL_STENCIL_INDEX8, Stencil16 = GL_STENCIL_INDEX16,
    Depth24Stencil8 = GL_DEPTH24_STENCIL8,
    RGBA8 = GL_RGBA, RGBA8U = GL_RGBA8UI, RGBAf = GL_RGBA32F, RGB10A2 = GL_RGB10_A2
};

namespace detail
{
class OGLUAPI _oglRenderBuffer : public NonCopyable, public NonMovable
{
    friend class _oglFrameBuffer;
private:
    GLuint RBOId = GL_INVALID_INDEX;
    const RBOFormat InnerFormat;
    const uint32_t Width, Height;
public:
    _oglRenderBuffer(const uint32_t width, const uint32_t height, const RBOFormat format);
    ~_oglRenderBuffer();
};
}
using oglRBO = Wrapper<detail::_oglRenderBuffer>;

namespace detail
{
class OGLUAPI _oglFrameBuffer : public NonCopyable, public NonMovable
{
    friend class _oglContext;
public:
    using FBOAttachment = variant<std::monostate, Wrapper<detail::_oglRenderBuffer>, oglTex2D, pair<oglTex2DArray, uint32_t>>;
private:
    GLuint FBOId = GL_INVALID_INDEX;
    vector<FBOAttachment> ColorAttachemnts;
    FBOAttachment DepthAttachment;
    FBOAttachment StencilAttachment;
public:
    _oglFrameBuffer();
    ~_oglFrameBuffer();
    bool CheckIsComplete() const;
    void AttackTexture(const oglTex2D& tex, const uint8_t attachment);
    void AttackTexture(const oglTex2DArray& tex, const uint32_t layer, const uint8_t attachment);
    void AttackRBO(const oglRBO& rbo, const uint8_t attachment);
    void AttackDepthTexture(const oglTex2D& tex);
    void AttackDepthTexture(const oglRBO& rbo);
    void AttackStencilTexture(const oglTex2D& tex);
    void AttackStencilTexture(const oglRBO& rbo);
};
}
using oglFBO = Wrapper<detail::_oglFrameBuffer>;

}
