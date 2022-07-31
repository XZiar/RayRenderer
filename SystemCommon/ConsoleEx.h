#pragma once
#include "SystemCommonRely.h"

namespace common
{
namespace console
{

class ConsoleEx
{
private:
    struct ConsolePrinter
    {
        const ConsoleEx& Console;
        ScreenColor FgColor, BgColor;
        ConsolePrinter(const ConsoleEx& console) : Console(console), FgColor(false), BgColor(true)
        {
            FgColor.IsUnchanged = true;
            BgColor.IsUnchanged = true;
            Console.LockOutput();
        }
        ~ConsolePrinter() 
        {
            Console.UnlockOutput();
        }
        void SetColor(ScreenColor color) noexcept
        {
            auto& target = color.IsBackground ? BgColor : FgColor;
            if (target == color)
                return;
            target = color;
            target.IsUnchanged = false;
        }
        void Print(std::u16string_view str)
        {
            Console.PrintSegment(FgColor, BgColor, str);
            FgColor.IsUnchanged = true;
            BgColor.IsUnchanged = true;
        }
        void Print(span<const std::byte> raw)
        {
            Console.PrintSegment(FgColor, BgColor, raw);
            FgColor.IsUnchanged = true;
            BgColor.IsUnchanged = true;
        }
    };
protected:
    ConsoleEx();
    virtual void LockOutput() const = 0;
    virtual void UnlockOutput() const = 0;
    virtual void PrintSegment(const ScreenColor fgColor, const ScreenColor bgColor, span<const std::byte> raw) const = 0;
    virtual void PrintSegment(const ScreenColor fgColor, const ScreenColor bgColor, std::u16string_view str) const = 0;
public:
    COMMON_NO_COPY(ConsoleEx)
    COMMON_NO_MOVE(ConsoleEx)
    virtual ~ConsoleEx();
    virtual void Print(const CommonColor color, std::u16string_view str) const = 0;
    virtual void Print(std::u16string_view str) const = 0;
    [[nodiscard]] ConsolePrinter PrintSegments() const { return *this; }
    [[nodiscard]] virtual std::optional<str::Encoding> PreferEncoding() const noexcept = 0;
    [[nodiscard]] virtual bool SupportColor() const noexcept = 0;
    [[nodiscard]] virtual std::pair<uint32_t, uint32_t> GetConsoleSize() const noexcept = 0;
    virtual bool ClearConsole() const noexcept = 0;

    SYSCOMMONAPI static char ReadCharImmediate(bool ShouldEcho) noexcept;
    SYSCOMMONAPI static std::string ReadLine(const std::string& prompt = {});
    SYSCOMMONAPI static const ConsoleEx& Get() noexcept;
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


}
}
