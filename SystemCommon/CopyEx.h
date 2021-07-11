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
public:
    using TBroadcast2 = void(uint16_t* dest, const uint16_t src, size_t count) noexcept;
    using TBroadcast4 = void(uint32_t* dest, const uint32_t src, size_t count) noexcept;
    using TZExtCopy12 = void(uint16_t* dest, const uint8_t* src, size_t count) noexcept;
    using TZExtCopy14 = void(uint32_t* dest, const uint8_t* src, size_t count) noexcept;
private:
    using VarItem = std::pair<std::string_view, std::string_view>;
    TBroadcast2* Broadcast2 = nullptr;
    TBroadcast4* Broadcast4 = nullptr;
    TZExtCopy12* ZExtCopy12 = nullptr;
    TZExtCopy14* ZExtCopy14 = nullptr;
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
        return Broadcast2 && Broadcast4 && ZExtCopy12 && ZExtCopy14;
    }

    template<typename T>
    forceinline void BroadcastMany(T* const dest, const T& src, const size_t count) const noexcept
    {
        if constexpr (sizeof(T) == 1)
        {
            memset_s(dest, count, &src, 1);
        }
        else if constexpr (sizeof(T) == 2)
            Broadcast2(reinterpret_cast<uint16_t*>(dest), *reinterpret_cast<const uint16_t*>(&src), count);
        else if constexpr (sizeof(T) == 4)
            Broadcast4(reinterpret_cast<uint32_t*>(dest), *reinterpret_cast<const uint32_t*>(&src), count);
        else
        {
            for (size_t i = 0; i < count; ++i)
                *dest++ = src;
        }
    }
    template<typename T, typename U>
    forceinline void ZExtCopy(T* const dest, const U* src, const size_t count) const noexcept
    {
        if constexpr (sizeof(T) == sizeof(U))
        {
            memcpy_s(dest, count * sizeof(T), src, count * sizeof(U));
        }
        else if constexpr (sizeof(U) == 1)
        {
            if constexpr (sizeof(T) == 2)
                ZExtCopy12(reinterpret_cast<uint16_t*>(dest), reinterpret_cast<const uint8_t*>(src), count);
            else if constexpr (sizeof(T) == 4)
                ZExtCopy14(reinterpret_cast<uint32_t*>(dest), reinterpret_cast<const uint8_t*>(src), count);
            else
                static_assert(AlwaysTrue<T>, "datatype casting not supported");
        }
        else
            static_assert(AlwaysTrue<T>, "datatype casting not supported");
    }
};

SYSCOMMONAPI extern const CopyManager CopyEx;


#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

}