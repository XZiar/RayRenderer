#pragma once

#include "SystemCommonRely.h"
#include "common/Delegate.hpp"
#include "common/EnumEx.hpp"
#include "common/AtomicUtil.hpp"
#include "common/TimeUtil.hpp"
#include <memory>
#include <functional>
#include <atomic>


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace common
{
namespace detail
{
class PromiseResultCore;
class PromiseActiveProxy;
using PmsCore = std::shared_ptr<PromiseResultCore>;
template<typename T>
class PromiseResult_;
template<typename RetType, typename MidType>
class StagedResult_;
}
template<typename T>
using PromiseResult = std::shared_ptr<detail::PromiseResult_<T>>;
template<typename R, typename M>
using StagedResult = std::shared_ptr<detail::StagedResult_<R, M>>;



enum class PromiseState : uint8_t
{
    Invalid = 0, Unissued = 1, Issued = 8, Executing = 64, Executed = 128, Success = 144, Error = 192
};
MAKE_ENUM_RANGE(PromiseState)
enum class PromiseFlags : uint8_t 
{
    None = 0x0, Prepared = 0x1, Attached = 0x2, ResultAssigned = 0x4, CallbackExecuted = 0x8, ResultExtracted = 0x10
};
MAKE_ENUM_BITFIELD(PromiseFlags)


namespace detail
{

class SYSCOMMONAPI COMMON_EMPTY_BASES PromiseResultCore : public NonCopyable
{
    friend class PromiseActiveProxy;
protected:
    virtual PromiseState GetState() noexcept;
    virtual void PreparePms();
    virtual void EnsureActive(PmsCore&& pms);
    virtual void WaitPms() noexcept = 0;
    virtual void ExecuteCallback() = 0;
public:
    virtual ~PromiseResultCore();
    void Prepare();
    PromiseState State();
    void WaitFinish();
    virtual uint64_t ElapseNs() { return 0; };
    virtual uint64_t ChainedElapseNs() { return ElapseNs(); };
protected:
    RWSpinLock PromiseLock;
    AtomicBitfield<PromiseFlags> Flags = PromiseFlags::None;
    forceinline void CheckResultAssigned()
    {
        if (Flags.Add(PromiseFlags::ResultAssigned))
            COMMON_THROW(BaseException, u"Result already assigned");
    }
    forceinline void CheckResultExtracted()
    {
        if (Flags.Add(PromiseFlags::ResultExtracted))
            COMMON_THROW(BaseException, u"Result already extracted");
    }
private:
    PromiseState CachedState = PromiseState::Invalid;
};


struct EmptyDummy {};
template<typename T>
class ResultHolder
{
protected:
    T Result;
    template<typename U>
    ResultHolder(U&& result) : Result(std::forward<U>(result)) { }
};
template<>
class ResultHolder<void>
{
protected:
    constexpr ResultHolder(EmptyDummy) noexcept { }
};


template<typename T>
class COMMON_EMPTY_BASES PromiseResult_ : public PromiseResultCore, public std::enable_shared_from_this<PromiseResult_<T>>
{
protected:
    Delegate<const PromiseResult<T>&> Callback;
    PromiseResult_()
    { }
    forceinline PromiseResult<T> GetSelf() { return this->shared_from_this(); }
    virtual T GetResult() = 0;
    void ExecuteCallback() override
    {
        {
            auto lock = PromiseLock.WriteScope();
            if (Flags.Add(PromiseFlags::CallbackExecuted))
                return;
        }
        Callback(GetSelf());
        Callback.Clear();
    }
public:
    using ResultType = T;
    PromiseResult_(PromiseResult_&&) = default;
    T Get()
    {
        WaitFinish();
        return GetResult();
    }
    template<typename U>
    CallbackToken OnComplete(U&& callback)
    {
        if (State() < PromiseState::Executed)
        {
            auto lock = PromiseLock.ReadScope();
            if (!Flags.Check(PromiseFlags::CallbackExecuted))
            {
                auto token = Callback += callback;
                EnsureActive(GetSelf());
                return token;
            }
        }
        if constexpr (std::is_invocable_v<U, const PromiseResult_<T>&>)
        {
            callback(GetSelf());
        }
        else if constexpr (std::is_invocable_v<U>)
        {
            callback();
        }
        return {};
    }
};
//constexpr auto kkl = sizeof(Delegate<const PromiseResult<void>&>);
//constexpr auto kkl = sizeof(PromiseResult_<void>);


template<typename RetType, typename MidType>
class StagedResult_ : public PromiseResult_<RetType>
{
public:
    using PostExecute = std::function<RetType(MidType&&)>;
private:
    PromiseResult<MidType> Stage1;
    PostExecute Stage2;
protected:
    PromiseState GetState() noexcept override
    {
        const auto state = Stage1->GetState();
        if (state == PromiseState::Success)
            return PromiseState::Executed;
        return state;
    }
    void WaitPms() noexcept override 
    { 
        Stage1->WaitPms();
    }
    RetType GetResult() override
    {
        return Stage2(Stage1->GetResult());
    }
public:
    StagedResult_(PromiseResult<MidType>&& stage1, PostExecute&& stage2)
        : Stage1(std::move(stage1)), Stage2(std::move(stage2))
    { }
    ~StagedResult_() override {}
};
//constexpr auto kkl = sizeof(StagedResult_<int, double>);

}


template<typename T>
class FinishedResult
{
private:
    class COMMON_EMPTY_BASES FinishedResult_ final : protected detail::ResultHolder<T>, public detail::PromiseResult_<T>
    {
        PromiseState GetState() noexcept override { return PromiseState::Executed; }
        void WaitPms() noexcept override { }
        T GetResult() override 
        { 
            if constexpr (std::is_same_v<T, void>)
                return;
            else
            {
                this->CheckResultExtracted();
                return std::move(this->Result);
            }
        }
    public:
        template<typename U>
        FinishedResult_(U&& data) : detail::ResultHolder<T>(std::forward<U>(data)) {}
        ~FinishedResult_() override {}
    };
public:
    template<typename U>
    static PromiseResult<T> Get(U&& data)
    {
        static_assert(!std::is_same_v<T, void>);
        return std::make_shared<FinishedResult_>(std::forward<U>(data));
    }
    static PromiseResult<T> Get()
    {
        static_assert(std::is_same_v<T, void>);
        return std::make_shared<FinishedResult_>(detail::EmptyDummy{});
    }
};


template<typename T, typename P>
class BasicPromise;

namespace detail
{

template<typename T>
struct ExceptionResult
{
    T Exception;
};
template<typename T>
class ResultExHolder
{
private:
    using T2 = std::conditional_t<std::is_same_v<T, void>, char, T>;
protected:
    std::variant<std::monostate, ExceptionResult<std::exception_ptr>, ExceptionResult<std::shared_ptr<BaseException>>, T2> Result;
    T ExtraResult()
    {
        switch (Result.index())
        {
        case 0:
            COMMON_THROW(common::BaseException, u"Result unset");
            break;
        case 1:
            std::rethrow_exception(std::get<1>(Result).Exception);
            break;
        case 2:
            std::get<2>(Result).Exception->ThrowSelf();
            break;
        case 3:
            if constexpr (std::is_same_v<T, void>)
                return;
            else
                return std::get<3>(std::move(Result));
        }
        return {};
    }
    bool IsException() const noexcept
    {
        return Result.index() == 1 || Result.index() == 2;
    }
};

class SYSCOMMONAPI COMMON_EMPTY_BASES BasicPromiseResult_
{
private:
    struct Waitable;
    Waitable* Ptr = nullptr;
protected:
    std::atomic<PromiseState> TheState = PromiseState::Executing;
    BasicPromiseResult_();
    ~BasicPromiseResult_();
    void Wait() noexcept;
    void NotifyState(PromiseState state);
};

template<typename T>
class COMMON_EMPTY_BASES BasicResult_ : public PromiseResult_<T>, public BasicPromiseResult_, protected ResultExHolder<T>
{
    template<typename, typename> friend class ::common::BasicPromise;
private:
    SimpleTimer Timer;
    PromiseState GetState() noexcept override { return this->TheState; }
    void WaitPms() noexcept override { return this->Wait(); }
    T GetResult() override
    {
        if constexpr (std::is_same_v<T, void>)
            return;
        else
        {
            this->CheckResultExtracted();
            auto lock = this->PromiseLock.WriteScope();
            return this->ExtraResult();
        }
    }
    void EnsureActive(detail::PmsCore&&) override
    { }
    uint64_t ElapseNs() override { return this->Timer.ElapseNs(); };
    template<typename U>
    forceinline void SetResult(U&& data)
    {
        static_assert(!std::is_same_v<T, void>);
        this->CheckResultAssigned();
        {
            auto lock = this->PromiseLock.WriteScope();
            this->Result = T(std::forward<U>(data));
        }
        Timer.Stop();
        this->NotifyState(PromiseState::Success);
        this->ExecuteCallback();
    }
    forceinline void SetResult(EmptyDummy)
    {
        static_assert(std::is_same_v<T, void>);
        this->CheckResultAssigned();
        Timer.Stop();
        this->NotifyState(PromiseState::Success);
        this->ExecuteCallback();
    }
    forceinline void SetException(std::exception_ptr ex)
    {
        this->CheckResultAssigned();
        Timer.Stop();
        {
            auto lock = this->PromiseLock.WriteScope();
            this->Result = ExceptionResult<std::exception_ptr>{ ex };
        }
        this->NotifyState(PromiseState::Error);
        this->ExecuteCallback();
    }
    forceinline void SetException(std::shared_ptr<BaseException> ex)
    {
        this->CheckResultAssigned();
        Timer.Stop();
        {
            auto lock = this->PromiseLock.WriteScope();
            this->Result = std::move(ex); 
        }
        this->NotifyState(PromiseState::Error);
        this->ExecuteCallback();
    }
    template<typename U, typename = std::enable_if_t<std::is_base_of_v<BaseException, U>>>
    forceinline void SetException(const U& ex)
    {
        this->CheckResultAssigned();
        Timer.Stop();
        {
            auto lock = this->PromiseLock.WriteScope();
            this->Result = ex.Share();
        }
        this->NotifyState(PromiseState::Error);
        this->ExecuteCallback();
    }
public:
    BasicResult_()
    {
        Timer.Start();
    }
    ~BasicResult_() override {}
};

}

template<typename T, typename P = detail::BasicResult_<T>>
class BasicPromise
{
    static_assert(std::is_base_of_v<detail::BasicResult_<T>, P>, "should be derived from BasicResult_<T>");
private:
    std::shared_ptr<P> Promise;
public:
    BasicPromise() : Promise(std::make_shared<P>()) { }
    constexpr const std::shared_ptr<P>& GetRawPromise() const noexcept
    {
        return Promise;
    }
    PromiseResult<T> GetPromiseResult() const noexcept
    {
        return Promise;
    }
    template<typename U>
    void SetData(U&& data) const noexcept
    {
        static_assert(!std::is_same_v<T, void>);
        Promise->SetResult(std::forward<U>(data));
    }
    void SetData() const noexcept
    {
        static_assert(std::is_same_v<T, void>);
        Promise->SetResult(detail::EmptyDummy{});
    }
    template<typename U>
    void SetException(U&& ex) const noexcept
    {
        Promise->SetException(std::forward<U>(ex));
    }
};



template<typename T>
struct PromiseChecker
{
private:
    using PureType = common::remove_cvref_t<T>;
    static_assert(common::is_specialization<PureType, std::shared_ptr>::value, "task should be wrapped by shared_ptr");
    using TaskType = typename PureType::element_type;
    static_assert(common::is_specialization<TaskType, common::detail::PromiseResult_>::value, "task should be PromiseResult");
public:
    using TaskRet = typename TaskType::ResultType;
};
template<typename T>
inline constexpr bool IsPromiseResult()
{
    if constexpr (!common::is_specialization<T, std::shared_ptr>::value)
        return false;
    else
    {
        using TaskType = typename T::element_type;
        if (!common::is_specialization<TaskType, common::detail::PromiseResult_>::value)
            return false;
    }
    return true;
}
template<typename T>
inline constexpr void EnsurePromiseResult()
{
    static_assert(common::is_specialization<T, std::shared_ptr>::value, "task should be wrapped by shared_ptr");
    using TaskType = typename T::element_type;
    static_assert(common::is_specialization<TaskType, common::detail::PromiseResult_>::value, "task should be PromiseResult");
}


class [[nodiscard]] PromiseStub : public NonCopyable
{
private:
    std::variant<
        std::monostate,
        std::reference_wrapper<const common::PromiseResult<void>>,
        std::reference_wrapper<const std::vector<common::PromiseResult<void>>>,
        std::reference_wrapper<const std::initializer_list<common::PromiseResult<void>>>,
        std::vector<std::shared_ptr<detail::PromiseResultCore>>
    > Promises;
public:
    constexpr PromiseStub() noexcept : Promises() { }
    constexpr PromiseStub(const common::PromiseResult<void>& promise) noexcept :
        Promises(promise) { }
    constexpr PromiseStub(const std::vector<common::PromiseResult<void>>& promises) noexcept :
        Promises(promises) { }
    constexpr PromiseStub(const std::initializer_list<common::PromiseResult<void>>& promises) noexcept :
        Promises(promises) { }
    template<typename... Args>
    PromiseStub(const common::PromiseResult<Args>&... promises) noexcept
    {
        static_assert(sizeof...(Args) > 0, "should atleast give 1 argument");
        std::vector<std::shared_ptr<detail::PromiseResultCore>> pmss;
        pmss.reserve(sizeof...(Args));
        (pmss.push_back(promises), ...);
        Promises = std::move(pmss);
    }

    constexpr size_t size() const noexcept
    {
        switch (Promises.index())
        {
        case 0: return 0;
        case 1: return 1;
        case 2: return std::get<2>(Promises).get().size();
        case 3: return std::get<3>(Promises).get().size();
        case 4: return std::get<4>(Promises)      .size();
        default:return 0;
        }
    }

    template<typename T>
    constexpr std::vector<std::shared_ptr<T>> FilterOut() const noexcept
    {
        std::vector<std::shared_ptr<T>> ret;
        switch (Promises.index())
        {
        case 1:
            if (auto pms = std::dynamic_pointer_cast<T>(std::get<1>(Promises).get()); pms)
                ret.push_back(pms);
            break;
        case 2:
            for (const auto& pms : std::get<2>(Promises).get())
                if (auto pms2 = std::dynamic_pointer_cast<T>(pms); pms2)
                    ret.push_back(pms2);
            break;
        case 3:
            for (const auto& pms : std::get<3>(Promises).get())
                if (auto pms2 = std::dynamic_pointer_cast<T>(pms); pms2)
                    ret.push_back(pms2);
            break;
        case 4:
            for (const auto& pms : std::get<4>(Promises))
                if (auto pms2 = std::dynamic_pointer_cast<T>(pms); pms2)
                    ret.push_back(pms2);
            break;
        default:
            break;
        }
        return ret;
    }
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
