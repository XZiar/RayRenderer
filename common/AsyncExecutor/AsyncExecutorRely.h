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


#include "common/CommonRely.hpp"
#include "common/Exceptions.hpp"
#include "common/PromiseTask.hpp"
#include "common/IntrusiveDoubleLinkList.hpp"
#include <cstdint>
#include <string>
#include <memory>


namespace common
{
namespace asyexe
{


class AsyncAgent;
class AsyncManager;

using PmsCore = std::shared_ptr<::common::detail::PromiseResultCore>;
using AsyncTaskFunc = std::function<void(const AsyncAgent&)>;

class AsyncTaskException : public BaseException
{
public:
    EXCEPTION_CLONE_EX(AsyncTaskException);
    enum class Reason : uint8_t { Terminated, Timeout, Cancelled };
    const Reason reason;
    AsyncTaskException(const Reason reason_, const std::u16string_view& msg, const std::any& data_ = std::any())
        : BaseException(TYPENAME, msg, data_), reason(reason_)
    { }
    virtual ~AsyncTaskException() {}
};

namespace detail
{

template<class T>
class AsyncResult_ : public common::detail::PromiseResultCore, public std::enable_shared_from_this<AsyncResult_<T>>
{
    friend class common::asyexe::AsyncAgent;
private:
    T Wait()
    {
        Prepare();
        return WaitPms();
    }
protected:
    AsyncResult_()
    { }
    T virtual WaitPms() = 0;
};

}
// for the results which should only be waited inside executor thread
template<typename T>
using AsyncResult = std::shared_ptr<detail::AsyncResult_<T>>;

struct StackSize 
{
    constexpr static uint32_t Default = 0, Tiny = 4096, Small = 65536, Big = 512 * 1024, Large = 1024 * 1024, Huge = 4 * 1024 * 1024;
};


}
}
