#define SYSCOMMON_CHARCONV_BUILD 1
#define BOOST_CHARCONV_STATIC_LINK 1
#define BOOST_CHARCONV_NO_QUADMATH 1
#define BOOST_CHARCONV_NO_LIB 1
#include "CharConvs.h"

#if CM_CHARCONV_INT == 1 || CM_CHARCONV_FP == 1
#   pragma message("Compiling CharConv with std charconv[" STRINGIZE(__cpp_lib_to_chars) "]" )
#endif

#if CM_CHARCONV_INT == 2 || CM_CHARCONV_FP == 2
#   pragma message("Compiling CharConv with boost charconv[" STRINGIZE(BOOST_LIB_VERSION) "]" )
#endif

#if CM_CHARCONV_INT == 0 || CM_CHARCONV_FP == 0
#   pragma message("Compiling CharConv with C lib" )
#endif

#if COMMON_COMPILER_MSVC
#   define SYSCOMMONTPL SYSCOMMONAPI
#else
#   define SYSCOMMONTPL
#endif

namespace common::detail
{
template SYSCOMMONTPL std::pair<bool, const char*> StrToInt(const std::string_view str, char& val, const int32_t base);
template SYSCOMMONTPL std::pair<bool, const char*> StrToInt(const std::string_view str, signed char& val, const int32_t base);
template SYSCOMMONTPL std::pair<bool, const char*> StrToInt(const std::string_view str, unsigned char& val, const int32_t base);
template SYSCOMMONTPL std::pair<bool, const char*> StrToInt(const std::string_view str, short& val, const int32_t base);
template SYSCOMMONTPL std::pair<bool, const char*> StrToInt(const std::string_view str, unsigned short& val, const int32_t base);
template SYSCOMMONTPL std::pair<bool, const char*> StrToInt(const std::string_view str, int& val, const int32_t base);
template SYSCOMMONTPL std::pair<bool, const char*> StrToInt(const std::string_view str, unsigned int& val, const int32_t base);
template SYSCOMMONTPL std::pair<bool, const char*> StrToInt(const std::string_view str, long& val, const int32_t base);
template SYSCOMMONTPL std::pair<bool, const char*> StrToInt(const std::string_view str, unsigned long& val, const int32_t base);
template SYSCOMMONTPL std::pair<bool, const char*> StrToInt(const std::string_view str, long long& val, const int32_t base);
template SYSCOMMONTPL std::pair<bool, const char*> StrToInt(const std::string_view str, unsigned long long& val, const int32_t base);

template SYSCOMMONTPL std::pair<bool, const char*> StrToFP(const std::string_view str, float& val, [[maybe_unused]] const bool isScientific);
template SYSCOMMONTPL std::pair<bool, const char*> StrToFP(const std::string_view str, double& val, [[maybe_unused]] const bool isScientific);
}
