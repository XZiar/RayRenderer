#pragma once
#include "../../common/Exceptions.hpp"

namespace ft
{

class FTException : public common::BaseException
{
public:
	EXCEPTION_CLONE_EX(FTException);
	FTException(const std::wstring& msg, const std::any& data_ = std::any())
		: BaseException(TYPENAME, msg, data)
	{ }
	virtual ~FTException() {}
};


}