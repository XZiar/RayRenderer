#pragma once

#include "oclRely.h"
#include "oclCmdQue.h"



#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclBuffer_;
class oclImage_;
class oclKernel_;


class OCLUAPI DependEvents : public common::NonCopyable
{
private:
    std::vector<cl_event> Events;
    DependEvents(std::vector<cl_event>&& event);
public:
    ~DependEvents();
    std::pair<const cl_event*, cl_uint> Get() const noexcept
    {
        if (Events.empty())
            return { nullptr, 0 };
        else
            return { Events.data(), static_cast<cl_uint>(Events.size()) };
    }
    static DependEvents Create(const common::PromiseStub& pmss) noexcept;
};

class OCLUAPI oclPromiseCore
{
    friend class oclBuffer_;
    friend class oclImage_;
    friend class oclKernel_;
    friend class DependEvents;
private:
    struct oclEvents
    {
    private:
        std::optional<std::vector<cl_event>> Events;
    public:
        constexpr oclEvents() {}
        constexpr oclEvents(std::vector<cl_event>&& evts) : Events(std::move(evts)) {}
        std::pair<const cl_event*, cl_uint> Get() const noexcept
        {
            if (Events.has_value())
                return { Events.value().data(), static_cast<cl_uint>(Events.value().size()) };
            else
                return { nullptr, 0 };
        }
    };
protected:
    using PrevType = std::variant<std::monostate, std::vector<std::shared_ptr<oclPromiseCore>>, std::shared_ptr<oclPromiseCore>>;
    [[nodiscard]] static PrevType TrimPms(std::vector<std::shared_ptr<oclPromiseCore>>&& pmss) noexcept
    {
        switch (pmss.size())
        {
        case 0:  return std::monostate{};
        case 1:  return pmss[0];
        default: return std::move(pmss);
        }
    }

    const cl_event Event;
    const oclCmdQue Queue;
    PrevType Prev;
    oclPromiseCore(PrevType&& prev, const cl_event e, oclCmdQue que);
    ~oclPromiseCore();
    void Flush();
    [[nodiscard]] common::PromiseState State();
    void Wait();
    [[nodiscard]] uint64_t ElapseNs();
    [[nodiscard]] uint64_t ChainedElapseNs();
    [[nodiscard]] const cl_event& GetEvent() { return Event; }
public:
    [[nodiscard]] static std::pair<std::vector<std::shared_ptr<oclPromiseCore>>, oclEvents>
        ParsePms(const common::PromiseStub& pmss) noexcept;

    enum class TimeType { Queued, Submit, Start, End };
    uint64_t QueryTime(TimeType type) const noexcept;
    std::string_view GetEventName() const noexcept;
};


template<typename T>
class oclPromise : public ::common::detail::PromiseResult_<T>, public oclPromiseCore
{
private:
    using RDataType = std::conditional_t<std::is_same_v<T, void>, uint32_t, T>;
    std::variant<std::monostate, std::exception_ptr, RDataType> Result;
    virtual void PreparePms() override
    {
        Flush();
        if (Result.index() == 1)
            std::rethrow_exception(std::get<1>(Result));
    }
    [[nodiscard]] common::PromiseState virtual State() override
    {
        if (Result.index() == 1)
            return common::PromiseState::Error;
        return oclPromiseCore::State();
    }
    [[nodiscard]] T WaitPms() override
    {
        if (Result.index() == 1)
            std::rethrow_exception(std::get<1>(Result));
        oclPromiseCore::Wait();
        if constexpr(!std::is_same_v<T, void>)
            return std::move(std::get<2>(Result));
    }

public:
    oclPromise(std::exception_ptr ex)
        : oclPromiseCore(std::monostate{}, nullptr, {}), Result(ex) { }
    oclPromise(PrevType&& prev, const cl_event e, const oclCmdQue& que)
        : oclPromiseCore(std::move(prev), e, que) { }
    oclPromise(PrevType&& prev, const cl_event e, const oclCmdQue& que, RDataType&& data)
        : oclPromiseCore(std::move(prev), e, que), Result(std::move(data)) { }

    ~oclPromise() override { }
    [[nodiscard]] uint64_t ElapseNs() override
    { 
        return oclPromiseCore::ElapseNs();
    }
    [[nodiscard]] uint64_t ChainedElapseNs() override
    { 
        return oclPromiseCore::ChainedElapseNs();
    };


    [[nodiscard]] static std::shared_ptr<oclPromise> CreateError(std::exception_ptr ex)
    {
        return std::make_shared<oclPromise>(std::monostate{}, nullptr, {});
    }
    [[nodiscard]] static std::shared_ptr<oclPromise> Create(const cl_event e, oclCmdQue que)
    {
        static_assert(std::is_same_v<T, void>, "Need return value");
        return std::make_shared<oclPromise>(std::monostate{}, e, que);
    }
    template<typename U>
    [[nodiscard]] static std::shared_ptr<oclPromise> Create(const cl_event e, oclCmdQue que, U&& data)
    {
        static_assert(!std::is_same_v<T, void>, "Don't want return value");
        return std::make_shared<oclPromise>(std::monostate{}, e, que, std::forward<U>(data));
    }
    [[nodiscard]] static std::shared_ptr<oclPromise> Create(std::vector<std::shared_ptr<oclPromiseCore>>&& prev, const cl_event e, oclCmdQue que)
    {
        static_assert(std::is_same_v<T, void>, "Need return value");
        return std::make_shared<oclPromise>(TrimPms(std::move(prev)), e, que);
    }
    template<typename U>
    [[nodiscard]] static std::shared_ptr<oclPromise> Create(std::vector<std::shared_ptr<oclPromiseCore>>&& prev, const cl_event e, oclCmdQue que, U&& data)
    {
        static_assert(!std::is_same_v<T, void>, "Don't want return value");
        return std::make_shared<oclPromise>(TrimPms(std::move(prev)), e, que, std::forward<U>(data));
    }
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
