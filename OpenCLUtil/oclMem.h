#pragma once

#include "oclRely.h"
#include "oclCmdQue.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{

enum class MemFlag : cl_mem_flags
{
    Empty = 0,
    ReadOnly = CL_MEM_READ_ONLY, WriteOnly = CL_MEM_WRITE_ONLY, ReadWrite = CL_MEM_READ_WRITE,
    UseHost = CL_MEM_USE_HOST_PTR, HostAlloc = CL_MEM_ALLOC_HOST_PTR, HostCopy = CL_MEM_COPY_HOST_PTR,
    HostWriteOnly = CL_MEM_HOST_WRITE_ONLY, HostReadOnly = CL_MEM_HOST_READ_ONLY, HostNoAccess = CL_MEM_HOST_NO_ACCESS,
    DeviceAccessMask = ReadOnly | WriteOnly | ReadWrite, HostInitMask = UseHost | HostAlloc | HostCopy, HostAccessMask = HostWriteOnly | HostReadOnly | HostNoAccess,
};
MAKE_ENUM_BITFIELD(MemFlag)

constexpr MemFlag AddMemHostCopyFlag(const MemFlag flag, const void* ptr)
{
    if (ptr != nullptr && (flag & MemFlag::UseHost) == MemFlag::Empty)
        return flag | MemFlag::HostCopy;
    else
        return flag;
}

enum class MapFlag : cl_map_flags
{
    Read = CL_MAP_READ, Write = CL_MAP_WRITE, ReadWrite = Read | Write, InvalidWrite = CL_MAP_WRITE_INVALIDATE_REGION,
};
MAKE_ENUM_BITFIELD(MapFlag)

class oclMapPtr;
template<typename T>
class oclGLMem;


class oclMem_;

class OCLUAPI oclMapPtr_ : public NonCopyable, public NonMovable
{
    friend class oclMem_;
    friend class oclMapPtr;
private:
    const oclCmdQue Queue;
    const cl_mem MemId;
    void* Pointer;
    oclMapPtr_(const oclCmdQue& que, const cl_mem mem, void* pointer) : Queue(que), MemId(mem), Pointer(pointer) {}
public:
    ~oclMapPtr_();
    void* GetPtr() const { return Pointer; }
};


class oclMapPtr : public std::shared_ptr<oclMapPtr_>
{
public:
    constexpr oclMapPtr() {}
    oclMapPtr(const std::shared_ptr<oclMapPtr_>& ptr) : std::shared_ptr<oclMapPtr_>(ptr) {}
    operator void*() const { return (*this)->GetPtr(); }
    template<typename T>
    T* AsType() const { return reinterpret_cast<T*>((*this)->GetPtr()); }
};


class OCLUAPI oclMem_ : public NonCopyable, public NonMovable
{
    friend class oclKernel_;
    friend class oclContext_;
    template<typename> friend class oclGLObject_;
private:
    std::atomic_flag LockObj = ATOMIC_FLAG_INIT;
    oclMapPtr MapPtr;
    std::weak_ptr<oclMapPtr_> TmpPtr;
protected:
    const oclContext Context;
    const cl_mem MemID;
    const MemFlag Flag;
    oclMem_(const oclContext& ctx, cl_mem mem, const MemFlag flag);
    virtual void* MapObject(const oclCmdQue& que, const MapFlag mapFlag) = 0;
    oclMapPtr TryGetMap() const;
public:
    virtual ~oclMem_();
    oclMapPtr Map(const oclCmdQue& que, const MapFlag mapFlag);
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif