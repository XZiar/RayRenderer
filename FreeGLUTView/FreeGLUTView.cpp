#include "FreeGLUTView.h"

#include <cstdint>
#include <cstdio>
#include <tuple>
#include <functional>
#include <atomic>
#include <mutex>
#include <future>
#include <vector>
#include "common/TimeUtil.hpp"
#include "common/miniLogger/miniLogger.h"
#include "common/ContainerEx.hpp"
#include "common/AsyncExecutor/AsyncManager.h"

#ifndef _DEBUG
#   define NDEBUG 1
#endif
#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX 1
#define FREEGLUT_STATIC
#include "freeglut/freeglut.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#if defined(_WIN32)
#   include <Windows.h>
#   include <shellapi.h>
#   include <conio.h>
#   include "GL/wglext.h"
#   include <commdlg.h>
#   pragma comment(lib, "Opengl32.lib")
#else
#   include <X11/X.h>
#   include <X11/Xlib.h>
#   include <GL/glx.h>
//fucking X11 defines some terrible macro
#   undef Always
#   undef None
#endif


namespace glutview
{

common::mlog::MiniLogger<false>& fgvLog()
{
    using namespace common::mlog;
    static MiniLogger<false> fgvlog(u"FGView", { GetConsoleBackend() });
    return fgvlog;
}

namespace detail
{
using namespace common::asyexe;

class FreeGLUTManager final
{
private:
    struct FGView
    {
        _FreeGLUTView *View;
#if defined(_WIN32)
        using HandleType = HWND;
#else
        using HandleType = Window;
#endif
        const HandleType Handle;
        int32_t ID;
    };
    using FGViewMap = boost::multi_index_container<FGView, boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<boost::multi_index::member<FGView, int32_t, &FGView::ID>>,
        boost::multi_index::ordered_unique<boost::multi_index::member<FGView, const FGView::HandleType, &FGView::Handle>>
        >>;
    static FGViewMap& GetViewMap()
    {
        static FGViewMap viewMap;
        return viewMap;
    }
    static _FreeGLUTView* GetView(const FGView::HandleType handle)
    {
        auto& wndkey = GetViewMap().get<1>();
        auto it = wndkey.find(handle);
        if (it == wndkey.end())
            return nullptr;
        return (*it).View;
    }
    friend class _FreeGLUTView;
    //static inline WNDPROC oldWndProc;
    static inline std::atomic_bool shouldInvoke{ false }, readyInvoke{ false };
    static inline std::tuple<_FreeGLUTView*, std::function<bool(void)>, std::promise<bool>> invokeData;
    
    static void regist(_FreeGLUTView* view)
    {
        //const auto handle = fgWindowByID(view->wdID)->Window.Handle;
        //GetViewMap().insert({ view, handle, view->wdID });
        // const auto hdc = wglGetCurrentDC();
        // const auto hwnd = WindowFromDC(hdc);
        // const auto hrc = wglGetCurrentContext();
        // DragAcceptFiles(hwnd, TRUE);
        // oldWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (intptr_t)&HackWndProc);
        // auto& rckey = getMap().get<2>();
        // if (!rckey.empty())
        // {//share gl context
        //     wglShareLists((*rckey.begin()).hrc, hrc);
        //     fgvLog().info(u"Share GL_RC {} from {}\n", (void*)hrc, (void*)(*rckey.begin()).hrc);
        // }
        // getMap().insert({ view,hwnd,hrc });
        //regist glutCallbacks
        view->usethis();
#if defined(_WIN32)
        const auto hdc = wglGetCurrentDC();
        const FGView::HandleType handle = WindowFromDC(hdc);
        DragAcceptFiles(handle, TRUE);
#else
        const FGView::HandleType handle = glXGetCurrentDrawable();
#endif
        GetViewMap().insert({ view, handle, view->wdID });
        fgvLog().info(u"New Window [{}]: handle[{}]\n", view->wdID, reinterpret_cast<uintptr_t>(handle));

        glutCloseFuncUcall(FreeGLUTManager::onClose, view);
        glutDisplayFuncUcall(FreeGLUTManager::onDisplay, view);
        glutReshapeFuncUcall(FreeGLUTManager::onReshape, view);
        glutKeyboardFuncUcall(FreeGLUTManager::onKeyboard, view);
        glutSpecialFuncUcall(FreeGLUTManager::onSpecial, view);
        glutMouseWheelFuncUcall(FreeGLUTManager::onMouseWheel, view);
        glutMotionFuncUcall(FreeGLUTManager::onMotion, view);
        glutMouseFuncUcall(FreeGLUTManager::onMouse, view);
        initExtension();
    }
    static void initExtension()
    {
    #if defined(_WIN32)
        auto wglewGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
        const auto exts = wglewGetExtensionsStringEXT();
        if(strstr(exts, "WGL_EXT_swap_control_tear") != nullptr)
        {
            auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
            //Adaptive Vsync
            wglSwapIntervalEXT(-1);
        }
    #else
        Display *display = glXGetCurrentDisplay();
        const auto exts = glXQueryExtensionsString(display, DefaultScreen(display));
        if(strstr(exts, "GLX_EXT_swap_control_tear") != nullptr)
        {
            constexpr auto ExtName = "GLX_EXT_swap_control";
            auto glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress(reinterpret_cast<const GLubyte*>(ExtName));
            //Adaptive Vsync
            GLXDrawable drawable = glXGetCurrentDrawable();
            glXSwapIntervalEXT(display, drawable, -1);
        }
    #endif
    }
    static bool unregist(_FreeGLUTView* view)
    {
        auto it = GetViewMap().find(view->wdID);
        if (it == GetViewMap().end())
            return false;
        GetViewMap().erase(it);
        return true;
    }
    static void onClose(void* ptr)
    {
        const auto view = reinterpret_cast<_FreeGLUTView*>(ptr);
        view->onClose();
        unregist(view);
    }
    static void onDisplay(void* ptr)
    {
        const auto view = reinterpret_cast<_FreeGLUTView*>(ptr);
        view->display();
    }
    static void onReshape(int w, int h, void* ptr)
    {
        const auto view = reinterpret_cast<_FreeGLUTView*>(ptr);
        view->reshape(w, h);
    }
    static void onKeyboard(unsigned char key, int x, int y, void* ptr)
    {
        const auto view = reinterpret_cast<_FreeGLUTView*>(ptr);
        view->onKeyboard(key, x, y);
    }
    static void onSpecial(int key, int x, int y, void* ptr)
    {
        const auto view = reinterpret_cast<_FreeGLUTView*>(ptr);
        view->onKeyboard(key, x, y);
    }
    static void onMouseWheel(int button, int dir, int x, int y, void* ptr)
    {
        const auto view = reinterpret_cast<_FreeGLUTView*>(ptr);
        view->onWheel(button, dir, x, y);
    }
    static void onMotion(int x, int y, void* ptr)
    {
        const auto view = reinterpret_cast<_FreeGLUTView*>(ptr);
        view->onMouse(x, y);
    }
    static void onMouse(int button, int state, int x, int y, void* ptr)
    {
        const auto view = reinterpret_cast<_FreeGLUTView*>(ptr);
        view->onMouse(button, state, x, y);
    }

    // static LRESULT CALLBACK HackWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    // {
    //     switch (uMsg)
    //     {
    //     case WM_DROPFILES:
    //         {
    //             HDROP hdrop = (HDROP)wParam;
    //             std::u16string pathBuf(2048, u'\0');
    //             DragQueryFile(hdrop, 0, (LPWSTR)pathBuf.data(), 2000);
    //             DragFinish(hdrop);
    //             const auto view = getView(hWnd);
    //             view->onDropFile(std::u16string(pathBuf.c_str()));
    //         }
    //         return 0;
    //     }
    //     return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
    // }

#if defined(_WIN32)
    static void FilterMessage()
    {
        MSG message;
        while (PeekMessage(&message, NULL, 0, 0, PM_NOREMOVE))
        {
            if (GetMessage(&message, NULL, 0, 0) == 0)
                return;
            if (message.message == WM_DROPFILES)
            {
                HDROP hdrop = (HDROP)message.wParam;
                std::u16string pathBuf(2048, u'\0');
                DragQueryFile(hdrop, 0, (LPWSTR)pathBuf.data(), 2000);
                DragFinish(hdrop);
                const auto view = GetView(message.hwnd);
                if (view)
                {
                    view->onDropFile(std::u16string(pathBuf.c_str()));
                    continue;
                }
            }
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }
#else
    static void FilterMessage()
    {
    }
#endif
public:
    static AsyncManager& Executor()
    { 
        static AsyncManager executor(u"FreeGLUTView", 1000 / 240, 1000 / 120, true);
        return executor;
    };
    static void Run()
    {
        Executor().Run([&](std::function<void(void)> stopper) 
        {
            Executor().AddTask([stopper=std::move(stopper)](const AsyncAgent& agent)
            {
                SimpleTimer timer;
                constexpr uint64_t stepTime = 1000 * 1000 / 120; // us
                while (GetViewMap().size() > 0)
                {
                    timer.Start();
                    FilterMessage();
                    glutMainLoopEvent();
                    timer.Stop();
                    const auto uiTime = timer.ElapseUs();
                    if (uiTime < stepTime && stepTime - uiTime > 1000)
                    {
                        const auto leftTime = stepTime - uiTime;
                        if (leftTime > 1000)
                            agent.Sleep(static_cast<uint32_t>(leftTime / 1000));
                    }
                    else
                        agent.YieldThis();
                }
                stopper();
            }, u"FGEventLoop", StackSize::Large);
        });
    }
};


FreeGLUTView _FreeGLUTView::getSelf()
{
    return this->shared_from_this();
}

void _FreeGLUTView::usethis()
{
    glutSetWindow(wdID);
}

void _FreeGLUTView::display()
{
    //SimpleTimer timer;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (funDisp != nullptr)
        funDisp(getSelf());
    glutSwapBuffers();
    //timer.Stop();
    //fgvLog().debug(L"Display cost {} us\n", timer.ElapseUs());
}

void _FreeGLUTView::reshape(const int w, const int h)
{
    width = w; height = h;
    //glViewport((w & 0x3f) / 2, (h & 0x3f) / 2, w & 0xffc0, h & 0xffc0);
    if (funReshape != nullptr)
        funReshape(getSelf(), w, h);
    refresh();
}

void _FreeGLUTView::onKeyboard(int key, int x, int y)
{
    uint8_t newkey;
    switch (key)
    {
    case GLUT_KEY_LEFT:
        newkey = (uint8_t)Key::Left; break;
    case GLUT_KEY_RIGHT:
        newkey = (uint8_t)Key::Right; break;
    case GLUT_KEY_UP:
        newkey = (uint8_t)Key::Up; break;
    case GLUT_KEY_DOWN:
        newkey = (uint8_t)Key::Down; break;
    case GLUT_KEY_PAGE_UP:
        newkey = (uint8_t)Key::PageUp; break;
    case GLUT_KEY_PAGE_DOWN:
        newkey = (uint8_t)Key::PageDown; break;
    case GLUT_KEY_HOME:
        newkey = (uint8_t)Key::Home; break;
    case GLUT_KEY_END:
        newkey = (uint8_t)Key::End; break;
    case GLUT_KEY_INSERT:
        newkey = (uint8_t)Key::Insert; break;
    case GLUT_KEY_F1:
    case GLUT_KEY_F2:
    case GLUT_KEY_F3:
    case GLUT_KEY_F4:
    case GLUT_KEY_F5:
    case GLUT_KEY_F6:
    case GLUT_KEY_F7:
    case GLUT_KEY_F8:
    case GLUT_KEY_F9:
    case GLUT_KEY_F10:
    case GLUT_KEY_F11:
    case GLUT_KEY_F12:
        newkey = static_cast<uint8_t>((uint8_t)Key::F1 + (key - GLUT_KEY_F1)); break;
        break;
    default:
        return;
    }
    onKey(newkey, x, y);
}

void _FreeGLUTView::onKeyboard(unsigned char key, int x, int y)
{
    onKey(key, x, y);
}

void _FreeGLUTView::onKey(uint8_t key, const int x, const int y)
{
    if (funKeyEvent != nullptr)
    {
        const auto mk = glutGetModifiers();
        const bool isCtrl = ((mk & GLUT_ACTIVE_CTRL) != 0);
        const bool isShift = ((mk & GLUT_ACTIVE_SHIFT) != 0);
        const bool isAlt = ((mk & GLUT_ACTIVE_ALT) != 0);
        if (isCtrl)//get real key when ctrl pressed
        {
            if (key <= 26)//character
                key = key + 'a';
            else if (key <= 29)//[\]
                key += (91 - 27);
            else if (key == 10)//enter
                key = 13;
            else if (key == 127)//delete->backspace
                key = 8;
        }
        funKeyEvent(getSelf(), KeyEvent(key, x, y, isCtrl, isShift, isAlt));
    }
}

void _FreeGLUTView::onWheel(int, int dir, int x, int y)
{
    if (funMouseEvent == nullptr)
        return;
    //forward if dir > 0, backward if dir < 0
    funMouseEvent(getSelf(), MouseEvent(MouseEventType::Wheel, MouseButton::None, x, height - y, dir, 0));
}

void _FreeGLUTView::onMouse(int x, int y)
{
    if (funMouseEvent == nullptr)
        return;
    if (isMovingMouse != (uint8_t)MouseButton::None)
    {
        if (!isMoved)
        {
            const float adx = std::abs(x - sx)*1.0f; const float ady = std::abs(y - sy)*1.0f;
            if (adx / width > 0.002f || ady / height > 0.002f)
                isMoved = true;
            else
                return;
        }
        const int dx = x - lx, dy = y - ly;
        lx = x, ly = y;
        //origin in GLUT is at TopLeft, While OpenGL choose BottomLeft as origin, hence dy should be fliped
        funMouseEvent(getSelf(), MouseEvent(MouseEventType::Moving, static_cast<MouseButton>(isMovingMouse), x, height - y, dx, -dy));
    }
}

void _FreeGLUTView::onMouse(int button, int state, int x, int y)
{
    if (funMouseEvent == nullptr)
        return;
    MouseButton btn;
    if (button == GLUT_LEFT_BUTTON)
        btn = MouseButton::Left;
    else if (button == GLUT_MIDDLE_BUTTON)
        btn = MouseButton::Middle;
    else if (button == GLUT_RIGHT_BUTTON)
        btn = MouseButton::Right;
    if (state == GLUT_DOWN)
    {
        isMovingMouse = (uint8_t)btn;
        lx = sx = x, ly = sy = y; isMoved = !deshake;
        funMouseEvent(getSelf(), MouseEvent(MouseEventType::Down, btn, x, height - y, 0, 0));
    }
    else
    {
        isMovingMouse = (uint8_t)MouseButton::None;
        funMouseEvent(getSelf(), MouseEvent(MouseEventType::Up, btn, x, height - y, 0, 0));
    }
}

void _FreeGLUTView::onDropFile(const u16string& fname)
{
    if (funDropFile == nullptr)
        return;
    funDropFile(getSelf(), fname);
}

void _FreeGLUTView::onClose()
{
    if (funOnClose == nullptr)
        return;
    funOnClose(getSelf());
}


_FreeGLUTView::_FreeGLUTView(const int w, const int h)
{
#if defined(_WIN32)
    const auto screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const auto screenHeight = GetSystemMetrics(SM_CYSCREEN);
#else
    Display *display = XOpenDisplay(nullptr);
    const auto screenWidth = XDisplayWidth(display, DefaultScreen(display));
    const auto screenHeight = XDisplayHeight(display, DefaultScreen(display));
#endif
    glutInitWindowSize(w, h);
    glutInitWindowPosition((screenWidth - w) / 2, (screenHeight - h) / 2);
    wdID = glutCreateWindow("");
    FreeGLUTManager::regist(this);
}


_FreeGLUTView::~_FreeGLUTView()
{
    if (FreeGLUTManager::unregist(this))
        glutDestroyWindow(wdID);
}

void _FreeGLUTView::setTimerCallback(FuncTimer funTime, const uint16_t ms)
{
    FreeGLUTManager::Executor().AddTask([self = getSelf(), task = std::move(funTime), timer = SimpleTimer(), ms](const AsyncAgent& agent) mutable
    {
        while (true)
        {
            timer.Stop();
            const auto shouldContinue = task(self, (uint32_t)timer.ElapseMs());
            if (!shouldContinue)
                break;
            timer.Start();
            agent.Sleep(ms);
        }
    });
}

void _FreeGLUTView::setTitle(const string& title)
{
    usethis();
    glutSetWindowTitle(title.c_str());
}

void _FreeGLUTView::refresh()
{
    glutPostWindowRedisplay(wdID);
}


void _FreeGLUTView::invoke(std::function<bool(void)> task)
{
    auto self = getSelf();
    auto fut = FreeGLUTManager::Executor().AddTask([self=std::move(self), task=std::move(task)](const AsyncAgent& agent)
    {
        if (task())
            self->refresh();
        return true;
    });
    fut->wait();
    return;
}


}


void FreeGLUTViewInit()
{
    int args = 0; char **argv = nullptr;
    glutInit(&args, argv);
    //glutInitContextVersion(3, 2);
    glutInitContextFlags(GLUT_DEBUG);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
    //glutSetOption(GLUT_RENDERING_CONTEXT, GLUT_USE_CURRENT_CONTEXT);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
#if defined(_WIN32)
    const auto screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const auto screenHeight = GetSystemMetrics(SM_CYSCREEN);
#else
    Display *display = XOpenDisplay(nullptr);
    const auto screenWidth = XDisplayWidth(display, DefaultScreen(display));
    const auto screenHeight = XDisplayHeight(display, DefaultScreen(display));
#endif
    fgvLog().info(L"screen W/H [{},{}]\n", screenWidth, screenHeight);
}

void FreeGLUTViewRun()
{
    detail::FreeGLUTManager::Run();
}



}