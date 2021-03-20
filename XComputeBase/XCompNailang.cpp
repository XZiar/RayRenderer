#include "XCompNailang.h"
#include "SystemCommon/StackTrace.h"
#include "StringUtil/Convert.h"
#include "common/StrParsePack.hpp"
#include <shared_mutex>

namespace xcomp
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using xziar::nailang::Arg;
using xziar::nailang::ArgLocator;
using xziar::nailang::SubQuery;
using xziar::nailang::Expr;
using xziar::nailang::Block;
using xziar::nailang::RawBlock;
using xziar::nailang::Statement;
using xziar::nailang::NilCheck;
using xziar::nailang::CustomVar;
using xziar::nailang::CompareResultCore;
using xziar::nailang::CompareResult;
using xziar::nailang::NativeWrapper;
using xziar::nailang::FixedArray;
using xziar::nailang::FuncCall;
using xziar::nailang::FuncPack;
using xziar::nailang::FuncEvalPack;
using xziar::nailang::MetaSet;
using xziar::nailang::MetaEvalPack;
using xziar::nailang::ArgLimits;
using xziar::nailang::NailangFrame;
using xziar::nailang::NailangExecutor;
using xziar::nailang::NailangRuntime;
using xziar::nailang::NailangRuntimeException;
using xziar::nailang::detail::ExceptionTarget;
using common::mlog::LogLevel;
using common::str::Charset;
using common::str::IsBeginWith;
using common::simd::VecDataInfo;
using FuncInfo = xziar::nailang::FuncName::FuncInfo;

MAKE_ENABLER_IMPL(ReplaceDepend)
MAKE_ENABLER_IMPL(XCNLProgram)


#define NLRT_THROW_EX(...) HandleException(CREATE_EXCEPTION(NailangRuntimeException, __VA_ARGS__))
#define APPEND_FMT(str, syntax, ...) fmt::format_to(std::back_inserter(str), FMT_STRING(syntax), __VA_ARGS__)



struct TemplateBlockInfo : public OutputBlock::BlockInfo
{
    std::vector<std::u32string_view> ReplaceArgs;
    TemplateBlockInfo(common::span<const xziar::nailang::Expr> args)
    {
        ReplaceArgs.reserve(args.size());
        for (const auto& arg : args)
        {
            ReplaceArgs.emplace_back(arg.GetVar<Expr::Type::Var>().Name);
        }
    }
    ~TemplateBlockInfo() override {}
};


std::u32string_view OutputBlock::BlockTypeName(const BlockType type) noexcept
{
    switch (type)
    {
    case BlockType::Global:     return U"Global"sv;
    case BlockType::Struct:     return U"Struct"sv;
    case BlockType::Instance:   return U"Instance"sv;
    case BlockType::Template:   return U"Template"sv;
    default:                    return U"Unknwon"sv;
    }
}


NamedTextHolder::NamedText::NamedText(std::u32string_view content,
    common::StringPiece<char32_t> idstr, size_t offset, size_t cnt) :
    Content(content), IDStr(idstr), Dependency{gsl::narrow_cast<uint32_t>(offset), gsl::narrow_cast<uint32_t>(cnt) }
{ }

NamedTextHolder::~NamedTextHolder()
{ }

void NamedTextHolder::ForceAdd(std::vector<NamedText>& container, std::u32string_view id, common::str::StrVariant<char32_t> content, U32StrSpan depends)
{
    const auto offset = Dependencies.size();
    Dependencies.reserve(offset + depends.size());
    for (const auto& dep : depends)
    {
        if (dep == id) // early check for self-dependency
            COMMON_THROW(common::BaseException, FMTSTR(u"self dependency at [{}] for [{}]"sv, &dep - depends.data(), id));

        bool match = false;
        for (size_t i = 0; i < container.size(); ++i)
        {
            const auto& item = container[i];
            if (GetID(item) == dep)
            {
                const auto idx = gsl::narrow_cast<uint32_t>(i);
                Dependencies.emplace_back(item.IDStr, idx);
                match = true;
                break;
            }
        }

        if (!match)
        {
            const auto str = Names.AllocateString(dep);
            Dependencies.emplace_back(str, UINT32_MAX);
        }
    }
    const auto idstr = Names.AllocateString(id);
    container.push_back({ content.ExtractStr(), idstr, offset, depends.size() });
}

void NamedTextHolder::Write(std::u32string& output, const NamedText& item) const
{
    APPEND_FMT(output, U"    //vvvvvvvv below injected by {}  vvvvvvvv\r\n"sv, GetID(item));
    output.append(item.Content).append(U"\r\n"sv);
    APPEND_FMT(output, U"    //^^^^^^^^ above injected by {}  ^^^^^^^^\r\n\r\n"sv, GetID(item));
}

void NamedTextHolder::Write(std::u32string& output, const std::vector<NamedText>& container) const
{
    if (Dependencies.empty() || container.empty()) // fast path
    {
        for (const auto& item : container)
            Write(output, item);
        return;
    }
    std::vector<bool> outputMask(container.size(), false);
    size_t waiting = container.size();
    while (waiting > 0)
    {
        const auto last = waiting;
        for (size_t i = 0; i < container.size(); ++i)
        {
            if (outputMask[i])
                continue;
            const auto& item = container[i];
            if (item.DependCount() != 0)
            {
                bool satisfy = true;
                for (uint32_t j = 0; satisfy && j < item.DependCount(); ++j)
                {
                    const auto depIdx = SearchDepend(container, item, j);
                    if (!depIdx.has_value())
                    {
                        satisfy = false;
                        COMMON_THROW(common::BaseException, FMTSTR(u"unsolved dependency [{}] for [{}]"sv,
                            GetDepend(item, j), GetID(item)));
                    }
                    else if (!outputMask[depIdx.value()])
                    {
                        satisfy = false;
                    }
                }
                if (!satisfy) // skip this item
                    continue;
            }
            Write(output, item);
            outputMask[i] = true;
            waiting--;
        }
        if (last == waiting) // some item cannot be resolved
            break;
    }
    if (waiting > 0) // generate report and throw
    {
        std::u16string report = u"unmatched dependencies:\r\n"s;
        for (size_t i = 0; i < container.size(); ++i)
        {
            if (outputMask[i])
                continue;
            const auto& item = container[i];
            Expects(item.DependCount() != 0);

            APPEND_FMT(report, u"[{}]: total [{}] depends, unmatch:[ "sv, GetID(item), item.DependCount());
            for (uint32_t j = 0, unmth = 0; j < item.DependCount(); ++j)
            {
                const auto depIdx = SearchDepend(container, item, j);
                Expects(depIdx.has_value());
                if (!outputMask[depIdx.value()])
                {
                    if (unmth++ > 0)
                        report.append(u", "sv);
                    const auto dstName = GetID(container[depIdx.value()]);
                    report.append(common::str::to_u16string(dstName));
                }
            }
            report.append(u" ]\r\n"sv);
        }
        COMMON_THROW(common::BaseException, report);
    }
}

std::shared_ptr<const ReplaceDepend> NamedTextHolder::GenerateReplaceDepend(const NamedText& item, bool isPthBlk, 
    U32StrSpan pthBlk, U32StrSpan bdyPfx) const
{
    if (item.DependCount() == 0)
    {
        return ReplaceDepend::Create(pthBlk, bdyPfx);
    }

    std::vector<std::u32string_view> newDeps;
    auto& oldDeps = isPthBlk ? pthBlk : bdyPfx;
    newDeps.reserve(oldDeps.size() + item.DependCount());
    newDeps.insert(newDeps.begin(), oldDeps.begin(), oldDeps.end());
    for (uint32_t i = 0; i < item.DependCount(); ++i)
    {
        newDeps.push_back(GetDepend(item, i));
    }
    oldDeps = newDeps;
    return ReplaceDepend::Create(pthBlk, bdyPfx);
}


InstanceArgInfo::InstanceArgInfo(Types type, TexTypes texType, std::u32string_view name, std::u32string_view dtype,
    const std::vector<xziar::nailang::Arg>& args) :
    Name(name), DataType(dtype), ExtraArg(args), TexType(texType), Type(type), Flag(Flags::Empty)
{
    Extra.reserve(ExtraArg.size());
    for (const auto& arg : ExtraArg)
    {
        switch (const auto& txt = arg.GetStr().value(); common::DJBHash::HashC(txt))
        {
        HashCase(txt, U"read")      Flag |= Flags::Read; break;
        HashCase(txt, U"write")     Flag |= Flags::Write; break;
        HashCase(txt, U"restrict")  Flag |= Flags::Restrict; break;
        HashCase(txt, U"")          break; // ignore empty
        default:                    Extra.push_back(txt); break; // only add unrecognized flags
        }
    }
}
std::string_view InstanceArgInfo::GetTexTypeName(const TexTypes type) noexcept
{
    switch (type)
    {
    case TexTypes::Tex1D:       return "Tex1D"sv;
    case TexTypes::Tex2D:       return "Tex2D"sv;
    case TexTypes::Tex3D:       return "Tex3D"sv;
    case TexTypes::Tex1DArray:  return "Tex1DArray"sv;
    case TexTypes::Tex2DArray:  return "Tex2DArray"sv;
    default:                    return "Empty"sv;
    }
}


struct InstanceArgData
{
    std::vector<xziar::nailang::Arg> Args;
    std::u32string Name;
    std::u32string DataType;
    InstanceArgInfo::Types Type;
    InstanceArgInfo::TexTypes TexType;
};
struct InstanceArgCustomVar : public xziar::nailang::CustomVar::Handler
{
    struct InstanceArgStore
    {
        std::vector<xziar::nailang::Arg> ExtraArgs;
        std::u32string Name;
        std::u32string DataType;
        InstanceArgInfo ArgInfo;
        InstanceArgStore(InstanceArgData&& data) :
            ExtraArgs(std::move(data.Args)), Name(std::move(data.Name)), DataType(std::move(data.DataType)),
            ArgInfo(data.Type, data.TexType, Name, DataType, ExtraArgs)
        { }
    };
    struct COMMON_EMPTY_BASES InstanceArgHolder : public common::FixedLenRefHolder<InstanceArgHolder, InstanceArgStore>
    {
        using Host = common::FixedLenRefHolder<InstanceArgHolder, InstanceArgStore>;
        uint64_t Pointer = 0;
        InstanceArgHolder(InstanceArgData&& data)
        {
            auto ptr = Host::Allocate(1).data();
            new(ptr) InstanceArgStore(std::move(data));
            Pointer = reinterpret_cast<uintptr_t>(ptr);
        }
        InstanceArgHolder(const InstanceArgHolder& other) noexcept : Pointer(other.Pointer)
        {
            this->Increase();
        }
        InstanceArgHolder(InstanceArgHolder&& other) noexcept : Pointer(other.Pointer)
        {
            other.Pointer = 0;
        }
        ~InstanceArgHolder()
        {
            this->Decrease();
        }
        [[nodiscard]] forceinline uintptr_t GetDataPtr() const noexcept
        {
            return static_cast<uintptr_t>(Pointer);
        }
        [[nodiscard]] forceinline InstanceArgStore* GetObj() const noexcept
        {
            return reinterpret_cast<InstanceArgStore*>(GetDataPtr());
        }
        forceinline void Destruct() noexcept 
        { 
            Expects(Pointer != 0);
            GetObj()->~InstanceArgStore();
        }
        using Host::Increase;
        using Host::Decrease;
    };
    static_assert(sizeof(InstanceArgHolder) == sizeof(uint64_t));
    forceinline static const InstanceArgHolder& GetHolder(const CustomVar& var) noexcept
    {
        return *reinterpret_cast<const InstanceArgHolder*>(&var.Meta0);
    }
    forceinline static InstanceArgHolder& GetHolder(CustomVar& var) noexcept
    {
        return *reinterpret_cast<InstanceArgHolder*>(&var.Meta0);
    }
    forceinline static InstanceArgStore* GetObj(const CustomVar& var) noexcept
    {
        return GetHolder(var).GetObj();
    }
    void IncreaseRef(CustomVar& var) noexcept final
    {
        GetHolder(var).Increase();
    };
    void DecreaseRef(CustomVar& var) noexcept final
    {
        GetHolder(var).Decrease();
    };
    common::str::StrVariant<char32_t> ToString(const CustomVar& var) noexcept final
    {
        const auto obj = GetObj(var);
        if (!obj)
            return U"{ Empty }";
        switch (obj->ArgInfo.Type)
        {
        case InstanceArgInfo::Types::RawBuf:    return FMTSTR(U"{{ Raw Buf Arg [{}] }}",    obj->ArgInfo.Name);
        case InstanceArgInfo::Types::TypedBuf:  return FMTSTR(U"{{ Typed Buf Arg [{}] }}",  obj->ArgInfo.Name);
        case InstanceArgInfo::Types::Texture:   return FMTSTR(U"{{ Texture Arg [{}] }}",    obj->ArgInfo.Name);
        case InstanceArgInfo::Types::Simple:    return FMTSTR(U"{{ Simple Arg [{}] }}",     obj->ArgInfo.Name);
        default:                                return U"{ Unknown Arg }";
        }
    };
    static const InstanceArgInfo& GetArgInfo(const CustomVar& var) noexcept
    {
        return GetObj(var)->ArgInfo;
    }
    CompareResult CompareSameClass(const CustomVar& lhs, const CustomVar& rhs) final
    {
        return CompareResultCore::Equality | (GetObj(lhs) == GetObj(rhs) ? CompareResultCore::Equal : CompareResultCore::NotEqual);
    }
    std::u32string_view GetTypeName() noexcept final
    {
        return U"xcomp::arg"sv;
    }
    static InstanceArgCustomVar Handler;
    static CustomVar Create(InstanceArgData&& data)
    {
        InstanceArgHolder holder(std::move(data));
        holder.Increase();
        return { &Handler, holder.Pointer, 0, 0 };
    }
};
InstanceArgCustomVar InstanceArgCustomVar::Handler;


std::shared_ptr<const ReplaceDepend> ReplaceDepend::Create(U32StrSpan patchedBlk, U32StrSpan bodyPfx)
{
    static_assert(sizeof(std::u32string_view) % sizeof(char32_t) == 0);

    const auto count = patchedBlk.size() + bodyPfx.size();
    if (count == 0)
        return Empty();

    const auto ret = MAKE_ENABLER_SHARED(ReplaceDepend, ());
    std::vector<common::StringPiece<char32_t>> tmp; tmp.reserve(count + 1);
    {
        std::u32string dummy; dummy.resize(count * sizeof(std::u32string_view) / sizeof(char32_t));
        tmp.push_back(ret->Names.AllocateString(dummy));
    }
    for (const auto& str : patchedBlk)
    {
        tmp.push_back(ret->Names.AllocateString(str));
    }
    for (const auto& str : bodyPfx)
    {
        tmp.push_back(ret->Names.AllocateString(str));
    }

    const auto tmp2Space = ret->Names.GetStringView(tmp[0]);
    const auto ptr = reinterpret_cast<std::u32string_view*>(const_cast<char32_t*>(tmp2Space.data()));
    const common::span<std::u32string_view> space{ ptr, count };
    for (size_t i = 0; i < count; ++i)
    {
        space[i] = ret->Names.GetStringView(tmp[i + 1]);
    }

    ret->PatchedBlock = space.subspan(0, patchedBlk.size());
    ret->BodyPrefixes = space.subspan(patchedBlk.size(), bodyPfx.size());
    return ret;
}

std::shared_ptr<const ReplaceDepend> ReplaceDepend::Empty()
{
    static auto EMPTY = MAKE_ENABLER_SHARED(ReplaceDepend, ());
    return EMPTY;
}


XCNLExtension::XCNLExtension(XCNLContext& context) : Context(context)
{ }
XCNLExtension::~XCNLExtension() { }


struct ExtHolder
{
    std::vector<XCNLExtension::XCNLExtGen> Container;
    std::shared_mutex Mutex;
    auto LockShared() { return std::shared_lock<std::shared_mutex>(Mutex); }
    auto LockUnique() { return std::unique_lock<std::shared_mutex>(Mutex); }
};
static auto& XCNLExtGenerators() noexcept
{
    static ExtHolder Holder;
    return Holder;
}
uintptr_t XCNLExtension::RegExt(XCNLExtGen creator) noexcept
{
    auto& holder = XCNLExtGenerators();
    const auto locker = holder.LockUnique();
    holder.Container.emplace_back(creator);
    return reinterpret_cast<uintptr_t>(creator);
}


XCNLContext::XCNLContext(const common::CLikeDefines& info)
{
    for (const auto [key, val] : info)
    {
        const auto varName = common::str::to_u32string(key, Charset::UTF8);
        const xziar::nailang::LateBindVar var(varName);
        switch (val.index())
        {
        case 1: LocateArg(var, true).Set(std::get<1>(val)); break;
        case 2: LocateArg(var, true).Set(std::get<2>(val)); break;
        case 3: LocateArg(var, true).Set(std::get<3>(val)); break;
        case 4: LocateArg(var, true).Set(common::str::to_u32string(std::get<4>(val), Charset::UTF8)); break;
        case 0:
        default:
            break;
        }
    }
}
XCNLContext::~XCNLContext()
{ }

XCNLExtension* XCNLContext::FindExt(std::function<bool(const XCNLExtension*)> func) const
{
    for (const auto& ext : Extensions)
    {
        if (func(ext.get()))
            return ext.get();
    }
    return nullptr;
}

void XCNLContext::Write(std::u32string& output, const NamedText& item) const
{
    APPEND_FMT(output, U"/* Patched Block [{}] */\r\n"sv, GetID(item));
    output.append(item.Content).append(U"\r\n\r\n"sv);
}


Arg XCNLExecutor::EvaluateFunc(FuncEvalPack& func)
{
    if (func.NamePartCount() >= 2 && func.NamePart(0) == U"xcomp"sv)
    {
        if (func.NamePart(1) == U"vec"sv)
        {
            auto ret = GetRuntime().CreateGVec(func.NamePart(2), func);
            if (ret.has_value())
                return std::move(ret.value());
        }
        if (func.NamePart(1) == U"Arg"sv)
        {
            return InstanceArgCustomVar::Create(GetRuntime().ParseInstanceArg(func.NamePart(2), func));
        }
        else if (func.NamePartCount() == 2)
        {
            auto ret = GetRuntime().CommonFunc(func.NamePart(1), func);
            if (ret.has_value())
                return std::move(ret.value());
        }
    }
    for (const auto& ext : GetExtensions())
    {
        auto ret = ext->ConfigFunc(*this, func);
        if (ret)
            return std::move(ret.value());
    }
    return NailangExecutor::EvaluateFunc(func);
}


XCNLConfigurator::XCNLConfigurator() {}
XCNLConfigurator::~XCNLConfigurator() {}

bool XCNLConfigurator::HandleRawBlockMeta(const FuncCall& meta, OutputBlock& block, MetaSet&)
{
    auto& executor = GetExecutor();
    switch (const auto name = meta.FullFuncName(); common::DJBHash::HashC(name))
    {
    HashCase(name, U"xcomp.ReplaceVariable")
        block.ReplaceVar = true;
        return true;
    HashCase(name, U"xcomp.ReplaceFunction")
        block.ReplaceFunc = true;
        return true;
    HashCase(name, U"xcomp.Replace")
        block.ReplaceVar = block.ReplaceFunc = true;
        return true;
    HashCase(name, U"xcomp.PreAssign")
        executor.ThrowByArgCount(meta, 2, ArgLimits::Exact);
        executor.ThrowByArgType(meta, Expr::Type::Var, 0);
        block.PreAssignArgs.emplace_back(meta.Args[0].GetVar<Expr::Type::Var>(), meta.Args[1]);
        return true;
    HashCase(name, U"xcomp.TemplateArgs")
    {
        if (block.Type != OutputBlock::BlockType::Template)
            executor.NLRT_THROW_EX(FMTSTR(u"xcomp.TemplateArgs cannot be applied to [{}], which type is [{}]"sv,
                block.Name(), block.GetBlockTypeName()), meta, block.Block);
        for (uint32_t i = 0; i < meta.Args.size(); ++i)
        {
            if (meta.Args[i].TypeData != Expr::Type::Var)
                executor.NLRT_THROW_EX(FMTSTR(u"xcomp.TemplateArgs's arg[{}] is [{}]. not [Var]"sv,
                    i, meta.Args[i].GetTypeName()), meta, block.Block);
        }
        block.ExtraInfo = std::make_unique<TemplateBlockInfo>(meta.Args);
        return true;
    }
    default: break;
    }
    return false;
}

bool XCNLConfigurator::PrepareOutputBlock(OutputBlock& block)
{
    auto& executor = GetExecutor();
    auto frame = executor.GetFrameStack().PushFrame<OutputBlockFrame>(&executor, executor.CreateContext(), block);
    xziar::nailang::MetaSet allMetas(block.MetaFunc);
    return executor.HandleMetaFuncs(allMetas, { block.Block });
}


XCNLRawExecutor::~XCNLRawExecutor()
{ }

InstanceContext& XCNLRawExecutor::GetInstanceInfo() const
{
    auto& executor = GetExecutor();
    for (auto frame = &executor.GetFrame(); frame; frame = frame->PrevFrame)
    {
        if (const auto iframe = dynamic_cast<const InstanceFrame*>(&executor.GetFrame()); iframe)
            return *iframe->Instance;
    }
    executor.NLRT_THROW_EX(u"No InstanceFrame found."sv);
    //return *static_cast<InstanceFrame&>(executor.GetFrame()).Instance;
}

void XCNLRawExecutor::ThrowByReplacerArgCount(const std::u32string_view call, U32StrSpan args,
    const size_t count, const ArgLimits limit) const
{
    std::u32string_view prefix;
    switch (limit)
    {
    case ArgLimits::Exact:
        if (args.size() == count) return;
        prefix = U""; break;
    case ArgLimits::AtMost:
        if (args.size() <= count) return;
        prefix = U"at most"; break;
    case ArgLimits::AtLeast:
        if (args.size() >= count) return;
        prefix = U"at least"; break;
    }
    GetExecutor().NLRT_THROW_EX(FMTSTR(u"Repalcer-Func [{}] requires {} [{}] args, which gives [{}]."sv, 
        call, prefix, count, args.size()));
}

xziar::nailang::NailangFrameStack::FrameHolder<xziar::nailang::NailangRawBlockFrame> XCNLRawExecutor::PushFrame(
    const OutputBlock& block, std::shared_ptr<xziar::nailang::EvaluateContext> ctx, InstanceContext* instance)
{
    auto& executor = GetExecutor();
    if (instance)
        return executor.GetFrameStack().PushFrame<InstanceFrame>(&executor, ctx, block, instance);
    else
        return executor.PushRawBlockFrame(std::move(ctx), NailangFrame::FrameFlags::VarScope, block.Block, block.MetaFunc);
}

bool XCNLRawExecutor::HandleInstanceMeta(MetaEvalPack& meta)
{
    auto& executor = GetExecutor();
    auto& runtime = GetRuntime();
    auto& kerCtx = *dynamic_cast<const InstanceFrame*>(&executor.GetFrame())->Instance;
    for (const auto& ext : GetExtensions())
    {
        ext->InstanceMeta(executor, meta, kerCtx);
    }
    const auto& fname = meta.GetName();
    if (meta.Name->PartCount >= 2 && fname.GetRange(0, 2) == U"xcomp.Arg"sv)
    {
        if (meta.Name->PartCount == 3)
        {
            auto data = runtime.ParseInstanceArg(fname[2], meta);
            InstanceArgInfo info(data.Type, data.TexType, data.Name, data.DataType, data.Args);
            runtime.HandleInstanceArg(info, kerCtx, meta, nullptr);
        }
        else
        {
            if (meta.Params.size() != 1 || !meta.Params[0].IsCustomType<InstanceArgCustomVar>())
                runtime.NLRT_THROW_EX(FMTSTR(u"xcom.Arg requires xcomp::arg as argument, get [{}]"sv,
                    meta.TryGetOr(0, &Arg::GetTypeName, U"<empty>"sv)), meta);
            const auto& info = InstanceArgCustomVar::GetArgInfo(meta.Params[0].GetCustom());
            runtime.HandleInstanceArg(info, kerCtx, meta, &meta.Params[0]);
        }
        return true;
    }
    return false;
}

std::optional<common::str::StrVariant<char32_t>> XCNLRawExecutor::CommonReplaceFunc(const std::u32string_view name,
    const std::u32string_view call, U32StrSpan args)
{
    auto& executor = GetExecutor();
    auto& runtime = GetRuntime();
    switch (common::DJBHash::HashC(name))
    {
    HashCase(name, U"GetVecTypeName")
    {
        ThrowByReplacerArgCount(call, args, 1, ArgLimits::Exact);
        return runtime.GetVecTypeName(args[0]);
    }
    HashCase(name, U"CodeBlock")
    {
        ThrowByReplacerArgCount(call, args, 1, ArgLimits::AtLeast);
        const OutputBlock* block = nullptr;
        for (const auto& blk : runtime.XCContext.TemplateBlocks)
            if (blk.Block->Name == args[0])
            {
                block = &blk; break;
            }
        if (block)
        {
            auto ctx = runtime.ConstructEvalContext();
            if (const auto extra = static_cast<const TemplateBlockInfo*>(block->ExtraInfo.get()); extra)
            {
                ThrowByReplacerArgCount(call, args, extra->ReplaceArgs.size() + 1, ArgLimits::AtLeast);
                for (size_t i = 0; i < extra->ReplaceArgs.size(); ++i)
                {
                    auto var = executor.DecideDynamicVar(extra->ReplaceArgs[i], u"CodeBlock"sv);
                    var.Info |= xziar::nailang::LateBindVar::VarInfo::Local;
                    ctx->LocateArg(var, true).Set(args[i + 1]);
                    //constexpr NilCheck nilcheck(NilCheck::Behavior::Throw, NilCheck::Behavior::Pass);
                    //runtime.SetArg(var, {}, Arg(args[i + 1]), nilcheck);
                }
            }
            auto frame = PushFrame(*block, std::move(ctx), nullptr);
            auto output = FMTSTR(U"// template block [{}]\r\n"sv, args[0]);
            DirectOutput(*block, output);
            return output;
        }
        break;
    }
    default: break;
    }
    return {};
}

ReplaceResult XCNLRawExecutor::ExtensionReplaceFunc(std::u32string_view func, U32StrSpan args)
{
    auto& runtime = GetRuntime();
    for (const auto& ext : GetExtensions())
    {
        if (!ext) continue;
        auto ret = ext->ReplaceFunc(*this, func, args);
        const auto str = ret.GetStr();
        if (!ret && ret.CheckAllowFallback())
        {
            if (!str.empty())
                runtime.Logger.warning(FMT_STRING(u"when replace-func [{}]: {}\r\n"), func, ret.GetStr());
            continue;
        }
        if (ret)
        {
            if (const auto dep = ret.GetDepends(); dep)
            {
                auto txt = FMTSTR(u"Result for ReplaceFunc[{}] : [{}]\r\n"sv, func, str);
                if (!dep->PatchedBlock.empty())
                {
                    txt += u"Depend on PatchedBlock:\r\n";
                    for (const auto& name : dep->PatchedBlock)
                        APPEND_FMT(txt, u" - [{}]\r\n", name);
                }
                if (!dep->BodyPrefixes.empty())
                {
                    txt += u"Depend on BodyPrefix:\r\n";
                    for (const auto& name : dep->BodyPrefixes)
                        APPEND_FMT(txt, u" - [{}]\r\n", name);
                }
                runtime.Logger.verbose(txt);
            }
        }
        return ret;
    }
    return { FMTSTR(U"[{}] with [{}]args not resolved", func, args.size()), false };
}

void XCNLRawExecutor::OnReplaceOptBlock(std::u32string& output, void*, std::u32string_view cond, std::u32string_view content)
{
    auto& executor = GetExecutor();
    if (cond.back() == U';')
        cond.remove_suffix(1);
    if (cond.empty())
    {
        executor.NLRT_THROW_EX(u"Empty condition occurred when replace-opt-block"sv);
        return;
    }

    common::parser::ParserContext context(cond);
    const auto rawarg = xziar::nailang::NailangParser::ParseSingleExpr(GetRuntime().MemPool, context, ""sv, U""sv);
    Arg ret;
    if (rawarg)
    {
        ret = executor.EvaluateExpr(rawarg);
    }
    if (ret.IsEmpty())
    {
        executor.NLRT_THROW_EX(u"No value returned as condition when replace-opt-block"sv);
        return;
    }
    else if (!HAS_FIELD(ret.TypeData, Arg::Type::Boolable))
    {
        executor.NLRT_THROW_EX(u"Evaluated condition is not boolable when replace-opt-block"sv);
        return;
    }
    else if (ret.GetBool().value())
    {
        output.append(content);
    }
}

void XCNLRawExecutor::OnReplaceVariable(std::u32string& output, [[maybe_unused]] void* cookie, std::u32string_view var)
{
    auto& executor = GetExecutor();
    if (var.size() == 0)
        executor.NLRT_THROW_EX(u"replace-variable does not accept empty"sv);
    auto& runtime = GetRuntime();
    if (var[0] == U'@')
    {
        const auto str = runtime.GetVecTypeName(var.substr(1), u"replace-variable"sv);
        output.append(str);
    }
    else // '@' and '#' not allowed in var anyway
    {
        bool toVecName = false;
        if (var[0] == U'#')
            toVecName = true, var.remove_prefix(1);
        const auto var_ = executor.DecideDynamicVar(var, u"ReplaceVariable"sv);
        const auto val = runtime.LookUpArg(var_);
        if (val.IsEmpty() || val.TypeData == Arg::Type::Var)
        {
            executor.NLRT_THROW_EX(FMTSTR(u"Arg [{}] not found when replace-variable"sv, var));
            return;
        }
        if (toVecName)
        {
            if (!val.IsStr())
                executor.NLRT_THROW_EX(FMTSTR(u"Arg [{}] is not a vetype string when replace-variable"sv, var));
            const auto str = runtime.GetVecTypeName(val.GetStr().value(), u"replace-variable"sv);
            output.append(str);
        }
        else
            output.append(val.ToString().StrView());
    }
}

void XCNLRawExecutor::OnReplaceFunction(std::u32string& output, void*, std::u32string_view func, common::span<const std::u32string_view> args)
{
    auto& executor = GetExecutor();
    if (IsBeginWith(func, U"xcomp."sv))
    {
        const auto subName = func.substr(6);
        auto ret = CommonReplaceFunc(subName, func, args);
        if (ret.has_value())
        {
            output.append(ret->StrView());
            return;
        }
    }

    {
        const auto ret = ExtensionReplaceFunc(func, args);
        if (ret)
        {
            output.append(ret.GetStr());
        }
        else
        {
            executor.NLRT_THROW_EX(FMTSTR(u"replace-func [{}] with [{}]args error: {}", func, args.size(), ret.GetStr()));
        }
    }
}

void XCNLRawExecutor::OutputConditions(common::span<const xziar::nailang::FuncCall> metas, std::u32string& dst)
{
    // std::set<std::u32string_view> lateVars;
    for (const auto& meta : metas)
    {
        std::u32string_view prefix;
        if (meta.GetName() == U"If"sv)
            prefix = U"If "sv;
        else if (meta.GetName() == U"Skip"sv)
            prefix = U"Skip if "sv;
        else
            continue;
        if (meta.Args.size() == 1)
            APPEND_FMT(dst, U"    /* {:7} :  {} */\r\n"sv, prefix, xziar::nailang::Serializer::Stringify(meta.Args[0]));
    }
}

void XCNLRawExecutor::BeforeOutputBlock(const OutputBlock& block, std::u32string& dst) const
{
    APPEND_FMT(dst, U"\r\n/* From {} Block [{}] */\r\n"sv, block.GetBlockTypeName(), block.Block->Name);

    // std::set<std::u32string_view> lateVars;

    OutputConditions(block.MetaFunc, dst);
}

void XCNLRawExecutor::DirectOutput(const OutputBlock& block, std::u32string& dst)
{
    auto& executor = GetExecutor();
    auto& runtime = GetRuntime();
    Expects(runtime.CurFrame() != nullptr);
    OutputConditions(block.MetaFunc, dst);
    common::str::StrVariant<char32_t> source(block.Block->Source);
    for (const auto& [var, arg] : block.PreAssignArgs)
    {
        runtime.SetArg(var, {}, executor.EvaluateExpr(arg));
    }
    if (block.ReplaceVar || block.ReplaceFunc)
        source = ProcessOptBlock(source.StrView(), U"$$@"sv, U"@$$"sv);
    if (block.ReplaceVar)
        source = ProcessVariable(source.StrView(), U"$$!{"sv, U"}"sv);
    if (block.ReplaceFunc)
        source = ProcessFunction(source.StrView(), U"$$!"sv, U""sv);
    dst.append(source.StrView());
}

void XCNLRawExecutor::ProcessGlobal(const OutputBlock& block, std::u32string& dst)
{
    auto frame = PushFrame(block, GetExecutor().CreateContext(), nullptr);
    BeforeOutputBlock(block, dst);
    DirectOutput(block, dst);
}

void XCNLRawExecutor::ProcessStruct(const OutputBlock& block, std::u32string& dst)
{
    auto frame = PushFrame(block, GetExecutor().CreateContext(), nullptr);
    BeforeOutputBlock(block, dst);
    DirectOutput(block, dst);
}

void XCNLRawExecutor::ProcessInstance(const OutputBlock& block, std::u32string& output)
{
    Expects(block.Type == OutputBlock::BlockType::Instance);
    BeforeOutputBlock(block, output);

    auto& executor = GetExecutor();
    auto& runtime = executor.GetRuntime();
    const auto kerCtx = PrepareInstance(block);
    auto frame = PushFrame(block, executor.CreateContext(), kerCtx.get());
    for (const auto& ext : GetExtensions())
    {
        ext->BeginInstance(runtime, *kerCtx);
    }
    xziar::nailang::MetaSet allMetas(block.MetaFunc);
    const auto ret = executor.HandleMetaFuncs(allMetas, {});
    Ensures(ret);

    DirectOutput(block, kerCtx->Content);

    for (const auto& ext : GetExtensions())
    {
        ext->FinishInstance(runtime, *kerCtx);
    }

    OutputInstance(block, output);
}

void XCNLRawExecutor::HandleException(const xziar::nailang::NailangParseException& ex) const
{
    GetRuntime().Logger.error(u"{}\n", ex.Message());
    ReplaceEngine::HandleException(ex);
}
void XCNLRawExecutor::HandleException(const xziar::nailang::NailangRuntimeException& ex) const
{
    GetExecutor().HandleException(ex);
}


XCNLRuntime::XCNLRuntime(common::mlog::MiniLogger<false>& logger, std::shared_ptr<XCNLContext> evalCtx) :
    NailangRuntime(evalCtx), XCContext(*evalCtx), Logger(logger)
{ }
XCNLRuntime::~XCNLRuntime()
{ }

void XCNLRuntime::HandleException(const NailangRuntimeException& ex) const
{
    Logger.error(u"{}\n", ex.Message());
    NailangRuntime::HandleException(ex);
}

void XCNLRuntime::InnerLog(common::mlog::LogLevel level, std::u32string_view str)
{
    Logger.log(level, u"[XCNL]{}\n", str);
}

static std::u16string VecTypeErrorStr(const std::u32string_view var, const std::u16string_view reason, const std::variant<std::u16string_view, std::function<std::u16string(void)>>& exInfo)
{
    switch (exInfo.index())
    {
    case 0:
        if (const auto str = std::get<0>(exInfo); str.size() == 0)
            return FMTSTR(u"Type [{}] {}."sv, var, reason);
        else
            return FMTSTR(u"Type [{}] {} when {}."sv, var, reason, str);
    case 1:
        return FMTSTR(u"Type [{}] {}, {}."sv, var, reason, std::get<1>(exInfo)());
    default:
        return {};
    }
}

common::simd::VecDataInfo XCNLRuntime::ParseVecType(const std::u32string_view var,
    std::variant<std::u16string_view, std::function<std::u16string(void)>> extraInfo) const
{
    const auto vtype = XCContext.ParseVecType(var);
    if (!vtype)
        NLRT_THROW_EX(VecTypeErrorStr(var, u"not recognized as VecType"sv, extraInfo));
    if (!vtype.TypeSupport)
        Logger.warning(u"Potential use of unsupported type[{}] with [{}].\n", StringifyVDataType(vtype.Info), var);

    return vtype.Info;
}

std::u32string_view XCNLRuntime::GetVecTypeName(const std::u32string_view vname,
    std::variant<std::u16string_view, std::function<std::u16string(void)>> extraInfo) const
{
    const auto vtype = ParseVecType(vname, extraInfo);
    const auto str = XCContext.GetVecTypeName(vtype);
    if (str.empty())
        NLRT_THROW_EX(VecTypeErrorStr(vname, u"not supported"sv, extraInfo));
    return str;
}

constexpr auto LogLevelParser = SWITCH_PACK(Hash, 
    (U"error",   common::mlog::LogLevel::Error), 
    (U"success", common::mlog::LogLevel::Success),
    (U"warning", common::mlog::LogLevel::Warning),
    (U"info",    common::mlog::LogLevel::Info),
    (U"verbose", common::mlog::LogLevel::Verbose), 
    (U"debug",   common::mlog::LogLevel::Debug));
std::optional<common::mlog::LogLevel> ParseLogLevel(const Arg& arg) noexcept
{
    const auto str = arg.GetStr();
    if (str)
        return LogLevelParser(str.value());
    return {};
}
std::optional<Arg> XCNLRuntime::CommonFunc(const std::u32string_view name, FuncEvalPack& func)
{
    switch (common::DJBHash::HashC(name))
    {
    HashCase(name, U"GetVecTypeName")
    {
        ThrowByParamTypes<1>(func, { Arg::Type::String });
        return GetVecTypeName(func.Params[0].GetStr().value());
    }
    HashCase(name, U"Log")
    {
        ThrowIfNotFuncTarget(func, FuncInfo::Empty);
        ThrowByParamTypes<2, ArgLimits::AtLeast>(func, { Arg::Type::String, Arg::Type::String });
        const auto logLevel = ParseLogLevel(func.Params[0]);
        if (!logLevel)
            NLRT_THROW_EX(u"Arg[0] of [Log] should be LogLevel"sv, func);
        const auto fmtStr = func.Params[1].GetStr().value();
        if (func.Params.size() == 2)
            InnerLog(logLevel.value(), fmtStr);
        else
        {
            try
            {
                const auto str = FormatString(fmtStr, func.Params.subspan(2));
                InnerLog(logLevel.value(), str);
            }
            catch (const xziar::nailang::NailangFormatException& nfe)
            {
                Logger.error(u"Error when formating inner log: {}\n", nfe.Message());
            }
        }
        return Arg{};
    }
    default: break;
    }
    return {};
}
std::optional<Arg> XCNLRuntime::CreateGVec(const std::u32string_view type, FuncEvalPack& func)
{
    auto [info, least] = xcomp::ParseVDataType(type);
    if (info.Bit == 0)
        NLRT_THROW_EX(FMTSTR(u"Type [{}] not recognized"sv, type), func);
    if (info.Dim0 < 2 || info.Dim0 > 4)
        NLRT_THROW_EX(FMTSTR(u"Vec only support [2~4] as length, get [{}]."sv, info.Dim0), func);
    const auto argc = func.Args.size();
    if (argc != 0 && argc != 1 && argc != info.Dim0)
        NLRT_THROW_EX(FMTSTR(u"Vec{0} expects [0,1,{0}] args, get [{1}]."sv, info.Dim0, argc), func);

    VecDataInfo sinfo{ info.Type, info.Bit, 1, 0 };
    auto atype = FixedArray::Type::Any;
#define GENV(vtype, bit, at) \
    case static_cast<uint32_t>(VecDataInfo{VecDataInfo::DataTypes::vtype, bit, 1, 0}): atype = FixedArray::Type::at; break;
    switch (static_cast<uint32_t>(sinfo))
    {
    GENV(Unsigned, 8,  U8);
    GENV(Unsigned, 16, U16);
    GENV(Unsigned, 32, U32);
    GENV(Unsigned, 64, U64);
    GENV(Signed,   8,  I8);
    GENV(Signed,   16, I16);
    GENV(Signed,   32, I32);
    GENV(Signed,   64, I64);
    GENV(Float,    16, F16);
    GENV(Float,    32, F32);
    GENV(Float,    64, F64);
    default: return {};
    }
#undef GENV

    auto vec = GeneralVec::Create(atype, info.Dim0);
    if (argc > 0)
    {
        const auto arr = GeneralVec::ToArray(vec);
        if (argc == 1)
        {
            for (uint32_t i = 0; i < info.Dim0; ++i)
            {
                arr.Set(i, func.Params[0]);
            }
        }
        else
        {
            Expects(argc == info.Dim0);
            for (uint32_t i = 0; i < argc; ++i)
            {
                arr.Set(i, std::move(func.Params[i]));
            }
        }
    }
    return std::move(vec);
}


void XCNLRuntime::ProcessStruct(const Block&, common::span<const FuncCall>)
{

}

OutputBlock::BlockType XCNLRuntime::GetBlockType(const RawBlock& block, MetaFuncs) const noexcept
{
    switch (common::DJBHash::HashC(block.Type))
    {
    HashCase(block.Type, U"xcomp.Global")   return OutputBlock::BlockType::Global;
    HashCase(block.Type, U"xcomp.Struct")   return OutputBlock::BlockType::Struct;
    HashCase(block.Type, U"xcomp.Kernel")   return OutputBlock::BlockType::Instance;
    HashCase(block.Type, U"xcomp.Template") return OutputBlock::BlockType::Template;
    default:                                break;
    }
    return OutputBlock::BlockType::None;
}

void XCNLRuntime::HandleInstanceArg(const InstanceArgInfo&, InstanceContext&, const FuncPack&, const xziar::nailang::Arg*)
{ }

InstanceArgData XCNLRuntime::ParseInstanceArg(std::u32string_view argTypeName, FuncPack& func)
{
    std::u32string name;
    std::u32string dtype;
    size_t offset = 0;
    auto argType = InstanceArgInfo::Types::RawBuf;
    auto texType = InstanceArgInfo::TexTypes::Empty;
    switch (common::DJBHash::HashC(argTypeName))
    {
    HashCase(argTypeName, U"Buf")
    {
        ThrowByParamTypes<2, ArgLimits::AtLeast>(func, { Arg::Type::String, Arg::Type::String });
        dtype = func.Params[0].GetStr().value();
        name  = func.Params[1].GetStr().value();
        offset = 2;
        argType = InstanceArgInfo::Types::RawBuf;
    } break;
    HashCase(argTypeName, U"Typed")
    {
        ThrowByParamTypes<2, ArgLimits::AtLeast>(func, { Arg::Type::String, Arg::Type::String });
        dtype = func.Params[0].GetStr().value();
        name  = func.Params[1].GetStr().value();
        offset = 2;
        argType = InstanceArgInfo::Types::TypedBuf;
    } break;
    HashCase(argTypeName, U"Simple")
    {
        ThrowByParamTypes<2, ArgLimits::AtLeast>(func, { Arg::Type::String, Arg::Type::String });
        dtype = func.Params[0].GetStr().value();
        name  = func.Params[1].GetStr().value();
        offset = 2;
        argType = InstanceArgInfo::Types::Simple;
    } break;
    HashCase(argTypeName, U"Tex")
    {
        ThrowByParamTypes<3, ArgLimits::AtLeast>(func, { Arg::Type::String, Arg::Type::String, Arg::Type::String });
        const auto tname = func.Params[0].GetStr().value();
        dtype = func.Params[1].GetStr().value();
        name  = func.Params[2].GetStr().value();
        constexpr auto TexTypeParser = SWITCH_PACK(Hash,
            (U"tex1d",      InstanceArgInfo::TexTypes::Tex1D),
            (U"tex2d",      InstanceArgInfo::TexTypes::Tex2D),
            (U"tex3d",      InstanceArgInfo::TexTypes::Tex3D),
            (U"tex1darr",   InstanceArgInfo::TexTypes::Tex1DArray),
            (U"tex2darr",   InstanceArgInfo::TexTypes::Tex2DArray));
        if (const auto ttype = TexTypeParser(tname); !ttype)
            NLRT_THROW_EX(FMTSTR(u"Unrecognized Texture Type [{}]"sv, tname), func);
        else
            texType = ttype.value();
        offset = 3;
        argType = InstanceArgInfo::Types::Texture;
    } break;
    default:
        break;
    }
    std::vector<Arg> args;
    args.reserve(func.Args.size() - offset);
    for (size_t idx = offset; idx < func.Args.size(); ++idx)
    {
        const auto& arg = args.emplace_back(std::move(func.Params[idx]));
        ThrowByParamType(func, arg, Arg::Type::String, idx);
    }
    return { std::move(args), std::move(name), std::move(dtype), argType, texType };
}

void XCNLRuntime::BeforeFinishOutput(std::u32string&, std::u32string&)
{ }

void XCNLRuntime::ProcessConfigBlock(const Block& block, MetaFuncs metas)
{
    auto& executor = GetConfigurator().GetExecutor();
    auto frame = executor.PushBlockFrame(ConstructEvalContext(), NailangFrame::FrameFlags::FlowScope, &block, metas);
    if (!metas.empty())
    {
        Statement target(&block);
        MetaSet allMetas(metas);
        if (!executor.HandleMetaFuncs(allMetas, target))
            return;
    }
    frame->Execute();
}

void XCNLRuntime::CollectRawBlock(const RawBlock& block, MetaFuncs metas)
{
    const auto type = GetBlockType(block, metas);
    if (type == OutputBlock::BlockType::None)
        return;
    OutputBlock outBlk(&block, metas, type);
    if (!GetConfigurator().PrepareOutputBlock(outBlk))
        return;

    auto& dst = type == OutputBlock::BlockType::Template ? XCContext.TemplateBlocks : XCContext.OutputBlocks;
    dst.push_back(std::move(outBlk));
}

std::string XCNLRuntime::GenerateOutput()
{
    std::u32string prefix, output;
    auto& rawExe = GetRawExecutor();

    for (const auto& block : XCContext.OutputBlocks)
    {
        switch (block.Type)
        {
        case OutputBlock::BlockType::Instance:
            rawExe.ProcessInstance(block, output);
            break;
        case OutputBlock::BlockType::Global:
            rawExe.ProcessGlobal(block, output);
            break;
        case OutputBlock::BlockType::Struct:
            rawExe.ProcessStruct(block, output);
            break;
        default:
            Logger.warning(u"Unexpected RawBlock (Type[{}], Name[{}]) when generating output.\n", block.Block->Type, block.Block->Name); break;
            break;
        }
    }
    for (const auto& ext : XCContext.Extensions)
    {
        ext->FinishXCNL(*this);
    }

    BeforeFinishOutput(prefix, output);

    // Output patched blocks
    XCContext.WritePatchedBlock(prefix);

    return common::str::to_string(prefix + output, Charset::UTF8, Charset::UTF32);
}


class XComputeParser : xziar::nailang::NailangParser
{
    using NailangParser::NailangParser;
public:
    static void GetBlock(xziar::nailang::MemoryPool& pool, const std::u32string_view src, const std::u16string_view fname, xziar::nailang::Block& dst)
    {
        common::parser::ParserContext context(src, fname);
        XComputeParser parser(pool, context);
        parser.ParseContentIntoBlock(true, dst);
    }
};


std::shared_ptr<XCNLProgram> XCNLProgram::Create_(std::u32string source, std::u16string fname)
{
    return MAKE_ENABLER_SHARED(XCNLProgram, (std::move(source), std::move(fname)));
}

std::shared_ptr<XCNLProgram> XCNLProgram::Create(std::u32string source, std::u16string fname)
{
    return CreateBy<XComputeParser>(std::move(source), std::move(fname));
}


XCNLProgStub::XCNLProgStub(const std::shared_ptr<const XCNLProgram>& program, std::shared_ptr<XCNLContext>&& context,
    std::unique_ptr<XCNLRuntime>&& runtime) : Program(program), Context(std::move(context)), Runtime(std::move(runtime))
{
    Prepare(Runtime->Logger);
}
XCNLProgStub::~XCNLProgStub()
{ }
void XCNLProgStub::Prepare(common::mlog::MiniLogger<false>& logger)
{
    Context->Extensions.clear();
    {
        auto& holder = XCNLExtGenerators();
        const auto locker = holder.LockShared();
        for (const auto& creator : holder.Container)
        {
            auto ext = creator(logger, *Context);
            if (ext)
            {
                Context->Extensions.push_back(std::move(ext));
            }
        }
    }
    for (const auto& creator : Program->ExtraExtension)
    {
        auto ext = creator(logger, *Context);
        if (ext)
        {
            Context->Extensions.push_back(std::move(ext));
        }
    }
}

void XCNLProgStub::ExecuteBlocks(const std::u32string_view type) const
{
    for (const auto& [meta, tmp] : Program->Program)
    {
        if (tmp.TypeData != Statement::Type::Block)
            continue;
        const auto& block = *tmp.template Get<Block>();
        if (block.Type == type)
            Runtime->ProcessConfigBlock(block, meta);
    }
}
void XCNLProgStub::Prepare(common::span<const std::u32string_view> types) const
{
    for (const auto type : types)
    {
        ExecuteBlocks(type);
    }
}
void XCNLProgStub::Collect(common::span<const std::u32string_view> prefixes) const
{
    for (const auto& [meta, tmp] : Program->Program)
    {
        if (tmp.TypeData != Statement::Type::RawBlock)
            continue;
        const auto& block = *tmp.template Get<RawBlock>();
        for (const auto prefix : prefixes)
        {
            if (IsBeginWith(block.Type, prefix))
            {
                Runtime->CollectRawBlock(block, meta);
                break;
            }
        }
    }
}
void XCNLProgStub::PostAct(common::span<const std::u32string_view> types) const
{
    for (const auto type : types)
    {
        ExecuteBlocks(type);
    }
}


enum class GVecRefFlags : uint8_t
{
    Empty = 0x0,
    Vec2 = 0x2, Vec3 = 0x3, Vec4 = 0x4,
    ReadOnly = 0x80
};
MAKE_ENUM_BITFIELD(GVecRefFlags)
static GeneralVecRef GVecRefHandler;
static GeneralVec GVecHandler;

CustomVar GeneralVecRef::Create(FixedArray arr)
{
    auto flag = GVecRefFlags::Empty;
    switch (arr.Length)
    {
    case 2: flag = GVecRefFlags::Vec2; break;
    case 3: flag = GVecRefFlags::Vec3; break;
    case 4: flag = GVecRefFlags::Vec4; break;
    default:
        COMMON_THROWEX(common::BaseException, FMTSTR(u"Vec ref only support [2~4] as length, get [{}].", arr.Length));
    }
    if (arr.IsReadOnly)
        flag |= GVecRefFlags::ReadOnly;
    return { &GVecRefHandler, arr.DataPtr, common::enum_cast(flag), common::enum_cast(arr.ElementType) };
}
FixedArray GeneralVecRef::ToArray(const xziar::nailang::CustomVar& var) noexcept
{
    Expects(var.Host == &GVecRefHandler || var.Host == &GVecHandler);
    const auto flag = static_cast<GVecRefFlags>(var.Meta1);
    const bool isReadOnly = HAS_FIELD(flag, GVecRefFlags::ReadOnly);
    const uint64_t length = (var.Meta1 & 0xfu);
    return { var.Meta0, length, static_cast<FixedArray::Type>(var.Meta2), isReadOnly };
}
size_t GeneralVecRef::ToIndex(const CustomVar& var, const FixedArray& arr, std::u32string_view field)
{
    size_t tidx = SIZE_MAX;
    switch (common::DJBHash::HashC(field))
    {
    HashCase(field, U"x") tidx = 0; break;
    HashCase(field, U"y") tidx = 1; break;
    HashCase(field, U"z") tidx = 2; break;
    HashCase(field, U"w") tidx = 3; break;
    default: break;
    }
    if (tidx == SIZE_MAX)
        return {};
    if (tidx > arr.Length)
    {
        COMMON_THROW(NailangRuntimeException, FMTSTR(u"Cannot access [{}] on a vector of length[{}]"sv,
            field, arr.Length), var);
    }
    return tidx;
}

ArgLocator GeneralVecRef::HandleQuery(CustomVar& var, SubQuery subq, NailangExecutor& executor)
{
    const auto arr = ToArray(var);
    const auto [type, query] = subq[0];
    size_t tidx = SIZE_MAX;
    switch (type)
    {
    case SubQuery::QueryType::Index:
        tidx = xziar::nailang::NailangHelper::BiDirIndexCheck(static_cast<size_t>(arr.Length),
            executor.EvaluateExpr(query), &query);
        break;
    case SubQuery::QueryType::Sub:
    {
        const auto field = query.GetVar<Expr::Type::Str>();
        if (field == U"Length"sv)
            return { arr.Length, 1 };
        tidx = ToIndex(var, arr, field);
    } break;
    default:
        return {};
    }
    return arr.Access(tidx);
}
bool GeneralVecRef::HandleAssign(CustomVar& var, Arg arg)
{
    const auto arr = ToArray(var);
    if (arr.IsReadOnly)
        COMMON_THROW(NailangRuntimeException, FMTSTR(u"Cannot modify a constant {}"sv, GetExactType(arr)), var);
    if (arg.IsCustomType<xcomp::GeneralVecRef>())
    {
        const auto other = ToArray(arg.GetCustom());
        if (arr.Length != other.Length)
            COMMON_THROW(NailangRuntimeException, FMTSTR(u"vecref is assigned with different length, expect [{}] get [{}, {}]",
                arr.Length, other.GetElementTypeName(), other.Length), var);
        for (size_t idx = 0; idx < arr.Length; ++idx)
            arr.Set(idx, other.Get(idx));
        return true;
    }
    if (arg.IsNumber())
    {
        for (size_t idx = 0; idx < arr.Length; ++idx)
            arr.Set(idx, arg);
        return true;
    }
    COMMON_THROW(NailangRuntimeException, FMTSTR(u"vecref can only be set with vecref or single num, get [{}]", arg.GetTypeName()), var);
    return false;
}
CompareResult GeneralVecRef::CompareSameClass(const CustomVar& var, const CustomVar& other)
{
    const auto lhs = ToArray(var), rhs = ToArray(other);
    if (lhs.ElementType != rhs.ElementType || lhs.Length != rhs.Length)
        return {};
    const auto getter = NativeWrapper::GetGetter(lhs.ElementType);
    const auto ldata = static_cast<uintptr_t>(lhs.DataPtr), rdata = static_cast<uintptr_t>(rhs.DataPtr);
    for (size_t idx = 0; idx < lhs.Length; ++idx)
    {
        auto lval = getter(ldata, idx), rval = getter(rdata, idx);
        const auto res = lval.HandleBinary(xziar::nailang::EmbedOps::Equal, rval);
        if (res.IsEmpty())
            return {};
        if (!res.GetBool().value())
            return { CompareResultCore::NotEqual | CompareResultCore::Equality };
    }
    return { CompareResultCore::Equal | CompareResultCore::Equality };
}
common::str::StrVariant<char32_t> GeneralVecRef::ToString(const CustomVar& var) noexcept
{
    return GetExactType(ToArray(var));
}
std::u32string GeneralVecRef::GetExactType(const xziar::nailang::FixedArray& arr) noexcept
{
    return FMTSTR(U"xcomp::vecref<{},{}>"sv, arr.GetElementTypeName(), arr.Length);
}
std::u32string_view GeneralVecRef::GetTypeName() noexcept
{
    return U"xcomp::vecref"sv;
}


template<typename T>
struct COMMON_EMPTY_BASES VecArray : public common::FixedLenRefHolder<VecArray<T>, T>
{
    common::span<T> Data;
    VecArray(size_t size)
    {
        Data = common::FixedLenRefHolder<VecArray<T>, T>::Allocate(size);
        if (Data.size() > 0)
        {
            common::ZeroRegion(Data.data(), sizeof(T) * Data.size());
        }
    }
    VecArray(const VecArray<T>& other) noexcept : Data(other.Data)
    {
        this->Increase();
    }
    VecArray(VecArray<T>&& other) noexcept : Data(other.Data)
    {
        other.Data = {};
    }
    ~VecArray()
    {
        this->Decrease();
    }
    [[nodiscard]] forceinline uintptr_t GetDataPtr() const noexcept
    {
        return reinterpret_cast<uintptr_t>(Data.data());
    }
    forceinline void Destruct() noexcept { }
    using common::FixedLenRefHolder<VecArray<T>, T>::Increase;
    using common::FixedLenRefHolder<VecArray<T>, T>::Decrease;
};
CustomVar GeneralVec::Create(FixedArray::Type type, size_t len)
{
    auto flag = GVecRefFlags::Empty;
    switch (len)
    {
    case 2: flag = GVecRefFlags::Vec2; break;
    case 3: flag = GVecRefFlags::Vec3; break;
    case 4: flag = GVecRefFlags::Vec4; break;
    default:
        COMMON_THROWEX(common::BaseException, FMTSTR(u"Vec only support [2~4] as length, get [{}].", len));
    }
    uint64_t ptr = 0;
#define RET(tenum, type) case FixedArray::Type::tenum: { VecArray<type> vec(len); vec.Increase(); ptr = vec.GetDataPtr(); break; }
    switch (type)
    {
    RET(U8,  uint8_t)
    RET(I8,  int8_t)
    RET(U16, uint16_t)
    RET(I16, int16_t)
    RET(F16, half_float::half)
    RET(U32, uint32_t)
    RET(I32, int32_t)
    RET(F32, float)
    RET(U64, uint64_t)
    RET(I64, int64_t)
    RET(F64, double)
    default: break;
    }
#undef RET
    return { &GVecHandler, ptr, common::enum_cast(flag), common::enum_cast(type) };
}
template<typename F>
static void CallOnVecArray(const CustomVar& var, F&& func) noexcept
{
    const auto type = static_cast<FixedArray::Type>(var.Meta2);
    const uint8_t length = (var.Meta1 & 0xfu);
#define COVA(tenum, type)                                                       \
    case FixedArray::Type::tenum:                                               \
    {                                                                           \
        static_assert(sizeof(VecArray<type>) == sizeof(common::span<type>));    \
        common::span<type> sp(reinterpret_cast<type*>(var.Meta0), length);      \
        auto& vec = *reinterpret_cast<VecArray<type>*>(&sp);                    \
        func(vec);                                                              \
        break;                                                                  \
    }

    switch (type)
    {
    COVA(U8,  uint8_t)
    COVA(I8,  int8_t)
    COVA(U16, uint16_t)
    COVA(I16, int16_t)
    COVA(F16, half_float::half)
    COVA(U32, uint32_t)
    COVA(I32, int32_t)
    COVA(F32, float)
    COVA(U64, uint64_t)
    COVA(I64, int64_t)
    COVA(F64, double)
    default: break;
    }
#undef COVA
}
void GeneralVec::IncreaseRef(CustomVar& var) noexcept
{
    CallOnVecArray(var, [](auto& vec) { vec.Increase(); });
}
void GeneralVec::DecreaseRef(CustomVar& var) noexcept
{
    CallOnVecArray(var, [&](auto& vec) 
        { 
            if (vec.Decrease())
                var.Meta0 = 0;
        });
}
common::str::StrVariant<char32_t> GeneralVec::ToString(const CustomVar& var) noexcept
{
    const auto arr = ToArray(var);
    return FMTSTR(U"xcomp::vec<{},{}>"sv, arr.GetElementTypeName(), arr.Length);
}
std::u32string_view GeneralVec::GetTypeName() noexcept
{
    return U"xcomp::vec"sv;
}


}