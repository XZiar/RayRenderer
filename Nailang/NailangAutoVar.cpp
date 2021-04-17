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

Arg AutoVarHandlerBase::HandleSubFields(const CustomVar& var, SubQuery<const Expr>& subfields)
{
    Expects(subfields.Size() > 0);
    const bool nonConst = HAS_FIELD(static_cast<VarFlags>(var.Meta2), VarFlags::NonConst);
    auto field = subfields[0].GetVar<Expr::Type::Str>();
    if (const auto accessor = FindMember(field); accessor)
    {
        const auto ptr = reinterpret_cast<void*>(var.Meta0);
        const auto isConst = !nonConst || accessor->IsConst;
        subfields.Consume();
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
    return {};
}
Arg AutoVarHandlerBase::HandleIndexes(const CustomVar& var, SubQuery<Arg>& indexes)
{
    Expects(indexes.Size() > 0);
    const bool nonConst = HAS_FIELD(static_cast<VarFlags>(var.Meta2), VarFlags::NonConst);
    auto result = IndexerGetter(var, indexes[0], indexes.GetRaw(0));
    if (!result.IsEmpty())
    {
        indexes.Consume();
        if ((result.IsCustom() || result.IsArray()) && !nonConst)
            result.TypeData |= Arg::Type::ConstBit;
        return result;
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

Arg AutoVarHandlerBase::AutoVarArrayHandler::HandleSubFields(const CustomVar & var, SubQuery<const Expr>& subfields)
{
    Expects(subfields.Size() > 0);
    auto field = subfields[0].GetVar<Expr::Type::Str>();
    if (field == U"Length")
    {
        subfields.Consume();
        return uint64_t(var.Meta1);
    }
    return {};
}
Arg AutoVarHandlerBase::AutoVarArrayHandler::HandleIndexes(const CustomVar & var, SubQuery<Arg>& indexes)
{
    Expects(indexes.Size() > 0);
    const bool nonConst = HAS_FIELD(static_cast<VarFlags>(var.Meta2), VarFlags::NonConst);
    const auto& idxval = indexes[0];
    std::optional<size_t> tidx;
    if (ExtendIndexer)
        tidx = ExtendIndexer(reinterpret_cast<void*>(var.Meta0), var.Meta1, idxval);
    if (tidx.has_value())
    {
        if (tidx.value() == SIZE_MAX)
            COMMON_THROWEX(NailangRuntimeException,
                FMTSTR(u"cannot find element for index [{}] in array of [{}]"sv, idxval.ToString().StrView(), var.Meta1), indexes.GetRaw(0));
    }
    else
    {
        tidx = NailangHelper::BiDirIndexCheck(var.Meta1, idxval, &indexes.GetRaw(0));
    }
    const auto ptr = OffsetPtr(static_cast<uintptr_t>(var.Meta0), tidx.value());
    indexes.Consume();
    return { CustomVar{ &Parent, ptr, 0, var.Meta2 }, !nonConst };
}

std::u32string_view AutoVarHandlerBase::AutoVarArrayHandler::GetTypeName(const CustomVar&) noexcept
{
    return TypeName;
}


}
