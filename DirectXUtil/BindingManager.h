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


class DXUAPI DxBindingManager
{
protected:
    template<typename T>
    struct BindItem
    {
        T Item;
        uint16_t DescOffset = 0;
        uint16_t Reference = 0;
    };
    struct DescHeapProxy;
    std::vector<BindItem<DxBuffer_::BufferView>> BindBuffer;
    std::vector<BindItem<DxBuffer_::BufferView>> BindTexture;
    std::vector<BindItem<DxBuffer_::BufferView>> BindSampler;
public:
    virtual uint16_t SetBuf(DxBuffer_::BufferView bufview, uint16_t offset) = 0;
    PtrProxy<DescHeapProxy> GetCSUHeap(const DxDevice device) const;
};

class DXUAPI DxSharedBindingManager : public DxBindingManager
{
public:
    uint16_t SetBuf(DxBuffer_::BufferView bufview, uint16_t offset) override;
};

class DXUAPI DxUniqueBindingManager : public DxBindingManager
{
public:
    DxUniqueBindingManager(size_t bufCount, size_t texCount, size_t samplerCount);
    uint16_t SetBuf(DxBuffer_::BufferView bufview, uint16_t offset) override;
};


}