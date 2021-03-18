#include "XCompDebugExt.h"
#include "Nailang/NailangRuntime.h"


namespace xcomp::debug
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using common::simd::VecDataInfo;

#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)
#define NLRT_THROW_EX(...) HandleException(CREATE_EXCEPTION(xziar::nailang::NailangRuntimeException, __VA_ARGS__))



template<typename F>
static NamedVecPair GenerateInput(XCNLRuntime& runtime, std::u32string_view str, F&& errInfo)
{
    const auto idx = str.find(U':');
    std::u32string_view name;
    if (idx != std::u32string_view::npos)
    {
        if (idx != 0)
        {
            name = str.substr(0, idx);
            constexpr auto firstCheck = common::parser::ASCIIChecker<true>("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
            constexpr auto restCheck = common::parser::ASCIIChecker<true>("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_");
            bool pass = firstCheck(name[0]);
            for (size_t i = 1; i < name.size() && pass;)
                pass &= restCheck(name[i++]);
            if (!pass)
                runtime.NLRT_THROW_EX(FMTSTR(u"Arg name [{}] is not acceptable", name));
        }
        str.remove_prefix(idx + 1);
    }
    const auto vtype = runtime.ParseVecType(str, errInfo);
    return { name, vtype };
}

void XCNLDebugExt::DefineMessage(XCNLExecutor& executor, const xziar::nailang::FuncPack& func)
{
    using namespace xziar::nailang;
    auto& runtime = executor.GetRuntime();
    executor.ThrowIfNotFuncTarget(func, FuncName::FuncInfo::Empty);
    executor.ThrowByParamTypes<3, ArgLimits::AtLeast>(func, { Arg::Type::String, Arg::Type::String, Arg::Type::String });
    const auto id = func.Params[0].GetStr().value();
    const auto formatter = func.Params[1].GetStr().value();
    std::vector<NamedVecPair> argInfos;
    argInfos.reserve(func.Args.size() - 2);
    size_t i = 2;
    for (const auto& arg : common::to_span(func.Params).subspan(2))
    {
        if (!arg.IsStr())
            runtime.NLRT_THROW_EX(FMTSTR(u"Arg[{}] of [DefineDebugString] should be string, which gives [{}]",
                i, arg.GetTypeName()), func);
        argInfos.push_back(GenerateInput(runtime, arg.GetStr().value(), [&]()
            {
                return FMTSTR(u"Arg[{}] of [DefineDebugString]"sv, i);
            }));
    }
    if (!DebugInfos.try_emplace(std::u32string(id), formatter, std::move(argInfos)).second)
        runtime.NLRT_THROW_EX(fmt::format(u"DebugString [{}] repeately defined"sv, id), func);
}

const XCNLDebugExt::DbgContent& XCNLDebugExt::DefineMessage(XCNLRawExecutor& executor, std::u32string_view func, const common::span<const std::u32string_view> args)
{
    using namespace xziar::nailang;
    auto& runtime = executor.GetRuntime();
    executor.ThrowByReplacerArgCount(func, args, 2, ArgLimits::AtLeast);
    if (args.size() % 2)
        runtime.NLRT_THROW_EX(FMTSTR(u"Repalcer-Func [DebugStr] requires even number of args, which gives [{}]."sv, args.size()));
    if (args[1].size() < 2 || args[1].front() != U'"' || args[1].back() != U'"')
        runtime.NLRT_THROW_EX(FMTSTR(u"Repalcer-Func [DebugStr]'s arg[1] expects to a string with \", get [{}]", args[1]));
    const auto id = args[0], formatter = args[1].substr(1, args[1].size() - 2);

    std::vector<NamedVecPair> types;
    const auto argCnt = args.size() / 2 - 1;
    types.reserve(argCnt);
    for (size_t i = 2; i < args.size(); i += 2)
    {
        types.push_back(GenerateInput(runtime, args[i], [&]()
            {
                return FMTSTR(u"Arg[{}] of [DebugStr]"sv, i);
            }));
    }
    const auto [info, ret] = DebugInfos.try_emplace(std::u32string(id), formatter, std::move(types));
    if (!ret)
        runtime.NLRT_THROW_EX(FMTSTR(u"DebugString [{}] repeately defined", id));
    return info->second;
}


}
