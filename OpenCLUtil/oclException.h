#pragma once
#include "oclRely.h"
#include "oclUtil.h"

namespace oclu
{


class OCLException : public common::BaseException
{
public:
	EXCEPTION_CLONE_EX(OCLException);
	enum class CLComponent { Compiler, Driver, Accellarator };
	const CLComponent exceptionSource;
	OCLException(const CLComponent source, const std::wstring& msg, const std::any& data_ = std::any())
		: BaseException(TYPENAME, msg, data_), exceptionSource(source)
	{ }
	virtual ~OCLException() {}
};

inline wstring errString(const wchar_t *prefix, cl_int errcode)
{
	return wstring(prefix) + L" --ERROR: " + oclUtil::getErrorString(errcode);
}


}