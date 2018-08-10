#include "oglRely.h"
#include "oglFBO.h"
#include "oglException.h"


namespace oglu::detail
{

static _oglRenderBuffer::RBOType ParseType(const RBOFormat format)
{
    switch (format)
    {
    case RBOFormat::Depth:
    case RBOFormat::Depth16:
    case RBOFormat::Depth24:
    case RBOFormat::Depth32:
        return _oglRenderBuffer::RBOType::Depth;
    case RBOFormat::Stencil:
    case RBOFormat::Stencil1:
    case RBOFormat::Stencil8:
    case RBOFormat::Stencil16:
        return _oglRenderBuffer::RBOType::Stencil;
    case RBOFormat::Depth24Stencil8:
    case RBOFormat::Depth32Stencil8:
        return _oglRenderBuffer::RBOType::DepthStencil;
    case RBOFormat::RGBA8:
    case RBOFormat::RGBA8U:
    case RBOFormat::RGBAf:
    case RBOFormat::RGB10A2:
        return _oglRenderBuffer::RBOType::Color;
    default:
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"invalid RBO format");
    }
}

_oglRenderBuffer::_oglRenderBuffer(const uint32_t width, const uint32_t height, const RBOFormat format)
    : InnerFormat(format), Width(width), Height(height), Type(ParseType(format))
{
    glGenRenderbuffers(1, &RBOId);
    glNamedRenderbufferStorageEXT(RBOId, (GLenum)InnerFormat, Width, Height);
}

_oglRenderBuffer::~_oglRenderBuffer()
{
    glDeleteRenderbuffers(1, &RBOId);
}

_oglFrameBuffer::_oglFrameBuffer()
{
    glGenFramebuffers(1, &FBOId);
    GLint maxAttach;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttach);
    ColorAttachemnts.resize(maxAttach);
}

_oglFrameBuffer::~_oglFrameBuffer()
{
    glDeleteFramebuffers(1, &FBOId);
}

FBOStatus _oglFrameBuffer::CheckStatus() const
{
    switch (glCheckNamedFramebufferStatusEXT(FBOId, GL_FRAMEBUFFER))
    {
    case GL_FRAMEBUFFER_COMPLETE:
        return FBOStatus::Complete;
    case GL_FRAMEBUFFER_UNDEFINED:
        return FBOStatus::Undefined;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return FBOStatus::Unsupported;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return FBOStatus::IncompleteAttachment;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        return FBOStatus::IncompleteDrawBuffer;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return FBOStatus::MissingAttachment;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        return FBOStatus::IncompleteReadBuffer;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return FBOStatus::IncompleteMultiSample;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        return FBOStatus::IncompleteLayerTargets;
    default:
        return FBOStatus::Unknown;
    }
}

void _oglFrameBuffer::AttachColorTexture(const oglTex2D& tex, const uint8_t attachment)
{
    glNamedFramebufferTexture2DEXT(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D, tex->textureID, 0);
    ColorAttachemnts[attachment] = tex;
}

void _oglFrameBuffer::AttachColorTexture(const oglTex2DArray& tex, const uint32_t layer, const uint8_t attachment)
{
    if (layer >= tex->Layers)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture layer overflow");
    glNamedFramebufferTextureLayerEXT(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D_ARRAY, 0, layer);
    ColorAttachemnts[attachment] = std::make_pair(tex, layer);
}

void _oglFrameBuffer::AttachColorTexture(const oglRBO& rbo, const uint8_t attachment)
{
    if (rbo->Type != _oglRenderBuffer::RBOType::Color)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"rbo type missmatch");
    glNamedFramebufferRenderbufferEXT(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_RENDERBUFFER, rbo->RBOId);
    ColorAttachemnts[attachment] = rbo;
}

void _oglFrameBuffer::AttachDepthTexture(const oglTex2D& tex)
{
    glNamedFramebufferTexture2DEXT(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex->textureID, 0);
    DepthAttachment = tex;
}

void _oglFrameBuffer::AttachDepthTexture(const oglRBO& rbo)
{
    if (rbo->Type != _oglRenderBuffer::RBOType::Depth)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"rbo type missmatch");
    glNamedFramebufferRenderbufferEXT(FBOId, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo->RBOId);
    DepthAttachment = rbo;
}

void _oglFrameBuffer::AttachStencilTexture(const oglTex2D& tex)
{
    glNamedFramebufferTexture2DEXT(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, tex->textureID, 0);
    StencilAttachment = tex;
}

void _oglFrameBuffer::AttachStencilTexture(const oglRBO& rbo)
{
    if (rbo->Type != _oglRenderBuffer::RBOType::Stencil)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"rbo type missmatch");
    glNamedFramebufferRenderbufferEXT(FBOId, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo->RBOId);
    StencilAttachment = rbo;
}

void _oglFrameBuffer::AttachDepthStencilBuffer(const oglRBO& rbo)
{
    if (rbo->Type != _oglRenderBuffer::RBOType::DepthStencil)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"rbo type missmatch");
    glNamedFramebufferRenderbufferEXT(FBOId, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo->RBOId);
    DepthAttachment = rbo; StencilAttachment = rbo;
}

static void BlitColor(const GLuint from, const GLuint to, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    GLint drawFboId = 0, readFboId = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, from);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, to);
    const auto&[x0, y0, x1, y1] = rect;
    glBlitFramebuffer(x0, y0, x1, y1, x0, y0, x1, y1, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, readFboId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFboId);
}

void _oglFrameBuffer::BlitColorTo(const oglFBO& to, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    BlitColor(FBOId, to ? to->FBOId : 0, rect);
}
void _oglFrameBuffer::BlitColorFrom(const oglFBO& from, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    BlitColor(from ? from->FBOId : 0, FBOId, rect);
}

}