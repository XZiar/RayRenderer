#pragma once

#include "AsyncExecutorRely.h"
#include <functional>


namespace common
{
namespace asyexe
{
namespace detail
{
class AsyncCaller;
}

class ASYEXEAPI AsyncProxy
{
    friend class AsyncCaller;
public:
    template<typename T> using CompleteCallback = std::function<void(T&&)>;
    using ErrorCallback = std::function<void(const BaseException&)>;
private:
    struct AsyncNodeBase : public NonCopyable, public NonMovable
    {
        const PmsCore Promise;
        AsyncNodeBase(PmsCore&& promise) : Promise(std::move(promise)) {}
        virtual ~AsyncNodeBase() = 0;
        virtual void Resolve() const noexcept = 0;
    };
    template<typename T>
    struct AsyncNode : AsyncNodeBase
    {
        const CompleteCallback<T> OnCompleted;
        const ErrorCallback OnErrored;
        AsyncNode(const PromiseResult<T>& promise, CompleteCallback<T>&& onComplete, ErrorCallback&& onError)
            : AsyncNodeBase(promise), OnCompleted(std::move(onComplete)), OnErrored(std::move(onError))
        {}
        virtual ~AsyncNode() override {}
        virtual void Resolve() const noexcept
        {
            try 
            {
                auto pms = std::dynamic_pointer_cast<common::detail::PromiseResult_<T>>(Promise);
                OnCompleted(pms->Wait());
            }
            catch (const BaseException& be)
            {
                OnErrored(be);
            }
        }
    };
    static void AddNode(const AsyncNodeBase* node);
public:
    template<typename T>
    static void OnComplete(const PromiseResult<T>& pms, CompleteCallback<T>&& onComplete, ErrorCallback&& onError)
    {
        AddNode(new AsyncNode<T>(pms, std::move(onComplete), std::move(onError)));
    }
};


}
}