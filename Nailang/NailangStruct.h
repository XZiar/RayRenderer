#pragma once

#include "NailangRely.h"


namespace xziar::nailang
{


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


enum class EmbedOps : uint8_t 
{ 
    // binary
    Equal = 0, NotEqual, Less, LessEqual, Greater, GreaterEqual, And, Or, Add, Sub, Mul, Div, Rem, ValueOr, 
    // unary
    CheckExist = 128, Not 
};
struct EmbedOpHelper
{
    enum class OpCategory { Other = 0, Compare = 1, Logic = 2, Arth = 3 };
    [[nodiscard]] static constexpr bool IsUnaryOp(EmbedOps op) noexcept
    {
        return common::enum_cast(op) >= 128;
    }
    [[nodiscard]] static constexpr OpCategory GetCategory(EmbedOps op) noexcept
    {
        if (common::enum_cast(op) <= common::enum_cast(EmbedOps::GreaterEqual))
            return OpCategory::Compare;
        if (common::enum_cast(op) <= common::enum_cast(EmbedOps::Or) || op == EmbedOps::Not)
            return OpCategory::Logic;
        if (common::enum_cast(op) <= common::enum_cast(EmbedOps::Rem))
            return OpCategory::Arth;
        return OpCategory::Other;
    }
    [[nodiscard]] static std::u32string_view GetOpName(EmbedOps op) noexcept;
};


struct FuncCall;
struct UnaryExpr;
struct BinaryExpr;
struct TernaryExpr;
struct QueryExpr;
struct AssignExpr;

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

struct Expr
{
    static_assert( sizeof(detail::IntFPUnion) ==  sizeof(uint64_t));
    static_assert(alignof(detail::IntFPUnion) == alignof(uint64_t));
protected:
    detail::IntFPUnion Data1;
    uint32_t Data2;
    uint16_t Data3;
public:
    enum class Type : uint8_t { Empty = 0, Var, Str, Uint, Int, FP, Bool, Func, Unary, Binary, Ternary, Query, Assign };
    uint8_t ExtraFlag;
    Type TypeData;
#define Fill1(type) ExtraFlag(0), TypeData(Type::type)
#define Fill2(type) Data3(common::enum_cast(Type::type)), Fill1(type)
#define Fill3(type) Data2(common::enum_cast(Type::type)), Fill2(type)
public:
    constexpr Expr() noexcept : Data1(), Fill3(Empty) {}
    Expr(const FuncCall* ptr) noexcept : 
        Data1(reinterpret_cast<uint64_t>(ptr)), Fill3(Func) {}
    Expr(const UnaryExpr* ptr) noexcept :
        Data1(reinterpret_cast<uint64_t>(ptr)), Fill3(Unary) {}
    Expr(const BinaryExpr* ptr) noexcept :
        Data1(reinterpret_cast<uint64_t>(ptr)), Fill3(Binary) {}
    Expr(const TernaryExpr* ptr) noexcept :
        Data1(reinterpret_cast<uint64_t>(ptr)), Fill3(Ternary) {}
    Expr(const QueryExpr* ptr) noexcept :
        Data1(reinterpret_cast<uint64_t>(ptr)), Fill3(Query) {}
    Expr(const AssignExpr* ptr) noexcept :
        Data1(reinterpret_cast<uint64_t>(ptr)), Fill3(Assign) {}
    Expr(const LateBindVar& var) noexcept :
        Data1(reinterpret_cast<uint64_t>(var.Name.data())), Data2(static_cast<uint32_t>(var.Name.size())),
        Data3(common::enum_cast(var.Info)), Fill1(Var) 
    {
        Expects(var.Name.size() <= UINT32_MAX);
    }
    Expr(const std::u32string_view str) noexcept :
        Data1(reinterpret_cast<uint64_t>(str.data())), Data2(static_cast<uint32_t>(str.size())), Fill2(Str)
    {
        Expects(str.size() <= UINT32_MAX);
    }
    Expr(const uint64_t num) noexcept :
        Data1(num), Fill3(Uint) {}
    Expr(const int64_t num) noexcept :
        Data1(static_cast<uint64_t>(num)), Fill3(Int) {}
    Expr(const double num) noexcept :
        Data1(num), Fill3(FP) {}
    Expr(const bool boolean) noexcept :
        Data1(boolean ? 1 : 0), Fill3(Bool) {}
#undef Fill3
#undef Fill2
#undef Fill1
    template<Type T>
    [[nodiscard]] constexpr decltype(auto) GetVar() const noexcept
    {
        Expects(TypeData == T);
        if constexpr (T == Type::Var)
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
        else if constexpr (T == Type::Func)
            return reinterpret_cast<const FuncCall*>(Data1.Uint);
        else if constexpr (T == Type::Unary)
            return reinterpret_cast<const UnaryExpr*>(Data1.Uint);
        else if constexpr (T == Type::Binary)
            return reinterpret_cast<const BinaryExpr*>(Data1.Uint);
        else if constexpr (T == Type::Ternary)
            return reinterpret_cast<const TernaryExpr*>(Data1.Uint);
        else if constexpr (T == Type::Query)
            return reinterpret_cast<const QueryExpr*>(Data1.Uint);
        else if constexpr (T == Type::Assign)
            return reinterpret_cast<const AssignExpr*>(Data1.Uint);
        else
            static_assert(!common::AlwaysTrue2<T>, "Unknown Type");
    }

    using Variant = std::variant<
        const FuncCall*, const UnaryExpr*, const BinaryExpr*, const TernaryExpr*, const QueryExpr*, const AssignExpr*,
        LateBindVar, std::u32string_view, uint64_t, int64_t, double, bool>;
    [[nodiscard]] forceinline Variant GetVar() const noexcept
    {
        switch (TypeData)
        {
        case Type::Var:     return GetVar<Type::Var>();
        case Type::Str:     return GetVar<Type::Str>();
        case Type::Uint:    return GetVar<Type::Uint>();
        case Type::Int:     return GetVar<Type::Int>();
        case Type::FP:      return GetVar<Type::FP>();
        case Type::Bool:    return GetVar<Type::Bool>();
        case Type::Func:    return GetVar<Type::Func>();
        case Type::Unary:   return GetVar<Type::Unary>();
        case Type::Binary:  return GetVar<Type::Binary>();
        case Type::Ternary: return GetVar<Type::Ternary>();
        case Type::Query:   return GetVar<Type::Query>();
        case Type::Assign:  return GetVar<Type::Assign>();
        default:            Expects(false); return {};
        }
    }
    template<typename Visitor>
    [[nodiscard]] forceinline auto Visit(Visitor&& visitor) const
    {
        switch (TypeData)
        {
        case Type::Var:     return visitor(GetVar<Type::Var>());
        case Type::Str:     return visitor(GetVar<Type::Str>());
        case Type::Uint:    return visitor(GetVar<Type::Uint>());
        case Type::Int:     return visitor(GetVar<Type::Int>());
        case Type::FP:      return visitor(GetVar<Type::FP>());
        case Type::Bool:    return visitor(GetVar<Type::Bool>());
        case Type::Func:    return visitor(GetVar<Type::Func>());
        case Type::Unary:   return visitor(GetVar<Type::Unary>());
        case Type::Binary:  return visitor(GetVar<Type::Binary>());
        case Type::Ternary: return visitor(GetVar<Type::Ternary>());
        case Type::Query:   return visitor(GetVar<Type::Query>());
        case Type::Assign:  return visitor(GetVar<Type::Assign>());
        default:            return visitor(std::nullopt);
        }
    }

    [[nodiscard]] forceinline std::u32string_view GetTypeName() const noexcept
    {
        return TypeName(TypeData);
    }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return TypeData != Expr::Type::Empty; }
    [[nodiscard]] NAILANGAPI static std::u32string_view TypeName(const Type type) noexcept;
};


template<typename T>
struct SubQuery
{
    common::span<const Expr> Raw;
    common::span<T> Queries;
    constexpr SubQuery<T>& Consume(const size_t count = 1) noexcept
    {
        Expects(count <= Queries.size());
        Raw = Raw.subspan(count);
        Queries = Queries.subspan(count);
        return *this;
    }
    constexpr T& operator[](size_t idx) const noexcept
    {
        Expects(idx < Queries.size());
        return Queries[idx];
    }
    constexpr const Expr& GetRaw(size_t idx) const noexcept
    {
        Expects(idx < Raw.size());
        return Raw[idx];
    }
    constexpr size_t Size() const noexcept
    {
        return Queries.size();
    }
};


enum class CompareResultCore : uint32_t
{
    ResultMask = 0x0f, NotEqual = 0x0, Less = 0x1, Equal = 0x2, Greater = 0x3,
    FlagMask = 0xf0, Equality = 0x10, Orderable = 0x20, Reversable = 0x40, StrongOrder = 0x70,
};
MAKE_ENUM_BITFIELD(CompareResultCore)

struct CompareResult
{
    CompareResultCore Result;
    constexpr CompareResult() noexcept : Result(CompareResultCore::NotEqual) { }
    constexpr CompareResult(CompareResultCore val) noexcept : Result(val) { }
    constexpr bool HasEuqality() const noexcept
    {
        return HAS_FIELD(Result, CompareResultCore::Equality);
    }
    constexpr bool HasOrderable() const noexcept
    {
        return HAS_FIELD(Result, CompareResultCore::Orderable);
    }
    constexpr bool HasReversable() const noexcept
    {
        return HAS_FIELD(Result, CompareResultCore::Reversable);
    }
    constexpr bool HasReverseOrderable() const noexcept
    {
        return MATCH_FIELD(Result, CompareResultCore::Orderable | CompareResultCore::Reversable);
    }
    constexpr CompareResultCore GetResult() const noexcept
    {
        return Result & CompareResultCore::ResultMask;
    }
    constexpr bool IsEqual() const noexcept
    {
        if (HasEuqality())
            return (Result & CompareResultCore::ResultMask) == CompareResultCore::Equal;
        return false;
    }
    constexpr bool IsNotEqual() const noexcept
    {
        if (HasEuqality())
            return (Result & CompareResultCore::ResultMask) == CompareResultCore::NotEqual;
        return false;
    }
    constexpr bool IsLess() const noexcept
    {
        if (HasOrderable())
            return (Result & CompareResultCore::ResultMask) == CompareResultCore::Less;
        return false;
    }
    constexpr bool IsGreater() const noexcept
    {
        if (HasOrderable())
            return (Result & CompareResultCore::ResultMask) == CompareResultCore::Greater;
        return false;
    }
    constexpr bool IsLessEqual() const noexcept
    {
        if (HasOrderable())
        {
            const auto real = Result & CompareResultCore::ResultMask;
            return real == CompareResultCore::Less || real == CompareResultCore::Equal;
        }
        return false;
    }
    constexpr bool IsGreaterEqual() const noexcept
    {
        if (HasOrderable())
        {
            const auto real = Result & CompareResultCore::ResultMask;
            return real == CompareResultCore::Greater || real == CompareResultCore::Equal;
        }
        return false;
    }
};


struct Arg;

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
    [[nodiscard]] Arg CopyToArg() const noexcept;
};


enum class ArgAccess : uint16_t
{
    Readable = 0b1, NonConst = 0b10, Assignable = 0b110,
    Empty = 0, ReadOnly = Readable, WriteOnly = NonConst | Assignable,
    Mutable = Readable | NonConst, ReadWrite = Readable | Assignable
};
MAKE_ENUM_BITFIELD(ArgAccess)

struct GetSet
{
    struct Handler
    {
        [[nodiscard]] virtual ArgAccess GetAccess(const GetSet&) const noexcept = 0;
        [[nodiscard]] virtual Arg Get(const GetSet&) const = 0;
        [[nodiscard]] virtual bool Set(const GetSet&, Arg) const = 0;
        [[nodiscard]] forceinline Arg Get(const void* ptr, size_t idx, uint16_t meta) const;
        template<typename T>
        [[nodiscard]] forceinline bool Set(void* ptr, size_t idx, uint16_t meta, T&& val) const;
    };
    alignas(detail::IntFPUnion) const Handler* Host;
    uint64_t Ptr;
    uint32_t Idx;
    uint16_t Meta;
    [[nodiscard]] forceinline ArgAccess GetAccess() const noexcept { return Host->GetAccess(*this); }
    [[nodiscard]] forceinline Arg Get() const;
    template<typename T>
    [[nodiscard]] forceinline bool Set(T&& val) const { return Host->Set(*this, std::forward<T>(val)); }
};
template<typename T>
[[nodiscard]] inline bool GetSet::Handler::Set(void* ptr, size_t idx, uint16_t meta, T&& val) const
{
    const GetSet tmp{ this, reinterpret_cast<uintptr_t>(ptr), gsl::narrow_cast<uint32_t>(idx), meta };
    return Set(tmp, std::forward<T>(val));
}


struct NativeWrapper
{
    enum class Type : uint16_t
    {
        Any = 0, Bool,
        U8, I8, U16, I16, F16, U32, I32, F32, U64, I64, F64,
        Str8, Str16, Str32, Sv8, Sv16, Sv32
    };
    struct MetaType
    {
        static constexpr uint16_t EnableGet = 0x1;
        static constexpr uint16_t EnableSet = 0x2;
        static constexpr uint16_t MoveGet = 0x4;
    };
    [[nodiscard]] NAILANGAPI static std::u32string_view TypeName(Type type) noexcept;
    [[nodiscard]] NAILANGAPI static const GetSet::Handler* GetGetSet(Type type) noexcept;
    [[nodiscard]] static uint16_t GetMeta(bool canGet, bool canSet, bool moveGet) noexcept
    {
        uint16_t ret = 0;
        if (canGet)
        {
            ret |= MetaType::EnableGet;
            if (moveGet) ret |= MetaType::MoveGet;
        }
        if (canSet) ret |= MetaType::EnableSet;
        return ret;
    }
    [[nodiscard]] static GetSet GetLocator(Type type, uintptr_t pointer, bool isConst, size_t idx = 0) noexcept
    {
        return { GetGetSet(type), static_cast<uint64_t>(pointer), static_cast<uint32_t>(idx), GetMeta(true, !isConst, false) };
    }
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


struct ArrarRef
{
    using Type = NativeWrapper::Type;
    uint64_t DataPtr;
    uint32_t Length;
    Type ElementType;
    bool IsReadOnly;
    Arg Access(size_t idx) const noexcept;
    Arg Get(size_t idx) const noexcept;
    template<typename T>
    bool Set(size_t idx, T&& val) const noexcept;
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
    static ArrarRef Create(common::span<T> elements)
    {
        Expects(elements.size() < UINT32_MAX);
        static constexpr bool IsConst = std::is_const_v<T>;
        using R = std::remove_const_t<T>;
        return { reinterpret_cast<uintptr_t>(elements.data()), static_cast<uint32_t>(elements.size()), 
            NativeWrapper::GetType<R>(), IsConst };
    }
};


class NAILANGAPI NailangExecutor;
struct Arg
{
    static_assert(sizeof(uintptr_t) <= sizeof(uint64_t));
    static_assert(sizeof(size_t)    <= sizeof(uint64_t));
    static_assert(sizeof(std::u32string_view) == sizeof(common::SharedString<char32_t>));
public:
    enum class Type : uint16_t
    {
        Empty = 0, CategoryMask = 0xf000, MainCategoryMask = 0xc000, CapabilityMask = 0xf00, ExtendedMask = 0xf0, FlagMask = 0xf,
        CategoryCustom = 0x8000, CategoryArray = 0x7000, CategoryString = 0x6000, CategoryNumber = 0x2000, CategoryBool = 0x1000,
        CategoryGetSet = 0xf000, CategoryArgPtr = 0xe000,
        BoolBit = 0x100, NumberBit = 0x200, StringBit = 0x400, ArrayBit = 0x800,
        FPBit = 0x10, IntegerBit = 0x20, UnsignedBit = 0x40,
        ExTypeBit = 0x8, OwnershipBit = 0x4, ConstBit = 0x1,
        Boolable = BoolBit | ExTypeBit, String = StringBit | BoolBit | ExTypeBit, Number = NumberBit | BoolBit | ExTypeBit, 
        Integer = Number | IntegerBit, ArrayLike = ArrayBit | ExTypeBit,
        Bool   = CategoryBool   | Boolable,
        FP     = CategoryNumber | Number    | FPBit,
        Uint   = CategoryNumber | Integer   | UnsignedBit,
        Int    = CategoryNumber | Integer,
        U32Sv  = CategoryString | String,
        U32Str = CategoryString | String    | OwnershipBit,
        Array  = CategoryArray  | ArrayLike,
        Var    = CategoryCustom | ExTypeBit | OwnershipBit,
    };
private:
    detail::IntFPUnion Data0;
    uint64_t Data1;
    uint32_t Data2;
    uint16_t Data3;
    NAILANGAPI void Release() noexcept;
public:
    Type TypeData;

    Arg() noexcept : Data0(0), Data1(0), Data2(0), Data3(0), TypeData(Type::Empty)
    { }
    Arg(const GetSet& getset) noexcept : Data0(reinterpret_cast<uint64_t>(getset.Host)),
        Data1(getset.Ptr), Data2(getset.Idx), Data3(getset.Meta), TypeData(getset.Host ? Type::CategoryGetSet : Type::Empty)
    { }
    Arg(Arg* arg, ArgAccess access = ArgAccess::ReadWrite) noexcept : Data0(reinterpret_cast<uint64_t>(arg)),
        Data1(0), Data2(0), Data3(common::enum_cast(access)), TypeData(arg ? Type::CategoryArgPtr : Type::Empty)
    { }
    Arg(const Arg* arg) noexcept : Data0(reinterpret_cast<uint64_t>(arg)),
        Data1(0), Data2(0), Data3(common::enum_cast(ArgAccess::ReadOnly)), TypeData(arg ? Type::CategoryArgPtr : Type::Empty)
    { }
    Arg(const CustomVar& var) noexcept = delete;
    Arg(CustomVar&& var, bool isConst = false) noexcept;
    Arg(const ArrarRef arr) noexcept;
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
    ~Arg();
    Arg(Arg&& other) noexcept :
        Data0(other.Data0), Data1(other.Data1), Data2(other.Data2), Data3(other.Data3), TypeData(other.TypeData)
    {
        other.TypeData = Type::Empty;
    }
    forceinline Arg& operator=(Arg&& other) noexcept;
    NAILANGAPI Arg(const Arg& other) noexcept;
    NAILANGAPI Arg& operator=(const Arg& other) noexcept;


    template<Type T, bool Check = true>
    [[nodiscard]] constexpr auto GetVar() const noexcept;
    template<typename Visitor>
    [[nodiscard]] forceinline auto Visit(Visitor&& visitor) const;

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
    [[nodiscard]] forceinline constexpr bool IsArray () const noexcept;
    [[nodiscard]] forceinline constexpr bool IsCustom() const noexcept;
    [[nodiscard]] forceinline constexpr bool IsGetSet() const noexcept;
    [[nodiscard]] forceinline constexpr bool IsArgPtr() const noexcept;
    [[nodiscard]] forceinline constexpr bool IsNative() const noexcept;
    template<typename T>
    [[nodiscard]] forceinline bool IsCustomType() const noexcept
    {
        static_assert(std::is_base_of_v<CustomVar::Handler, T>, "type need to inherit from Handler");
        if (!IsCustom()) return false;
        return GetCustom().IsType<T>();
    }
    
    [[nodiscard]] forceinline CustomVar& GetCustom() noexcept
    {
        return *reinterpret_cast<CustomVar*>(this);
    }
    [[nodiscard]] forceinline const CustomVar& GetCustom() const noexcept
    {
        return *reinterpret_cast<const CustomVar*>(this);
    }
    [[nodiscard]] forceinline const GetSet& GetGetSet() const noexcept
    {
        return *reinterpret_cast<const GetSet*>(this);
    }
    [[nodiscard]] NAILANGAPI std::optional<bool>                GetBool()   const noexcept;
    [[nodiscard]] NAILANGAPI std::optional<uint64_t>            GetUint()   const noexcept;
    [[nodiscard]] NAILANGAPI std::optional<int64_t>             GetInt()    const noexcept;
    [[nodiscard]] NAILANGAPI std::optional<double>              GetFP()     const noexcept;
    [[nodiscard]] NAILANGAPI std::optional<std::u32string_view> GetStr()    const noexcept;
    [[nodiscard]] NAILANGAPI common::str::StrVariant<char32_t>  ToString()  const noexcept;

    [[nodiscard]] forceinline CompareResult Compare(const Arg& right) const noexcept;

    NAILANGAPI void Decay();
    NAILANGAPI Arg DecayTo();
    NAILANGAPI bool Set(Arg val);
    [[nodiscard]] forceinline ArgAccess GetAccess() const noexcept;
    [[nodiscard]] NAILANGAPI Arg HandleSubFields(SubQuery<const Expr>& subfields);
    [[nodiscard]] NAILANGAPI Arg HandleIndexes(SubQuery<Arg>& indexes);
    [[nodiscard]] NAILANGAPI CompareResult NativeCompare(const Arg& right) const noexcept;
    [[nodiscard]] NAILANGAPI Arg HandleUnary(const EmbedOps op) const;
    [[nodiscard]] NAILANGAPI Arg HandleBinary(const EmbedOps op, const Arg& right) const;
    [[nodiscard]] NAILANGAPI bool HandleBinaryOnSelf(const EmbedOps op, const Arg& right);

    [[nodiscard]] std::u32string_view GetTypeName() const noexcept;
    [[nodiscard]] NAILANGAPI static std::u32string_view TypeName(const Type type) noexcept;
};
MAKE_ENUM_BITFIELD(Arg::Type)
static_assert( sizeof(Arg) ==  sizeof(CustomVar));
static_assert(alignof(Arg) == alignof(CustomVar));
static_assert( sizeof(Arg) ==  sizeof(GetSet));
static_assert(alignof(Arg) == alignof(GetSet));

struct NAILANGAPI CustomVar::Handler
{
    [[nodiscard]] virtual std::u32string_view GetTypeName(const CustomVar&) noexcept;
    virtual void IncreaseRef(const CustomVar&) noexcept;
    virtual void DecreaseRef(CustomVar&) noexcept;
    [[nodiscard]] virtual Arg::Type QueryConvertSupport(const CustomVar&) noexcept;
    [[nodiscard]] virtual Arg IndexerGetter(const CustomVar&, const Arg&, const Expr&);
    [[nodiscard]] virtual Arg SubfieldGetter(const CustomVar&, std::u32string_view);
    [[nodiscard]] virtual Arg HandleSubFields(const CustomVar&, SubQuery<const Expr>&);
    [[nodiscard]] virtual Arg HandleIndexes(const CustomVar&, SubQuery<Arg>&);
    [[nodiscard]] virtual bool HandleBinaryOnSelf(CustomVar&, const EmbedOps op, const Arg & right);
    [[nodiscard]] virtual bool HandleAssign(CustomVar&, Arg);
    [[nodiscard]] virtual CompareResult CompareSameClass(const CustomVar&, const CustomVar&);
    [[nodiscard]] virtual CompareResult Compare(const CustomVar&, const Arg&);
    [[nodiscard]] virtual common::str::StrVariant<char32_t> ToString(const CustomVar&) noexcept;
    [[nodiscard]] virtual Arg ConvertToCommon(const CustomVar&, Arg::Type) noexcept;
};

inline Arg::Arg(CustomVar&& var, bool isConst) noexcept : Data0(reinterpret_cast<uint64_t>(var.Host)),
    Data1(var.Meta0), Data2(var.Meta1), Data3(var.Meta2), TypeData((isConst ? Type::ConstBit : Type::Empty) | Type::Var)
{
    Expects(var.Host != nullptr);
    var.Host = nullptr;
    var.Meta0 = var.Meta1 = var.Meta2 = 0;
}
inline Arg::Arg(const ArrarRef arr) noexcept :
    Data0(arr.DataPtr), Data1(0), Data2(arr.Length), Data3(common::enum_cast(arr.ElementType)),
    TypeData(Type::Array | (arr.IsReadOnly ? Type::ConstBit : Type::Empty))
{ }
inline Arg::~Arg()
{
    if (HAS_FIELD(TypeData, Type::OwnershipBit))
        Release();
}
inline Arg& Arg::operator=(Arg&& other) noexcept
{
    if (HAS_FIELD(TypeData, Type::OwnershipBit))
        Release();
    Data0 = other.Data0;
    Data1 = other.Data1;
    Data2 = other.Data2;
    Data3 = other.Data3;
    TypeData = other.TypeData;
    other.TypeData = Type::Empty;
    return *this;
}
template<Arg::Type T, bool Check>
[[nodiscard]] inline constexpr auto Arg::GetVar() const noexcept
{
    if constexpr (Check)
    {
        const auto target = common::enum_cast(T);
        const auto type = common::enum_cast(TypeData);
        Expects((type & target) == target);
    }
    if constexpr (T == Type::Var)
        return CustomVar{ reinterpret_cast<CustomVar::Handler*>(Data0.Uint), Data1, Data2, Data3 };
    else if constexpr (T == Type::Array)
        return ArrarRef{ static_cast<uintptr_t>(Data0.Uint), Data2, static_cast<ArrarRef::Type>(Data3), HAS_FIELD(TypeData, Type::ConstBit) };
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
template<typename Visitor>
[[nodiscard]] inline auto Arg::Visit(Visitor&& visitor) const
{
    switch (REMOVE_MASK(TypeData, Type::ConstBit))
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
[[nodiscard]] inline constexpr bool Arg::IsArray () const noexcept
{
    return (TypeData & Type::CategoryMask) == Type::CategoryArray;
}
[[nodiscard]] inline constexpr bool Arg::IsCustom() const noexcept
{
    return (TypeData & Type::CategoryMask) == Type::CategoryCustom;
}
[[nodiscard]] inline constexpr bool Arg::IsGetSet() const noexcept
{
    return (TypeData & Type::CategoryMask) == Type::CategoryGetSet;
}
[[nodiscard]] inline constexpr bool Arg::IsArgPtr() const noexcept
{
    return (TypeData & Type::CategoryMask) == Type::CategoryArgPtr;
}
[[nodiscard]] inline constexpr bool Arg::IsNative() const noexcept
{
    const auto category = TypeData & Type::CategoryMask;
    return category == Type::CategoryBool || category == Type::CategoryNumber;
}
[[nodiscard]] inline CompareResult Arg::Compare(const Arg& right) const noexcept
{
    if (IsCustom())
        return GetCustom().Call<&CustomVar::Handler::Compare>(right);
    return NativeCompare(right);
}
[[nodiscard]] inline ArgAccess Arg::GetAccess() const noexcept
{
    if (IsGetSet())
        return GetGetSet().GetAccess();
    else if (IsArgPtr())
        return static_cast<ArgAccess>(Data3);
    else if (IsCustom() || IsArray())
        return HAS_FIELD(TypeData, Type::ConstBit) ? ArgAccess::ReadOnly : ArgAccess::ReadWrite;
    return ArgAccess::ReadOnly; // str and native are readonly
}
[[nodiscard]] inline std::u32string_view Arg::GetTypeName() const noexcept
{
    if (IsCustom())
        return GetCustom().Call<&CustomVar::Handler::GetTypeName>();
    return TypeName(TypeData);
}

[[nodiscard]] inline Arg CustomVar::CopyToArg() const noexcept
{
    Host->IncreaseRef(*this);
    auto self = *this;
    return self;
}
[[nodiscard]] inline Arg GetSet::Handler::Get(const void* ptr, size_t idx, uint16_t meta) const
{
    const GetSet tmp{ this, reinterpret_cast<uintptr_t>(ptr), gsl::narrow_cast<uint32_t>(idx), meta };
    return Get(tmp);
}
[[nodiscard]] inline Arg GetSet::Get() const 
{ 
    return Host->Get(*this); 
}
inline Arg ArrarRef::Access(size_t idx) const noexcept
{
    Expects(idx < Length);
    if (ElementType == Type::Any)
        return { reinterpret_cast<Arg*>(DataPtr) + idx, IsReadOnly ? ArgAccess::ReadOnly : ArgAccess::ReadWrite };
    else
        return NativeWrapper::GetLocator(ElementType, DataPtr, IsReadOnly, idx);
}
inline Arg ArrarRef::Get(size_t idx) const noexcept
{
    Expects(idx < Length);
    if (ElementType == Type::Any)
        return reinterpret_cast<Arg*>(DataPtr)[idx];
    else
        return NativeWrapper::GetLocator(ElementType, DataPtr, true, idx).Get();
}
template<typename T>
inline bool ArrarRef::Set(size_t idx, T&& val) const noexcept
{
    Expects(idx < Length);
    if (ElementType == Type::Any)
    {
        if (IsReadOnly) return false;
        reinterpret_cast<Arg*>(DataPtr)[idx] = std::forward<T>(val);
        return true;
    }
    else
        return NativeWrapper::GetLocator(ElementType, DataPtr, IsReadOnly, idx).Set(std::forward<T>(val));
}


struct WithPos
{
    std::pair<uint32_t, uint32_t> Position = { 0,0 };
};


struct FuncName : public PartedName
{
    enum class FuncInfo : uint16_t
    {
        MetaFlag = 0b10,
        Empty = 0b00, ExprPart = 0b01, Meta = MetaFlag | 0b00, PostMeta = MetaFlag | 0b01,
    };

    constexpr FuncInfo Info() const noexcept { return static_cast<FuncInfo>(ExternInfo); }
    constexpr common::EnumWrapper<FuncInfo> Info() noexcept { return ExternInfo; }
    constexpr bool IsMeta() const noexcept;

    static FuncInfo PrepareFuncInfo(std::u32string_view& name, FuncName::FuncInfo info) noexcept
    {
        if (info == FuncInfo::Meta && !name.empty() && name[0] == ':')
        {
            name.remove_prefix(1);
            return FuncInfo::PostMeta;
        }
        return info;
    }
    static FuncName* Create(MemoryPool& pool, std::u32string_view name, FuncInfo info)
    {
        if (name.size() == 0)
            return nullptr;
        return static_cast<FuncName*>(PartedName::Create(pool, name, common::enum_cast(info)));
    }
    static TempPartedName<FuncName> CreateTemp(std::u32string_view name, FuncInfo info)
    {
        return TempPartedName<FuncName>(name, common::enum_cast(info));
    }
};
MAKE_ENUM_BITFIELD(FuncName::FuncInfo)
inline constexpr bool FuncName::IsMeta() const noexcept 
{ 
    return HAS_FIELD(Info(), FuncInfo::MetaFlag);
}
using TempFuncName = TempPartedName<FuncName>;


struct FuncCall : public WithPos
{
    const FuncName* Name;
    common::span<const Expr> Args;
    constexpr FuncCall() noexcept : Name(nullptr) {}
    constexpr FuncCall(const FuncName* name, common::span<const Expr> args) noexcept : Name(name), Args(args) { }
    constexpr FuncCall(const FuncName* name, common::span<const Expr> args, const std::pair<uint32_t, uint32_t> pos) noexcept :
        WithPos{pos}, Name(name), Args(args)
    { }
    constexpr const FuncName& GetName() const noexcept { return *Name; }
    constexpr std::u32string_view FullFuncName() const noexcept { return Name->FullName(); }
};
struct UnaryExpr
{
    Expr Operand;
    EmbedOps Operator;
    UnaryExpr(const EmbedOps op, const Expr& operand) noexcept :
        Operand(operand), Operator(op) { }
};
struct BinaryExpr
{
    Expr LeftOperand, RightOperand;
    EmbedOps Operator;
    BinaryExpr(const EmbedOps op, const Expr& left, const Expr& right) noexcept :
        LeftOperand(left), RightOperand(right), Operator(op) { }
};
struct TernaryExpr
{
    Expr Condition, LeftOperand, RightOperand;
    TernaryExpr(const Expr& cond, const Expr& left, const Expr& right) noexcept :
        Condition(cond), LeftOperand(left), RightOperand(right) { }
};
struct QueryExpr
{
    enum class QueryType : uint8_t { Index = 1, Sub = 2 };
    Expr Target;
    const Expr* QueryPtr;
    uint32_t Count;
    QueryType TypeData;
    constexpr QueryExpr(const Expr& target, common::span<const Expr> queries, QueryType type) noexcept :
        Target(target), QueryPtr(queries.data()), Count(gsl::narrow_cast<uint32_t>(queries.size())), TypeData(type)
    { }
    constexpr common::span<const Expr> GetQueries() const noexcept
    {
        return { QueryPtr, Count };
    }
};
struct NilCheck
{
    enum class Behavior : uint32_t { Pass = 0, Skip = 1, Throw = 2 };
    uint8_t Value;
    explicit constexpr NilCheck(uint8_t val) noexcept : Value(val)
    { }
    constexpr NilCheck(Behavior notnull, Behavior null) noexcept :
        Value(static_cast<uint8_t>(common::enum_cast(notnull) | (common::enum_cast(null) << 4)))
    { }
    constexpr NilCheck() noexcept : NilCheck(Behavior::Pass, Behavior::Pass)
    { }
    constexpr Behavior WhenNotNull() const noexcept
    {
        return static_cast<Behavior>(Value & 0x0fu);
    }
    constexpr Behavior WhenNull() const noexcept
    {
        return static_cast<Behavior>((Value & 0xf0u) >> 4);
    }
};
struct AssignExpr : public WithPos
{
    Expr Target;
    Expr Statement;
    uint8_t AssignInfo;
    bool IsSelfAssign;

    constexpr AssignExpr(Expr target, Expr statement, uint8_t info, bool isSelfAssign,
        std::pair<uint32_t, uint32_t> pos = { 0,0 }) noexcept : WithPos{ pos },
        Target(target), Statement(statement), AssignInfo(info), IsSelfAssign(isSelfAssign) { }

    constexpr NilCheck GetCheck() const noexcept
    {
        if (IsSelfAssign)
            return { NilCheck::Behavior::Pass, NilCheck::Behavior::Throw };
        else
            return NilCheck{ AssignInfo };
    }
    constexpr std::optional<EmbedOps> GetSelfAssignOp() const noexcept
    {
        if (IsSelfAssign)
            return static_cast<EmbedOps>(AssignInfo);
        else
            return {};
    }
};


struct RawBlock;
struct Block;

struct Statement
{
    enum class Type : uint16_t { Empty = 0, FuncCall, Assign, RawBlock, Block };
    uintptr_t Pointer;
    uint32_t Offset;
    uint16_t Count;
    Type TypeData;
    constexpr Statement() noexcept : Pointer(0), Offset(0), Count(0), TypeData(Type::Empty)
    { }
    Statement(const FuncCall  * ptr, std::pair<uint32_t, uint16_t> meta = { 0u,(uint16_t)0 }) noexcept :
        Pointer(reinterpret_cast<uintptr_t>(ptr)), Offset(meta.first), Count(meta.second), TypeData(Type::FuncCall)
    { }
    Statement(const AssignExpr* ptr, std::pair<uint32_t, uint16_t> meta = { 0u,(uint16_t)0 }) noexcept :
        Pointer(reinterpret_cast<uintptr_t>(ptr)), Offset(meta.first), Count(meta.second), TypeData(Type::Assign)
    { }
    Statement(const RawBlock  * ptr, std::pair<uint32_t, uint16_t> meta = { 0u,(uint16_t)0 }) noexcept :
        Pointer(reinterpret_cast<uintptr_t>(ptr)), Offset(meta.first), Count(meta.second), TypeData(Type::RawBlock)
    { }
    Statement(const Block     * ptr, std::pair<uint32_t, uint16_t> meta = { 0u,(uint16_t)0 }) noexcept :
        Pointer(reinterpret_cast<uintptr_t>(ptr)), Offset(meta.first), Count(meta.second), TypeData(Type::Block)
    { }
    template<typename T>
    [[nodiscard]] forceinline const T* Get() const noexcept
    {
        return reinterpret_cast<const T*>(Pointer);
    }
    [[nodiscard]] std::variant<const AssignExpr*, const FuncCall*, const RawBlock*, const Block*>
        GetStatement() const noexcept
    {
        switch (TypeData)
        {
        case Type::FuncCall: return Get<  FuncCall>();
        case Type::Assign:   return Get<AssignExpr>();
        case Type::RawBlock: return Get<  RawBlock>();
        case Type::Block:    return Get<     Block>();
        default:             Expects(false); return {};
        }
    }
    template<typename Visitor>
    auto Visit(Visitor&& visitor) const
    {
        switch (TypeData)
        {
        case Type::FuncCall:    return visitor(Get<  FuncCall>());
        case Type::Assign:      return visitor(Get<AssignExpr>());
        case Type::RawBlock:    return visitor(Get<  RawBlock>());
        case Type::Block:       return visitor(Get<     Block>());
        default:Expects(false); return visitor(static_cast<const AssignExpr*>(nullptr));
        }
    }
    [[nodiscard]] forceinline bool operator==(const FuncCall* ptr) const noexcept
    {
        return TypeData == Type::FuncCall && Get<FuncCall>() == ptr;
    }
    [[nodiscard]] forceinline bool operator==(const AssignExpr* ptr) const noexcept
    {
        return TypeData == Type::Assign && Get<AssignExpr>() == ptr;
    }
    [[nodiscard]] forceinline bool operator==(const RawBlock* ptr) const noexcept
    {
        return TypeData == Type::RawBlock && Get<RawBlock>() == ptr;
    }
    [[nodiscard]] forceinline bool operator==(const Block* ptr) const noexcept
    {
        return TypeData == Type::Block && Get<Block>() == ptr;
    }
    template<typename T>
    [[nodiscard]] forceinline bool operator!=(const T* ptr) const noexcept
    {
        return !operator==(ptr);
    }
    std::pair<uint32_t, uint32_t> GetPosition() const noexcept;
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

struct Block : RawBlock
{
private:
    using value_type = std::pair<common::span<const FuncCall>, Statement>;
    [[nodiscard]] constexpr value_type Get(size_t index) const noexcept
    {
        const auto& content = Content[index];
        const auto metafuncs = MetaFuncations.subspan(content.Offset, content.Count);
        return { metafuncs, content };
    }
    using ItType = common::container::IndirectIterator<const Block, value_type, &Block::Get>;
    friend ItType;
public:
    common::span<const FuncCall> MetaFuncations;
    common::span<const Statement> Content;

    [[nodiscard]] constexpr ItType begin() const noexcept
    {
        return { this, 0 };
    }
    [[nodiscard]] constexpr ItType end() const noexcept
    {
        return { this, Size() };
    }
    [[nodiscard]] constexpr value_type operator[](size_t index) const noexcept
    {
        return Get(index);
    }
    [[nodiscard]] constexpr size_t Size() const noexcept { return Content.size(); }
};

inline std::pair<uint32_t, uint32_t> Statement::GetPosition() const noexcept
{
    return Visit([](const auto* ptr) { return ptr->Position; });
}


struct NAILANGAPI Serializer
{
    static void Stringify(std::u32string& output, const Expr& expr, const bool requestParenthese = false);
    static void Stringify(std::u32string& output, const FuncCall* call);
    static void Stringify(std::u32string& output, const UnaryExpr* expr);
    static void Stringify(std::u32string& output, const TernaryExpr* expr, const bool requestParenthese = false);
    static void Stringify(std::u32string& output, const BinaryExpr* expr, const bool requestParenthese = false);
    static void Stringify(std::u32string& output, const QueryExpr* expr);
    static void Stringify(std::u32string& output, const AssignExpr* expr);
    static void Stringify(std::u32string& output, const LateBindVar& var);
    static void Stringify(std::u32string& output, const std::u32string_view str);
    static void Stringify(std::u32string& output, const uint64_t u64);
    static void Stringify(std::u32string& output, const int64_t i64);
    static void Stringify(std::u32string& output, const double f64);
    static void Stringify(std::u32string& output, const bool boolean);
    static void Stringify(std::u32string& output, const common::span<const Expr> queries, QueryExpr::QueryType type);

    template<typename... Ts>
    static std::u32string Stringify(const Ts&... obj)
    {
        std::u32string result;
        Stringify(result, obj...);
        return result;
    }
};


}