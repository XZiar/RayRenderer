#pragma once

#include "SystemCommon/ColorConsole.h"
#include "SystemCommon/FileEx.h"
#include "common/TimeUtil.hpp"
#include "common/ResourceHelper.h"
#include "SystemCommon/MiniLogger.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <cinttypes>
#include "resource.h"


common::fs::path FindPath();
const std::vector<std::string_view>& GetCmdArgs();
uint32_t RegistTest(const char *name, void(*func)());
std::string LoadShaderFallback(const std::u16string& filename, int32_t id);

void PrintColored(const common::console::ConsoleColor color, const std::u16string_view str);
void PrintException(const common::BaseException& be, std::u16string_view info);
void ClearReturn();

char GetIdx36(const size_t idx);
uint32_t Select36(const size_t size);
template<typename T, typename F>
forceinline uint32_t SelectIdx(const T& container, std::u16string_view name, F&& printer)
{
    common::mlog::SyncConsoleBackend();
    size_t idx = 0;
    for (const auto& item : container)
    {
        PrintColored(common::console::ConsoleColor::BrightWhite,
            FMTSTR(u"{}[{}] {}\n", name, GetIdx36(idx++), printer(item)));
    }
    if (container.size() <= 1)
        return 0;
    PrintColored(common::console::ConsoleColor::BrightWhite,
        FMTSTR(u"Select {}:\n", name));
    return Select36(container.size());
}