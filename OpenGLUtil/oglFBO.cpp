#include "oglPch.h"
#include "oglFBO.h"
#include "oglContext.h"
#include "oglException.h"
#include "DSAWrapper.h"


namespace oglu
{
MAKE_ENABLER_IMPL(oglRenderBuffer_)
MAKE_ENABLER_IMPL(oglFrameBuffer2D_)
MAKE_ENABLER_IMPL(oglFrameBuffer3D_)


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
    default: COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Unknown RBOFormat.");
    }
}


oglRenderBuffer_::oglRenderBuffer_(const uint32_t width, const uint32_t height, const RBOFormat format)
    : RBOId(GL_INVALID_INDEX), InnerFormat(format), Width(width), Height(height)
{
    DSA->ogluCreateRenderbuffers(1, &RBOId);
    DSA->ogluNamedRenderbufferStorage(RBOId, ParseRBOFormat(InnerFormat), Width, Height);
}
oglRBO oglRenderBuffer_::Create(const uint32_t width, const uint32_t height, const RBOFormat format)
{
    return MAKE_ENABLER_SHARED(oglRenderBuffer_, (width, height, format));
}

oglRenderBuffer_::~oglRenderBuffer_()
{
    if (!EnsureValid()) return;
    DSA->ogluDeleteRenderbuffers(1, &RBOId);
}

void oglRenderBuffer_::SetName(std::u16string name) noexcept
{
    Name = std::move(name);
    DSA->ogluSetObjectLabel(GL_RENDERBUFFER, RBOId, Name);
}


oglFrameBuffer_::oglFrameBuffer_() : FBOId(GL_INVALID_INDEX)
{
    DSA->ogluCreateFramebuffers(1, &FBOId);
    ColorAttachemnts.resize(DSA->MaxColorAttachment);
}

oglFrameBuffer_::~oglFrameBuffer_()
{
    if (!EnsureValid()) return;
    DSA->ogluDeleteFramebuffers(1, &FBOId);
}

void oglFrameBuffer_::SetName(std::u16string name) noexcept
{
    Name = std::move(name);
    DSA->ogluSetObjectLabel(GL_FRAMEBUFFER, FBOId, Name);
}

GLuint oglFrameBuffer_::GetID(const oglRBO& rbo)
{
    return rbo->RBOId;
}
GLuint oglFrameBuffer_::GetID(const oglTexBase& tex)
{
    return tex->TextureID;
}

FBOStatus oglFrameBuffer_::CheckStatus() const
{
    CheckCurrent();
    switch (DSA->ogluCheckNamedFramebufferStatus(FBOId, GL_FRAMEBUFFER))
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

void oglFrameBuffer_::Use() const
{
    CheckCurrent();
    DSA->ogluBindFramebuffer(GL_FRAMEBUFFER, FBOId);
}

std::pair<GLuint, GLuint> oglFrameBuffer_::DebugBinding() const
{
    return oglFrameBuffer_::DebugBinding(FBOId);
}

void oglFrameBuffer_::UseDefault()
{
    DSA->ogluBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::pair<GLuint, GLuint> oglFrameBuffer_::DebugBinding(GLuint id)
{
    GLint clrId = 0, depId = 0;
    DSA->ogluGetNamedFramebufferAttachmentParameteriv(id, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &clrId);
    DSA->ogluGetNamedFramebufferAttachmentParameteriv(id, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depId);
    return std::pair<GLuint, GLuint>(clrId, depId);
}


oglFrameBuffer2D_::oglFrameBuffer2D_() : oglFrameBuffer_()
{ }
oglFrameBuffer2D_::~oglFrameBuffer2D_()
{ }

void oglFrameBuffer2D_::AttachColorTexture(const oglTex2D& tex, const uint8_t attachment)
{
    CheckCurrent();
    DSA->ogluNamedFramebufferTexture(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D, GetID(tex), 0);
    ColorAttachemnts[attachment] = tex;
}

void oglFrameBuffer2D_::AttachColorTexture(const oglTex2DArray& tex, const uint32_t layer, const uint8_t attachment)
{
    if (layer >= tex->Layers)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"texture layer overflow");
    CheckCurrent();
    DSA->ogluNamedFramebufferTextureLayer(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D_ARRAY, GetID(tex), 0, layer);
    ColorAttachemnts[attachment] = std::make_pair(tex, layer);
}

void oglFrameBuffer2D_::AttachColorTexture(const oglRBO& rbo, const uint8_t attachment)
{
    if (rbo->GetType() != RBOFormat::TYPE_COLOR)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"rbo type missmatch");
    CheckCurrent();
    DSA->ogluNamedFramebufferRenderbuffer(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_RENDERBUFFER, GetID(rbo));
    ColorAttachemnts[attachment] = rbo;
}

void oglFrameBuffer2D_::AttachDepthTexture(const oglTex2D& tex)
{
    CheckCurrent();
    DSA->ogluNamedFramebufferTexture(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GetID(tex), 0);
    DepthAttachment = tex;
}

void oglFrameBuffer2D_::AttachDepthTexture(const oglRBO& rbo)
{
    if (rbo->GetType() != RBOFormat::TYPE_DEPTH)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"rbo type missmatch");
    CheckCurrent();
    DSA->ogluNamedFramebufferRenderbuffer(FBOId, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, GetID(rbo));
    DepthAttachment = rbo;
}

void oglFrameBuffer2D_::AttachStencilTexture(const oglTex2D& tex)
{
    CheckCurrent();
    DSA->ogluNamedFramebufferTexture(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, GetID(tex), 0);
    StencilAttachment = tex;
}

void oglFrameBuffer2D_::AttachStencilTexture(const oglRBO& rbo)
{
    if (rbo->GetType() != RBOFormat::TYPE_STENCIL)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"rbo type missmatch");
    CheckCurrent();
    DSA->ogluNamedFramebufferRenderbuffer(FBOId, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GetID(rbo));
    StencilAttachment = rbo;
}

void oglFrameBuffer2D_::AttachDepthStencilBuffer(const oglRBO& rbo)
{
    if (rbo->GetType() != RBOFormat::TYPE_DEPTH_STENCIL)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"rbo type missmatch");
    CheckCurrent();
    DSA->ogluNamedFramebufferRenderbuffer(FBOId, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, GetID(rbo));
    DepthAttachment = rbo; StencilAttachment = rbo;
}

static void BlitColor(const GLuint from, const GLuint to, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    const auto& [x0, y0, x1, y1] = rect;
    DSA->ogluBlitNamedFramebuffer(from, to, x0, y0, x1, y1, x0, y0, x1, y1, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void oglFrameBuffer2D_::BlitColorTo(const oglFBO2D& to, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    CheckCurrent();
    if (to)
        to->CheckCurrent();
    BlitColor(FBOId, to ? to->FBOId : 0, rect);
}
void oglFrameBuffer2D_::BlitColorFrom(const oglFBO2D& from, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect)
{
    CheckCurrent();
    if (from)
        from->CheckCurrent();
    BlitColor(from ? from->FBOId : 0, FBOId, rect);
}

oglFBO2D oglFrameBuffer2D_::Create()
{
    return MAKE_ENABLER_SHARED(oglFrameBuffer2D_, ());
}


oglFrameBuffer3D_::oglFrameBuffer3D_() : oglFrameBuffer_()
{ }
oglFrameBuffer3D_::~oglFrameBuffer3D_()
{ }

void oglFrameBuffer3D_::CheckLayerMatch(const uint32_t layer)
{
    if (LayerCount != 0 && LayerCount != layer)
        COMMON_THROWEX(OGLException, OGLException::GLComponent::OGLU, u"Layered Framebuffer's layer not match");
    LayerCount = layer;
}

void oglFrameBuffer3D_::AttachColorTexture(const oglTex3D& tex, const uint8_t attachment)
{
    CheckLayerMatch(tex->Depth);
    CheckCurrent();
    DSA->ogluNamedFramebufferTexture(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_3D, GetID(tex), 0);
    ColorAttachemnts[attachment] = tex;
}

void oglFrameBuffer3D_::AttachColorTexture(const oglTex2DArray& tex, const uint8_t attachment)
{
    CheckLayerMatch(tex->Layers);
    CheckCurrent();
    DSA->ogluNamedFramebufferTexture(FBOId, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D_ARRAY, GetID(tex), 0);
    ColorAttachemnts[attachment] = tex;
}

void oglFrameBuffer3D_::AttachDepthTexture(const oglTex3D& tex)
{
    CheckLayerMatch(tex->Depth);
    CheckCurrent();
    DSA->ogluNamedFramebufferTexture(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_3D, GetID(tex), 0);
    DepthAttachment = tex;
}

void oglFrameBuffer3D_::AttachDepthTexture(const oglTex2DArray& tex)
{
    CheckLayerMatch(tex->Layers);
    CheckCurrent();
    DSA->ogluNamedFramebufferTexture(FBOId, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_ARRAY, GetID(tex), 0);
    DepthAttachment = tex;
}

void oglFrameBuffer3D_::AttachStencilTexture(const oglTex3D& tex)
{
    CheckLayerMatch(tex->Depth);
    CheckCurrent();
    DSA->ogluNamedFramebufferTexture(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_3D, GetID(tex), 0);
    StencilAttachment = tex;
}

void oglFrameBuffer3D_::AttachStencilTexture(const oglTex2DArray& tex)
{
    CheckLayerMatch(tex->Layers);
    CheckCurrent();
    DSA->ogluNamedFramebufferTexture(FBOId, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D_ARRAY, GetID(tex), 0);
    StencilAttachment = tex;
}

oglFBO3D oglFrameBuffer3D_::Create()
{
    return MAKE_ENABLER_SHARED(oglFrameBuffer3D_, ());
}



}