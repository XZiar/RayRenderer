#include "DxPch.h"
#include "DxBuffer.h"


namespace dxu
{
MAKE_ENABLER_IMPL(DxBuffer_);


DxBuffer_::DxMapPtr_::DxMapPtr_(const DxBuffer_* res, size_t offset, size_t size)
    : Resource(res->Resource)
{
    void* ptr = nullptr;
    D3D12_RANGE range{ offset, size };
    THROW_HR(Resource->Map(0, &range, &ptr), u"Failed to map directx resource");
    MemSpace = { reinterpret_cast<std::byte*>(ptr) + offset, size };
}
DxBuffer_::DxMapPtr_::~DxMapPtr_()
{
    Unmap();
}
void DxBuffer_::DxMapPtr_::Unmap()
{
    if (Resource)
    {
        Resource->Unmap(0, nullptr);
        Resource = nullptr;
    }
}

class DxBuffer_::DxMapPtr2_ : public DxBuffer_::DxMapPtr_
{
private:
    MAKE_ENABLER();
    DxCmdQue CmdQue;
    DxCmdList CopyList;
    const DxBuffer_& RealResource;
    DxBuffer MappedResource;
    uint64_t Offset;
    MapFlags MapType;
    DxMapPtr2_(DxCmdQue cmdQue, DxCmdList list, const DxBuffer_* real, DxBuffer mapped, MapFlags flag, size_t offset, size_t size) :
        DxMapPtr_(mapped.get(), 0, size), CmdQue(std::move(cmdQue)), CopyList(std::move(list)),
        RealResource(*real), MappedResource(std::move(mapped)), 
        Offset(offset), MapType(flag)
    { }
public:
    ~DxMapPtr2_() override
    {
        if (HAS_FIELD(MapType, MapFlags::WriteOnly))
        {
            MappedResource->TransitState(CopyList, ResourceState::CopySrc);
            RealResource.TransitState(CopyList, ResourceState::CopyDst);
            CopyList->FlushResourceState();
            RealResource.CopyRegionFrom(CopyList, Offset, *MappedResource, 0, MemSpace.size());
            CmdQue->ExecuteAnyList(CopyList)->WaitFinish();
        }
        Unmap();
    }
    static std::shared_ptr<DxBuffer_::DxMapPtr_> Create(const DxBuffer_* res, const DxCmdQue& que, MapFlags flag, size_t offset, size_t size)
    {
        auto mapped = DxBuffer_::Create(res->Device,
            { flag == MapFlags::WriteOnly ? CPUPageProps::WriteCombine : CPUPageProps::WriteBack , MemPrefer::PreferCPU },
            HeapFlags::Empty, size, ResourceFlags::Empty);
        auto cmdList = que->CreateList();
        cmdList->SetName(FMTSTR(u"CopyList for mapping [{}]", res->GetName()));
        if (HAS_FIELD(flag, MapFlags::ReadOnly))
        {
            res->TransitState(cmdList, ResourceState::CopySrc);
            mapped->TransitState(cmdList, ResourceState::CopyDst, true);
            cmdList->FlushResourceState();
            mapped->CopyRegionFrom(cmdList, 0, *res, offset, size);
            que->ExecuteAnyList(cmdList)->WaitFinish();
            cmdList->Reset(false);
        }
        return MAKE_ENABLER_SHARED(DxMapPtr2_, (que, cmdList, res, mapped, flag, offset, size));
    }
};
MAKE_ENABLER_IMPL(DxBuffer_::DxMapPtr2_);

class DxBufMapPtr::DxMemInfo : public common::AlignedBuffer::ExternBufInfo
{
    std::shared_ptr<const DxBuffer_::DxMapPtr_> Ptr;
    [[nodiscard]] size_t GetSize() const noexcept override
    {
        return Ptr->MemSpace.size();
    }
    [[nodiscard]] std::byte* GetPtr() const noexcept override
    {
        return Ptr->MemSpace.data();
    }
public:
    DxMemInfo(std::shared_ptr<const DxBuffer_::DxMapPtr_> ptr) noexcept : Ptr(std::move(ptr)) { }
    ~DxMemInfo() override {}
};
common::AlignedBuffer DxBufMapPtr::AsBuffer() const noexcept
{
    return common::AlignedBuffer::CreateBuffer(std::make_unique<DxMemInfo>(Ptr));
}


detail::ResourceDesc DxBuffer_::BufferDesc(ResourceFlags rFlag, size_t size) noexcept
{
    return 
    {
        D3D12_RESOURCE_DIMENSION_BUFFER,    // Dimension
        0,                                  // Alignment
        static_cast<uint64_t>(size),        // Width
        1,                                  // Height
        1,                                  // DepthOrArraySize
        1,                                  // MipLevels
        DXGI_FORMAT_UNKNOWN,                // Format
        {1, 0},                             // SampleDesc
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        static_cast<D3D12_RESOURCE_FLAGS>(rFlag)
    };
}
DxBuffer_::DxBuffer_(DxDevice device, HeapProps heapProps, HeapFlags hFlag, ResourceFlags rFlag, size_t size)
    : DxResource_(device, heapProps, hFlag, BufferDesc(rFlag, size), ResourceState::Common), Size(size)
{ }
DxBuffer_::~DxBuffer_()
{ }

bool DxBuffer_::IsBufOrSATex() const noexcept
{
    return true;
}

DxBufMapPtr DxBuffer_::Map(size_t offset, size_t size)
{
    if (HeapInfo.CPUPage == CPUPageProps::NotAvailable || HeapInfo.Memory == MemPrefer::PreferGPU)
        COMMON_THROWEX(DxException, u"Cannot map resource with CPUPageProps = NotAvaliable.\n")
        .Attach("heapinfo", HeapInfo);
    return DxBufMapPtr(GetSelf(), std::make_shared<DxMapPtr_>(this, offset, size));
}

DxBufMapPtr DxBuffer_::Map(const DxCmdQue& que, MapFlags flag, size_t offset, size_t size)
{
    if (HeapInfo.CPUPage != CPUPageProps::NotAvailable && HeapInfo.Memory != MemPrefer::PreferGPU)
        return Map(offset, size);
    return DxBufMapPtr(GetSelf(), DxMapPtr2_::Create(this, que, flag, offset, size));
}

common::PromiseResult<void> DxBuffer_::ReadSpan_(const DxCmdQue& que, common::span<std::byte> buf, const size_t offset)
{
    if (HeapInfo.CPUPage != CPUPageProps::NotAvailable && HeapInfo.Memory != MemPrefer::PreferGPU)
    {
        const auto mapped = Map(offset, buf.size());
        const auto src = mapped.Get();
        memcpy_s(buf.data(), buf.size(), src.data(), src.size());
        return common::FinishedResult<void>::Get();
    }
    else
    {
        auto mapped = DxBuffer_::Create(Device, HeapType::Readback,
            HeapFlags::Empty, buf.size(), ResourceFlags::Empty);
        auto cmdList = que->CreateList();
        cmdList->SetName(FMTSTR(u"CopyList for readback [{}]", GetName()));
        TransitState(cmdList, ResourceState::CopySrc);
        mapped->TransitState(cmdList, ResourceState::CopyDst, true); // should be skipped
        cmdList->FlushResourceState();
        mapped->CopyRegionFrom(cmdList, 0, *this, offset, buf.size());
        return common::StagedResult::TwoStage(
            que->ExecuteAnyList(cmdList),
            [=]() mutable
            {
                cmdList.reset();
                const auto mapptr = mapped->Map(offset, buf.size());
                const auto src = mapptr.Get();
                memcpy_s(buf.data(), buf.size(), src.data(), src.size());
            });
    }
}

common::PromiseResult<void> DxBuffer_::WriteSpan_(const DxCmdQue& que, common::span<const std::byte> buf, const size_t offset)
{
    if (HeapInfo.CPUPage != CPUPageProps::NotAvailable && HeapInfo.Memory != MemPrefer::PreferGPU)
    {
        const auto mapped = Map(offset, buf.size());
        const auto src = mapped.Get();
        memcpy_s(src.data(), src.size(), buf.data(), buf.size());
        return common::FinishedResult<void>::Get();
    }
    else
    {
        auto tmpBuf = DxBuffer_::Create(Device, HeapType::Upload,
            HeapFlags::Empty, buf.size(), ResourceFlags::Empty);
        tmpBuf->SetName(FMTSTR(u"CopyList for write [{}]", GetName()));
        {
            const auto mapptr = tmpBuf->Map(0, buf.size());
            const auto src = mapptr.Get();
            memcpy_s(src.data(), src.size(), buf.data(), buf.size());
        }
        auto cmdList = que->CreateList();
        cmdList->SetName(FMTSTR(u"CopyList for write [{}]", GetName()));
        TransitState(cmdList, ResourceState::CopyDst);
        cmdList->FlushResourceState();
        CopyRegionFrom(cmdList, offset, *tmpBuf, 0, buf.size());
        const auto pms = que->ExecuteAnyList(cmdList);
        pms->OnComplete([=]() mutable
            {
                cmdList.reset();
                tmpBuf.reset();
            });
        return pms;
    }
}

common::PromiseResult<common::AlignedBuffer> DxBuffer_::Read_(const DxCmdQue& que, const size_t size, const size_t offset)
{
    common::AlignedBuffer buf(size);
    return common::StagedResult::Convert(
        ReadSpan_(que, buf.AsSpan(), offset),
        [buf_ = buf.CreateSubBuffer()]() { return buf_.CreateSubBuffer(); });
}

DxBuffer DxBuffer_::Create(DxDevice device, HeapProps heapProps, HeapFlags hFlag, const size_t size, ResourceFlags rFlag)
{
    return MAKE_ENABLER_SHARED(DxBuffer_, (device, heapProps, hFlag, rFlag, size));
}


}
