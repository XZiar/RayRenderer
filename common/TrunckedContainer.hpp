#pragma once
#include "CommonRely.hpp"
#include "AlignedBase.hpp"
#include <tuple>
#include <vector>


namespace common::container
{


template<typename T>
class TrunckedContainer
{
protected:
    struct Trunk
    {
        T* Ptr;
        size_t Offset;
        size_t Avaliable;
        constexpr Trunk(T* ptr, size_t size, size_t used) noexcept : Ptr(ptr), Offset(used), Avaliable(size - used) { }
    };

    static constexpr size_t EleSize  = sizeof(T);
    static constexpr size_t EleAlign = alignof(T);
    std::vector<Trunk> Trunks;
    size_t DefaultTrunkLength, DefaultTrunkAlignment;
private:
    template<bool IsConst>
    class Iterator
    {
        friend TrunckedContainer;
    private:
        const std::vector<Trunk>* Host;
        size_t MainIdx, SubIdx;
        constexpr Iterator(const std::vector<Trunk>* host, size_t idx) noexcept : Host(host), MainIdx(idx), SubIdx(0) 
        {
            if (MainIdx < Host->size() && (*Host)[MainIdx].Offset == 0)
                ++*this;
        }
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = std::conditional_t<IsConst, const T&, T&>;

        [[nodiscard]] value_type operator*() const noexcept
        {
            return (*Host)[MainIdx].Ptr[SubIdx];
        }
        template<bool IsConst2>
        [[nodiscard]] constexpr bool operator!=(const Iterator<IsConst2>& other) const noexcept
        {
            return Host != other.Host || MainIdx != other.MainIdx || SubIdx != other.SubIdx;
        }
        template<bool IsConst2>
        [[nodiscard]] constexpr bool operator==(const Iterator<IsConst2>& other) const noexcept
        {
            return Host == other.Host && MainIdx == other.MainIdx && SubIdx == other.SubIdx;
        }
        constexpr Iterator& operator++() noexcept
        {
            SubIdx++;
            while (MainIdx < Host->size() && SubIdx >= (*Host)[MainIdx].Offset)
            {
                ++MainIdx;
                SubIdx = 0;
            }
            return *this;
        }
        constexpr Iterator& operator--() noexcept
        {
            while (SubIdx == 0)
            {
                if (MainIdx == 0)
                    return *this;
                MainIdx--;
                SubIdx = (*Host)[MainIdx].Offset;
            }
            SubIdx--;
            return *this;
        }
    };
public:
    using const_iterator = Iterator<true>;
    TrunckedContainer(const size_t trunkLength, const size_t trunkAlign = alignof(T)) noexcept 
        : DefaultTrunkLength(trunkLength), DefaultTrunkAlignment(trunkAlign) 
    {
        Expects(DefaultTrunkAlignment >= EleAlign);
        Expects(DefaultTrunkAlignment % EleAlign == 0);
    }
    TrunckedContainer(TrunckedContainer&&) noexcept = default;
    COMMON_NO_COPY(TrunckedContainer)
    ~TrunckedContainer()
    {
        for (const auto& trunk : Trunks)
        {
            ::common::free_align(trunk.Ptr);
        }
    }
    [[nodiscard]] T& AllocOne() noexcept
    {
        for (auto it = Trunks.end(); it != Trunks.begin();)
        {
            auto& trunk = *--it;
            if (trunk.Avaliable > 0)
            {
                T& ret = trunk.Ptr[trunk.Offset];
                trunk.Offset++;
                trunk.Avaliable--;
                return ret;
            }
        }
        {
            auto ptr = reinterpret_cast<T*>(::common::malloc_align(EleSize * DefaultTrunkLength, DefaultTrunkAlignment));
            Trunks.emplace_back(ptr, DefaultTrunkLength, 1);
            return *ptr;
        }
    }
    [[nodiscard]] span<T> Alloc(const size_t count, const size_t align = 1) noexcept
    {
        Expects(align > 0);
        for (auto it = Trunks.end(); it != Trunks.begin();)
        {
            auto& trunk = *--it;
            if (trunk.Avaliable >= count)
            {
                const auto baseline = reinterpret_cast<uintptr_t>(trunk.Ptr) / EleAlign;
                const auto trunkSize = trunk.Offset + trunk.Avaliable;
                const auto idx = (baseline + trunk.Offset + align - 1) / align * align - baseline;
                const auto newOffset = idx + count;
                if (newOffset <= trunk.Offset + trunk.Avaliable)
                {
                    trunk.Offset    = newOffset;
                    trunk.Avaliable = trunkSize - newOffset;
                    return { trunk.Ptr + idx, count };
                }
            }
        }
        if (count >= 8 * DefaultTrunkLength)
        {
            auto ptr = reinterpret_cast<T*>(::common::malloc_align(EleSize * count, EleAlign * align));
            Trunks.emplace_back(ptr, count, count);
            return { ptr, count };
        }
        else
        {
            const auto total = (count + DefaultTrunkLength - 1) / DefaultTrunkLength * DefaultTrunkLength;
            auto ptr = reinterpret_cast<T*>(::common::malloc_align(EleSize * total, EleAlign * align));
            Trunks.emplace_back(ptr, total, count);
            return { ptr, count };
        }
    }
    [[nodiscard]] bool TryDealloc(span<const T> space, const bool stackDealloc = false) noexcept
    {
        for (auto it = Trunks.begin(); it != Trunks.end(); ++it)
        {
            auto& trunk = *it;
            const auto maxPtr = trunk.Ptr + trunk.Offset;
            if (space.data() >= trunk.Ptr && space.data() < maxPtr)
            {
                const auto spaceEnd = space.data() + space.size();
                Expects(spaceEnd <= maxPtr);
                if constexpr (!std::is_trivially_destructible_v<T>)
                {
                    for (auto& item : space)
                        item.~T();
                }
                if (spaceEnd == maxPtr || stackDealloc)
                {
                    const auto len = maxPtr - space.data();
                    trunk.Offset    -= len;
                    trunk.Avaliable += len;
                    if (trunk.Offset == 0)
                    {
                        ::common::free_align(trunk.Ptr);
                        Trunks.erase(it);
                    }
                    return true;
                }
                return false;
            }
        }
        Expects(false);
        // not found
        return false;
    }
    [[nodiscard]] bool TryDealloc(const T& ele) noexcept
    {
        return TryDealloc(span<const T>{&ele, 1});
    }
    size_t GetAllocatedSlot() const noexcept 
    {
        size_t sum = 0;
        for (const auto& trunk : Trunks)
        {
            sum += trunk.Offset;
        }
        return sum;
    }

    constexpr Iterator<true>  begin() const noexcept { return { &Trunks,             0 }; }
    constexpr Iterator<true>  end()   const noexcept { return { &Trunks, Trunks.size() }; }
    constexpr Iterator<false> begin()       noexcept { return { &Trunks,             0 }; }
    constexpr Iterator<false> end()         noexcept { return { &Trunks, Trunks.size() }; }
};

}
