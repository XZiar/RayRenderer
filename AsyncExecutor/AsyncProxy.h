#pragma once

#include "AsyncExecutorRely.h"
#include "LoopBase.h"
#include "MiniLogger/MiniLogger.h"
#include <functional>
#include <optional>


namespace common
{
namespace asyexe
{

namespace detail
{
template<typename T>
struct CompleteCallback : public std::function<void(T)>
{
    using std::function<void(T)>::function;
};
template<>
struct CompleteCallback<void> : public std::function<void()>
{
    using std::function<void()>::function;
};
}

class ASYEXEAPI AsyncProxy final : private LoopBase
{
public:
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
        virtual ~AsyncNodeBase() {};
        virtual void Resolve(const ErrorCallback& onErr) const noexcept = 0;
    };
    

    template<typename T>
    struct AsyncNode : public AsyncNodeBase
    {
        detail::CompleteCallback<T> OnCompleted;
        ErrorCallback OnErrored;

        AsyncNode(PromiseResult<T> promise, detail::CompleteCallback<T> onComplete, ErrorCallback onError)
            : AsyncNodeBase(std::move(promise)), OnCompleted(std::move(onComplete)), OnErrored(std::move(onError))
        {}
        virtual ~AsyncNode() override {}
        virtual void Resolve(const ErrorCallback& onErr) const noexcept
        {
            try 
            {
                auto pms = std::dynamic_pointer_cast<common::detail::PromiseResult_<T>>(Promise);
                if constexpr (std::is_same_v<T, void>)
                {
                    pms->Wait();
                    OnCompleted();
                }
                else
                    OnCompleted(pms->Wait());
            }
            catch (const BaseException& be)
            {
                if (OnErrored)
                    OnErrored(be);
                else
                    onErr(be);
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
    virtual bool SleepCheck() noexcept override; // double check if shoul sleep
    virtual bool OnStart(std::any cookie) noexcept override;
    void AddNode(AsyncNodeBase* node);
public:
    template<typename Pms, typename CB>
    static void OnComplete(Pms&& pms, CB&& onComplete, ErrorCallback onError = nullptr)
    {
        using RetType = typename PromiseChecker<Pms>::TaskRet;
        GetSelf().AddNode(new AsyncNode<RetType>(std::forward<Pms>(pms), std::forward<CB>(onComplete), std::move(onError)));
    }
};


}
}