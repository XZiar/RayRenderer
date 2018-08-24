#pragma once

#include "fmt/format.h"
#include "fmt/utfext.h"
#include "common/ColorConsole.h"
#include <tuple>
#include <array>


fmt::basic_memory_buffer<char16_t>& GetFmtBuffer();
template<class... Args>
inline std::u16string_view ToU16Str(const std::u16string_view& formater, Args&&... args)
{
    auto& buffer = GetFmtBuffer();
    buffer.resize(0);
    fmt::format_to(buffer, formater, std::forward<Args>(args)...);
    return std::u16string_view(buffer.data(), buffer.size());
}
inline std::u16string_view ToU16Str(const std::u16string_view& content)
{
    return content;
}

enum class LogType { Error, Success, Warning, Info, Verbose };
inline std::u16string ToColor(const LogType type)
{
    using namespace common::console;
    switch (type)
    {
    case LogType::Error:    return ConsoleHelper::GetColorStr(ConsoleColor::BrightRed);
    case LogType::Success:  return ConsoleHelper::GetColorStr(ConsoleColor::BrightGreen);
    case LogType::Warning:  return ConsoleHelper::GetColorStr(ConsoleColor::BrightYellow);
    default:
    case LogType::Info:     return ConsoleHelper::GetColorStr(ConsoleColor::BrightWhite);
    case LogType::Verbose:  return ConsoleHelper::GetColorStr(ConsoleColor::BrightCyan);
    }
}
const common::console::ConsoleHelper& GetConsole();
template<class... Args>
inline void Log(const LogType type, const std::u16string_view& formater, Args&&... args)
{
    const auto txt = ToColor(type).append(ToU16Str(formater, std::forward<Args>(args)...)).append(u"\x1b[39m\n");
    GetConsole().Print(txt);
}


uint32_t RegistTest(const char16_t *name, const uint32_t simdLv, bool(*func)());
