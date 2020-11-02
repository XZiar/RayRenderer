#pragma once
#include "DxRely.h"
#include "DxResource.h"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DxBufMapPtr;
class DxBuffer_;
using DxBuffer = std::shared_ptr<DxBuffer_>;


class DXUAPI DxBuffer_ : public DxResource_
{
    friend class DxBufMapPtr;
private:
    class DXUAPI COMMON_EMPTY_BASES DxMapPtr_ : public common::NonCopyable, public common::NonMovable
    {
        friend class DxBufMapPtr;
    protected:
        ResProxy* Resource;
        common::span<std::byte> MemSpace;
        void Unmap();
    public:
        DxMapPtr_(DxBuffer_* res, size_t offset, size_t size);
        virtual ~DxMapPtr_();
    };
    class DxMapPtr2_;
protected:
    MAKE_ENABLER();
    static ResDesc BufferDesc(ResourceFlags rFlags, size_t size) noexcept;
    DxBuffer_(DxDevice device, HeapProps heapProps, HeapFlags hFlag, ResourceFlags rFlag, size_t size);
public:
    const size_t Size;
    ~DxBuffer_() override;
    [[nodiscard]] DxBufMapPtr Map(size_t offset, size_t size);
    [[nodiscard]] DxBufMapPtr Map(const DxCmdQue& que, MapFlags flag, size_t offset, size_t size);
    //[[nodiscard]] DxBufMapPtr MapAsync(const DxCmdQue& que, MapFlags flag, size_t offset, size_t size);

    [[nodiscard]] static DxBuffer Create(DxDevice device, HeapProps heapProps, HeapFlags hFlag, const size_t size, ResourceFlags rFlag = ResourceFlags::AllowUnorderAccess);
};


class DXUAPI DxBufMapPtr
{
    friend class DxBuffer_;
private:
    class DxMemInfo;
    std::shared_ptr<const DxBuffer_::DxMapPtr_> Ptr;
    DxBuffer Res;
    DxBufMapPtr(DxBuffer res, std::shared_ptr<DxBuffer_::DxMapPtr_> ptr) noexcept
        : Ptr(std::move(ptr)), Res(std::move(res)) { }
public:
    constexpr DxBufMapPtr() noexcept {}
    [[nodiscard]] common::span<std::byte> Get() const noexcept { return Ptr->MemSpace; }
    template<typename T>
    [[nodiscard]] common::span<T> AsType() const noexcept { return common::span<T>(reinterpret_cast<T*>(Get().data()), Get().size() / sizeof(T)); }
    [[nodiscard]] common::AlignedBuffer AsBuffer() const noexcept;
};


}