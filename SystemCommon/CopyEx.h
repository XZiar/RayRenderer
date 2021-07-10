#pragma once
#include "SystemCommonRely.h"
#include <cstdlib>

namespace common
{

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class SYSCOMMONAPI CopyManager
{
private:
    using VarItem = std::pair<std::string_view, std::string_view>;
    void (*Broadcast2)(uint16_t* dest, const uint16_t src, size_t count) noexcept = nullptr;
    void (*Broadcast4)(uint32_t* dest, const uint32_t src, size_t count) noexcept = nullptr;
    std::vector<VarItem> VariantMap;
public:
    [[nodiscard]] static common::span<const VarItem> GetSupportMap() noexcept;
    CopyManager(common::span<const VarItem> requests) noexcept;
    CopyManager() noexcept : CopyManager(GetSupportMap()) { }
    [[nodiscard]] common::span<const VarItem> GetIntrinMap() const noexcept
    {
        return VariantMap;
    }
    [[nodiscard]] bool IsComplete() const noexcept
    {
        return Broadcast2 && Broadcast4;
    }

    template<typename T>
    forceinline void BroadcastMany(T* const dest, const T& src, const size_t count) const noexcept
    {
        if constexpr (sizeof(T) == 1)
        {
            memset_s(dest, count, &src, 1);
        }
        else if constexpr (sizeof(T) == 2)
            Broadcast2(reinterpret_cast<uint16_t*>(dest), *(const uint16_t*)&src, count);
        else if constexpr (sizeof(T) == 4)
            Broadcast4(reinterpret_cast<uint32_t*>(dest), *(const uint32_t*)&src, count);
        else
        {
            for (size_t i = 0; i < count; ++i)
                *dest++ = src;
        }
    }
};

SYSCOMMONAPI extern const CopyManager CopyEx;


#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

}