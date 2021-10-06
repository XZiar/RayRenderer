#pragma once
#include "SystemCommonRely.h"

namespace common
{

namespace console
{
using ConsoleColor = CommonColor;


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif


class SYSCOMMONAPI ConsoleHelper : public NonCopyable
{
private:
    uintptr_t Handle;
#if COMMON_OS_WIN
    uint16_t Dummy;
#endif
    bool IsVTMode;
public:
    ConsoleHelper();
    ~ConsoleHelper();
    void Print(const ConsoleColor color, const std::u16string_view& str) const;
    void Print(const std::u16string_view& str) const;

    static constexpr const char16_t(&GetColorStr(const ConsoleColor color) noexcept)[6]
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


#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


}

}