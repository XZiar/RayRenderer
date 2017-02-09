#include "OGLVRely.h"

#include <msclr\marshal_cppstd.h>

using namespace msclr::interop;
using namespace System;
using namespace System::Windows::Forms;

namespace OpenGLView
{
	public ref class ResizeEventArgs : public EventArgs
	{
	public:
		int Width;
		int Height;

		ResizeEventArgs(const int w, const int h) :Width(w), Height(h) { };
	};
	public ref class BTN abstract sealed
	{
	public:
		static const int Left = 0x1;
		static const int Right = 0x2;
		static const int Middle = 0x4;
	};
	public ref class MouseDragEventArgs : public EventArgs
	{
	public:
		int X, Y;
		int dx, dy;
		int btn;

		MouseDragEventArgs(const int b, const int idx, const int idy, const int ix, const int iy)
			:btn(b), dx(idx), dy(idy), X(ix), Y(iy)
		{
		};
	};

	using std::move;
	using std::string;
	public ref class OGLView : public Control
	{
	private:
		HDC hDC = nullptr;
		HGLRC hRC = nullptr;
		bool btn_Left = false, btn_Right = false;
		int lastX, lastY;
		int m_pW, m_pH;
		Timers::Timer^ AutoRefreshTimer;
		void construct()
		{
			glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
			glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
			glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		}
	public:
		UINT64 rfsCount = 0;
		property bool ResizeBGDraw;
		delegate void DrawEventHandler();
		delegate void ResizeEventHandler(Object^ o, ResizeEventArgs^ e);
		delegate void MouseDragEventHandler(Object^ o, MouseDragEventArgs^ e);

		event ResizeEventHandler^ Resize;
		event DrawEventHandler^ Draw;
		event MouseDragEventHandler^ MouseDrag;

		static OGLView()
		{
			//glewInit();
		}
		OGLView()
		{
			ResizeBGDraw = true;
			AutoRefreshTimer = gcnew Timers::Timer(40);
			AutoRefreshTimer->AutoReset = true;
			AutoRefreshTimer->Elapsed += gcnew Timers::ElapsedEventHandler(this, &OGLView::AutoRfs);
			hDC = GetDC(HWND(this->Handle.ToPointer()));
			static PIXELFORMATDESCRIPTOR pfd =  // pfd Tells Windows How We Want Things To Be
			{
				sizeof(PIXELFORMATDESCRIPTOR),  // Size Of This Pixel Format Descriptor
				1,                              // Version Number
				PFD_DRAW_TO_WINDOW |            // Format Must Support Window
				PFD_SUPPORT_OPENGL |            // Format Must Support OpenGL
				PFD_DOUBLEBUFFER,               // Must Support Double Buffering
				PFD_TYPE_RGBA,                  // Request An RGBA Format
				24,                             // Select Our Color Depth
				0, 0, 0, 0, 0, 0,               // Color Bits Ignored
				0,                              // No Alpha Buffer
				0,                              // Shift Bit Ignored
				0,                              // No Accumulation Buffer
				0, 0, 0, 0,                     // Accumulation Bits Ignored
				24,                             // 16Bit Z-Buffer (Depth Buffer) 
				8,                              // No Stencil Buffer
				0,                              // No Auxiliary Buffer
				PFD_MAIN_PLANE,                 // Main Drawing Layer
				0,                              // Reserved
				0, 0, 0                         // Layer Masks Ignored
			};
			const int PixelFormat = ChoosePixelFormat(hDC, &pfd);
			SetPixelFormat(hDC, PixelFormat, &pfd);
			hRC = wglCreateContext(hDC);
			wglMakeCurrent(hDC, hRC);
			construct();
		}
		~OGLView() { this->!OGLView(); };
		!OGLView()
		{
			AutoRefreshTimer->Enabled = false;
			AutoRefreshTimer->Close();
			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(hRC);
			DeleteDC(hDC);
		}
	protected:
		void OnPaintBackground(PaintEventArgs^ pevent) override
		{
			if (ResizeBGDraw)
				Control::OnPaintBackground(pevent);
		}
		void OnResize(EventArgs^ e) override
		{
			Control::OnResize(e);
			glViewport((Width & 0x3f) / 2, (Height & 0x3f) / 2, Width & 0xffc0, Height & 0xffc0);
			Resize((Object^)this, gcnew ResizeEventArgs(Width, Height));
			this->Invalidate();
		}
		void OnPaint(PaintEventArgs^ e) override
		{
			wglMakeCurrent(hDC, hRC);
			rfsCount++;
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			Draw();
			SwapBuffers(hDC);
		}
		void OnMouseDown(MouseEventArgs^ e) override
		{
			switch (e->Button)
			{
			case ::MouseButtons::Left:
				btn_Left = true;
				lastX = e->X;
				lastY = e->Y;
				break;
			case ::MouseButtons::Right:
				btn_Right = true;
				lastX = e->X;
				lastY = e->Y;
				break;
			}
			Control::OnMouseDown(e);
		}
		void OnMouseUp(MouseEventArgs^ e) override
		{
			switch (e->Button)
			{
			case ::MouseButtons::Left:
				btn_Left = false;
				break;
			case ::MouseButtons::Right:
				btn_Right = false;
				break;
			}
			Control::OnMouseUp(e);
		}
		void OnMouseMove(MouseEventArgs^ e) override
		{
			if (btn_Left || btn_Right)
			{
				int dX = e->X - lastX, dY = e->Y - lastY;
				lastX = e->X, lastY = e->Y;
				int btn = 0x0;
				if (btn_Left)
					btn += BTN::Left;
				if (btn_Right)
					btn += BTN::Right;
				MouseDrag(this, gcnew MouseDragEventArgs(btn, dX, dY, e->X, e->Y));
			}
			Control::OnMouseMove(e);
		}
		void AutoRfs(Object ^sender, Timers::ElapsedEventArgs ^e)
		{
			this->Invalidate();
		}
	};
}

