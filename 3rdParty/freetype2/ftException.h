#pragma once
#include "common/Exceptions.hpp"

namespace ft
{

class FTException : public common::BaseException
{
public:
	EXCEPTION_CLONE_EX(FTException);
	FTException(const std::u16string_view& msg) : BaseException(TYPENAME, msg)
	{ }
	virtual ~FTException() {}
};


}