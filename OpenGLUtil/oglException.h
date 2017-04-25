#pragma once
#include "oglRely.h"

namespace oglu
{


class OGLException : public BaseException
{
public:
	EXCEPTION_CLONE_EX(OGLException);
	enum class GLComponent { Compiler, Driver, GPU, OGLU };
	const GLComponent exceptionSource;
	OGLException(const GLComponent source, const std::wstring& msg, const std::any& data_ = std::any())
		: BaseException(TYPENAME, msg, data), exceptionSource(source)
	{ }
	virtual ~OGLException() {}
};

//class GL


}