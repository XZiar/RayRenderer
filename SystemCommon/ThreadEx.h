#pragma once

#include "SystemCommonRely.h"
#include "MiscIntrins.h"
#include "common/STLEx.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <memory>
#include <optional>

#if !defined(_MANAGED) && !defined(_M_CEE)
#   include <thread>
#endif

namespace common
{

class ThreadObject;

struct TopologyInfo
{
    uint32_t BitsPerGroup = 0;
    [[nodiscard]] virtual uint32_t GetTotalProcessorCount() const noexcept = 0;
    [[nodiscard]] virtual uint32_t GetGroupCount() const noexcept = 0;
    [[nodiscard]] virtual uint32_t GetCountInGroup(uint32_t group) const noexcept = 0;
    [[nodiscard]] std::pair<uint32_t, uint32_t> TranslateLinearIdx(uint32_t idx) const noexcept
    {
        for (uint32_t gid = 0; gid < GetGroupCount(); ++gid)
        {
            const auto count = GetCountInGroup(gid);
            if (idx < count)
                return { gid, idx };
            idx -= count;
        }
        return { UINT32_MAX, UINT32_MAX };
    }
    SYSCOMMONAPI [[nodiscard]] static const TopologyInfo& Get() noexcept;
    SYSCOMMONAPI static void PrintTopology();
};


struct ThreadAffinity
{
    friend ThreadObject;
private:
    SmallBitset Mask;
    [[nodiscard]] std::byte* GetRawData(uint32_t group = 0) noexcept { return Mask.Data() + group * (TopologyInfo::Get().BitsPerGroup / 8); }
public:
    ThreadAffinity() noexcept : Mask(TopologyInfo::Get().GetGroupCount() * TopologyInfo::Get().BitsPerGroup, false) {}
    bool Get(uint32_t group, uint32_t idx) const noexcept
    {
        const auto& info = TopologyInfo::Get();
        Expects(group < info.GetGroupCount());
        Expects(idx < info.BitsPerGroup);
        Expects(idx < info.GetCountInGroup(group));
        return Mask.Get(group * info.BitsPerGroup + (idx % info.BitsPerGroup));
    }
    bool Get(uint32_t idx) const noexcept
    {
        const auto [gid, i] = TopologyInfo::Get().TranslateLinearIdx(idx);
        return Get(gid, i);
    }
    void Set(uint32_t group, uint32_t idx, const bool val) noexcept
    {
        const auto& info = TopologyInfo::Get();
        Expects(group < info.GetGroupCount());
        Expects(idx < info.BitsPerGroup);
        Expects(idx < info.GetCountInGroup(group));
        Mask.Set(group * info.BitsPerGroup + (idx % info.BitsPerGroup), val);
    }
    void Set(uint32_t idx, const bool val) noexcept
    {
        const auto [gid, i] = TopologyInfo::Get().TranslateLinearIdx(idx);
        Set(gid, i, val);
    }
    void SetGroup(uint32_t group, const bool val) noexcept
    {
        const auto& info = TopologyInfo::Get();
        Expects(group < info.GetGroupCount());
        const auto count = info.GetCountInGroup(group);
        for (uint32_t i = 0; i < count; ++i)
            Mask.Set(group * info.BitsPerGroup + i, val);
    }
    void SetAll(const bool val) noexcept
    {
        const auto& info = TopologyInfo::Get();
        const auto count = info.GetGroupCount();
        for (uint32_t i = 0; i < count; ++i)
            SetGroup(i, val);
    }
    bool Flip(uint32_t group, uint32_t idx) noexcept
    {
        const auto& info = TopologyInfo::Get();
        Expects(group < info.GetGroupCount());
        Expects(idx < info.BitsPerGroup);
        Expects(idx < info.GetCountInGroup(group));
        return Mask.Flip(group * info.BitsPerGroup + (idx % info.BitsPerGroup));
    }
    bool Flip(uint32_t idx) noexcept
    {
        const auto [gid, i] = TopologyInfo::Get().TranslateLinearIdx(idx);
        return Flip(gid, i);
    }
    [[nodiscard]] const std::byte* GetRawData(uint32_t group = 0) const noexcept { return Mask.Data() + group * (TopologyInfo::Get().BitsPerGroup / 8); }
    [[nodiscard]] uint32_t GetCount() const noexcept
    {
        const auto& info = TopologyInfo::Get();
        return MiscIntrin.PopCountRange<std::byte>({ GetRawData(), info.GetGroupCount() * info.BitsPerGroup / 8 });
    }
    SYSCOMMONAPI [[nodiscard]] std::string ToString(std::pair<char, char> ch = { 'x','.' }) const noexcept;
};


struct CPUPartition
{
    std::string PackageName;
    std::string PartitionName;
    ThreadAffinity Affinity;
    CPUPartition(std::string_view pkgName) : PackageName(pkgName) {}
    SYSCOMMONAPI [[nodiscard]] static span<const CPUPartition> GetPartitions() noexcept;
};


enum class ThreadQoS
{
    Default, Background, Utility, Burst, High
};


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
class ThreadObject
{
protected:
    uintptr_t Handle;
    uint64_t TId;
    constexpr ThreadObject(uintptr_t handle, uint64_t id) noexcept : Handle(handle), TId(id) { }
    SYSCOMMONAPI static bool CompareHandle(uintptr_t lhs, uintptr_t rhs) noexcept;
public:
    SYSCOMMONAPI static ThreadObject GetCurrentThreadObject();
    SYSCOMMONAPI static uint64_t GetCurrentThreadId();
#if !defined(_MANAGED) && !defined(_M_CEE)
    SYSCOMMONAPI static ThreadObject GetThreadObject(std::thread& thr);
#endif
    constexpr ThreadObject() noexcept : ThreadObject(0u, 0u) { }
    COMMON_NO_COPY(ThreadObject)
    ThreadObject(ThreadObject&& other) noexcept
    {
        Handle = other.Handle;
        other.Handle = 0;
        TId = other.TId;
    }
    ThreadObject& operator=(ThreadObject&& other) noexcept
    {
        std::swap(Handle, other.Handle);
        TId = other.TId;
        return *this;
    }
    SYSCOMMONAPI ~ThreadObject();
    SYSCOMMONAPI ThreadObject Duplicate() const;

    bool operator==(const ThreadObject& other) const noexcept
    {
        return CompareHandle(Handle, other.Handle);
    }
    bool operator!=(const ThreadObject& other) const noexcept
    {
        return !operator==(other);
    }

    SYSCOMMONAPI [[nodiscard]] std::optional<bool> IsAlive() const;
    SYSCOMMONAPI [[nodiscard]] bool IsCurrent() const;
    [[nodiscard]] constexpr uint64_t GetId() const noexcept { return TId; }
    SYSCOMMONAPI [[nodiscard]] std::u16string GetName() const;
    SYSCOMMONAPI bool SetName(const std::u16string_view name) const;
    SYSCOMMONAPI [[nodiscard]] std::optional<ThreadAffinity> GetAffinity() const;
    SYSCOMMONAPI bool SetAffinity(const ThreadAffinity& affinity) const;
    SYSCOMMONAPI [[nodiscard]] std::optional<ThreadQoS> GetQoS() const;
    SYSCOMMONAPI std::optional<ThreadQoS> SetQoS(ThreadQoS qos) const;
};

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif


}