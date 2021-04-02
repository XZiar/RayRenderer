#pragma once

#include "SystemCommonRely.h"
#include "common/Delegate.hpp"
#include "common/EnumEx.hpp"
#include "common/AtomicUtil.hpp"
#include "common/TimeUtil.hpp"
#include <memory>
#include <functional>
#include <atomic>
#include <variant>


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace common
{
namespace detail
{
template<typename T>
class PromiseResult_;
template<typename RetType, typename MidType>
class StagedResult_;
template<typename T, typename P>
class BasicResult_;
}
class PromiseResultCore;
class PromiseActiveProxy;
using PmsCore = std::shared_ptr<PromiseResultCore>;
template<typename T>
using PromiseResult = std::shared_ptr<detail::PromiseResult_<T>>;
template<typename T, typename P>
using BasicResult = std::shared_ptr<detail::BasicResult_<T, P>>;
template<typename T, typename R>
class BasicPromise;



namespace detail
{

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
struct ExceptionResult
{
    T Exception;
};
template<typename T>
class ResultExHolder
{
private:
    using T2 = std::conditional_t<std::is_same_v<T, void>, char, T>;
public:
    std::variant<std::monostate, ExceptionResult<std::exception_ptr>, ExceptionResult<std::shared_ptr<ExceptionBasicInfo>>, T2> Result;
    template<typename U>
    void SetException(U&& ex)
    {
        using U2 = std::decay_t<U>;
        if constexpr (std::is_base_of_v<common::BaseException, U2>)
            Result.template emplace<2>(ExceptionResult<std::shared_ptr<ExceptionBasicInfo>>{ex.InnerInfo()});
        else
            Result.template emplace<1>(ExceptionResult<std::exception_ptr>{std::forward<U>(ex)});
    }
    [[nodiscard]] T ExtraResult()
    {
        switch (Result.index())
        {
        case 0:
            COMMON_THROW(common::BaseException, u"Result unset");
        case 1:
            std::rethrow_exception(std::get<1>(Result).Exception);
        case 2:
            std::get<2>(Result).Exception->ThrowReal();
        default:
            break;
        }
        Ensures(Result.index() == 3);
        if constexpr (std::is_same_v<T, void>)
            return;
        else
            return std::get<3>(std::move(Result));
    }
    [[nodiscard]] bool IsException() const noexcept
    {
        return Result.index() == 1 || Result.index() == 2;
    }
};

}


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


class SYSCOMMONAPI COMMON_EMPTY_BASES PromiseProvider : public NonCopyable
{
    friend class PromiseActiveProxy;
    friend class PromiseResultCore;
    template<typename> friend class PromiseResult_;
public:
    RWSpinLock PromiseLock;
    AtomicBitfield<PromiseFlags> Flags = PromiseFlags::None;
    virtual ~PromiseProvider();
    [[nodiscard]] virtual PromiseState GetState() noexcept;
    virtual void PreparePms();
    virtual void MakeActive(PmsCore&& pms);
    virtual PromiseState WaitPms() noexcept = 0;
    [[nodiscard]] virtual uint64_t ElapseNs() noexcept { return 0; }
    [[nodiscard]] virtual PromiseProvider* GetParentProvider() const noexcept { return nullptr; }
    void Prepare();
};

template<typename T>
class CachedPromiseProvider final : public T
{
    static_assert(std::is_base_of_v<PromiseProvider, T>);
private:
    PromiseState CachedState = PromiseState::Invalid;
public:
    using T::T;
    ~CachedPromiseProvider() override { }
    [[nodiscard]] PromiseState GetState() noexcept final
    {
        if (CachedState >= PromiseState::Executed)
            return CachedState;
        else
            return CachedState = T::GetState();
    }
    PromiseState WaitPms() noexcept final
    {
        if (CachedState >= PromiseState::Executed)
            return CachedState;
        else
            return CachedState = T::WaitPms();
    }
};


class SYSCOMMONAPI COMMON_EMPTY_BASES PromiseResultCore
{
    friend class PromiseActiveProxy;
protected:
    [[nodiscard]] forceinline bool CheckFlag(const PromiseFlags field) noexcept
    {
        return GetPromise().Flags.Check(field);
    }
    [[nodiscard]] forceinline bool AddFlag(const PromiseFlags field) noexcept
    {
        return GetPromise().Flags.Add(field);
    }
    forceinline void CheckResultAssigned()
    {
        if (AddFlag(PromiseFlags::ResultAssigned))
            COMMON_THROW(BaseException, u"Result already assigned");
    }
    forceinline void CheckResultExtracted()
    {
        if (AddFlag(PromiseFlags::ResultExtracted))
            COMMON_THROW(BaseException, u"Result already extracted");
    }
public:
    virtual ~PromiseResultCore();
    virtual void AddCallback(std::function<void()> func) = 0;
    virtual void AddCallback(std::function<void(const PmsCore&)> func) = 0;
    virtual void ExecuteCallback() = 0;
    [[nodiscard]] virtual PromiseProvider& GetPromise() noexcept = 0;
    template<typename T> 
    [[nodiscard]] forceinline T* GetPromise() noexcept
    {
        return dynamic_cast<T*>(&GetPromise());
    }
    [[nodiscard]] PromiseProvider& GetRootPromise() noexcept;
    forceinline void Prepare()
    {
        GetPromise().Prepare();
    }
    [[nodiscard]] PromiseState State();
    void WaitFinish();
    [[nodiscard]] uint64_t ElapseNs() noexcept;
};


namespace detail
{

template<typename T>
class COMMON_EMPTY_BASES PromiseResult_ : public PromiseResultCore, public std::enable_shared_from_this<PromiseResult_<T>>
{
protected:
    Delegate<const PromiseResult<T>&> Callback;
    PromiseResult_()
    { }
    [[nodiscard]] forceinline PromiseResult<T> GetSelf() 
    { 
        return this->shared_from_this();
    }
    [[nodiscard]] virtual T GetResult() = 0;
    void ExecuteCallback() override
    {
        {
            auto lock = GetPromise().PromiseLock.WriteScope();
            if (AddFlag(PromiseFlags::CallbackExecuted))
                return;
        }
        Callback(GetSelf());
        Callback.Clear();
    }
public:
    using ResultType = T;
    PromiseResult_(PromiseResult_&&) = default;
    void AddCallback(std::function<void()> func) override
    {
        OnComplete(std::move(func));
    }
    void AddCallback(std::function<void(const PmsCore&)> func) override
    {
        OnComplete(std::move(func));
    }
    [[nodiscard]] T Get()
    {
        WaitFinish();
        return GetResult();
    }
    template<typename U>
    CallbackToken OnComplete(U&& callback)
    {
        if (State() < PromiseState::Executed)
        {
            auto lock = GetPromise().PromiseLock.ReadScope();
            if (!CheckFlag(PromiseFlags::CallbackExecuted))
            {
                auto token = Callback += callback;
                if (!AddFlag(PromiseFlags::Attached))
                    GetPromise().MakeActive(GetSelf());
                return token;
            }
        }
        if constexpr (std::is_invocable_v<U, const PromiseResult<T>&>)
        {
            callback(GetSelf());
        }
        else if constexpr (std::is_invocable_v<U>)
        {
            callback();
        }
        else
            static_assert(!AlwaysTrue<U>, "callback type unsupported");
        return {};
    }
};
//constexpr auto kkl = sizeof(Delegate<const PromiseResult<void>&>);
//constexpr auto kkl = sizeof(PromiseResult_<void>);


template<typename T, typename F>
struct InvokeRet
{
    static_assert(std::is_invocable_v<F, T>, "");
    using RetType = std::invoke_result_t<F, T>;
};
template<typename F>
struct InvokeRet<void, F>
{
    static_assert(std::is_invocable_v<F>, "");
    using RetType = std::invoke_result_t<F>;
};

}


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


class StagedResult
{
private:
    template<typename RetType, typename MidType>
    class ConvertResult_ final : public detail::PromiseResult_<RetType>
    {
    public:
        using PostExecute = std::function<RetType(MidType)>;
    private:
        PromiseResult<MidType> Stage1;
        PostExecute Stage2;
    protected:
        [[nodiscard]] RetType GetResult() override
        {
            if constexpr (std::is_same_v<MidType, void>)
                return Stage2();
            else
                return Stage2(Stage1->Get());
        }
        [[nodiscard]] PromiseProvider& GetPromise() noexcept override
        {
            return Stage1->GetPromise();
        }
    public:
        ConvertResult_(PromiseResult<MidType> stage1, PostExecute stage2)
            : Stage1(std::move(stage1)), Stage2(std::move(stage2))
        { }
        ~ConvertResult_() override {}
    };
    struct PostExecutor
    {
        virtual void PostProcess() = 0;
        virtual PromiseProvider& GetProvider() = 0;
    };
    struct SYSCOMMONAPI PostPmsProvider : public PromiseProvider
    {
        PostExecutor& Host;
        PostPmsProvider(PostExecutor& host) noexcept : Host(host) {}
        ~PostPmsProvider() override {}
        [[nodiscard]] PromiseState GetState() noexcept override;
        void PreparePms() final;
        void MakeActive(PmsCore&& pms) final;
        PromiseState WaitPms() noexcept override;
        [[nodiscard]] uint64_t ElapseNs() noexcept final;
        [[nodiscard]] PromiseProvider* GetParentProvider() const noexcept final;
    };
    template<typename RetType, typename MidType>
    class StagedResult_ final : public detail::PromiseResult_<RetType>, private PostExecutor
    {
    public:
        using PostExecute = std::function<RetType(MidType)>;
    private:
        PromiseResult<MidType> Stage1;
        PostExecute Stage2;
        CachedPromiseProvider<PostPmsProvider> Pms;
        std::optional<RetType> Result;

        void PostProcess() override
        {
            auto lock = Pms.PromiseLock.WriteScope();
            if constexpr (std::is_same_v<MidType, void>)
            {
                Result.emplace(Stage2());
            }
            else
            {
                Result.emplace(Stage2(Stage1->Get()));
            }
        }
        PromiseProvider& GetProvider() override
        {
            return Stage1->GetPromise();
        }
    protected:
        [[nodiscard]] RetType GetResult() override
        {
            if constexpr (std::is_same_v<RetType, void>)
                return;
            else
            {
                this->CheckResultExtracted();
                auto lock = Pms.PromiseLock.WriteScope();
                return std::move(Result.value());
            }
        }
    public:
        StagedResult_(PromiseResult<MidType> stage1, PostExecute stage2)
            : Stage1(std::move(stage1)), Stage2(std::move(stage2)), Pms(*this)
        { }
        ~StagedResult_() override {}
        [[nodiscard]] PromiseProvider& GetPromise() noexcept override
        {
            return Pms;
        }
    };
    template<typename MidType>
    class StagedResult_<void, MidType> final : public detail::PromiseResult_<void>, private PostExecutor
    {
    public:
        using PostExecute = std::function<void(MidType)>;
    private:
        PromiseResult<MidType> Stage1;
        PostExecute Stage2;
        CachedPromiseProvider<PostPmsProvider> Pms;

        void PostProcess() override
        {
            auto lock = Pms.PromiseLock.WriteScope();
            if constexpr (std::is_same_v<MidType, void>)
            {
                Stage2();
            }
            else
            {
                Stage2(Stage1->Get());
            }
        }
        PromiseProvider& GetProvider() override
        {
            return Stage1->GetPromise();
        }
    protected:
        void GetResult() override
        {
            return;
        }
    public:
        StagedResult_(PromiseResult<MidType> stage1, PostExecute stage2)
            : Stage1(std::move(stage1)), Stage2(std::move(stage2)), Pms(*this)
        { }
        ~StagedResult_() override {}
        [[nodiscard]] PromiseProvider& GetPromise() noexcept override
        {
            return Pms;
        }
    };
public:
    template<typename P, typename F>
    [[nodiscard]] static auto TwoStage(P&& stage1, F&& stage2) 
    {
        using MidType = typename PromiseChecker<P>::TaskRet;
        using RetType = typename detail::InvokeRet<MidType, F>::RetType;
        PromiseResult<RetType> ret = std::make_shared<StagedResult_<RetType, MidType>>(std::move(stage1), std::move(stage2));
        return ret;
    }
    template<typename P, typename F>
    [[nodiscard]] static auto Convert(P&& stage1, F&& stage2)
    {
        using MidType = typename PromiseChecker<P>::TaskRet;
        using RetType = typename detail::InvokeRet<MidType, F>::RetType;
        static_assert(!std::is_same_v<RetType, void>, "Should not return void");
        PromiseResult<RetType> ret = std::make_shared<ConvertResult_<RetType, MidType>>(std::move(stage1), std::move(stage2));
        return ret;
    }
};


template<typename T>
class FinishedResult
{
private:
    class COMMON_EMPTY_BASES FinishedResult_ final : protected detail::ResultHolder<T>, protected PromiseProvider,
        public detail::PromiseResult_<T>
    {
        [[nodiscard]] PromiseState GetState() noexcept override 
        { 
            return PromiseState::Executed;
        }
        PromiseState WaitPms() noexcept override 
        { 
            return PromiseState::Executed;
        }
        [[nodiscard]] T GetResult() override
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
        [[nodiscard]] PromiseProvider& GetPromise() noexcept override
        {
            return *this;
        }
    };
public:
    template<typename U>
    [[nodiscard]] static PromiseResult<T> Get(U&& data)
    {
        static_assert(!std::is_same_v<T, void>);
        return std::make_shared<FinishedResult_>(std::forward<U>(data));
    }
    [[nodiscard]] static PromiseResult<T> Get()
    {
        static_assert(std::is_same_v<T, void>);
        return std::make_shared<FinishedResult_>(detail::EmptyDummy{});
    }
};


class SYSCOMMONAPI BasicPromiseProvider : public PromiseProvider
{
    template<typename, typename> friend class detail::BasicResult_;
private:
    struct Waitable;
    Waitable* Ptr = nullptr;
    SimpleTimer Timer;
protected:
    std::atomic<PromiseState> TheState = PromiseState::Executing;
    BasicPromiseProvider();
public:
    ~BasicPromiseProvider() override;
    [[nodiscard]] PromiseState GetState() noexcept override;
    void MakeActive(PmsCore&&) override { }
    PromiseState WaitPms() noexcept override;
    [[nodiscard]] uint64_t ElapseNs() noexcept override;
    void NotifyState(PromiseState state);
};


namespace detail
{

template<typename T, typename P = BasicPromiseProvider>
class COMMON_EMPTY_BASES BasicResult_ : public PromiseResult_<T>
{
    static_assert(std::is_base_of_v<BasicPromiseProvider, P>);
    template<typename, typename> friend class ::common::BasicPromise;
protected:
    P Promise;
    ResultExHolder<T> Holder;
    [[nodiscard]] T GetResult() override
    {
        if constexpr (std::is_same_v<T, void>)
            return;
        else
        {
            this->CheckResultExtracted();
            auto lock = Promise.PromiseLock.WriteScope();
            return Holder.ExtraResult();
        }
    }
    template<typename U>
    forceinline void SetResult(U&& data)
    {
        static_assert(!std::is_same_v<T, void>);
        this->CheckResultAssigned();
        {
            auto lock = Promise.PromiseLock.WriteScope();
            Holder.Result = T(std::forward<U>(data));
        }
        Promise.NotifyState(PromiseState::Success);
        this->ExecuteCallback();
    }
    forceinline void SetResult(EmptyDummy)
    {
        static_assert(std::is_same_v<T, void>);
        this->CheckResultAssigned();
        Promise.NotifyState(PromiseState::Success);
        this->ExecuteCallback();
    }
    template<typename U>
    forceinline void SetException(U&& ex)
    {
        this->CheckResultAssigned();
        {
            auto lock = Promise.PromiseLock.WriteScope();
            Holder.SetException(std::move<U>(ex));
        }
        Promise.NotifyState(PromiseState::Error);
        this->ExecuteCallback();
    }
public:
    ~BasicResult_() override {}
    [[nodiscard]] P& GetPromise() noexcept override
    {
        return Promise;
    }
};

}

template<typename T, typename R = detail::BasicResult_<T>>
class BasicPromise
{
    static_assert(common::is_base_of_template<detail::BasicResult_, R>::value, "should be derived from BasicResult_<T, P>");
private:
    std::shared_ptr<R> Promise;
public:
    BasicPromise() : Promise(std::make_shared<R>()) { }
    constexpr const std::shared_ptr<R>& GetRawPromise() const noexcept
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


class [[nodiscard]] PromiseStub : public NonCopyable
{
private:
    std::vector<PmsCore> Promises;
    forceinline void Insert(const PmsCore& pms) noexcept
    {
        if (pms)
            Promises.emplace_back(pms);
    }
public:
    PromiseStub() noexcept 
    { }
    PromiseStub(const common::PromiseResult<void>& promise) noexcept
    {
        if (promise) Promises.emplace_back(promise);
    }
    PromiseStub(const std::vector<common::PromiseResult<void>>& promises) noexcept
    {
        Promises.reserve(promises.size());
        for (const auto& pms : promises)
            if (pms)
                Promises.emplace_back(pms);
    }
    PromiseStub(const std::initializer_list<common::PromiseResult<void>>& promises) noexcept
    {
        Promises.reserve(promises.size());
        for (const auto& pms : promises)
            if (pms)
                Promises.emplace_back(pms);
    }
    template<typename... Args>
    PromiseStub(const common::PromiseResult<Args>&... promises) noexcept
    {
        static_assert(sizeof...(Args) > 0, "should atleast give 1 argument");
        Promises.reserve(sizeof...(Args));
        (Insert(promises), ...);
    }

    [[nodiscard]] size_t size() const noexcept
    {
        return Promises.size();
    }

    template<typename T>
    [[nodiscard]] std::vector<std::shared_ptr<T>> FilterOut() const noexcept
    {
        std::vector<std::shared_ptr<T>> ret;
        for (const auto& pms : Promises)
            if (auto pms2 = pms->GetPromise<T>(); pms2)
                ret.emplace_back(pms, pms2);
        return ret;
    }
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
