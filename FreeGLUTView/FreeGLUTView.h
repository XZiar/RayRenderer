#pragma once

#ifdef GLUTVIEW_EXPORT
#   define GLUTVIEWAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define GLUTVIEWAPI _declspec(dllimport)
#endif

#include "../3DBasic/3dElement.hpp"
#include "../common/CommonBase.hpp"
#include <string>
#include <functional>

namespace glutview
{

using namespace common;
using std::string;
using namespace b3d;

enum class Key : uint8_t
{
	Space = ' ', ESC = 27, Enter = 13, Delete = 127, Backspace = 8,
	F1 = 128, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
	Left, Up, Right, Down, Home, End, PageUp, PageDown, Insert,
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

class GLUTVIEWAPI _FreeGLUTView : public NonCopyable, public NonMovable
{
	friend class GLUTHacker;
public:
	using FuncBasic = std::function<void(void)>;
	using FuncReshape = std::function<void(const int, const int)>;
	using FuncKeyEvent = std::function<void(KeyEvent)>;
	using FuncMouseEvent = std::function<void(MouseEvent)>;
	using FuncTimer = std::function<bool()>;
private:
	uint8_t instanceID = UINT8_MAX;
	int wdID;
	int width, height;
	int sx, sy, lx, ly;//record x/y, last x/y
	uint8_t isMovingMouse = 0;
	bool isMoved = false;
	FuncBasic funDisp = nullptr;
	FuncReshape funReshape = nullptr;
	FuncKeyEvent funKeyEvent = nullptr;
	FuncMouseEvent funMouseEvent = nullptr;
	FuncTimer funTimer = nullptr;
	static void usethis(_FreeGLUTView& wd);
	void display();
	void reshape(const int w, const int h);
	void onKeyboard(int key, int x, int y);
	void onKeyboard(unsigned char key, int x, int y);
	void onKey(const uint8_t key, const int x, const int y);
	void onWheel(int button, int dir, int x, int y);
	void onMouse(int x, int y);
	void onMouse(int button, int state, int x, int y);
	void onTimer(const uint16_t lastms);
public:
	bool deshake = true;
	_FreeGLUTView(FuncBasic funInit, FuncBasic funDisp_, FuncReshape funReshape_);
	~_FreeGLUTView();
	uint8_t getWindowID();
	void setKeyEventCallback(FuncKeyEvent funKey);
	void setMouseEventCallback(FuncMouseEvent funMouse);
	void setTimerCallback(FuncTimer funTime, const uint16_t ms);
	void setTitle(const string& title);
	void refresh();
};
using FreeGLUTView = Wrapper<_FreeGLUTView, true>;

GLUTVIEWAPI void FreeGLUTViewInit(const int w = 1280, const int h = 720, const int x = 50, const int y = 50);
GLUTVIEWAPI void FreeGLUTViewRun();


}