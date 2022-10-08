#include "SystemCommonPch.h"
#include "ConsoleEx.h"

#include <boost/container/small_vector.hpp>

#include <iostream>
#if COMMON_OS_WIN
#elif COMMON_OS_UNIX
# if __has_include(<readline/readline.h>)
#   include <readline/readline.h>
#   include <readline/history.h>
# else // hack for iOS with Procursus, whose libreadline-dev lacks header
extern "C" 
{
extern char *readline(const char *);
extern void add_history(const char *);
}
# endif
#endif


namespace common::console
{
using namespace std::string_view_literals;

ConsoleEx::ConsoleEx()
{ }
ConsoleEx::~ConsoleEx()
{ }

void ConsoleEx::ConsolePrinter::Print(std::u16string_view str, span<const ColorSeg> seg)
{
    uint32_t prevIdx = 0;
    for (const auto& [offset, color] : seg)
    {
        if (offset > prevIdx)
            Print(str.substr(prevIdx, offset - prevIdx));
        SetColor(color);
        prevIdx = offset;
    }
    if (prevIdx < str.length())
        Print(str.substr(prevIdx));
    SetColor({ false });
    SetColor({ true });
    Print(span<const std::byte>{});
}


template<typename Char>
struct EscapeColorFiller
{
    std::array<Char, 19 * 2 + 1> Buffer; // 24bit(\x1b[38;2;255;255;255m)
    uint32_t Count = 0;
    void Put256(uint8_t val) noexcept
    {
        if (val < 10)
            Buffer[Count++] = static_cast<Char>('0' + val);
        else if (val < 100)
        {
            Buffer[Count++] = static_cast<Char>('0' + (val / 10));
            Buffer[Count++] = static_cast<Char>('0' + (val % 10));
        }
        else
        {
            Buffer[Count++] = static_cast<Char>('0' + (val / 100));
            Buffer[Count++] = static_cast<Char>('0' + ((val % 100) / 10));
            Buffer[Count++] = static_cast<Char>('0' + (val % 10));
        }
    }
    void PutColor(ScreenColor color) noexcept
    {
        switch (color.Type)
        {
        case ScreenColor::ColorType::Common:
        {
            Buffer[Count++] = '\x1b';
            Buffer[Count++] = '[';
            const bool isBright = color.Value[0] & 0x8;
            if (color.IsBackground)
            {
                if (isBright)
                {
                    Buffer[Count++] = '1';
                    Buffer[Count++] = '0';
                }
                else
                    Buffer[Count++] = '4';
            }
            else
            {
                Buffer[Count++] = isBright ? '9' : '3';
            }
            Buffer[Count++] = static_cast<Char>('0' + (color.Value[0] & 0x7));
            Buffer[Count++] = 'm';
        } break;
        case ScreenColor::ColorType::Bit8:
        {
            Buffer[Count++] = '\x1b';
            Buffer[Count++] = '[';
            Buffer[Count++] = color.IsBackground ? '4' : '3';
            Buffer[Count++] = '8';
            Buffer[Count++] = ';';
            Buffer[Count++] = '5';
            Buffer[Count++] = ';';
            Put256(color.Value[0]);
            Buffer[Count++] = 'm';
        } break;
        case ScreenColor::ColorType::Bit24:
        {
            Buffer[Count++] = '\x1b';
            Buffer[Count++] = '[';
            Buffer[Count++] = color.IsBackground ? '4' : '3';
            Buffer[Count++] = '8';
            Buffer[Count++] = ';';
            Buffer[Count++] = '2';
            Buffer[Count++] = ';';
            Put256(color.Value[0]);
            Buffer[Count++] = ';';
            Put256(color.Value[1]);
            Buffer[Count++] = ';';
            Put256(color.Value[2]);
            Buffer[Count++] = 'm';
        } break;
        case ScreenColor::ColorType::Default:
        default:
        {
            Buffer[Count++] = '\x1b';
            Buffer[Count++] = '[';
            Buffer[Count++] = color.IsBackground ? '4' : '3';
            Buffer[Count++] = '9';
            Buffer[Count++] = 'm';
        } break;
        }
    }
    std::basic_string_view<Char> GetString() noexcept
    {
        Buffer[Count] = '\0';
        return { Buffer.data(), Count };
    }
};


#if COMMON_OS_WIN

class NormalConsole final : public ConsoleEx
{
    std::optional<str::Encoding> DefaultEncoding;
    void LockOutput() const final 
    {
        _lock_file(stdout);
    }
    void UnlockOutput() const final
    {
        fflush(stdout);
        _unlock_file(stdout);
    }
    void PrintCore(std::u16string_view str) const
    {
        if (fwide(stdout, 0) > 0)
        {
            fwrite(str.data(), sizeof(char16_t), str.size(), stdout);
        }
        else
        {
            _unlock_file(stdout);
            std::string newStr;
            if (DefaultEncoding)
                newStr = str::to_string(str, *DefaultEncoding);
            else
            {
                const auto size = WideCharToMultiByte(CP_ACP, 0, reinterpret_cast<const wchar_t*>(str.data()),
                    gsl::narrow_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
                if (size == 0)
                    newStr = common::str::to_string(str);
                else
                {
                    newStr.resize(size);
                    WideCharToMultiByte(CP_ACP, 0, reinterpret_cast<const wchar_t*>(str.data()),
                        gsl::narrow_cast<int>(str.size()), newStr.data(), size, nullptr, nullptr);
                }
            }
            _lock_file(stdout);
            fwrite(newStr.data(), sizeof(char), newStr.size(), stdout);
        }
    }
    void PrintSegment(const ScreenColor, const ScreenColor, span<const std::byte> raw) const final
    {
        PrintCore({ reinterpret_cast<const char16_t*>(raw.data()), raw.size() / sizeof(char16_t) });
    }
    void PrintSegment(const ScreenColor, const ScreenColor, std::u16string_view str) const final
    {
        PrintCore(str);
    }
    std::optional<str::Encoding> PreferEncoding() const noexcept final { return {}; }
    bool SupportColor() const noexcept final { return false; }
    void Print(const CommonColor, std::u16string_view str) const final
    {
        Print(str);
    }
    void Print(std::u16string_view str) const final
    {
        LockOutput();
        PrintCore(str);
        UnlockOutput();
    }
    std::pair<uint32_t, uint32_t> GetConsoleSize() const noexcept final
    {
        return { 0,0 };
    }
    bool ClearConsole() const noexcept final
    {
        return false;
    }
public:
    NormalConsole()
    {
        switch (GetACP())
        {
        case 936:   DefaultEncoding = str::Encoding::GB18030; break;
        case 65000: DefaultEncoding = str::Encoding::UTF7;    break;
        case 65001: DefaultEncoding = str::Encoding::UTF8;    break;
        default: break;
        }
    }
};

class NativeConsole : public ConsoleEx
{
protected:
    HANDLE OutHandle;
    WORD DefaultAttr = 0;
    std::pair<CommonColor, CommonColor> DefaultColor = { CommonColor::White, CommonColor::White };
    NativeConsole(HANDLE handle) : OutHandle(handle)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbInfo = {};
        ::GetConsoleScreenBufferInfo(handle, &csbInfo);
        DefaultAttr = csbInfo.wAttributes;
        DefaultColor = GetCurrentColor(DefaultAttr);
    }
    void LockOutput() const final
    {
        _lock_file(stdout);
    }
    void UnlockOutput() const final
    {
        _unlock_file(stdout);
    }
    using ConsoleEx::PrintSegment;
    void PrintSegment(const ScreenColor fgColor, const ScreenColor bgColor, span<const std::byte> raw) const final
    {
        PrintSegment(fgColor, bgColor, std::u16string_view{ reinterpret_cast<const char16_t*>(raw.data()), raw.size() / sizeof(char16_t) });
    }
    std::optional<str::Encoding> PreferEncoding() const noexcept final { return str::Encoding::UTF16LE; }
    bool SupportColor() const noexcept final { return true; }
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
    void Print(const CommonColor color, std::u16string_view str) const final
    {
        LockOutput();
        SetConsoleTextAttribute(OutHandle, GetColorVal(color));
        WriteToConsole(str.data(), str.size());
        SetConsoleTextAttribute(OutHandle, DefaultAttr);
        UnlockOutput();
    }
    std::pair<uint32_t, uint32_t> GetConsoleSize() const noexcept final
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (!::GetConsoleScreenBufferInfo(OutHandle, &csbi))
            return { 0,0 };
        const auto wdRow = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        const auto wdCol = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        return { wdRow, wdCol };
    }
    bool ClearConsole() const noexcept final
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        bool isSuc = false;
        LockOutput();
        if (::GetConsoleScreenBufferInfo(OutHandle, &csbi))
        {
            const DWORD wdSize = csbi.dwSize.X * csbi.dwSize.Y;
            COORD origin = { 0,0 };
            DWORD writtenChars;
            if (::FillConsoleOutputCharacterW(OutHandle, L' ', wdSize, origin, &writtenChars))
            {
                if (::FillConsoleOutputAttribute(OutHandle, csbi.wAttributes, wdSize, origin, &writtenChars))
                {
                    isSuc = ::SetConsoleCursorPosition(OutHandle, origin) != 0;
                }
            }
        }
        UnlockOutput();
        return isSuc;
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
    static constexpr WORD GetColorVal(const std::pair<CommonColor, CommonColor> color) noexcept
    {
        const auto fg = GetColorVal(color.first), bg = GetColorVal(color.second);
        return fg | (bg << 4);
    }
    static constexpr CommonColor GetValColor(const WORD attr) noexcept
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
    static constexpr std::pair<CommonColor, CommonColor> GetCurrentColor(const WORD attr) noexcept
    {
        const auto fg = GetValColor(attr), bg = GetValColor(attr >> 4);
        return { fg, bg };
    }
public:
    ~NativeConsole() override
    { }
};

class ClassicConsole final : public NativeConsole
{
    void PrintSegment(const ScreenColor fgColor, const ScreenColor bgColor, std::u16string_view str) const final
    {
        auto color = DefaultColor;
        if (fgColor.Type == ScreenColor::ColorType::Common)
            color.first = static_cast<CommonColor>(fgColor.Value[0]);
        if (bgColor.Type == ScreenColor::ColorType::Common)
            color.second = static_cast<CommonColor>(bgColor.Value[0]);
        SetConsoleTextAttribute(OutHandle, GetColorVal(color));
        WriteToConsole(str.data(), str.size());
    }
    void Print(std::u16string_view str) const final
    {
        const size_t len = str.size();
        if (len < 5) // not gonna happen, skip
        {
            WriteToConsole(str.data(), len);
            return;
        }
        _lock_file(stdout);
        CONSOLE_SCREEN_BUFFER_INFO csbInfo;
        ::GetConsoleScreenBufferInfo(OutHandle, &csbInfo);
        const WORD baseAttr = csbInfo.wAttributes & 0xff00;
        auto curColor = GetCurrentColor(csbInfo.wAttributes);
        // CommonColor std::pair<CommonColor, CommonColor>::*
        constexpr auto fgClr = &std::pair<CommonColor, CommonColor>::first;
        constexpr auto bgClr = &std::pair<CommonColor, CommonColor>::second;
        while (true)
        {
            const auto newOffset = str.find(u"\x1b[");
            if (newOffset == std::u16string_view::npos || newOffset + 5 > len)
                break;
            const auto extra = str.substr(0, newOffset);
            if (!extra.empty())
                WriteToConsole(extra.data(), extra.size());
            str.remove_prefix(newOffset + 2);

            boost::container::small_vector<uint8_t, 8> codes;
            bool sucFinish = false;
            {
                uint32_t val = 0;
                bool hasVal = false;
                while (!str.empty())
                {
                    const auto ch = str[0];
                    if (ch >= u'0' && ch <= u'9')
                    {
                        val = val * 10 + (ch - u'0');
                        str.remove_prefix(1);
                        if (ch > UINT8_MAX)
                            break;
                        hasVal = true;
                    }
                    else if (ch == u';' || ch == u'm')
                    {
                        str.remove_prefix(1);
                        if (!hasVal && ch == u';')
                            break;
                        if (hasVal)
                        {
                            codes.push_back(static_cast<uint8_t>(val));
                            val = 0, hasVal = false;
                        }
                        if (ch == u';')
                            continue;
                        else
                        {
                            sucFinish = true;
                            break;
                        }
                    }
                    else
                        break;
                }
            }
            if (!sucFinish)
                continue;

            if (codes.empty()) // set default
            {
                SetConsoleTextAttribute(OutHandle, DefaultAttr);
                continue;
            }

            auto newColor = curColor;
            size_t offset = 0;
            while (offset < codes.size())
            {
                bool isBright = false;
                if (codes[offset] == 0)
                {
                    newColor = DefaultColor;
                    offset++;
                }
                else if (codes[offset] == 1 || codes[offset] == 22) // ESC[1;3xm
                {
                    isBright = codes[offset] == 1;
                    offset++;
                }
                else if (codes[offset] == 39 || codes[offset] == 49)
                {
                    const auto accessor = codes[offset] == 39 ? fgClr : bgClr;
                    newColor.*accessor = DefaultColor.*accessor;
                    offset++;
                }
                else if (codes[offset] == 38 || codes[offset] == 48) // 8bit/24bit
                {
                    const auto accessor = codes[offset] == 38 ? fgClr : bgClr;
                    offset++;
                    if (codes.size() <= offset)
                        break;
                    const auto codetype = codes[offset];
                    offset++;
                    if (codes.size() <= offset)
                        break;
                    if (codetype == 2) // 24bit, skip
                    {
                        offset += 3;
                    }
                    else if (codetype == 5)
                    {
                        const auto clrval = codes[offset];
                        if (clrval < 16)
                            newColor.*accessor = static_cast<CommonColor>(clrval);
                        offset++;
                    }
                    else
                        break;
                }
                else if (const auto high = codes[offset] / 10, low = codes[offset] % 10;
                    low >= 0 && low <= 7 && (high == 3 || high == 4 || high == 9 || high == 10))
                {
                    const auto accessor = (high & 0x1) ? fgClr : bgClr;
                    const auto baseClr = static_cast<CommonColor>(low);
                    const bool useBright = high > 4 || isBright;
                    newColor.*accessor = baseClr | (useBright ? CommonColor::BrightBlack : CommonColor::Black);
                    offset++;
                }
                else
                    break;
            }
            if (newColor != curColor)
            {
                curColor = newColor;
                SetConsoleTextAttribute(OutHandle, baseAttr | GetColorVal(curColor));
            }
        }
        if (!str.empty())
            WriteToConsole(str.data(), str.size());
        _unlock_file(stdout);
    }
public:
    ClassicConsole(HANDLE handle) : NativeConsole(handle)
    { }
};

class VTConsole final : public NativeConsole
{
    void PrintSegment(const ScreenColor fgColor, const ScreenColor bgColor, std::u16string_view str) const final
    {
        EscapeColorFiller<char16_t> filler;
        if (!fgColor.IsUnchanged)
            filler.PutColor(fgColor);
        if (!bgColor.IsUnchanged)
            filler.PutColor(bgColor);
        const auto clr = filler.GetString();
        WriteToConsole(clr.data(), clr.size());
        WriteToConsole(str.data(), str.size());
    }
    void Print(std::u16string_view str) const final
    {
        _lock_file(stdout);
        WriteToConsole(str.data(), str.size());
        _unlock_file(stdout);
    }
public:
    VTConsole(HANDLE handle) : NativeConsole(handle)
    { }
};

const ConsoleEx& ConsoleEx::Get() noexcept
{
    static auto console = []() -> std::unique_ptr<ConsoleEx>
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

#elif COMMON_OS_UNIX || 1

class NormalConsole : public ConsoleEx
{
protected:
    void LockOutput() const final
    {
        flockfile(stdout);
    }
    void UnlockOutput() const final
    {
        fflush(stdout);
        funlockfile(stdout);
    }
private:
    void PrintSegment(const ScreenColor, const ScreenColor, span<const std::byte> raw) const override
    {
        fwrite(raw.data(), sizeof(char), raw.size(), stdout);
    }
    void PrintSegment(const ScreenColor fgColor, const ScreenColor bgColor, std::u16string_view str) const final
    {
        const auto newStr = str::to_string(str, common::str::Encoding::UTF8);
        PrintSegment(fgColor, bgColor, as_bytes(to_span(newStr)));
    }
    std::optional<str::Encoding> PreferEncoding() const noexcept final { return str::Encoding::UTF8; }
    bool SupportColor() const noexcept override { return false; }
    void Print(const CommonColor, std::u16string_view str) const override
    {
        Print(str);
    }
    void Print(std::u16string_view str) const final
    {
        const auto newStr = str::to_string(str, common::str::Encoding::UTF8);
        LockOutput();
        fwrite(newStr.data(), sizeof(char), newStr.size(), stdout);
        UnlockOutput();
    }
    std::pair<uint32_t, uint32_t> GetConsoleSize() const noexcept final
    {
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
        const auto wdRow = ws.ws_col;
        const auto wdCol = ws.ws_row;
        return std::pair<uint32_t, uint32_t>(wdRow, wdCol);
    }
    bool ClearConsole() const noexcept override
    {
        return false;
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
    void PrintSegment(const ScreenColor fgColor, const ScreenColor bgColor, span<const std::byte> raw) const final
    {
        EscapeColorFiller<char> filler;
        if (!fgColor.IsUnchanged)
            filler.PutColor(fgColor);
        if (!bgColor.IsUnchanged)
            filler.PutColor(bgColor);
        const auto clr = filler.GetString();
        fwrite(clr.data(), sizeof(char), clr.size(), stdout);
        fwrite(raw.data(), sizeof(char), raw.size(), stdout);
    }
    bool SupportColor() const noexcept final { return true; }
    void Print(const CommonColor color, std::u16string_view str) const final
    {
        const auto newStr = str::to_string(str, common::str::Encoding::UTF8);
        const auto pfx = GetColorPfx(color);
        constexpr auto sfx = "\x1b[39m"sv;
        LockOutput();
        fwrite(pfx.data(), sizeof(char), pfx.size(), stdout);
        fwrite(newStr.data(), sizeof(char), newStr.size(), stdout);
        fwrite(sfx.data(), sizeof(char), sfx.size(), stdout);
        UnlockOutput();
    }
    bool ClearConsole() const noexcept final
    {
        LockOutput();
        constexpr auto seq = "\033c\x1b[2J"sv;
        fwrite(seq.data(), sizeof(char), seq.size(), stdout);
        /*printf("\033c");
        printf("\x1b[2J");*/
        UnlockOutput();
        return true;
    }
};

const ConsoleEx& ConsoleEx::Get() noexcept
{
    static auto console = []() -> std::unique_ptr<ConsoleEx>
    {
        const auto fd = fileno(stdout);
        const auto isAtty = isatty(fd);
        if (isAtty)
        {
            const auto str = GetEnvVar("TERM");
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


char ConsoleEx::ReadCharImmediate(bool ShouldEcho) noexcept
{
#if COMMON_OS_WIN
    const auto ret = ShouldEcho ? _getche() : _getch();
    return static_cast<char>(ret);
#else
    static thread_local termios info;
    flockfile(stdin);
    tcgetattr(0, &info);
    const auto c_lflag = info.c_lflag;
    info.c_lflag &= ~ICANON;
    if (ShouldEcho)
        info.c_lflag |= ECHO;
    else
        info.c_lflag &= ~ECHO;
    const auto c_cc_VMIN = info.c_cc[VMIN];
    const auto c_cc_VTIME = info.c_cc[VTIME];
    info.c_cc[VMIN] = 1;
    info.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &info);

    char buf = 0;
    read(0, &buf, 1);

    info.c_lflag = c_lflag;
    info.c_cc[VMIN] = c_cc_VMIN;
    info.c_cc[VTIME] = c_cc_VTIME;
    tcsetattr(0, TCSADRAIN, &info);
    funlockfile(stdin);
    return buf;
#endif
}

//std::pair<uint32_t, uint32_t> common::console::ConsoleEx::GetConsoleSize() noexcept
//{
//#if COMMON_OS_WIN
//    CONSOLE_SCREEN_BUFFER_INFO csbi;
//    if (const auto handle = GetStdHandle(STD_OUTPUT_HANDLE); handle == INVALID_HANDLE_VALUE)
//        return { 0,0 };
//    else if (!GetConsoleScreenBufferInfo(handle, &csbi))
//        return { 0,0 };
//    const auto wdRow = csbi.srWindow.Right - csbi.srWindow.Left + 1;
//    const auto wdCol = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
//#else
//    struct winsize ws;
//    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
//    const auto wdRow = ws.ws_col;
//    const auto wdCol = ws.ws_row;
//#endif
//    return std::pair<uint32_t, uint32_t>(wdRow, wdCol);
//}


//bool common::console::ConsoleEx::ClearConsole() noexcept
//{
//#if COMMON_OS_WIN
//    CONSOLE_SCREEN_BUFFER_INFO csbi;
//    if (const auto handle = GetStdHandle(STD_OUTPUT_HANDLE); handle == INVALID_HANDLE_VALUE)
//        return false;
//    else if (!GetConsoleScreenBufferInfo(handle, &csbi))
//        return false;
//    else
//    {
//        const DWORD wdSize = csbi.dwSize.X * csbi.dwSize.Y;
//        COORD origin = { 0,0 };
//        DWORD writtenChars;
//        if (!FillConsoleOutputCharacterW(handle, L' ', wdSize, origin, &writtenChars))
//            return false;
//        if (!FillConsoleOutputAttribute(handle, csbi.wAttributes, wdSize, origin, &writtenChars))
//            return false;
//        return SetConsoleCursorPosition(handle, origin) != 0;
//    }
//#else
//    printf("\033c");
//    printf("\x1b[3J");
//    return true;
//#endif
//}


std::string ConsoleEx::ReadLine(const std::string& prompt)
{
    std::string result;
#if COMMON_OS_WIN
    if (!prompt.empty())
        std::cout << prompt;
    std::getline(std::cin, result);
#else
    flockfile(stdin);
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
    funlockfile(stdin);
#endif
    return result;
}


}
