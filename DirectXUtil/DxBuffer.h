#pragma once
#include "DxRely.h"
#include "DxResource.h"

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DxBufMapPtr;
class DxBuffer_;
using DxBuffer = std::shared_ptr<DxBuffer_>;
using DxBufferConst = std::shared_ptr<const DxBuffer_>;


class DXUAPI DxBuffer_ : public DxResource_
{
    friend class DxBufMapPtr;
private:
    class DXUAPI COMMON_EMPTY_BASES DxMapPtr_ : public common::NonCopyable, public common::NonMovable
    {
        friend class DxBufMapPtr;
    protected:
        detail::Resource* Resource;
        common::span<std::byte> MemSpace;
        void Unmap();
    public:
        DxMapPtr_(const DxBuffer_* res, size_t offset, size_t size);
        virtual ~DxMapPtr_();
    };
    class DxMapPtr2_;
    bool IsBufOrSATex() const noexcept final;
protected:
    MAKE_ENABLER();
    static detail::ResourceDesc BufferDesc(ResourceFlags rFlags, size_t size) noexcept;
    DxBuffer_(DxDevice device, HeapProps heapProps, HeapFlags hFlag, ResourceFlags rFlag, size_t size);
    DxBufferConst GetSelf() const noexcept
    {
        return std::static_pointer_cast<const DxBuffer_>(shared_from_this());
    }
    DxBuffer GetSelf() noexcept
    {
        return std::static_pointer_cast<DxBuffer_>(shared_from_this());
    }
    void CopyFrom_(const DxCmdList& list, const uint64_t offset, const DxBuffer_& src, 
        const uint64_t srcOffset, const uint64_t numBytes, bool dstNew, bool srcNew) const;
    [[nodiscard]] common::PromiseResult<void> ReadSpan_(const DxCmdQue& que, common::span<std::byte> buf, const size_t offset);
    [[nodiscard]] common::PromiseResult<void> WriteSpan_(const DxCmdQue& que, common::span<const std::byte> buf, const size_t offset);
    [[nodiscard]] common::PromiseResult<common::AlignedBuffer> Read_(const DxCmdQue& que, const size_t size, const size_t offset);
public:
    const size_t Size;
    ~DxBuffer_() override;
    [[nodiscard]] DxBufMapPtr Map(size_t offset, size_t size);
    [[nodiscard]] DxBufMapPtr Map(const DxCmdQue& que, MapFlags flag, size_t offset, size_t size);

    template<typename U>
    [[nodiscard]] common::PromiseResult<void> ReadSpan(const std::shared_ptr<U>& que, common::span<std::byte> buf, const size_t offset = 0)
    {
        DXU_CMD_CHECK(U, Copy, Que);
        return ReadSpan_(que, buf, offset);
    }
    template<typename T, typename U>
    [[nodiscard]] common::PromiseResult<void> ReadSpan(const std::shared_ptr<U>& que, T& buf, const size_t offset = 0)
    {
        DXU_CMD_CHECK(U, Copy, Que);
        return ReadSpan_(que, common::as_writable_bytes(common::to_span(buf)), offset);
    }
    template<typename T, typename U>
    [[nodiscard]] common::PromiseResult<void> ReadInto(const std::shared_ptr<U>& que, T& buf, const size_t offset = 0)
    {
        DXU_CMD_CHECK(U, Copy, Que);
        return ReadSpan_(que, common::span<std::byte>(reinterpret_cast<std::byte*>(&buf), sizeof(buf)), offset);
    }
    template<typename U>
    [[nodiscard]] common::PromiseResult<common::AlignedBuffer> Read(const std::shared_ptr<U>& que, const size_t size, const size_t offset = 0)
    {
        DXU_CMD_CHECK(U, Copy, Que);
        return Read_(que, size, offset);
    }

    template<typename T, typename U>
    [[nodiscard]] common::PromiseResult<void> WriteSpan(const std::shared_ptr<U>& que, const T& buf, const size_t offset = 0)
    {
        DXU_CMD_CHECK(U, Copy, Que);
        return WriteSpan_(que, common::as_bytes(common::to_span(buf)), offset);
    }
    template<typename T, typename U>
    [[nodiscard]] common::PromiseResult<void> Write(const std::shared_ptr<U>& que, const T& buf, const size_t offset = 0)
    {
        DXU_CMD_CHECK(U, Copy, Que);
        return WriteSpan_(que, common::span<const std::byte>(reinterpret_cast<const std::byte*>(&buf), sizeof(buf)), offset);
    }
    void CopyFrom(const DxCmdList& list, const uint64_t offset,
        const DxBuffer& src, const uint64_t srcOffset, const uint64_t numBytes) const
    {
        CopyFrom_(list, offset, *src, srcOffset, numBytes, false, false);
    }

    template<typename T = const DxBuffer_*>
    struct BufferView
    {
        T Buffer;
        uint32_t Offset;
        uint32_t Count;
        uint32_t Stride;
        xziar::img::TextureFormat Format;
        BoundedResourceType Type;
        template<typename U>
        bool operator==(const BufferView<U>& other) const noexcept
        {
            if (Buffer != other.Buffer || Type != other.Type || Format != other.Format)
                return false;
            if (Offset != other.Offset || Count != other.Count || Stride != other.Stride)
                return false;
            return true;
        }
        BufferView<DxBufferConst> WithOwnership(BoundedResourceType type = BoundedResourceType::Others) const noexcept
        {
            if (type == BoundedResourceType::Others) 
                type = Type;
            return { Buffer->GetSelf(), Offset, Count, Stride, Format, type };
        }
    };
    BufferView<> CreateStructuredView(size_t unitSize, size_t count, size_t offset = 0) const noexcept;
    template<typename T>
    BufferView<> CreateStructuredView(size_t count, size_t offset = 0) const noexcept
    {
        return CreateStructuredView(sizeof(T), count, offset);
    }
    BufferView<> CreateRawView(size_t count, size_t offset = 0) const noexcept;
    BufferView<> CreateTypedView(xziar::img::TextureFormat format, size_t count, size_t offset = 0) const noexcept;
    BufferView<> CreateConstBufView(size_t size, size_t offset = 0) const noexcept;

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

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
