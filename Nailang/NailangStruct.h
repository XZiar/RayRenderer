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

    common::span<std::byte> Alloc(const size_t size, const size_t align = 64);
    std::pair<size_t, size_t> Usage() const noexcept;

    template<typename T>
    forceinline common::span<std::byte> Alloc()
    {
        return Alloc(sizeof(T), alignof(T));
    }

    template<typename T, typename... Args>
    forceinline T* Create(Args&&... args)
    {
        const auto space = Alloc<T>();
        new (space.data()) T(std::forward<Args>(args)...);
        return reinterpret_cast<T*>(space.data());
    }
    template<typename C>
    forceinline auto CreateArray(C&& container)
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
    constexpr bool operator==(const LateBindVar& other) const noexcept
    {
        return Name == other.Name;
    }
    constexpr bool operator==(const std::u32string_view other) const noexcept
    {
        return Name == other;
    }

    constexpr auto Parts() const noexcept
    {
        return common::str::SplitStream(Name, U'.', true);
    }
    constexpr std::u32string_view operator[](size_t index) const noexcept
    {
        return common::str::SplitStream(Name, U'.', true).Skip(index).TryGetFirst().value();
    }
};

enum class EmbedOps : uint8_t { Equal = 0, NotEqual, Less, LessEqual, Greater, GreaterEqual, And, Or, Not, Add, Sub, Mul, Div, Rem };
struct EmbedOpHelper
{
    static constexpr bool IsUnaryOp(EmbedOps op) noexcept
    {
        return op == EmbedOps::Not;
    }
};


struct FuncCall;
struct UnaryExpr;
struct BinaryExpr;


struct RawArg
{
    enum class Type : uint32_t { Empty = 0, Func, Unary, Binary, Var, Str, Uint, Int, FP, Bool };
    using Variant = std::variant<const FuncCall*, const UnaryExpr*, const BinaryExpr*, LateBindVar, std::u32string_view, uint64_t, int64_t, double, bool>;
    uint64_t Data1;
    uint32_t Data2;
    Type TypeData;

    constexpr RawArg() noexcept : Data1(0), Data2(0), TypeData(Type::Empty) {}
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
        Data1(*reinterpret_cast<const uint64_t*>(&num)), Data2(8), TypeData(Type::FP) {}
    RawArg(const bool boolean) noexcept :
        Data1(boolean ? 1 : 0), Data2(9), TypeData(Type::Bool) {}

    template<Type T>
    constexpr auto GetVar() const
    {
        Expects(TypeData == T);
        if constexpr (T == Type::Func)
            return reinterpret_cast<const FuncCall*>(Data1);
        else if constexpr (T == Type::Unary)
            return reinterpret_cast<const UnaryExpr*>(Data1);
        else if constexpr (T == Type::Binary)
            return reinterpret_cast<const BinaryExpr*>(Data1);
        else if constexpr (T == Type::Var)
            return LateBindVar{ {reinterpret_cast<const char32_t*>(Data1), Data2} };
        else if constexpr (T == Type::Str)
            return std::u32string_view{ reinterpret_cast<const char32_t*>(Data1), Data2 };
        else if constexpr (T == Type::Uint)
            return Data1;
        else if constexpr (T == Type::Int)
            return static_cast<int64_t>(Data1);
        else if constexpr (T == Type::FP)
            return *reinterpret_cast<const double*>(&Data1);
        else if constexpr (T == Type::Bool)
            return Data1 == 1;
        else
            static_assert(!common::AlwaysTrue<decltype(T)>, "");
    }

    forceinline Variant GetVar() const
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
    forceinline auto Visit(Visitor&& visitor) const
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
        default:            Expects(false); return visitor(Data1);
        }
    }
};

struct CustomVar : LateBindVar
{
    uint64_t Meta0;
    uint32_t Meta1;
    uint16_t Meta2;
};
//using Arg = std::variant<CustomVar, std::u32string_view, uint64_t, int64_t, double, bool>;
struct Arg
{
    enum class InternalType : uint16_t { Empty = 0, Var, U32Str, U32Sv, Uint, Int, FP, Bool };
    std::u32string Str;
    uint64_t Data1;
    uint32_t Data2;
    uint16_t Data3;
    InternalType TypeData;

    Arg() noexcept : Data1(0), Data2(0), Data3(0), TypeData(InternalType::Empty)
    { }
    Arg(const CustomVar var) noexcept :
        Str(var.Name), Data1(var.Meta0), Data2(var.Meta1), Data3(var.Meta2), TypeData(InternalType::Var)
    { }
    Arg(const std::u32string& str) noexcept : Str(str), Data1(0), Data2(0), Data3(0), TypeData(InternalType::U32Str)
    { }
    Arg(const std::u32string_view str) noexcept :
        Data1(reinterpret_cast<uint64_t>(str.data())), Data2(gsl::narrow_cast<uint32_t>(str.size())),
        Data3(0), TypeData(InternalType::U32Sv)
    {
        Expects(str.size() <= UINT32_MAX);
    }
    Arg(const uint64_t num) noexcept :
        Data1(num), Data2(6), Data3(0), TypeData(InternalType::Uint)
    { }
    Arg(const int64_t num) noexcept :
        Data1(static_cast<uint64_t>(num)), Data2(7), Data3(0), TypeData(InternalType::Int)
    { }
    Arg(const double num) noexcept :
        Data1(*reinterpret_cast<const uint64_t*>(&num)), Data2(8), Data3(0), TypeData(InternalType::FP)
    { }
    Arg(const bool boolean) noexcept :
        Data1(boolean ? 1 : 0), Data2(9), Data3(0), TypeData(InternalType::Bool)
    { }

    template<InternalType T>
    constexpr auto GetVar() const
    {
        Expects(TypeData == T);
        if constexpr (T == InternalType::Var)
            return CustomVar{ std::u32string_view(Str), Data1, Data2, Data3 };
        else if constexpr (T == InternalType::U32Str)
            return std::u32string_view{ Str };
        else if constexpr (T == InternalType::U32Sv)
            return std::u32string_view{ reinterpret_cast<const char32_t*>(Data1), Data2 };
        else if constexpr (T == InternalType::Uint)
            return Data1;
        else if constexpr (T == InternalType::Int)
            return static_cast<int64_t>(Data1);
        else if constexpr (T == InternalType::FP)
            return *reinterpret_cast<const double*>(&Data1);
        else if constexpr (T == InternalType::Bool)
            return Data1 == 1;
        else
            static_assert(!common::AlwaysTrue<decltype(T)>, "");
    }

    template<typename Visitor>
    forceinline auto Visit(Visitor&& visitor) const
    {
        switch (TypeData)
        {
        case InternalType::Var:     return visitor(GetVar<InternalType::Var>());
        case InternalType::U32Str:  return visitor(GetVar<InternalType::U32Str>());
        case InternalType::U32Sv:   return visitor(GetVar<InternalType::U32Sv>());
        case InternalType::Uint:    return visitor(GetVar<InternalType::Uint>());
        case InternalType::Int:     return visitor(GetVar<InternalType::Int>());
        case InternalType::FP:      return visitor(GetVar<InternalType::FP>());
        case InternalType::Bool:    return visitor(GetVar<InternalType::Bool>());
        default:    Expects(false); return visitor(Data1);
        }
    }

    forceinline constexpr bool IsEmpty() const noexcept
    {
        return TypeData == InternalType::Empty;
    }
    forceinline constexpr bool IsBool() const noexcept
    {
        return TypeData == InternalType::Bool;
    }
    forceinline constexpr bool IsInteger() const noexcept
    {
        return TypeData == InternalType::Uint || TypeData == InternalType::Int;
    }
    forceinline constexpr bool IsFloatPoint() const noexcept
    {
        return TypeData == InternalType::FP;
    }
    forceinline constexpr bool IsNumber() const noexcept
    {
        return IsFloatPoint() || IsInteger();
    }
    forceinline constexpr bool IsStr() const noexcept
    {
        return TypeData == InternalType::U32Str || TypeData == InternalType::U32Sv;
    }
    forceinline constexpr std::optional<bool> GetBool() const noexcept
    {
        switch (TypeData)
        {
        case InternalType::Uint:    return  GetVar<InternalType::Uint>() != 0;
        case InternalType::Int:     return  GetVar<InternalType::Int>() != 0;
        case InternalType::FP:      return  GetVar<InternalType::FP>() != 0;
        case InternalType::Bool:    return  GetVar<InternalType::Bool>();
        case InternalType::U32Str:  return !GetVar<InternalType::U32Str>().empty();
        case InternalType::U32Sv:   return !GetVar<InternalType::U32Sv>().empty();
        default:                    return {};
        }
    }
    forceinline constexpr std::optional<uint64_t> GetUint() const noexcept
    {
        switch (TypeData)
        {
        case InternalType::Uint:    return GetVar<InternalType::Uint>();
        case InternalType::Int:     return static_cast<uint64_t>(GetVar<InternalType::Int>());
        case InternalType::FP:      return static_cast<uint64_t>(GetVar<InternalType::FP>());
        case InternalType::Bool:    return GetVar<InternalType::Bool>() ? 1 : 0;
        default:                    return {};
        }
    }
    forceinline constexpr std::optional<int64_t> GetInt() const noexcept
    {
        switch (TypeData)
        {
        case InternalType::Uint:    return static_cast<int64_t>(GetVar<InternalType::Uint>());
        case InternalType::Int:     return GetVar<InternalType::Int>();
        case InternalType::FP:      return static_cast<int64_t>(GetVar<InternalType::FP>());
        case InternalType::Bool:    return GetVar<InternalType::Bool>() ? 1 : 0;
        default:                    return {};
        }
    }
    forceinline constexpr std::optional<double> GetFP() const noexcept
    {
        switch (TypeData)
        {
        case InternalType::Uint:    return static_cast<double>(GetVar<InternalType::Uint>());
        case InternalType::Int:     return static_cast<double>(GetVar<InternalType::Int>());
        case InternalType::FP:      return GetVar<InternalType::FP>();
        case InternalType::Bool:    return GetVar<InternalType::Bool>() ? 1. : 0.;
        default:                    return {};
        }
    }
    forceinline constexpr std::optional<std::u32string_view> GetStr() const noexcept
    {
        switch (TypeData)
        {
        case InternalType::U32Str:  return GetVar<InternalType::U32Str>();
        case InternalType::U32Sv:   return GetVar<InternalType::U32Sv>();
        default:                    return {};
        }
    }
};


struct FuncCall
{
    std::u32string_view Name;
    common::span<const RawArg> Args;
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

struct RawBlock
{
    std::u32string_view Type;
    std::u32string_view Name;
    std::u32string_view Source;
    std::u16string FileName;
    std::pair<size_t, size_t> Position;
};
using RawBlockWithMeta = WithMeta<RawBlock>;
struct Block;

struct Assignment
{
    LateBindVar Variable;
    RawArg Statement;
};

struct BlockContent
{
    enum class Type : uint8_t { Assignment = 0, FuncCall = 1, RawBlock = 2, Block = 3 };
    uintptr_t Pointer;

    static BlockContent Generate(const Assignment* ptr)
    {
        const auto pointer = reinterpret_cast<uintptr_t>(ptr);
        Expects(pointer % 4 == 0); // should be at least 4 bytes aligned
        return BlockContent{ pointer | common::enum_cast(Type::Assignment) };
    }
    static BlockContent Generate(const FuncCall* ptr)
    {
        const auto pointer = reinterpret_cast<uintptr_t>(ptr);
        Expects(pointer % 4 == 0); // should be at least 4 bytes aligned
        return BlockContent{ pointer | common::enum_cast(Type::FuncCall) };
    }
    static BlockContent Generate(const RawBlock* ptr)
    {
        const auto pointer = reinterpret_cast<uintptr_t>(ptr);
        Expects(pointer % 4 == 0); // should be at least 4 bytes aligned
        return BlockContent{ pointer | common::enum_cast(Type::RawBlock) };
    }
    static BlockContent Generate(const Block* ptr)
    {
        const auto pointer = reinterpret_cast<uintptr_t>(ptr);
        Expects(pointer % 4 == 0); // should be at least 4 bytes aligned
        return BlockContent{ pointer | common::enum_cast(Type::Block) };
    }

    template<typename T>
    forceinline const T* Get() const noexcept
    {
        return reinterpret_cast<const T*>(Pointer & ~(uintptr_t)(0x3));
    }

    forceinline constexpr Type GetType() const noexcept
    {
        return static_cast<Type>(Pointer & 0x3);
    }
    std::variant<const Assignment*, const FuncCall*, const RawBlock*, const Block*>
        GetStatement() const
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
    static BlockContentItem Generate(const T* ptr, const uint32_t offset, const uint32_t count)
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

        constexpr value_type operator*() const noexcept
        {
            return (*Host)[Idx];
        }
        constexpr bool operator!=(const BlockIterator& other) const noexcept
        {
            return Host != other.Host || Idx != other.Idx;
        }
        constexpr bool operator==(const BlockIterator& other) const noexcept
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

    constexpr BlockIterator begin() const noexcept
    {
        return { this, 0 };
    }
    constexpr BlockIterator end() const noexcept
    {
        return { this, Size() };
    }
    constexpr value_type operator[](size_t index) const noexcept
    {
        const auto& content = Content[index];
        const auto metafuncs = MetaFuncations.subspan(content.Offset, content.Count);
        return { metafuncs, content };
    }
    constexpr size_t Size() const noexcept { return Content.size(); }
};


}