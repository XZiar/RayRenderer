#include "NailangPch.h"
#include "NailangAutoVar.h"
#include "NailangRuntime.h"


namespace xziar::nailang
{
using namespace std::string_view_literals;


AutoVarHandlerBase::Accessor::Accessor() noexcept :
    Dummy{}, TypeData(Type::Empty), IsConst(true)
{ }
AutoVarHandlerBase::Accessor::Accessor(Accessor&& other) noexcept : 
    Dummy{}, TypeData(other.TypeData), IsConst(other.IsConst)
{
    other.TypeData = Type::Empty;
    switch (TypeData)
    {
    case Type::Auto:
        new (&AutoMember) TAuto(std::move(other.AutoMember));
        return;
    case Type::Proxy:
    case Type::Direct:
        new (&SimpleMember) TSimp(std::move(other.SimpleMember));
        return;
    default:
        return;
    }
}
void AutoVarHandlerBase::Accessor::SetCustom(std::function<CustomVar(void*)> accessor) noexcept
{
    Release();
    TypeData = Type::Auto;
    IsConst = true;
    new (&AutoMember) TAuto(std::move(accessor));
}
void AutoVarHandlerBase::Accessor::SetProxy (std::function<Arg(void*)> getset, bool isConst) noexcept
{
    Release();
    TypeData = Type::Proxy;
    IsConst = isConst;
    new (&SimpleMember) TSimp(std::move(getset));
}
void AutoVarHandlerBase::Accessor::SetDirect(std::function<Arg(void*)> getter) noexcept
{
    Release();
    TypeData = Type::Proxy;
    IsConst = true;
    new (&SimpleMember) TSimp(std::move(getter));
}
void AutoVarHandlerBase::Accessor::Release() noexcept
{
    switch (TypeData)
    {
    case Type::Empty: 
        return;
    case Type::Auto:  
        std::destroy_at(&SimpleMember); return;
    case Type::Proxy:
    case Type::Direct:
        std::destroy_at(&AutoMember); return;
    default:
        return;
    }
}
AutoVarHandlerBase::Accessor::~Accessor()
{
    Release();
}
AutoVarHandlerBase::AccessorBuilder& AutoVarHandlerBase::AccessorBuilder::SetConst(bool isConst)
{
    if (!isConst && (Host.TypeData == Accessor::Type::Direct || Host.TypeData == Accessor::Type::Empty))
        COMMON_THROW(common::BaseException, u"No setter provided");
    Host.IsConst = isConst;
    return *this;
}


AutoVarHandlerBase::AutoVarHandlerBase(std::u32string_view typeName) : TypeName(typeName)
{ }
AutoVarHandlerBase::~AutoVarHandlerBase() { }

AutoVarHandlerBase::Accessor* AutoVarHandlerBase::FindMember(std::u32string_view name, bool create)
{
    const common::str::HashedStrView hsv(name);
    for (auto& [pos, acc] : MemberList)
    {
        if (NamePool.GetHashedStr(pos) == hsv)
            return &acc;
    }
    if (create)
    {
        const auto piece = NamePool.AllocateString(name);
        return &MemberList.emplace_back(piece, Accessor{}).second;
    }
    return nullptr;
}

Arg AutoVarHandlerBase::HandleQuery(CustomVar& var, SubQuery& subq, NailangExecutor& executor)
{
    Expects(subq.Size() > 0);
    const bool nonConst = HAS_FIELD(static_cast<VarFlags>(var.Meta2), VarFlags::NonConst);
    const auto [type, query] = subq[0];
    switch (type)
    {
    case SubQuery::QueryType::Index:
    {
        const auto idx = executor.EvaluateExpr(query);
        auto result = IndexerGetter(var, idx, query);
        if (!result.IsEmpty())
        {
            subq.Consume();
            return result;
        }
    } break;
    case SubQuery::QueryType::Sub:
    {
        auto field = query.GetVar<Expr::Type::Str>();
        if (const auto accessor = FindMember(field); accessor)
        {
            const auto ptr = reinterpret_cast<void*>(var.Meta0);
            const auto isConst = !nonConst || accessor->IsConst;
            subq.Consume();
            switch (accessor->TypeData)
            {
            case Accessor::Type::Auto:
                return { accessor->AutoMember(ptr), isConst };
            case Accessor::Type::Empty:
                return {};
            default:
                return accessor->SimpleMember(ptr);
            }
        }
    } break;
    default: break;
    }
    return {};
}

bool AutoVarHandlerBase::HandleAssign(CustomVar& var, Arg val)
{
    if (!Assigner)
        return false;
    const auto ptr = reinterpret_cast<void*>(var.Meta0);
    Assigner(ptr, std::move(val));
    return true;
}

std::u32string_view AutoVarHandlerBase::GetTypeName(const CustomVar&) noexcept
{
    return TypeName;
}


AutoVarHandlerBase::AutoVarArrayHandler::AutoVarArrayHandler(AutoVarHandlerBase& parent) : Parent(parent), TypeName(Parent.TypeName + U"[]")
{ }
AutoVarHandlerBase::AutoVarArrayHandler::~AutoVarArrayHandler()
{ }
Arg AutoVarHandlerBase::AutoVarArrayHandler::HandleQuery(CustomVar& var, SubQuery& subq, NailangExecutor& executor)
{
    Expects(subq.Size() > 0);
    const bool nonConst = HAS_FIELD(static_cast<VarFlags>(var.Meta2), VarFlags::NonConst);
    const auto [type, query] = subq[0];
    switch (type)
    {
    case SubQuery::QueryType::Index:
    {
        const auto idx = executor.EvaluateExpr(query);
        std::optional<size_t> tidx;
        if (ExtendIndexer)
            tidx = ExtendIndexer(reinterpret_cast<void*>(var.Meta0), var.Meta1, idx);
        if (tidx.has_value())
        {
            if (tidx.value() == SIZE_MAX)
                COMMON_THROWEX(NailangRuntimeException,
                    FMTSTR(u"cannot find element for index [{}] in array of [{}]"sv, idx.ToString().StrView(), var.Meta1), query);
        }
        else
        {
            tidx = NailangHelper::BiDirIndexCheck(var.Meta1, idx, &query);
        }
        const auto ptr = OffsetPtr(static_cast<uintptr_t>(var.Meta0), tidx.value());
        subq.Consume();
        return { CustomVar{ &Parent, ptr, 0, var.Meta2 }, !nonConst };
    }
    case SubQuery::QueryType::Sub:
    {
        auto field = query.GetVar<Expr::Type::Str>();
        if (field == U"Length")
        {
            subq.Consume();
            return uint64_t(var.Meta1);
        }
    } break;
    default: break;
    }
    return {};
}

std::u32string_view AutoVarHandlerBase::AutoVarArrayHandler::GetTypeName(const CustomVar&) noexcept
{
    return TypeName;
}


}
