#pragma once

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

public enum class Key
{
	Space = ' ', ESC = 27, Enter = 13, Delete = 127, Backspace = 8,
	F1 = 128, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
	Left, Up, Right, Down, Home, End, PageUp, PageDown, Insert,
	NONE = 255
};
public ref class KeyBoardEventArgs : public EventArgs
{
private:
	uint8_t helperKey;
public:
	wchar_t key;
	int x, y;
	KeyBoardEventArgs(const wchar_t key_, const int x_, const int y_,
		const bool isCtrl, const bool isShift, const bool isAlt) :key(key_), x(x_), y(y_)
	{
		helperKey = (isCtrl ? 0x1 : 0x0) + (isShift ? 0x2 : 0x0) + (isAlt ? 0x4 : 0x0);
	}
	bool isSpecialKey() { return key > 127; }
	bool hasCtrl() { return (helperKey & 0x1) != 0; }
	bool hasShift() { return (helperKey & 0x2) != 0; }
	bool hasAlt() { return (helperKey & 0x4) != 0; }
	Key SpecialKey() { return (key > 127 ? (Key)key : Key::NONE); }
};

public enum class MouseButton : uint8_t { None = 0, Left = 1, Middle = 2, Right = 4 };
public enum class MouseEventType : uint8_t { Down, Up, Moving, Over, Wheel };
public ref class MouseEventExArgs : public EventArgs
{
public:
	MouseEventType type;
	MouseButton btn;
	int x, y, dx, dy;
	MouseEventExArgs(const MouseEventType type_, const MouseButton btn_, const int x_, const int y_, const int dx_, const int dy_)
		:type(type_), btn(btn_), x(x_), y(y_), dx(dx_), dy(dy_)
	{
	}
};

}