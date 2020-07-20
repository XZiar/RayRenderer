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
#include "common/EnumEx.hpp"
#include "common/StrBase.hpp"
#include "common/StringLinq.hpp"
#include <vector>
#include <variant>
#include <string>
#include <string_view>
#include <cstdio>
#include <optional>


namespace xziar::nailang
{

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class NAILANGAPI MemoryPool : public common::NonCopyable
{
public:
    std::vector<std::tuple<std::byte*, size_t, size_t>> Trunks;
    size_t TrunkSize;
public:
    MemoryPool(const size_t trunkSize = 2 * 1024 * 1024) : TrunkSize(trunkSize) { }
    MemoryPool(MemoryPool&& other) noexcept = default;
    ~MemoryPool();

    [[nodiscard]] common::span<std::byte> Alloc(const size_t size, const size_t align = 64);
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

#if COMPILER_MSVC
#   pragma warning(pop)
#endif


struct LateBindVar
{
    using value_type = std::u32string_view;
public:

    std::u32string_view Name;
    [[nodiscard]] constexpr bool operator==(const LateBindVar& other) const noexcept
    {
        return Name == other.Name;
    }
    [[nodiscard]] constexpr bool operator==(const std::u32string_view other) const noexcept
    {
        return Name == other;
    }

    [[nodiscard]] constexpr auto Parts() const noexcept
    {
        return common::str::SplitStream(Name, U'.', true);
    }
    [[nodiscard]] constexpr std::u32string_view operator[](size_t index) const noexcept
    {
        return common::str::SplitStream(Name, U'.', true).Skip(index).TryGetFirst().value();
    }
};

enum class EmbedOps : uint8_t { Equal = 0, NotEqual, Less, LessEqual, Greater, GreaterEqual, And, Or, Not, Add, Sub, Mul, Div, Rem };
struct EmbedOpHelper
{
    [[nodiscard]] static constexpr bool IsUnaryOp(EmbedOps op) noexcept
    {
        return op == EmbedOps::Not;
    }
};


struct FuncCall;
struct UnaryExpr;
struct BinaryExpr;

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
private:
    detail::IntFPUnion Data1;
    uint32_t Data2;
public:
    enum class Type : uint32_t { Empty = 0, Func, Unary, Binary, Var, Str, Uint, Int, FP, Bool };
    Type TypeData;

    constexpr RawArg() noexcept : Data1(), Data2(0), TypeData(Type::Empty) {}
    RawArg(const FuncCall* ptr) noexcept : 
        Data1(reinterpret_cast<uint64_t>(ptr)), Data2(1), TypeData(Type::Func) {}
    RawArg(const UnaryExpr* ptr) noexcept :
        Data1(reinterpret_cast<uint64_t>(ptr)), Data2(2), TypeData(Type::Unary) {}
    RawArg(const BinaryExpr* ptr) noexcept :
        Data1(reinterpret_cast<uint64_t>(ptr)), Data2(3), TypeData(Type::Binary) {}
    RawArg(const LateBindVar var) noexcept :
        Data1(reinterpret_cast<uint64_t>(var.Name.data())), Data2(static_cast<uint32_t>(var.Name.size())), TypeData(Type::Var) 
    {
        Expects(var.Name.size() <= UINT32_MAX);
    }
    RawArg(const std::u32string_view str) noexcept :
        Data1(reinterpret_cast<uint64_t>(str.data())), Data2(static_cast<uint32_t>(str.size())), TypeData(Type::Str)
    {
        Expects(str.size() <= UINT32_MAX);
    }
    RawArg(const uint64_t num) noexcept :
        Data1(num), Data2(6), TypeData(Type::Uint) {}
    RawArg(const int64_t num) noexcept :
        Data1(static_cast<uint64_t>(num)), Data2(7), TypeData(Type::Int) {}
    RawArg(const double num) noexcept :
        Data1(num), Data2(8), TypeData(Type::FP) {}
    RawArg(const bool boolean) noexcept :
        Data1(boolean ? 1 : 0), Data2(9), TypeData(Type::Bool) {}

    template<Type T>
    [[nodiscard]] constexpr auto GetVar() const noexcept
    {
        Expects(TypeData == T);
        if constexpr (T == Type::Func)
            return reinterpret_cast<const FuncCall*>(Data1.Uint);
        else if constexpr (T == Type::Unary)
            return reinterpret_cast<const UnaryExpr*>(Data1.Uint);
        else if constexpr (T == Type::Binary)
            return reinterpret_cast<const BinaryExpr*>(Data1.Uint);
        else if constexpr (T == Type::Var)
            return LateBindVar{ {reinterpret_cast<const char32_t*>(Data1.Uint), Data2} };
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

    using Variant = std::variant<const FuncCall*, const UnaryExpr*, const BinaryExpr*, LateBindVar, std::u32string_view, uint64_t, int64_t, double, bool>;
    [[nodiscard]] forceinline Variant GetVar() const noexcept
    {
        switch (TypeData)
        {
        case Type::Func:    return GetVar<Type::Func>();
        case Type::Unary:   return GetVar<Type::Unary>();
        case Type::Binary:  return GetVar<Type::Binary>();
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
        case Type::Var:     return visitor(GetVar<Type::Var>());
        case Type::Str:     return visitor(GetVar<Type::Str>());
        case Type::Uint:    return visitor(GetVar<Type::Uint>());
        case Type::Int:     return visitor(GetVar<Type::Int>());
        case Type::FP:      return visitor(GetVar<Type::FP>());
        case Type::Bool:    return visitor(GetVar<Type::Bool>());
        default:            return visitor(std::nullopt);
        }
    }
};

struct CustomVar : LateBindVar
{
    uint64_t Meta0;
    uint32_t Meta1;
    uint16_t Meta2;
};
struct Arg
{
private:
    std::u32string Str;
    detail::IntFPUnion Data1;
    uint32_t Data2;
    uint16_t Data3;
public:
    enum class Type : uint16_t
    { 
        Empty = 0, RealType = 0x8000, 
        Custom = 0b10000000, Boolable = 0b1, String = 0b11, Number = 0b101, Integer = 0b1101, 
        Bool   = RealType | Boolable,
        U32Sv  = RealType | String   | 0x000,
        U32Str = RealType | String   | 0x100,
        FP     = RealType | Number   | 0x000,
        Uint   = RealType | Integer  | 0x100,
        Int    = RealType | Integer  | 0x000,
        Var    = RealType | Custom,
    };
    Type TypeData;

    Arg() noexcept : Data1(0), Data2(0), Data3(0), TypeData(Type::Empty)
    { }
    Arg(const CustomVar var) noexcept :
        Str(var.Name), Data1(var.Meta0), Data2(var.Meta1), Data3(var.Meta2), TypeData(Type::Var)
    { }
    Arg(const std::u32string& str) noexcept : Str(str), Data1(0), Data2(0), Data3(0), TypeData(Type::U32Str)
    { }
    Arg(const std::u32string_view str) noexcept :
        Data1(reinterpret_cast<uint64_t>(str.data())), Data2(gsl::narrow_cast<uint32_t>(str.size())),
        Data3(0), TypeData(Type::U32Sv)
    {
        Expects(str.size() <= UINT32_MAX);
    }
    Arg(const uint64_t num) noexcept :
        Data1(num), Data2(6), Data3(0), TypeData(Type::Uint)
    { }
    Arg(const int64_t num) noexcept :
        Data1(static_cast<uint64_t>(num)), Data2(7), Data3(0), TypeData(Type::Int)
    { }
    Arg(const double num) noexcept :
        Data1(num), Data2(8), Data3(0), TypeData(Type::FP)
    { }
    Arg(const bool boolean) noexcept :
        Data1(boolean ? 1 : 0), Data2(9), Data3(0), TypeData(Type::Bool)
    { }

    template<Type T>
    [[nodiscard]] constexpr auto GetVar() const noexcept
    {
        Expects(TypeData == T);
        if constexpr (T == Type::Var)
            return CustomVar{ std::u32string_view{Str}, Data1.Uint, Data2, Data3 };
        else if constexpr (T == Type::U32Str)
            return std::u32string_view{ Str };
        else if constexpr (T == Type::U32Sv)
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

    template<typename Visitor>
    [[nodiscard]] forceinline auto Visit(Visitor&& visitor) const
    {
        switch (TypeData)
        {
        case Type::Var:     return visitor(GetVar<Type::Var>());
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
    [[nodiscard]] forceinline constexpr std::optional<bool> GetBool() const noexcept
    {
        switch (TypeData)
        {
        case Type::Uint:    return  GetVar<Type::Uint>() != 0;
        case Type::Int:     return  GetVar<Type::Int>() != 0;
        case Type::FP:      return  GetVar<Type::FP>() != 0;
        case Type::Bool:    return  GetVar<Type::Bool>();
        case Type::U32Str:  return !GetVar<Type::U32Str>().empty();
        case Type::U32Sv:   return !GetVar<Type::U32Sv>().empty();
        default:                    return {};
        }
    }
    [[nodiscard]] forceinline constexpr std::optional<uint64_t> GetUint() const noexcept
    {
        switch (TypeData)
        {
        case Type::Uint:    return GetVar<Type::Uint>();
        case Type::Int:     return static_cast<uint64_t>(GetVar<Type::Int>());
        case Type::FP:      return static_cast<uint64_t>(GetVar<Type::FP>());
        case Type::Bool:    return GetVar<Type::Bool>() ? 1 : 0;
        default:                    return {};
        }
    }
    [[nodiscard]] forceinline constexpr std::optional<int64_t> GetInt() const noexcept
    {
        switch (TypeData)
        {
        case Type::Uint:    return static_cast<int64_t>(GetVar<Type::Uint>());
        case Type::Int:     return GetVar<Type::Int>();
        case Type::FP:      return static_cast<int64_t>(GetVar<Type::FP>());
        case Type::Bool:    return GetVar<Type::Bool>() ? 1 : 0;
        default:                    return {};
        }
    }
    [[nodiscard]] forceinline constexpr std::optional<double> GetFP() const noexcept
    {
        switch (TypeData)
        {
        case Type::Uint:    return static_cast<double>(GetVar<Type::Uint>());
        case Type::Int:     return static_cast<double>(GetVar<Type::Int>());
        case Type::FP:      return GetVar<Type::FP>();
        case Type::Bool:    return GetVar<Type::Bool>() ? 1. : 0.;
        default:                    return {};
        }
    }
    [[nodiscard]] forceinline constexpr std::optional<std::u32string_view> GetStr() const noexcept
    {
        switch (TypeData)
        {
        case Type::U32Str:  return GetVar<Type::U32Str>();
        case Type::U32Sv:   return GetVar<Type::U32Sv>();
        default:                    return {};
        }
    }
    [[nodiscard]] NAILANGAPI common::str::StrVariant<char32_t> ToString() const noexcept;
};
MAKE_ENUM_BITFIELD(Arg::Type)


struct WithPos
{
    std::pair<uint32_t, uint32_t> Position = { 0,0 };
};

struct FuncCall : public WithPos
{
    std::u32string_view Name;
    common::span<const RawArg> Args;
    constexpr FuncCall() noexcept {}
    constexpr FuncCall(const std::u32string_view name, common::span<const RawArg> args) noexcept : Name(name), Args(args) { }
    constexpr FuncCall(const std::u32string_view name, common::span<const RawArg> args, const uint32_t row, const uint32_t col) noexcept :
        WithPos{ {row, col} }, Name(name), Args(args)
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

struct Assignment
{
    std::u32string_view Variable_;
    RawArg Statement;
    constexpr std::u32string_view GetVar() const noexcept
    { 
        return { Variable_.data(), Variable_.size() / 2 };
    }
    constexpr bool IsNilCheck() const noexcept
    {
        return Variable_.size() % 2 == 1;
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
    static void Stringify(std::u32string& output, const RawArg& arg, const bool requestParenthese = false);
    static void Stringify(std::u32string& output, const FuncCall* call);
    static void Stringify(std::u32string& output, const UnaryExpr* expr);
    static void Stringify(std::u32string& output, const BinaryExpr* expr, const bool requestParenthese = false);
    static void Stringify(std::u32string& output, const LateBindVar var);
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

}