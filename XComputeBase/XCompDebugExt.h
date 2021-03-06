#pragma once

#include "XCompRely.h"
#include "XCompNailang.h"
#include "XCompDebug.h"

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace xcomp::debug
{

struct XCOMPBASAPI XCNLDebugExt
{
    DebugManager DebugMan;
    using DbgContent = std::pair<std::u32string, std::vector<NamedVecPair>>;
    std::map<std::u32string, DbgContent, std::less<>> DebugInfos;

    void DefineMessage(XCNLExecutor& executor, const xziar::nailang::FuncPack& func);
    const DbgContent& DefineMessage(XCNLRawExecutor& executor, std::u32string_view func, const common::span<const std::u32string_view> args);

    forceinline const MessageBlock& AppendBlock(const std::u32string_view name,
        const std::u32string_view formatter, common::span<const NamedVecPair> args)
    {
        return DebugMan.AppendBlock(name, formatter, args);
    }
};

}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

