#include "NailangStruct.h"
#include "StringUtil/Format.h"
#include "StringUtil/Convert.h"
#include "common/AlignedBase.hpp"

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include <cassert>

namespace xziar::nailang
{
using namespace std::string_view_literals;


std::pair<size_t, size_t> MemoryPool::Usage() const noexcept
{
    size_t used = 0, unused = 0;
    for (const auto& trunk : Trunks)
    {
        used += trunk.Offset, unused += trunk.Avaliable;
    }
    return { used, used + unused };
}


static constexpr PartedName::PartType DummyPart[1] = { {(uint16_t)0, (uint16_t)0} };
TempPartedNameBase::TempPartedNameBase(std::u32string_view name, common::span<const PartedName::PartType> parts, uint16_t info)
    : Var(name, parts, info)
{
    Expects(parts.size() > 0);
    constexpr auto SizeP = sizeof(PartedName::PartType);
    const auto partSize = parts.size() * SizeP;
    switch (parts.size())
    {
    case 1:
        break;
    case 2:
    case 3:
    case 4:
        memcpy_s(Extra.Parts, partSize, parts.data(), partSize);
        break;
    default:
    {
        const auto ptr = reinterpret_cast<PartedName*>(malloc(sizeof(PartedName) + partSize));
        Extra.Ptr = ptr;
        new (ptr) PartedName(name, parts, info);
        memcpy_s(ptr + 1, partSize, parts.data(), partSize);
        break;
    }
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


Arg::Arg(const CustomVar& var) noexcept :
    Data0(reinterpret_cast<uint64_t>(var.Host)), Data1(var.Meta0), Data2(var.Meta1), Data3(var.Meta2), TypeData(Type::Var)
{
    Expects(var.Host != nullptr);
    auto& var_ = GetCustom();
    var_.Host->IncreaseRef(var_);
}
Arg::Arg(const Arg& other) noexcept :
    Data0(other.Data0), Data1(other.Data1), Data2(other.Data2), Data3(other.Data3), TypeData(other.TypeData)
{
    if (TypeData == Type::U32Str && Data1 > 0)
    {
        auto str = other.GetVar<Type::U32Str>();
        common::SharedString<char32_t>::PlacedIncrease(str);
    }
    else if (TypeData == Type::Var)
    {
        auto& var = GetCustom();
        var.Host->IncreaseRef(var);
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
    if (TypeData == Type::U32Str && Data1 > 0)
    {
        auto str = other.GetVar<Type::U32Str>();
        common::SharedString<char32_t>::PlacedIncrease(str);
    }
    else if (TypeData == Type::Var)
    {
        auto& var = GetCustom();
        var.Host->IncreaseRef(var);
    }
    return *this;
}

void Arg::Release() noexcept
{
    if (TypeData == Type::U32Str && Data1 > 0)
    {
        auto str = GetVar<Type::U32Str>();
        common::SharedString<char32_t>::PlacedDecrease(str);
    }
    else if (TypeData == Type::Var)
    {
        auto& var = GetCustom();
        var.Host->DecreaseRef(var);
    }
    TypeData = Type::Empty;
}

std::optional<bool> Arg::GetBool() const noexcept
{
    switch (TypeData)
    {
    case Type::Uint:    return  GetVar<Type::Uint>() != 0;
    case Type::Int:     return  GetVar<Type::Int>() != 0;
    case Type::FP:      return  GetVar<Type::FP>() != 0;
    case Type::Bool:    return  GetVar<Type::Bool>();
    case Type::U32Str:  return !GetVar<Type::U32Str>().empty();
    case Type::U32Sv:   return !GetVar<Type::U32Sv>().empty();
    case Type::Var:     return  ConvertCustomType(Type::Bool).GetBool();
    default:            return {};
    }
}
std::optional<uint64_t> Arg::GetUint() const noexcept
{
    switch (TypeData)
    {
    case Type::Uint:    return GetVar<Type::Uint>();
    case Type::Int:     return static_cast<uint64_t>(GetVar<Type::Int>());
    case Type::FP:      return static_cast<uint64_t>(GetVar<Type::FP>());
    case Type::Bool:    return GetVar<Type::Bool>() ? 1 : 0;
    case Type::Var:     return ConvertCustomType(Type::Uint).GetUint();
    default:            return {};
    }
}
std::optional<int64_t> Arg::GetInt() const noexcept
{
    switch (TypeData)
    {
    case Type::Uint:    return static_cast<int64_t>(GetVar<Type::Uint>());
    case Type::Int:     return GetVar<Type::Int>();
    case Type::FP:      return static_cast<int64_t>(GetVar<Type::FP>());
    case Type::Bool:    return GetVar<Type::Bool>() ? 1 : 0;
    case Type::Var:     return ConvertCustomType(Type::Int).GetInt();
    default:            return {};
    }
}
std::optional<double> Arg::GetFP() const noexcept
{
    switch (TypeData)
    {
    case Type::Uint:    return static_cast<double>(GetVar<Type::Uint>());
    case Type::Int:     return static_cast<double>(GetVar<Type::Int>());
    case Type::FP:      return GetVar<Type::FP>();
    case Type::Bool:    return GetVar<Type::Bool>() ? 1. : 0.;
    case Type::Var:     return ConvertCustomType(Type::FP).GetFP();
    default:            return {};
    }
}
std::optional<std::u32string_view> Arg::GetStr() const noexcept
{
    switch (TypeData)
    {
    case Type::U32Str:  return GetVar<Type::U32Str>();
    case Type::U32Sv:   return GetVar<Type::U32Sv>();
    case Type::Var:     return ConvertCustomType(Type::U32Sv).GetStr();
    default:            return {};
    }
}

template<typename T>
static common::str::StrVariant<char32_t> ArgToString(const T& val) noexcept
{
    return fmt::format(FMT_STRING(U"{}"), val);
}
template<>
common::str::StrVariant<char32_t> ArgToString<CustomVar>(const CustomVar& val) noexcept
{
    return val.Host->ToString(val);
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
template<>
common::str::StrVariant<char32_t> ArgToString<std::nullopt_t>(const std::nullopt_t&) noexcept
{
    return {};
}
common::str::StrVariant<char32_t> Arg::ToString() const noexcept
{
    /*return Visit([](const auto& val) -> common::str::StrVariant<char32_t>
        {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, CustomVar>)
                return val.Host->ToString(val);
            else if constexpr (std::is_same_v<T, FixedArray>)
            {
                std::u32string ret = U"[";
                val.PrintToStr(ret, U", "sv);
                ret.append(U"]"sv);
                return std::move(ret);
            }
            else if constexpr (std::is_same_v<T, std::nullopt_t>)
                return {};
            else if constexpr (std::is_same_v<T, std::u32string_view>)
                return val;
            else
                return fmt::format(FMT_STRING(U"{}"), val);
        });*/
    return Visit([](const auto& val)
        {
            return ArgToString(val);
        });
}

std::u32string_view Arg::TypeName(const Arg::Type type) noexcept
{
    switch (type)
    {
    case Arg::Type::Empty:    return U"empty"sv;
    case Arg::Type::Var:      return U"variable"sv;
    case Arg::Type::U32Str:   return U"string"sv;
    case Arg::Type::U32Sv:    return U"string_view"sv;
    case Arg::Type::Uint:     return U"uint"sv;
    case Arg::Type::Int:      return U"int"sv;
    case Arg::Type::FP:       return U"fp"sv;
    case Arg::Type::Bool:     return U"bool"sv;
    case Arg::Type::Custom:   return U"custom"sv;
    case Arg::Type::Boolable: return U"boolable"sv;
    case Arg::Type::String:   return U"string-ish"sv;
    case Arg::Type::Number:   return U"number"sv;
    case Arg::Type::Integer:  return U"integer"sv;
    default:                  return U"error"sv;
    }
}


void CustomVar::Handler::IncreaseRef(CustomVar&) noexcept { }
void CustomVar::Handler::DecreaseRef(CustomVar&) noexcept { }
Arg CustomVar::Handler::IndexerGetter(const CustomVar&, const Arg&, const Expr&) 
{ 
    return {};
}
Arg CustomVar::Handler::SubfieldGetter(const CustomVar&, std::u32string_view)
{
    return {};
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
common::str::StrVariant<char32_t> CustomVar::Handler::ToString(const CustomVar&) noexcept
{
    return GetTypeName();
}
Arg CustomVar::Handler::ConvertToCommon(const CustomVar&, Arg::Type) noexcept
{
    return {};
}
std::u32string_view CustomVar::Handler::GetTypeName() noexcept 
{ 
    return U"CustomVar"sv;
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
ArgLocator ArgLocator::HandleQuery(SubQuery subq, NailangRuntimeBase& runtime)
{
    switch (Type)
    {
    case LocateType::Ptr:    return reinterpret_cast<Arg*>(static_cast<uintptr_t>(*Val.GetUint()))->HandleQuery(subq, runtime);
    case LocateType::GetSet: return reinterpret_cast<const GetSet*>(static_cast<uintptr_t>(*Val.GetUint()))->Get().HandleQuery(subq, runtime);
    case LocateType::Arg:    return Val.HandleQuery(subq, runtime);
    default:                 return {};
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


}
