#pragma once

#ifdef ASYEXE_EXPORT
#   define ASYEXEAPI _declspec(dllexport)
#   define COMMON_EXPORT
#else
#   define ASYEXEAPI _declspec(dllimport)
#endif

#include <cstdint>
#include <string>
#include <memory>

#include "common/Exceptions.hpp"
#include "common/PromiseTask.hpp"

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
    AsyncTaskException(const Reason reason_, const std::wstring& msg, const std::any& data_ = std::any())
        : BaseException(TYPENAME, msg, data), reason(reason)
    { }
    virtual ~AsyncTaskException() {}
};

namespace detail
{

template<class T>
class COMMONTPL AsyncResult_ : public ::common::detail::PromiseResultCore, public std::enable_shared_from_this<AsyncResult_<T>>
{
    friend class common::asyexe::AsyncAgent;
protected:
    AsyncResult_()
    { }
    T virtual wait() = 0;
public:
    AsyncResult_(AsyncResult_&&) = default;
};

}
template<typename T>
using AsyncResult = std::shared_ptr<detail::AsyncResult_<T>>;

enum class StackSize : uint32_t { Default = 0, Tiny = 4096, Small = 65536, Big = 512 * 1024, Large = 1024 * 1024, Huge = 4 * 1024 * 1024 };


}
}
