#include "TestRely.h"
#include "OpenGLUtil/OpenGLUtil.h"
#include "OpenGLUtil/oglException.h"
#include "SystemCommon/ConsoleEx.h"
#include "common/StringLinq.hpp"
#include <iostream>
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
#   undef None
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


#if COMMON_OS_WIN
template<typename... Args>
static oglContext InitContextWGL(oglLoader& loader_, CreateInfo& cinfo, HDC dc, Args&&...)
{
    auto& loader = static_cast<WGLLoader&>(loader_);
    const auto host = loader.CreateHost(dc);
    return loader.CreateContext(host, cinfo);
}
#endif
#if COMMON_OS_UNIX
template<typename... Args>
static oglContext InitContextGLX(oglLoader& loader_, CreateInfo& cinfo, Display* display, int32_t screen, Args&&...)
{
    auto& loader = static_cast<GLXLoader&>(loader_);
    const auto host = loader.CreateHost(display, screen, true);
    if (!host)
    {
        log().error(u"Failed to init glx host\n");
        COMMON_THROW(BaseException, u"Error");
    }
    return loader.CreateContext(host, cinfo);
}
#endif
template<typename... Args>
static oglContext InitContextEGL(oglLoader& loader_, CreateInfo& cinfo, void* dc, Args&&...)
{
    auto& loader = static_cast<EGLLoader&>(loader_);
    const auto host = loader.CreateHost(dc, true);
    if (!host)
    {
        log().error(u"Failed to init egl host\n");
        COMMON_THROW(BaseException, u"Error");
    }
    return loader.CreateContext(host, cinfo);
}

template<typename... Args>
static oglContext InitContext(oglLoader& loader_, CreateInfo& cinfo, const Args&... args)
{
#if COMMON_OS_WIN
    if (loader_.Name() == "WGL") return InitContextWGL(loader_, cinfo, args...);
#endif
#if COMMON_OS_UNIX
    if (loader_.Name() == "GLX") return InitContextGLX(loader_, cinfo, args...);
#endif
    if (loader_.Name() == "EGL") return InitContextEGL(loader_, cinfo, args...);

    log().error(u"Unknown loader\n");
    return {};
}


static void OGLStub()
{
    [[maybe_unused]] const auto commondevs = xcomp::ProbeDevice();
    const auto loaders = oglLoader::GetLoaders();
    if (loaders.size() == 0)
    {
        log().error(u"No OpenGL loader found!\n");
        return;
    }

#if COMMON_OS_WIN
    HWND tmpWND = CreateWindow(
        L"Static", L"Fake Window",            // window class, title
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN,  // style
        0, 0,                               // position x, y
        1, 1,                               // width, height
        NULL, NULL,                         // parent window, menu
        nullptr, NULL);                     // instance, param
    HDC tmpDC = GetDC(tmpWND);
#else
    const char* const disp = getenv("DISPLAY");
    Display* display = XOpenDisplay(disp ? disp : ":0.0");
    if (!display)
    {
        log().error(u"Failed to open display\n");
        COMMON_THROW(BaseException, u"Error");
    }
    const auto defScreen = DefaultScreen(display);
#endif

    while (true)
    {
        const auto ldridx = SelectIdx(loaders, u"loader", [&](const auto& loader)
            {
                return FMTSTR(u"[{}]", loader->Name());
            });

        auto& loader = *loaders[ldridx];
        CreateInfo cinfo;
        cinfo.PrintFuncLoadFail = cinfo.PrintFuncLoadSuccess = true;
#if COMMON_OS_WIN
        auto ctx = InitContext(loader, cinfo, tmpDC);
#else
        auto ctx = InitContext(loader, cinfo, display, defScreen);
#endif

        ctx->UseContext();
        ctx->SetDebug(MsgSrc::All, MsgType::All, MsgLevel::Notfication);
        log().success(u"GL Context Version: [{}]\n", ctx->Capability->VersionString);
        log().info(u"{}\n", ctx->Capability->GenerateSupportLog());
        while (true)
        {
            common::mlog::SyncConsoleBackend();
            string fpath = common::console::ConsoleEx::ReadLine("input opengl file:");
            if (fpath == "EXTENSION")
            {
                string exttxts("Platform Extensions:\n");
                for (const auto& ext : ctx->GetPlatformExtensions())
                    exttxts.append(ext).append("\n");
                exttxts.append("Extensions:\n");
                for (const auto& ext : ctx->GetExtensions())
                    exttxts.append(ext).append("\n");
                log().verbose(u"{}\n", exttxts);
                continue;
            }
            else if (fpath == "clear")
            {
                GetConsole().ClearConsole();
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
                auto stub = oglProgram_::Create();
                stub.AddExtShaders(shaderSrc, config);
                log().info(u"Try Draw Program\n");
                [[maybe_unused]] auto drawProg = stub.LinkDrawProgram(u"Draw Prog");
                log().info(u"Try Compute Program\n");
                [[maybe_unused]] auto compProg = stub.LinkComputeProgram(u"Compute Prog");
            }
            catch (const BaseException& be)
            {
                PrintException(be, u"Error here");
            }
        }
    }
    getchar();
}

const static uint32_t ID = RegistTest("OGLStub", &OGLStub);

