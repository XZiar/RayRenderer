#pragma once

#include "SystemCommonRely.h"
#include "common/AlignedBuffer.hpp"


namespace common
{

class SharedMemory : public std::enable_shared_from_this<SharedMemory>
{
    MAKE_ENABLER();
    struct Holder;
    uintptr_t Handle;
    span<std::byte> Space;
    constexpr SharedMemory(uintptr_t handle, span<std::byte> space) noexcept : Handle(handle), Space(space) {}
public:
    COMMON_NO_COPY(SharedMemory)
    COMMON_NO_MOVE(SharedMemory)
    SYSCOMMONAPI ~SharedMemory();
    [[nodiscard]] uintptr_t GetHandle() const noexcept { return Handle; }
    [[nodiscard]] span<std::byte> AsSpan() const noexcept { return Space; }
    SYSCOMMONAPI [[nodiscard]] AlignedBuffer AsBuffer() const noexcept;

    SYSCOMMONAPI [[nodiscard]] static std::shared_ptr<SharedMemory> CreateAnonymous(size_t size);
#if COMMON_OS_UNIX
    SYSCOMMONAPI [[nodiscard]] static int CreateMemFd(const char* name, uint32_t flags);
#endif
};

}
