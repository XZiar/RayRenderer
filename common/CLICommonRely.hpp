#pragma once

#ifndef _M_CEE
#error("CLI header should only be used with /clr")
#endif

#include "CommonRely.hpp"
#include "Wrapper.hpp"
#include <msclr/marshal_cppstd.h>

namespace common
{


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


template<typename T>
public value class CLIWrapper
{
private:
    T *Src;
public:
    CLIWrapper(const T& src) : Src(new T(src)) { }
    CLIWrapper(T&& src) : Src(new T(std::move(src))) { }
    T Extract() { T ret = *this; return ret; }
    static operator T(CLIWrapper<T> val) 
    {
        T obj = *val.Src;
        delete val.Src;
        val.Src = nullptr;
        return obj;
    }
};

}