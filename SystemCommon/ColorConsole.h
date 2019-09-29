#pragma once
#include "SystemCommonRely.h"

namespace common
{

namespace console
{

enum class ConsoleColor : uint8_t 
{ 
    Black = 0, Red = 1, Green = 2, Yellow = 3, Blue = 4, Magenta = 5, Cyan = 6, White = 7, 
    BrightBlack = 8, BrightRed = 9, BrightGreen = 10, BrightYellow = 11, BrightBlue = 12, BrightMagenta = 13, BrightCyan = 14, BrightWhite = 15
};
MAKE_ENUM_BITFIELD(ConsoleColor)


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif


class SYSCOMMONAPI ConsoleHelper : public NonCopyable
{
private:
    uintptr_t Handle;
#if defined(_WIN32)
    uint16_t Dummy;
#endif
    bool IsVTMode;
public:
    ConsoleHelper();
    ~ConsoleHelper();
    void Print(const ConsoleColor color, const std::u16string_view& str) const;
    void Print(const std::u16string_view& str) const;

    static constexpr const char16_t(&GetColorStr(const ConsoleColor color))[6]
    {
        switch (color)
        {
        case ConsoleColor::Black:           return u"\x1b[30m";
        case ConsoleColor::Red:             return u"\x1b[31m";
        case ConsoleColor::Green:           return u"\x1b[32m";
        case ConsoleColor::Yellow:          return u"\x1b[33m";
        case ConsoleColor::Blue:            return u"\x1b[34m";
        case ConsoleColor::Magenta:         return u"\x1b[35m";
        case ConsoleColor::Cyan:            return u"\x1b[36m";
        default:
        case ConsoleColor::White:           return u"\x1b[37m";
        case ConsoleColor::BrightBlack:     return u"\x1b[90m";
        case ConsoleColor::BrightRed:       return u"\x1b[91m";
        case ConsoleColor::BrightGreen:     return u"\x1b[92m";
        case ConsoleColor::BrightYellow:    return u"\x1b[93m";
        case ConsoleColor::BrightBlue:      return u"\x1b[94m";
        case ConsoleColor::BrightMagenta:   return u"\x1b[95m";
        case ConsoleColor::BrightCyan:      return u"\x1b[96m";
        case ConsoleColor::BrightWhite:     return u"\x1b[97m";
        }
    }
};


#if COMPILER_MSVC
#   pragma warning(pop)
#endif


}

}