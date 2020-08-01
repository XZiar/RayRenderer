#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
# ifdef ASYEXE_EXPORT
#   define ASYEXEAPI _declspec(dllexport)
#   define COMMON_EXPORT
# else
#   define ASYEXEAPI _declspec(dllimport)
# endif
#else
# ifdef ASYEXE_EXPORT
#   define ASYEXEAPI __attribute__((visibility("default")))
#   define COMMON_EXPORT
# else
#   define ASYEXEAPI
# endif
#endif


#include "SystemCommon/PromiseTask.h"
#include "common/CommonRely.hpp"
#include "common/Exceptions.hpp"
#include "common/IntrusiveDoubleLinkList.hpp"
#include "common/TimeUtil.hpp"
#include <cstdint>
#include <string>
#include <memory>


namespace common
{
namespace asyexe
{


class AsyncAgent;
class AsyncManager;


class AsyncTaskException : public BaseException
{
public:
    EXCEPTION_CLONE_EX(AsyncTaskException);
    enum class Reason : uint8_t { Terminated, Timeout, Cancelled };
    const Reason reason;
    AsyncTaskException(const Reason reason_, const std::u16string_view& msg)
        : BaseException(TYPENAME, msg), reason(reason_)
    { }
    virtual ~AsyncTaskException() {}
};


struct StackSize 
{
    constexpr static uint32_t Default = 0, Tiny = 4096, Small = 65536, Big = 512 * 1024, Large = 1024 * 1024, Huge = 4 * 1024 * 1024;
};


}
}
