#include "NailangPch.h"
#include "NailangStruct.h"
#include "NailangRuntime.h"
#include "common/ContainerEx.hpp"
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
    RET_NAME(BitAnd);
    RET_NAME(BitOr);
    RET_NAME(BitXor);
    RET_NAME(ValueOr);
    RET_NAME(CheckExist);
    RET_NAME(Not);
    default: return U"Unknwon"sv;
    }
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
    case Expr::Type::Assign:  return U"assign-expr"sv;
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
struct NativeGetSet final : public GetSet::Handler
{
    static constexpr auto DisableSet = common::is_specialization<T, std::basic_string_view>::value;
    [[nodiscard]] ArgAccess GetAccess(const GetSet& data) const noexcept override
    {
        auto ret = ArgAccess::Empty;
        if (data.Meta & NativeWrapper::MetaType::EnableGet)
            ret |= ArgAccess::ReadOnly;
        if constexpr(!DisableSet)
            if (data.Meta & NativeWrapper::MetaType::EnableSet)
                ret |= ArgAccess::WriteOnly;
        return ret;
    }
    [[nodiscard]] Arg Get(const GetSet& data) const override
    {
        Expects(data.Meta & NativeWrapper::MetaType::EnableGet);
        [[maybe_unused]] const bool shouldMove = data.Meta & NativeWrapper::MetaType::MoveGet;
        const auto ptr = reinterpret_cast<T*>(static_cast<uintptr_t>(data.Ptr)) + data.Idx;
        if constexpr (std::is_floating_point_v<T> || std::is_same_v<T, half_float::half>)
            return static_cast<double>(*ptr);
        else if constexpr (std::is_unsigned_v<T>)
            return static_cast<uint64_t>(*ptr);
        else if constexpr (std::is_signed_v<T>)
            return static_cast<int64_t>(*ptr);
        else if constexpr (std::is_same_v<T, Arg>)
        {
            if (shouldMove)
                return std::move(*ptr);
            else
                return *ptr;
        }
        else if constexpr (std::is_same_v<T, std::u32string>)
        {
            if (shouldMove)
                return *ptr;
            else
                return std::u32string_view(*ptr);
        }
        else if constexpr (std::is_same_v<T, std::u32string_view>)
        {
            if (shouldMove)
                return std::u32string(*ptr);
            else
                return *ptr;
        }
        else if constexpr (common::is_specialization<T, std::basic_string>::value || common::is_specialization<T, std::basic_string_view>::value)
        {
            using Char = typename T::value_type;
            constexpr auto chset = std::is_same_v<Char, char> ? common::str::Charset::UTF8 : common::str::DefaultCharset<Char>;
            return common::str::to_u32string(*ptr, chset);
        }
        else if constexpr (std::is_same_v<T, bool>)
            return *ptr;
        else
        {
            Expects(false);
            return {};
        }
    }
    [[nodiscard]] bool Set(const GetSet& data, Arg arg) const override
    {
        Expects(data.Meta & NativeWrapper::MetaType::EnableSet);
        [[maybe_unused]] const auto ptr = reinterpret_cast<T*>(static_cast<uintptr_t>(data.Ptr)) + data.Idx;
        if constexpr (std::is_floating_point_v<T>)
        {
            if (const auto val = arg.GetFP(); val.has_value())
            {
                *ptr = static_cast<T>(*val);
                return true;
            }
        }
        else if constexpr (std::is_same_v<T, half_float::half>)
        {
            if (const auto val = arg.GetFP(); val.has_value())
            {
                *ptr = static_cast<T>(static_cast<float>(*val));
                return true;
            }
        }
        else if constexpr (std::is_unsigned_v<T>)
        {
            if (const auto val = arg.GetUint(); val.has_value())
            {
                *ptr = static_cast<T>(*val);
                return true;
            }
        }
        else if constexpr (std::is_signed_v<T>)
        {
            if (const auto val = arg.GetInt(); val.has_value())
            {
                *ptr = static_cast<T>(*val);
                return true;
            }
        }
        else if constexpr (std::is_same_v<T, Arg>)
        {
            *ptr = std::move(arg);
            return true;
        }
        else if constexpr (common::is_specialization<T, std::basic_string>::value)
        {
            if (const auto val = arg.GetStr(); val.has_value())
            {
                using U = typename T::value_type;
                if constexpr (std::is_same_v<U, char32_t>)
                    *ptr = *val;
                else if constexpr (std::is_same_v<U, u8ch_t>)
                    *ptr = common::str::to_u8string(*val);
                else if constexpr (std::is_same_v<U, char16_t>)
                    *ptr = common::str::to_u16string(*val);
                else
                    static_assert(!common::AlwaysTrue<T>, "unsupported char type");
                return true;
            }
        }
        else
            Expects(false);
        return false;
    }
};

const GetSet::Handler* NativeWrapper::GetGetSet(Type type) noexcept
{
#define RET(tenum, t) case Type::tenum: { static NativeGetSet<t> Host; return &Host; }
    switch (type)
    {
    NATIVE_TYPE_EACH(RET)
    default: return nullptr;
    }
#undef RET
}


template<typename T> struct U8StrFix { using Type = T; };
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
template<> struct U8StrFix<std::u8string> { using Type = std::string; };
template<> struct U8StrFix<std::u8string_view> { using Type = std::string_view; };
#endif
ArrayRef::SpanVariant ArrayRef::GetSpan() const noexcept
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

void ArrayRef::PrintToStr(std::u32string& str, std::u32string_view delim) const
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
    if (HAS_FIELD(TypeData, Type::OwnershipBit))
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
        return GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::BoolBit).GetBool();
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
        return GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::NumberBit | Type::BoolBit).GetUint();
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
        return GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::NumberBit | Type::BoolBit).GetUint();
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
        return GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::NumberBit | Type::BoolBit).GetFP();
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
common::str::StrVariant<char32_t> ArgToString<ArrayRef>(const ArrayRef& val) noexcept
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


#define TYPE_ITEM(type, name) { common::enum_cast(type), U"" name ""sv }
constexpr std::pair<uint16_t, std::u32string_view> ArgTypeMappings[] =
{
    TYPE_ITEM(Arg::Type::CategoryGetSet, "getset"),
    TYPE_ITEM(Arg::Type::CategoryArgPtr, "argptr"),
    TYPE_ITEM(Arg::Type::Empty,     "empty"),
    TYPE_ITEM(Arg::Type::Var   | Arg::Type::ConstBit, "variable"),
    TYPE_ITEM(Arg::Type::Var,       "variable"),
    TYPE_ITEM(Arg::Type::Array | Arg::Type::ConstBit, "array"),
    TYPE_ITEM(Arg::Type::Array,     "array"),
    TYPE_ITEM(Arg::Type::U32Str,    "string"),
    TYPE_ITEM(Arg::Type::U32Sv,     "string_view"),
    TYPE_ITEM(Arg::Type::Uint,      "uint"),
    TYPE_ITEM(Arg::Type::Int,       "int"),
    TYPE_ITEM(Arg::Type::FP,        "fp"),
    TYPE_ITEM(Arg::Type::Bool,      "bool"),
    TYPE_ITEM(Arg::Type::Boolable,  "boolable"),
    TYPE_ITEM(Arg::Type::String,    "string-ish"),
    TYPE_ITEM(Arg::Type::ArrayLike, "array-like"),
    TYPE_ITEM(Arg::Type::Number,    "number"),
    TYPE_ITEM(Arg::Type::Integer,   "integer"),
    TYPE_ITEM(Arg::Type::BoolBit,   "(bool)"),
    TYPE_ITEM(Arg::Type::StringBit, "(str)"),
    TYPE_ITEM(Arg::Type::NumberBit, "(num)"),
    TYPE_ITEM(Arg::Type::ArrayBit,  "(arr)"),
    TYPE_ITEM(Arg::Type::BoolBit   | Arg::Type::StringBit,  "(bool|str)"),
    TYPE_ITEM(Arg::Type::BoolBit   | Arg::Type::NumberBit,  "(bool|num)"),
    TYPE_ITEM(Arg::Type::BoolBit   | Arg::Type::ArrayBit,   "(bool|arr)"),
    TYPE_ITEM(Arg::Type::StringBit | Arg::Type::NumberBit,  "(str|num)"),
    TYPE_ITEM(Arg::Type::StringBit | Arg::Type::ArrayBit,   "(str|arr)"),
    TYPE_ITEM(Arg::Type::NumberBit | Arg::Type::ArrayBit,   "(num|arr)"),
    TYPE_ITEM(Arg::Type::BoolBit   | Arg::Type::StringBit | Arg::Type::NumberBit,   "(bool|str|num)"),
    TYPE_ITEM(Arg::Type::BoolBit   | Arg::Type::StringBit | Arg::Type::ArrayBit,    "(bool|str|arr)"),
    TYPE_ITEM(Arg::Type::BoolBit   | Arg::Type::NumberBit | Arg::Type::ArrayBit,    "(bool|num|arr)"),
    TYPE_ITEM(Arg::Type::StringBit | Arg::Type::NumberBit | Arg::Type::ArrayBit,    "(str|num|arr)"),
    TYPE_ITEM(Arg::Type::BoolBit   | Arg::Type::StringBit | Arg::Type::NumberBit | Arg::Type::ArrayBit, "(bool|str|num|arr)"),
};
#undef TYPE_ITEM
std::u32string_view Arg::TypeName(const Arg::Type type) noexcept
{
    constexpr auto Mapping = common::container::BuildTableStore<ArgTypeMappings>();
    return Mapping(common::enum_cast(type)).value_or(U"unknown"sv);
}

void Arg::Decay()
{
    if (IsGetSet())
        *this = GetGetSet().Get();
    else if (IsArgPtr())
    {
        const auto isConst = !HAS_FIELD(static_cast<ArgAccess>(Data3), ArgAccess::WriteOnly);
        *this = *reinterpret_cast<const Arg*>(Data0.Uint);
        if (IsCustom() && isConst)
            TypeData |= Type::ConstBit;
    }
}
bool Arg::Set(Arg val)
{
    if (IsGetSet())
        return GetGetSet().Set(std::move(val));
    else if (IsArgPtr())
    {
        if (!MATCH_FIELD(static_cast<ArgAccess>(Data3), ArgAccess::Assignable))
            return false;
        *reinterpret_cast<Arg*>(Data0.Uint) = std::move(val);
        return true;
    }
    if (IsCustom())
        return GetCustom().Call<&CustomVar::Handler::HandleAssign>(std::move(val));
    return false;
}

static CompareResult NumCompare(const Arg& left, const Arg& right) noexcept
{
    // Expects(HAS_FIELD(right.TypeData, Arg::Type::NumberBit));
    if (!HAS_FIELD(right.TypeData, Arg::Type::NumberBit))
        return {};
    if (const bool isLeftFP = HAS_FIELD(left.TypeData, Arg::Type::FPBit), isRightFP = HAS_FIELD(right.TypeData, Arg::Type::FPBit);
        isLeftFP || isRightFP)
    {
        static_assert(std::numeric_limits<double>::is_iec559, "Need support for IEEE 754");
        const auto l = left.GetFP().value(), r = right.GetFP().value();
        if (std::isnan(l) || std::isnan(r))
            return CompareResultCore::NotEqual | CompareResultCore::Equality;
        if (l < r) return CompareResultCore::Less    | CompareResultCore::StrongOrder;
        if (l > r) return CompareResultCore::Greater | CompareResultCore::StrongOrder;
        return CompareResultCore::Equal | CompareResultCore::StrongOrder;
    }
    const bool isLeftUnsigned  = HAS_FIELD( left.TypeData, Arg::Type::UnsignedBit);
    const bool isRightUnsigned = HAS_FIELD(right.TypeData, Arg::Type::UnsignedBit);
    const bool isLeftNeg  = !isLeftUnsigned  &&  left.GetVar<Arg::Type::Int, false>() < 0;
    const bool isRightNeg = !isRightUnsigned && right.GetVar<Arg::Type::Int, false>() < 0;
    if (isLeftNeg && !isRightNeg)
        return CompareResultCore::Less | CompareResultCore::StrongOrder;
    if (!isLeftNeg && isRightNeg)
        return CompareResultCore::Greater | CompareResultCore::StrongOrder;
    if (isLeftNeg) // both neg
    {
        const auto l =  left.GetVar<Arg::Type::Int>();
        const auto r = right.GetVar<Arg::Type::Int>();
        if (l < r) return CompareResultCore::Less    | CompareResultCore::StrongOrder;
        if (l > r) return CompareResultCore::Greater | CompareResultCore::StrongOrder;
        return CompareResultCore::Equal | CompareResultCore::StrongOrder;
    }
    else // both pos
    {
        const auto l =  left.GetVar<Arg::Type::Uint, false>();
        const auto r = right.GetVar<Arg::Type::Uint, false>();
        if (l < r) return CompareResultCore::Less    | CompareResultCore::StrongOrder;
        if (l > r) return CompareResultCore::Greater | CompareResultCore::StrongOrder;
        return CompareResultCore::Equal | CompareResultCore::StrongOrder;
    }
}
CompareResult Arg::NativeCompare(const Arg& right) const noexcept
{
    Expects(!IsCustom());
    if (HAS_FIELD(TypeData, Type::StringBit)) // direct process string, no decay
    {
        const auto r_ = right.GetStr();
        if (r_.has_value())
        {
            const auto l = GetVar<Type::U32Sv, false>(), r = r_.value();
            const auto ret = std::char_traits<char32_t>::compare(l.data(), r.data(), std::min(l.size(), r.size()));
            if (ret < 0)             return CompareResultCore::StrongOrder | CompareResultCore::Less;
            if (ret > 0)             return CompareResultCore::StrongOrder | CompareResultCore::Greater;
            if (l.size() < r.size()) return CompareResultCore::StrongOrder | CompareResultCore::Less;
            if (l.size() > r.size()) return CompareResultCore::StrongOrder | CompareResultCore::Greater;
            return CompareResultCore::StrongOrder | CompareResultCore::Equal;
        }
    }
    else if (HAS_FIELD(TypeData, Type::NumberBit))
    {
        if (right.IsCustom())
            return NumCompare(*this, right.GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::NumberBit));
        else
            return NumCompare(*this, right);
    }
    else if (IsBool())
    {
        const auto r = right.GetBool();
        if (r.has_value())
            return CompareResultCore::Equality |
                (GetVar<Type::Bool>() == *r ? CompareResultCore::Equal : CompareResultCore::NotEqual);
    }
    return {};
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

Arg Arg::HandleUnary(const EmbedOps op) const
{
    Expects(op == EmbedOps::Not || op == EmbedOps::BitNot);
    if (op == EmbedOps::Not)
    {
        const auto val = GetBool();
        if (val.has_value())
            return !val.value();
    }
    else if (op == EmbedOps::BitNot)
    {
        if (MATCH_FIELD(TypeData, Type::Integer)) // only accept integer
            return ~GetVar<Type::Uint, false>();
    }
    return {};
}
Arg Arg::HandleBinary(const EmbedOps op, const Arg& right) const
{
    const auto opCat = EmbedOpHelper::GetCategory(op);
    Expects(opCat != EmbedOpHelper::OpCategory::Other); // should not be handled here
    switch (opCat)
    {
    case EmbedOpHelper::OpCategory::Logic: // should not enter in most cases
    {
        const auto l_ = GetBool();
        if (!l_.has_value())
            return {};
        const auto r_ = right.GetBool();
        if (!r_.has_value())
            return {};
        return op == EmbedOps::And ? (*l_ && *r_) : (*l_ || *r_);
    }
    case EmbedOpHelper::OpCategory::Compare:
    {
        const auto ret1 = Compare(right);
        const auto val1 = ret1.GetResult();
        using CR = CompareResultCore;
        switch (op)
        {
        case EmbedOps::Equal:           if (ret1.HasEuqality())  return val1 == CR::Equal;
            break;
        case EmbedOps::NotEqual:        if (ret1.HasEuqality())  return val1 != CR::Equal;
            break;
        case EmbedOps::Less:            if (ret1.HasOrderable()) return val1 == CR::Less;
            break;
        case EmbedOps::LessEqual:       if (ret1.HasOrderable()) return val1 == CR::Less || val1 == CR::Equal;
            break;
        case EmbedOps::Greater:         if (ret1.HasOrderable()) return val1 == CR::Greater;
            break;
        case EmbedOps::GreaterEqual:    if (ret1.HasOrderable()) return val1 == CR::Greater || val1 == CR::Equal;
            break;
        default: return {}; // should not happen
        }
        // try reverse compare
        const auto ret2 = right.Compare(*this);
        const auto val2 = ret2.GetResult();
        switch (op)
        {
        case EmbedOps::Equal:           if (ret2.HasEuqality())  return val2 == CR::Equal;
            break;
        case EmbedOps::NotEqual:        if (ret2.HasEuqality())  return val2 != CR::Equal;
            break;
        case EmbedOps::Less:            if (ret1.HasReverseOrderable()) return val2 == CR::Greater || val2 == CR::Equal;
            break;
        case EmbedOps::LessEqual:       if (ret1.HasReverseOrderable()) return val2 == CR::Greater;
            break;
        case EmbedOps::Greater:         if (ret1.HasReverseOrderable()) return val2 == CR::Less || val2 == CR::Equal;
            break;
        case EmbedOps::GreaterEqual:    if (ret1.HasReverseOrderable()) return val2 == CR::Less;
            break;
        default: return {}; // should not happen
        }
        return {};
    }
    case EmbedOpHelper::OpCategory::Bitwise:
    {
        if (!MATCH_FIELD(TypeData, Type::Integer) || !MATCH_FIELD(right.TypeData, Type::Integer)) // only accept integer
            return {};
        const auto l = GetVar<Type::Uint, false>(), r = right.GetVar<Type::Uint, false>();
        switch (op)
        {
        case EmbedOps::BitAnd:  return l & r;
        case EmbedOps::BitOr:   return l | r;
        case EmbedOps::BitXor:  return l ^ r;
        case EmbedOps::BitShiftLeft:    return HAS_FIELD(TypeData, Type::UnsignedBit) ? Arg(l << r) : Arg(static_cast<int64_t>(l) << r);
        case EmbedOps::BitShiftRight:   return HAS_FIELD(TypeData, Type::UnsignedBit) ? Arg(l >> r) : Arg(static_cast<int64_t>(l) >> r);
        default:                return {}; // should not happen
        }
    }
    default:
        Expects(opCat == EmbedOpHelper::OpCategory::Arth);
        if (HAS_FIELD(TypeData, Type::StringBit)) // direct process string
        {
            if (op == EmbedOps::Add) // only support add
            {
                const auto r_ = right.GetStr();
                if (r_.has_value())
                {
                    const auto l = GetVar<Type::U32Sv, false>(), r = r_.value();
                    std::u32string ret;
                    ret.reserve(l.size() + r.size());
                    ret.append(l).append(r);
                    return ret;
                }
            }
            return {};
        }
        if (HAS_FIELD(TypeData, Type::NumberBit)) // process as number
        {
            if (right.IsCustom())
                return NumArthOp(op, *this, right.GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::NumberBit));
            else
                return NumArthOp(op, *this, right);
        }
        else if (TypeData == Type::Bool) // consider right can be number
        {
            if (right.IsCustom())
                return NumArthOp(op, *this, right.GetCustom().Call<&CustomVar::Handler::ConvertToCommon>(Type::NumberBit));
            if (HAS_FIELD(TypeData, Type::NumberBit))
                return NumArthOp(op, *this, right);
        }
        return {};
    }
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

Arg Arg::HandleSubFields(SubQuery<const Expr>& subfields)
{
    Expects(subfields.Size() > 0);
    if (IsCustom())
    {
        return GetCustom().Call<&CustomVar::Handler::HandleSubFields>(subfields);
    }
    const auto field = subfields[0].GetVar<Expr::Type::Str>();
    if (IsArray())
    {
        const auto arr = GetVar<Type::Array>();
        if (field == U"Length"sv)
        {
            subfields.Consume();
            return static_cast<uint64_t>(arr.Length);
        }
        return {};
    }
    if (IsStr())
    {
        const auto str = GetStr().value();
        if (field == U"Length"sv)
        {
            subfields.Consume();
            return uint64_t(str.size());
        }
        return {};
    }
    return {};
}
Arg Arg::HandleIndexes(SubQuery<Arg>& indexes)
{
    Expects(indexes.Size() > 0);
    if (IsCustom())
    {
        return GetCustom().Call<&CustomVar::Handler::HandleIndexes>(indexes);
    }
    const auto& idxval = indexes[0];
    if (IsArray())
    {
        const auto arr = GetVar<Type::Array>();
        const auto idx = NailangHelper::BiDirIndexCheck(arr.Length, idxval, &indexes.GetRaw(0));
        indexes.Consume();
        return arr.Access(idx);
    }
    if (IsStr())
    {
        const auto str = GetStr().value();
        const auto idx = NailangHelper::BiDirIndexCheck(str.size(), idxval, &indexes.GetRaw(0));
        indexes.Consume();
        if (TypeData == Type::U32Str)
            return std::u32string(1, str[idx]);
        else // (TypeData == Type::U32Sv)
            return str.substr(idx, 1);
    }
    return {};
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
Arg CustomVar::Handler::HandleSubFields(const CustomVar& var, SubQuery<const Expr>& subfields)
{
    Expects(subfields.Size() > 0);
    auto result = SubfieldGetter(var, subfields[0].GetVar<Expr::Type::Str>());
    if (!result.IsEmpty())
    {
        subfields.Consume();
        return result;
    }
    return {};
}
Arg CustomVar::Handler::HandleIndexes(const CustomVar& var, SubQuery<Arg>& indexes)
{
    Expects(indexes.Size() > 0);
    auto result = IndexerGetter(var, indexes[0], indexes.GetRaw(0));
    if (!result.IsEmpty())
    {
        indexes.Consume();
        return result;
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
    case EmbedOps::BitNot:
        output.append(U"~"sv);
        Stringify(output, expr->Operand, true);
        break;
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
        SET_OP_STR(Equal,           " == ");
        SET_OP_STR(NotEqual,        " != ");
        SET_OP_STR(Less,            " < ");
        SET_OP_STR(LessEqual,       " <= ");
        SET_OP_STR(Greater,         " > ");
        SET_OP_STR(GreaterEqual,    " >= ");
        SET_OP_STR(And,             " && ");
        SET_OP_STR(Or,              " || ");
        SET_OP_STR(Add,             " + ");
        SET_OP_STR(Sub,             " - ");
        SET_OP_STR(Mul,             " * ");
        SET_OP_STR(Div,             " / ");
        SET_OP_STR(Rem,             " % ");
        SET_OP_STR(BitAnd,          " & ");
        SET_OP_STR(BitOr,           " | ");
        SET_OP_STR(BitXor,          " ^ ");
        SET_OP_STR(BitShiftLeft,    " << ");
        SET_OP_STR(BitShiftRight,   " >> ");
        SET_OP_STR(ValueOr,         " ?? ");
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
    Stringify(output, expr->GetQueries(), expr->TypeData);
}

void Serializer::Stringify(std::u32string& output, const AssignExpr* expr)
{
    Stringify(output, expr->Target);
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

void Serializer::Stringify(std::u32string& output, const common::span<const Expr> queries, QueryExpr::QueryType type)
{
    for (const auto& query : queries)
    {
        if (type == QueryExpr::QueryType::Index)
        {
            output.push_back(U'[');
            Stringify(output, query, false);
            output.push_back(U']');
        }
        else
        {
            output.push_back(U'.');
            output.append(query.GetVar<Expr::Type::Str>());
        }
    }
}


}
