#include "ColorConsole.h"
#include <algorithm>
#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN 1
#   define NOMINMAX 1
#   include <Windows.h>
#   include "WinVersionHelper.hpp"
#else
#   include "StrCharset.hpp"
#endif

namespace common::console
{


ConsoleHelper::ConsoleHelper()
{
#if defined(_WIN32)
    const auto handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE)
        throw std::runtime_error("no console handle avaliable");
    Handle = reinterpret_cast<uintptr_t>(handle);
    DWORD mode;
    if (::GetConsoleMode(handle, &mode) == 0)
        throw std::runtime_error("can't get console mode");
    if (GetWinBuildNumber() >= 10586) // since win10 1511(th2)
    {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        ::SetConsoleMode(handle, mode);
        IsVTMode = true;
    }
    else
        IsVTMode = false;
    CONSOLE_SCREEN_BUFFER_INFO csbInfo;
    ::GetConsoleScreenBufferInfo(handle, &csbInfo);
    Dummy = csbInfo.wAttributes;
#else
    Handle = reinterpret_cast<uintptr_t>(stdout);
    IsVTMode = true;
#endif
}
ConsoleHelper::~ConsoleHelper()
{
#if defined(_WIN32)
    const auto handle = (HANDLE)Handle;
    if (handle != INVALID_HANDLE_VALUE)
        CloseHandle(handle);
#endif
}

#if defined(_WIN32)
static constexpr WORD GetColorVal(const ConsoleColor color)
{
    switch (color)
    {
    case ConsoleColor::Black:           return 0;
    case ConsoleColor::Red:             return FOREGROUND_RED;
    case ConsoleColor::Green:           return FOREGROUND_GREEN;
    case ConsoleColor::Yellow:          return FOREGROUND_RED | FOREGROUND_GREEN;
    case ConsoleColor::Blue:            return FOREGROUND_BLUE;
    case ConsoleColor::Magenta:         return FOREGROUND_RED | FOREGROUND_BLUE;
    case ConsoleColor::Cyan:            return FOREGROUND_BLUE | FOREGROUND_GREEN;
    default:
    case ConsoleColor::White:           return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    case ConsoleColor::BrightBlack:     return FOREGROUND_INTENSITY | 0;
    case ConsoleColor::BrightRed:       return FOREGROUND_INTENSITY | FOREGROUND_RED;
    case ConsoleColor::BrightGreen:     return FOREGROUND_INTENSITY | FOREGROUND_GREEN;
    case ConsoleColor::BrightYellow:    return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN;
    case ConsoleColor::BrightBlue:      return FOREGROUND_INTENSITY | FOREGROUND_BLUE;
    case ConsoleColor::BrightMagenta:   return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE;
    case ConsoleColor::BrightCyan:      return FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN;
    case ConsoleColor::BrightWhite:     return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
}
static constexpr ConsoleColor GetCurrentColor(const WORD attr)
{
    ConsoleColor color = ConsoleColor::Black;
    if (attr & FOREGROUND_RED)
        color |= ConsoleColor::Red;
    if (attr & FOREGROUND_GREEN)
        color |= ConsoleColor::Green;
    if (attr & FOREGROUND_BLUE)
        color |= ConsoleColor::Blue;
    if (attr & FOREGROUND_INTENSITY)
        color |= ConsoleColor::BrightBlack;
    return color;
}
static ConsoleColor GetCurrentColor(const HANDLE hConsole)
{
    CONSOLE_SCREEN_BUFFER_INFO csbInfo;
    ::GetConsoleScreenBufferInfo(hConsole, &csbInfo);
    return GetCurrentColor(csbInfo.wAttributes);
}
forceinline static void WriteToConsole(const HANDLE hConsole, const char16_t* str, const size_t len)
{
    for (size_t offset = 0; offset < len;)
    {
        uint32_t outlen;
        const uint32_t desire = static_cast<uint32_t>(std::min<size_t>(len - offset, UINT32_MAX));
        ::WriteConsole(hConsole, str + offset, desire, (LPDWORD)&outlen, nullptr);
        if (outlen == 0) // error here, just return
            return;
        offset += outlen;
    }
}
void ConsoleHelper::Print(const ConsoleColor color, const std::u16string_view& str) const
{
    const auto hConsole = reinterpret_cast<HANDLE>(Handle);
    SetConsoleTextAttribute(hConsole, GetColorVal(color));
    WriteToConsole(hConsole, str.data(), str.size());
    SetConsoleTextAttribute(hConsole, Dummy);
}
void ConsoleHelper::Print(const std::u16string_view& str) const
{
    const auto hConsole = reinterpret_cast<HANDLE>(Handle);
    if (IsVTMode)
    {
        WriteToConsole(hConsole, str.data(), str.size());
        return;
    }
    //emulate VT mode
    const size_t len = str.size();
    if (len < 5)
    {
        WriteToConsole(hConsole, str.data(), len);
        return;
    }
    const size_t limit = len - 5;
    auto curColor = GetCurrentColor(hConsole);
    size_t offset = 0, last = 0;
    while (offset <= limit)
    {
        if (str[offset] == u'\x1b' && str[offset + 1] == u'[')
        {
            if (str[offset + 4] == u'm')
            {
                const auto high = str[offset + 2], low = str[offset + 3];
                if ((high == u'3' || high == u'9') && (low <= u'9' && low >= u'0' && low != u'8'))
                {
                    if (last < offset)
                    {
                        WriteToConsole(hConsole, str.data() + last, offset - last);
                        last = offset;
                    }
                    const auto color = low == u'9' ?
                        GetCurrentColor(Dummy) :
                        static_cast<ConsoleColor>(low - u'0') | (high == u'9' ? ConsoleColor::BrightBlack : ConsoleColor::Black);
                    if (color != curColor)
                    {
                        SetConsoleTextAttribute(hConsole, GetColorVal(color));
                        curColor = color;
                    }
                    last += 5;
                }
                offset += 5;
            }
            else
                offset += 2;
        }
        else
            offset += 1;
    }
    if (last < offset)
        WriteToConsole(hConsole, str.data() + last, len - last);
}
#else
static constexpr const char(&GetColorCharStr(const ConsoleColor color))[13]
{
    switch (color)
    {
    case ConsoleColor::Black:           return "\x1b[30m%s\x1b[39m";
    case ConsoleColor::Red:             return "\x1b[31m%s\x1b[39m";
    case ConsoleColor::Green:           return "\x1b[32m%s\x1b[39m";
    case ConsoleColor::Yellow:          return "\x1b[33m%s\x1b[39m";
    case ConsoleColor::Blue:            return "\x1b[34m%s\x1b[39m";
    case ConsoleColor::Magenta:         return "\x1b[35m%s\x1b[39m";
    case ConsoleColor::Cyan:            return "\x1b[36m%s\x1b[39m";
    default:
    case ConsoleColor::White:           return "\x1b[37m%s\x1b[39m";
    case ConsoleColor::BrightBlack:     return "\x1b[90m%s\x1b[39m";
    case ConsoleColor::BrightRed:       return "\x1b[91m%s\x1b[39m";
    case ConsoleColor::BrightGreen:     return "\x1b[92m%s\x1b[39m";
    case ConsoleColor::BrightYellow:    return "\x1b[93m%s\x1b[39m";
    case ConsoleColor::BrightBlue:      return "\x1b[94m%s\x1b[39m";
    case ConsoleColor::BrightMagenta:   return "\x1b[95m%s\x1b[39m";
    case ConsoleColor::BrightCyan:      return "\x1b[96m%s\x1b[39m";
    case ConsoleColor::BrightWhite:     return "\x1b[97m%s\x1b[39m";
}
}
void ConsoleHelper::Print(const ConsoleColor color, const std::u16string_view& str) const
{
    const auto hStdout = reinterpret_cast<FILE*>(Handle);
    const auto u8str = str::to_u8string(str, str::Charset::UTF16LE);
    fprintf(hStdout, GetColorCharStr(color), u8str.c_str());
    fflush(hStdout);
}
void ConsoleHelper::Print(const std::u16string_view& str) const
{
    const auto hStdout = reinterpret_cast<FILE*>(Handle);
    const auto u8str = str::to_u8string(str, str::Charset::UTF16LE);
    fprintf(hStdout, "%s", u8str.c_str());
    fflush(hStdout);
}
#endif


}
