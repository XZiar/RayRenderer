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


}
}
