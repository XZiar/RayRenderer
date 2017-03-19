#include "OGLVRely.h"

#include <msclr\marshal_cppstd.h>
#include "OpenGLViewEvents.h"

using namespace msclr::interop;
using namespace System;
using namespace System::Windows::Forms;

namespace OpenGLView
{
using std::wstring;
	public ref class OGLView : public Control
	{
	private:
		static HGLRC baseRC = nullptr, curRC = nullptr;
		HDC hDC = nullptr;
		HGLRC hRC = nullptr;
		MouseButton curMouseBTN = MouseButton::None;
		KeyBoardEventArgs^ curKeyEvent;
		int lastX, lastY, startX, startY, curX, curY;
		bool isMoved;
		bool isCaptital = false;
		static void makeCurrent(HDC hDC, HGLRC hRC)
		{
			if (curRC != hRC)
				wglMakeCurrent(hDC, curRC = hRC);
		}
		static void initRC(HDC hDC)
		{
			if (baseRC != nullptr)
				return;
			baseRC = wglCreateContext(hDC);
			wglMakeCurrent(hDC, curRC = baseRC);
		}
		static void initExtension()
		{
			auto wglewGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
			const auto exts = wglewGetExtensionsStringEXT();
			if (strstr(exts, "WGL_EXT_swap_control_tear") != nullptr)
			{
				auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
				//Adaptive Vsync
				wglSwapIntervalEXT(-1);
			}
		}
	public:
		UINT64 rfsCount = 0;
		property bool ResizeBGDraw;
		property bool deshake;
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
		}
		OGLView()
		{
			ResizeBGDraw = true;
			deshake = true;
			hDC = GetDC(HWND(this->Handle.ToPointer()));
			static PIXELFORMATDESCRIPTOR pfd =  // pfd Tells Windows How We Want Things To Be
			{
				sizeof(PIXELFORMATDESCRIPTOR),  // Size Of This Pixel Format Descriptor
				1,                              // Version Number
				PFD_DRAW_TO_WINDOW/*Support Window*/ | PFD_SUPPORT_OPENGL/*Support OpenGL*/ | PFD_DOUBLEBUFFER/*Support Double Buffering*/,
				PFD_TYPE_RGBA,                  // Request An RGBA Format
				24,                             // Select Our Color Depth
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
			const int PixelFormat = ChoosePixelFormat(hDC, &pfd);
			SetPixelFormat(hDC, PixelFormat, &pfd);
			initRC(hDC);
			int ctxAttrb[] =
			{
				/*WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
				WGL_CONTEXT_MINOR_VERSION_ARB, 2,*/
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
				WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
				0
			};
			auto wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
			hRC = wglCreateContextAttribsARB(hDC, baseRC, ctxAttrb);
			makeCurrent(hDC, hRC);
			initExtension();
		}
		~OGLView() { this->!OGLView(); };
		!OGLView()
		{
			makeCurrent(nullptr, nullptr);
			wglDeleteContext(hRC);
			DeleteDC(hDC);
		}
	protected:
		void OnResize(EventArgs^ e) override
		{
			makeCurrent(hDC, hRC);
			Control::OnResize(e);
			glViewport((Width & 0x3f) / 2, (Height & 0x3f) / 2, Width & 0xffc0, Height & 0xffc0);
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
			makeCurrent(hDC, hRC);
			rfsCount++;
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			Draw();
			SwapBuffers(hDC);
		}

		void OnMouseDown(MouseEventArgs^ e) override
		{
			auto earg = gcnew MouseEventExArgs(MouseEventType::Down, MouseButton::None, e->X, e->Y, 0, 0);
			switch (e->Button)
			{
			case ::MouseButtons::Left:
				earg->btn = MouseButton::Left; break;
			case ::MouseButtons::Right:
				earg->btn = MouseButton::Right; break;
			case ::MouseButtons::Middle:
				earg->btn = MouseButton::Middle; break;
			}
			lastX = e->X; lastY = e->Y;
			if (curMouseBTN == MouseButton::None)
			{
				isMoved = !deshake;
				startX = lastX, startY = lastY;
			}
			curMouseBTN = curMouseBTN | earg->btn;
			Control::OnMouseDown(e);
			MouseAction(this, earg);
		}
		void OnMouseUp(MouseEventArgs^ e) override
		{
			auto earg = gcnew MouseEventExArgs(MouseEventType::Up, MouseButton::None, e->X, e->Y, 0, 0);
			switch (e->Button)
			{
			case ::MouseButtons::Left:
				earg->btn = MouseButton::Left; break;
			case ::MouseButtons::Right:
				earg->btn = MouseButton::Right; break;
			case ::MouseButtons::Middle:
				earg->btn = MouseButton::Middle; break;
			}
			curMouseBTN = curMouseBTN ^ earg->btn;
			Control::OnMouseUp(e);
			MouseAction(this, earg);
		}
		void OnMouseWheel(MouseEventArgs^ e) override
		{
			MouseAction(this, gcnew MouseEventExArgs(MouseEventType::Wheel, curMouseBTN, e->X, e->Y, e->Delta / 120, 0));
			Control::OnMouseWheel(e);
		}
		void OnMouseMove(MouseEventArgs^ e) override
		{
			curX = e->X, curY = e->Y;
			if (curMouseBTN != MouseButton::None)
			{
				if (!isMoved)
				{
					const float adx = std::abs(e->X - startX)*1.0f; const float ady = std::abs(e->Y - startY)*1.0f;
					if (adx / Width > 0.002f || ady / Height > 0.002f)
						isMoved = true;
				}
				if (isMoved)
				{
					const int dX = e->X - lastX, dY = e->Y - lastY;
					lastX = e->X, lastY = e->Y;
					//origin in Winforms is at TopLeft, While OpenGL choose BottomLeft as origin, hence dy should be fliped
					MouseAction(this, gcnew MouseEventExArgs(MouseEventType::Moving, curMouseBTN, e->X, Height - e->Y, dX, -dY));
				}
			}
			Control::OnMouseMove(e);
		}

		bool ProcessDialogKey(Keys key) override
		{
			if (key == Keys::Left || key == Keys::Right || key == Keys::Up || key == Keys::Down || key == Keys::Tab || key == Keys::Escape)
				return false;
			else
				return Control::ProcessDialogKey(key);
		}
		void OnKeyDown(KeyEventArgs^ e) override
		{
			curKeyEvent = gcnew KeyBoardEventArgs(L'\0', curX, curY, e->Control, e->Shift, e->Alt);
			if (e->KeyCode == Keys::Capital)
				isCaptital = true;
			else if (e->KeyValue >= 65 && e->KeyValue <= 90)
			{
				curKeyEvent->key = (e->Shift ^ isCaptital ? L'A' : L'a') + (e->KeyValue - 65);
			}
			else if (e->KeyValue >= 96 && e->KeyValue <= 105)
				curKeyEvent->key = L'0' + (e->KeyValue - 96);
			else if (e->KeyValue >= 112 && e->KeyValue <= 123)
				curKeyEvent->key = (wchar_t)Key::F1 + (e->KeyValue - 112);
			else if (e->KeyValue >= 37 && e->KeyValue <= 40)
				curKeyEvent->key = (wchar_t)Key::Left + (e->KeyValue - 37);
			else switch (e->KeyCode)
			{
			case Keys::Home:
				curKeyEvent->key = (wchar_t)Key::Home; break;
			case Keys::End:
				curKeyEvent->key = (wchar_t)Key::End; break;
			case Keys::PageUp:
				curKeyEvent->key = (wchar_t)Key::PageUp; break;
			case Keys::PageDown:
				curKeyEvent->key = (wchar_t)Key::PageDown; break;
			case Keys::Insert:
				curKeyEvent->key = (wchar_t)Key::Insert; break;
			case Keys::Delete:
				curKeyEvent->key = (wchar_t)Key::Delete; break;
			}
			if (curKeyEvent->key != L'\0')
			{
				KeyAction(this, curKeyEvent);
				e->Handled = true;
			}
			Control::OnKeyDown(e);
		}
		void OnKeyPress(KeyPressEventArgs^ e) override
		{
			curKeyEvent->key = e->KeyChar;
			KeyAction(this, curKeyEvent);
			Control::OnKeyPress(e);
		}
		void OnKeyUp(KeyEventArgs^ e) override
		{
			if (e->KeyCode == Keys::Capital)
				isCaptital = false;
			printf("key up %d\n", e->KeyValue);
			Control::OnKeyUp(e);
		}
	};
}

