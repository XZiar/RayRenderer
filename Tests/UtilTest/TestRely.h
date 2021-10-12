#pragma once

#include "SystemCommon/FileEx.h"
#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/ConsoleEx.h"
#include "common/TimeUtil.hpp"
#include "common/ResourceHelper.h"
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

//void GetConsole().Print(const common::CommonColor color, const std::u16string_view str);
inline const common::console::ConsoleEx& GetConsole()
{
    return common::console::ConsoleEx::Get();
}
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
        GetConsole().Print(common::CommonColor::BrightWhite,
            FMTSTR(u"{}[{}] {}\n", name, GetIdx36(idx++), printer(item)));
    }
    if (container.size() <= 1)
        return 0;
    GetConsole().Print(common::CommonColor::BrightWhite, FMTSTR(u"Select {}:\n", name));
    return Select36(container.size());
}