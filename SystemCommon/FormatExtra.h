#include "Format.h"
#include "Exceptions.h"
#include "common/STLEx.hpp"
#include <boost/container/small_vector.hpp>


namespace common::str
{

class SYSCOMMONAPI ArgMismatchException : public BaseException
{
public:
    enum class Reasons : uint8_t { TooLessIndexArg, TooLessNamedArg, IndexArgTypeMismatch, NamedArgTypeMismatch };
private:
    COMMON_EXCEPTION_PREPARE(ArgMismatchException, BaseException,
        const Reasons Reason;
        const uint8_t MismatchIndex;
        const std::pair<uint8_t, uint8_t> IndexArgCount;
        const std::pair<uint8_t, uint8_t> NamedArgCount;
        const std::pair<ArgDispType, ArgRealType> ArgType;
        const std::string MismatchName;
        ExceptionInfo(const std::u16string_view msg, Reasons reason,
            std::pair<uint8_t, uint8_t> indexArgCount, std::pair<uint8_t, uint8_t> namedArgCount) noexcept;
        ExceptionInfo(const std::u16string_view msg, std::pair<uint8_t, uint8_t> indexArgCount, std::pair<uint8_t, uint8_t> namedArgCount,
            uint8_t idx, std::pair<ArgDispType, ArgRealType> types) noexcept;
        ExceptionInfo(const std::u16string_view msg, std::pair<uint8_t, uint8_t> indexArgCount, std::pair<uint8_t, uint8_t> namedArgCount,
            std::string&& name, std::pair<ArgDispType, ArgRealType> types) noexcept;
    );
public:
    ArgMismatchException(const std::u16string_view msg, Reasons reason,
        std::pair<uint8_t, uint8_t> indexArgCount, std::pair<uint8_t, uint8_t> namedArgCount)
        : BaseException(T_<ExceptionInfo>{}, msg, reason, indexArgCount, namedArgCount)
    { }
    ArgMismatchException(const std::u16string_view msg, std::pair<uint8_t, uint8_t> indexArgCount, std::pair<uint8_t, uint8_t> namedArgCount,
        uint8_t idx, std::pair<ArgDispType, ArgRealType> types)
        : BaseException(T_<ExceptionInfo>{}, msg, indexArgCount, namedArgCount, idx, types)
    { }
    ArgMismatchException(const std::u16string_view msg, std::pair<uint8_t, uint8_t> indexArgCount, std::pair<uint8_t, uint8_t> namedArgCount,
        std::string&& name, std::pair<ArgDispType, ArgRealType> types)
        : BaseException(T_<ExceptionInfo>{}, msg, indexArgCount, namedArgCount, std::move(name), types)
    { }
    [[nodiscard]] Reasons GetReason() const noexcept { return GetInfo().Reason; }
    [[nodiscard]] std::pair<uint8_t, uint8_t> GetIndexArgCount() const noexcept { return GetInfo().IndexArgCount; }
    [[nodiscard]] std::pair<uint8_t, uint8_t> GetNamedArgCount() const noexcept { return GetInfo().NamedArgCount; }
    [[nodiscard]] std::pair<ArgDispType, ArgRealType> GetMismatchType() const noexcept { return GetInfo().ArgType; }
    [[nodiscard]] std::variant<std::monostate, uint8_t, std::string_view> GetMismatchTarget() const noexcept 
    {
        const auto& info = GetInfo();
        if (!info.MismatchName.empty()) return info.MismatchName;
        if (info.MismatchIndex != UINT8_MAX) return info.MismatchIndex;
        return {};
    }
};


class COMMON_EMPTY_BASES DynamicTrimedResult : public FixedLenRefHolder<DynamicTrimedResult, uint32_t>
{
    friend RefHolder<DynamicTrimedResult>;
    friend FixedLenRefHolder<DynamicTrimedResult, uint32_t>;
private:
    static constexpr size_t NASize = sizeof(ParseResultCommon::NamedArgType);
    static constexpr size_t IASize = sizeof(ArgDispType);
    [[nodiscard]] forceinline uintptr_t GetDataPtr() const noexcept
    {
        return Pointer;
    }
    forceinline void Destruct() noexcept { }
protected:
    uintptr_t Pointer = 0;
    uint32_t StrSize = 0;
    uint16_t OpCount = 0;
    uint8_t NamedArgCount = 0;
    uint8_t IdxArgCount = 0;
    SYSCOMMONAPI DynamicTrimedResult(const ParseResultCommon& result, span<const uint8_t> opcodes, size_t strLength, size_t charSize);
    DynamicTrimedResult(const DynamicTrimedResult& other) noexcept :
        Pointer(other.Pointer), StrSize(other.StrSize), OpCount(other.OpCount), NamedArgCount(other.NamedArgCount), IdxArgCount(other.IdxArgCount)
    {
        this->Increase();
    }
    DynamicTrimedResult(DynamicTrimedResult&& other) noexcept :
        Pointer(other.Pointer), StrSize(other.StrSize), OpCount(other.OpCount), NamedArgCount(other.NamedArgCount), IdxArgCount(other.IdxArgCount)
    {
        other.Pointer = 0;
    }
    span<const uint8_t> GetOpcodes() const noexcept
    {
        return { reinterpret_cast<const uint8_t*>(GetStrPtr() + StrSize + IASize * IdxArgCount), OpCount };
    }
    span<const ArgDispType> GetIndexTypes() const noexcept
    {
        if (IdxArgCount > 0)
            return { reinterpret_cast<const ArgDispType*>(GetStrPtr() + StrSize), IdxArgCount };
        return {};
    }
    uintptr_t GetStrPtr() const noexcept
    {
        return Pointer + NASize * NamedArgCount;
    }
    span<const ParseResultCommon::NamedArgType> GetNamedTypes() const noexcept
    {
        if (NamedArgCount > 0)
            return { reinterpret_cast<const ParseResultCommon::NamedArgType*>(Pointer), NamedArgCount };
        return {};
    }
public:
    ~DynamicTrimedResult()
    {
        this->Decrease();
    }
};

template<typename Char>
class DynamicTrimedResultCh : public DynamicTrimedResult
{
public:
    template<uint16_t Size>
    DynamicTrimedResultCh(const ParseResult<Size>& result, std::basic_string_view<Char> str) :
        DynamicTrimedResult(result, { result.Opcodes, result.OpCount }, str.size(), sizeof(Char))
    {
        const auto strptr = reinterpret_cast<Char*>(GetStrPtr());
        memcpy_s(strptr, StrSize, str.data(), sizeof(Char) * str.size());
        strptr[str.size()] = static_cast<Char>('\0');
    }
    DynamicTrimedResultCh(const DynamicTrimedResultCh& other) noexcept : DynamicTrimedResult(other) {}
    DynamicTrimedResultCh(DynamicTrimedResultCh&& other) noexcept : DynamicTrimedResult(other) {}
    constexpr operator StrArgInfoCh<Char>() const noexcept
    {
        return { { GetOpcodes(), GetIndexTypes(), GetNamedTypes() },
            { reinterpret_cast<Char*>(GetStrPtr()), StrSize / sizeof(Char) - 1 } };
    }
    constexpr StrArgInfoCh<Char> ToStrArgInfo() const noexcept
    {
        return *this;
    }
};


struct DynamicArgPack
{
    boost::container::small_vector<uint16_t, 36> ArgStore;
    DynamicArgPack(size_t argCount) 
    {
        ArgStore.resize(argCount, uint16_t(0));
    }
    template<typename T>
    forceinline void Put(T arg, uint16_t idx) noexcept
    {
        constexpr auto NeedSize = sizeof(T);
        constexpr auto NeedSlot = (NeedSize + 1) / 2;
        const auto offset = ArgStore.size();
        ArgStore.resize(offset + NeedSlot);
        *reinterpret_cast<T*>(&ArgStore[offset]) = arg;
        ArgStore[idx] = static_cast<uint16_t>(offset);
    }
};


struct FormatSpecCacher
{
    std::vector<OpaqueFormatSpec> IndexSpec;
    std::vector<OpaqueFormatSpec> NamedSpec;
    SmallBitset CachedBitMap;
    SYSCOMMONAPI bool Cache(const StrArgInfo& strInfo, const ArgInfo& argInfo, const NamedMapper& mapper) noexcept;
    template<typename T, typename... Args>
    static auto CreateFrom(const T&, Args&&...);
};
template<typename Char>
struct FormatSpecCacherCh : private FormatSpecCacher
{
    const StrArgInfoCh<Char>& StrInfo;
    const ArgInfo& TheArgInfo;
    const NamedMapper& Mapping;
    FormatSpecCacherCh(const StrArgInfoCh<Char>& strInfo, const ArgInfo& argInfo, const NamedMapper& mapper) noexcept :
        StrInfo(strInfo), TheArgInfo(argInfo), Mapping(mapper)
    {
        Cache(StrInfo, TheArgInfo, Mapping);
    }
    SYSCOMMONAPI void FormatTo(std::basic_string<Char>& dst, span<const uint16_t> argStore);
    template<typename... Args>
    auto Format(std::basic_string<Char>& dst, Args&&... args)
    {
        // TODO: check equality of cache and real argsinfo
        static constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
        [[maybe_unused]] const auto mapping = ArgChecker::CheckDD(StrInfo, ArgsInfo);
        const auto argStore = ArgInfo::PackArgsStatic(std::forward<Args>(args)...);
        FormatTo(dst, argStore.ArgStore);
    }
};
template<typename T, typename... Args>
auto FormatSpecCacher::CreateFrom(const T&, Args&&...)
{
    using Char = typename std::decay_t<T>::CharType;
    static constexpr auto Mapping = ArgChecker::CheckSS<T, Args...>();
    static constexpr T StrInfo;
    static constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
    return FormatSpecCacherCh<Char>{ StrInfo, ArgsInfo, Mapping };
}


template<typename Char, typename Fmter>
struct CombinedExecutor : public FormatterExecutor, protected Fmter
{
    static_assert(std::is_base_of_v<Formatter<Char>, Fmter>, "Fmter need to derive from Formatter<Char>");
    using CFmter = CommonFormatter<Char>;
    using CTX = FormatterExecutor::Context;
    struct Context : public CTX
    {
        std::basic_string<Char>& Dst;
        std::basic_string_view<Char> FmtStr;
        constexpr Context(std::basic_string<Char>& dst, std::basic_string_view<Char> fmtstr) noexcept : 
            Dst(dst), FmtStr(fmtstr) { }
    };

    void OnBrace(CTX& ctx, bool isLeft) final
    {
        auto& context = static_cast<Context&>(ctx);
        context.Dst.push_back(ParseLiterals<Char>::BracePair[isLeft ? 0 : 1]);
    }
    void OnColor(CTX& ctx, ScreenColor color) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutColor(context.Dst, color);
    }
    
    void PutString(CTX& ctx, ::std::   string_view str, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutString(context.Dst, str, &spec);
    }
    void PutString(CTX& ctx, ::std::u16string_view str, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutString(context.Dst, str, &spec);
    }
    void PutString(CTX& ctx, ::std::u32string_view str, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutString(context.Dst, str, &spec);
    }
    void PutInteger(CTX& ctx, uint32_t val, bool isSigned, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutInteger(context.Dst, val, isSigned, spec);
    }
    void PutInteger(CTX& ctx, uint64_t val, bool isSigned, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutInteger(context.Dst, val, isSigned, spec);
    }
    void PutFloat  (CTX& ctx, float  val, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutFloat(context.Dst, val, spec);
    }
    void PutFloat  (CTX& ctx, double val, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutFloat(context.Dst, val, spec);
    }
    void PutPointer(CTX& ctx, uintptr_t val, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutPointer(context.Dst, val, spec);
    }

    void PutString(CTX& ctx, std::   string_view str, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutString(context.Dst, str, spec);
    }
    void PutString(CTX& ctx, std::u16string_view str, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutString(context.Dst, str, spec);
    }
    void PutString(CTX& ctx, std::u32string_view str, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutString(context.Dst, str, spec);
    }
    void PutInteger(CTX& ctx, uint32_t val, bool isSigned, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutInteger(context.Dst, val, isSigned, spec);
    }
    void PutInteger(CTX& ctx, uint64_t val, bool isSigned, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutInteger(context.Dst, val, isSigned, spec);
    }
    void PutFloat  (CTX& ctx, float  val, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutFloat(context.Dst, val, spec);
    }
    void PutFloat  (CTX& ctx, double val, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutFloat(context.Dst, val, spec);
    }
    void PutPointer(CTX& ctx, uintptr_t val, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutPointer(context.Dst, val, spec);
    }
    void PutDate   (CTX& ctx, std::string_view fmtStr, const std::tm& date) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutDateBase(context.Dst, fmtStr, date);
    }
protected:
    using Fmter::PutString;
    using Fmter::PutInteger;
    using Fmter::PutFloat;
    using Fmter::PutPointer;
    using Fmter::PutDate;
private:
    void OnFmtStr(CTX& ctx, uint32_t offset, uint32_t length) final
    {
        auto& context = static_cast<Context&>(ctx);
        context.Dst.append(context.FmtStr.data() + offset, length);
    }
};


template<typename Char>
struct FormatterBase::StaticExecutor
{
    struct Context 
    {
        std::basic_string<Char>& Dst;
        const StrArgInfoCh<Char>& StrInfo;
        const ArgInfo& TheArgInfo;
        span<const uint16_t> ArgStore;
        NamedMapper Mapping;
    };
    Formatter<Char>& Fmter;
    forceinline void OnFmtStr(Context& context, uint32_t offset, uint32_t length)
    {
        context.Dst.append(context.StrInfo.FormatString.data() + offset, length);
    }
    forceinline void OnBrace(Context& context, bool isLeft)
    {
        context.Dst.push_back(ParseLiterals<Char>::BracePair[isLeft ? 0 : 1]);
    }
    forceinline void OnColor(Context& context, ScreenColor color)
    {
        Fmter.PutColor(context.Dst, color);
    }
    SYSCOMMONAPI void OnArg(Context& context, uint8_t argIdx, bool isNamed, SpecReader& reader);
};


}
