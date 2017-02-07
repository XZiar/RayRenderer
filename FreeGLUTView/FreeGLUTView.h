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

enum class Key : uint8_t
{
	Space = ' ', ESC = 27, Enter = 13, Delete = 127, Backspace = 8,
	F1 = 128, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
	Left, Right, Up, Down, Home, End, PageUp, PageDown, Insert,
	ERROR = 255
};
class GLUTVIEWAPI KeyEvent
{
private:
	uint8_t helperKey;
public:
	uint8_t key;
	int x, y;
	KeyEvent(const uint8_t key_, const int x_, const int y_, const bool isCtrl = false, const bool isShift = false, const bool isAlt = false)
		:key(key_), x(x_), y(y_)
	{
		helperKey = (isCtrl ? 0x1 : 0x0) + (isShift ? 0x2 : 0x0) + (isAlt ? 0x4 : 0x0);
	}
	bool isSpecialKey() const { return key > 127; }
	bool hasCtrl() const { return (helperKey & 0x1) != 0; }
	bool hasShift() const { return (helperKey & 0x2) != 0; }
	bool hasAlt() const { return (helperKey & 0x4) != 0; }
	Key SpecialKey() const { return (key > 127 ? (Key)key : Key::ERROR); }
};

enum class MouseButton : uint8_t { None = 0, Left = 1, Middle = 2, Right = 3 };
enum class MouseEventType : uint8_t { Down, Up, Moving, Over, Wheel };
class GLUTVIEWAPI MouseEvent
{
public:
	MouseEventType type;
	MouseButton btn;
	int x, y, dx, dy;
	MouseEvent(const MouseEventType type_, const MouseButton btn_, const int x_, const int y_, const int dx_, const int dy_)
		:type(type_), btn(btn_), x(x_), y(y_), dx(dx_), dy(dy_)
	{
	}
};

class GLUTVIEWAPI FreeGLUTView
{
	friend class GLUTHacker;
public:
	using FuncBasic = std::function<void(void)>;
	using FuncReshape = std::function<void(const int, const int)>;
	using FuncKeyEvent = std::function<void(KeyEvent)>;
	using FuncMouseEvent = std::function<void(MouseEvent)>;
private:
	int wdID;
	int width, height;
	int sx, sy, lx, ly;//record x/y, last x/y
	uint8_t isMovingMouse = 0;
	bool isMoved = false;
	FuncBasic funDisp = nullptr;
	FuncReshape funReshape = nullptr;
	FuncKeyEvent funKeyEvent = nullptr;
	FuncMouseEvent funMouseEvent = nullptr;
	static void usethis(FreeGLUTView& wd);
	void display();
	void reshape(const int w, const int h);
	void onKeyboard(int key, int x, int y);
	void onKeyboard(unsigned char key, int x, int y);
	void onKey(const uint8_t key, const int x, const int y);
	void onWheel(int button, int dir, int x, int y);
	void onMouse(int x, int y);
	void onMouse(int button, int state, int x, int y);
public:
	bool deshake = true;
	static void init(const int w = 1280, const int h = 720, const int x = 50, const int y = 50);
	static void run(FuncBasic onExit = nullptr);
	FreeGLUTView(FuncBasic funInit, FuncBasic funDisp_, FuncReshape funReshape_);
	~FreeGLUTView();

	void setKeyEventlback(FuncKeyEvent funKey);
	void setMouseEventlback(FuncMouseEvent funMouse);
	void setTitle(const string& title);
	void refresh();
};

}