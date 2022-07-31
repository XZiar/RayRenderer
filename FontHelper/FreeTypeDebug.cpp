//#include "ft2build.h"
//#include <freetype/config/ftoption.h>
//#include <freetype/freetype.h>
#include <freetype/ftlogging.h>
#include <freetype/internal/ftdebug.h>
//#include <freetype/internal/ftobjs.h>

#ifndef FT_DEBUG_LEVEL_ERROR
#   error need debug level
#endif

#include "SystemCommon/MiniLogger.h"
#include "SystemCommon/StringConvert.h"
#include <cstdio>
#include <string_view>


#if defined(__GNUC__) && !defined(__STDC_LIB_EXT1__)
#   define PrintTo vsnprintf
#else
#   define PrintTo vsnprintf_s
#endif

using namespace common::mlog;
MiniLogger<false>& Logger()
{
    static MiniLogger<false> ftlog(u"FreeType", { GetConsoleBackend() });
    return ftlog;
}

#ifdef FT_DEBUG_LEVEL_ERROR
static auto& GetBuffer() noexcept
{
    thread_local char str[2048];
    return str;
}
#endif


extern "C"
{


#ifdef FT_DEBUG_LEVEL_ERROR

/* documentation is in ftdebug.h */

FT_BASE_DEF(void) FT_Message(const char* fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    auto& str = GetBuffer();
    PrintTo(str, 2000, fmt, ap);
    Logger().Warning(std::string_view(str));
    va_end(ap);
}


/* documentation is in ftdebug.h */

FT_BASE_DEF(void) FT_Panic(const char* fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    auto& str = GetBuffer();
    PrintTo(str, 2000, fmt, ap);
    Logger().Error(std::string_view(str));
    va_end(ap);

    exit(EXIT_FAILURE);
}


/* documentation is in ftdebug.h */

FT_BASE_DEF(int) FT_Throw(FT_Error error, int line, const char* file)
{
    /* activating the code in this block makes FreeType very chatty */
    Logger().Warning(u"{}:{}: error {:02x}: {}\n", 
        file, line, error, FT_Error_String(error));

    return 0;
}


#endif /* FT_DEBUG_LEVEL_ERROR */


#ifdef FT_DEBUG_LEVEL_TRACE

#else  /* !FT_DEBUG_LEVEL_TRACE */


FT_BASE_DEF(void)
ft_debug_init(void)
{
    /* nothing */
}


FT_BASE_DEF(FT_Int)
FT_Trace_Get_Count(void)
{
    return 0;
}


FT_BASE_DEF(const char*)
FT_Trace_Get_Name(FT_Int  idx)
{
    FT_UNUSED(idx);

    return NULL;
}


FT_BASE_DEF(void)
FT_Trace_Disable(void)
{
    /* nothing */
}


/* documentation is in ftdebug.h */

FT_BASE_DEF(void)
FT_Trace_Enable(void)
{
    /* nothing */
}

#endif /* !FT_DEBUG_LEVEL_TRACE */


#ifdef FT_DEBUG_LOGGING

#else /* !FT_DEBUG_LOGGING */

FT_EXPORT_DEF(void)
FT_Trace_Set_Level(const char* level)
{
    FT_UNUSED(level);
}


FT_EXPORT_DEF(void)
FT_Trace_Set_Default_Level(void)
{
    /* nothing */
}


FT_EXPORT_DEF(void)
FT_Set_Log_Handler(FT_Custom_Log_Handler  handler)
{
    FT_UNUSED(handler);
}


FT_EXPORT_DEF(void)
FT_Set_Default_Log_Handler(void)
{
    /* nothing */
}

#endif /* !FT_DEBUG_LOGGING */


}


/* END */
