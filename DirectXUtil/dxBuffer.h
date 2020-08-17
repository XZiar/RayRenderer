#pragma once
#include "dxRely.h"
#include "dxResource.h"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DXBuffer_;
using DXBuffer = std::shared_ptr<DXBuffer_>;


class DXUAPI DXBuffer_ : public DXResource_
{
protected:
    MAKE_ENABLER();
    static ResDesc BufferDesc(ResourceFlags rFlags, size_t size) noexcept;
    DXBuffer_(DXDevice device, HeapProps heapProps, HeapFlags hFlag, ResourceFlags rFlag, size_t size);
public:
    const size_t Size;
    ~DXBuffer_() override;
    [[nodiscard]] static DXBuffer Create(DXDevice device, HeapProps heapProps, HeapFlags hFlag, const size_t size, ResourceFlags rFlag = ResourceFlags::AllowUnorderAccess);
};


}