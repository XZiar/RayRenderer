#pragma once

#ifndef _M_CEE
#error("CLI header should only be used with /clr")
#endif

#include "CommonRely.hpp"
#include "Wrapper.hpp"
#include <msclr/marshal_cppstd.h>
#include <string_view>


#define CLI_PUBLIC_PROPERTY(Type, Name, name) \
    property Type Name \
    { \
        Type get() { return name; } \
        void set(Type value) { name = value; } \
    }
#define CLI_PRIVATE_PROPERTY(Type, Name, name) \
    property Type Name \
    { \
    public: \
        Type get() { return name; } \
    private: \
        void set(Type value) { name = value; } \
    }
#define CLI_READONLY_PROPERTY(Type, Name, name) \
    property Type Name \
    { \
        Type get() { return name; } \
    }

namespace common
{


forceinline std::u16string ToU16Str(System::String^ str)
{
    cli::pin_ptr<const wchar_t> ptr = PtrToStringChars(str);
    return std::u16string(reinterpret_cast<const char16_t*>(static_cast<const wchar_t*>(ptr)), str->Length);
}
forceinline std::string ToCharStr(System::String^ str)
{
    return msclr::interop::marshal_as<std::string>(str);
}
forceinline System::String^ ToStr(const std::string& str)
{
    return gcnew System::String(str.c_str(), 0, (int)str.length());
}
forceinline System::String^ ToStr(const std::u16string& str)
{
    return gcnew System::String(reinterpret_cast<const wchar_t*>(str.c_str()), 0, (int)str.length());
}
forceinline System::String^ ToStr(const std::wstring& str)
{
    return gcnew System::String(str.c_str(), 0, (int)str.length());
}
forceinline System::String^ ToStr(const std::string_view& str)
{
    return gcnew System::String(str.data(), 0, (int)str.length());
}
forceinline System::String^ ToStr(const std::u16string_view& str)
{
    return gcnew System::String(reinterpret_cast<const wchar_t*>(str.data()), 0, (int)str.length());
}
forceinline System::String^ ToStr(const std::wstring_view& str)
{
    return gcnew System::String(str.data(), 0, (int)str.length());
}

template<typename T>
inline T ExchangeNullptr(T% pointer)
{
    return reinterpret_cast<T>(System::Threading::Interlocked::Exchange((System::IntPtr%)pointer, System::IntPtr::Zero).ToPointer());
}

template<typename T>
public ref class CLIWrapper
{
private:
    T* Src;
public:
    template<typename U>
    CLIWrapper(U&& src) : Src(new T(std::forward<U>(src))) { }
    ~CLIWrapper() { this->!CLIWrapper(); }
    !CLIWrapper() 
    { 
        if (auto src = ExchangeNullptr(Src); src)
            delete src;
    }
    T Extract() 
    { 
        T ret(std::move(static_cast<T>(this)));
        return ret;
    }
    T& Ref()
    {
        return *Src;
    }
    static operator T(CLIWrapper<T>^ val) 
    {
        auto src = ExchangeNullptr(val->Src);
        try
        {
            T obj(std::move(*src));
            return obj;
        }
        finally
        {
            delete src;
        }
    }
};

}