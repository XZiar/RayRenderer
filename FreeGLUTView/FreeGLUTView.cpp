#include "FreeGLUTView.h"
#include "freeglutRely.hpp"

namespace glutview
{

void FreeGLUTView::usethis(FreeGLUTView& wd)
{
	static thread_local int curID = -1;
	if (curID != wd.wdID)
		glutSetWindow(curID = wd.wdID);
}

void FreeGLUTView::display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (funDisp != nullptr)
		funDisp();
	glutSwapBuffers();
}

void FreeGLUTView::reshape(const int w, const int h)
{
	width = w; height = h;
	if (funReshape != nullptr)
		funReshape(w, h);
	refresh();
}

void FreeGLUTView::onKeyboard(int key, int x, int y)
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

void FreeGLUTView::onKeyboard(unsigned char key, int x, int y)
{
	onKey(key, x, y);
}

void FreeGLUTView::onKey(uint8_t key, const int x, const int y)
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
		funKeyEvent(KeyEvent(key, x, y, isCtrl, isShift, isAlt));
	}
}

void FreeGLUTView::onWheel(int button, int dir, int x, int y)
{
	if (funMouseEvent == nullptr)
		return;
	//forward if dir > 0, backward if dir < 0
	funMouseEvent(MouseEvent(MouseEventType::Wheel, MouseButton::None, x, height - y, dir, 0));
}

void FreeGLUTView::onMouse(int x, int y)
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
		funMouseEvent(MouseEvent(MouseEventType::Moving, static_cast<MouseButton>(isMovingMouse), x, height - y, dx, -dy));
	}
}

void FreeGLUTView::onMouse(int button, int state, int x, int y)
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
		funMouseEvent(MouseEvent(MouseEventType::Down, btn, x, height - y, 0, 0));
	}
	else
	{
		isMovingMouse = (uint8_t)MouseButton::None;
		funMouseEvent(MouseEvent(MouseEventType::Up, btn, x, height - y, 0, 0));
	}
}

void FreeGLUTView::init(const int w, const int h, const int x, const int y)
{
	int args = 0; char **argv = nullptr;
	glutInit(&args, argv);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(w, h);
	glutInitWindowPosition(x, y);
}

void FreeGLUTView::run(FuncBasic onExit)
{
	glutMainLoop();
	if (onExit != nullptr)
		onExit();
}

FreeGLUTView::FreeGLUTView(FuncBasic funInit, FuncBasic funDisp_, FuncReshape funReshape_) :funDisp(funDisp_), funReshape(funReshape_)
{
	wdID = glutCreateWindow("");
	GLUTHacker::regist(this);
	usethis(*this);
	funInit();
	glutDisplayFunc(GLUTHacker::getDisplay(this));
	glutReshapeFunc(GLUTHacker::getReshape(this));
	glutKeyboardFunc(GLUTHacker::getOnKey1(this));
	glutSpecialFunc(GLUTHacker::getOnkey2(this));
	glutMouseWheelFunc(GLUTHacker::getOnWheel(this));
	glutMotionFunc(GLUTHacker::getOnMouse1(this));
	glutMouseFunc(GLUTHacker::getOnMouse2(this));
}


FreeGLUTView::~FreeGLUTView()
{
	glutDestroyWindow(wdID);
	GLUTHacker::unregist(this);
}

void FreeGLUTView::setKeyEventlback(FuncKeyEvent funKey)
{
	funKeyEvent = funKey;
}

void FreeGLUTView::setMouseEventlback(FuncMouseEvent funMouse)
{
	funMouseEvent = funMouse;
}

void FreeGLUTView::setTitle(const string& title)
{
	usethis(*this);
	glutSetWindowTitle(title.c_str());
}

void FreeGLUTView::refresh()
{
	glutPostWindowRedisplay(wdID);
}

}