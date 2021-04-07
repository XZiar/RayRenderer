#include "NailangPch.h"
#include "NailangAutoVar.h"
#include "NailangRuntime.h"


namespace xziar::nailang
{
using namespace std::string_view_literals;


AutoVarHandlerBase::Accessor::Accessor() noexcept :
    AutoMember{}, IsSimple(false), IsConst(true)
{ }
AutoVarHandlerBase::Accessor::Accessor(Accessor&& other) noexcept : 
    AutoMember{}, IsSimple(false), IsConst(other.IsConst)
{
    if (other.IsSimple)
    {
        Release();
        IsSimple = true;
        new (&SimpleMember) TSimp(std::move(other.SimpleMember));
        //this->SimpleMember = std::move(other.SimpleMember);
    }
    else
    {
        AutoMember = std::move(other.AutoMember);
    }
}
void AutoVarHandlerBase::Accessor::SetCustom(std::function<CustomVar(void*)> accessor) noexcept
{
    Release();
    IsSimple = false;
    IsConst = true;
    new (&AutoMember) TAuto(std::move(accessor));
    //this->AutoMember = std::move(accessor);
}
void AutoVarHandlerBase::Accessor::SetGetSet(std::function<Arg(void*)> getter, std::function<bool(void*, Arg)> setter) noexcept
{
    Release();
    IsSimple = true;
    IsConst = !(bool)setter;
    new (&SimpleMember) TSimp(std::move(getter), std::move(setter));
    //this->SimpleMember = std::make_pair(std::move(getter), std::move(setter));
}
void AutoVarHandlerBase::Accessor::Release() noexcept
{
    if (IsSimple)
    {
        std::destroy_at(&SimpleMember);
        //SimpleMember.~TSimp();
    }
    else
    {
        std::destroy_at(&AutoMember);
        //AutoMember.~TAuto();
    }
}
AutoVarHandlerBase::Accessor::~Accessor()
{
    Release();
}
AutoVarHandlerBase::AccessorBuilder& AutoVarHandlerBase::AccessorBuilder::SetConst(bool isConst)
{
    if (!isConst && Host.IsSimple && !Host.SimpleMember.second)
        COMMON_THROW(common::BaseException, u"No setter provided");
    Host.IsConst = isConst;
    return *this;
}


AutoVarHandlerBase::AutoVarHandlerBase(std::u32string_view typeName, size_t typeSize) : TypeName(typeName), TypeSize(typeSize)
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

ArgLocator AutoVarHandlerBase::HandleQuery(CustomVar& var, SubQuery subq, NailangExecutor& executor)
{
    Expects(subq.Size() > 0);
    const bool nonConst = HAS_FIELD(static_cast<VarFlags>(var.Meta2), VarFlags::NonConst);
    const auto [type, query] = subq[0];
    if (var.Meta1 < UINT32_MAX) // is array
    {
        switch (type)
        {
        case SubQuery::QueryType::Index:
        {
            const auto idx  = executor.EvaluateExpr(query);
            size_t tidx = 0;
            if (ExtendIndexer)
            {
                const auto idx_ = ExtendIndexer(reinterpret_cast<void*>(var.Meta0), var.Meta1, idx);
                if (idx_.has_value())
                    tidx = *idx_;
                else
                    COMMON_THROWEX(NailangRuntimeException,
                        FMTSTR(u"cannot find element for index [{}] in array of [{}]"sv, idx.ToString().StrView(), var.Meta1), query);
            }
            else
            {
                tidx = NailangHelper::BiDirIndexCheck(var.Meta1, idx, &query);
            }
            const auto ptr  = static_cast<uintptr_t>(var.Meta0) + tidx * TypeSize;
            const auto flag = nonConst ? ArgLocator::LocateFlags::ReadWrite : ArgLocator::LocateFlags::ReadOnly;
            return { CustomVar{ var.Host, ptr, UINT32_MAX, var.Meta2 }, 1, flag };
        }
        case SubQuery::QueryType::Sub:
        {
            auto field = query.GetVar<Expr::Type::Str>();
            if (field == U"Length")
                return { uint64_t(var.Meta1), 1 };
        } break;
        default: break;
        }
    }
    else
    {
        switch (type)
        {
        case SubQuery::QueryType::Index:
        {
            const auto idx = executor.EvaluateExpr(query);
            auto result = IndexerGetter(var, idx, query);
            if (!result.IsEmpty())
                return { std::move(result), 1u };
        } break;
        case SubQuery::QueryType::Sub:
        {
            auto field = query.GetVar<Expr::Type::Str>();
            if (const auto accessor = FindMember(field); accessor)
            {
                const auto ptr = reinterpret_cast<void*>(var.Meta0);
                const auto isConst = !nonConst || accessor->IsConst;
                if (accessor->IsSimple)
                    return { [=]() { return accessor->SimpleMember.first(ptr); },
                        isConst ? ArgLocator::Setter{} : [=](Arg val) { return accessor->SimpleMember.second(ptr, std::move(val)); },
                        1 };
                else
                    return { accessor->AutoMember(ptr), 1, isConst ? ArgLocator::LocateFlags::ReadOnly : ArgLocator::LocateFlags::ReadWrite };
            }
        } break;
        default: break;
        }
    }
    return {};
}

bool AutoVarHandlerBase::HandleAssign(CustomVar& var, Arg val)
{
    if (var.Meta1 < UINT32_MAX) // is array
        return false;
    if (!Assigner)
        return false;
    const auto ptr = reinterpret_cast<void*>(var.Meta0);
    Assigner(ptr, std::move(val));
    return true;
}

std::u32string_view AutoVarHandlerBase::GetTypeName() noexcept
{
    return TypeName;
}


}
