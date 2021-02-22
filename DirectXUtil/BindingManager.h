#pragma once
#include "DxRely.h"
#include "DxDevice.h"
#include "DxBuffer.h"
#include "common/CLikeConfig.hpp"
#include "common/StringPool.hpp"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DxProgram_;


class DXUAPI DxBindingManager
{
    friend DxProgram_;
protected:
    template<typename T>
    struct BindItem
    {
        T Item;
        uint16_t DescOffset = 0;
        uint16_t Reference = 0;
    };
    std::vector<BindItem<DxBuffer_::BufferView<DxBufferConst>>> BindBuffer;
    std::vector<BindItem<DxBuffer_::BufferView<DxBufferConst>>> BindTexture;
    std::vector<BindItem<DxBuffer_::BufferView<DxBufferConst>>> BindSampler;
public:
    virtual uint16_t SetBuf(DxBuffer_::BufferView<DxBufferConst> bufview, uint16_t offset) = 0;
    PtrProxy<detail::DescHeap> GetCSUHeap(const DxDevice device) const;
};

class DXUAPI DxSharedBindingManager : public DxBindingManager
{
public:
    uint16_t SetBuf(DxBuffer_::BufferView<DxBufferConst> bufview, uint16_t offset) override;
};

class DXUAPI DxUniqueBindingManager : public DxBindingManager
{
public:
    DxUniqueBindingManager(size_t bufCount, size_t texCount, size_t samplerCount);
    uint16_t SetBuf(DxBuffer_::BufferView<DxBufferConst> bufview, uint16_t offset) override;
};


}