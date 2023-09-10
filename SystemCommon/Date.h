#pragma once
#include "common/CommonRely.hpp"
#include <chrono>

/* chrone-date compatible include */

#if !COMMON_CPP_17
#   error Need at least C++17
#endif

#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
#   define SYSCOMMON_DATE 1
namespace common::date
{
using tzdb = std::chrono::tzdb;
using time_zone = std::chrono::time_zone;
using local_t = std::chrono::local_t;
template<class T> using zoned_traits = std::chrono::zoned_traits<T>;
template<class Duration, class TimeZonePtr = const std::chrono::time_zone*> using zoned_time = std::chrono::zoned_time<Duration, TimeZonePtr>;
using std::chrono::locate_zone;
using std::chrono::current_zone;

template<class Duration> using hh_mm_ss = std::chrono::hh_mm_ss<Duration>;
using std::chrono::day;
using std::chrono::month;
using std::chrono::year;
using std::chrono::weekday;
using std::chrono::month_day;
using std::chrono::month_day_last;
using std::chrono::month_weekday;
using std::chrono::year_month;
using std::chrono::year_month_day;
using std::chrono::year_month_day_last;
using std::chrono::year_month_weekday;
}
#else
#   define SYSCOMMON_DATE 2
#   define DATE_USE_DLL
#   define HAS_DEDUCTION_GUIDES 1
#   include "3rdParty/date/include/date/tz.h"
#   include "3rdParty/date/include/date/date.h"
namespace common::date
{
using tzdb = ::date::tzdb;
using time_zone = ::date::time_zone;
using local_t = ::date::local_t;
template<class T> using zoned_traits = ::date::zoned_traits<T>;
template<class Duration, class TimeZonePtr = const ::date::time_zone*> using zoned_time = ::date::zoned_time<Duration, TimeZonePtr>;
using ::date::locate_zone;
using ::date::current_zone;

template<class Duration> using hh_mm_ss = ::date::hh_mm_ss<Duration>;
using ::date::day;
using ::date::month;
using ::date::year;
using ::date::weekday;
using ::date::month_day;
using ::date::month_day_last;
using ::date::month_weekday;
using ::date::year_month;
using ::date::year_month_day;
using ::date::year_month_day_last;
using ::date::year_month_weekday;
}
#endif
