#pragma once
#include "SystemCommonRely.h"
#include "common/Exceptions.hpp"
#include "common/EasyIterator.hpp"
#include <vector>


namespace common
{

std::vector<StackTraceItem> SYSCOMMONAPI GetStack(size_t skip = 0) noexcept;
#define CREATE_EXCEPTIONEX(ex, ...) ::common::BaseException::CreateWithStacks<ex>(::common::GetStack(), __VA_ARGS__)
#define COMMON_THROWEX(ex, ...) throw CREATE_EXCEPTIONEX(ex, __VA_ARGS__)


template<typename T>
class StackDataset
{
private:
    struct Record
    {
        uint32_t Offset;
        uint32_t Size;
        mutable T Data;
        template<typename... Args>
        Record(uint32_t offset, uint32_t size, Args&&... args) :
            Offset(offset), Size(size), Data(std::forward<Args>(args)...) { }
    };
    std::vector<StackTraceItem> AllStacks;
    std::vector<Record> Records;
    class RecordWrapper
    {
        friend StackDataset<T>;
        span<const StackTraceItem> TheStack;
        T* TheData;
        constexpr RecordWrapper(::common::span<const StackTraceItem> stacks, T* data) noexcept :
            TheStack(stacks), TheData(data) { }
    public:
        decltype(auto) Stack() const noexcept { return TheStack; }
        constexpr T* operator->() const noexcept { return TheData; }
        constexpr T& operator*() const noexcept { return *TheData; }
    };
    [[nodiscard]] constexpr RecordWrapper Get(size_t index) const noexcept
    {
        const auto& item = Records[index];
        return { ::common::to_span(AllStacks).subspan(item.Offset, item.Size), &item.Data };
    }
    using ItType = container::IndirectIterator<const StackDataset<T>, RecordWrapper, &StackDataset<T>::Get>;
    friend ItType;
public:
    template<typename... Args>
    T& Emplace(Args&&... args)
    {
        auto stacks = ::common::GetStack(1);
        const auto offset = AllStacks.size();
        if (stacks.size() > 0)
        {
            AllStacks.insert(AllStacks.end(), std::make_move_iterator(stacks.begin()), std::make_move_iterator(stacks.end()));
        }
        const auto size = AllStacks.size() - offset;
        return Records.emplace_back(gsl::narrow_cast<uint32_t>(offset), gsl::narrow_cast<uint32_t>(size), 
            std::forward<Args>(args)...).Data;
    }
    void Clear() noexcept
    {
        Records.clear();
        AllStacks.clear();
    }
    [[nodiscard]] constexpr ItType begin() const noexcept
    {
        return { this, 0 };
    }
    [[nodiscard]] constexpr ItType end() const noexcept
    {
        return { this, Records.size() };
    }
};

}