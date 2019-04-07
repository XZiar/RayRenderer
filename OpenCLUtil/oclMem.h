#pragma once

#include "oclRely.h"
#include "oclCmdQue.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275)
#endif

namespace oclu
{

enum class MemFlag : cl_mem_flags
{
    ReadOnly = CL_MEM_READ_ONLY, WriteOnly = CL_MEM_WRITE_ONLY, ReadWrite = CL_MEM_READ_WRITE,
    UseHost = CL_MEM_USE_HOST_PTR, HostAlloc = CL_MEM_ALLOC_HOST_PTR, HostCopy = CL_MEM_COPY_HOST_PTR,
    HostWriteOnly = CL_MEM_HOST_WRITE_ONLY, HostReadOnly = CL_MEM_HOST_READ_ONLY, HostNoAccess = CL_MEM_HOST_NO_ACCESS,
    DeviceAccessMask = ReadOnly | WriteOnly | ReadWrite, HostInitMask = UseHost | HostAlloc | HostCopy, HostAccessMask = HostWriteOnly | HostReadOnly | HostNoAccess,
};
MAKE_ENUM_BITFIELD(MemFlag)

enum class MapFlag : cl_map_flags
{
    Read = CL_MAP_READ, Write = CL_MAP_WRITE, ReadWrite = Read | Write, InvalidWrite = CL_MAP_WRITE_INVALIDATE_REGION,
};
MAKE_ENUM_BITFIELD(MapFlag)

class oclMapPtr;
template<typename T>
class oclGLMem;


namespace detail
{
class _oclMem;

class OCLUAPI _oclMapPtr : public NonCopyable, public NonMovable
{
    friend class _oclMem;
    friend class oclMapPtr;
private:
    const oclCmdQue Queue;
    const cl_mem MemId;
    void* Pointer;
    _oclMapPtr(const oclCmdQue& que, const cl_mem mem, void* pointer) : Queue(que), MemId(mem), Pointer(pointer) {}
public:
    ~_oclMapPtr();
    void* GetPtr() const { return Pointer; }
};
}
class oclMapPtr : public std::shared_ptr<detail::_oclMapPtr>
{
public:
    constexpr oclMapPtr() {}
    oclMapPtr(const std::shared_ptr<detail::_oclMapPtr>& ptr) : std::shared_ptr<detail::_oclMapPtr>(ptr) {}
    operator void*() const { return (*this)->GetPtr(); }
    template<typename T>
    T* AsType() const { return reinterpret_cast<T*>((*this)->GetPtr()); }
};

namespace detail
{

class OCLUAPI _oclMem : public NonCopyable, public NonMovable
{
    friend class _oclKernel;
    friend class _oclContext;
    template<typename> friend class _oclGLMem;
private:
    std::atomic_flag LockObj = ATOMIC_FLAG_INIT;
    oclMapPtr MapPtr;
    std::weak_ptr<_oclMapPtr> TmpPtr;
protected:
    const oclContext Context;
    const cl_mem MemID;
    const MemFlag Flag;
    _oclMem(const oclContext& ctx, cl_mem mem, const MemFlag flag);
    virtual void* MapObject(const oclCmdQue& que, const MapFlag mapFlag) = 0;
    oclMapPtr TryGetMap() const;
public:
    virtual ~_oclMem();
    oclMapPtr Map(const oclCmdQue& que, const MapFlag mapFlag);
};


class OCLUAPI GLResLocker : public NonCopyable, public NonMovable
{
private:
    const oclCmdQue Queue;
    const cl_mem Mem;
protected:
    GLResLocker(const oclCmdQue& que, const cl_mem mem);
public:
    virtual ~GLResLocker();
};

template<typename T>
class _oclGLMem : public GLResLocker
{
    template<typename> friend class oclGLMem;
private:
    T Obj;
public:
    _oclGLMem(const oclCmdQue& que, const T& obj) : GLResLocker(que, obj->MemID), Obj(obj) { }
    virtual ~_oclGLMem() override {}
    T GetObj() const { return Obj; }
};

class OCLUAPI GLInterOP
{
public:
    static cl_mem CreateMemFromGLBuf(const oclContext ctx, MemFlag flag, const oglu::oglBuffer& bufId);
    static cl_mem CreateMemFromGLTex(const oclContext ctx, MemFlag flag, const oglu::oglTexBase& tex);
};


}
template<typename T>
class oclGLMem : public std::shared_ptr<detail::_oclGLMem<T>>
{
public:
    constexpr oclGLMem() {}
    oclGLMem(const std::shared_ptr<detail::_oclGLMem<T>>& ptr) : std::shared_ptr<detail::_oclGLMem<T>>(ptr) {}
    operator const T&() const { return (*this)->GetObj(); }
    T Get() const { return (*this)->GetObj(); }
};

namespace detail
{

template<typename T>
class _oclGLObject
{
protected:
    const Wrapper<T> CLObject;
    template<typename... Args>
    _oclGLObject(Args&&... args) : CLObject(new T(std::forward<Args>(args)...)) { }
public:
    oclGLMem<Wrapper<T>> Lock(const oclCmdQue& que) const
    {
        return std::make_shared<detail::_oclGLMem<Wrapper<T>>>(que, CLObject);
    }
};

}

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif