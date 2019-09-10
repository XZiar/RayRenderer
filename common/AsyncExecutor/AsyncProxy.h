#pragma once

#include "AsyncExecutorRely.h"
#include "LoopBase.h"
#include "common/miniLogger/miniLogger.h"
#include <functional>


namespace common
{
namespace asyexe
{


class ASYEXEAPI AsyncProxy : public LoopBase
{
public:
    template<typename T> using CompleteCallback = std::function<void(T&&)>;
    using ErrorCallback = std::function<void(const BaseException&)>;

private:
    struct AsyncNodeBase : public NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<AsyncNodeBase>
    {
        const PmsCore Promise;
        common::SimpleTimer Timer;
        std::uint32_t Id = 0;
        AsyncNodeBase(PmsCore&& promise) : Promise(std::move(promise))
        { 
            Timer.Start();
        }
        virtual ~AsyncNodeBase() = 0;
        virtual void Resolve() const noexcept = 0;
    };
    template<typename T>
    struct AsyncNode : public AsyncNodeBase
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

    static AsyncProxy& GetSelf();

    common::container::IntrusiveDoubleLinkList<AsyncNodeBase> TaskList;
    common::mlog::MiniLogger<false> Logger;
    std::atomic_uint32_t TaskUid{ 0 };
    AsyncProxy();
    virtual ~AsyncProxy() override {}
    virtual LoopState OnLoop() override;
    virtual bool OnStart() noexcept override;
    void AddNode(AsyncNodeBase* node);
public:
    template<typename T>
    static void OnComplete(const PromiseResult<T>& pms, CompleteCallback<T>&& onComplete, ErrorCallback&& onError)
    {
        GetSelf().AddNode(new AsyncNode<T>(pms, std::move(onComplete), std::move(onError)));
    }
};


}
}