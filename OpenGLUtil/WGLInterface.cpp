#include "oglPch.h"
#include "oglUtil.h"
#include "SystemCommon/HResultHelper.h"

#undef APIENTRY
#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX 1
#include <Windows.h>
#include "GL/wglext.h"
//fucking wingdi defines some terrible macro
#undef ERROR
#undef MemoryBarrier
#pragma message("Compiling OpenGLUtil with wgl-ext[" STRINGIZE(WGL_WGLEXT_VERSION) "]")
#pragma comment(lib, "opengl32.lib")// link Microsoft OpenGL lib
#pragma comment(lib, "glu32.lib")// link OpenGL Utility lib

namespace oglu
{
using namespace std::string_view_literals;


constexpr PIXELFORMATDESCRIPTOR PixFmtDesc =    // pfd Tells Windows How We Want Things To Be
{
    sizeof(PIXELFORMATDESCRIPTOR),              // Size Of This Pixel Format Descriptor
    1,                                          // Version Number
    PFD_DRAW_TO_WINDOW/*Support Window*/ | PFD_SUPPORT_OPENGL/*Support OpenGL*/ | PFD_DOUBLEBUFFER/*Support Double Buffering*/ | PFD_GENERIC_ACCELERATED,
    PFD_TYPE_RGBA,                              // Request An RGBA Format
    32,                                         // Select Our Color Depth
    0, 0, 0, 0, 0, 0,                           // Color Bits Ignored
    0, 0,                                       // No Alpha Buffer, Shift Bit Ignored
    0, 0, 0, 0, 0,                              // No Accumulation Buffer, Accumulation Bits Ignored
    24,                                         // 24Bit Z-Buffer (Depth Buffer) 
    8,                                          // 8Bit Stencil Buffer
    0,                                          // No Auxiliary Buffer
    PFD_MAIN_PLANE,                             // Main Drawing Layer
    0,                                          // Reserved
    0, 0, 0                                     // Layer Masks Ignored
};

class WGLLoader_ final : public WGLLoader
{
private:
    struct WGLHost final : public WGLLoader::WGLHost
    {
        friend WGLLoader_;
        HDC DeviceContext;
        PFNWGLCREATECONTEXTATTRIBSARBPROC CreateContextAttribsARB = nullptr;
        WGLHost(WGLLoader_& loader, HDC dc) noexcept : WGLLoader::WGLHost(loader), DeviceContext(dc)
        {
            SupportDesktop = true;
            if (const auto funcARB = loader.GetFunction<PFNWGLGETEXTENSIONSSTRINGARBPROC>("wglGetExtensionsStringARB"))
            {
                Extensions = common::str::Split(funcARB(DeviceContext), ' ', false);
            }
            else if (const auto funcEXT = loader.GetFunction<PFNWGLGETEXTENSIONSSTRINGEXTPROC>("wglGetExtensionsStringEXT"))
            {
                Extensions = common::str::Split(funcEXT(), ' ', false);
            }
            CreateContextAttribsARB = loader.GetFunction<PFNWGLCREATECONTEXTATTRIBSARBPROC>("wglCreateContextAttribsARB");
            SupportES = Extensions.Has("WGL_EXT_create_context_es2_profile");
            SupportSRGB = Extensions.Has("WGL_ARB_framebuffer_sRGB") || Extensions.Has("WGL_EXT_framebuffer_sRGB");
            SupportFlushControl = Extensions.Has("WGL_ARB_context_flush_control");
        }
        ~WGLHost() final {}
        bool MakeGLContextCurrent_(void* hRC) const final
        {
            return wglMakeCurrent(DeviceContext, (HGLRC)hRC);
        }
        void DeleteGLContext(void* hRC) const final
        {
            wglDeleteContext((HGLRC)hRC);
        }
        void SwapBuffer() const final
        {
            SwapBuffers(DeviceContext);
        }
        void ReportFailure(std::u16string_view action) const final
        {
            oglLog().error(u"Failed to {} on HDC[{}], error: {}\n", action, (void*)DeviceContext, common::Win32ErrorHolder(GetLastError()));
        }
        void TemporalInsideContext(void* hRC, const std::function<void(void* hRC)>& func) const final
        {
            const auto oldHdc = wglGetCurrentDC();
            const auto oldHrc = wglGetCurrentContext();
            if (wglMakeCurrent(DeviceContext, (HGLRC)hRC))
            {
                func(hRC);
                wglMakeCurrent(oldHdc, oldHrc);
            }
        }
        uint32_t GetVersion() const noexcept final { return 0; }
        void* GetDeviceContext() const noexcept final { return DeviceContext; }
    };
public:
    WGLLoader_()
    {

    }
    ~WGLLoader_() final {}
private:
    std::string_view Name() const noexcept final { return "WGL"sv; }

    /*void Init() override
    { }*/
    std::shared_ptr<WGLLoader::WGLHost> CreateHost(void* hdc_) final
    {
        const auto hdc = reinterpret_cast<HDC>(hdc_);
        const int pixFormat = ChoosePixelFormat(hdc, &PixFmtDesc);
        //WGL_ARB_pixel_format
        SetPixelFormat(hdc, pixFormat, &PixFmtDesc);
        const auto oldHdc = wglGetCurrentDC();
        const auto oldHrc = wglGetCurrentContext();
        const auto tmpHrc = wglCreateContext(hdc);
        wglMakeCurrent(hdc, tmpHrc);
        auto info = std::make_shared<WGLHost>(*this, hdc);
        wglMakeCurrent(oldHdc, oldHrc);
        wglDeleteContext(tmpHrc);
        return info;
    }
    void* CreateContext_(const GLHost& host_, const CreateInfo& cinfo, void* sharedCtx) noexcept final
    {
        auto host = std::static_pointer_cast<WGLHost>(host_);
        detail::AttribList attrib;
        attrib.Set(WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB);
        if (cinfo.DebugContext)
            attrib.Set(WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB);
        if (cinfo.Version != 0)
        {
            attrib.Set(WGL_CONTEXT_MAJOR_VERSION_ARB, static_cast<int32_t>(cinfo.Version / 10));
            attrib.Set(WGL_CONTEXT_MINOR_VERSION_ARB, static_cast<int32_t>(cinfo.Version % 10));
        }
        if (cinfo.FlushWhenSwapContext && host->SupportFlushControl)
        {
            attrib.Set(WGL_CONTEXT_RELEASE_BEHAVIOR_ARB, cinfo.FlushWhenSwapContext.value() ? 
                WGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB : WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB);
        }
        return host->CreateContextAttribsARB(host->DeviceContext, reinterpret_cast<HGLRC>(sharedCtx), attrib.Data());
    }
    void* GetFunction_(std::string_view name) const noexcept final
    {
        return wglGetProcAddress(name.data());
    }

    static inline const auto Singleton = oglLoader::RegisterLoader<WGLLoader_>();
};

}
