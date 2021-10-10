#pragma once

#include "oclRely.h"
#include "oclCmdQue.h"


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclMem_;
using oclMem = std::shared_ptr<oclMem_>;

enum class MemFlag : uint64_t
{
    Empty = 0,
    ReadOnly = (1 << 2), WriteOnly = (1 << 1), ReadWrite = (1 << 0),
    UseHost  = (1 << 3), HostAlloc = (1 << 4), HostCopy  = (1 << 5),
    HostWriteOnly = (1 << 7), HostReadOnly = (1 << 8), HostNoAccess = (1 << 9),
    DeviceAccessMask = ReadOnly      | WriteOnly    | ReadWrite, 
    HostInitMask     = UseHost       | HostAlloc    | HostCopy, 
    HostAccessMask   = HostWriteOnly | HostReadOnly | HostNoAccess,
};
MAKE_ENUM_BITFIELD(MemFlag)

[[nodiscard]] constexpr bool CheckMemFlagDevAccess(const MemFlag flag) noexcept
{
    const auto isReadOnly = HAS_FIELD(flag, MemFlag::ReadOnly)  ? 1 : 0,
              isWriteOnly = HAS_FIELD(flag, MemFlag::WriteOnly) ? 1 : 0,
              isReadWrite = HAS_FIELD(flag, MemFlag::ReadWrite) ? 1 : 0;
    return isReadOnly + isWriteOnly + isReadWrite == 1;
}
[[nodiscard]] constexpr bool CheckMemFlagHostAccess(const MemFlag flag) noexcept
{
    const auto isReadOnly = HAS_FIELD(flag, MemFlag::HostReadOnly)  ? 1 : 0,
              isWriteOnly = HAS_FIELD(flag, MemFlag::HostWriteOnly) ? 1 : 0,
               isNoAccess = HAS_FIELD(flag, MemFlag::HostNoAccess)  ? 1 : 0;
    return isReadOnly + isWriteOnly + isNoAccess <= 1;
}
[[nodiscard]] constexpr bool CheckMemFlagHostInit(const MemFlag flag) noexcept
{
    if (HAS_FIELD(flag, MemFlag::UseHost))
        return (flag & MemFlag::HostInitMask) == MemFlag::UseHost;
    else
       return true;
}

enum class MapFlag : uint64_t
{
    Read = (1 << 0), Write = (1 << 1), ReadWrite = Read | Write, InvalidWrite = (1 << 2),
};
MAKE_ENUM_BITFIELD(MapFlag)


class oclMapPtr;

class OCLUAPI oclMem_ : public detail::oclCommon, public std::enable_shared_from_this<oclMem_>
{
    friend class oclKernel_;
    friend class oclContext_;
    friend class oclMapPtr;
    template<typename> friend class oclGLObject_;
private:
    class OCLUAPI oclMapPtr_
    {
        friend class oclMapPtr;
    private:
        oclCmdQue Queue;
        oclMem_& Mem;
        common::span<std::byte> MemSpace;
    public:
        MAKE_ENABLER();
        COMMON_NO_COPY(oclMapPtr_)
        COMMON_NO_MOVE(oclMapPtr_)
        oclMapPtr_(oclCmdQue&& que, oclMem_* mem, const MapFlag mapFlag);
        ~oclMapPtr_();
    };
protected:
    const oclContext Context;
    CLHandle<detail::CLMem> MemID;
    const MemFlag Flag;
    oclMem_(oclContext ctx, void* mem, const MemFlag flag);
    [[nodiscard]] virtual common::span<std::byte> MapObject(CLHandle<detail::CLCmdQue> que, const MapFlag mapFlag) = 0;
public:
    COMMON_NO_COPY(oclMem_)
    COMMON_NO_MOVE(oclMem_)
    virtual ~oclMem_();
    [[nodiscard]] oclMapPtr Map(oclCmdQue que, const MapFlag mapFlag);
    void Flush(const oclCmdQue& que);
    static MemFlag ProcessMemFlag(const oclContext_& context, MemFlag flag, const void* ptr);
};


class OCLUAPI oclMapPtr
{
    friend class oclMem_;
private:
    class oclMemInfo;
    oclMem Mem;
    std::shared_ptr<const oclMem_::oclMapPtr_> Ptr;
    oclMapPtr(oclMem mem, std::shared_ptr<const oclMem_::oclMapPtr_> ptr) noexcept : Mem(mem), Ptr(std::move(ptr)) { }
public:
    constexpr oclMapPtr() noexcept {}
    [[nodiscard]] common::span<std::byte> Get() const noexcept;
    template<typename T>
    [[nodiscard]] common::span<T> AsType() const noexcept { return common::span<T>(reinterpret_cast<T*>(Get().data()), Get().size() / sizeof(T)); }
    [[nodiscard]] common::AlignedBuffer AsBuffer() const noexcept;

    [[nodiscard]] static bool CheckIsCLBuffer(const common::AlignedBuffer& buffer) noexcept;
};


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif