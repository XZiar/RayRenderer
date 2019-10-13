#pragma once
#include "oglRely.h"

namespace oglu
{


class OGLException : public common::BaseException
{
public:
	EXCEPTION_CLONE_EX(OGLException);
	enum class GLComponent { Compiler, Driver, GPU, OGLU, Tex };
	const GLComponent exceptionSource;
protected:
	OGLException(const char* const type, const GLComponent source, const std::u16string_view& msg, const std::any& data_ = std::any())
		: common::BaseException(type, msg, data_), exceptionSource(source) { }
public:
	OGLException(const GLComponent source, const std::u16string_view& msg, const std::any& data_ = std::any())
		: OGLException(TYPENAME, source, msg, data_)
	{ }
	virtual ~OGLException() {}
};

//class GL


}