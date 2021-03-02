#pragma once
#include "DxRely.h"
#include "DxDevice.h"
#include "DxCmdQue.h"
#include "DxBuffer.h"
#include "DxShader.h"
#include "BindingManager.h"
#include "common/CLikeConfig.hpp"
#include "common/StringPool.hpp"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DxProgram_;
using DxProgram = std::shared_ptr<const DxProgram_>;
class DxDrawProgram_;
using DxDrawProgram = std::shared_ptr<const DxDrawProgram_>;
class DxComputeProgram_;
using DxComputeProgram = std::shared_ptr<const DxComputeProgram_>;


class DXUAPI DxProgram_ : public std::enable_shared_from_this<DxProgram_>
{
    friend DxDrawProgram_;
    friend DxComputeProgram_;
private:
    struct BoundedResWrapper
    {
        std::string_view Name;
        uint16_t Space;
        uint16_t BindReg;
        uint16_t Count;
        uint16_t DescOffset;
        BoundedResourceType Type;
    };
    BoundedResWrapper GetBufSlot(const size_t idx) const noexcept;
    BoundedResWrapper GetTexSlot(const size_t idx) const noexcept;
    COMMON_EASY_CONST_ITER(BufSlotList, DxProgram_, BoundedResWrapper, GetBufSlot, 0, Host->BufferSlots.size());
    COMMON_EASY_CONST_ITER(TexSlotList, DxProgram_, BoundedResWrapper, GetTexSlot, 0, Host->TextureSlots.size());
    //const PtrProxy<DxDevice_::DeviceProxy>& GetDevice() const noexcept;
protected:
    struct BoundedResource
    {
        common::str::HashedStrView<char> HashedName;
        const detail::BindResourceDetail* Detail;
        uint16_t DescOffset;
        BoundedResourceType Type;
    };
    const DxDevice Device;
    std::vector<BoundedResource> BufferSlots;
    std::vector<BoundedResource> TextureSlots;
    std::vector<BoundedResource> SamplerSlots;
    DxProgram_(DxDevice dev);
    void Initialize(common::span<const DxShader> shaders);
    const BoundedResource* GetSlot(const std::vector<BoundedResource>& container, common::str::HashedStrView<char> name) const;
public:
    virtual ~DxProgram_();
    constexpr BufSlotList BufSlots() const noexcept { return this; }
    constexpr TexSlotList TexSlots() const noexcept { return this; }
protected:
    class DXUAPI DxProgramPrepareBase
    {
        friend class DxProgram_;
    private:
        struct BindRecord
        {
            const BoundedResource* Slot;
            const DxResource_* Resource;
            uint16_t Offset;
        };
    protected:
        const DxProgram Program;
        std::shared_ptr<DxBindingManager> BindMan;
        std::vector<BindRecord> Bindings;
        DxProgramPrepareBase(DxProgram program);
        bool SetBuf(common::str::HashedStrView<char> name, const DxBuffer_::BufferView<>& bufview);
    public:
        ~DxProgramPrepareBase();
    };
    template<typename T>
    class DxProgramPrepare : public DxProgramPrepareBase
    {
        static_assert(std::is_base_of_v<DxProgram_, T>);
        friend T;
    private:
        DxProgramPrepare(std::shared_ptr<const T> program) : DxProgramPrepareBase(std::move(program)) {}
        std::shared_ptr<const T> GetProgram() const noexcept { return std::static_pointer_cast<const T>(Program); }
    public:
        DxProgramPrepare& SetBuf(std::string_view name, const DxBuffer_::BufferView<>& bufview)
        {
            DxProgramPrepareBase::SetBuf(name, bufview);
            return *this;
        }
        auto Finish()
        {
            return T::FinishPrepare(*this);
        }
    };
    class DXUAPI DxProgramCall
    {
    private:
        struct ResStateRecord
        {
            const DxResource_* Resource;
            ResourceState State;
        };
    protected:
        static const PtrProxy<detail::CmdList>& GetCmdList(const DxCmdList& cmdlist) noexcept
        {
            return cmdlist->CmdList;
        }
        DxDevice Device;
        std::shared_ptr<DxBindingManager> BindMan;
        std::vector<ResStateRecord> ResStates;
        PtrProxy<detail::RootSignature> RootSig;
        PtrProxy<detail::PipelineState> PSO;
        PtrProxy<detail::DescHeap> CSUDescHeap;
        PtrProxy<detail::DescHeap> SamplerHeap;
        DxProgramCall(DxProgramPrepareBase& prepare);
        void SetPSOAndHeaps(const DxCmdList& cmdlist) const;
        void PutResourceBarrier(const DxCmdList& cmdlist) const;
        const PtrProxy<detail::Device>& GetDevice() const noexcept
        {
            return Device->Device;
        }
    public:
        ~DxProgramCall();
    };
}; 


class DXUAPI DxComputeProgram_ : public DxProgram_
{
    friend DxProgramPrepare<DxComputeProgram_>;
private:
    class DXUAPI DxComputeCall : protected DxProgram_::DxProgramCall
    {
        const DxComputeProgram Program;
    private:
        void ExecuteIn(const DxCmdList& cmdlist, const std::array<uint32_t, 3>& threadgroups) const;
        common::PromiseResult<void> ExecuteIn(const DxCmdQue& que, const std::array<uint32_t, 3>& threadgroups) const;
    public:
        DxComputeCall(DxProgramPrepare<DxComputeProgram_>& prepare);
        template<typename U>
        auto Execute(const std::shared_ptr<U>& cmd, const std::array<uint32_t, 3>& threadgroups) const
        {
            static_assert(HAS_FIELD(U::CmdCaps, DxCmdList_::Capability::Compute), "need a Compute List/Que");
            return ExecuteIn(cmd, threadgroups);
        }
        template<typename U>
        common::PromiseResult<void> Execute(const std::shared_ptr<U>& cmd, const std::array<uint32_t, 3>& wgSize, const std::array<uint32_t, 3>& lcSize) const
        {
            std::array<uint32_t, 3> tgSize = { 0 };
            tgSize[0] = (wgSize[0] + lcSize[0] - 1) / wgSize[0];
            tgSize[1] = (wgSize[1] + lcSize[1] - 1) / wgSize[1];
            tgSize[2] = (wgSize[2] + lcSize[2] - 1) / wgSize[2];
            return ExecuteIn(cmd, tgSize);
        }
    };
    template<typename U>
    static void CheckListType() noexcept { DXU_CMD_CHECK(U, Compute, List); }
    static forceinline DxComputeCall FinishPrepare(DxProgramPrepare<DxComputeProgram_>& prepare)
    {
        return { prepare };
    }
    DxShader Shader;
public:
    DxComputeProgram_(DxShader shader);
    ~DxComputeProgram_() override;
    const std::array<uint32_t, 3>& GetThreadGroupSize() const { return Shader->ThreadGroupSize; }
    [[nodiscard]] DxProgramPrepare<DxComputeProgram_> Prepare() const;

    [[nodiscard]] static DxComputeProgram Create(DxDevice dev, std::string str, const DxShaderConfig& config);
};


}