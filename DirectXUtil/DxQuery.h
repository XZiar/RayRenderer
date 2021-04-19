#pragma once
#include "DxRely.h"
#include "DxDevice.h"


#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DxBuffer_;
class DxCmdList_;
class DxCmdQue_;
class DxQueryProvider;
class DxQueryResolver;


class DxQueryTokenBase
{
    friend DxQueryProvider;
    friend DxQueryResolver;
private:
    const DxQueryProvider* Provider;
    uint32_t BlockIdx, BlockOffset;
    DxQueryTokenBase(const DxQueryProvider* provider, uint32_t idx, uint32_t offset) :
        Provider(provider), BlockIdx(idx), BlockOffset(offset)
    { }
};
class DxTimestampToken : protected DxQueryTokenBase
{
    friend DxQueryProvider;
    friend DxQueryResolver;
    using DxQueryTokenBase::DxQueryTokenBase;
};


class DXUAPI DxQueryProvider : public std::enable_shared_from_this<DxQueryProvider>
{
    friend DxCmdList_;
    friend DxCmdQue_;
    friend DxQueryResolver;
private:
    MAKE_ENABLER();
    struct ResolveRecord
    {
        std::shared_ptr<const DxQueryProvider> Provider;
        std::shared_ptr<DxBuffer_> Result;
        std::shared_ptr<DxCmdList_> ResolveList;
        double TimestampRatio;
    };
    struct QueryBlock
    {
        detail::QueryHeap* Heap;
        uint32_t Size;
        uint32_t Usage;
        uint32_t Offset;
        uint8_t Type;
        constexpr QueryBlock(uint32_t size, uint8_t type) : Heap(nullptr), Size(size), Usage(0), Offset(0), Type(type) {}
    };
    boost::container::small_vector<QueryBlock, 8> Heaps;
    DxDevice Device;
    uint32_t TotalSize;
    DxQueryProvider(DxDevice dev);
    QueryBlock* GetQueryBlock(uint8_t type, uint32_t initSize = 32);
public:
    ~DxQueryProvider();
    [[nodiscard]] DxTimestampToken AllocateTimeQuery(const DxCmdList_& cmdList);
    void Finish();
    [[nodiscard]] ResolveRecord GenerateResolve(const DxCmdQue_& que) const;
};


class DXUAPI DxQueryResolver
{
    friend DxCmdQue_;
private:
    std::shared_ptr<const DxQueryProvider> Provider;
    std::unique_ptr<uint64_t[]> Result;
    double TimestampRatio;
    DxQueryResolver() noexcept;
    DxQueryResolver(DxQueryProvider::ResolveRecord& record);
public:
    [[nodiscard]] uint64_t ResolveQuery(const DxTimestampToken& token) const;
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return static_cast<bool>(Provider);
    }
};


}

#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif
