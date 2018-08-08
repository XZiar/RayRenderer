#pragma once
#include "oclRely.h"
#include "oclUtil.h"

namespace oclu
{


class OCLException : public common::BaseException
{
public:
	EXCEPTION_CLONE_EX(OCLException);
	enum class CLComponent { Compiler, Driver, Accellarator, OCLU };
	const CLComponent exceptionSource;
	OCLException(const CLComponent source, const std::wstring& msg, const std::any& data_ = std::any())
		: BaseException(TYPENAME, msg, data_), exceptionSource(source)
	{ }
	virtual ~OCLException() {}
};




}