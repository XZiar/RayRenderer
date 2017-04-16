#pragma once

#include "CommonRely.hpp"
#include "Wrapper.hpp"
#include "StringEx.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <exception>
#include <filesystem>
#include <any>

#pragma warning(disable:4101)
namespace common
{

class BaseException;

namespace detail
{
class OtherException;
class AnyException : protected std::runtime_error, public std::enable_shared_from_this<AnyException>
{
	friend BaseException;
	friend OtherException;
private:
	explicit AnyException(const std::string& type) : std::runtime_error(type) {}
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
	StackTraceItem(const char* file_, const char* func_, const size_t pos) : file(to_wstring(file_)), func(to_wstring(func_)), line(pos) {}
};
#define GENARATE_STACK_TRACE ::common::StackTraceItem(__FILE__, __func__, __LINE__)


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
			const auto beptr = dynamic_cast<BaseException*>(&*innerException);
			if(beptr == nullptr)
				stks.push_back(StackTraceItem("stdException", "stdException", 0));
			else
				beptr->exceptionstack(stks);
		}
		stks.push_back(stackitem);
	}
public:
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
			const auto beptr = dynamic_cast<BaseException*>(&*innerException);
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
		return be.clone();
	}
	catch (...)
	{
		return Wrapper<detail::OtherException>(new detail::OtherException(cex));
	}
}


namespace detail
{
class ExceptionHelper
{
public:
	template<class T, class = typename std::enable_if<std::is_base_of<BaseException, T>::value>::type>
	static T __cdecl setStackItem(T ex, StackTraceItem sti)
	{
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
private:
	fs::path filepath;
public:
	static constexpr auto TYPENAME = "FileException";
	const Reason reason;
	FileException(Reason why, const fs::path& file, const std::wstring& msg, const std::any& data_ = std::any())
		: BaseException(TYPENAME, msg, data_), reason(why), filepath(file)
	{ }
	virtual ~FileException() {}
	virtual Wrapper<BaseException> clone() const override
	{
		return Wrapper<FileException>(*this);
	}
};


}
#pragma warning(default:4101)
