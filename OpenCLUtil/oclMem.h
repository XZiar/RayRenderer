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

enum class MemFlag : cl_mem_flags
{
    Empty = 0,
    ReadOnly = CL_MEM_READ_ONLY,    WriteOnly = CL_MEM_WRITE_ONLY,     ReadWrite = CL_MEM_READ_WRITE,
    UseHost  = CL_MEM_USE_HOST_PTR, HostAlloc = CL_MEM_ALLOC_HOST_PTR, HostCopy  = CL_MEM_COPY_HOST_PTR,
    HostWriteOnly = CL_MEM_HOST_WRITE_ONLY, HostReadOnly = CL_MEM_HOST_READ_ONLY, HostNoAccess = CL_MEM_HOST_NO_ACCESS,
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

enum class MapFlag : cl_map_flags
{
    Read = CL_MAP_READ, Write = CL_MAP_WRITE, ReadWrite = Read | Write, InvalidWrite = CL_MAP_WRITE_INVALIDATE_REGION,
};
MAKE_ENUM_BITFIELD(MapFlag)


class oclMapPtr;

class OCLUAPI oclMem_ : public common::NonCopyable, public common::NonMovable, public std::enable_shared_from_this<oclMem_>
{
    friend class oclKernel_;
    friend class oclContext_;
    friend class oclMapPtr;
    template<typename> friend class oclGLObject_;
private:
    class OCLUAPI oclMapPtr_ : public common::NonCopyable, public common::NonMovable
    {
        friend class oclMapPtr;
    private:
        oclCmdQue Queue;
        oclMem_& Mem;
        common::span<std::byte> MemSpace;
    public:
        MAKE_ENABLER();
        oclMapPtr_(oclCmdQue&& que, oclMem_* mem, const MapFlag mapFlag);
        ~oclMapPtr_();
    };
protected:
    const oclContext Context;
    const cl_mem MemID;
    const MemFlag Flag;
    oclMem_(oclContext ctx, cl_mem mem, const MemFlag flag);
    [[nodiscard]] virtual common::span<std::byte> MapObject(const cl_command_queue& que, const MapFlag mapFlag) = 0;
public:
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