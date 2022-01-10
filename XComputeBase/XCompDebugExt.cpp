#include "XCompDebugExt.h"
#include "Nailang/NailangRuntime.h"
#include "VecBase.hpp"
#include "VecSIMD.hpp"
#include "MatBase.hpp"

namespace xcomp::debug
{
using namespace std::string_literals;
using namespace std::string_view_literals;

#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)
#define NLRT_THROW_EX(...) HandleException(CREATE_EXCEPTION(xziar::nailang::NailangRuntimeException, __VA_ARGS__))


template<typename V2, typename V3, typename IV3, typename V4, typename IV4>
void TestVec()
{
    static_assert(sizeof(V2)  == 8);
    static_assert(sizeof(V3)  == 16);
    static_assert(sizeof(IV3) == 16);
    static_assert(sizeof(V4)  == 16);
    static_assert(sizeof(IV4) == 16);

    V2 a2, b2, d2;
    [[maybe_unused]] const auto c2 = Dot(a2, b2);
    [[maybe_unused]] const auto e2 = (a2[1] + Min(c2, d2)) + (b2[1] - Max(a2, b2)) - c2 + a2[0] - b2[0];
    [[maybe_unused]] const auto f2 = b2[1] / (a2[1] * (e2 * a2[0] / b2[1]));
    [[maybe_unused]] const auto g2 = e2.Sqrt().Negative().Length();

    V3 a3, b3(1.0f, b2);
    [[maybe_unused]] const auto c3 = Dot(a3, b3);
    [[maybe_unused]] const auto d3 = Cross(a3, b3);
    [[maybe_unused]] const auto e3 = (a3[1] + Min(c3, d3)) + (b3[1] - Max(a3, b3)) - c3 + a3[0] - b3[0];
    [[maybe_unused]] const auto f3 = b3[1] / (a3[1] * (e3 * a3[0] / b3[1]));
    [[maybe_unused]] const auto g3 = e3.Sqrt().Negative().Length();

    IV3 a3i, b3i;
    [[maybe_unused]] const auto c3i = Dot(a3i, b3i);
    [[maybe_unused]] const auto e3i = (a3i[1] + Min(c3i, d3.template As<IV3>())) + (b3i[1] - Max(a3i, b3i)) - c3i + a3i[0] - b3i[0];
    [[maybe_unused]] const auto f3i = b3i[1] / (a3i[1] * (e3i * a3i[0] / b3i[1]));
    [[maybe_unused]] const auto g3i = e3i.Negative().LengthSqr();

    V4 a4(a2, b2), b4(3.0f, b3);
    [[maybe_unused]] const auto c4 = Dot(a4, b4);
    [[maybe_unused]] const auto e4 = (a4[1] + Min(c4, d3.template As<V4>())) + (b4[1] - Max(a4, b4)) - c4 + a4[0] - b4[0];
    [[maybe_unused]] const auto f4 = b4[1] / (a4[1] * (e4 * a4[0] / b4[1]));
    [[maybe_unused]] const auto g4 = e4.Sqrt().Negative().Length();

    IV4 a4i, b4i;
    [[maybe_unused]] const auto c4i = Dot(a4i, b4i);
    [[maybe_unused]] const auto e4i = (a4i[1] + Min(c4i, d3.template As<IV4>())) + (b4i[1] - Max(a4i, b4i)) - c4i + a4i[0] - b4i[0];
    [[maybe_unused]] const auto f4i = b4i[1] / (a4i[1] * (e4i * a4i[0] / b4i[1]));
    [[maybe_unused]] const auto g4i = e4i.Negative().LengthSqr();
}

void TestVec2()
{
    using namespace math;
    TestVec<base::Vec2, base::Vec3, base::IVec3, base::Vec4, base::IVec4>();
    TestVec<simd::Vec2, simd::Vec3, simd::IVec3, simd::Vec4, simd::IVec4>();
    [[maybe_unused]] base::Normal x, y, z = x + y;
}


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
            constexpr auto firstCheck = common::ASCIIChecker<true>("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
            constexpr auto restCheck = common::ASCIIChecker<true>("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_");
            bool pass = firstCheck(name[0]);
            for (size_t i = 1; i < name.size() && pass;)
                pass &= restCheck(name[i++]);
            if (!pass)
                runtime.NLRT_THROW_EX(FMTSTR(u"Arg name [{}] is not acceptable", name));
        }
        str.remove_prefix(idx + 1);
    }
    const auto vtype = runtime.ParseVecDataInfo(str, errInfo);
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
