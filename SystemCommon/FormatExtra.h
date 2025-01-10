#pragma once
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

class SYSCOMMONAPI ArgFormatException : public BaseException
{
private:
    COMMON_EXCEPTION_PREPARE(ArgFormatException, BaseException,
        const int16_t ArgIndex;
        const std::pair<ArgDispType, ArgRealType> ArgType;
        ExceptionInfo(const std::u16string_view msg, int16_t index, ArgDispType dispType, ArgRealType realType) noexcept;
    );
public:
    ArgFormatException(const std::u16string_view msg, int16_t index, ArgDispType dispType, ArgRealType realType)
        : BaseException(T_<ExceptionInfo>{}, msg, index, dispType, realType)
    { }
    [[nodiscard]] ArgDispType GetArgDisplayType() const noexcept { return GetInfo().ArgType.first; }
    [[nodiscard]] ArgRealType GetArgRealType() const noexcept { return GetInfo().ArgType.second; }
    [[nodiscard]] int16_t GetArgIndex() const noexcept { return GetInfo().ArgIndex; }
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
    forceinline void Put(const T& arg, uint16_t idx) noexcept
    {
        constexpr auto NeedSize = sizeof(T);
        constexpr auto NeedSlot = (NeedSize + 1) / 2;
        const auto offset = ArgStore.size();
        ArgStore.resize(offset + NeedSlot);
        memcpy(&ArgStore[offset], &arg, sizeof(T));
        //*reinterpret_cast<T*>(&ArgStore[offset]) = arg;
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
    const StrArgInfoCh<Char> StrInfo;
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
    
    void PutString(CTX& ctx, const void* str, size_t len, StringType type, const OpaqueFormatSpec& spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        CFmter::PutString(context.Dst, str, len, type, &spec);
    }
    forceinline void PutString(CTX& ctx, std::  string_view str, const OpaqueFormatSpec& spec)
    {
        PutString(ctx, str.data(), str.size(), StringType::UTF8, spec);
    }
    forceinline void PutString(CTX& ctx, std::u16string_view str, const OpaqueFormatSpec& spec)
    {
        PutString(ctx, str.data(), str.size(), StringType::UTF16, spec);
    }
    forceinline void PutString(CTX& ctx, std::u32string_view str, const OpaqueFormatSpec& spec)
    {
        PutString(ctx, str.data(), str.size(), StringType::UTF32, spec);
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

    void PutString(CTX& ctx, const void* str, size_t len, StringType type, const FormatSpec* spec) override
    {
        auto& context = static_cast<Context&>(ctx);
        Fmter::PutString(context.Dst, str, len, type, spec);
    }
    forceinline void PutString(CTX& ctx, std::   string_view str, const FormatSpec* spec)
    {
        PutString(ctx, str.data(), str.size(), StringType::UTF8, spec);
    }
    forceinline void PutString(CTX& ctx, std::u16string_view str, const FormatSpec* spec)
    {
        PutString(ctx, str.data(), str.size(), StringType::UTF16, spec);
    }
    forceinline void PutString(CTX& ctx, std::u32string_view str, const FormatSpec* spec)
    {
        PutString(ctx, str.data(), str.size(), StringType::UTF32, spec);
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
    void PutDate   (CTX& ctx, std::string_view fmtStr, const DateStructure& date) override
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


struct ParseResultCombineNamed : public ParseResultCommon
{
    static constexpr uint16_t NameTotalSize = 4096;
    uint16_t BufLength = 0;
    char NameBuffer[NameTotalSize] = { '\0' };
};
template<uint16_t NamedMapCount_>
struct ParseResultCombine : public ParseResultCombineNamed
{
    static constexpr uint16_t NamedMapCount = NamedMapCount_;
    uint8_t NamedMap[NamedMapCount] = { 0 };
};
template<>
struct ParseResultCombine<0> : public ParseResultCommon
{
    static constexpr uint16_t NamedMapCount = 0;
    uint16_t BufLength = 0;
    char* const NameBuffer = nullptr;
    uint8_t* const NamedMap = nullptr;
};

struct FormatterCombiner
{
    template<typename T, typename... Ts> 
    static forceinline constexpr typename T::CharType GetCharType(const T&, const Ts&...) noexcept
    {
        return '\0';
    }
    template<typename T, typename... Ts>
    static forceinline constexpr typename T::CharType GetCharType2() noexcept
    {
        return '\0';
    }
    template<typename T, typename... Ts>
    static forceinline constexpr bool CheckSameChar() noexcept
    {
        using Char = typename T::CharType;
        return (... && std::is_same_v<typename Ts::CharType, Char>);
    }

    template<typename Char>
    static forceinline constexpr bool CombineNamedArg(ParseResultCombineNamed& result, uint8_t& slotIdx,
        const ParseResultCommon::NamedArgType& arg, std::basic_string_view<Char> fmtString) noexcept
    {
        const auto argName = &fmtString[arg.Offset];
        for (uint8_t i = 0; i < result.NamedArgCount; ++i)
        {
            auto& target = result.NamedTypes[i];
            if (target.Length == arg.Length)
            {
                uint16_t j = 0;
                for (; j < arg.Length; ++j)
                {
                    if (result.NameBuffer[target.Offset + j] != argName[j])
                        break;
                }
                if (j == arg.Length) // name match
                {
                    const auto compType = ParseResultBase::CheckCompatible(target.Type, arg.Type);
                    IF_UNLIKELY(!compType)
                    {
                        result.ErrorNum = common::enum_cast(ParseResultBase::ErrorCode::IncompType);
                        return false;
                    }
                    target.Type = *compType;
                    slotIdx = i;
                    return true;
                }
            }
        }
        if (result.NamedArgCount < ParseResultCommon::NamedArgSlots)
        {
            if (result.BufLength + arg.Length > ParseResultCombineNamed::NameTotalSize)
            {
                result.ErrorNum = common::enum_cast(ParseResultBase::ErrorCode::ArgNameTooLong);
                return false;
            }
            result.NamedTypes[result.NamedArgCount] = { result.BufLength, arg.Length, arg.Type };
            for (uint8_t i = 0; i < arg.Length;)
            {
                result.NameBuffer[result.BufLength++] = static_cast<char>(argName[i++]);
            }
            slotIdx = result.NamedArgCount++;
            return true;
        }
        result.ErrorNum = common::enum_cast(ParseResultBase::ErrorCode::TooManyNamedArg);
        return false;
    }

    template<typename T>
    static forceinline constexpr bool CombineNamedArg(ParseResultCombineNamed& result, uint8_t* mapSlots, uint32_t& mapSlotIdx, uint32_t infoIdx, const T& strInfo) noexcept
    {
        if constexpr (T::NamedArgCount > 0)
        {
            for (uint8_t idx = 0; idx < T::NamedArgCount; ++idx)
            {
                const auto ret = CombineNamedArg<typename T::CharType>(result, mapSlots[mapSlotIdx++], strInfo.NamedTypes[idx], strInfo.FormatString);
                if (!ret)
                {
                    result.ErrorPos = static_cast<uint16_t>((infoIdx << 8) + idx);
                    return false;
                }
            }
        }
        return true;
    }

    template<typename T>
    static forceinline constexpr bool CombineIdxArg(ParseResultCommon& result, uint32_t infoIdx, const T& strInfo) noexcept
    {
        if constexpr (T::IdxArgCount > 0)
        {
            auto errorPos = infoIdx << 8;
            if constexpr (T::IdxArgCount > ParseResultCommon::IdxArgSlots)
            {
                result.SetError(errorPos, ParseResultBase::ErrorCode::TooManyIdxArg);
                return false;
            }
            result.IdxArgCount = std::max(result.IdxArgCount, T::IdxArgCount);
            for (uint8_t idx = 0; idx < T::IdxArgCount; ++idx)
            {
                const auto compType = ParseResultBase::CheckCompatible(result.IndexTypes[idx], strInfo.IndexTypes[idx]);
                IF_UNLIKELY(!compType)
                {
                    result.SetError(errorPos + idx, ParseResultBase::ErrorCode::IncompType);
                    return false;
                }
                else
                {
                    result.IndexTypes[idx] = *compType;
                }
            }
        }
        return true;
    }

    template<typename Char, typename... Ts, size_t... Indexes>
    static forceinline constexpr auto ToStrArgInfoChs(std::index_sequence<Indexes...>, const std::tuple<Ts...>& src) noexcept
    {
        std::array<StrArgInfoCh<Char>, sizeof...(Ts)> result = {};
        (..., void(result[Indexes] = std::get<Indexes>(src)));
        return result;
    }

    template<typename... Ts, size_t... Indexes>
    static forceinline constexpr auto GetNamedOffsets(std::index_sequence<Indexes...>) noexcept
    {
        constexpr uint32_t Count = sizeof...(Ts);
        std::array<uint8_t, Count * 2> result = { 0 };
        (..., void(result[Indexes] = Ts::NamedArgCount));
        for (uint32_t i = 1; i < Count; ++i)
        {
            result[i + Count] = result[i - 1 + Count] + result[i - 1];
        }
        return result;
    }

    template<typename... Ts>
    static forceinline constexpr auto DoCombine(const Ts&... args) noexcept
    {
        static_assert((... && std::is_base_of_v<CompileTimeFormatter, Ts>), "Need to be CompileTimeFormatter");
        static_assert(CheckSameChar<Ts...>(), "Need to be same Char type");
        constexpr uint32_t NamedMapCount = (... + Ts::NamedArgCount);
        ParseResultCombine<NamedMapCount> result;
        {
            uint32_t infoIdx = 0;
            (... && CombineIdxArg(result, infoIdx++, args));
        }
        if constexpr (NamedMapCount > 0)
        {
            uint32_t infoIdx = 0;
            uint32_t mapSlotIdx = 0;
            (... && CombineNamedArg(result, result.NamedMap, mapSlotIdx, infoIdx++, args));
        }
        return result;
    }

    template<typename... Ts>
    static forceinline const auto& Combine(const Ts&...) noexcept;
};


template<uint8_t NamedMapCount, uint16_t NameBufLen>
struct NamedMapLimiter
{
    using CharType = char;
    uint8_t NamedMap[NamedMapCount] = { 0 };
    char NameBuf[NameBufLen] = { '\0' };
    constexpr NamedMapLimiter(const uint8_t* mapping, const char* name) noexcept
    {
        for (uint8_t i = 0; i < NamedMapCount; ++i)
            NamedMap[i] = mapping[i];
        for (uint16_t i = 0; i < NameBufLen; ++i)
            NameBuf[i] = name[i];
    }
    constexpr std::string_view GetNameSource() const noexcept { return { NameBuf, NameBufLen }; }
};
template<>
struct NamedMapLimiter<0, 0>
{
    constexpr NamedMapLimiter(const uint8_t*, const char*) noexcept {}
};

template<uint16_t NamedMapCount_, uint16_t NameBufLen_, uint8_t NamedArgCount_, uint8_t IdxArgCount_>
struct COMMON_EMPTY_BASES TrimedCombineResult : public NamedMapLimiter<NamedMapCount_, NameBufLen_>, public NamedArgLimiter<NamedArgCount_>, public IdxArgLimiter<IdxArgCount_>
{
    static constexpr uint8_t NamedArgCount = NamedArgCount_;
    static constexpr uint8_t IdxArgCount = IdxArgCount_;
    constexpr TrimedCombineResult(const ParseResultCombine<NamedMapCount_>& result) noexcept :
        NamedMapLimiter<NamedMapCount_, NameBufLen_>(result.NamedMap, result.NameBuffer),
        NamedArgLimiter<NamedArgCount>(result.NamedTypes),
        IdxArgLimiter<IdxArgCount>(result.IndexTypes)
    { }
};

template<typename T, typename Char>
struct CombineSelection : public CustomFormatterTag
{
    using CharType = Char;
    const StrArgInfoCh<Char>& StrInfo;
    span<const uint8_t> NamedMapping;
    constexpr CombineSelection(const StrArgInfoCh<Char>& info, span<const uint8_t> namedMapping) noexcept : StrInfo(info), NamedMapping(namedMapping) {}

    template<typename... Args>
    forceinline std::pair<std::reference_wrapper<const StrArgInfoCh<Char>>, NamedMapper> TranslateInfo() const noexcept
    {
        static constexpr auto Mapping = ArgChecker::CheckSS<T, Args...>();
        auto mapping = ArgChecker::EmptyMapper;
        uint32_t idx = 0;
        for (const auto target : NamedMapping)
            mapping[idx++] = Mapping[target];
        return { StrInfo, mapping };
    }

    template<typename... Args>
    forceinline void To(std::basic_string<Char>& dst, Formatter<Char>& formatter, Args&&... args) const
    {
        static constexpr auto Mapping = ArgChecker::CheckSS<T, Args...>();
        static constexpr auto ArgsInfo = ArgInfo::ParseArgs<Args...>();
        const auto argStore = ArgInfo::PackArgsStatic(std::forward<Args>(args)...);
        auto mapping = ArgChecker::EmptyMapper;
        uint32_t idx = 0;
        for (const auto target : NamedMapping)
        {
            mapping[idx++] = Mapping[target];
        }
        FormatterBase::FormatTo<Char>(formatter, dst, StrInfo, ArgsInfo, argStore.ArgStore, mapping);
    }

    template<typename... Args>
    forceinline std::basic_string<Char> operator()(Formatter<Char>& formatter, Args&&... args) const
    {
        std::basic_string<Char> ret;
        To(ret, formatter, std::forward<Args>(args)...);
        return ret;
    }
};

template<typename T, typename Char, typename... Ts>
struct TrimedCombineHost
{
    static constexpr size_t CombineCount = sizeof...(Ts);
    static constexpr std::array<uint8_t, CombineCount * 2> NamedOffsets = FormatterCombiner::GetNamedOffsets<Ts...>(std::make_index_sequence<CombineCount>{});
    static constexpr T Combine = {};
    static constexpr std::tuple<Ts...> StrInfoTuple = {};
    static constexpr std::array<StrArgInfoCh<Char>, CombineCount> StrInfos = FormatterCombiner::ToStrArgInfoChs<Char>(std::make_index_sequence<CombineCount>{}, StrInfoTuple);

    constexpr CombineSelection<T, Char> operator()(uint32_t idx) const noexcept
    {
        Expects(idx < CombineCount);
        if constexpr (T::NamedArgCount > 0)
        {
            return { StrInfos[idx], { &Combine.NamedMap[NamedOffsets[idx + CombineCount]], NamedOffsets[idx] } };
        }
        else
        {
            return { StrInfos[idx], {} };
        }
    }
};

template<typename... Ts>
forceinline const auto& FormatterCombiner::Combine(const Ts&...) noexcept
{
    using TsTuple = std::tuple<Ts...>;
    static constexpr auto Data = std::apply(DoCombine<Ts...>, TsTuple{});
    using Char = decltype(FormatterCombiner::GetCharType2<Ts...>());
    using Base = TrimedCombineResult<Data.NamedMapCount, Data.BufLength,Data.NamedArgCount, Data.IdxArgCount>;
    struct Type : public Base
    {
        constexpr Type() noexcept : Base(Data) {}
    };
    static constexpr TrimedCombineHost<Type, Char, Ts...> Dummy;
    return Dummy;
}


}
