#pragma once

#include "SystemCommonRely.h"
#include "FormatInclude.h"
#include "common/CommonRely.hpp"
#include "common/SharedString.hpp"
#include "common/ResourceDict.hpp"
#include "common/EasyIterator.hpp"
#include "common/AlignedBuffer.hpp"
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
    StackTraceItem() noexcept : Line(0) {}
    StackTraceItem(const char16_t* const file, const char16_t* const func, const size_t pos) noexcept
        : File(file), Func(func), Line(pos) {}
    StackTraceItem(const char16_t* const file, const char* const func, const size_t pos) noexcept
        : File(file), Func(std::u16string(func, func + std::char_traits<char>::length(func))), Line(pos) {}
    template<typename T1, typename T2>
    StackTraceItem(T1&& file, T2&& func, const size_t pos) noexcept
        : File(std::forward<T1>(file)), Func(std::forward<T2>(func)), Line(pos) {}
};

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif


class SYSCOMMONAPI ExceptionBasicInfo : public std::enable_shared_from_this<ExceptionBasicInfo>
{
    friend class BaseException;
private:
    static constexpr auto TYPENAME = "BaseException";
    [[nodiscard]] virtual StackTraceItem GetStack(size_t idx) const noexcept;
    [[nodiscard]] virtual size_t GetStackCount() const noexcept;
    COMMON_EASY_CONST_ITER(StackList, ExceptionBasicInfo, StackTraceItem, GetStack, 0, Host->GetStackCount());
    AlignedBuffer DelayedStacks;
protected:
    template<typename T>
    ExceptionBasicInfo(const char* type, T&& msg) noexcept;
    std::vector<StackTraceItem> StackTrace;
public:
    const char* TypeName;
    std::u16string Message;
    std::shared_ptr<ExceptionBasicInfo> InnerException;
    container::ResourceDict Resources;
    ExceptionBasicInfo(const std::u16string_view msg) noexcept : ExceptionBasicInfo(TYPENAME, msg) { }
    ExceptionBasicInfo(ExceptionBasicInfo&&) noexcept = delete;
    ExceptionBasicInfo(const ExceptionBasicInfo&) noexcept = delete;
    ExceptionBasicInfo& operator=(ExceptionBasicInfo&&) noexcept = delete;
    ExceptionBasicInfo& operator=(const ExceptionBasicInfo&) noexcept = delete;
    virtual ~ExceptionBasicInfo();
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
    constexpr StackList GetStacks() const noexcept { return this; }
};


class SYSCOMMONAPI [[nodiscard]] BaseException : public std::runtime_error
{
    friend ExceptionBasicInfo;
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
    ~BaseException() override;
    const char* what() const noexcept override;
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
    [[nodiscard]] std::vector<StackTraceItem> GetAllStacks() const noexcept;
    [[nodiscard]] std::vector<StackTraceItem> ExceptionStacks() const;
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
    [[nodiscard]] std::u16string_view Message() const noexcept
    {
        return Info->Message;
    }
    [[nodiscard]] std::u16string_view GetDetailMessage() const noexcept;
    void FormatWith(::common::str::FormatterHost&, ::common::str::FormatterContext&, const ::common::str::FormatSpec*) const noexcept;

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
    template<typename T, typename... Args>
    [[nodiscard]] static T CreateWithDelayedStacks(AlignedBuffer&& stacks, Args... args)
    {
        static_assert(std::is_base_of_v<BaseException, T>, "COMMON_THROW can only be used on Exception derivered from BaseException");
        T ex(std::forward<Args>(args)...);
        static_cast<BaseException*>(&ex)->Info->DelayedStacks = std::move(stacks);
        return ex;
    }
    [[nodiscard]] static std::shared_ptr<ExceptionBasicInfo> GetCurrentException() noexcept;
};

template<typename T>
inline ExceptionBasicInfo::ExceptionBasicInfo(const char* type, T&& msg) noexcept
    : TypeName(type), Message(std::forward<T>(msg)), InnerException(BaseException::GetCurrentException()) { }

#define COMMON_EXCEPTION_PREPARE(type, parent, ...)                                 \
    using TPInfo = parent::TInfo;                                                   \
    struct ExceptionInfo : public TPInfo                                            \
    {                                                                               \
        static constexpr auto TYPENAME = #type;                                     \
        ~ExceptionInfo() override;                                                  \
        [[noreturn]] void ThrowReal() override;                                     \
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
    ~type() override


#define COMMON_EXCEPTION_IMPL(type)                     \
type::ExceptionInfo::~ExceptionInfo() {}                \
type::~type() {}                                        \
[[noreturn]] void type::ExceptionInfo::ThrowReal()      \
{                                                       \
    throw type(std::nullopt, this->shared_from_this()); \
}


class SYSCOMMONAPI [[nodiscard]] OtherException : public BaseException
{
    COMMON_EXCEPTION_PREPARE(OtherException, BaseException, 
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
    const char* what() const noexcept override;
};

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


#if defined(_MSC_VER)
#   define CREATE_EXCEPTION(ex, ...) ::common::BaseException::CreateWithStack<ex>({ UTF16ER(__FILE__), u"" __FUNCSIG__, (size_t)(__LINE__) }, __VA_ARGS__)
#else
#   define CREATE_EXCEPTION(ex, ...) ::common::BaseException::CreateWithStack<ex>({ UTF16ER(__FILE__), __PRETTY_FUNCTION__, (size_t)(__LINE__) }, __VA_ARGS__)
#endif
#define COMMON_THROW(ex, ...) throw CREATE_EXCEPTION(ex, __VA_ARGS__)



}

