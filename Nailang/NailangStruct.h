#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef NAILANG_EXPORT
#   define NAILANGAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define NAILANGAPI _declspec(dllimport)
# endif
#else
# ifdef NAILANG_EXPORT
#   define NAILANGAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define NAILANGAPI
# endif
#endif


#include "common/CommonRely.hpp"
#include "common/TrunckedContainer.hpp"
#include "common/EnumEx.hpp"
#include "common/StrBase.hpp"
#include "common/StringLinq.hpp"
#include "common/StringPool.hpp"
#include "common/SharedString.hpp"
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

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class NAILANGAPI MemoryPool : protected common::container::TrunckedContainer<std::byte>
{
public:
    MemoryPool(const size_t trunkSize = 2 * 1024 * 1024) : common::container::TrunckedContainer<std::byte>(trunkSize, 64) { }

    [[nodiscard]] forceinline common::span<std::byte> Alloc(const size_t size, const size_t align = 64)
    {
        return common::container::TrunckedContainer<std::byte>::Alloc(size, align);
    }
    [[nodiscard]] std::pair<size_t, size_t> Usage() const noexcept;

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
struct COMMON_EMPTY_BASES PartedName : public common::NonCopyable, public common::NonMovable
{
    friend class TempPartedNameBase;
    using value_type = char32_t;
    using PartType = std::pair<uint16_t, uint16_t>;
    [[nodiscard]] constexpr operator std::u32string_view() const noexcept
    {
        return { Ptr, Length };
    }
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
        return other == *this;
    }
    [[nodiscard]] std::u32string_view operator[](size_t index) const noexcept
    {
        if (index >= PartCount)
            return {};
        if (PartCount == 1)
            return *this;
        const auto parts = reinterpret_cast<const PartType*>(this + 1);
        const auto [offset, len] = parts[index];
        return { Ptr + offset, len };
    }
    std::u32string_view FullName() const noexcept
    {
        return *this;
    }
    std::u32string_view GetRest(const uint32_t index) const noexcept
    {
        if (index >= PartCount)
            return {};
        if (index == 0)
            return *this;
        const auto parts = reinterpret_cast<const PartType*>(this + 1);
        const auto offset = parts[index].first;
        return { Ptr + offset, Length - offset };
    }
    std::u32string_view GetRange(const uint32_t from, const uint32_t to) const noexcept
    {
        Expects(to > from);
        if (to > PartCount)
            return {};
        if (PartCount == 1)
            return *this;
        const auto parts = reinterpret_cast<const PartType*>(this + 1);
        const uint32_t idxF = parts[from].first, idxT = parts[to].first - 1;
        return { Ptr + idxF, static_cast<size_t>(idxT - idxF) };
    }
    [[nodiscard]] constexpr auto Parts() const noexcept
    {
        return common::linq::FromRange<uint32_t>(0u, PartCount)
            .Select([self = this](uint32_t idx) { return (*self)[idx]; });
    }
    NAILANGAPI static std::vector<PartType> GetParts(std::u32string_view name);
    NAILANGAPI static PartedName* Create(MemoryPool& pool, std::u32string_view name, uint16_t exinfo = 0);
public:
    const char32_t* Ptr;
    uint32_t Length;
    uint16_t PartCount;
protected:
    uint16_t ExternInfo;
    constexpr PartedName(std::u32string_view name, common::span<const PartType> parts, uint16_t exinfo) noexcept :
        Ptr(name.data()), Length(gsl::narrow_cast<uint32_t>(name.size())),
        PartCount(gsl::narrow_cast<uint16_t>(parts.size())),
        ExternInfo(exinfo)
    { }
};

class NAILANGAPI TempPartedNameBase
{
protected:
    PartedName Var;
    union Tail
    {
        const PartedName* Ptr;
        PartedName::PartType Parts[4];
        constexpr Tail() noexcept : Ptr(nullptr) {}
    } Extra;
    explicit TempPartedNameBase(std::u32string_view name, common::span<const PartedName::PartType> parts, uint16_t info);
    explicit TempPartedNameBase(const PartedName* var) noexcept;
    TempPartedNameBase(TempPartedNameBase&& other) noexcept;
    TempPartedNameBase Copy() const noexcept;
    const PartedName& Get() const noexcept
    {
        return (Var.PartCount > 4 || Var.PartCount == 0) ? *Extra.Ptr : Var;
    }
public:
    ~TempPartedNameBase();
};

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

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
    /// <summary>Create a TempBindVar that wrappers a LateBindVar</summary>
    /// <param name="var">LateBindVar</param>
    /// <returns>TempBindVar</returns>
    static TempPartedName Wrapper(const T* var) noexcept
    {
        return TempPartedName(var);
    }
};


struct LateBindVar
{
    enum class VarInfo : uint16_t 
    { 
        Empty = 0x0, Root = 0x1, Local = 0x2, PrefixMask = Root | Local, 
    };
    std::u32string_view Name;
    VarInfo Info;
    constexpr explicit LateBindVar(const char32_t* ptr, const uint32_t size, VarInfo info) noexcept :
        Name(ptr, size), Info(info) 
    {
        Expects(Name.size() > 0);
    }
    constexpr LateBindVar(const std::u32string_view name) noexcept : Name(name), Info(VarInfo::Empty)
    {
        Expects(Name.size() > 0);
        if (Name[0] == U'`')
            Info = VarInfo::Root, Name.remove_prefix(1);
        else if (Name[0] == U':')
            Info = VarInfo::Local, Name.remove_prefix(1);
    }
    constexpr operator std::u32string_view() const noexcept
    {
        return Name;
    }
};
MAKE_ENUM_BITFIELD(LateBindVar::VarInfo)


enum class EmbedOps : uint8_t { Equal = 0, NotEqual, Less, LessEqual, Greater, GreaterEqual, And, Or, Not, Add, Sub, Mul, Div, Rem };
struct EmbedOpHelper
{
    [[nodiscard]] static constexpr bool IsUnaryOp(EmbedOps op) noexcept
    {
        return op == EmbedOps::Not;
    }
    [[nodiscard]] static std::u32string_view GetOpName(EmbedOps op) noexcept;
};


struct FuncCall;
struct UnaryExpr;
struct BinaryExpr;
struct QueryExpr;

namespace detail
{
union IntFPUnion
{
    uint64_t Uint;
    double FP;
    constexpr IntFPUnion() noexcept : Uint(0) { }
    template<typename T>
    constexpr IntFPUnion(const T val, typename std::enable_if_t< std::is_floating_point_v<T>>* = 0) noexcept : FP(val) { }
    template<typename T>
    constexpr IntFPUnion(const T val, typename std::enable_if_t<!std::is_floating_point_v<T>>* = 0) noexcept : Uint(val) { }
};
}

struct RawArg
{
    static_assert( sizeof(detail::IntFPUnion) ==  sizeof(uint64_t));
    static_assert(alignof(detail::IntFPUnion) == alignof(uint64_t));
private:
    detail::IntFPUnion Data1;
    uint32_t Data2;
    uint16_t Data3;
public:
    enum class Type : uint8_t { Empty = 0, Func, Unary, Binary, Query, Var, Str, Uint, Int, FP, Bool };
    uint8_t ExtraFlag;
    Type TypeData;
#define Fill1(type) ExtraFlag(0), TypeData(Type::type)
#define Fill2(type) Data3(common::enum_cast(Type::type)), Fill1(type)
#define Fill3(type) Data2(common::enum_cast(Type::type)), Fill2(type)
    constexpr RawArg() noexcept : Data1(), Fill3(Empty) {}
    RawArg(const FuncCall* ptr) noexcept : 
        Data1(reinterpret_cast<uint64_t>(ptr)), Fill3(Func) {}
    RawArg(const UnaryExpr* ptr) noexcept :
        Data1(reinterpret_cast<uint64_t>(ptr)), Fill3(Unary) {}
    RawArg(const BinaryExpr* ptr) noexcept :
        Data1(reinterpret_cast<uint64_t>(ptr)), Fill3(Binary) {}
    RawArg(const QueryExpr* ptr) noexcept :
        Data1(reinterpret_cast<uint64_t>(ptr)), Fill3(Query) {}
    RawArg(const LateBindVar& var) noexcept :
        Data1(reinterpret_cast<uint64_t>(var.Name.data())), Data2(static_cast<uint32_t>(var.Name.size())),
        Data3(common::enum_cast(var.Info)), Fill1(Var) 
    {
        Expects(var.Name.size() <= UINT32_MAX);
    }
    RawArg(const std::u32string_view str) noexcept :
        Data1(reinterpret_cast<uint64_t>(str.data())), Data2(static_cast<uint32_t>(str.size())), Fill2(Str)
    {
        Expects(str.size() <= UINT32_MAX);
    }
    RawArg(const uint64_t num) noexcept :
        Data1(num), Fill3(Uint) {}
    RawArg(const int64_t num) noexcept :
        Data1(static_cast<uint64_t>(num)), Fill3(Int) {}
    RawArg(const double num) noexcept :
        Data1(num), Fill3(FP) {}
    RawArg(const bool boolean) noexcept :
        Data1(boolean ? 1 : 0), Fill3(Bool) {}
#undef Fill3
#undef Fill2
#undef Fill1
    template<Type T>
    [[nodiscard]] constexpr decltype(auto) GetVar() const noexcept
    {
        Expects(TypeData == T);
        if constexpr (T == Type::Func)
            return reinterpret_cast<const FuncCall*>(Data1.Uint);
        else if constexpr (T == Type::Unary)
            return reinterpret_cast<const UnaryExpr*>(Data1.Uint);
        else if constexpr (T == Type::Binary)
            return reinterpret_cast<const BinaryExpr*>(Data1.Uint);
        else if constexpr (T == Type::Query)
            return reinterpret_cast<const QueryExpr*>(Data1.Uint);
        else if constexpr (T == Type::Var)
            return LateBindVar(reinterpret_cast<const char32_t*>(Data1.Uint), Data2, static_cast<LateBindVar::VarInfo>(Data3));
        else if constexpr (T == Type::Str)
            return std::u32string_view{ reinterpret_cast<const char32_t*>(Data1.Uint), Data2 };
        else if constexpr (T == Type::Uint)
            return Data1.Uint;
        else if constexpr (T == Type::Int)
            return static_cast<int64_t>(Data1.Uint);
        else if constexpr (T == Type::FP)
            return Data1.FP;
        else if constexpr (T == Type::Bool)
            return Data1.Uint == 1;
        else
            static_assert(!common::AlwaysTrue2<T>, "Unknown Type");
    }

    using Variant = std::variant<
        const FuncCall*, const UnaryExpr*, const BinaryExpr*, const QueryExpr*, 
        LateBindVar, std::u32string_view, uint64_t, int64_t, double, bool>;
    [[nodiscard]] forceinline Variant GetVar() const noexcept
    {
        switch (TypeData)
        {
        case Type::Func:    return GetVar<Type::Func>();
        case Type::Unary:   return GetVar<Type::Unary>();
        case Type::Binary:  return GetVar<Type::Binary>();
        case Type::Query:   return GetVar<Type::Query>();
        case Type::Var:     return GetVar<Type::Var>();
        case Type::Str:     return GetVar<Type::Str>();
        case Type::Uint:    return GetVar<Type::Uint>();
        case Type::Int:     return GetVar<Type::Int>();
        case Type::FP:      return GetVar<Type::FP>();
        case Type::Bool:    return GetVar<Type::Bool>();
        default:            Expects(false); return {};
        }
    }
    template<typename Visitor>
    [[nodiscard]] forceinline auto Visit(Visitor&& visitor) const
    {
        switch (TypeData)
        {
        case Type::Func:    return visitor(GetVar<Type::Func>());
        case Type::Unary:   return visitor(GetVar<Type::Unary>());
        case Type::Binary:  return visitor(GetVar<Type::Binary>());
        case Type::Query:   return visitor(GetVar<Type::Query>());
        case Type::Var:     return visitor(GetVar<Type::Var>());
        case Type::Str:     return visitor(GetVar<Type::Str>());
        case Type::Uint:    return visitor(GetVar<Type::Uint>());
        case Type::Int:     return visitor(GetVar<Type::Int>());
        case Type::FP:      return visitor(GetVar<Type::FP>());
        case Type::Bool:    return visitor(GetVar<Type::Bool>());
        default:            return visitor(std::nullopt);
        }
    }

    [[nodiscard]] forceinline std::u32string_view GetTypeName() const noexcept
    {
        return TypeName(TypeData);
    }
    [[nodiscard]] NAILANGAPI static std::u32string_view TypeName(const Type type) noexcept;
};


struct SubQuery
{
    enum class QueryType : uint8_t { Index = 1, Sub = 2 };
    common::span<const RawArg> Queries;
    constexpr SubQuery Sub(const size_t offset = 1) const noexcept
    {
        Expects(offset < Queries.size());
        return { Queries.subspan(offset) };
    }
    constexpr std::pair<QueryType, RawArg> operator[](size_t idx) const noexcept
    {
        Expects(idx < Queries.size());
        return { static_cast<QueryType>(Queries[idx].ExtraFlag), Queries[idx] };
    }
    NAILANGAPI std::u32string_view ExpectSubField(size_t idx) const;
    NAILANGAPI const RawArg&       ExpectIndex(size_t idx) const;
    constexpr size_t Size() const noexcept
    {
        return Queries.size();
    }
    static void PushQuery(std::vector<RawArg>& container, std::u32string_view subField)
    {
        RawArg query(subField);
        query.ExtraFlag = common::enum_cast(QueryType::Sub);
        container.push_back(query);
    }
    static void PushQuery(std::vector<RawArg>& container, RawArg indexer)
    {
        indexer.ExtraFlag = common::enum_cast(QueryType::Index);
        container.push_back(indexer);
    }
};
struct QueryExpr : public SubQuery
{
    RawArg Target;
    QueryExpr(const RawArg target, common::span<const RawArg> queries) noexcept :
        SubQuery{ queries }, Target(target) { }
};


struct CustomVar
{
    struct Handler;
    alignas(detail::IntFPUnion) Handler* Host;
    uint64_t Meta0;
    uint32_t Meta1;
    uint16_t Meta2;
    template<auto F, typename... Args>
    auto Call(Args&&... args) const
    {
        static_assert(std::is_invocable_v<decltype(F), Handler&, const CustomVar&, Args...>,
            "F need to be member function of Handler, accept const CustomVar& and args");
        return (Host->*F)(*this, std::forward<Args>(args)...);
    }
    template<auto F, typename... Args>
    auto Call(Args&&... args)
    {
        static_assert(std::is_invocable_v<decltype(F), Handler&, CustomVar&, Args...>,
            "F need to be member function of Handler, accept const CustomVar& and args");
        return (Host->*F)(*this, std::forward<Args>(args)...);
    }
    template<typename T>
    [[nodiscard]] forceinline bool IsType() const noexcept
    {
        static_assert(std::is_base_of_v<Handler, T>, "type need to inherit from Handler");
        return dynamic_cast<T*>(Host) != nullptr;
    }
};


struct Arg;
class NAILANGAPI ArgLocator;


struct NativeWrapper
{
    enum class Type : uint16_t
    {
        Any = 0, Bool,
        U8, I8, U16, I16, F16, U32, I32, F32, U64, I64, F64,
        Str8, Str16, Str32, Sv8, Sv16, Sv32
    };
    using Getter = Arg(*)(uintptr_t pointer, size_t idx);
    using Setter = bool(*)(uintptr_t pointer, size_t idx, Arg arg);
    NAILANGAPI static std::u32string_view TypeName(Type type) noexcept;
    NAILANGAPI static ArgLocator GetLocator(Type type, uintptr_t pointer, bool isConst, size_t idx = 0) noexcept;
    NAILANGAPI static Getter GetGetter(Type type, bool move = false) noexcept;
    NAILANGAPI static Setter GetSetter(Type type) noexcept;
    template<typename T>
    static constexpr Type GetType() noexcept
    {
#define Ret(native, type) if constexpr (std::is_same_v<T, native>) return Type::type
             Ret(Arg,                   Any);
        else Ret(bool,                  Bool);
        else Ret(uint8_t,               U8);
        else Ret(int8_t,                I8);
        else Ret(uint16_t,              U16);
        else Ret(int16_t,               I16);
        else Ret(half_float::half,      F16);
        else Ret(uint32_t,              U32);
        else Ret(int32_t,               I32);
        else Ret(float,                 F32);
        else Ret(uint64_t,              U64);
        else Ret(int64_t,               I64);
        else Ret(double,                F64);
        else Ret(std::string,           Str8);
        else Ret(std::string_view,      Sv8);
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
        else Ret(std::u8string,         Str8);
        else Ret(std::u8string_view,    Sv8);
#endif
        else Ret(std::u16string,        Str16);
        else Ret(std::u16string_view,   Sv16);
        else Ret(std::u32string,        Str32);
        else Ret(std::u32string_view,   Sv32);
        else static_assert(!common::AlwaysTrue<T>, "unsupported type");
#undef Ret
    }
};


struct FixedArray
{
    using Type = NativeWrapper::Type;
    uint64_t DataPtr;
    uint64_t Length;
    Type ElementType;
    bool IsReadOnly;
    ArgLocator Access(size_t idx) const noexcept;
    Arg Get(size_t idx) const noexcept;
    bool Set(size_t idx, Arg val) const noexcept;
#define SPT(type) common::span<type>, common::span<const type>
    using SpanVariant = std::variant<std::monostate, SPT(Arg), SPT(bool),
        SPT(uint8_t), SPT(int8_t), SPT(uint16_t), SPT(int16_t), SPT(half_float::half),
        SPT(uint32_t), SPT(int32_t), SPT(float), SPT(uint64_t), SPT(int64_t), SPT(double),
        SPT(std::string), SPT(std::u16string), SPT(std::u32string),
        SPT(std::string_view), SPT(std::u16string_view), SPT(std::u32string_view)>;
#undef SPT
    NAILANGAPI SpanVariant GetSpan() const noexcept;
    NAILANGAPI void PrintToStr(std::u32string& str, std::u32string_view delim) const;
    forceinline std::u32string_view GetElementTypeName() const noexcept
    {
        return NativeWrapper::TypeName(ElementType);
    }
    template<typename T>
    static FixedArray Create(common::span<T> elements)
    {
        static constexpr bool IsConst = std::is_const_v<T>;
        using R = std::remove_const_t<T>;
        return { reinterpret_cast<uintptr_t>(elements.data()), elements.size(), NativeWrapper::GetType<R>(), IsConst };
    }
};

class NAILANGAPI NailangRuntimeBase;
class NAILANGAPI NailangRuntimeException;
struct Arg
{
    static_assert(sizeof(uintptr_t) <= sizeof(uint64_t));
    static_assert(sizeof(size_t)    <= sizeof(uint64_t));
    static_assert(sizeof(std::u32string_view) == sizeof(common::SharedString<char32_t>));
public:
    enum class Type : uint16_t
    {
        // |0   |1   |2  |3  |4  |...|7     |
        // |real|bool|str|num|arr|...|custom|
        // For Str
        // |8       |...15|
        // |real str|.....|
        // For Num
        // |8  |9       |...15|
        // |int|unsigned|.....|
        Empty = 0, RealType = 0b00000001,
        Boolable = 0b00000010, String = 0b00000110, Number = 0b00001010, ArrayLike = 0b00010000, Custom = 0b10000000,
        IntegerBit = 0b00000001 << 8, UnsignedBit = 0b00000010 << 8, 
        StrRealBit = 0b00000001 << 8,
        Integer = Number | IntegerBit,
        Bool   = RealType | Boolable,
        U32Sv  = RealType | String   | Empty,
        U32Str = RealType | String   | StrRealBit,
        FP     = RealType | Number   | Empty,
        Uint   = RealType | Integer  | UnsignedBit, 
        Int    = RealType | Integer  | Empty,
        Array  = RealType | ArrayLike,
        Var    = RealType | Custom,
    };
private:
    detail::IntFPUnion Data0;
    uint64_t Data1;
    uint32_t Data2;
    uint16_t Data3;
    NAILANGAPI void Release() noexcept;
    [[nodiscard]] Arg ConvertCustomType(Type type) const noexcept;
public:
    Type TypeData;

    template<Type T>
    [[nodiscard]] constexpr auto GetVar() const noexcept
    {
        Expects(TypeData == T);
        if constexpr (T == Type::Var)
            return CustomVar{ reinterpret_cast<CustomVar::Handler*>(Data0.Uint), Data1, Data2, Data3 };
        else if constexpr (T == Type::Array)
            return FixedArray{ static_cast<uintptr_t>(Data0.Uint), Data1, static_cast<FixedArray::Type>(Data2), static_cast<bool>(Data3) };
        else if constexpr (T == Type::U32Str)
            return std::u32string_view{ reinterpret_cast<const char32_t*>(Data0.Uint), Data1 };
        else if constexpr (T == Type::U32Sv)
            return std::u32string_view{ reinterpret_cast<const char32_t*>(Data0.Uint), Data1 };
        else if constexpr (T == Type::Uint)
            return Data0.Uint;
        else if constexpr (T == Type::Int)
            return static_cast<int64_t>(Data0.Uint);
        else if constexpr (T == Type::FP)
            return Data0.FP;
        else if constexpr (T == Type::Bool)
            return Data0.Uint == 1;
        else
            static_assert(!common::AlwaysTrue2<T>, "Unknown Type");
    }

    Arg() noexcept : Data0(0), Data1(0), Data2(0), Data3(0), TypeData(Type::Empty)
    { }
    Arg(CustomVar&& var) noexcept :
        Data0(reinterpret_cast<uint64_t>(var.Host)), Data1(var.Meta0), Data2(var.Meta1), Data3(var.Meta2), TypeData(Type::Var)
    {
        Expects(var.Host != nullptr);
        var.Host = nullptr;
        var.Meta0 = var.Meta1 = var.Meta2 = 0;
    }
    Arg(const FixedArray arr) noexcept :
        Data0(arr.DataPtr), Data1(arr.Length), Data2(common::enum_cast(arr.ElementType)), Data3(arr.IsReadOnly ? 1 : 0), TypeData(Type::Array)
    { }
    Arg(const std::u32string& str) noexcept : 
        Data0(0), Data1(0), Data2(0), Data3(4), TypeData(Type::U32Str)
    {
        if (str.size() > 0)
        {
            const auto tmp = common::SharedString<char32_t>::PlacedCreate(str);
            Data0 = reinterpret_cast<uint64_t>(tmp.data());
            Data1 = str.size();
        }
    }
    Arg(const std::u32string_view str) noexcept :
        Data0(reinterpret_cast<uint64_t>(str.data())), Data1(str.size()),
        Data2(0), Data3(5), TypeData(Type::U32Sv)
    { }
    Arg(const uint64_t num) noexcept :
        Data0(num), Data1(0), Data2(0), Data3(6), TypeData(Type::Uint)
    { }
    Arg(const int64_t num) noexcept :
        Data0(static_cast<uint64_t>(num)), Data1(0), Data2(0), Data3(7), TypeData(Type::Int)
    { }
    Arg(const double num) noexcept :
        Data0(num), Data1(0), Data2(0), Data3(8), TypeData(Type::FP)
    { }
    Arg(const bool boolean) noexcept :
        Data0(boolean ? 1 : 0), Data1(0), Data2(0), Data3(9), TypeData(Type::Bool)
    { }
    ~Arg()
    {
        Release();
    }
    Arg(Arg&& other) noexcept :
        Data0(other.Data0), Data1(other.Data1), Data2(other.Data2), Data3(other.Data3), TypeData(other.TypeData)
    {
        other.TypeData = Type::Empty;
    }
    Arg& operator=(Arg&& other) noexcept
    {
        Release();
        Data0 = other.Data0;
        Data1 = other.Data1;
        Data2 = other.Data2;
        Data3 = other.Data3;
        TypeData = other.TypeData;
        other.TypeData = Type::Empty;
        return *this;
    }
    NAILANGAPI Arg(const Arg& other) noexcept;
    NAILANGAPI Arg& operator=(const Arg& other) noexcept;

    template<typename Visitor>
    [[nodiscard]] forceinline auto Visit(Visitor&& visitor) const
    {
        switch (TypeData)
        {
        case Type::Var:     return visitor(GetVar<Type::Var>());
        case Type::Array:   return visitor(GetVar<Type::Array>());
        case Type::U32Str:  return visitor(GetVar<Type::U32Str>());
        case Type::U32Sv:   return visitor(GetVar<Type::U32Sv>());
        case Type::Uint:    return visitor(GetVar<Type::Uint>());
        case Type::Int:     return visitor(GetVar<Type::Int>());
        case Type::FP:      return visitor(GetVar<Type::FP>());
        case Type::Bool:    return visitor(GetVar<Type::Bool>());
        default:            return visitor(std::nullopt);
        }
    }

    [[nodiscard]] forceinline constexpr bool IsEmpty() const noexcept
    {
        return TypeData == Type::Empty;
    }
    [[nodiscard]] forceinline constexpr bool IsBool() const noexcept
    {
        return TypeData == Type::Bool;
    }
    [[nodiscard]] forceinline constexpr bool IsInteger() const noexcept
    {
        return TypeData == Type::Uint || TypeData == Type::Int;
    }
    [[nodiscard]] forceinline constexpr bool IsFloatPoint() const noexcept
    {
        return TypeData == Type::FP;
    }
    [[nodiscard]] forceinline constexpr bool IsNumber() const noexcept
    {
        return IsFloatPoint() || IsInteger();
    }
    [[nodiscard]] forceinline constexpr bool IsStr() const noexcept
    {
        return TypeData == Type::U32Str || TypeData == Type::U32Sv;
    }
    [[nodiscard]] forceinline constexpr bool IsArray() const noexcept
    {
        return TypeData == Type::Array;
    }
    [[nodiscard]] forceinline constexpr bool IsCustom() const noexcept
    {
        return TypeData == Type::Var;
    }
    template<typename T>
    [[nodiscard]] forceinline bool IsCustomType() const noexcept
    {
        static_assert(std::is_base_of_v<CustomVar::Handler, T>, "type need to inherit from Handler");
        if (!IsCustom()) return false;
        return GetVar<Type::Var>().IsType<T>();
    }
    
    [[nodiscard]] forceinline CustomVar& GetCustom() noexcept
    {
        static_assert( sizeof(Arg) ==  sizeof(CustomVar));
        static_assert(alignof(Arg) == alignof(CustomVar));
        return *reinterpret_cast<CustomVar*>(this);
    }
    [[nodiscard]] forceinline const CustomVar& GetCustom() const noexcept
    {
        return *reinterpret_cast<const CustomVar*>(this);
    }
    [[nodiscard]] NAILANGAPI std::optional<bool>                GetBool()   const noexcept;
    [[nodiscard]] NAILANGAPI std::optional<uint64_t>            GetUint()   const noexcept;
    [[nodiscard]] NAILANGAPI std::optional<int64_t>             GetInt()    const noexcept;
    [[nodiscard]] NAILANGAPI std::optional<double>              GetFP()     const noexcept;
    [[nodiscard]] NAILANGAPI std::optional<std::u32string_view> GetStr()    const noexcept;
    [[nodiscard]] NAILANGAPI common::str::StrVariant<char32_t>  ToString()  const noexcept;
    [[nodiscard]] NAILANGAPI ArgLocator             HandleQuery(SubQuery, NailangRuntimeBase&);

    [[nodiscard]] std::u32string_view GetTypeName() const noexcept;
    [[nodiscard]] NAILANGAPI static std::u32string_view TypeName(const Type type) noexcept;
};
MAKE_ENUM_BITFIELD(Arg::Type)

struct NAILANGAPI CustomVar::Handler
{
protected:
    static Arg EvaluateArg(NailangRuntimeBase& runtime, const RawArg& arg);
    static void HandleException(NailangRuntimeBase& runtime, const NailangRuntimeException& ex);
public:
    virtual void IncreaseRef(CustomVar&) noexcept;
    virtual void DecreaseRef(CustomVar&) noexcept;
    [[nodiscard]] virtual Arg IndexerGetter(const CustomVar&, const Arg&, const RawArg&);
    [[nodiscard]] virtual Arg SubfieldGetter(const CustomVar&, std::u32string_view);
    [[nodiscard]] virtual ArgLocator HandleQuery(CustomVar&, SubQuery, NailangRuntimeBase&);
    [[nodiscard]] virtual bool HandleAssign(CustomVar&, Arg);
    [[nodiscard]] virtual common::str::StrVariant<char32_t> ToString(const CustomVar&) noexcept;
    [[nodiscard]] virtual Arg ConvertToCommon(const CustomVar&, Arg::Type) noexcept;
    [[nodiscard]] virtual std::u32string_view GetTypeName() noexcept;
};

[[nodiscard]] inline std::u32string_view Arg::GetTypeName() const noexcept
{
    if (IsCustom())
        return GetCustom().Host->GetTypeName();
    return TypeName(TypeData);
}
[[nodiscard]] inline Arg Arg::ConvertCustomType(Type type) const noexcept
{
    return GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(type);
}


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class NAILANGAPI ArgLocator
{
public:
    enum class LocateFlags : uint16_t 
    { 
        Readable = 0b1, NonConst = 0b10, Assignable = 0b110,
        Empty = 0, ReadOnly = Readable, WriteOnly = NonConst | Assignable,
        Mutable = Readable | NonConst, ReadWrite = Readable | Assignable
    };
    using Getter = std::function<Arg(void)>;
    using Setter = std::function<bool(Arg)>;
private:
    struct GetSet
    {
        Getter Get;
        Setter Set;
        GetSet(Getter&& getter, Setter&& setter) noexcept : Get(std::move(getter)), Set(std::move(setter))
        { }
    };
    enum class LocateType  : uint16_t { Empty, Arg, Ptr, GetSet };
    Arg Val;
    uint32_t Consumed;
    LocateType Type;
    LocateFlags Flag;
    static Arg PointerToVal(const void* ptr) noexcept
    {
        if (ptr)
            return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(ptr));
        else
            return {};
    }
public:
    ArgLocator() noexcept : Val{}, Consumed(0), Type(LocateType::Empty), Flag(LocateFlags::Empty) 
    { }
    ArgLocator(Arg&& arg, uint32_t consumed, LocateFlags flag = LocateFlags::ReadOnly) noexcept
        : Val{ std::move(arg) }, Consumed(consumed), Type(LocateType::Arg), Flag(flag)
    { }
    ArgLocator(const Arg* arg, uint32_t consumed, LocateFlags flag = LocateFlags::ReadOnly) noexcept :
        Val(PointerToVal(arg)), Consumed(consumed), Type(arg ? LocateType::Ptr : LocateType::Empty), 
        Flag(arg ? flag : LocateFlags::Empty)
    { }
    ArgLocator(Arg* arg, uint32_t consumed, LocateFlags flag = LocateFlags::ReadWrite) noexcept :
        Val(PointerToVal(arg)), Consumed(consumed), Type(arg ? LocateType::Ptr : LocateType::Empty), 
        Flag(arg ? flag : LocateFlags::Empty)
    { }
    ArgLocator(Getter getter, Setter setter, uint32_t consumed) noexcept;
    ArgLocator(const ArgLocator&) = delete;
    ArgLocator(ArgLocator&& arg) noexcept : Val(std::move(arg.Val)), Consumed(arg.Consumed), Type(arg.Type), Flag(arg.Flag)
    {
        arg.Type = LocateType::Empty;
    }
    ~ArgLocator();
    ArgLocator& operator=(const ArgLocator&) = delete;
    ArgLocator& operator=(ArgLocator&&) = delete;
    explicit constexpr operator bool() const noexcept
    {
        return Type != LocateType::Empty;
    }
    [[nodiscard]] constexpr bool CanRead()   const noexcept;
    [[nodiscard]] constexpr bool CanAssign() const noexcept;
    [[nodiscard]] constexpr bool IsMutable() const noexcept;
    Arg Get() const;
    Arg ExtractGet();
    bool Set(Arg val);
    [[nodiscard]] ArgLocator HandleQuery(SubQuery, NailangRuntimeBase&);
};

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

MAKE_ENUM_BITFIELD(ArgLocator::LocateFlags)
[[nodiscard]] inline constexpr bool ArgLocator::CanRead()   const noexcept
{
    return HAS_FIELD(Flag, LocateFlags::Readable);
}
[[nodiscard]] inline constexpr bool ArgLocator::CanAssign() const noexcept
{
    return HAS_FIELD(Flag, LocateFlags::Assignable);
}
[[nodiscard]] inline constexpr bool ArgLocator::IsMutable() const noexcept
{
    return HAS_FIELD(Flag, LocateFlags::Mutable);
}


inline ArgLocator FixedArray::Access(size_t idx) const noexcept
{
    Expects(idx < Length);
    return NativeWrapper::GetLocator(ElementType, DataPtr, IsReadOnly, idx);
}
inline Arg FixedArray::Get(size_t idx) const noexcept
{
    Expects(idx < Length);
    return NativeWrapper::GetGetter(ElementType)(static_cast<uintptr_t>(DataPtr), idx);
}
inline bool FixedArray::Set(size_t idx, Arg val) const noexcept
{
    Expects(idx < Length);
    return NativeWrapper::GetSetter(ElementType)(static_cast<uintptr_t>(DataPtr), idx, std::move(val));
}


struct WithPos
{
    std::pair<uint32_t, uint32_t> Position = { 0,0 };
};


struct FuncName : public PartedName
{
    enum class FuncInfo : uint16_t
    {
        Empty = 0x0, Meta = 0x1, ExprPart = 0x2,
    };

    constexpr FuncInfo Info() const noexcept { return static_cast<FuncInfo>(ExternInfo); }
    constexpr common::EnumWrapper<FuncInfo> Info() noexcept { return ExternInfo; }

    static FuncName* Create(MemoryPool& pool, std::u32string_view name, FuncInfo info)
    {
        if (name.size() == 0)
            return nullptr;
        const auto ptr = static_cast<FuncName*>(PartedName::Create(pool, name));
        ptr->ExternInfo = common::enum_cast(info);
        return ptr;
    }
    static TempPartedName<FuncName> CreateTemp(std::u32string_view name, FuncInfo info)
    {
        const auto parts = PartedName::GetParts(name);
        return TempPartedName<FuncName>(name, parts, common::enum_cast(info));
    }
private:
    friend TempPartedName<FuncName>;
    using PartedName::PartedName;
};
MAKE_ENUM_BITFIELD(FuncName::FuncInfo)
using TempFuncName = TempPartedName<FuncName>;


struct FuncCall : public WithPos
{
    const FuncName* Name;
    common::span<const RawArg> Args;
    constexpr FuncCall() noexcept : Name(nullptr) {}
    constexpr FuncCall(const FuncName* name, common::span<const RawArg> args) noexcept : Name(name), Args(args) { }
    constexpr FuncCall(const FuncName* name, common::span<const RawArg> args, const std::pair<uint32_t, uint32_t> pos) noexcept :
        WithPos{pos}, Name(name), Args(args)
    { }
};
struct UnaryExpr
{
    RawArg Oprend;
    EmbedOps Operator;
    UnaryExpr(const EmbedOps op, const RawArg oprend) noexcept :
        Oprend(oprend), Operator(op) { }
};
struct BinaryExpr
{
    RawArg LeftOprend, RightOprend;
    EmbedOps Operator;
    BinaryExpr(const EmbedOps op, const RawArg left, const RawArg right) noexcept :
        LeftOprend(left), RightOprend(right), Operator(op) { }
};


template<typename T>
struct WithMeta : public T
{
    common::span<const FuncCall> MetaFunctions;
};

struct RawBlock : public WithPos
{
    std::u32string_view Type;
    std::u32string_view Name;
    std::u32string_view Source;
    std::u16string FileName;
};
using RawBlockWithMeta = WithMeta<RawBlock>;
struct Block;

enum class NilCheck : uint8_t { None = 0, ReqNull = 0x1, ReqNotNull = 0x2 };
struct Assignment : public WithPos
{
    LateBindVar Target;
    SubQuery Queries;
    RawArg Statement;
    NilCheck CheckNil;

    constexpr Assignment(std::u32string_view name) noexcept :
        Target(name), CheckNil(NilCheck::None) { }

    constexpr std::u32string_view GetVar() const noexcept
    { 
        return Target.Name;
    }
};

struct BlockContent
{
    enum class Type : uint8_t { Assignment = 0, FuncCall = 1, RawBlock = 2, Block = 3 };
    uintptr_t Pointer;

    [[nodiscard]] static BlockContent Generate(const Assignment* ptr) noexcept
    {
        const auto pointer = reinterpret_cast<uintptr_t>(ptr);
        Expects(pointer % 4 == 0); // should be at least 4 bytes aligned
        return BlockContent{ pointer | common::enum_cast(Type::Assignment) };
    }
    [[nodiscard]] static BlockContent Generate(const FuncCall* ptr) noexcept
    {
        const auto pointer = reinterpret_cast<uintptr_t>(ptr);
        Expects(pointer % 4 == 0); // should be at least 4 bytes aligned
        return BlockContent{ pointer | common::enum_cast(Type::FuncCall) };
    }
    [[nodiscard]] static BlockContent Generate(const RawBlock* ptr) noexcept
    {
        const auto pointer = reinterpret_cast<uintptr_t>(ptr);
        Expects(pointer % 4 == 0); // should be at least 4 bytes aligned
        return BlockContent{ pointer | common::enum_cast(Type::RawBlock) };
    }
    [[nodiscard]] static BlockContent Generate(const Block* ptr) noexcept
    {
        const auto pointer = reinterpret_cast<uintptr_t>(ptr);
        Expects(pointer % 4 == 0); // should be at least 4 bytes aligned
        return BlockContent{ pointer | common::enum_cast(Type::Block) };
    }

    template<typename T>
    [[nodiscard]] forceinline const T* Get() const noexcept
    {
        return reinterpret_cast<const T*>(Pointer & ~(uintptr_t)(0x3));
    }

    [[nodiscard]] forceinline constexpr Type GetType() const noexcept
    {
        return static_cast<Type>(Pointer & 0x3);
    }
    [[nodiscard]] std::variant<const Assignment*, const FuncCall*, const RawBlock*, const Block*>
        GetStatement() const noexcept
    {
        switch (GetType())
        {
        case Type::Assignment:  return Get<Assignment>();
        case Type::FuncCall:    return Get<  FuncCall>();
        case Type::RawBlock:    return Get<  RawBlock>();
        case Type::   Block:    return Get<     Block>();
        default:                Expects(false); return {};
        }
    }
    template<typename Visitor> 
    auto Visit(Visitor&& visitor) const
    {
        switch (GetType())
        {
        case Type::Assignment:  return visitor(Get<Assignment>());
        case Type::FuncCall:    return visitor(Get<  FuncCall>());
        case Type::RawBlock:    return visitor(Get<  RawBlock>());
        case Type::Block:       return visitor(Get<     Block>());
        default:Expects(false); return visitor(static_cast<const Assignment*>(nullptr));
        }
    }
};
struct BlockContentItem : public BlockContent
{
    uint32_t Offset, Count;
    template<typename T>
    [[nodiscard]] static BlockContentItem Generate(const T* ptr, const uint32_t offset, const uint32_t count) noexcept
    {
        return BlockContentItem{ BlockContent::Generate(ptr), offset, count };
    }
};

struct Block : RawBlock
{
private:
    using value_type = std::pair<common::span<const FuncCall>, BlockContent>;

    class BlockIterator
    {
        friend struct Block;
        const Block* Host;
        size_t Idx;
        constexpr BlockIterator(const Block* block, size_t idx) noexcept : Host(block), Idx(idx) {}
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = typename Block::value_type;
        using difference_type = std::ptrdiff_t;

        [[nodiscard]] constexpr value_type operator*() const noexcept
        {
            return (*Host)[Idx];
        }
        [[nodiscard]] constexpr bool operator!=(const BlockIterator& other) const noexcept
        {
            return Host != other.Host || Idx != other.Idx;
        }
        [[nodiscard]] constexpr bool operator==(const BlockIterator& other) const noexcept
        {
            return Host == other.Host && Idx == other.Idx;
        }
        constexpr BlockIterator& operator++() noexcept
        {
            Idx++;
            return *this;
        }
        constexpr BlockIterator& operator--() noexcept
        {
            Idx--;
            return *this;
        }
        constexpr BlockIterator& operator+=(size_t n) noexcept
        {
            Idx += n;
            return *this;
        }
        constexpr BlockIterator& operator-=(size_t n) noexcept
        {
            Idx -= n;
            return *this;
        }
    };
public:
    common::span<const FuncCall> MetaFuncations;
    common::span<const BlockContentItem> Content;

    [[nodiscard]] constexpr BlockIterator begin() const noexcept
    {
        return { this, 0 };
    }
    [[nodiscard]] constexpr BlockIterator end() const noexcept
    {
        return { this, Size() };
    }
    [[nodiscard]] constexpr value_type operator[](size_t index) const noexcept
    {
        const auto& content = Content[index];
        const auto metafuncs = MetaFuncations.subspan(content.Offset, content.Count);
        return { metafuncs, content };
    }
    [[nodiscard]] constexpr size_t Size() const noexcept { return Content.size(); }
};


struct NAILANGAPI Serializer
{
    static void Stringify(std::u32string& output, const SubQuery& subq);
    static void Stringify(std::u32string& output, const RawArg& arg, const bool requestParenthese = false);
    static void Stringify(std::u32string& output, const FuncCall* call);
    static void Stringify(std::u32string& output, const UnaryExpr* expr);
    static void Stringify(std::u32string& output, const BinaryExpr* expr, const bool requestParenthese = false);
    static void Stringify(std::u32string& output, const QueryExpr* expr);
    static void Stringify(std::u32string& output, const LateBindVar& var);
    static void Stringify(std::u32string& output, const std::u32string_view str);
    static void Stringify(std::u32string& output, const uint64_t u64);
    static void Stringify(std::u32string& output, const int64_t i64);
    static void Stringify(std::u32string& output, const double f64);
    static void Stringify(std::u32string& output, const bool boolean);

    template<typename T>
    static std::u32string Stringify(const T& obj)
    {
        std::u32string result;
        Stringify(result, obj);
        return result;
    }
};


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class NAILANGAPI AutoVarHandlerBase : public CustomVar::Handler
{
protected:
    struct NAILANGAPI Accessor
    {
        using TAuto = std::function<CustomVar(void*)>;
        using TSimp = std::pair<std::function<Arg(void*)>, std::function<bool(void*, Arg)>>;
        union
        {
            TAuto AutoMember;
            TSimp SimpleMember;
        };
        bool IsSimple;
        bool IsConst;
        Accessor() noexcept;
        Accessor(const Accessor&) = delete;
        Accessor(Accessor&& other) noexcept;
        Accessor& operator=(const Accessor&) = delete;
        Accessor& operator=(Accessor&& other) = delete;
        void SetCustom(std::function<CustomVar(void*)> accessor) noexcept;
        void SetGetSet(std::function<Arg(void*)> getter, std::function<bool(void*, Arg)> setter) noexcept;
        void Release() noexcept;
        ~Accessor();
    };
    class AccessorBuilder
    {
        Accessor& Host;
    public:
        constexpr AccessorBuilder(Accessor& host) noexcept : Host(host) {}
        NAILANGAPI AccessorBuilder& SetConst(bool isConst = true);
    };
    std::u32string TypeName;
    size_t TypeSize;
    common::HashedStringPool<char32_t> NamePool;
    std::vector<std::pair<common::StringPiece<char32_t>, Accessor>> MemberList;
    std::function<void(void*, Arg)> Assigner;
    AutoVarHandlerBase(std::u32string_view typeName, size_t typeSize);
    Accessor* FindMember(std::u32string_view name, bool create = false);
    template<typename T>
    static constexpr auto GetFlag() noexcept
    {
        return common::enum_cast(std::is_const_v<T> ? VarFlags::Empty : VarFlags::NonConst);
    }
public:
    enum class VarFlags : uint16_t { Empty = 0x0, NonConst = 0x1 };
    virtual ~AutoVarHandlerBase();
    AutoVarHandlerBase(const AutoVarHandlerBase&) = delete;
    AutoVarHandlerBase(AutoVarHandlerBase&&) = delete;
    AutoVarHandlerBase& operator=(const AutoVarHandlerBase&) = delete;
    AutoVarHandlerBase& operator=(AutoVarHandlerBase&&) = delete;

    ArgLocator HandleQuery(CustomVar& var, SubQuery subq, NailangRuntimeBase& runtime) override;
    bool HandleAssign(CustomVar& var, Arg val) override;
    std::u32string_view GetTypeName() noexcept override;
};

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

MAKE_ENUM_BITFIELD(AutoVarHandlerBase::VarFlags)

template<typename T>
struct AutoVarHandler : public AutoVarHandlerBase
{
private:
    std::vector<std::unique_ptr<AutoVarHandlerBase>> UnnamedHandler;
public:
    AutoVarHandler(std::u32string_view typeName) : AutoVarHandlerBase(typeName, sizeof(T)) 
    { }
    ~AutoVarHandler() override {}
    template<typename U, typename F>
    forceinline AccessorBuilder AddAutoMember(std::u32string_view name, AutoVarHandler<U>& handler, F&& accessor)
    {
        const auto dst = FindMember(name, true);
        dst->SetCustom([handlerPtr = &handler, accessor = std::move(accessor)](void* ptr) -> CustomVar
        {
            auto& parent = *reinterpret_cast<T*>(ptr);
            using X = std::invoke_result_t<F, T&>;
            if constexpr (std::is_same_v<X, U*> || std::is_same_v<X, const U*>)
            {
                static constexpr auto Flag = AutoVarHandlerBase::GetFlag<std::remove_pointer_t<X>>();
                const auto fieldPtr = reinterpret_cast<uintptr_t>(accessor(parent));
                return CustomVar{ handlerPtr, static_cast<uint64_t>(fieldPtr), UINT32_MAX, Flag };
            }
            else if constexpr (std::is_same_v<X, common::span<U>> || std::is_same_v<X, common::span<const U>>)
            {
                static constexpr auto Flag = AutoVarHandlerBase::GetFlag<typename X::value_type>();
                const auto sp = accessor(parent);
                const auto elePtr = reinterpret_cast<uintptr_t>(sp.data());
                Expects(sp.size() < SIZE_MAX);
                const auto eleNum = static_cast<uint32_t>(sp.size());
                return CustomVar{ handlerPtr, static_cast<uint64_t>(elePtr), eleNum, Flag };
            }
            else
            {
                static_assert(!common::AlwaysTrue<X>, "accessor should accept T& and return U* or span<U>");
            }
        });
        return *dst;
    }
    template<typename U, typename F, typename R>
    forceinline AccessorBuilder AddAutoMember(std::u32string_view name, std::u32string_view typeName, F&& accessor, R&& initer)
    {
        using H = AutoVarHandler<U>;
        static_assert(std::is_invocable_v<R, H&>, "initer need to accept autohandler of U");
        UnnamedHandler.push_back(std::make_unique<H>(typeName));
        auto& handler = *static_cast<H*>(UnnamedHandler.back().get());
        initer(handler);
        return AddAutoMember<U, F>(name, handler, std::forward<F>(accessor));
    }
    template<typename F>
    forceinline AccessorBuilder AddCustomMember(std::u32string_view name, F&& accessor)
    {
        static_assert(std::is_invocable_r_v<CustomVar, F, T&>, "accessor should accept T& and return CustomVar");
        const auto dst = FindMember(name, true);
        dst->SetCustom([accessor = std::move(accessor)](void* ptr) -> CustomVar
        {
            auto& parent = *reinterpret_cast<T*>(ptr);
            return accessor(parent);
        });
        return *dst;
    }
    template<typename F>
    forceinline AccessorBuilder AddSimpleMember(std::u32string_view name, F&& accessor)
    {
        static_assert(std::is_invocable_v<F, T&>, "accessor should accept T&");
        using R = std::invoke_result_t<F, T&>;
        static constexpr bool IsPtr = std::is_pointer_v<R>;
        using U = std::remove_pointer_t<R>;
        static constexpr bool IsConst = !IsPtr || std::is_const_v<U>;
        static constexpr auto Type = NativeWrapper::GetType<std::remove_const_t<U>>();

        const auto dst = FindMember(name, true);
        std::function<bool(void*, Arg)> setFunc;
        if constexpr (!IsConst)
        {
            setFunc = [accessor, setter = NativeWrapper::GetSetter(Type)](void* ptr, Arg val)
            {
                auto& parent = *reinterpret_cast<T*>(ptr);
                return setter(reinterpret_cast<uintptr_t>(accessor(parent)), 0, std::move(val));
            };
        }
        dst->SetGetSet([accessor, getter = NativeWrapper::GetGetter(Type, !IsPtr)](void* ptr) -> Arg
        {
            auto& parent = *reinterpret_cast<T*>(ptr);
            if constexpr (IsPtr)
            {
                return getter(reinterpret_cast<uintptr_t>(accessor(parent)), 0);
            }
            else
            {
                auto tmp = accessor(parent);
                return getter(reinterpret_cast<uintptr_t>(&tmp), 0);
            }
        }, std::move(setFunc));
        return *dst;
    }
    template<typename F>
    void SetAssigner(F&& assigner)
    {
        static_assert(std::is_invocable_r_v<void, F, T&, Arg>, "assigner should accept T&");
        //using R = std::invoke_result_t<F, T&, Arg>;
        Assigner = [assigner = std::move(assigner)](void* ptr, Arg val)
        {
            auto& host = *reinterpret_cast<T*>(ptr);
            //if constexpr (std::is_void_v<R>)
                assigner(host, std::move(val));
            /*else
            {
                const bool ret = assigner(host, std::move(val));
                if (!ret)

            }*/
        };
    }
    CustomVar CreateVar(const T& obj)
    {
        const auto ptr     = reinterpret_cast<uintptr_t>(&obj);
        return { this, ptr, UINT32_MAX, 0 };
    }
    CustomVar CreateVar(T& obj)
    {
        auto var = CreateVar(const_cast<const T&>(obj));
        var.Meta2 = common::enum_cast(VarFlags::NonConst);
        return var;
    }
    CustomVar CreateVar(common::span<const T> objs)
    {
        Expects(objs.size() < UINT32_MAX);
        const auto ptr     = reinterpret_cast<uintptr_t>(objs.data());
        const auto eleNum  = static_cast<uint32_t>(objs.size());
        return { this, ptr, eleNum, 0 };
    }
    CustomVar CreateVar(common::span<T> objs)
    {
        auto var = CreateVar(common::span<const T>(objs));
        var.Meta2 = common::enum_cast(VarFlags::NonConst);
        return var;
    }
};


}