#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <commdlg.h>
#include <conio.h>
#include <cstdio>
#include <locale>
#include <vector>
#include <thread>

#define FREEGLUT_STATIC
#include <GL/freeglut.h>//Free GLUT Header

namespace glutview
{

class GLUTHacker final
{
	friend class FreeGLUTView;
	static FreeGLUTView* table[8];
	static HGLRC rcs[8];
	static void makeshare(HGLRC rc, const uint8_t pos)
	{
		for (uint8_t a = 0; a < 8; ++a)
		{
			if (a != pos && table[a] != nullptr)//can use this
				wglShareLists(rcs[a], rc);
		}
	}
	static void regist(FreeGLUTView* view)
	{
		for (uint8_t a = 0; a < 8; ++a)
			if (table[a] == nullptr)
			{
				table[a] = view;
				rcs[a] = wglGetCurrentContext();
				makeshare(rcs[a], a);
				return;
			}
	}
	static void unregist(FreeGLUTView* view)
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
	static void(*getDisplay(FreeGLUTView* view))(void)
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
	static void(*getReshape(FreeGLUTView* view))(int w, int h)
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
};

FreeGLUTView* GLUTHacker::table[8] = { nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr };
HGLRC GLUTHacker::rcs[8];
}