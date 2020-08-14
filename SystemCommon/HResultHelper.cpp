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

std::u16string HrToStr(long hresult)
{
    _com_error err(hresult);
    return reinterpret_cast<const char16_t*>(err.ErrorMessage());
}

}