#include "HResultHelper.h"
#include <comdef.h>

#if !COMMON_OS_WIN
#   error "only support win32"
#endif

#ifdef _NATIVE_WCHAR_T_DEFINED
# ifdef _DEBUG
#   pragma comment(lib, "comsuppwd.lib")
# else
#   pragma comment(lib, "comsuppw.lib")
# endif
#else
# ifdef _DEBUG
#   pragma comment(lib, "comsuppd.lib")
# else
#   pragma comment(lib, "comsupp.lib")
# endif
#endif

namespace common
{

typename ::fmt::u16format_context::iterator HResultHolder::Format(fmt::u16format_context& ctx) const
{
    _com_error err(Value);
    return fmt::format_to(ctx.out(), FMT_STRING(u"{}"), err.ErrorMessage());
}
typename ::fmt::u32format_context::iterator HResultHolder::Format(fmt::u32format_context& ctx) const
{
    _com_error err(Value);
    return fmt::format_to(ctx.out(), FMT_STRING(U"{}"), err.ErrorMessage());
}
typename ::fmt::  wformat_context::iterator HResultHolder::Format(fmt::  wformat_context& ctx) const
{
    _com_error err(Value);
    return fmt::format_to(ctx.out(), FMT_STRING(L"{}"), err.ErrorMessage());
}
std::u16string HResultHolder::FormatHr(long hresult)
{
    _com_error err(hresult);
    return reinterpret_cast<const char16_t*>(err.ErrorMessage());
}


static const wchar_t* FormatWin32Error(unsigned long err)
{
    thread_local wchar_t tmp[2048] = { 0 };
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
        err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        tmp, 2000, NULL);
    return tmp;
}
typename ::fmt::u16format_context::iterator Win32ErrorHolder::Format(fmt::u16format_context& ctx) const
{
    return fmt::format_to(ctx.out(), FMT_STRING(u"{}"), FormatWin32Error(Value));
}
typename ::fmt::u32format_context::iterator Win32ErrorHolder::Format(fmt::u32format_context& ctx) const
{
    return fmt::format_to(ctx.out(), FMT_STRING(U"{}"), FormatWin32Error(Value));
}
typename ::fmt::  wformat_context::iterator Win32ErrorHolder::Format(fmt::  wformat_context& ctx) const
{
    return fmt::format_to(ctx.out(), FMT_STRING(L"{}"), FormatWin32Error(Value));
}
std::u16string Win32ErrorHolder::FormatError(unsigned long err)
{
    return reinterpret_cast<const char16_t*>(FormatWin32Error(err));
}
Win32ErrorHolder Win32ErrorHolder::GetLastError()
{
    return Win32ErrorHolder{ ::GetLastError() };
}


}