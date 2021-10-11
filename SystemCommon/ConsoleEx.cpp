#include "SystemCommonPch.h"
#include "ConsoleEx.h"
#include <iostream>
#if COMMON_OS_WIN
#elif COMMON_OS_UNIX
#   include <readline/readline.h>
#   include <readline/history.h>
#endif


namespace common::console
{
using namespace std::string_view_literals;

ColorConsole::ColorConsole() 
{ }
ColorConsole::~ColorConsole() 
{ }


#if COMMON_OS_WIN
class NormalConsole final : public ColorConsole
{
    void Print(const CommonColor, const std::u16string_view& str) const final
    {
        Print(str);
    }
    void Print(const std::u16string_view& str) const final
    {
        _lock_file(stdout);
        if (fwide(stdout, 0) > 0)
        {
            fwrite(str.data(), sizeof(char16_t), str.size(), stdout);
        }
        else
        {
            _unlock_file(stdout);
            const auto size = WideCharToMultiByte(CP_ACP, 0, reinterpret_cast<const wchar_t*>(str.data()), 
                gsl::narrow_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
            std::string newStr;
            if (size == 0)
                newStr = common::str::to_string(str);
            else
            {
                newStr.resize(size);
                WideCharToMultiByte(CP_ACP, 0, reinterpret_cast<const wchar_t*>(str.data()),
                    gsl::narrow_cast<int>(str.size()), newStr.data(), size, nullptr, nullptr);
            }
            _lock_file(stdout);
            fwrite(newStr.data(), sizeof(char), newStr.size(), stdout);
        }
        fflush(stdout);
        _unlock_file(stdout);
    }
};
class NativeConsole : public ColorConsole
{
protected:
    HANDLE OutHandle;
    WORD DefaultAttr = 0;
    NativeConsole(HANDLE handle) : OutHandle(handle)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbInfo;
        ::GetConsoleScreenBufferInfo(handle, &csbInfo);
        DefaultAttr = csbInfo.wAttributes;
    }
    forceinline void WriteToConsole(const char16_t* str, const size_t len) const noexcept
    {
        for (size_t offset = 0; offset < len;)
        {
            uint32_t outlen;
            const uint32_t desire = static_cast<uint32_t>(std::min<size_t>(len - offset, UINT32_MAX));
            ::WriteConsoleW(OutHandle, str + offset, desire, (LPDWORD)&outlen, nullptr);
            if (outlen == 0) // error here, just return
                return;
            offset += outlen;
        }
    }
    void Print(const CommonColor color, const std::u16string_view& str) const final
    {
        _lock_file(stdout);
        SetConsoleTextAttribute(OutHandle, GetColorVal(color));
        WriteToConsole(str.data(), str.size());
        SetConsoleTextAttribute(OutHandle, DefaultAttr);
        _unlock_file(stdout);
    }
    static constexpr WORD GetColorVal(const CommonColor color) noexcept
    {
        switch (color)
        {
        case CommonColor::Black:           return 0;
        case CommonColor::Red:             return FOREGROUND_RED;
        case CommonColor::Green:           return FOREGROUND_GREEN;
        case CommonColor::Yellow:          return FOREGROUND_RED | FOREGROUND_GREEN;
        case CommonColor::Blue:            return FOREGROUND_BLUE;
        case CommonColor::Magenta:         return FOREGROUND_RED | FOREGROUND_BLUE;
        case CommonColor::Cyan:            return FOREGROUND_BLUE | FOREGROUND_GREEN;
        default:
        case CommonColor::White:           return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        case CommonColor::BrightBlack:     return FOREGROUND_INTENSITY | 0;
        case CommonColor::BrightRed:       return FOREGROUND_INTENSITY | FOREGROUND_RED;
        case CommonColor::BrightGreen:     return FOREGROUND_INTENSITY | FOREGROUND_GREEN;
        case CommonColor::BrightYellow:    return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN;
        case CommonColor::BrightBlue:      return FOREGROUND_INTENSITY | FOREGROUND_BLUE;
        case CommonColor::BrightMagenta:   return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE;
        case CommonColor::BrightCyan:      return FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN;
        case CommonColor::BrightWhite:     return FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }
    }
public:
    ~NativeConsole() override
    { }
};
class ClassicConsole final : public NativeConsole
{
    static constexpr CommonColor GetCurrentColor(const WORD attr) noexcept
    {
        CommonColor color = CommonColor::Black;
        if (attr & FOREGROUND_RED)
            color |= CommonColor::Red;
        if (attr & FOREGROUND_GREEN)
            color |= CommonColor::Green;
        if (attr & FOREGROUND_BLUE)
            color |= CommonColor::Blue;
        if (attr & FOREGROUND_INTENSITY)
            color |= CommonColor::BrightBlack;
        return color;
    }
    static CommonColor GetCurrentColor(const HANDLE hConsole) noexcept
    {
        CONSOLE_SCREEN_BUFFER_INFO csbInfo;
        ::GetConsoleScreenBufferInfo(hConsole, &csbInfo);
        return GetCurrentColor(csbInfo.wAttributes);
    }
    
    void Print(const std::u16string_view& str) const final
    {
        const size_t len = str.size();
        if (len < 5)
        {
            WriteToConsole(str.data(), len);
            return;
        }
        const size_t limit = len - 5;
        _lock_file(stdout);
        auto curColor = GetCurrentColor(OutHandle);
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
                            WriteToConsole(str.data() + last, offset - last);
                            last = offset;
                        }
                        const auto color = low == u'9' ? GetCurrentColor(DefaultAttr) :
                            static_cast<CommonColor>(low - u'0') | (high == u'9' ? CommonColor::BrightBlack : CommonColor::Black);
                        if (color != curColor)
                        {
                            SetConsoleTextAttribute(OutHandle, GetColorVal(color));
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
        if (last < len)
            WriteToConsole(str.data() + last, len - last);
        _unlock_file(stdout);
    }
public:
    ClassicConsole(HANDLE handle) : NativeConsole(handle)
    { }
};
class VTConsole final : public NativeConsole
{
    void Print(const std::u16string_view& str) const final
    {
        _lock_file(stdout);
        WriteToConsole(str.data(), str.size());
        _unlock_file(stdout);
    }
public:
    VTConsole(HANDLE handle) : NativeConsole(handle)
    { }
};

const ColorConsole& ColorConsole::Get() noexcept
{
    static auto console = []() -> std::unique_ptr<ColorConsole>
    {
        // no need to close
        // see https://docs.microsoft.com/en-us/windows/console/getstdhandle#handle-disposal
        const auto handle = ::GetStdHandle(STD_OUTPUT_HANDLE); 
        if (handle != INVALID_HANDLE_VALUE)
        {
            DWORD mode;
            if (::GetConsoleMode(handle, &mode) != 0)
            {
                if (GetWinBuildNumber() >= 10586) // since win10 1511(th2)
                {
                    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                    ::SetConsoleMode(handle, mode);
                    return std::make_unique<VTConsole>(handle);
                }
                return std::make_unique<ClassicConsole>(handle);
            }
        }
        return std::make_unique<NormalConsole>();
    }();
    return *console;
}

#elif COMMON_OS_UNIX

class NormalConsole : public ColorConsole
{
    void Print(const CommonColor, const std::u16string_view& str) const
    {
        Print(str);
    }
    void Print(const std::u16string_view& str) const final
    {
        const auto newStr = str::to_string(str, common::str::Encoding::UTF8);
        flockfile(stdout);
        fwrite(newStr.data(), sizeof(char), newStr.size(), stdout);
        fflush(stdout);
        funlockfile(stdout);
    }
};

class DirectConsole final : public NormalConsole
{
    static constexpr std::string_view GetColorPfx(const CommonColor color) noexcept
    {
        switch (color)
        {
        case CommonColor::Black:           return "\x1b[30m"sv;
        case CommonColor::Red:             return "\x1b[31m"sv;
        case CommonColor::Green:           return "\x1b[32m"sv;
        case CommonColor::Yellow:          return "\x1b[33m"sv;
        case CommonColor::Blue:            return "\x1b[34m"sv;
        case CommonColor::Magenta:         return "\x1b[35m"sv;
        case CommonColor::Cyan:            return "\x1b[36m"sv;
        default:
        case CommonColor::White:           return "\x1b[37m"sv;
        case CommonColor::BrightBlack:     return "\x1b[90m"sv;
        case CommonColor::BrightRed:       return "\x1b[91m"sv;
        case CommonColor::BrightGreen:     return "\x1b[92m"sv;
        case CommonColor::BrightYellow:    return "\x1b[93m"sv;
        case CommonColor::BrightBlue:      return "\x1b[94m"sv;
        case CommonColor::BrightMagenta:   return "\x1b[95m"sv;
        case CommonColor::BrightCyan:      return "\x1b[96m"sv;
        case CommonColor::BrightWhite:     return "\x1b[97m"sv;
        }
    }
    void Print(const CommonColor color, const std::u16string_view& str) const final
    {
        const auto newStr   = str::to_string(str, common::str::Encoding::UTF8);
        const auto pfx      = GetColorPfx(color);
        constexpr auto sfx  = "\x1b[39m"sv;
        flockfile(stdout);
        fwrite(pfx.data(),    sizeof(char), pfx.size(),    stdout);
        fwrite(newStr.data(), sizeof(char), newStr.size(), stdout);
        fwrite(sfx.data(),    sizeof(char), sfx.size(),    stdout);
        fflush(stdout);
        funlockfile(stdout);
    }
};

const ColorConsole& ColorConsole::Get() noexcept
{
    static auto console = []() -> std::unique_ptr<ColorConsole>
    {
        const auto fd = fileno(stdout);
        const auto isAtty = isatty(fd);
        if (isAtty)
        {
            const std::string str = getenv("TERM");
            constexpr std::string_view ColorTerms[] = 
            {
                "xterm", "xterm-color", "xterm-256color", "screen", "screen-256color", "tmux", "tmux-256color", "rxvt-unicode", "rxvt-unicode-256color", "linux", "cygwin"
            };
            for (const auto t : ColorTerms)
            {
                if (str == t)
                {
                    return std::make_unique<DirectConsole>();
                }
            }
        }
        return std::make_unique<NormalConsole>();
    }();
    return *console;
}

#endif

}


char common::console::ConsoleEx::ReadCharImmediate(bool ShouldEcho) noexcept
{
#if COMMON_OS_WIN
    const auto ret = ShouldEcho ? _getche() : _getch();
    return static_cast<char>(ret);
#else
    static thread_local termios info;
    tcgetattr(0, &info);
    const auto c_lflag = info.c_lflag;
    info.c_lflag &= ~ICANON;
    if (ShouldEcho)
        info.c_lflag |=  ECHO;
    else
        info.c_lflag &= ~ECHO;
    const auto c_cc_VMIN  = info.c_cc[VMIN ];
    const auto c_cc_VTIME = info.c_cc[VTIME];
    info.c_cc[VMIN ] = 1;
    info.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &info);

    char buf = 0;
    read(0, &buf, 1);

    info.c_lflag     = c_lflag;
    info.c_cc[VMIN ] = c_cc_VMIN;
    info.c_cc[VTIME] = c_cc_VTIME;
    tcsetattr(0, TCSADRAIN, &info);
    return buf;
#endif
}

std::pair<uint32_t, uint32_t> common::console::ConsoleEx::GetConsoleSize() noexcept
{
#if COMMON_OS_WIN
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (const auto handle = GetStdHandle(STD_OUTPUT_HANDLE); handle == INVALID_HANDLE_VALUE)
        return { 0,0 };
    else if (!GetConsoleScreenBufferInfo(handle, &csbi))
        return { 0,0 };
    const auto wdRow = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    const auto wdCol = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    const auto wdRow = ws.ws_col;
    const auto wdCol = ws.ws_row;
#endif
    return std::pair<uint32_t, uint32_t>(wdRow, wdCol);
}


bool common::console::ConsoleEx::ClearConsole() noexcept
{
#if COMMON_OS_WIN
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (const auto handle = GetStdHandle(STD_OUTPUT_HANDLE); handle == INVALID_HANDLE_VALUE)
        return false;
    else if (!GetConsoleScreenBufferInfo(handle, &csbi))
        return false;
    else
    {
        const DWORD wdSize = csbi.dwSize.X * csbi.dwSize.Y;
        COORD origin = { 0,0 };
        DWORD writtenChars;
        if (!FillConsoleOutputCharacterW(handle, L' ', wdSize, origin, &writtenChars))
            return false;
        if (!FillConsoleOutputAttribute(handle, csbi.wAttributes, wdSize, origin, &writtenChars))
            return false;
        return SetConsoleCursorPosition(handle, origin) != 0;
    }
#else
    printf("\033c");
    printf("\x1b[3J");
    return true;
#endif
}


std::string common::console::ConsoleEx::ReadLine(const std::string& prompt)
{
    std::string result;
#if COMMON_OS_WIN
    if (!prompt.empty())
        std::cout << prompt;
    std::getline(std::cin, result);
#else
    const auto line = readline(prompt.c_str());
    if (line)
    {
        if (*line)
        {
            add_history(line);
            result.assign(line);
        }
        free(line);
    }
    else
    {
        std::cin.setstate(std::ios_base::failbit | std::ios_base::eofbit);
    }
#endif
    return result;
}


