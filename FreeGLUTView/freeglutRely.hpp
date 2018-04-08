#pragma once

#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX 1
#include <Windows.h>
#include <shellapi.h>

#ifndef _DEBUG
#   define NDEBUG 1
#endif

#define FREEGLUT_STATIC
#include "freeglut/freeglut.h"

#include <GL/wglext.h>

#include <cstdint>
#include <cstdio>
#include <tuple>
#include <functional>
#include <atomic>
#include <mutex>
#include <future>
#include "common/TimeUtil.hpp"
#include "common/miniLogger/miniLogger.h"
#include "FreeGLUTView.h"

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING 1
#pragma warning(disable:4996)
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#pragma warning(default:4996)


namespace glutview
{


using namespace common::mlog;
common::mlog::MiniLogger<false>& fgvLog()
{
    static MiniLogger<false> fgvlog(u"FGView", { GetConsoleBackend() });
    return fgvlog;
}


class GLUTHacker final
{
private:
    struct FGView
    {
        detail::_FreeGLUTView *view;
        const HWND hwnd;
        const HGLRC hrc;
    };
    using FGViewMap = boost::multi_index_container<FGView, boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<boost::multi_index::member<FGView, detail::_FreeGLUTView*, &FGView::view>>,
        boost::multi_index::ordered_unique<boost::multi_index::member<FGView, const HWND, &FGView::hwnd>>,
        boost::multi_index::ordered_unique<boost::multi_index::member<FGView, const HGLRC, &FGView::hrc>>
        >>;
    static FGViewMap& getMap()
    {
        static FGViewMap viewMap;
        return viewMap;
    }
    friend class detail::_FreeGLUTView;
    static inline WNDPROC oldWndProc;
    static inline std::atomic_bool shouldInvoke{ false }, readyInvoke{ false };
    static inline std::tuple<detail::_FreeGLUTView*, std::function<bool(void)>, std::promise<bool>> invokeData;
    
    static detail::_FreeGLUTView* getView()
    {
        const auto hrc = wglGetCurrentContext();
        auto& rckey = getMap().get<2>();
        auto it = rckey.find(hrc);
        if (it == rckey.end())
            return nullptr;
        return (*it).view;
    }
    static detail::_FreeGLUTView* getView(const HWND hwnd)
    {
        auto& wndkey = getMap().get<1>();
        auto it = wndkey.find(hwnd);
        if (it == wndkey.end())
            return nullptr;
        return (*it).view;
    }
    static void regist(detail::_FreeGLUTView* view)
    {
        const auto hdc = wglGetCurrentDC();
        const auto hwnd = WindowFromDC(hdc);
        const auto hrc = wglGetCurrentContext();
        DragAcceptFiles(hwnd, TRUE);
        oldWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (intptr_t)&HackWndProc);
        auto& rckey = getMap().get<2>();
        if (!rckey.empty())
        {//share gl context
            wglShareLists((*rckey.begin()).hrc, hrc);
            fgvLog().info(u"Share GL_RC {} from {}\n", (void*)hrc, (void*)(*rckey.begin()).hrc);
        }
        getMap().insert({ view,hwnd,hrc });
        //regist glutCallbacks
        view->usethis();
        glutCloseFunc(GLUTHacker::onClose);
        glutDisplayFunc(GLUTHacker::onDisplay);
        glutReshapeFunc(GLUTHacker::onReshape);
        glutKeyboardFunc(GLUTHacker::onKeyboard);
        glutSpecialFunc(GLUTHacker::onSpecial);
        glutMouseWheelFunc(GLUTHacker::onMouseWheel);
        glutMotionFunc(GLUTHacker::onMotion);
        glutMouseFunc(GLUTHacker::onMouse);
        initExtension();
    }
    static void initExtension()
    {
        auto wglewGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
        const auto exts = wglewGetExtensionsStringEXT();
        if(strstr(exts, "WGL_EXT_swap_control_tear") != nullptr)
        {
            auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
            //Adaptive Vsync
            wglSwapIntervalEXT(-1);
        }
    }
    static bool unregist(detail::_FreeGLUTView* view)
    {
        auto it = getMap().find(view);
        if (it == getMap().end())
            return false;
        getMap().erase(it);
        return true;
    }
    static std::future<bool> putInvoke(detail::_FreeGLUTView* view, std::function<bool(void)>& task)
    {
        while(shouldInvoke.exchange(true))//keep trying until it acquire the invoke
        {
            std::this_thread::yield();
        }//act like a spin-lock
        invokeData = { view,task,std::promise<bool>() };
        auto fut = std::get<2>(invokeData).get_future();
        readyInvoke.store(true);
        return fut;
    }
    //promise all operation inside this function is in GUI thread
    static void idle()
    {
        if (shouldInvoke.load())//need to invoke
        {
            while(!readyInvoke.load())
            {
                std::this_thread::yield();
            }//act like a spin-lock
            try
            {
                auto&[wd, task, pms] = invokeData;
                if (task())
                    wd->refresh();
                pms.set_value(true);
            }
        #pragma warning(disable:4101)
            catch (std::exception& e)
            {
                std::get<2>(invokeData).set_exception(std::current_exception());
            }
        #pragma warning(default:4101)
            readyInvoke.store(false);
            shouldInvoke.store(false);
        }
        else
        {
            int64_t waittime = 1000 * 1000 / 120;//us
            detail::_FreeGLUTView *objView = nullptr;
            for (auto& ele : getMap())
            {
                auto& view = *ele.view;
                if (view.timerus >= 0)
                {
                    view.uitimer.Stop();
                    const auto lefttime = view.timerus - (int64_t)view.uitimer.ElapseUs();
                    if (lefttime < waittime)
                        waittime = lefttime, objView = &view;
                }
            }
            if (waittime > 1000)//should sleep
                std::this_thread::sleep_for(std::chrono::microseconds(waittime));
            if (objView)//should vall timer
                objView->onTimer();
        }
    }
    static void onClose()
    {
        const auto view = getView();
        view->onClose();
        unregist(view);
    }
    static void onDisplay()
    {
        const auto view = getView();
        view->display();
    }
    static void onReshape(int w, int h)
    {
        const auto view = getView();
        view->reshape(w, h);
    }
    static void onKeyboard(unsigned char key, int x, int y)
    {
        const auto view = getView();
        view->onKeyboard(key, x, y);
    }
    static void onSpecial(int key, int x, int y)
    {
        const auto view = getView();
        view->onKeyboard(key, x, y);
    }
    static void onMouseWheel(int button, int dir, int x, int y)
    {
        const auto view = getView();
        view->onWheel(button, dir, x, y);
    }
    static void onMotion(int x, int y)
    {
        const auto view = getView();
        view->onMouse(x, y);
    }
    static void onMouse(int button, int state, int x, int y)
    {
        const auto view = getView();
        view->onMouse(button, state, x, y);
    }

    static LRESULT CALLBACK HackWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_DROPFILES:
            {
                HDROP hdrop = (HDROP)wParam;
                wchar_t filePath[MAX_PATH + 1];
                DragQueryFile(hdrop, 0, filePath, MAX_PATH);
                DragFinish(hdrop);
                const auto view = getView(hWnd);
                view->onDropFile(filePath);
            }
            return 0;
        }
        return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
    }
public:
    static void init()
    {
        glutIdleFunc(GLUTHacker::idle);
    }
};




}