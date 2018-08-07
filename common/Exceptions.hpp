#pragma once

#include "CommonRely.hpp"
#include "Wrapper.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <exception>
#include <any>
#if defined(__GNUC__)
#   include <experimental/filesystem>
#else
#   include <filesystem>
#endif

namespace common
{

class BaseException;

namespace detail
{
class OtherException;
class AnyException : public std::runtime_error, public std::enable_shared_from_this<AnyException>
{
protected:
    const char * const TypeName;
    explicit AnyException(const char* const type) : std::runtime_error(type), TypeName(type) {}
    using std::runtime_error::what;
};
class OtherException : public AnyException
{
    friend BaseException;
public:
    static constexpr auto TYPENAME = "OtherException";
    const char* What() const
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
    const std::exception_ptr StdException;
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
    std::u16string_view File;
#if defined(_MSC_VER)
    std::u16string_view Func;
#else
    std::u16string Func;
#endif
    size_t Line;
    StackTraceItem() : File(u"Undefined"), Func(u"Undefined"), Line(0) {}
#if defined(_MSC_VER)
    StackTraceItem(const char16_t* const file, const char16_t* const func, const size_t pos) : File(file), Func(func), Line(pos) {}
#else
    StackTraceItem(const char16_t* const file, const char16_t* const func, const size_t pos) : File(file), Func(func), Line(pos) {}
#endif
};


class BaseException : public detail::AnyException
{
    friend detail::ExceptionHelper;
public:
    static constexpr auto TYPENAME = "BaseException";
    std::wstring message;
    std::any data;
protected:
    Wrapper<detail::AnyException> InnerException;
    StackTraceItem StackItem;
    static Wrapper<detail::AnyException> getCurrentException();
    BaseException(const char * const type, const std::wstring& msg, const std::any& data_)
        : detail::AnyException(type), message(msg), data(data_), InnerException(getCurrentException())
    { }
private:
    void CollectStack(std::vector<StackTraceItem>& stks) const
    {
        if (InnerException)
        {
            const auto bewapper = InnerException.cast_dynamic<BaseException>();
            if (bewapper)
                bewapper->CollectStack(stks);
            else
                stks.push_back(StackTraceItem(u"StdException", u"StdException", 0));
        }
        stks.push_back(StackItem);
    }
public:
    BaseException(const std::wstring& msg, const std::any& data_ = std::any())
        : BaseException(TYPENAME, msg, data_)
    { }
    ~BaseException() override {}
    virtual Wrapper<BaseException> clone() const
    {
        return Wrapper<BaseException>(*this);
    }
    Wrapper<detail::AnyException> NestedException() const { return InnerException; }
    StackTraceItem Stacktrace() const
    {
        return StackItem;
    }
    std::vector<StackTraceItem> ExceptionStack() const
    {
        std::vector<StackTraceItem> ret;
        CollectStack(ret);
        return ret;
    }
};
#define EXCEPTION_CLONE_EX(type) static constexpr auto TYPENAME = #type;\
    virtual ::common::Wrapper<::common::BaseException> clone() const override\
    { return ::common::Wrapper<type>(*this).cast_static<::common::BaseException>(); }


inline Wrapper<detail::AnyException> BaseException::getCurrentException()
{
    const auto cex = std::current_exception();
    if (!cex)
        return Wrapper<detail::AnyException>();
    try
    {
        std::rethrow_exception(cex);
    }
    catch (const BaseException& be)
    {
        return be.clone().cast_static<detail::AnyException>();
    }
    catch (...)
    {
        return Wrapper<detail::OtherException>(new detail::OtherException(cex)).cast_static<detail::AnyException>();
    }
}


namespace detail
{
class ExceptionHelper
{
public:
    template<class T>
    static T SetStackItem(T ex, StackTraceItem sti)
    {
        static_assert(std::is_base_of_v<BaseException, T>, "COMMON_THROW can only be used on Exception derivered from BaseException");
        static_cast<BaseException*>(&ex)->StackItem = sti;
        return ex;
    }
};
}
#if defined(_MSC_VER)
#   define COMMON_THROW(ex, ...) throw ::common::detail::ExceptionHelper::SetStackItem(ex(__VA_ARGS__), { u"" __FILE__, u"" __FUNCSIG__, (size_t)(__LINE__) })
#else
template<size_t N>
inline constexpr auto MakeGccToU16Str(const char (&str)[N])
{
    std::array<char16_t, N> ret{ 0 };
    for (size_t i = 0; i < N; ++i)
        ret[i] = str[i];
    return ret;
}
#   define COMMON_THROW(ex, ...) throw ::common::detail::ExceptionHelper::SetStackItem(ex(__VA_ARGS__), { u"" __FILE__, ::common::MakeGccToU16Str(__PRETTY_FUNCTION__).data(), (size_t)(__LINE__) })
#endif

#if defined(__GNUC__)
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

class FileException : public BaseException
{
public:
    enum class Reason { NotExist, WrongFormat, OpenFail, ReadFail, WriteFail, CloseFail };
public:
    fs::path filepath;
public:
    EXCEPTION_CLONE_EX(FileException);
    const Reason reason;
    FileException(const Reason why, const fs::path& file, const std::wstring& msg, const std::any& data_ = std::any())
        : BaseException(TYPENAME, msg, data_), filepath(file), reason(why)
    { }
    ~FileException() override {}
};


}

