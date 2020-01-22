#include "TestRely.h"
#include "OpenGLUtil/OpenGLUtil.h"
#include "OpenGLUtil/oglException.h"
#include "common/StringLinq.hpp"
#include <algorithm>

#if COMMON_OS_WIN
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include <Windows.h>
#   pragma comment(lib, "Opengl32.lib")
#else
#   include <X11/X.h>
#   include <X11/Xlib.h>
#   include <GL/gl.h>
#   include <GL/glx.h>
//fucking X11 defines some terrible macro
#   undef Always
#endif


using namespace common;
using namespace common::mlog;
using namespace oglu;
using std::string;
using std::cin;

static MiniLogger<false>& log()
{
    static MiniLogger<false> logger(u"OGLStub", { GetConsoleBackend() });
    return logger;
}

oglContext CreateContext()
{
    oglContext_::Refresh();
    oglUtil::InitLatestVersion();
    // uint32_t major, minor;
    // log().info(u"Input major and minor version...");
    // std::cin >> major >> minor;
    // std::cin.ignore();
    // auto ctx = oglContext::NewContext(oglContext::CurrentContext(), false, { (uint8_t)major,(uint8_t)minor });
    oglu::oglUtil::SetFuncLoadingDebug(true, true);
    auto ctx = oglContext_::NewContext(oglContext_::CurrentContext(), false, oglu::oglContext_::GetLatestVersion());
    oglu::oglUtil::SetFuncLoadingDebug(false, false);
    ctx->UseContext();
    ctx->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
    return ctx;
}
#if defined(_WIN32)
oglContext InitContext()
{
    HWND tmpWND = CreateWindow(
        L"Static", L"Fake Window",            // window class, title
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN,  // style
        0, 0,                               // position x, y
        1, 1,                               // width, height
        NULL, NULL,                         // parent window, menu
        nullptr, NULL);                     // instance, param
    HDC tmpDC = GetDC(tmpWND);
    static PIXELFORMATDESCRIPTOR pfd =  // pfd Tells Windows How We Want Things To Be
    {
        sizeof(PIXELFORMATDESCRIPTOR),  // Size Of This Pixel Format Descriptor
        1,                              // Version Number
        PFD_DRAW_TO_WINDOW/*Support Window*/ | PFD_SUPPORT_OPENGL/*Support OpenGL*/ | PFD_DOUBLEBUFFER/*Support Double Buffering*/ | PFD_GENERIC_ACCELERATED,
        PFD_TYPE_RGBA,                  // Request An RGBA Format
        32,                             // Select Our Color Depth
        0, 0, 0, 0, 0, 0,               // Color Bits Ignored
        0, 0,                           // No Alpha Buffer, Shift Bit Ignored
        0, 0, 0, 0, 0,                  // No Accumulation Buffer, Accumulation Bits Ignored
        24,                             // 24Bit Z-Buffer (Depth Buffer) 
        8,                              // 8Bit Stencil Buffer
        0,                              // No Auxiliary Buffer
        PFD_MAIN_PLANE,                 // Main Drawing Layer
        0,                              // Reserved
        0, 0, 0                         // Layer Masks Ignored
    };
    const int PixelFormat = ChoosePixelFormat(tmpDC, &pfd);
    SetPixelFormat(tmpDC, PixelFormat, &pfd);
    HGLRC tmpRC = wglCreateContext(tmpDC);
    wglMakeCurrent(tmpDC, tmpRC);

    const auto pp1 = wglGetProcAddress("wglChoosePixelFormatARB");
    const auto pp2 = wglGetProcAddress("wglGetExtensionsStringARB");

    auto ctx = CreateContext();
    oglContext_::ReleaseExternContext(tmpRC);
    wglDeleteContext(tmpRC);
    return ctx;
}
#else
oglContext InitContext()
{
    static int visual_attribs[] = 
    {
        GLX_X_RENDERABLE, true,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_DOUBLEBUFFER, true,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        None
    };
    const char* const disp = getenv("DISPLAY");
    Display* display = XOpenDisplay(disp ? disp : ":0.0");
    /* open display */
    if (!display)
    {
        log().error(u"Failed to open display\n");
        COMMON_THROW(BaseException, u"Error");
    }

    /* get framebuffer configs, any is usable (might want to add proper attribs) */
    int fbcount = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &fbcount);
    if (!fbc) 
    {
        log().error(u"Failed to get FBConfig\n");
        COMMON_THROW(BaseException, u"Error");
    }
    XVisualInfo *vi = glXGetVisualFromFBConfig(display, fbc[0]);
    const auto tmpCtx = glXCreateContext(display, vi, 0, GL_TRUE);
    Window win = DefaultRootWindow(display);
    glXMakeCurrent(display, win, tmpCtx);

    auto ctx = CreateContext();
    oglContext_::ReleaseExternContext(tmpCtx);
    glXDestroyContext(display, tmpCtx);
    return ctx;
}
#endif


static void OGLStub()
{
    const auto ctx = InitContext();
    log().success(u"GL Context Version: [{}]\n", ctx->Capability->VersionString);
    log().info(u"{}\n", ctx->Capability->GenerateSupportLog());
    while (true)
    {
        log().info(u"input opengl file:");
        string fpath;
        std::getline(cin, fpath);
        if (fpath == "EXTENSION")
        {
            string exttxts("Extensions:\n");
            for(const auto& ext : ctx->GetExtensions())
                exttxts.append(ext).append("\n");
            log().verbose(u"{}\n", exttxts);
            continue;
        }
        bool exConfig = false;
        if (fpath.size() > 0 && fpath.back() == '#')
            fpath.pop_back(), exConfig = true;
        common::fs::path filepath = FindPath() / fpath;
        log().debug(u"loading gl file [{}]\n", filepath.u16string());
        try
        {
            const auto shaderSrc = common::file::ReadAllText(filepath);
            ShaderConfig config;
            if (exConfig)
            {
                string line;
                while (cin >> line)
                {
                    ClearReturn();
                    if (line.size() == 0) break;
                    const auto parts = common::str::Split(line, '=');
                    string key(parts[0].substr(1));
                    switch (line.front())
                    {
                    case '#':
                        if (parts.size() > 1)
                            config.Defines[key] = string(parts[1].cbegin(), parts.back().cend());
                        else
                            config.Defines[key] = std::monostate{};
                        continue;
                    case '@':
                        if (parts.size() == 2)
                            config.Routines.insert_or_assign(key, string(parts[1]));
                        continue;
                    }
                    break;
                }
            }
            try
            {
                auto stub = oglProgram_::Create();
                stub.AddExtShaders(shaderSrc, config);
                log().info(u"Try Draw Program\n");
                [[maybe_unused]] auto drawProg = stub.LinkDrawProgram(u"Draw Prog");
                log().info(u"Try Compute Program\n");
                [[maybe_unused]] auto compProg = stub.LinkComputeProgram(u"Compute Prog");
            }
            catch (const OGLException & gle)
            {
                log().error(u"OpenGL shader fail:\n{}\n", gle.message);
                const auto buildLog = gle.data.has_value() ? std::any_cast<std::u16string>(&gle.data) : nullptr;
                if (buildLog)
                    log().error(u"Extra info:{}\n", *buildLog);
            }
        }
        catch (const BaseException& be)
        {
            log().error(u"Error here:\n{}\n\n", be.message);
        }
    }
    getchar();
}

const static uint32_t ID = RegistTest("OGLStub", &OGLStub);

