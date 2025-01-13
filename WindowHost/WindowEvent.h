#pragma once

#include "WindowHostRely.h"
#include <optional>

namespace xziar::gui::event
{


struct Distance
{
    int32_t X, Y;
    constexpr Distance() noexcept : X(0), Y(0) { }
    constexpr Distance(int32_t x, int32_t y) noexcept : X(x), Y(y) { }
};
struct Position
{
    int32_t X, Y;
    constexpr Position() noexcept : X(0), Y(0) { }
    constexpr Position(int32_t x, int32_t y) noexcept : X(x), Y(y) { }
    constexpr bool operator==(const Position other) const noexcept 
    { 
        return X == other.X && Y == other.Y;
    }
    constexpr bool operator!=(const Position other) const noexcept
    {
        return X != other.X || Y != other.Y;
    }
    constexpr Distance operator-(const Position other) const noexcept
    {
        return Distance(X - other.X, Y - other.Y);
    }
};


struct MouseEvent
{
    Position Pos;
    constexpr MouseEvent(Position pos) noexcept : Pos(pos) { }
};


struct MouseMoveEvent : public MouseEvent
{
    Distance Delta;
    constexpr MouseMoveEvent(Position lastPos, Position curPos) noexcept : 
        MouseEvent(curPos), Delta(curPos - lastPos) { }
};


struct MouseDragEvent : public MouseEvent
{
    Position BeginPos;
    Distance Delta;
    constexpr MouseDragEvent(Position beginPos, Position lastPos, Position curPos) noexcept :
        MouseEvent(curPos), BeginPos(beginPos), Delta(curPos - lastPos) { }
    constexpr Distance TotalDistance() const noexcept
    {
        return Pos - BeginPos;
    }
};


struct MouseScrollEvent : public MouseEvent
{
    float Horizontal, Vertical;
    constexpr MouseScrollEvent(Position curPos, const float dh, const float dv) noexcept :
        MouseEvent(curPos), Horizontal(dh), Vertical(dv) { }
};


struct DropFileEvent2 : public MouseEvent
{
private:
    common::StringPool<char16_t> FileNamePool;
    std::vector<common::StringPiece<char16_t>> FileNamePieces;
public:
    DropFileEvent2(Position pos, common::StringPool<char16_t>&& namepool, std::vector<common::StringPiece<char16_t>>&& names) noexcept :
        MouseEvent(pos), FileNamePool(std::move(namepool)), FileNamePieces(std::move(names)) { }
    size_t Size() const noexcept { return FileNamePieces.size(); }
    size_t size() const noexcept { return FileNamePieces.size(); }
    std::u16string_view operator[](size_t idx) const noexcept 
    { 
        return FileNamePool.GetStringView(FileNamePieces[idx]);
    }
};

struct DropFileEvent : public MouseEvent, public FileList
{
public:
    DropFileEvent(Position pos, FileList&& list) noexcept : MouseEvent(pos), FileList(std::move(list)) { }
};


enum class MouseButton : uint8_t { None = 0x0, Left = 0x1, Middle = 0x2, Right = 0x4 };
MAKE_ENUM_BITFIELD(MouseButton)

struct MouseButtonEvent : public MouseEvent
{
    MouseButton PressedButton;
    MouseButton ChangedButton;
    constexpr MouseButtonEvent(Position curPos, const MouseButton pressed, const MouseButton changed) noexcept :
        MouseEvent(curPos), PressedButton(pressed), ChangedButton(changed) { }
    constexpr bool HasPressLeftButton() const noexcept
    {
        return HAS_FIELD(PressedButton, MouseButton::Left);
    }
    constexpr bool HasPressMiddleButton() const noexcept
    {
        return HAS_FIELD(PressedButton, MouseButton::Middle);
    }
    constexpr bool HasPressRightButton() const noexcept
    {
        return HAS_FIELD(PressedButton, MouseButton::Right);
    }
};


enum class ModifierKeys : uint8_t
{
    None = 0x0,
    Ctrl = 0x1,
    Shift = 0x2,
    Alt = 0x4,
};
MAKE_ENUM_BITFIELD(ModifierKeys)

enum class CommonKeys : uint8_t
{
    None = 0, Esc = 27, Delete = 127, Backspace = 8, 
    Add = '+', Minus = '-', Multiply = '*', Divide = '/', Comma = ',', Dot = '.',
    Space = ' ', Tab = '\t', Enter = '\r',
    F1 = 128, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Left, Up, Right, Down, 
    Home, End, PageUp, PageDown, 
    Insert,
    Ctrl, Shift, Alt,
    CapsLock = 254,
    UNDEFINE = 255
};

struct CombinedKey
{
    CommonKeys Key;
    constexpr CombinedKey(const uint8_t key) noexcept : Key(static_cast<CommonKeys>(key)) { }
    constexpr CombinedKey(const CommonKeys key = CommonKeys::UNDEFINE) noexcept : Key(key) { }
    constexpr char GetASCIIKey() const noexcept
    {
        if (common::enum_cast(Key) < 0x7f)
            return static_cast<char>(Key);
        return 0;
    }
    constexpr std::optional<char> TryGetASCIIKey() const noexcept
    {
        if (common::enum_cast(Key) < 0x7f)
            return static_cast<char>(Key);
        return {};
    }
    constexpr std::optional<char> TryGetPrintable() const noexcept
    {
        const auto ascii = common::enum_cast(Key);
        if (ascii >= 0x20 && ascii <= 0x7e)
            return ascii;
        /*if (ascii >= '0' && ascii <= '9')
            return ascii;
        if (ascii >= 'A' && ascii <= 'Z')
            return ascii;
        if (ascii >= 'a' && ascii <= 'z')
            return ascii;*/
        return {};
    }
    constexpr ModifierKeys GetModifier() const noexcept
    {
        switch (Key)
        {
        case CommonKeys::Ctrl:  return ModifierKeys::Ctrl;
        case CommonKeys::Shift: return ModifierKeys::Shift;
        case CommonKeys::Alt:   return ModifierKeys::Alt;
        default:                return ModifierKeys::None;
        }
    }
};

struct KeyEvent
{
    Position Pos;
    ModifierKeys Modifiers;
    CombinedKey ChangedKey;
    constexpr KeyEvent(Position curPos, const ModifierKeys modifiers, const CombinedKey changed) noexcept :
        Pos(curPos), Modifiers(modifiers), ChangedKey(changed) { }
    constexpr bool HasCtrl() const noexcept
    {
        return HAS_FIELD(Modifiers, ModifierKeys::Ctrl);
    }
    constexpr bool HasShift() const noexcept
    {
        return HAS_FIELD(Modifiers, ModifierKeys::Shift);
    }
    constexpr bool HasAlt() const noexcept
    {
        return HAS_FIELD(Modifiers, ModifierKeys::Alt);
    }
};



}

