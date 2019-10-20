#include "oglPch.h"
#include "oglFBO.h"
#include "oglContext.h"
#include "oglException.h"
#include "DSAWrapper.h"


namespace oglu
{
MAKE_ENABLER_IMPL(oglRenderBuffer_)
MAKE_ENABLER_IMPL(oglFrameBuffer_)


static GLenum ParseRBOFormat(const RBOFormat format)
{
    switch (format)
    {
    case RBOFormat::Depth:              return GL_DEPTH_COMPONENT;
    case RBOFormat::Depth16:            return GL_DEPTH_COMPONENT16;
    case RBOFormat::Depth24:            return GL_DEPTH_COMPONENT24;
    case RBOFormat::Depth32:            return GL_DEPTH_COMPONENT32;
    case RBOFormat::Stencil:            return GL_STENCIL_INDEX;
    case RBOFormat::Stencil1:           return GL_STENCIL_INDEX1;
    case RBOFormat::Stencil8:           return GL_STENCIL_INDEX8;
    case RBOFormat::Stencil16:          return GL_STENCIL_INDEX16;
    case RBOFormat::Depth24Stencil8:    return GL_DEPTH24_STENCIL8;
    case RBOFormat::Depth32Stencil8:    return GL_DEPTH32F_STENCIL8;
    case RBOFormat::RGBA8:              return GL_RGBA;
    case RBOFormat::RGBA8U:             return GL_RGBA8UI;
    case RBOFormat::RGBAf:              return GL_RGBA32F;
    case RBOFormat::RGB10A2:            return GL_RGB10_A2;
    default: COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"Unknown RBOFormat.");
    }
}


oglRenderBuffer_::oglRenderBuffer_(const uint32_t width, const uint32_t height, const RBOFormat format)
    : RBOId(GL_INVALID_INDEX), InnerFormat(format), Width(width), Height(height)
{
    glGenRenderbuffers(1, &RBOId);
    glNamedRenderbufferStorageEXT(RBOId, ParseRBOFormat(InnerFormat), Width, Height);
}
oglRBO oglRenderBuffer_::Create(const uint32_t width, const uint32_t height, const RBOFormat format)
{
    return MAKE_ENABLER_SHARED(oglRenderBuffer_, (width, height, format));
}

oglRenderBuffer_::~oglRenderBuffer_()
{
    if (!EnsureValid()) return;
    glDeleteRenderbuffers(1, &RBOId);
}


oglFrameBuffer_::oglFrameBuffer_() : FBOId(GL_INVALID_INDEX)
{
    DSA->ogluCreateFramebuffers(1, &FBOId);
    GLint maxAttach;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttach);
    ColorAttachemnts.resize(maxAttach);
}
oglFBO oglFrameBuffer_::Create()
{
    return MAKE_ENABLER_SHARED(oglFrameBuffer_, ());
}

oglFrameBuffer_::~oglFrameBuffer_()
{
    if (!EnsureValid()) return;
    glDeleteFramebuffers(1, &FBOId);
}

FBOStatus oglFrameBuffer_::CheckStatus() const
{
    CheckCurrent();
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

void oglFrameBuffer_::AttachColorTexture(const oglTex2D& tex, const uint8_t attachment)
{
    CheckCurrent();
    DSA->ogluFramebufferTexture(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D, tex->textureID, 0);
    ColorAttachemnts[attachment] = tex;
}

void oglFrameBuffer_::AttachColorTexture(const oglTex2DArray& tex, const uint32_t layer, const uint8_t attachment)
{
    if (layer >= tex->Layers)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"texture layer overflow");
    CheckCurrent();
    DSA->ogluFramebufferTextureLayer(FBOId, GL_COLOR_ATTACHMENT0 + attachment, tex->textureID, 0, layer);
    ColorAttachemnts[attachment] = std::make_pair(tex, layer);
}

void oglFrameBuffer_::AttachColorTexture(const oglRBO& rbo, const uint8_t attachment)
{
    if (rbo->GetType() != RBOFormat::TYPE_COLOR)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"rbo type missmatch");
    CheckCurrent();
    DSA->ogluFramebufferRenderbuffer(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_RENDERBUFFER, rbo->RBOId);
    ColorAttachemnts[attachment] = rbo;
}

void oglFrameBuffer_::AttachDepthTexture(const oglTex2D& tex)
{
    CheckCurrent();
    DSA->ogluFramebufferTextureLayer(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex->textureID, 0);
    DepthAttachment = tex;
}

void oglFrameBuffer_::AttachDepthTexture(const oglRBO& rbo)
{
    if (rbo->GetType() != RBOFormat::TYPE_DEPTH)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"rbo type missmatch");
    CheckCurrent();
    DSA->ogluFramebufferRenderbuffer(FBOId, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo->RBOId);
    DepthAttachment = rbo;
}

void oglFrameBuffer_::AttachStencilTexture(const oglTex2D& tex)
{
    CheckCurrent();
    DSA->ogluFramebufferTexture(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, tex->textureID, 0);
    StencilAttachment = tex;
}

void oglFrameBuffer_::AttachStencilTexture(const oglRBO& rbo)
{
    if (rbo->GetType() != RBOFormat::TYPE_STENCIL)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"rbo type missmatch");
    CheckCurrent();
    DSA->ogluFramebufferRenderbuffer(FBOId, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo->RBOId);
    StencilAttachment = rbo;
}

void oglFrameBuffer_::AttachDepthStencilBuffer(const oglRBO& rbo)
{
    if (rbo->GetType() != RBOFormat::TYPE_DEPTH_STENCIL)
        COMMON_THROW(OGLException, OGLException::GLComponent::OGLU, u"rbo type missmatch");
    CheckCurrent();
    DSA->ogluFramebufferRenderbuffer(FBOId, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo->RBOId);
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

void oglFrameBuffer_::BlitColorTo(const oglFBO& to, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    CheckCurrent();
    if (to)
        to->CheckCurrent();
    BlitColor(FBOId, to ? to->FBOId : 0, rect);
}
void oglFrameBuffer_::BlitColorFrom(const oglFBO& from, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    CheckCurrent();
    if (from)
        from->CheckCurrent();
    BlitColor(from ? from->FBOId : 0, FBOId, rect);
}

struct FBOCtxConfig : public CtxResConfig<false, GLuint>
{
    GLuint Construct() const { return 0; }
};
static FBOCtxConfig FBO_CTXCFG;
GLuint GetCurFBO()
{
    return oglContext_::CurrentContext()->GetOrCreate<false>(FBO_CTXCFG);
}
void oglFrameBuffer_::Use() const
{
    CheckCurrent();
    auto& fbo = oglContext_::CurrentContext()->GetOrCreate<false>(FBO_CTXCFG);
    if (FBOId != fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, fbo = FBOId);
}

std::pair<GLuint, GLuint> oglFrameBuffer_::DebugBinding() const
{
    return oglFrameBuffer_::DebugBinding(FBOId);
}


void oglFrameBuffer_::UseDefault()
{
    auto& fbo = oglContext_::CurrentContext()->GetOrCreate<false>(FBO_CTXCFG);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo = 0);
}

std::pair<GLuint, GLuint> oglFrameBuffer_::DebugBinding(GLuint id)
{
    GLint clrId = 0, depId = 0;
    glGetNamedFramebufferAttachmentParameterivEXT(id, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &clrId);
    glGetNamedFramebufferAttachmentParameterivEXT(id, GL_DEPTH_ATTACHMENT , GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depId);
    return std::pair<GLuint, GLuint>(clrId, depId);
}

}