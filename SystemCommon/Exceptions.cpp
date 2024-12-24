#include "Exceptions.h"
#include <variant>

namespace common
{

ExceptionBasicInfo::~ExceptionBasicInfo() {}
void ExceptionBasicInfo::ThrowReal()
{
    throw BaseException(std::nullopt, this->shared_from_this());
}
BaseException ExceptionBasicInfo::GetException()
{
    return BaseException{ std::nullopt, this->shared_from_this() };
}

BaseException::~BaseException() {}
const char* BaseException::what() const noexcept { return Info->TypeName; }

std::vector<StackTraceItem> BaseException::GetAllStacks() const noexcept
{
    const auto cnt = Info->GetStackCount();
    if (Info->StackTrace.size() == cnt)
        return Info->StackTrace;
    std::vector<StackTraceItem> ret;
    ret.reserve(cnt);
    for (size_t i = 0; i < cnt; ++i)
        ret.emplace_back(Info->GetStack(i));
    return ret;
}

[[nodiscard]] std::vector<StackTraceItem> BaseException::ExceptionStacks() const
{
    static SharedString<char16_t> Undef = u"Undefined";
    std::vector<StackTraceItem> ret;
    for (auto ex = Info.get(); ex; ex = ex->InnerException.get())
    {
        if (ex->GetStackCount() > 0)
            ret.push_back(ex->GetStack(0));
        else
            ret.emplace_back(Undef, Undef, 0);
    }
    return ret;
}

[[nodiscard]] std::u16string_view BaseException::GetDetailMessage() const noexcept
{
    const auto ptr = Info->Resources.QueryItem("detail");
    if (ptr)
    {
        if (ptr->type() == typeid(common::SharedString<char16_t>))
            return *std::any_cast<common::SharedString<char16_t>>(ptr);
        if (ptr->type() == typeid(std::u16string))
            return *std::any_cast<std::u16string>(ptr);
        if (ptr->type() == typeid(std::u16string_view))
            return *std::any_cast<std::u16string_view>(ptr);
    }
    return {};
}

void BaseException::FormatWith(::common::str::FormatterExecutor& executor, ::common::str::FormatterExecutor::Context& context, const ::common::str::FormatSpec*) const noexcept
{
    executor.PutString(context, Message(), nullptr);
    const auto ptr = Info->Resources.QueryItem("detail");
    if (ptr)
    {
        std::variant<std::monostate, std::u16string_view, std::string_view, std::u32string_view, std::wstring_view> detail;
        const auto& tid = ptr->type();
#define STRMatch(ch)                                                    \
        if (tid == typeid(common::SharedString<ch>))                    \
            detail = *std::any_cast<common::SharedString<ch>>(ptr);     \
        else if (tid == typeid(std::basic_string<ch>))                  \
            detail = *std::any_cast<std::basic_string<ch>>(ptr);        \
        else if (tid == typeid(std::basic_string_view<ch>))             \
            detail = *std::any_cast<std::basic_string_view<ch>>(ptr)
        STRMatch(char16_t);
        else STRMatch(char);
        else STRMatch(char32_t);
        else STRMatch(wchar_t);
#undef STRMatch
        if (detail.index())
        {
            executor.PutString(context, ", ", nullptr);
            std::visit([&](const auto& txt) 
            {
                if constexpr (!std::is_same_v<std::decay_t<decltype(txt)>, std::monostate>)
                    executor.PutString(context, txt, nullptr);
            }, detail);
        }
    }
}

std::shared_ptr<ExceptionBasicInfo> BaseException::GetCurrentException() noexcept
{
    const auto cex = std::current_exception();
    if (!cex)
        return {};
    try
    {
        std::rethrow_exception(cex);
    }
    catch (const BaseException& be)
    {
        return be.Info;
    }
    catch (...)
    {
        return OtherException(cex).Info;
    }
}


COMMON_EXCEPTION_IMPL(OtherException)
const char* OtherException::what() const noexcept
{
    return GetInfo().GetInnerMessage();
}

}
