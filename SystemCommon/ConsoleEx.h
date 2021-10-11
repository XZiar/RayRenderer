#pragma once
#include "SystemCommonRely.h"

namespace common
{
namespace console
{

class SYSCOMMONAPI ConsoleEx
{
public:
    static char ReadCharImmediate(bool ShouldEcho) noexcept;
    [[nodiscard]] static std::pair<uint32_t, uint32_t> GetConsoleSize() noexcept;
    static bool ClearConsole() noexcept;
    static std::string ReadLine(const std::string& prompt = {});
};


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif


class ColorConsole
{
protected:
    ColorConsole();
public:
    COMMON_NO_COPY(ColorConsole)
    COMMON_NO_MOVE(ColorConsole)
    virtual ~ColorConsole();
    virtual void Print(const CommonColor color, const std::u16string_view& str) const = 0;
    virtual void Print(const std::u16string_view& str) const = 0;
    SYSCOMMONAPI static const ColorConsole& Get() noexcept;
    static constexpr const char16_t(&GetColorStr(const CommonColor color) noexcept)[6]
    {
        switch (color)
        {
        case CommonColor::Black:           return u"\x1b[30m";
        case CommonColor::Red:             return u"\x1b[31m";
        case CommonColor::Green:           return u"\x1b[32m";
        case CommonColor::Yellow:          return u"\x1b[33m";
        case CommonColor::Blue:            return u"\x1b[34m";
        case CommonColor::Magenta:         return u"\x1b[35m";
        case CommonColor::Cyan:            return u"\x1b[36m";
        default:
        case CommonColor::White:           return u"\x1b[37m";
        case CommonColor::BrightBlack:     return u"\x1b[90m";
        case CommonColor::BrightRed:       return u"\x1b[91m";
        case CommonColor::BrightGreen:     return u"\x1b[92m";
        case CommonColor::BrightYellow:    return u"\x1b[93m";
        case CommonColor::BrightBlue:      return u"\x1b[94m";
        case CommonColor::BrightMagenta:   return u"\x1b[95m";
        case CommonColor::BrightCyan:      return u"\x1b[96m";
        case CommonColor::BrightWhite:     return u"\x1b[97m";
        }
    }
};


#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


}
}
