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
	glutPostWindowRedisplay(wdID);
}

void FreeGLUTView::init(const int w, const int h, const int x, const int y)
{
	int args = 0; char **argv = nullptr;
	glutInit(&args, argv);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
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
	//static thread_local FreeGLUTInit _init;
	wdID = glutCreateWindow("");
	GLUTHacker::regist(this);
	//usethis(*this);
	funInit();
	glutDisplayFunc(GLUTHacker::getDisplay(this));
	glutReshapeFunc(GLUTHacker::getReshape(this));
}


FreeGLUTView::~FreeGLUTView()
{
	glutDestroyWindow(wdID);
	GLUTHacker::unregist(this);
}

void FreeGLUTView::setTitle(const string& title)
{
	usethis(*this);
	glutSetWindowTitle(title.c_str());
}

}