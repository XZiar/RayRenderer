#include "NailangPch.h"

namespace xziar::nailang
{
using namespace std::string_view_literals;


//std::pair<size_t, size_t> MemoryPool::Usage() const noexcept
//{
//    size_t used = 0, unused = 0;
//    for (const auto& trunk : Trunks)
//    {
//        used += trunk.Offset, unused += trunk.Avaliable;
//    }
//    return { used, used + unused };
//}


std::vector<PartedName::PartType> PartedName::GetParts(std::u32string_view name)
{
    std::vector<PartType> parts;
    for (const auto piece : common::str::SplitStream(name, U'.', true))
    {
        const auto offset = piece.data() - name.data();
        if (offset > UINT16_MAX)
            COMMON_THROW(NailangPartedNameException, u"Name too long"sv, name, piece);
        if (piece.size() > UINT16_MAX)
            COMMON_THROW(NailangPartedNameException, u"Name part too long"sv, name, piece);
        if (piece.size() == 0)
            COMMON_THROW(NailangPartedNameException, u"Empty name part"sv, name, piece);
        parts.emplace_back(static_cast<uint16_t>(offset), static_cast<uint16_t>(piece.size()));
    }
    return parts;
}
PartedName* PartedName::Create(MemoryPool& pool, std::u32string_view name, uint16_t exinfo)
{
    Expects(name.size() > 0);
    const auto parts = GetParts(name);
    if (parts.size() == 1)
    {
        const auto space = pool.Alloc<PartedName>();
        const auto ptr = reinterpret_cast<PartedName*>(space.data());
        new (ptr) PartedName(name, parts, exinfo);
        return ptr;
    }
    else
    {
        const auto partSize = parts.size() * sizeof(PartType);
        const auto space = pool.Alloc(sizeof(PartedName) + partSize, alignof(PartedName));
        const auto ptr = reinterpret_cast<PartedName*>(space.data());
        new (ptr) PartedName(name, parts, exinfo);
        memcpy_s(ptr + 1, partSize, parts.data(), partSize);
        return ptr;
    }
}


static constexpr PartedName::PartType DummyPart[1] = { {(uint16_t)0, (uint16_t)0} };
TempPartedNameBase::TempPartedNameBase(std::u32string_view name, common::span<const PartedName::PartType> parts, uint16_t info)
    : Var(name, parts, info)
{
    Expects(parts.size() > 0);
    constexpr auto SizeP = sizeof(PartedName::PartType);
    const auto partSize = parts.size() * SizeP;
    if (parts.size() > 4)
    {
        const auto ptr = reinterpret_cast<PartedName*>(malloc(sizeof(PartedName) + partSize));
        Extra.Ptr = ptr;
        new (ptr) PartedName(name, parts, info);
        memcpy_s(ptr + 1, partSize, parts.data(), partSize);
    }
    else if (parts.size() > 1)
    {
        memcpy_s(Extra.Parts, partSize, parts.data(), partSize);
    }
}
TempPartedNameBase::TempPartedNameBase(const PartedName* var) noexcept : Var(var->FullName(), DummyPart, var->ExternInfo)
{
    Extra.Ptr = var;
}
TempPartedNameBase::TempPartedNameBase(TempPartedNameBase&& other) noexcept :
    Var(other.Var.FullName(), DummyPart, other.Var.ExternInfo), Extra(other.Extra)
{
    Var.PartCount = other.Var.PartCount;
    other.Var.Ptr = nullptr;
    other.Var.PartCount = 0;
    other.Extra.Ptr = nullptr;
}
TempPartedNameBase::~TempPartedNameBase()
{
    if (Var.PartCount > 4)
        free(const_cast<PartedName*>(Extra.Ptr));
}

TempPartedNameBase TempPartedNameBase::Copy() const noexcept
{
    common::span<const PartedName::PartType> parts;
    if (Var.PartCount > 4 || Var.PartCount == 0)
    {
        parts = { reinterpret_cast<const PartedName::PartType*>(Extra.Ptr + 1), Extra.Ptr->PartCount };
    }
    else
    {
        parts = { Extra.Parts, Var.PartCount };
    }
    return TempPartedNameBase(Var.FullName(), parts, Var.ExternInfo);
}


}
