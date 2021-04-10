#include "NailangPch.h"
#include "NailangStruct.h"
#include "NailangRuntime.h"

#include <cassert>

namespace xziar::nailang
{
using namespace std::string_view_literals;
#define NLRT_THROW_EX(...) HandleException(CREATE_EXCEPTIONEX(NailangRuntimeException, __VA_ARGS__))


std::u32string_view EmbedOpHelper::GetOpName(EmbedOps op) noexcept
{
    switch (op)
    {
    #define RET_NAME(op) case EmbedOps::op: return U"" STRINGIZE(op)
    RET_NAME(Equal);
    RET_NAME(NotEqual);
    RET_NAME(Less);
    RET_NAME(LessEqual);
    RET_NAME(Greater);
    RET_NAME(GreaterEqual);
    RET_NAME(And);
    RET_NAME(Or);
    RET_NAME(Add);
    RET_NAME(Sub);
    RET_NAME(Mul);
    RET_NAME(Div);
    RET_NAME(Rem);
    RET_NAME(ValueOr);
    RET_NAME(CheckExist);
    RET_NAME(Not);
    default: return U"Unknwon"sv;
    }
}


std::u32string_view SubQuery::ExpectSubField(size_t idx) const
{
    Expects(idx < Queries.size());
    if (static_cast<QueryType>(Queries[idx].ExtraFlag) != QueryType::Sub)
        COMMON_THROWEX(NailangRuntimeException, u"Expect [Subfield] as sub-query, get [Index]"sv);
    return Queries[idx].GetVar<Expr::Type::Str>();
}
const Expr& SubQuery::ExpectIndex(size_t idx) const
{
    Expects(idx < Queries.size());
    if (static_cast<QueryType>(Queries[idx].ExtraFlag) != QueryType::Index)
        COMMON_THROWEX(NailangRuntimeException, u"Expect [Index] as sub-query, get [Subfield]"sv);
    return Queries[idx];
}


std::u32string_view Expr::TypeName(const Expr::Type type) noexcept
{
    switch (type)
    {
    case Expr::Type::Empty:   return U"empty"sv;
    case Expr::Type::Func:    return U"func-call"sv;
    case Expr::Type::Unary:   return U"unary-expr"sv;
    case Expr::Type::Binary:  return U"binary-expr"sv;
    case Expr::Type::Ternary: return U"ternary-expr"sv;
    case Expr::Type::Query:   return U"query-expr"sv;
    case Expr::Type::Var:     return U"variable"sv;
    case Expr::Type::Str:     return U"string"sv;
    case Expr::Type::Uint:    return U"uint"sv;
    case Expr::Type::Int:     return U"int"sv;
    case Expr::Type::FP:      return U"fp"sv;
    case Expr::Type::Bool:    return U"bool"sv;
    default:                  return U"error"sv;
    }
}


#define NATIVE_TYPE_MAP BOOST_PP_VARIADIC_TO_SEQ(   \
    (Any,    Arg),                  \
    (Bool,   bool),                 \
    (U8,     uint8_t),              \
    (I8,     int8_t),               \
    (U16,    uint16_t),             \
    (I16,    int16_t),              \
    (F16,    half_float::half),     \
    (U32,    uint32_t),             \
    (I32,    int32_t),              \
    (F32,    float),                \
    (U64,    uint64_t),             \
    (I64,    int64_t),              \
    (F64,    double),               \
    (Str8,   common::str::u8string),\
    (Str16,  std::u16string),       \
    (Str32,  std::u32string),       \
    (Sv8,    std::string_view),     \
    (Sv16,   std::u16string_view),  \
    (Sv32,   std::u32string_view))

#define NATIVE_TYPE_EACH_(r, func, tp) func(BOOST_PP_TUPLE_ELEM(2, 0, tp), BOOST_PP_TUPLE_ELEM(2, 1, tp))
#define NATIVE_TYPE_EACH(func) BOOST_PP_SEQ_FOR_EACH(NATIVE_TYPE_EACH_, func, NATIVE_TYPE_MAP)

std::u32string_view NativeWrapper::TypeName(NativeWrapper::Type type) noexcept
{
#define RET(tenum, type) case Type::tenum: return U"" STRINGIZE(tenum) ""sv;
    switch (type)
    {
    NATIVE_TYPE_EACH(RET)
    default: return U"Unknown"sv;
    }
#undef RET
}

template<typename T>
static Arg NativeGetter(uintptr_t pointer, size_t idx)
{
    using common::str::Charset;
    using U = std::remove_pointer_t<T>;
    [[maybe_unused]] static constexpr bool ShouldMove = std::is_pointer_v<T>;
    const auto ptr = reinterpret_cast<U*>(pointer) + idx;
    if constexpr (std::is_unsigned_v<U>)
        return static_cast<uint64_t>(*ptr);
    else if constexpr (std::is_signed_v<U>)
        return static_cast<int64_t>(*ptr);
    else if constexpr (std::is_floating_point_v<U> || std::is_same_v<U, half_float::half>)
        return static_cast<double>(*ptr);
    else if constexpr (std::is_same_v<U, Arg>)
    {
        if constexpr (ShouldMove)
            return std::move(*ptr);
        else
            return *ptr;
    }
    else if constexpr (std::is_same_v<U, std::u32string>)
    {
        if constexpr (ShouldMove)
            return *ptr;
        else
            return std::u32string_view(*ptr);
    }
    else if constexpr (std::is_same_v<U, std::u32string_view>)
    {
        if constexpr (ShouldMove)
            return std::u32string(*ptr);
        else
            return *ptr;
    }
    else if constexpr (common::is_specialization<U, std::basic_string>::value || common::is_specialization<U, std::basic_string_view>::value)
    {
        return common::str::to_u32string(*ptr);
    }
    else if constexpr (std::is_same_v<U, bool>)
        return *ptr;
    else
    {
        Expects(false);
        return {};
    }
}
NativeWrapper::Getter NativeWrapper::GetGetter(Type type, bool move) noexcept
{
#define RET(tenum, type) case Type::tenum: return move ? &NativeGetter<type*> : &NativeGetter<type>;

    switch (type)
    {
    NATIVE_TYPE_EACH(RET)
    default: return nullptr;
    }
#undef RET
}

template<typename T>
static bool NativeSetter(uintptr_t pointer, size_t idx, Arg arg)
{
    using common::str::Charset;
    [[maybe_unused]] const auto ptr = reinterpret_cast<T*>(pointer) + idx;
    if constexpr (std::is_same_v<T, Arg>)
    {
        *ptr = std::move(arg);
        return true;
    }
    else if constexpr (std::is_unsigned_v<T>)
    {
        if (const auto real = arg.GetUint(); real.has_value())
        {
            *ptr = static_cast<T>(*real);
            return true;
        }
    }
    else if constexpr (std::is_signed_v<T>)
    {
        if (const auto real = arg.GetInt(); real.has_value())
        {
            *ptr = static_cast<T>(*real);
            return true;
        }
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        if (const auto real = arg.GetFP(); real.has_value())
        {
            *ptr = static_cast<T>(*real);
            return true;
        }
    }
    else if constexpr (std::is_same_v<T, half_float::half>)
    {
        if (const auto real = arg.GetFP(); real.has_value())
        {
            *ptr = static_cast<T>(static_cast<float>(*real));
            return true;
        }
    }
    else if constexpr (common::is_specialization<T, std::basic_string>::value)
    {
        if (const auto real = arg.GetStr(); real.has_value())
        {
            using U = typename T::value_type;
            if constexpr (std::is_same_v<U, char32_t>)
                *ptr = *real;
            else if constexpr (std::is_same_v<U, u8ch_t>)
                *ptr = common::str::to_u8string(*real);
            else if constexpr (std::is_same_v<U, char16_t>)
                *ptr = common::str::to_u16string(*real);
            else
                static_assert(!common::AlwaysTrue<T>, "unsupported char type");
            return true;
        }
    }
    else
        Expects(false);
    return false;
}
NativeWrapper::Setter NativeWrapper::GetSetter(Type type) noexcept
{
#define RET(tenum, type) case Type::tenum: return &NativeSetter<type>;

    switch (type)
    {
    NATIVE_TYPE_EACH(RET)
    default: return nullptr;
    }
#undef RET
}

ArgLocator NativeWrapper::GetLocator(Type type, uintptr_t pointer, bool isConst, size_t idx) noexcept
{
    constexpr auto Combine = [](Type type, auto ptr, bool isConst) -> ArgLocator
    {
        using T = decltype(*ptr);
        if constexpr (std::is_same_v<T, Arg>)
            return { ptr, 1, isConst ? ArgLocator::LocateFlags::ReadOnly : ArgLocator::LocateFlags::ReadWrite };
        else
        {
            const bool noSetter = isConst || common::is_specialization<decltype(*ptr), std::basic_string_view>::value;
            const auto ptrVal = reinterpret_cast<uintptr_t>(ptr);
            return
            {
                [ptrVal, func = NativeWrapper::GetGetter(type)] () { return func(ptrVal, 0); },
                noSetter ? ArgLocator::Setter{} :
                    [ptrVal, func = NativeWrapper::GetSetter(type)] (Arg val) { return func(ptrVal, 0, std::move(val)); },
                1
            };
        }
    };
#define RET(tenum, t) case Type::tenum: return Combine(type, reinterpret_cast<t*>(pointer) + idx, isConst);

    switch (type)
    {
    NATIVE_TYPE_EACH(RET)
    default: return {};
    }
#undef RET
}


template<typename T> struct U8StrFix { using Type = T; };
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template<> struct U8StrFix<std::u8string> { using Type = std::string; };
template<> struct U8StrFix<std::u8string_view> { using Type = std::string_view; };
#endif
FixedArray::SpanVariant FixedArray::GetSpan() const noexcept
{
#define SP(type) common::span<type>{ reinterpret_cast<type*>(DataPtr), static_cast<size_t>(Length) }
#define RET(tenum, type) case Type::tenum:                              \
        if (IsReadOnly) return SP(const typename U8StrFix<type>::Type); \
        else return SP(typename U8StrFix<type>::Type);                  \

    switch (ElementType)
    {
    NATIVE_TYPE_EACH(RET)
    default: return {};
    }
#undef RET
#undef SP
}

void FixedArray::PrintToStr(std::u32string& str, std::u32string_view delim) const
{
    constexpr auto Append = [](std::u32string& str, auto delim, auto ptr, const size_t len)
    {
        for (uint32_t idx = 0; idx < len; ++idx)
        {
            if (idx > 0) str.append(delim);
            if constexpr (std::is_same_v<const Arg&, decltype(*ptr)>)
                str.append(ptr[idx].ToString().StrView());
            else
                fmt::format_to(std::back_inserter(str), FMT_STRING(U"{}"), ptr[idx]);
        }
    };
#define PRT(tenum, type) case Type::tenum:                              \
    Append(str, delim, reinterpret_cast<const type*>(DataPtr), Length); \
    return;

    switch (ElementType)
    {
    NATIVE_TYPE_EACH(PRT)
    default: return;
    }
#undef PRT
}

#undef NATIVE_TYPE_EACH
#undef NATIVE_TYPE_EACH_
#undef NATIVE_TYPE_MAP


Arg::Arg(const Arg& other) noexcept :
    Data0(other.Data0), Data1(other.Data1), Data2(other.Data2), Data3(other.Data3), TypeData(other.TypeData)
{
    if (IsCustom())
    {
        const auto& var = GetCustom();
        var.Host->IncreaseRef(var);
    }
    else if (TypeData == Type::U32Str && Data1 > 0)
    {
        auto str = other.GetVar<Type::U32Str>();
        common::SharedString<char32_t>::PlacedIncrease(str);
    }
}

Arg& Arg::operator=(const Arg& other) noexcept
{
    Release();
    Data0 = other.Data0;
    Data1 = other.Data1;
    Data2 = other.Data2;
    Data3 = other.Data3;
    TypeData = other.TypeData;
    if (IsCustom())
    {
        const auto& var = GetCustom();
        var.Host->IncreaseRef(var);
    }
    else if (TypeData == Type::U32Str && Data1 > 0)
    {
        auto str = other.GetVar<Type::U32Str>();
        common::SharedString<char32_t>::PlacedIncrease(str);
    }
    return *this;
}

void Arg::Release() noexcept
{
    if (IsCustom())
    {
        auto& var = GetCustom();
        var.Host->DecreaseRef(var);
    }
    else if (TypeData == Type::U32Str && Data1 > 0)
    {
        auto str = GetVar<Type::U32Str>();
        common::SharedString<char32_t>::PlacedDecrease(str);
    }
    TypeData = Type::Empty;
}

std::optional<bool> Arg::GetBool() const noexcept
{
    if (IsCustom())
        return GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::Boolable).GetBool();
    switch (TypeData)
    {
    case Type::Uint:    return  GetVar<Type::Uint>() != 0;
    case Type::Int:     return  GetVar<Type::Int>() != 0;
    case Type::FP:      return  GetVar<Type::FP>() != 0;
    case Type::Bool:    return  GetVar<Type::Bool>();
    case Type::U32Str:  return !GetVar<Type::U32Str>().empty();
    case Type::U32Sv:   return !GetVar<Type::U32Sv>().empty();
    default:            return {};
    }
}
std::optional<uint64_t> Arg::GetUint() const noexcept
{
    if (IsCustom())
        return GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::NumberBit | Type::Boolable).GetUint();
    switch (TypeData)
    {
    case Type::Uint:    return GetVar<Type::Uint>();
    case Type::Int:     return static_cast<uint64_t>(GetVar<Type::Int>());
    case Type::FP:      return static_cast<uint64_t>(GetVar<Type::FP>());
    case Type::Bool:    return GetVar<Type::Bool>() ? 1u : 0u;
    default:            return {};
    }
    
}
std::optional<int64_t> Arg::GetInt() const noexcept
{
    if (IsCustom())
        return GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::NumberBit | Type::Boolable).GetUint();
    switch (TypeData)
    {
    case Type::Uint:    return static_cast<int64_t>(GetVar<Type::Uint>());
    case Type::Int:     return GetVar<Type::Int>();
    case Type::FP:      return static_cast<int64_t>(GetVar<Type::FP>());
    case Type::Bool:    return GetVar<Type::Bool>() ? 1 : 0;
    default:            return {};
    }
    
}
std::optional<double> Arg::GetFP() const noexcept
{
    if (IsCustom())
        return GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::NumberBit | Type::Boolable).GetFP();
    switch (TypeData)
    {
    case Type::Uint:    return static_cast<double>(GetVar<Type::Uint>());
    case Type::Int:     return static_cast<double>(GetVar<Type::Int>());
    case Type::FP:      return GetVar<Type::FP>();
    case Type::Bool:    return GetVar<Type::Bool>() ? 1. : 0.;
    default:            return {};
    }
}
std::optional<std::u32string_view> Arg::GetStr() const noexcept
{
    if (IsCustom())
        return GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::StringBit).GetStr();
    switch (TypeData)
    {
    case Type::U32Str:  return GetVar<Type::U32Str>();
    case Type::U32Sv:   return GetVar<Type::U32Sv>();
    default:            return {};
    }
}

template<typename T>
static common::str::StrVariant<char32_t> ArgToString([[maybe_unused]]const T& val) noexcept
{
    if constexpr (std::is_same_v<T, CustomVar> || std::is_same_v<T, std::nullopt_t>)
        return {};
    else
        return fmt::format(FMT_STRING(U"{}"), val);
}
template<>
common::str::StrVariant<char32_t> ArgToString<FixedArray>(const FixedArray& val) noexcept
{
    std::u32string ret = U"[";
    val.PrintToStr(ret, U", "sv);
    ret.append(U"]"sv);
    return std::move(ret);
}
template<>
common::str::StrVariant<char32_t> ArgToString<std::u32string_view>(const std::u32string_view& val) noexcept
{
    return val;
}
common::str::StrVariant<char32_t> Arg::ToString() const noexcept
{
    if (IsCustom())
    {
        return GetCustom().Call<&CustomVar::Handler::ToString>();
    }
    return Visit([](const auto& val)
        {
            return ArgToString(val);
        });
}
std::u32string_view Arg::TypeName(const Arg::Type type) noexcept
{
#if COMPILER_GCC
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wswitch"
#elif COMPILER_CLANG
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wswitch"
#elif COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4063)
#endif
    switch (type)
    {
    case Arg::Type::Empty:      return U"empty"sv;
    case Arg::Type::Var:        return U"variable"sv;
    case Arg::Type::U32Str:     return U"string"sv;
    case Arg::Type::U32Sv:      return U"string_view"sv;
    case Arg::Type::Uint:       return U"uint"sv;
    case Arg::Type::Int:        return U"int"sv;
    case Arg::Type::FP:         return U"fp"sv;
    case Arg::Type::Bool:       return U"bool"sv;
    case Arg::Type::Boolable:   return U"boolable"sv;
    case Arg::Type::String:     return U"string-ish"sv;
    case Arg::Type::ArrayLike:  return U"array-like"sv;
    case Arg::Type::Number:     return U"number"sv;
    case Arg::Type::Integer:    return U"integer"sv;
    case Arg::Type::BoolBit:    return U"(bool)"sv;
    case Arg::Type::StringBit:  return U"(str)"sv;
    case Arg::Type::NumberBit:  return U"(num)"sv;
    case Arg::Type::ArrayBit:   return U"(arr)"sv;
    case Arg::Type::BoolBit   | Arg::Type::StringBit: return U"(bool|str)"sv;
    case Arg::Type::BoolBit   | Arg::Type::NumberBit: return U"(bool|num)"sv;
    case Arg::Type::BoolBit   | Arg::Type::ArrayBit:  return U"(bool|arr)"sv;
    case Arg::Type::StringBit | Arg::Type::NumberBit: return U"(str|num)"sv;
    case Arg::Type::StringBit | Arg::Type::ArrayBit:  return U"(str|arr)"sv;
    case Arg::Type::NumberBit | Arg::Type::ArrayBit:  return U"(num|arr)"sv;
    case Arg::Type::BoolBit   | Arg::Type::StringBit | Arg::Type::NumberBit: return U"(bool|str|num)"sv;
    case Arg::Type::BoolBit   | Arg::Type::StringBit | Arg::Type::ArrayBit:  return U"(bool|str|arr)"sv;
    case Arg::Type::BoolBit   | Arg::Type::NumberBit | Arg::Type::ArrayBit:  return U"(bool|num|arr)"sv;
    case Arg::Type::StringBit | Arg::Type::NumberBit | Arg::Type::ArrayBit:  return U"(str|num|arr)"sv;
    case Arg::Type::BoolBit   | Arg::Type::StringBit | Arg::Type::NumberBit | Arg::Type::ArrayBit: return U"(bool|str|num|arr)"sv;
    default:                    return U"unknown"sv;
    }
#if COMPILER_GCC
#   pragma GCC diagnostic pop
#elif COMPILER_CLANG
#   pragma clang diagnostic pop
#elif COMPILER_MSVC
#   pragma warning(pop)
#endif
}

ArgLocator Arg::HandleQuery(SubQuery subq, NailangExecutor& runtime)
{
    Expects(subq.Size() > 0);
    if (IsCustom())
    {
        auto& var = GetCustom();
        return var.Call<&CustomVar::Handler::HandleQuery>(subq, runtime);
    }
    if (IsArray())
    {
        const auto arr = GetVar<Type::Array>();
        const auto [type, query] = subq[0];
        switch (type)
        {
        case SubQuery::QueryType::Index:
        {
            const auto val = runtime.EvaluateExpr(query);
            const auto idx = NailangHelper::BiDirIndexCheck(arr.Length, val, &query);
            return arr.Access(idx);
        }
        case SubQuery::QueryType::Sub:
        {
            const auto field = query.GetVar<Expr::Type::Str>();
            if (field == U"Length"sv)
                return { arr.Length, 1u };
        } break;
        default: break;
        }
        return {};
    }
    if (IsStr())
    {
        const auto str = GetStr().value();
        const auto [type, query] = subq[0];
        switch (type)
        {
        case SubQuery::QueryType::Index:
        {
            const auto val = runtime.EvaluateExpr(query);
            const auto idx = NailangHelper::BiDirIndexCheck(str.size(), val, &query);
            if (TypeData == Type::U32Str)
                return { std::u32string(1, str[idx]), 1u };
            else // (TypeData == Type::U32Sv)
                return { str.substr(idx, 1), 1u };
        }
        case SubQuery::QueryType::Sub:
        {
            const auto field = query.GetVar<Expr::Type::Str>();
            if (field == U"Length"sv)
                return { uint64_t(str.size()), 1u };
        } break;
        default: break;
        }
        return {};
    }
    return {};
}

Arg Arg::HandleUnary(const EmbedOps op) const
{
    if (IsCustom())
    {
        auto& var = GetCustom();
        auto ret = var.Call<&CustomVar::Handler::HandleUnary>(op);
        if (!ret.IsEmpty())
            return ret;
    }
    if (op == EmbedOps::Not)
    {
        if (auto ret = GetBool(); ret)
            return !ret.value();
    }
    return {};
}

static Arg StrBinaryOp(const EmbedOps op, const std::u32string_view left, const std::u32string_view right) noexcept
{
    switch (op)
    {
    case EmbedOps::Equal:           return left == right;
    case EmbedOps::NotEqual:        return left != right;
    case EmbedOps::Less:            return left <  right;
    case EmbedOps::LessEqual:       return left <= right;
    case EmbedOps::Greater:         return left >  right;
    case EmbedOps::GreaterEqual:    return left >= right;
    case EmbedOps::Add:
    {
        std::u32string ret;
        ret.reserve(left.size() + right.size());
        ret.append(left).append(right);
        return ret;
    }
    default:                        return {};
    }
}
static Arg NumCompareOp(const EmbedOps op, const Arg& left, const Arg& right) noexcept
{
    // Expects(HAS_FIELD(right.TypeData, Arg::Type::NumberBit));
    if (!HAS_FIELD(right.TypeData, Arg::Type::NumberBit))
        return {};
    if (const bool isLeftFP = HAS_FIELD(left.TypeData, Arg::Type::FPBit), isRightFP = HAS_FIELD(right.TypeData, Arg::Type::FPBit); 
        isLeftFP || isRightFP)
    {
        const auto l = left.GetFP().value(), r = right.GetFP().value();
        switch (op)
        {
        case EmbedOps::Equal:           return l == r;
        case EmbedOps::NotEqual:        return l != r;
        case EmbedOps::Less:            return l <  r;
        case EmbedOps::LessEqual:       return l <= r;
        case EmbedOps::Greater:         return l >  r;
        case EmbedOps::GreaterEqual:    return l >= r;
        default:                        return {};
        }
    }
    const bool isLeftUnsigned = HAS_FIELD(left.TypeData, Arg::Type::UnsignedBit),
        isRightUnsigned = HAS_FIELD(right.TypeData, Arg::Type::UnsignedBit);
    if (isLeftUnsigned)
    {
        const auto l = left.GetVar<Arg::Type::Uint>();
        const bool sameSign = isRightUnsigned || right.GetVar<Arg::Type::Int, false>() >= 0;
        const auto r = right.GetVar<Arg::Type::Uint, false>();
        switch (op)
        {
        case EmbedOps::Equal:           return  sameSign && l == r;
        case EmbedOps::NotEqual:        return !sameSign || l != r;
        case EmbedOps::Less:            return  sameSign && l <  r;
        case EmbedOps::LessEqual:       return  sameSign && l <= r;
        case EmbedOps::Greater:         return !sameSign || l >  r;
        case EmbedOps::GreaterEqual:    return !sameSign || l >= r;
        default:                        return {};
        }
    }
    else
    {
        const auto l = left.GetVar<Arg::Type::Int>();
        const bool sameSign = !isRightUnsigned;
        const auto r = right.GetVar<Arg::Type::Int, false>();
        switch (op)
        {
        case EmbedOps::Equal:           return  sameSign && l == r;
        case EmbedOps::NotEqual:        return !sameSign || l != r;
        case EmbedOps::Less:            return !sameSign || l <  r;
        case EmbedOps::LessEqual:       return !sameSign || l <= r;
        case EmbedOps::Greater:         return  sameSign && l >  r;
        case EmbedOps::GreaterEqual:    return  sameSign && l >= r;
        default:                        return {};
        }
    }
}
static Arg NumArthOp(const EmbedOps op, const Arg& left, const Arg& right) noexcept
{
    // Expects(HAS_FIELD(right.TypeData, Arg::Type::NumberBit | Arg::Type::BoolBit));
    if (!HAS_FIELD(right.TypeData, Arg::Type::NumberBit | Arg::Type::BoolBit))
        return {};
    if (const bool isLeftFP = HAS_FIELD(left.TypeData, Arg::Type::FPBit), isRightFP = HAS_FIELD(right.TypeData, Arg::Type::FPBit); 
        isLeftFP || isRightFP)
    {
        const auto l = left.GetFP().value(), r = right.GetFP().value();
        switch (op)
        {
        case EmbedOps::Add: return l + r;
        case EmbedOps::Sub: return l - r;
        case EmbedOps::Mul: return l * r;
        case EmbedOps::Div: return l / r;
        case EmbedOps::Rem: return std::fmod(l, r);
        default:            return {};
        }
    }
    if (const bool isLeftUnsigned = HAS_FIELD(left.TypeData, Arg::Type::UnsignedBit),
        isRightUnsigned = HAS_FIELD(right.TypeData, Arg::Type::UnsignedBit); isLeftUnsigned || isRightUnsigned)
    {
        const auto l = left.GetUint().value(), r = right.GetUint().value();
        switch (op)
        {
        case EmbedOps::Add: return l + r;
        case EmbedOps::Sub: return l - r;
        case EmbedOps::Mul: return l * r;
        case EmbedOps::Div: return l / r;
        case EmbedOps::Rem: return l % r;
        default:            return {};
        }
    }
    {
        const auto l = left.GetInt().value(), r = right.GetInt().value();
        switch (op)
        {
        case EmbedOps::Add: return l + r;
        case EmbedOps::Sub: return l - r;
        case EmbedOps::Mul: return l * r;
        case EmbedOps::Div: return l / r;
        case EmbedOps::Rem: return l % r;
        default:            return {};
        }
    }
}

Arg Arg::HandleBinary(const EmbedOps op, const Arg& right) const
{
    const auto opCat = EmbedOpHelper::GetCategory(op);
    Expects(opCat != EmbedOpHelper::OpCategory::Other); // should not be handled here
    if (IsCustom())
    {
        auto& var = GetCustom();
        auto ret = var.Call<&CustomVar::Handler::HandleBinary>(op, right);
        if (!ret.IsEmpty())
            return ret;
        // try match right side type
        if (!ret.IsCustom())
        {
            switch (op)
            {
            case EmbedOps::Equal:        return right.HandleBinary(EmbedOps::Equal,        *this);
            case EmbedOps::NotEqual:     return right.HandleBinary(EmbedOps::NotEqual,     *this);
            case EmbedOps::Less:         return right.HandleBinary(EmbedOps::Greater,      *this);
            case EmbedOps::LessEqual:    return right.HandleBinary(EmbedOps::GreaterEqual, *this);
            case EmbedOps::Greater:      return right.HandleBinary(EmbedOps::Less,         *this);
            case EmbedOps::GreaterEqual: return right.HandleBinary(EmbedOps::LessEqual,    *this);
            default:                     break;
            }
        }
        return {};
    }
    if (opCat == EmbedOpHelper::OpCategory::Logic) // special handle logic op, since val can decay to bool
    {
        if (!HAS_FIELD(TypeData, Type::Boolable))
            return {};
        const auto r_ = right.GetBool();
        if (!r_.has_value())
            return {};
        const auto l = GetBool().value(), r = r_.value();
        return op == EmbedOps::And ? (l && r) : (l || r);
    }
    if (HAS_FIELD(TypeData, Type::StringBit)) // direct process string, no decay
    {
        const auto r_ = right.GetStr();
        if (!r_.has_value())
            return {};
        return StrBinaryOp(op, GetVar<Type::U32Sv, false>(), r_.value());
    }
    const bool isLeftNum = HAS_FIELD(TypeData, Type::NumberBit);
    if (opCat == EmbedOpHelper::OpCategory::Compare) // special handle compare op, due to signeness
    {
        if (!isLeftNum)
            return {};
        if (right.IsCustom())
            return NumCompareOp(op, *this, right.GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::NumberBit));
        else
            return NumCompareOp(op, *this, right);
    }
    Expects(opCat == EmbedOpHelper::OpCategory::Arth);
    if (isLeftNum) // process as number
    {
        if (right.IsCustom())
            return NumArthOp(op, *this, right.GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::NumberBit));
        else
            return NumArthOp(op, *this, right);
    }
    else if (TypeData == Type::Bool) // consider is right can be number
    {
        if (right.IsCustom())
            return NumArthOp(op, *this, right.GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::NumberBit));
        if (HAS_FIELD(TypeData, Type::NumberBit))
            return NumArthOp(op, *this, right);
    }
    return {};
}

bool Arg::HandleBinaryOnSelf(const EmbedOps op, const Arg& right)
{
    if (IsCustom())
    {
        auto& var = GetCustom();
        return var.Call<&CustomVar::Handler::HandleBinaryOnSelf>(op, right);
    }
    return false; // for native type, always use HandleBinary + Assign
}


std::u32string_view CustomVar::Handler::GetTypeName(const CustomVar&) noexcept
{
    return U"CustomVar"sv;
}
void CustomVar::Handler::IncreaseRef(const CustomVar&) noexcept { }
void CustomVar::Handler::DecreaseRef(CustomVar&) noexcept { }
Arg::Type CustomVar::Handler::QueryConvertSupport(const CustomVar&) noexcept
{
    return Arg::Type::Empty;
}
Arg CustomVar::Handler::IndexerGetter(const CustomVar&, const Arg&, const Expr&) 
{ 
    return {};
}
Arg CustomVar::Handler::SubfieldGetter(const CustomVar&, std::u32string_view)
{
    return {};
}
ArgLocator CustomVar::Handler::HandleQuery(CustomVar& var, SubQuery subq, NailangExecutor& executor)
{
    Expects(subq.Size() > 0);
    const auto [type, query] = subq[0];
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
        auto result = SubfieldGetter(var, field);
        if (!result.IsEmpty())
            return { std::move(result), 1u };
    } break;
    default: break;
    }
    return {};
}
Arg CustomVar::Handler::HandleUnary(const CustomVar&, const EmbedOps)
{
    return {};
}
Arg CustomVar::Handler::HandleBinary(const CustomVar& var, const EmbedOps op, const Arg& right)
{
    switch (op)
    {
    case EmbedOps::Equal:
        if (const auto ret = Compare(var, right); HAS_FIELD(ret.Result, CompareResultCore::Equality))
            return ret.IsEqual();
        break;
    case EmbedOps::NotEqual:
        if (const auto ret = Compare(var, right); HAS_FIELD(ret.Result, CompareResultCore::Equality))
            return ret.IsNotEqual();
        break;
    case EmbedOps::Less:
        if (const auto ret = Compare(var, right); HAS_FIELD(ret.Result, CompareResultCore::Equality))
            return ret.IsLess();
        break;
    case EmbedOps::LessEqual:
        if (const auto ret = Compare(var, right); HAS_FIELD(ret.Result, CompareResultCore::Equality))
            return ret.IsLessEqual();
        break;
    case EmbedOps::Greater:
        if (const auto ret = Compare(var, right); HAS_FIELD(ret.Result, CompareResultCore::Equality))
            return ret.IsGreater();
        break;
    case EmbedOps::GreaterEqual:
        if (const auto ret = Compare(var, right); HAS_FIELD(ret.Result, CompareResultCore::Equality))
            return ret.IsGreaterEqual();
        break;
    default:
        break;
    }
    return {};
}
bool CustomVar::Handler::HandleBinaryOnSelf(CustomVar&, const EmbedOps, const Arg&)
{
    return false;
}
bool CustomVar::Handler::HandleAssign(CustomVar&, Arg)
{
    return false;
}
CompareResult CustomVar::Handler::CompareSameClass(const CustomVar&, const CustomVar&)
{
    return { };
}
CompareResult CustomVar::Handler::Compare(const CustomVar& var, const Arg& other)
{
    if (other.IsCustom())
    {
        if (const auto& rhs = other.GetCustom(); rhs.Host == var.Host)
            return CompareSameClass(var, rhs);
    }
    return { };
}
common::str::StrVariant<char32_t> CustomVar::Handler::ToString(const CustomVar& var) noexcept
{
    return GetTypeName(var);
}
Arg CustomVar::Handler::ConvertToCommon(const CustomVar&, Arg::Type) noexcept
{
    return {};
}


ArgLocator::ArgLocator(Getter getter, Setter setter, uint32_t consumed) noexcept :
    Val{}, Consumed(consumed), Type(LocateType::GetSet), Flag(LocateFlags::Empty)
{
    if (const bool bg = (bool)getter, bs = (bool)setter; bg == bs)
        Flag = bg ? LocateFlags::ReadWrite : LocateFlags::Empty;
    else
        Flag = bg ? LocateFlags::ReadOnly : LocateFlags::WriteOnly;
    if (Flag == LocateFlags::Empty)
        Type = LocateType::Empty;
    else
    {
        auto ptr = new GetSet(std::move(getter), std::move(setter));
        Val = PointerToVal(ptr);
    }
}
ArgLocator::~ArgLocator()
{
    if (Type == LocateType::GetSet)
    {
        auto ptr = reinterpret_cast<GetSet*>(static_cast<uintptr_t>(Val.GetUint().value()));
        delete ptr;
    }
}
Arg ArgLocator::Get() const
{
    switch (Type)
    {
    case LocateType::Ptr:       return *reinterpret_cast<const Arg*>(static_cast<uintptr_t>(Val.GetUint().value()));
    case LocateType::GetSet:    return reinterpret_cast<const GetSet*>(static_cast<uintptr_t>(Val.GetUint().value()))->Get();
    case LocateType::Arg:       return Val;
    default:                    return {};
    }
}
Arg ArgLocator::ExtractGet()
{
    switch (Type)
    {
    case LocateType::Ptr:       return *reinterpret_cast<const Arg*>(static_cast<uintptr_t>(Val.GetUint().value()));
    case LocateType::GetSet:    return reinterpret_cast<const GetSet*>(static_cast<uintptr_t>(Val.GetUint().value()))->Get();
    case LocateType::Arg:       return std::move(Val);
    default:                    return {};
    }
}
bool ArgLocator::Set(Arg val)
{
    switch (Type)
    {
    case LocateType::Ptr:
        *reinterpret_cast<Arg*>(static_cast<uintptr_t>(Val.GetUint().value())) = std::move(val); 
        return true;
    case LocateType::GetSet:
        reinterpret_cast<const GetSet*>(static_cast<uintptr_t>(Val.GetUint().value()))->Set(std::move(val));
        return true;
    case LocateType::Arg:
        if (Val.IsCustom())
            return Val.GetCustom().Call<&CustomVar::Handler::HandleAssign>(std::move(val));
        return false;
    default:
        return false;
    }
}
Arg& ArgLocator::ResolveGetter()
{
    switch (Type)
    {
    case LocateType::Ptr:
        return *reinterpret_cast<Arg*>(static_cast<uintptr_t>(Val.GetUint().value()));
    case LocateType::GetSet:
    {
        auto ptr = reinterpret_cast<GetSet*>(static_cast<uintptr_t>(Val.GetUint().value()));
        Val = ptr->Get();
        delete ptr;
        Type = Val.IsEmpty() ? LocateType::Empty : LocateType::Arg;
    }
    [[fallthrough]];
    default:
        return Val;
    }
}


void Serializer::Stringify(std::u32string& output, const Expr& arg, const bool requestParenthese)
{
    arg.Visit([&](const auto& data)
        {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T, std::nullopt_t>)
                Expects(false);
            else if constexpr (std::is_same_v<T, const BinaryExpr*>)
                Stringify(output, data, requestParenthese);
            else if constexpr (std::is_same_v<T, const TernaryExpr*>)
                Stringify(output, data, requestParenthese);
            else
                Stringify(output, data);
        });
}

void Serializer::Stringify(std::u32string& output, const FuncCall* call)
{
    output.append(call->FullFuncName()).push_back(U'(');
    size_t i = 0;
    for (const auto& arg : call->Args)
    {
        if (i++ > 0)
            output.append(U", "sv);
        Stringify(output, arg);
    }
    output.push_back(U')');
}

void Serializer::Stringify(std::u32string& output, const UnaryExpr* expr)
{
    Expects(EmbedOpHelper::IsUnaryOp(expr->Operator));
    switch (expr->Operator)
    {
    case EmbedOps::Not:
        output.append(U"!"sv);
        Stringify(output, expr->Operand, true);
        break;
    case EmbedOps::CheckExist:
        output.append(U"?"sv);
        Stringify(output, expr->Operand, true);
        break;
    default:
        Expects(false);
        break;
    }
}

void Serializer::Stringify(std::u32string& output, const BinaryExpr* expr, const bool requestParenthese)
{
    Expects(!EmbedOpHelper::IsUnaryOp(expr->Operator));
    std::u32string_view opStr;
#define SET_OP_STR(op, str) case EmbedOps::op: opStr = U##str##sv; break;
    switch (expr->Operator)
    {
        SET_OP_STR(Equal,       " == ");
        SET_OP_STR(NotEqual,    " != ");
        SET_OP_STR(Less,        " < ");
        SET_OP_STR(LessEqual,   " <= ");
        SET_OP_STR(Greater,     " > ");
        SET_OP_STR(GreaterEqual," >= ");
        SET_OP_STR(And,         " && ");
        SET_OP_STR(Or,          " || ");
        SET_OP_STR(Add,         " + ");
        SET_OP_STR(Sub,         " - ");
        SET_OP_STR(Mul,         " * ");
        SET_OP_STR(Div,         " / ");
        SET_OP_STR(Rem,         " % ");
        SET_OP_STR(ValueOr,     " ?? ");
    default:
        assert(false); // Expects(false);
        return;
    }
#undef SET_OP_STR
    if (requestParenthese)
        output.push_back(U'(');
    Stringify(output, expr->LeftOperand, true);
    output.append(opStr);
    Stringify(output, expr->RightOperand, true);
    if (requestParenthese)
        output.push_back(U')');
}

void Serializer::Stringify(std::u32string& output, const TernaryExpr* expr, const bool requestParenthese)
{
    if (requestParenthese)
        output.push_back(U'(');
    Stringify(output, expr->Condition, true);
    output.append(U" ? "sv);
    Stringify(output, expr->LeftOperand, true);
    output.append(U" : "sv);
    Stringify(output, expr->RightOperand, true);
    if (requestParenthese)
        output.push_back(U')');
}

void Serializer::Stringify(std::u32string& output, const QueryExpr* expr)
{
    Stringify(output, expr->Target);
    Stringify(output, *static_cast<const SubQuery*>(expr));
}

void Serializer::Stringify(std::u32string& output, const AssignExpr* expr)
{
    Stringify(output, expr->Target);
    Stringify(output, expr->Queries);
    output.append(U" = "sv);
    Stringify(output, expr->Statement);
}

void Serializer::Stringify(std::u32string& output, const LateBindVar& var)
{
    if (HAS_FIELD(var.Info, LateBindVar::VarInfo::Root))
        output.push_back(U'`');
    else if (HAS_FIELD(var.Info, LateBindVar::VarInfo::Local))
        output.push_back(U':');
    output.append(var.Name);
}

void Serializer::Stringify(std::u32string& output, const std::u32string_view str)
{
    output.push_back(U'"');
    output.append(str); //TODO:
    output.push_back(U'"');
}

void Serializer::Stringify(std::u32string& output, const uint64_t u64)
{
    const auto str = fmt::to_string(u64);
    output.insert(output.end(), str.begin(), str.end());
}

void Serializer::Stringify(std::u32string& output, const int64_t i64)
{
    const auto str = fmt::to_string(i64);
    output.insert(output.end(), str.begin(), str.end());
}

void Serializer::Stringify(std::u32string& output, const double f64)
{
    const auto str = fmt::to_string(f64);
    output.insert(output.end(), str.begin(), str.end());
}

void Serializer::Stringify(std::u32string& output, const bool boolean)
{
    output.append(boolean ? U"true"sv : U"false"sv);
}

void Serializer::Stringify(std::u32string& output, const SubQuery& subq)
{
    for (size_t i = 0; i < subq.Size(); ++i)
    {
        const auto [type, query] = subq[i];
        switch (type)
        {
        case SubQuery::QueryType::Index:
            output.push_back(U'[');
            Stringify(output, query, false);
            output.push_back(U']');
            break;
        case SubQuery::QueryType::Sub:
            output.push_back(U'.');
            output.append(query.GetVar<Expr::Type::Str>());
            break;
        default:
            break;
        }
    }
}


}
