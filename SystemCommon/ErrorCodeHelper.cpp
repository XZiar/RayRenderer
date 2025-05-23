#include "ErrorCodeHelper.h"
#include "Format.h"
#include "StringConvert.h"
#include <string.h>

#if COMMON_OS_WIN
# include <comdef.h>
# ifdef _NATIVE_WCHAR_T_DEFINED
#   ifdef _DEBUG
#     pragma comment(lib, "comsuppwd.lib")
#   else
#     pragma comment(lib, "comsuppw.lib")
#   endif
# else
#   ifdef _DEBUG
#     pragma comment(lib, "comsuppd.lib")
#   else
#     pragma comment(lib, "comsupp.lib")
#   endif
# endif
#endif

namespace common
{

#if COMMON_OS_WIN

void HResultHolder::FormatWith(str::FormatterHost& host, str::FormatterContext& context, const str::FormatSpec* spec) const
{
    _com_error err(Value);
    host.PutString(context, err.ErrorMessage(), nullptr);
    if (spec && spec->AlterForm)
    {
        host.PutString(context, "(", nullptr);
        host.PutInteger(context, static_cast<uint32_t>(Value), true, nullptr);
        host.PutString(context, ")", nullptr);
    }
}
std::u16string HResultHolder::FormatHr(long hresult) noexcept
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
void Win32ErrorHolder::FormatWith(str::FormatterHost& host, str::FormatterContext& context, const str::FormatSpec* spec) const
{
    host.PutString(context, FormatWin32Error(Value), nullptr);
    if (spec && spec->AlterForm)
    {
        host.PutString(context, "(", nullptr);
        host.PutInteger(context, static_cast<uint32_t>(Value), false, nullptr);
        host.PutString(context, ")", nullptr);
    }
}
std::u16string Win32ErrorHolder::FormatError(unsigned long err) noexcept
{
    return reinterpret_cast<const char16_t*>(FormatWin32Error(err));
}
Win32ErrorHolder Win32ErrorHolder::GetLastError() noexcept
{
    return Win32ErrorHolder{ ::GetLastError() };
}

#endif


#if COMMON_OS_WIN
const wchar_t* ErrnoText(int32_t err)
{
    thread_local wchar_t tmp[128] = { 0 };
    const auto ret = _wcserror_s(tmp, err);
    return ret == 0 ? tmp : nullptr;
}
#else
std::string_view ErrnoText(int32_t err)
{
    thread_local char tmp[128] = { 0 };
    [[maybe_unused]] const auto errret = strerror_r(err, tmp, sizeof(tmp));
#   if !COMMON_OS_DARWIN
    if constexpr (!std::is_integral_v<decltype(errret)>)
        return errret;
    else
#   endif
        return tmp;
}
#endif
void ErrnoHolder::FormatWith(str::FormatterHost& host, str::FormatterContext& context, const str::FormatSpec* spec) const
{
    host.PutString(context, ErrnoText(Value), nullptr);
    if (spec && spec->AlterForm)
    {
        host.PutString(context, "(", nullptr);
        host.PutInteger(context, static_cast<uint32_t>(Value), true, nullptr);
        host.PutString(context, ")", nullptr);
    }
}
std::u16string ErrnoHolder::FormatErrno(int32_t err) noexcept
{
#if COMMON_OS_WIN
    return reinterpret_cast<const char16_t*>(ErrnoText(err));
#else
    return str::to_u16string(ErrnoText(err));
#endif
}
ErrnoHolder ErrnoHolder::GetCurError() noexcept
{
    return errno;
}


}


