#pragma once

#pragma unmanaged

#include "RenderCore/RenderCore.h"
#include "common/Exceptions.hpp"
#include "common/miniLogger/miniLogger.h"

using namespace common;
using std::vector;

#pragma managed


#include <msclr/marshal_cppstd.h>

forceinline static std::u16string ToU16Str(System::String^ str)
{
    cli::pin_ptr<const wchar_t> ptr = PtrToStringChars(str);
    return std::u16string(reinterpret_cast<const char16_t*>(static_cast<const wchar_t*>(ptr)), str->Length);
}
forceinline static System::String^ ToStr(const std::string& str)
{
    return gcnew System::String(str.c_str(), 0, (int)str.length());
}
forceinline static System::String^ ToStr(const std::u16string& str)
{
    return gcnew System::String(reinterpret_cast<const wchar_t*>(str.c_str()), 0, (int)str.length());
}
forceinline static System::String^ ToStr(const std::wstring& str)
{
    return gcnew System::String(str.c_str(), 0, (int)str.length());
}
