#include "dxPch.h"
#include "dxBuffer.h"
#include "ProxyStruct.h"


namespace dxu
{
MAKE_ENABLER_IMPL(DXBuffer_);


DXResource_::ResDesc DXBuffer_::BufferDesc(ResourceFlags rFlag, size_t size) noexcept
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
DXBuffer_::DXBuffer_(DXDevice device, HeapProps heapProps, HeapFlags hFlag, ResourceFlags rFlag, size_t size)
    : DXResource_(device, heapProps, hFlag, BufferDesc(rFlag, size), ResourceState::Common), Size(size)
{ }
DXBuffer_::~DXBuffer_()
{ }

DXBuffer DXBuffer_::Create(DXDevice device, HeapProps heapProps, HeapFlags hFlag, const size_t size, ResourceFlags rFlag)
{
    return MAKE_ENABLER_SHARED(DXBuffer_, (device, heapProps, hFlag, rFlag, size));
}


}
