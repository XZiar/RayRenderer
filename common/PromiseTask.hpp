#pragma once

#include "CommonRely.hpp"
#include "EnumEx.hpp"
#include <memory>
#include <functional>
#include <atomic>

namespace common
{

enum class PromiseState : uint8_t
{
    Invalid = 0, Unissued = 1, Issued = 8, Executing = 64, Executed = 128, Success = 144, Error = 192
};
MAKE_ENUM_RANGE(PromiseState)

namespace detail
{

struct PromiseResultCore : public NonCopyable
{
private:
    std::atomic_flag HasPrepared = ATOMIC_FLAG_INIT;
protected:
    virtual void PreparePms() {}
    PromiseState virtual State() { return PromiseState::Invalid; }
public:
    virtual ~PromiseResultCore() {}
    void Prepare()
    {
        if (!HasPrepared.test_and_set())
            PreparePms();
    }
    PromiseState GetState()
    {
        Prepare();
        return State();
    }
    uint64_t virtual ElapseNs() { return 0; };
    uint64_t virtual ChainedElapseNs() { return ElapseNs(); };
};

template<typename T>
class PromiseResult_ : public PromiseResultCore, public std::enable_shared_from_this<PromiseResult_<T>>
{
protected:
    PromiseResult_()
    { }
    T virtual WaitPms() = 0;
public:
    using ResultType = T;
    PromiseResult_(PromiseResult_&&) = default;
    T Wait()
    {
        Prepare();
        return WaitPms();
    }
};

template<typename RetType, typename MidType>
class StagedResult_ : PromiseResult_<RetType>
{
public:
    using PostExecute = std::function<RetType(MidType&&)>;
private:
    std::shared_ptr<PromiseResult_<MidType>> Stage1;
    PostExecute Stage2;
protected:
    RetType WaitPms() override
    {
        auto midArg = Stage1->Wait();
        return Stage2(std::move(midArg));
    }
    PromiseState State() override
    { 
        const auto state = Stage1->GetState();
        if (state == PromiseState::Success)
            return PromiseState::Executed;
        return state;
    }
public:
    StagedResult_(std::shared_ptr<PromiseResult_<MidType>>&& stage1, PostExecute&& stage2)
        : Stage1(std::move(stage1)), Stage2(std::move(stage2))
    { }
    ~StagedResult_() override {}
};

}

template<typename T>
using PromiseResult = std::shared_ptr<detail::PromiseResult_<T>>;
template<typename R, typename M>
using StagedResult = std::shared_ptr<detail::StagedResult_<R, M>>;


template<typename T>
class FinishedResult
{
private:
    class FinishedResult_ : public detail::PromiseResult_<T>
    {
        friend class FinishedResult<T>;
    private:
        T Data;
    protected:
        PromiseState State() override { return PromiseState::Executed; }
        T WaitPms() override { return std::move(Data); }
    public:
        FinishedResult_(T&& data) : Data(data) {}
        ~FinishedResult_() override {}
    };
public:
    template<typename U>
    static PromiseResult<T> Get(U&& data)
    {
        return std::make_shared<FinishedResult_>(std::forward<U>(data));
    }
};

template<>
class FinishedResult<void>
{
private:
    class FinishedResult_ : public detail::PromiseResult_<void>
    {
        friend class FinishedResult<void>;
    protected:
        PromiseState State() override { return PromiseState::Executed; }
        void WaitPms() override { return ; }
    public:
        FinishedResult_() {}
        ~FinishedResult_() override {}
    };
public:
    static PromiseResult<void> Get()
    {
        return std::make_shared<FinishedResult_>();
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