#pragma once

#include "CommonBase.hpp"
#include <string>
#include <exception>
#include <filesystem>
#include <any>

namespace common
{


namespace detail
{


class AnyException : protected std::runtime_error, public std::enable_shared_from_this<AnyException>
{
protected:
	AnyException(const std::string& type) : std::runtime_error(type) {}
};

class NoException {};

}


class BaseException : public detail::AnyException
{
public:
	const Wrapper<detail::AnyException> innerExcption;
	std::wstring message;
	std::any data;
protected:
	template<class T, class = typename std::enable_if<std::is_base_of<detail::AnyException, T>::value>::type>
	BaseException(const char * const type, const std::wstring& msg, const std::any data_, const T& e)
		: detail::AnyException(type), message(msg), data(data_), innerExcption(std::make_shared<T>(e))
	{ }
	BaseException(const char * const type, const std::wstring& msg, const std::any data_)
		: detail::AnyException(type), message(msg), data(data_)
	{ }
public:
	static constexpr auto TYPENAME = "BaseException";
	template<class T, class = typename std::enable_if<std::is_base_of<detail::AnyException, T>::value>::type>
	BaseException(const T& e, const std::wstring& msg, const std::any data_ = std::any())
		: BaseException(TYPENAME, msg, data_, e)
	{ }
	BaseException(const std::wstring& msg, const std::any data_ = std::any())
		: BaseException(TYPENAME, msg, data_)
	{ }
	virtual ~BaseException() {}
};

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
	template<class T, class = typename std::enable_if<std::is_base_of<BaseException, T>::value>::type>
	FileException(const T& e, Reason why, const fs::path& file, const std::wstring& msg, const std::any data_ = std::any())
		: BaseException(TYPENAME, msg, data_, e), reason(why), filepath(file)
	{ }
	FileException(Reason why, const fs::path& file, const std::wstring& msg, const std::any data_ = std::any())
		: BaseException(TYPENAME, msg, data_), reason(why), filepath(file)
	{ }
	virtual ~FileException() {}
};


}