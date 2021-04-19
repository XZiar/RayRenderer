#pragma once
#include "oglRely.h"
#include "oglTexture.h"
#include "3DElement.hpp"
#include <bitset>


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oglu
{
class oglRenderBuffer_;
using oglRBO = std::shared_ptr<oglRenderBuffer_>;
class oglFrameBuffer_;
using oglFBO = std::shared_ptr<oglFrameBuffer_>;
class oglDefaultFrameBuffer_;
using oglDefaultFBO = std::shared_ptr<oglDefaultFrameBuffer_>;
class oglCustomFrameBuffer_;
using oglCustomFBO = std::shared_ptr<oglCustomFrameBuffer_>;
class oglFrameBuffer2D_;
using oglFBO2D = std::shared_ptr<oglFrameBuffer2D_>;
class oglLayeredFrameBuffer_;
using oglFBOLayered = std::shared_ptr<oglLayeredFrameBuffer_>;



enum class RBOFormat : uint8_t
{
    MASK_TYPE  = 0xe0,
    TYPE_DEPTH = 0x80, TYPE_STENCIL = 0x40, TYPE_DEPTH_STENCIL = 0xc0, TYPE_COLOR = 0x20,
    MASK_SRGB  = 0x10,
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
    friend class oglCustomFrameBuffer_;
private:
    MAKE_ENABLER();
    std::u16string Name;
    GLuint RBOId;
    RBOFormat InnerFormat;
    uint32_t Width, Height;
    oglRenderBuffer_(const uint32_t width, const uint32_t height, const RBOFormat format);
public:
    ~oglRenderBuffer_();
    
    void SetName(std::u16string name) noexcept;
   
    [[nodiscard]] RBOFormat GetType() const noexcept { return InnerFormat & RBOFormat::MASK_TYPE; }
    [[nodiscard]] bool IsSrgb() const noexcept { return HAS_FIELD(InnerFormat, RBOFormat::MASK_SRGB); }
    [[nodiscard]] std::u16string_view GetName() const noexcept { return Name; }

    [[nodiscard]] static oglRBO Create(const uint32_t width, const uint32_t height, const RBOFormat format);
};


class OGLUAPI oglFrameBuffer_ : 
    public common::NonMovable, 
    public std::enable_shared_from_this<oglFrameBuffer_>,
    public detail::oglCtxObject<false> // GL Container objects
{
    friend class oglContext_;
    friend class ProgDraw;
private:
    static std::pair<oglFBO, common::RWSpinLock::ReadScopeType> LockCurFBOLock();
    GLenum CheckIfBinded() const;
protected:
    class OGLUAPI FBOClear : public common::NonCopyable
    {
    protected:
        oglFrameBuffer_& NewFBO;
        forceinline GLuint NewFBOId() const noexcept { return NewFBO.FBOId; }
        forceinline void BindDraws(const common::span<GLenum> bindings) { NewFBO.BindDraws(bindings); }
    private:
        static common::RWSpinLock::ReadScopeType AcquireLock(oglFrameBuffer_& fbo);
        GLuint OldFBOId;
        common::RWSpinLock::ReadScopeType Lock;
    public:
        FBOClear(oglFrameBuffer_* fbo);
        virtual ~FBOClear();
        FBOClear& ClearColors(const b3d::Vec4& color = b3d::Vec4::zero());
        FBOClear& ClearDepth(const float depth = 0.f);
        FBOClear& ClearStencil(const GLint stencil = 0);
        FBOClear& ClearDepthStencil(const float depth = 0.f, const GLint stencil = 0);
        FBOClear& ClearAll(const b3d::Vec4& color = b3d::Vec4::zero(), const float depth = 0.f, const GLint stencil = 0)
        {
            return ClearColors(color).ClearDepthStencil(depth, stencil);
        }
    };

    std::vector<GLenum> DrawBindings;
    GLuint FBOId;
    uint32_t Width, Height;

    oglFrameBuffer_(const GLuint id);
    bool CheckIsSrgb(const GLenum attachment) const;
    /// return if delay update viewport
    bool SetViewPort(const uint32_t width, const uint32_t height);
    void BindDraws(const common::span<GLenum> bindings);
public:
    virtual ~oglFrameBuffer_();

    [[nodiscard]] FBOStatus CheckStatus() const;
    [[nodiscard]] virtual GLenum GetAttachPoint(const std::string_view name) const = 0;

    void Use();
    FBOClear Clear();
    void ClearAll();
    void BlitColorTo(const oglFBO& to, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect);
    void BlitColorFrom(const oglFBO& from, const std::tuple<int32_t, int32_t, int32_t, int32_t> rect);
};


class OGLUAPI oglDefaultFrameBuffer_ : public oglFrameBuffer_
{
    friend struct FBOCtxInfo;
    friend class oglContext_;
private:
    MAKE_ENABLER();
    bool IsSrgbColor;
    oglDefaultFrameBuffer_();
public:
    class OGLUAPI FBOClear : public oglFrameBuffer_::FBOClear
    {
        friend class oglDefaultFrameBuffer_;
    private:
        using oglFrameBuffer_::FBOClear::FBOClear;
    public:
        FBOClear& DiscardColors();
        FBOClear& DiscardDepth();
        FBOClear& DiscardStencil();
        FBOClear& DiscardDepthStencil();
    };
    ~oglDefaultFrameBuffer_() override;

    [[nodiscard]] bool IsSrgb() const;
    [[nodiscard]] GLenum GetAttachPoint(const std::string_view name) const override;

    void SetWindowSize(const uint32_t width, const uint32_t height);
    FBOClear Clear();

    [[nodiscard]] static oglDefaultFBO Get();
};


class OGLUAPI oglCustomFrameBuffer_ : public oglFrameBuffer_
{
public:
    using FBOAttachment = std::variant<std::monostate, oglRBO, oglTex2D, oglTex3D, oglTex2DArray, std::pair<oglTex2DArray, uint32_t>>;
private:
    void CheckAttachmentMatch(const uint32_t width, const uint32_t height);
protected:
    std::vector<FBOAttachment> ColorAttachemnts;
    std::vector<std::string> ColorAttachNames;
    FBOAttachment DepthAttachment;
    FBOAttachment StencilAttachment;
    std::u16string Name;

    oglCustomFrameBuffer_();
    void CheckAttachmentMatch(const oglRBO& rbo)
    { 
        CheckAttachmentMatch(rbo->Width, rbo->Height);
    }
    void CheckAttachmentMatch(const oglTex2D& tex)
    { 
        CheckAttachmentMatch(tex->Width, tex->Height);
    }
    void CheckAttachmentMatch(const oglTex2DArray& tex)
    { 
        CheckAttachmentMatch(tex->Width, tex->Height);
    }
    void CheckAttachmentMatch(const oglTex3D& tex)
    { 
        CheckAttachmentMatch(tex->Width, tex->Height);
    }
    [[nodiscard]] forceinline static GLuint GetID(const oglRBO&     rbo) noexcept { return rbo->RBOId; }
    [[nodiscard]] forceinline static GLuint GetID(const oglTexBase& tex) noexcept { return tex->TextureID; }
    template<typename T>
    forceinline void AttachColorTexture(const uint8_t attachment, const std::string_view name, T&& obj) noexcept
    {
        ColorAttachemnts[attachment] = std::forward<T>(obj);
        ColorAttachNames[attachment] = name;
    }
public:
    class OGLUAPI FBOClear : public oglFrameBuffer_::FBOClear
    {
        friend class oglCustomFrameBuffer_;
    private:
        oglCustomFrameBuffer_& TheFBO;
        std::vector<std::optional<b3d::Vec4>> ColorClears;
        FBOClear(oglCustomFrameBuffer_* fbo);
    public:
        ~FBOClear() override;
        FBOClear& ClearColor(const uint8_t attachment, const b3d::Vec4& color = b3d::Vec4::zero());
        FBOClear& DiscardColor(const uint8_t attachment);
        FBOClear& DiscardDepth();
        FBOClear& DiscardStencil();
        FBOClear& DiscardDepthStencil();
    };

    ~oglCustomFrameBuffer_() override;
    [[nodiscard]] std::u16string_view GetName() const noexcept { return Name; }
    [[nodiscard]] bool IsSrgb(const uint8_t attachment) const;
    [[nodiscard]] GLenum GetAttachPoint(const std::string_view name) const override;
    [[nodiscard]] std::pair<GLuint, GLuint> DebugBinding() const;

    void SetName(std::u16string name) noexcept;
    FBOClear Clear();

    [[nodiscard]] static std::pair<GLuint, GLuint> DebugBinding(GLuint id);
};


class OGLUAPI oglFrameBuffer2D_ : public oglCustomFrameBuffer_
{
    friend class oglContext_;
private:
    MAKE_ENABLER();
    oglFrameBuffer2D_();
public:
    ~oglFrameBuffer2D_();
    void AttachColorTexture(const oglTex2D& tex, const uint8_t attachment, const std::string_view name);
    void AttachColorTexture(const oglTex2DArray& tex, const uint32_t layer, const uint8_t attachment, const std::string_view name);
    void AttachColorTexture(const oglRBO& rbo, const uint8_t attachment, const std::string_view name);
    void AttachColorTexture(const oglTex2D& tex)
    {
        AttachColorTexture(tex, 0, "default");
    }
    void AttachColorTexture(const oglTex2DArray& tex, const uint32_t layer)
    {
        AttachColorTexture(tex, layer, 0, "default");
    }
    void AttachColorTexture(const oglRBO& rbo)
    {
        AttachColorTexture(rbo, 0, "default");
    }
    void AttachDepthTexture(const oglTex2D& tex);
    void AttachDepthTexture(const oglRBO& rbo);
    void AttachStencilTexture(const oglTex2D& tex);
    void AttachStencilTexture(const oglRBO& rbo);
    void AttachDepthStencilBuffer(const oglRBO& rbo);

    [[nodiscard]] static oglFBO2D Create();
};


class OGLUAPI oglLayeredFrameBuffer_ : public oglCustomFrameBuffer_
{
    friend class oglContext_;
private:
    MAKE_ENABLER();
    uint32_t LayerCount = 0;
    oglLayeredFrameBuffer_();
    void CheckLayerMatch(const uint32_t layer);
public:
    ~oglLayeredFrameBuffer_() override;
    void AttachColorTexture(const oglTex3D& tex, const uint8_t attachment, const std::string_view name);
    void AttachColorTexture(const oglTex2DArray& tex, const uint8_t attachment, const std::string_view name);
    void AttachColorTexture(const oglTex3D& tex)
    {
        AttachColorTexture(tex, 0, "default");
    }
    void AttachColorTexture(const oglTex2DArray& tex)
    {
        AttachColorTexture(tex, 0, "default");
    }
    void AttachDepthTexture(const oglTex3D& tex);
    void AttachDepthTexture(const oglTex2DArray& tex);
    void AttachStencilTexture(const oglTex3D& tex);
    void AttachStencilTexture(const oglTex2DArray& tex);

    [[nodiscard]] static oglFBOLayered Create();
};


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
