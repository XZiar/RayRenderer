#pragma once

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <shellapi.h>

#include <cstdint>
#include <cstdio>

#ifndef _DEBUG
#   define NDEBUG 1
#endif

#define FREEGLUT_STATIC
#include <GL/freeglut.h>//Free GLUT Header

namespace glutview
{

class GLUTHacker final
{
	friend class _FreeGLUTView;
	static _FreeGLUTView* table[8];
	static HGLRC rcs[8];
	static HWND hwnds[8];
	static WNDPROC oldWndProc;
	static void makeshare(HGLRC rc, const uint8_t pos)
	{
		for (uint8_t a = 0; a < 8; ++a)
		{
			if (a != pos && table[a] != nullptr)//can use this
				wglShareLists(rcs[a], rc);
		}
	}
	static _FreeGLUTView* getView(const HWND hwnd)
	{
		for (uint8_t a = 0; a < 8; ++a)
			if (hwnds[a] == hwnd)
				return table[a];
		return nullptr;
	}
	static uint8_t regist(_FreeGLUTView* view)
	{
		for (uint8_t a = 0; a < 8; ++a)
		{
			if (table[a] == nullptr)
			{
				table[a] = view;
				rcs[a] = wglGetCurrentContext();
				const auto hdc = wglGetCurrentDC();
				hwnds[a] = WindowFromDC(hdc);
				DragAcceptFiles(hwnds[a], TRUE);
				oldWndProc = (WNDPROC)SetWindowLongPtr(hwnds[a], GWLP_WNDPROC, (intptr_t)&HackWndProc);
				makeshare(rcs[a], a);
				return a;
			}
		}
		return UINT8_MAX;
	}
	static void unregist(_FreeGLUTView* view)
	{
		for (uint8_t a = 0; a < 8; ++a)
			if (table[a] == view)
			{
				table[a] = nullptr;
				return;
			}
	}
	template<uint8_t N>
	static void display()
	{
		table[N]->display();
	}
	template<uint8_t N>
	static void reshape(int w, int h)
	{
		table[N]->reshape(w, h);
	}
	template<uint8_t N>
	static void onKey1(unsigned char key, int x, int y)
	{
		table[N]->onKeyboard(key, x, y);
	}
	template<uint8_t N>
	static void onkey2(int key, int x, int y)
	{
		table[N]->onKeyboard(key, x, y);
	}
	template<uint8_t N>
	static void onWheel(int button, int dir, int x, int y)
	{
		table[N]->onWheel(button, dir, x, y);
	}
	template<uint8_t N>
	static void onMouse1(int x, int y)
	{
		table[N]->onMouse(x, y);
	}
	template<uint8_t N>
	static void onMouse2(int button, int state, int x, int y)
	{
		table[N]->onMouse(button, state, x, y);
	}
	static void onTimer(int value)
	{
		const uint8_t vid = value & UINT8_MAX;
		table[vid]->onTimer(value / 256);
	}
	static void setTimer(_FreeGLUTView* view, const uint16_t ms)
	{
		glutTimerFunc(ms, onTimer, (ms * 256) + view->instanceID);
	}
	static void(*getDisplay(_FreeGLUTView* view))(void)
	{
		if (table[0] == view)
			return display<0>;
		if (table[1] == view)
			return display<1>;
		if (table[2] == view)
			return display<2>;
		if (table[3] == view)
			return display<3>;
		if (table[4] == view)
			return display<4>;
		if (table[5] == view)
			return display<5>;
		if (table[6] == view)
			return display<6>;
		if (table[7] == view)
			return display<7>;
		return nullptr;
	}
	static void(*getReshape(_FreeGLUTView* view))(int w, int h)
	{
		if (table[0] == view)
			return reshape<0>;
		if (table[1] == view)
			return reshape<1>;
		if (table[2] == view)
			return reshape<2>;
		if (table[3] == view)
			return reshape<3>;
		if (table[4] == view)
			return reshape<4>;
		if (table[5] == view)
			return reshape<5>;
		if (table[6] == view)
			return reshape<6>;
		if (table[7] == view)
			return reshape<7>;
		return nullptr;
	}
	static void(*getOnKey1(_FreeGLUTView* view))(unsigned char key, int x, int y)
	{
		if (table[0] == view)
			return onKey1<0>;
		if (table[1] == view)
			return onKey1<1>;
		if (table[2] == view)
			return onKey1<2>;
		if (table[3] == view)
			return onKey1<3>;
		if (table[4] == view)
			return onKey1<4>;
		if (table[5] == view)
			return onKey1<5>;
		if (table[6] == view)
			return onKey1<6>;
		if (table[7] == view)
			return onKey1<7>;
		return nullptr;
	}
	static void(*getOnkey2(_FreeGLUTView* view))(int key, int x, int y)
	{
		if (table[0] == view)
			return onkey2<0>;
		if (table[1] == view)
			return onkey2<1>;
		if (table[2] == view)
			return onkey2<2>;
		if (table[3] == view)
			return onkey2<3>;
		if (table[4] == view)
			return onkey2<4>;
		if (table[5] == view)
			return onkey2<5>;
		if (table[6] == view)
			return onkey2<6>;
		if (table[7] == view)
			return onkey2<7>;
		return nullptr;
	}
	static void(*getOnWheel(_FreeGLUTView* view))(int button, int dir, int x, int y)
	{
		if (table[0] == view)
			return onWheel<0>;
		if (table[1] == view)
			return onWheel<1>;
		if (table[2] == view)
			return onWheel<2>;
		if (table[3] == view)
			return onWheel<3>;
		if (table[4] == view)
			return onWheel<4>;
		if (table[5] == view)
			return onWheel<5>;
		if (table[6] == view)
			return onWheel<6>;
		if (table[7] == view)
			return onWheel<7>;
		return nullptr;
	}
	static void(*getOnMouse1(_FreeGLUTView* view))(int x, int y)
	{
		if (table[0] == view)
			return onMouse1<0>;
		if (table[1] == view)
			return onMouse1<1>;
		if (table[2] == view)
			return onMouse1<2>;
		if (table[3] == view)
			return onMouse1<3>;
		if (table[4] == view)
			return onMouse1<4>;
		if (table[5] == view)
			return onMouse1<5>;
		if (table[6] == view)
			return onMouse1<6>;
		if (table[7] == view)
			return onMouse1<7>;
		return nullptr;
	}
	static void(*getOnMouse2(_FreeGLUTView* view))(int button, int state, int x, int y)
	{
		if (table[0] == view)
			return onMouse2<0>;
		if (table[1] == view)
			return onMouse2<1>;
		if (table[2] == view)
			return onMouse2<2>;
		if (table[3] == view)
			return onMouse2<3>;
		if (table[4] == view)
			return onMouse2<4>;
		if (table[5] == view)
			return onMouse2<5>;
		if (table[6] == view)
			return onMouse2<6>;
		if (table[7] == view)
			return onMouse2<7>;
		return nullptr;
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
};

_FreeGLUTView* GLUTHacker::table[8] = { nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr };
HGLRC GLUTHacker::rcs[8];
HWND GLUTHacker::hwnds[8];
WNDPROC GLUTHacker::oldWndProc;
}