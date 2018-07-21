#include "OGLVRely.h"
#include "OpenGLViewEvents.h"
#include "common/TimeUtil.hpp"
#include <string>
#include <vector>

#pragma unmanaged
thread_local HGLRC curRC = nullptr;
static void makeCurrent(HDC hDC, HGLRC hRC)
{
    if (curRC != hRC)
        wglMakeCurrent(hDC, curRC = hRC);
}
#pragma managed


using namespace System;
using namespace System::Windows::Forms;

namespace OpenGLView
{
    public ref class OGLView : public Control
    {
    private:
        static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
        static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
        static PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = nullptr;
        static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;
        HDC hDC = nullptr;
        HGLRC hRC = nullptr;
        MouseButton CurMouseButton = MouseButton::None;
        int lastX, lastY, startX, startY, curX, curY;
        bool isMoved;
        bool isCapital = false;
        uint64_t *drawTimes = nullptr;
        uint64_t sumTime = 0;
    public:
        uint64_t rfsCount = 0;
        property bool ResizeBGDraw;
        property bool Deshake;
        property bool IsCapital
        {
            bool get() { return isCapital; }
        }
        property uint64_t AvgDrawTime
        {
            uint64_t get() { return sumTime / (rfsCount > 30 ? 30 : rfsCount); }
        }
        delegate void DrawEventHandler();
        delegate void ResizeEventHandler(Object^ o, ResizeEventArgs^ e);
        delegate void MouseEventExHandler(Object^ o, MouseEventExArgs^ e);
        delegate void KeyBoardEventHandler(Object^ o, KeyBoardEventArgs^ e);

        event DrawEventHandler^ Draw;
        event ResizeEventHandler^ Resize;
        event MouseEventExHandler^ MouseAction;
        event KeyBoardEventHandler^ KeyAction;

        static OGLView()
        {
            HWND tmpWND = CreateWindow(
                L"Core", L"Fake Window",            // window class, title
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
            
            wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
            wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
            wglGetExtensionsStringEXT = reinterpret_cast<PFNWGLGETEXTENSIONSSTRINGEXTPROC>(wglGetProcAddress("wglGetExtensionsStringEXT"));
            const auto exts = wglGetExtensionsStringEXT();
            if (strstr(exts, "WGL_EXT_swap_control_tear") != nullptr)
                wglSwapIntervalEXT = reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(wglGetProcAddress("wglSwapIntervalEXT"));
            
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(tmpRC);
            DeleteDC(tmpDC);
            DestroyWindow(tmpWND);
        }
        OGLView() : OGLView(false, 0) { }
        OGLView(const bool isSRGB, const uint32_t multiSample)
        {
            drawTimes = new uint64_t[30];
            memset(drawTimes, 0x0, sizeof(uint64_t) * 30);

            this->ImeMode = System::Windows::Forms::ImeMode::Disable;
            ResizeBGDraw = true;
            Deshake = true;
            hDC = GetDC(HWND(this->Handle.ToPointer()));

            std::vector<int32_t> pixelAttribs(
                {
                WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
                WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
                WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
                WGL_COLOR_BITS_ARB, 32,
                WGL_ALPHA_BITS_ARB, 8,
                WGL_DEPTH_BITS_ARB, 24,
                WGL_STENCIL_BITS_ARB, 8,
                });
            if (isSRGB)
            {
                pixelAttribs.push_back(WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB);
                pixelAttribs.push_back(GL_TRUE);
            }
            if (multiSample > 0)
            {
                pixelAttribs.push_back(WGL_SAMPLE_BUFFERS_ARB);
                pixelAttribs.push_back(1);
                pixelAttribs.push_back(WGL_SAMPLES_ARB);
                pixelAttribs.push_back((int32_t)multiSample);
            }
            pixelAttribs.push_back(0);
            int pixelFormatID; UINT numFormats;
            wglChoosePixelFormatARB(hDC, pixelAttribs.data(), NULL, 1, &pixelFormatID, &numFormats);
            PIXELFORMATDESCRIPTOR pfd;
            DescribePixelFormat(hDC, pixelFormatID, sizeof(pfd), &pfd);
            SetPixelFormat(hDC, pixelFormatID, &pfd);

            /*if (!baseRC)
            {
                baseRC = wglCreateContext(hDC);
                wglMakeCurrent(hDC, curRC = baseRC);
            }*/
            static const int32_t ctxAttrb[] =
            {
                /*WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                WGL_CONTEXT_MINOR_VERSION_ARB, 2,*/
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
                0
            };
            hRC = wglCreateContextAttribsARB(hDC, nullptr, ctxAttrb);
            makeCurrent(hDC, hRC);
            if (wglSwapIntervalEXT)
                wglSwapIntervalEXT(-1);
        }
        ~OGLView() { this->!OGLView(); };
        !OGLView()
        {
            if (drawTimes)
            {
                delete drawTimes;
                drawTimes = nullptr;
            }
            makeCurrent(nullptr, nullptr);
            wglDeleteContext(hRC);
            DeleteDC(hDC);
        }
    protected:
        void OnResize(EventArgs^ e) override
        {
            //makeCurrent(hDC, hRC);
            Control::OnResize(e);
            //glViewport((Width & 0x3f) / 2, (Height & 0x3f) / 2, Width & 0xffc0, Height & 0xffc0);
            Resize((Object^)this, gcnew ResizeEventArgs(Width, Height));
            this->Invalidate();
        }
        void OnPaintBackground(PaintEventArgs^ pevent) override
        {
            if (ResizeBGDraw)
                Control::OnPaintBackground(pevent);
        }
        void OnPaint(PaintEventArgs^ e) override
        {
            common::SimpleTimer timer;
            timer.Start();
            //makeCurrent(hDC, hRC);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            Draw();
            SwapBuffers(hDC);
            timer.Stop();
            auto& objTime = drawTimes[(rfsCount++) % 30];
            sumTime -= objTime;
            objTime = timer.ElapseUs();
            sumTime += objTime;
        }

        void OnMouseDown(MouseEventArgs^ e) override
        {
            auto earg = gcnew MouseEventExArgs(MouseEventType::Down, MouseButton::None, e->X, e->Y, 0, 0);
            switch (e->Button)
            {
            case ::MouseButtons::Left:
                earg->Button = MouseButton::Left; break;
            case ::MouseButtons::Right:
                earg->Button = MouseButton::Right; break;
            case ::MouseButtons::Middle:
                earg->Button = MouseButton::Middle; break;
            }
            lastX = e->X; lastY = e->Y;
            if (CurMouseButton == MouseButton::None)
            {
                isMoved = !Deshake;
                startX = lastX, startY = lastY;
            }
            CurMouseButton = CurMouseButton | earg->Button;
            Control::OnMouseDown(e);
            MouseAction(this, earg);
        }
        void OnMouseUp(MouseEventArgs^ e) override
        {
            auto earg = gcnew MouseEventExArgs(MouseEventType::Up, MouseButton::None, e->X, e->Y, 0, 0);
            switch (e->Button)
            {
            case ::MouseButtons::Left:
                earg->Button = MouseButton::Left; break;
            case ::MouseButtons::Right:
                earg->Button = MouseButton::Right; break;
            case ::MouseButtons::Middle:
                earg->Button = MouseButton::Middle; break;
            }
            CurMouseButton = CurMouseButton & (~earg->Button);
            Control::OnMouseUp(e);
            MouseAction(this, earg);
        }
        void OnMouseWheel(MouseEventArgs^ e) override
        {
            MouseAction(this, gcnew MouseEventExArgs(MouseEventType::Wheel, CurMouseButton, e->X, e->Y, e->Delta / 120, 0));
            Control::OnMouseWheel(e);
        }
        void OnMouseMove(MouseEventArgs^ e) override
        {
            curX = e->X, curY = e->Y;
            if (CurMouseButton != MouseButton::None)
            {
                if (!isMoved)
                {
                    const float adx = std::abs(e->X - startX)*1.0f; const float ady = std::abs(e->Y - startY)*1.0f;
                    if (adx > 0.002f * Width || ady > 0.002f * Height)
                        isMoved = true;
                }
                if (isMoved)
                {
                    const int dX = e->X - lastX, dY = e->Y - lastY;
                    lastX = e->X, lastY = e->Y;
                    //origin in Winforms is at TopLeft, While OpenGL choose BottomLeft as origin, hence dy should be fliped
                    MouseAction(this, gcnew MouseEventExArgs(MouseEventType::Moving, CurMouseButton, e->X, Height - e->Y, dX, -dY));
                }
            }
            Control::OnMouseMove(e);
        }
        
        bool IsInputKey(Keys key) override
        {
            Console::WriteLine(L"key IsInputKey {0}\n", key);
            auto pureKey = key & (~(Keys::Shift | Keys::Control | Keys::Alt));
            if (pureKey == Keys::Left || pureKey == Keys::Right || pureKey == Keys::Up || pureKey == Keys::Down || pureKey == Keys::Tab || pureKey == Keys::Escape || pureKey == Keys::ShiftKey)
            {
                return true;
            }
            else
                return Control::IsInputKey(key);
        }
        
        void OnKeyDown(KeyEventArgs^ e) override
        {
            //Console::WriteLine(L"key OnKeyDown {0}\n", e->KeyValue);
            KeyBoardEventArgs^ keyDownEvent = gcnew KeyBoardEventArgs(L'\0', curX, curY, e->Control, e->Shift, e->Alt);
            if (e->KeyCode == Keys::Capital)
                isCapital = !isCapital;
            else if (e->KeyValue >= 65 && e->KeyValue <= 90)
            {
                keyDownEvent->key = (e->Shift ^ isCapital ? L'A' : L'a') + (e->KeyValue - 65);
            }
            else if (e->KeyValue >= 96 && e->KeyValue <= 105)
                keyDownEvent->key = L'0' + (e->KeyValue - 96);
            else if (e->KeyValue >= 112 && e->KeyValue <= 123)
                keyDownEvent->key = (wchar_t)Key::F1 + (e->KeyValue - 112);
            else if (e->KeyValue >= 37 && e->KeyValue <= 40)
                keyDownEvent->key = (wchar_t)Key::Left + (e->KeyValue - 37);
            else switch (e->KeyCode)
            {
            case Keys::Home:
                keyDownEvent->key = (wchar_t)Key::Home; break;
            case Keys::End:
                keyDownEvent->key = (wchar_t)Key::End; break;
            case Keys::PageUp:
                keyDownEvent->key = (wchar_t)Key::PageUp; break;
            case Keys::PageDown:
                keyDownEvent->key = (wchar_t)Key::PageDown; break;
            case Keys::Insert:
                keyDownEvent->key = (wchar_t)Key::Insert; break;
            case Keys::Delete:
                keyDownEvent->key = (wchar_t)Key::Delete; break;
            case Keys::Return:
                keyDownEvent->key = (wchar_t)Key::Enter; break;
            }
            if (keyDownEvent->key != L'\0')
            {
                KeyAction(this, keyDownEvent);
                //e->Handled = true;
            }
            Control::OnKeyDown(e);
        }
    };

    public ref class OGLViewSRGB : public OGLView
    {
    public:
        OGLViewSRGB() : OGLView(true, 0) {}
    };
}

