#pragma once

#ifdef GLUTVIEW_EXPORT
#   define GLUTVIEWAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define GLUTVIEWAPI _declspec(dllimport)
#endif

#include "../3DBasic/3dElement.hpp"
#include "../common/Wrapper.hpp"
#include "../common/TimeUtil.h"
#include <string>
#include <functional>


namespace glutview
{

using namespace common;
using std::string;
using std::wstring;
using namespace b3d;

enum class Key : uint8_t
{
	Space = ' ', ESC = 27, Enter = 13, Delete = 127, Backspace = 8,
	F1 = 128, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
	Left, Up, Right, Down, Home, End, PageUp, PageDown, Insert,
	UNDEFINE = 255
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
	Key SpecialKey() const { return (key > 127 ? (Key)key : Key::UNDEFINE); }
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


namespace detail
{
class _FreeGLUTView;
}
using FreeGLUTView = Wrapper<detail::_FreeGLUTView>;


using FuncBasic = std::function<void(FreeGLUTView)>;
using FuncReshape = std::function<void(FreeGLUTView, const int, const int)>;
using FuncKeyEvent = std::function<void(FreeGLUTView, KeyEvent)>;
using FuncMouseEvent = std::function<void(FreeGLUTView, MouseEvent)>;
//-param Window self
//-param elapse time(ms)
//-return whether continue this timer
using FuncTimer = std::function<bool(FreeGLUTView, uint32_t)>;
using FuncDropFile = std::function<void(FreeGLUTView, wstring filePath)>;


class GLUTHacker;
namespace detail
{


class GLUTVIEWAPI _FreeGLUTView : public NonCopyable, public NonMovable, public std::enable_shared_from_this<_FreeGLUTView>
{
	friend class GLUTHacker;
public:

private:
	SimpleTimer uitimer;
	int64_t timerus = INT64_MIN;
	FuncTimer funTimer = nullptr;
	int wdID;
	int width, height;
	int sx, sy, lx, ly;//record x/y, last x/y
	uint8_t isMovingMouse = 0;
	bool isMoved = false;

	FreeGLUTView getSelf();
	void usethis();
	void display();
	void reshape(const int w, const int h);
	void onKeyboard(int key, int x, int y);
	void onKeyboard(unsigned char key, int x, int y);
	void onKey(const uint8_t key, const int x, const int y);
	void onWheel(int button, int dir, int x, int y);
	void onMouse(int x, int y);
	void onMouse(int button, int state, int x, int y);
	void onTimer();
	void onDropFile(const wstring& fname);
public:
	bool deshake = true;
	FuncBasic funDisp = nullptr;
	FuncReshape funReshape = nullptr;
	FuncKeyEvent funKeyEvent = nullptr;
	FuncMouseEvent funMouseEvent = nullptr;
	FuncDropFile funDropFile = nullptr;
	_FreeGLUTView(const int w = 1280, const int h = 720);
	~_FreeGLUTView();
	void setTimerCallback(FuncTimer funTime, const uint16_t ms);
	void setTitle(const string& title);
	void refresh();
	void invoke(std::function<bool(void)> task);
};


}


GLUTVIEWAPI void FreeGLUTViewInit();
GLUTVIEWAPI void FreeGLUTViewRun();


}