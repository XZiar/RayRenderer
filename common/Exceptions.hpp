#pragma once

#include "CommonRely.hpp"
#include "CommonMacro.hpp"
#include "Wrapper.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <exception>
#include <filesystem>
#include <any>


namespace common
{

class BaseException;

namespace detail
{
class OtherException;
class AnyException : public std::runtime_error, public std::enable_shared_from_this<AnyException>
{
	friend BaseException;
	friend OtherException;
private:
	explicit AnyException(const std::string& type) : std::runtime_error(type) {}
	using std::runtime_error::what;
};
class OtherException : public AnyException
{
	friend BaseException;
public:
	static constexpr auto TYPENAME = "OtherException";
	const std::exception_ptr innerException;
private:
	OtherException(const std::exception& ex) : AnyException(TYPENAME), innerException(std::make_exception_ptr(ex))
	{}
	OtherException(const std::exception_ptr& exptr) : AnyException(TYPENAME), innerException(exptr)
	{}
};
class ExceptionHelper;
}


class StackTraceItem
{
public:
	std::wstring file;
	std::wstring func;
	size_t line;
	StackTraceItem() : file(L"Undefined"), func(L"Undefined"), line(0) {}
	StackTraceItem(const wchar_t* const file_, const wchar_t* const func_, const size_t pos) : file(file_), func(func_), line(pos) {}
};
#define GENARATE_STACK_TRACE ::common::StackTraceItem(WIDEN(__FILE__), WIDEN(__FUNCSIG__), __LINE__)


class BaseException : public detail::AnyException
{
	friend detail::ExceptionHelper;
public:
	static constexpr auto TYPENAME = "BaseException";
	std::wstring message;
	std::any data;
protected:
	Wrapper<detail::AnyException> innerException;
	StackTraceItem stackitem;
	static Wrapper<detail::AnyException> __cdecl getCurrentException();
	BaseException(const char * const type, const std::wstring& msg, const std::any& data_)
		: detail::AnyException(type), message(msg), data(data_), innerException(getCurrentException())
	{ }
private:
	void exceptionstack(std::vector<StackTraceItem>& stks) const
	{
		if (innerException)
		{
			const auto bewapper = innerException.cast_dynamic<BaseException>();
			if (bewapper)
				bewapper->exceptionstack(stks);
			else
				stks.push_back(StackTraceItem(L"stdException", L"stdException", 0));
		}
		stks.push_back(stackitem);
	}
public:
	//BaseException(const BaseException& be) = default;
	BaseException(const std::wstring& msg, const std::any& data_ = std::any())
		: BaseException(TYPENAME, msg, data_)
	{ }
	virtual ~BaseException() {}
	virtual Wrapper<BaseException> clone() const
	{
		return Wrapper<BaseException>(*this);
	}
	Wrapper<BaseException> nestedException() const
	{
		if (innerException)
		{
			const auto beptr = dynamic_cast<BaseException*>(innerException.get());
			if (beptr != nullptr)
				return innerException.cast_static<BaseException>();
		}
		return Wrapper<BaseException>();
	}
	StackTraceItem stacktrace() const
	{
		return stackitem;
	}
	std::vector<StackTraceItem> exceptionstack() const
	{
		std::vector<StackTraceItem> ret;
		exceptionstack(ret);
		return ret;
	}
};
#define EXCEPTION_CLONE_EX(type) static constexpr auto TYPENAME = #type;\
	virtual ::common::Wrapper<::common::BaseException> clone() const override\
	{ return ::common::Wrapper<type>(*this).cast_static<::common::BaseException>(); }

inline Wrapper<detail::AnyException> __cdecl BaseException::getCurrentException()
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
	static T __cdecl setStackItem(T ex, StackTraceItem sti)
	{
		static_assert(std::is_base_of_v<BaseException, T>, "COMMON_THROW can only be used on Exception derivered from BaseException");
		BaseException *bex = static_cast<BaseException*>(&ex);
		bex->stackitem = sti;
		return ex;
	}
};
}
#define COMMON_THROW(ex, ...) throw ::common::detail::ExceptionHelper::setStackItem(ex(__VA_ARGS__), GENARATE_STACK_TRACE)


namespace fs = std::experimental::filesystem;

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
		: BaseException(TYPENAME, msg, data_), reason(why), filepath(file)
	{ }
	virtual ~FileException() {}
};


}

