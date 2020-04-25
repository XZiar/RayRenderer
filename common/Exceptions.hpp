#pragma once

#include "CommonRely.hpp"
#include "SharedString.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <exception>
#include <memory>
#include <any>


namespace common
{

class BaseException;

namespace detail
{

class AnyException : public std::runtime_error, public std::enable_shared_from_this<AnyException>
{
protected:
    const char* TypeName;
    explicit AnyException(const char* const type) : std::runtime_error(type), TypeName(type) { }
    ~AnyException() override { }
    using std::runtime_error::what;
};

class OtherException : public AnyException
{
    friend BaseException;
public:
    static constexpr auto TYPENAME = "OtherException";
    [[nodiscard]] const char* What() const
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
    }
    ~OtherException() override { }
private:
    std::exception_ptr StdException;
    OtherException(const std::exception& ex) : AnyException(TYPENAME), StdException(std::make_exception_ptr(ex))
    {}
    OtherException(const std::exception_ptr& exptr) : AnyException(TYPENAME), StdException(exptr)
    {}
};

class ExceptionHelper;

}


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


class [[nodiscard]] BaseException : public detail::AnyException
{
    friend detail::ExceptionHelper;
public:
    static constexpr auto TYPENAME = "BaseException";
    SharedString<char16_t> message;
    std::any data;
protected:
    std::shared_ptr<detail::AnyException> InnerException;
    std::vector<StackTraceItem> StackTrace;
    static std::shared_ptr<detail::AnyException> GetCurrentException();
    BaseException(const char* const type, const std::u16string_view& msg, const std::any& data_)
        : detail::AnyException(type), message(msg), data(data_), InnerException(GetCurrentException())
    { }
private:
    void CollectStack(std::vector<StackTraceItem>& stks) const
    {
        if (InnerException)
        {
            const auto baseEx = std::dynamic_pointer_cast<BaseException>(InnerException);
            if (baseEx)
                baseEx->CollectStack(stks);
            else // is std exception
                stks.emplace_back(u"StdException", u"StdException", 0);
        }
        if (StackTrace.size() > 0)
            stks.push_back(StackTrace[0]);
        else
            stks.emplace_back(u"Undefined", u"Undefined", 0);
    }
public:
    BaseException(const std::u16string_view& msg, const std::any& data_ = std::any())
        : BaseException(TYPENAME, msg, data_)
    { }
    BaseException(const BaseException& baseEx) = default;
    BaseException(BaseException&& baseEx) = default;
    ~BaseException() override {}
    [[nodiscard]] virtual std::shared_ptr<BaseException> Share() const
    {
        return std::make_shared<BaseException>(*this);
    }
    [[noreturn]]  virtual void ThrowSelf() const
    {
        throw *this;
    }
    [[nodiscard]] std::shared_ptr<detail::AnyException> NestedException() const { return InnerException; }
    [[nodiscard]] constexpr const std::vector<StackTraceItem>& Stack() const
    {
        return StackTrace;
    }
    [[nodiscard]] std::vector<StackTraceItem> ExceptionStacks() const
    {
        std::vector<StackTraceItem> ret;
        CollectStack(ret);
        return ret;
    }
    template<typename T, typename... Args>
    [[nodiscard]] static T CreateWithStack(StackTraceItem&& sti, Args... args)
    {
        static_assert(std::is_base_of_v<BaseException, T>, "COMMON_THROW can only be used on Exception derivered from BaseException");
        T ex(std::forward<Args>(args)...);
        static_cast<BaseException*>(&ex)->StackTrace.push_back(std::move(sti));
        return ex;
    }
    template<typename T, typename... Args>
    [[nodiscard]] static T CreateWithStacks(std::vector<StackTraceItem>&& stacks, Args... args)
    {
        static_assert(std::is_base_of_v<BaseException, T>, "COMMON_THROW can only be used on Exception derivered from BaseException");
        T ex(std::forward<Args>(args)...);
        static_cast<BaseException*>(&ex)->StackTrace = std::move(stacks);
        return ex;
    }
};
#define EXCEPTION_CLONE_EX(type)                                                    \
    static constexpr auto TYPENAME = #type;                                         \
    type(const type& ex) = default;                                                 \
    [[nodiscard]] ::std::shared_ptr<::common::BaseException> Share() const override \
    {                                                                               \
        return ::std::static_pointer_cast<::common::BaseException>                  \
            (::std::make_shared<type>(*this));                                      \
    }                                                                               \
    [[noreturn]]  virtual void ThrowSelf() const override                           \
    {                                                                               \
        throw *this;                                                                \
    }                                                                               \


[[nodiscard]] inline std::shared_ptr<detail::AnyException> BaseException::GetCurrentException()
{
    const auto cex = std::current_exception();
    if (!cex)
        return std::shared_ptr<detail::AnyException>{};
    try
    {
        std::rethrow_exception(cex);
    }
    catch (const BaseException& be)
    {
        return std::static_pointer_cast<detail::AnyException>(be.Share());
    }
    catch (const std::shared_ptr<BaseException>& sbe)
    {
        return std::static_pointer_cast<detail::AnyException>(sbe);
    }
    catch (...)
    {
        return std::shared_ptr<detail::AnyException>(new detail::OtherException(cex));
    }
}


#if defined(_MSC_VER)
#   define COMMON_THROW(ex, ...) throw ::common::BaseException::CreateWithStack<ex>({ UTF16ER(__FILE__), u"" __FUNCSIG__, (size_t)(__LINE__) }, __VA_ARGS__)
#   define CREATE_EXCEPTION(ex, ...) ::common::BaseException::CreateWithStack<ex>({ UTF16ER(__FILE__), u"" __FUNCSIG__, (size_t)(__LINE__) }, __VA_ARGS__)
#else
#   define COMMON_THROW(ex, ...) throw ::common::BaseException::CreateWithStack<ex>({ UTF16ER(__FILE__), __PRETTY_FUNCTION__, (size_t)(__LINE__) }, __VA_ARGS__)
#   define CREATE_EXCEPTION(ex, ...) ::common::BaseException::CreateWithStack<ex>({ UTF16ER(__FILE__), __PRETTY_FUNCTION__, (size_t)(__LINE__) }, __VA_ARGS__)
#endif

}

