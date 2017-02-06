#pragma once

#include "../3DBasic/3dElement.hpp"
#include <string>
#include <functional>

#ifdef GLUTVIEW_EXPORT
#   define GLUTVIEWAPI _declspec(dllexport)
#else
#   define GLUTVIEWAPI _declspec(dllimport)
#endif

namespace glutview
{

using std::string;
using namespace b3d;


class GLUTVIEWAPI FreeGLUTView
{
	friend class GLUTHacker;
public:
	using FuncBasic = std::function<void(void)>;
	using FuncReshape = std::function<void(const int, const int)>;
	
private:
	int wdID;
	int width, height;
	FuncBasic funDisp = nullptr;
	FuncReshape funReshape = nullptr;
	static void usethis(FreeGLUTView& wd);
	void display();
	void reshape(const int w, const int h);
public:
	static void init(const int w = 1280, const int h = 720, const int x = 50, const int y = 50);
	static void run(FuncBasic onExit = nullptr);
	FreeGLUTView(FuncBasic funInit, FuncBasic funDisp_, FuncReshape funReshape_);
	~FreeGLUTView();

	void setTitle(const string& title);
};

}