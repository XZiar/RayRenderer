#include "FreeGLUTView.h"
#include "freeglutRely.hpp"
#include <commdlg.h>
#include <conio.h>
#include <vector>

namespace glutview
{

namespace detail
{


FreeGLUTView _FreeGLUTView::getSelf()
{
	return this;
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
		funDisp(this);
	glutSwapBuffers();
	//timer.Stop();
	//fgvLog().debug(L"Display cost {} us\n", timer.ElapseUs());
}

void _FreeGLUTView::reshape(const int w, const int h)
{
	width = w; height = h;
	glViewport((w & 0x3f) / 2, (h & 0x3f) / 2, w & 0xffc0, h & 0xffc0);
	if (funReshape != nullptr)
		funReshape(this, w, h);
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
		newkey = (uint8_t)Key::F1 + (key - GLUT_KEY_F1); break;
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
		funKeyEvent(this, KeyEvent(key, x, y, isCtrl, isShift, isAlt));
	}
}

void _FreeGLUTView::onWheel(int button, int dir, int x, int y)
{
	if (funMouseEvent == nullptr)
		return;
	//forward if dir > 0, backward if dir < 0
	funMouseEvent(this, MouseEvent(MouseEventType::Wheel, MouseButton::None, x, height - y, dir, 0));
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
		funMouseEvent(this, MouseEvent(MouseEventType::Moving, static_cast<MouseButton>(isMovingMouse), x, height - y, dx, -dy));
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
		funMouseEvent(this, MouseEvent(MouseEventType::Down, btn, x, height - y, 0, 0));
	}
	else
	{
		isMovingMouse = (uint8_t)MouseButton::None;
		funMouseEvent(this, MouseEvent(MouseEventType::Up, btn, x, height - y, 0, 0));
	}
}

void _FreeGLUTView::onTimer()
{
	if (funTimer == nullptr)
		return;
	uitimer.Stop();
	const bool isCon = funTimer(this, (uint32_t)uitimer.ElapseMs());
	if (!isCon)
		funTimer = nullptr, timerus = INT64_MIN;
	else
		uitimer.Start();
}

void _FreeGLUTView::onDropFile(const wstring& fname)
{
	if (funDropFile == nullptr)
		return;
	funDropFile(this, fname);
}

_FreeGLUTView::_FreeGLUTView(const int w, const int h)
{
	const auto screenWidth = GetSystemMetrics(SM_CXSCREEN);
	const auto screenHeight = GetSystemMetrics(SM_CYSCREEN);
	glutInitWindowSize(w, h);
	glutInitWindowPosition((screenWidth - w) / 2, (screenHeight - h) / 2);
	wdID = glutCreateWindow("");
	GLUTHacker::regist(this);
}


_FreeGLUTView::~_FreeGLUTView()
{
	if (GLUTHacker::unregist(this))
		glutDestroyWindow(wdID);
}

void _FreeGLUTView::setTimerCallback(FuncTimer funTime, const uint16_t ms)
{
	funTimer = funTime;
	timerus = ms * 1000;
	uitimer.Start();
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
	SimpleTimer timer;
	auto fut = GLUTHacker::putInvoke(this, task);
	fut.get();
	timer.Stop();
	fgvLog().debug(L"Invoke cost {} us\n", timer.ElapseUs());
	return;
}


}


void FreeGLUTViewInit()
{
	int args = 0; char **argv = nullptr;
	/*if (useNV)
		NvOptimusEnablement = 0x00000001;
	else
		NvOptimusEnablement = 0x00000000;*/
	glutInit(&args, argv);
	//glutInitContextVersion(3, 2);
	glutInitContextFlags(GLUT_DEBUG);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
	//glutSetOption(GLUT_RENDERING_CONTEXT, GLUT_USE_CURRENT_CONTEXT);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
	const auto screenWidth = GetSystemMetrics(SM_CXSCREEN);
	const auto screenHeight = GetSystemMetrics(SM_CYSCREEN);
	fgvLog().info(L"screen W/H [{},{}]\n", screenWidth, screenHeight);
	GLUTHacker::init();
}

void FreeGLUTViewRun()
{
	glutMainLoop();
}



}