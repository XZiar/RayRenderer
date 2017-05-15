#pragma once

#ifndef _M_CEE
#error("CLIAsync should only be used with /clr")
#endif

#include "Exceptions.hpp"

#include <msclr\marshal_cppstd.h>
using namespace System;
using namespace msclr::interop;

namespace common
{

public ref class CPPException : public Exception
{
private:
	String^ stacktrace;
	static Exception^ formInnerException(const BaseException& be)
	{
		if (auto inner = be.nestedException())
		{
			if (auto *fex = dynamic_cast<FileException*>(inner.get()))
			{
				if (fex->reason == FileException::Reason::NotExist)
					return gcnew IO::FileNotFoundException(marshal_as<String^>(fex->message),
						marshal_as<String^>(fex->filepath.wstring()), formInnerException(*fex));
				else
					return gcnew IO::FileLoadException(marshal_as<String^>(fex->message),
						marshal_as<String^>(fex->filepath.wstring()), formInnerException(*fex));
			}
			return gcnew CPPException(*inner);
		}
		else
			return nullptr;
	}
public:
	property String^ StackTrace
	{
		String^ get() override { return stacktrace; }
	}
	CPPException(const BaseException& be) : Exception(marshal_as<String^>(be.message), formInnerException(be))
	{
		auto stack = be.stacktrace();
		stacktrace = String::Format("at {0} : {1}\t({2})\n", marshal_as<String^>(stack.func), stack.line, marshal_as<String^>(stack.file));
	}
};

}