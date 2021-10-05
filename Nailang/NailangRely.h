#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef NAILANG_EXPORT
#   define NAILANGAPI _declspec(dllexport)
# else
#   define NAILANGAPI _declspec(dllimport)
# endif
#else
# define NAILANGAPI [[gnu::visibility("default")]]
#endif


#include "common/CommonRely.hpp"
#include "common/TrunckedContainer.hpp"
#include "common/EnumEx.hpp"
#include "common/StrBase.hpp"
#include "SystemCommon/Exceptions.h"
#include "common/StringLinq.hpp"
#include "common/StringPool.hpp"
#include "common/SharedString.hpp"
#include "common/EasyIterator.hpp"
#include "common/STLEx.hpp"
#ifndef HALF_ENABLE_F16C_INTRINSICS
#   define HALF_ENABLE_F16C_INTRINSICS __F16C__
#endif
#include "3rdParty/half/half.hpp"
#include <vector>
#include <variant>
#include <string>
#include <string_view>
#include <cstdio>
#include <optional>
#include <memory>


namespace xziar::nailang
{


class MemoryPool : protected common::container::TrunckedContainer<std::byte>
{
public:
    MemoryPool(const size_t trunkSize = 2 * 1024 * 1024) : common::container::TrunckedContainer<std::byte>(trunkSize, 64) { }

    [[nodiscard]] forceinline common::span<std::byte> Alloc(const size_t size, const size_t align = 64)
    {
        return common::container::TrunckedContainer<std::byte>::Alloc(size, align);
    }
    [[nodiscard]] std::pair<size_t, size_t> Usage() const noexcept
    {
        size_t used = 0, unused = 0;
        for (const auto& trunk : Trunks)
        {
            used += trunk.Offset, unused += trunk.Avaliable;
        }
        return { used, used + unused };
    }

    template<typename T>
    [[nodiscard]] forceinline common::span<std::byte> Alloc()
    {
        return Alloc(sizeof(T), alignof(T));
    }

    template<typename T, typename... Args>
    [[nodiscard]] forceinline T* Create(Args&&... args)
    {
        const auto space = Alloc<T>();
        new (space.data()) T(std::forward<Args>(args)...);
        return reinterpret_cast<T*>(space.data());
    }
    template<typename C>
    [[nodiscard]] forceinline auto CreateArray(C&& container)
    {
        const auto data = common::to_span(container);
        using T = std::remove_const_t<typename decltype(data)::element_type>;

        if (data.size() == 0) return common::span<T>{};
        const auto space = Alloc(sizeof(T) * data.size(), alignof(T));
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            memcpy_s(space.data(), space.size(), data.data(), sizeof(T) * data.size());
        }
        else
        {
            for (size_t i = 0; i < static_cast<size_t>(data.size()); ++i)
                new (space.data() + sizeof(T) * i) T(data[i]);
        }
        return common::span<T>(reinterpret_cast<T*>(space.data()), data.size());
    }
};


class TempPartedNameBase;
struct PartedName
{
    friend class TempPartedNameBase;
    using value_type = char32_t;
    using PartType = std::pair<uint16_t, uint16_t>;
    [[nodiscard]] constexpr bool operator==(const PartedName& other) const noexcept
    {
        if (Length != other.Length)
            return false;
        if (Ptr == other.Ptr)
            return true;
        return std::char_traits<char32_t>::compare(Ptr, other.Ptr, Length) == 0;
    }
    [[nodiscard]] constexpr bool operator==(const std::u32string_view other) const noexcept
    {
        return other == FullName();
    }
    [[nodiscard]] constexpr std::u32string_view FullName() const noexcept
    {
        return { Ptr, Length };
    }
    [[nodiscard]] std::u32string_view operator[](size_t index) const noexcept
    {
        return GetPart(index);
    }
    [[nodiscard]] std::u32string_view GetPart(size_t index) const noexcept
    {
        if (index >= PartCount)
            return {};
        if (PartCount == 1)
            return FullName();
        const auto parts = reinterpret_cast<const PartType*>(this + 1);
        const auto [offset, len] = parts[index];
        return { Ptr + offset, len };
    }
    [[nodiscard]] std::u32string_view GetRest(const uint32_t index) const noexcept
    {
        if (index >= PartCount)
            return {};
        if (index == 0)
            return FullName();
        const auto parts = reinterpret_cast<const PartType*>(this + 1);
        const auto offset = parts[index].first;
        return { Ptr + offset, Length - offset };
    }
    [[nodiscard]] std::u32string_view GetRange(const uint32_t from, const uint32_t to) const noexcept
    {
        Expects(to > from);
        if (to > PartCount)
            return {};
        if (to == PartCount)
            return GetRest(from);
        /*if (PartCount == 1)
            return FullName();*/
        const auto parts = reinterpret_cast<const PartType*>(this + 1);
        const uint32_t idxF = parts[from].first, idxT = parts[to].first - 1;
        return { Ptr + idxF, static_cast<size_t>(idxT - idxF) };
    }
    [[nodiscard]] constexpr auto Parts() const noexcept
    {
        return common::linq::FromRange<uint32_t>(0u, PartCount)
            .Select([self = this](uint32_t idx) { return self->GetPart(idx); });
    }
    NAILANGAPI static std::vector<PartType> GetParts(std::u32string_view name);
    NAILANGAPI static PartedName* Create(MemoryPool& pool, std::u32string_view name, uint16_t exinfo = 0);
public:
    const char32_t* Ptr;
    uint32_t Length;
    uint16_t PartCount;
protected:
    uint16_t ExternInfo;
private:
    constexpr PartedName(std::u32string_view name, common::span<const PartType> parts, uint16_t exinfo) noexcept :
        Ptr(name.data()), Length(gsl::narrow_cast<uint32_t>(name.size())),
        PartCount(gsl::narrow_cast<uint16_t>(parts.size())),
        ExternInfo(exinfo)
    { }
    constexpr PartedName(PartedName&& other) noexcept = default;
    PartedName& operator= (PartedName&&) noexcept = delete;
    COMMON_NO_COPY(PartedName)
};

class TempPartedNameBase
{
protected:
    PartedName Var;
    union Tail
    {
        const PartedName* Ptr;
        PartedName::PartType Parts[4];
        constexpr Tail() noexcept : Ptr(nullptr) {}
    } Extra;
    NAILANGAPI explicit TempPartedNameBase(std::u32string_view name, common::span<const PartedName::PartType> parts, uint16_t info);
    explicit TempPartedNameBase(std::u32string_view name, uint16_t info) : 
        TempPartedNameBase(name, PartedName::GetParts(name), info) { }
    NAILANGAPI explicit TempPartedNameBase(const PartedName* var) noexcept;
    NAILANGAPI TempPartedNameBase Copy() const noexcept;
    const PartedName& Get() const noexcept
    {
        return (Var.PartCount > 4 || Var.PartCount == 0) ? *Extra.Ptr : Var;
    }
public:
    NAILANGAPI ~TempPartedNameBase();
    NAILANGAPI TempPartedNameBase(TempPartedNameBase&& other) noexcept;
    TempPartedNameBase& operator= (TempPartedNameBase&&) noexcept = delete;
    COMMON_NO_COPY(TempPartedNameBase)
};

template<typename T>
class TempPartedName : public TempPartedNameBase
{
    static_assert(std::is_base_of_v<PartedName, T> && sizeof(T) == sizeof(PartedName));
    friend T;
    using TempPartedNameBase::TempPartedNameBase;
public:
    TempPartedName(TempPartedNameBase&& base) noexcept : TempPartedNameBase(std::move(base)) { }
    constexpr const T& Get() const noexcept
    {
        return static_cast<const T&>(TempPartedNameBase::Get());
    }
    constexpr const T* Ptr() const noexcept
    {
        return static_cast<const T*>(&TempPartedNameBase::Get());
    }
    TempPartedName Copy() const noexcept
    {
        return static_cast<TempPartedName>(TempPartedNameBase::Copy());
    }
    constexpr operator const T& () const noexcept
    {
        return Get();
    }
    static TempPartedName Wrapper(const T* var) noexcept
    {
        return TempPartedName(var);
    }
};


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
class NAILANGAPI NailangPartedNameException final : public common::BaseException
{
    friend class NailangParser;
    PREPARE_EXCEPTION(NailangPartedNameException, BaseException,
        std::u32string_view Name;
        std::u32string_view Part;
        ExceptionInfo(const std::u16string_view msg, const std::u32string_view name, const std::u32string_view part) noexcept
            : TPInfo(TYPENAME, msg), Name(name), Part(part)
        { }
    );
    NailangPartedNameException(const std::u16string_view msg, const std::u32string_view name, const std::u32string_view part)
        : BaseException(T_<ExceptionInfo>{}, msg, name, part)
    { }
};
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


}
