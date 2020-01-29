#include "SystemCommonPch.h"
#include "ConsoleEx.h"


char common::console::ConsoleEx::ReadCharImmediate(bool ShouldEcho) noexcept
{
#if defined(_WIN32)
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
#if defined(_WIN32)
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
#if defined(_WIN32)
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
