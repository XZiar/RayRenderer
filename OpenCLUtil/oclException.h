#pragma once
#include "oclRely.h"

namespace oclu
{


class OCLException : public BaseException
{
public:
	EXCEPTION_CLONE_EX(OCLException);
	enum class CLComponent { Compiler, Driver, Accellarator };
	const CLComponent exceptionSource;
	OCLException(const CLComponent source, const std::wstring& msg, const std::any& data_ = std::any())
		: BaseException(TYPENAME, msg, data), exceptionSource(source)
	{ }
	virtual ~OCLException() {}
};



}