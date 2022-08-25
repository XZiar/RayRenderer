#include "Format.h"
#include "Exceptions.h"
#include "common/STLEx.hpp"


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


struct FormatSpecCacher
{
    std::vector<OpaqueFormatSpec> IndexSpec;
    std::vector<OpaqueFormatSpec> NamedSpec;
    SmallBitset CachedBitMap;
    SYSCOMMONAPI bool Cache(const StrArgInfo& strInfo, const ArgInfo& argInfo, const ArgPack::NamedMapper& mapper) noexcept;
};

}
