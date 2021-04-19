#pragma once

#include "DxRely.h"



#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DxCmdQue_;


class DXUAPI DxPromiseCore : public common::PromiseProvider
{
    friend class DxCmdQue_;
private:
    static void EventCallback(void* context, unsigned char isTimeout);
protected:
    std::shared_ptr<const DxCmdQue_> CmdQue;
    uint64_t Num;
    void* Handle;
    std::shared_ptr<common::ExceptionBasicInfo> WaitException;
    bool IsException;
    [[nodiscard]] common::PromiseState GetState() noexcept override;
    void PreparePms() override;
    [[nodiscard]] common::PromiseState WaitPms() noexcept override;
    [[nodiscard]] uint64_t ElapseNs() noexcept override;
public:
    DxPromiseCore(const DxCmdQue_& cmdQue, const uint64_t num, const bool isException = false);
    DxPromiseCore(const bool isException);
    ~DxPromiseCore();
    std::optional<common::BaseException> GetException() const;
};

template<typename T>
class COMMON_EMPTY_BASES DxPromise : public common::detail::PromiseResult_<T>
{
    friend class DxPromiseCore;
private:
    common::CachedPromiseProvider<DxPromiseCore> Promise;
    common::detail::ResultExHolder<T> Holder;
    [[nodiscard]] T GetResult() override
    {
        this->CheckResultExtracted();
        if (auto ex = Promise.GetException())
            Holder.SetException(*ex);
        return Holder.ExtraResult();
    }
public:
    DxPromise(std::exception_ptr ex) : Promise(true)
    {
        Holder.SetException(ex);
    }
    DxPromise(const DxException& ex) : Promise(true)
    {
        Holder.SetException(ex);
    }
    DxPromise(const DxCmdQue_& cmdQue, const uint64_t num) : Promise(cmdQue, num)
    { }
    template<typename U>
    DxPromise(const DxCmdQue_& cmdQue, const uint64_t num, U&& data) : Promise(cmdQue, num)
    {
        Holder.Result = std::forward<U>(data);
    }
    ~DxPromise() override { }
    DxPromiseCore& GetPromise() noexcept override
    {
        return Promise;
    }

    [[nodiscard]] static std::shared_ptr<DxPromise> CreateError(std::exception_ptr ex)
    {
        return std::make_shared<DxPromise>(ex);
    }
    [[nodiscard]] static std::shared_ptr<DxPromise> CreateError(const DxException& ex)
    {
        return std::make_shared<DxPromise>(ex);
    }
    [[nodiscard]] static std::shared_ptr<DxPromise> Create(const DxCmdQue_& cmdQue, const uint64_t num)
    {
        static_assert(std::is_same_v<T, void>, "Need return value");
        return std::make_shared<DxPromise>(cmdQue, num);
    }
    template<typename U>
    [[nodiscard]] static std::shared_ptr<DxPromise> Create(const DxCmdQue_& cmdQue, const uint64_t num, U&& data)
    {
        static_assert(!std::is_same_v<T, void>, "Don't want return value");
        return std::make_shared<DxPromise>(cmdQue, num, std::forward<U>(data));
    }
};


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
