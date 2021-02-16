#pragma once
#include "DxRely.h"
#include "DxDevice.h"
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
    using ItTypeBuf = common::container::IndirectIterator<const DxProgram_, BoundedResWrapper, &DxProgram_::GetBufSlot>;
    using ItTypeTex = common::container::IndirectIterator<const DxProgram_, BoundedResWrapper, &DxProgram_::GetTexSlot>;
    friend ItTypeBuf;
    friend ItTypeTex;
    class BufSlotList
    {
        friend class DxProgram_;
        const DxProgram_* Host;
        constexpr BufSlotList(const DxProgram_* host) noexcept : Host(host) { }
    public:
        constexpr ItTypeBuf begin() const noexcept { return { Host, 0 }; }
        constexpr ItTypeBuf end()   const noexcept { return { Host, Host->BufferSlots.size() }; }
    };
    class TexSlotList
    {
        friend class DxProgram_;
        const DxProgram_* Host;
        constexpr TexSlotList(const DxProgram_* host) noexcept : Host(host) { }
    public:
        constexpr ItTypeBuf begin() const noexcept { return { Host, 0 }; }
        constexpr ItTypeBuf end()   const noexcept { return { Host, Host->TextureSlots.size() }; }
    };
    struct RootSignature;
    //const PtrProxy<DxDevice_::DeviceProxy>& GetDevice() const noexcept;
protected:
    struct BoundedResource
    {
        uint64_t Hash;
        common::StringPiece<char> Name;
        const DxShader_::BindResourceDetail* Detail;
        uint16_t DescOffset;
        BoundedResourceType Type;
    };
    class DXUAPI DxProgramPrepareBase
    {
    private:
    protected:
        const DxProgram Program;
        PtrProxy<RootSignature> RootSig;
        DxUniqueBindingManager BindMan;
        std::vector<std::pair<const BoundedResource*, uint16_t>> Bindings;
        DxProgramPrepareBase(DxProgram program);
        bool SetBuf(common::str::HashedStrView<char> name, const DxBuffer_::BufferView& bufview);
        void FinishBinding(const DxCmdList& cmdlist);
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
        DxProgramPrepare& SetBuf(std::string_view name, const DxBuffer_::BufferView& bufview)
        {
            DxProgramPrepareBase::SetBuf(name, bufview);
            return *this;
        }
        auto Finish(const DxCmdList& prevList = {})
        {
            return T::FinishPrepare(*this, prevList);
        }
    };
    const DxDevice Device;
    common::StringPool<char> StrPool;
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
}; 


class DXUAPI DxComputeProgram_ : public DxProgram_
{
    friend DxProgramPrepare<DxComputeProgram_>;
private:
    class DxComputeCall
    {
        const DxComputeProgram Program;
        const DxComputeCmdList CmdList;
        PtrProxy<DxProgram_::RootSignature> RootSig;
    public:
        DxComputeCall(DxProgramPrepare<DxComputeProgram_>& prepare, const DxCmdList& prevList);
    };
    template<typename U>
    static void CheckListType() noexcept { DXU_CMD_CHECK(U, Compute, List); }
    static forceinline DxComputeCall FinishPrepare(DxProgramPrepare<DxComputeProgram_>& prepare, const DxCmdList& prevList)
    {
        return { prepare, prevList };
    }
    DxShader Shader;
public:
    DxComputeProgram_(DxShader shader);
    ~DxComputeProgram_() override;
    [[nodiscard]] DxProgramPrepare<DxComputeProgram_> Prepare() const;

    [[nodiscard]] static DxComputeProgram Create(DxDevice dev, std::string str, const DxShaderConfig& config);
};


}