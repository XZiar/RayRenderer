#pragma once

#include "CommonRely.hpp"
#include "SharedString.hpp"
#include "ResourceDict.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <stdexcept>
#include <exception>
#include <optional>
#include <memory>


namespace common
{
class ExceptionBasicInfo;
class BaseException;


class StackTraceItem
{
public:
    SharedString<char16_t> File;
    SharedString<char16_t> Func;
    size_t Line;
    StackTraceItem(const char16_t* const file, const char16_t* const func, const size_t pos)
        : File(file), Func(func), Line(pos) {}
    StackTraceItem(const char16_t* const file, const char* const func, const size_t pos)
        : File(file), Func(std::u16string(func, func + std::char_traits<char>::length(func))), Line(pos) {}
    template<typename T1, typename T2>
    StackTraceItem(T1&& file, T2&& func, const size_t pos)
        : File(std::forward<T1>(file)), Func(std::forward<T2>(func)), Line(pos) {}
};

namespace detail
{

struct ExceptionHelper
{
public:
    [[nodiscard]] static std::shared_ptr<ExceptionBasicInfo> GetCurrentException() noexcept;
};

}


class COMMON_EMPTY_BASES ExceptionBasicInfo : public NonCopyable, public std::enable_shared_from_this<ExceptionBasicInfo>
{
    friend class BaseException;
private:
    static constexpr auto TYPENAME = "BaseException";
protected:
    template<typename T>
    ExceptionBasicInfo(const char* type, T&& msg) noexcept
        : TypeName(type), Message(std::forward<T>(msg)), InnerException(detail::ExceptionHelper::GetCurrentException()) { }
public:
    const char* TypeName;
    std::u16string Message;
    std::vector<StackTraceItem> StackTrace;
    std::shared_ptr<ExceptionBasicInfo> InnerException;
    container::ResourceDict Resources;
    ExceptionBasicInfo(const std::u16string_view msg) noexcept : ExceptionBasicInfo(TYPENAME, msg) { }
    virtual ~ExceptionBasicInfo() {}
    [[noreturn]] virtual void ThrowReal();
    [[nodiscard]] BaseException GetException();
    template<typename T>
    auto Cast() const noexcept
    {
        using TInfo = typename T::TInfo;
        return dynamic_cast<const TInfo*>(this);
    }
    template<typename T>
    auto Cast() noexcept
    {
        using TInfo = typename T::TInfo;
        return dynamic_cast<TInfo*>(this);
    }
};


class [[nodiscard]] BaseException : public std::runtime_error
{
    friend class ExceptionBasicInfo;
    friend struct detail::ExceptionHelper;
public:
    static constexpr auto TYPENAME = ExceptionBasicInfo::TYPENAME;
protected:
    template<typename T> struct T_ {};
    using TInfo = ExceptionBasicInfo;
    std::shared_ptr<ExceptionBasicInfo> Info;
    BaseException(std::nullopt_t, std::shared_ptr<ExceptionBasicInfo> info) noexcept 
        : std::runtime_error(info->TypeName), Info(std::move(info)) 
    { }
    template<typename T, typename... Args>
    BaseException(T_<T>, Args&&... args)
        : BaseException(std::nullopt, std::make_shared<T>(std::forward<Args>(args)...))
    { }
public:
    BaseException(const std::u16string_view msg) : BaseException(T_<TInfo>{}, msg)
    { }
    BaseException(const BaseException& baseEx) = default;
    BaseException(BaseException&& baseEx) noexcept = default;
    ~BaseException() override {}
    const char* what() const noexcept override { return Info->TypeName; }
    const std::shared_ptr<ExceptionBasicInfo>& InnerInfo() const noexcept
    {
        return Info;
    }
    [[noreturn]] void ThrowSelf() const
    {
        Info->ThrowReal();
    }
    [[nodiscard]] std::optional<BaseException> NestedException() const 
    { 
        if (Info->InnerException)
            return Info->GetException();
        return {};
    }
    [[nodiscard]] const std::vector<StackTraceItem>& Stack() const noexcept
    {
        return Info->StackTrace;
    }
    [[nodiscard]] std::u16string_view Message() const noexcept
    {
        return Info->Message;
    }
    [[nodiscard]] std::vector<StackTraceItem> ExceptionStacks() const
    {
        std::vector<StackTraceItem> ret;
        for (auto ex = Info.get(); ex; ex = ex->InnerException.get())
        {
            if (ex->StackTrace.size() > 0)
                ret.push_back(ex->StackTrace[0]);
            else
                ret.emplace_back(u"Undefined", u"Undefined", 0);
        }
        return ret;
    }
    template<typename T, typename S, typename U = T>
    const BaseException& Attach(S&& key, T&& value) const
    {
        Info->Resources.Add<U>(std::forward<S>(key), std::forward<T>(value));
        return *this;
    }
    template<typename T>
    [[nodiscard]] const T* GetResource(std::string_view key) const noexcept
    {
        return Info->Resources.QueryItem<T>(key);
    }
    [[nodiscard]] std::u16string_view GetDetailMessage() const noexcept
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
    template<typename T, typename... Args>
    [[nodiscard]] static T CreateWithStack(StackTraceItem&& sti, Args... args)
    {
        static_assert(std::is_base_of_v<BaseException, T>, "COMMON_THROW can only be used on Exception derivered from BaseException");
        T ex(std::forward<Args>(args)...);
        static_cast<BaseException*>(&ex)->Info->StackTrace.push_back(std::move(sti));
        return ex;
    }
    template<typename T, typename... Args>
    [[nodiscard]] static T CreateWithStacks(std::vector<StackTraceItem>&& stacks, Args... args)
    {
        static_assert(std::is_base_of_v<BaseException, T>, "COMMON_THROW can only be used on Exception derivered from BaseException");
        T ex(std::forward<Args>(args)...);
        static_cast<BaseException*>(&ex)->Info->StackTrace = std::move(stacks);
        return ex;
    }
};

[[noreturn]] inline void ExceptionBasicInfo::ThrowReal()
{
    throw BaseException(std::nullopt, this->shared_from_this());
}
inline BaseException ExceptionBasicInfo::GetException()
{
    return BaseException{ std::nullopt, this->shared_from_this() };
}


#define PREPARE_EXCEPTION(type, parent, ...)                                        \
    using TPInfo = parent::TInfo;                                                   \
    struct ExceptionInfo : public TPInfo                                            \
    {                                                                               \
        static constexpr auto TYPENAME = #type;                                     \
        ~ExceptionInfo() override { }                                               \
        [[noreturn]] void ThrowReal() override                                      \
        {                                                                           \
            throw type(std::nullopt, this->shared_from_this());                     \
        }                                                                           \
        __VA_ARGS__                                                                 \
    };                                                                              \
    ExceptionInfo& GetInfo() const noexcept                                         \
    {                                                                               \
        return *static_cast<ExceptionInfo*>(Info.get());                            \
    }                                                                               \
protected:                                                                          \
    type(std::nullopt_t, std::shared_ptr<common::ExceptionBasicInfo> info) noexcept \
        : parent(std::nullopt, std::move(info))                                     \
    { }                                                                             \
    template<typename T, typename... Args>                                          \
    type(T_<T>, Args&&... args)                                                     \
        : parent(T_<T>{}, std::forward<Args>(args)...)                              \
    { }                                                                             \
public:                                                                             \
    static constexpr auto TYPENAME = ExceptionInfo::TYPENAME;                       \
    using TInfo = ExceptionInfo;                                                    \
    type(const type& ex) = default;                                                 \
    type(type&& ex) noexcept = default;                                             \
    ~type() override {}                                                             \


class [[nodiscard]] OtherException : public BaseException
{
    PREPARE_EXCEPTION(OtherException, BaseException, 
        std::exception_ptr StdException;
        ExceptionInfo(const std::u16string_view msg, const std::exception_ptr& exptr)
            : TPInfo(TYPENAME, msg), StdException(exptr)
        { }
        ExceptionInfo(const std::u16string_view msg, const std::exception& ex)
            : ExceptionInfo(msg, std::make_exception_ptr(ex))
        { }
        const char* GetInnerMessage() const noexcept
        {
            if (!StdException)
                return "EMPTY";
            try
            {
                std::rethrow_exception(StdException);
            }
            catch (const std::runtime_error& re)
            {
                return re.what();
            }
            catch (...)
            {
                return "UNKNWON";
            }
        }
    );
    OtherException(const std::exception_ptr& exptr) : BaseException(T_<ExceptionInfo>{}, u"std_exception", exptr) {}
    const char* what() const noexcept override 
    { 
        return GetInfo().GetInnerMessage();
    }
};


[[nodiscard]] inline std::shared_ptr<ExceptionBasicInfo> detail::ExceptionHelper::GetCurrentException() noexcept
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

#if defined(_MSC_VER)
#   define CREATE_EXCEPTION(ex, ...) ::common::BaseException::CreateWithStack<ex>({ UTF16ER(__FILE__), u"" __FUNCSIG__, (size_t)(__LINE__) }, __VA_ARGS__)
#else
#   define CREATE_EXCEPTION(ex, ...) ::common::BaseException::CreateWithStack<ex>({ UTF16ER(__FILE__), __PRETTY_FUNCTION__, (size_t)(__LINE__) }, __VA_ARGS__)
#endif
#define COMMON_THROW(ex, ...) throw CREATE_EXCEPTION(ex, __VA_ARGS__)


}

