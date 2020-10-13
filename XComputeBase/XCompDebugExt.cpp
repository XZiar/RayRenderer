#include "XCompDebugExt.h"
#include "Nailang/NailangRuntime.h"


namespace xcomp::debug
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using common::simd::VecDataInfo;

#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)
#define NLRT_THROW_EX(...) Runtime.HandleException(CREATE_EXCEPTION(xziar::nailang::NailangRuntimeException, __VA_ARGS__))


struct XCNLRuntime_ : public XCNLRuntime
{
    friend XCNLDebugExt;
    using XCNLRuntime::HandleException;
    using XCNLRuntime::ParseVecType;
};


template<typename F>
static ArgsLayout::InputType GenerateInput(XCNLRuntime_& Runtime, std::u32string_view str, F&& errInfo)
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
                NLRT_THROW_EX(FMTSTR(u"Arg name [{}] is not acceptable", name));
        }
        str.remove_prefix(idx + 1);
    }
    const auto vtype = Runtime.ParseVecType(str, errInfo);
    return { name, vtype };
}

void XCNLDebugExt::DefineMessage(XCNLRuntime& runtime, const xziar::nailang::FuncCall& call)
{
    using namespace xziar::nailang;
    auto& Runtime = static_cast<XCNLRuntime_&>(runtime);
    Runtime.ThrowIfNotFuncTarget(call, FuncName::FuncInfo::Empty);
    Runtime.ThrowByArgCount(call, 3, ArgLimits::AtLeast);
    const auto arg2 = Runtime.EvaluateFuncArgs<2, ArgLimits::AtLeast>(call, { Arg::Type::String, Arg::Type::String });
    const auto id = arg2[0].GetStr().value();
    const auto formatter = arg2[1].GetStr().value();
    std::vector<ArgsLayout::InputType> argInfos;
    argInfos.reserve(call.Args.size() - 2);
    size_t i = 2;
    for (const auto& rawarg : call.Args.subspan(2))
    {
        const auto arg = Runtime.EvaluateArg(rawarg);
        if (!arg.IsStr())
            NLRT_THROW_EX(FMTSTR(u"Arg[{}] of [DefineDebugString] should be string, which gives [{}]",
                i, arg.GetTypeName()), call);
        argInfos.push_back(GenerateInput(Runtime, arg.GetStr().value(), [&]()
            {
                return FMTSTR(u"Arg[{}] of [DefineDebugString]"sv, i);
            }));
    }
    if (!DebugInfos.try_emplace(std::u32string(id), formatter, std::move(argInfos)).second)
        NLRT_THROW_EX(fmt::format(u"DebugString [{}] repeately defined"sv, id), call);
}

const XCNLDebugExt::DbgContent& XCNLDebugExt::DefineMessage(XCNLRuntime& runtime, std::u32string_view func, const common::span<const std::u32string_view> args)
{
    using namespace xziar::nailang;
    auto& Runtime = static_cast<XCNLRuntime_&>(runtime);
    Runtime.ThrowByReplacerArgCount(func, args, 2, ArgLimits::AtLeast);
    if (args.size() % 2)
        NLRT_THROW_EX(FMTSTR(u"Repalcer-Func [DebugStr] requires even number of args, which gives [{}]."sv, args.size()));
    if (args[1].size() < 2 || args[1].front() != U'"' || args[1].back() != U'"')
        NLRT_THROW_EX(FMTSTR(u"Repalcer-Func [DebugStr]'s arg[1] expects to a string with \", get [{}]", args[1]));
    const auto id = args[0], formatter = args[1].substr(1, args[1].size() - 2);

    std::vector<ArgsLayout::InputType> types;
    const auto argCnt = args.size() / 2 - 1;
    types.reserve(argCnt);
    for (size_t i = 2; i < args.size(); i += 2)
    {
        types.push_back(GenerateInput(Runtime, args[i], [&]()
            {
                return FMTSTR(u"Arg[{}] of [DebugStr]"sv, i);
            }));
    }
    const auto [info, ret] = DebugInfos.try_emplace(std::u32string(id), formatter, std::move(types));
    if (!ret)
        NLRT_THROW_EX(FMTSTR(u"DebugString [{}] repeately defined", id));
    return info->second;
}

void XCNLDebugExt::SetInfoProvider(std::unique_ptr<InfoProvider>&& infoProv) noexcept
{
    DebugManager.InfoProv = std::move(infoProv);
}


}
