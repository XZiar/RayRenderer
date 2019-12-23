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
    uint32_t X, Y;
    constexpr Position() noexcept : X(0), Y(0) { }
    constexpr Position(uint32_t x, uint32_t y) noexcept : X(x), Y(y) { }
    constexpr Distance operator-(const Position other) const noexcept
    {
        const auto dx = static_cast<int32_t>(static_cast<int64_t>(X) - static_cast<int64_t>(other.X));
        const auto dy = static_cast<int32_t>(static_cast<int64_t>(Y) - static_cast<int64_t>(other.Y));
        return Distance(dx, dy);
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


struct MouseWheelEvent : public MouseEvent
{
    float Delta;
    constexpr MouseWheelEvent(Position curPos, const float delta) noexcept :
        MouseEvent(curPos), Delta(delta) { }
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
    LeftCtrl = 0x1, RightCtrl = 0x2,
    LeftShift = 0x4, RightShift = 0x8,
    LeftAlt = 0x10, RightAlt = 0x20,
};
MAKE_ENUM_BITFIELD(ModifierKeys)

enum class CommonKeys : uint8_t
{
    None = 0, Space = ' ', ESC = 27, Enter = 13, Delete = 127, Backspace = 8,
    F1 = 128, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Left, Up, Right, Down, Home, End, PageUp, PageDown, Insert,
    LeftCtrl, RightCtrl, LeftShift, RightShift, LeftAlt, RightAlt,
    UNDEFINE = 255
};

struct CombinedKey
{
    CommonKeys Key;
    constexpr CombinedKey(const uint8_t key) noexcept : Key(static_cast<CommonKeys>(key)) { }
    constexpr CombinedKey(const CommonKeys key) noexcept : Key(key) { }
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
    constexpr ModifierKeys GetModifier() const noexcept
    {
        switch (Key)
        {
        case CommonKeys::LeftCtrl:      return ModifierKeys::LeftCtrl;
        case CommonKeys::RightCtrl:     return ModifierKeys::RightCtrl;
        case CommonKeys::LeftShift:     return ModifierKeys::LeftShift;
        case CommonKeys::RightShift:    return ModifierKeys::RightShift;
        case CommonKeys::LeftAlt:       return ModifierKeys::LeftAlt;
        case CommonKeys::RightAlt:      return ModifierKeys::RightAlt;
        default:                        return ModifierKeys::None;
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
        return HAS_FIELD(Modifiers, ModifierKeys::LeftCtrl) || HAS_FIELD(Modifiers, ModifierKeys::RightCtrl);
    }
    constexpr bool HasShift() const noexcept
    {
        return HAS_FIELD(Modifiers, ModifierKeys::LeftShift) || HAS_FIELD(Modifiers, ModifierKeys::RightShift);
    }
    constexpr bool HasAlt() const noexcept
    {
        return HAS_FIELD(Modifiers, ModifierKeys::LeftAlt) || HAS_FIELD(Modifiers, ModifierKeys::RightAlt);
    }
};



}

