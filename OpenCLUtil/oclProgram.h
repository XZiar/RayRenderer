#pragma once
#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"
#include "oclCmdQue.h"
#include "oclBuffer.h"
#include "oclImage.h"
#include "oclKernelDebug.h"
#include "oclPromise.h"
#include "common/FileBase.hpp"
#include "common/CLikeConfig.hpp"
#include "common/StringPool.hpp"



#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclProgram_;
using oclProgram = std::shared_ptr<const oclProgram_>;
class oclKernel_;
using oclKernel = std::shared_ptr<const oclKernel_>;


struct WorkGroupInfo
{
    uint64_t LocalMemorySize;
    uint64_t PrivateMemorySize;
    uint64_t SpillMemSize;
    size_t WorkGroupSize;
    size_t CompiledWorkGroupSize[3];
    size_t PreferredWorkGroupSizeMultiple;
};

struct SubgroupInfo
{
    size_t SubgroupSize;
    size_t SubgroupCount;
    size_t CompiledSubgroupSize;
};

enum class KerArgType  : uint8_t { Any, Buffer, Image, Simple };
enum class KerArgSpace : uint8_t { Private, Global, Constant, Local };
enum class ImgAccess   : uint8_t { None, ReadOnly, WriteOnly, ReadWrite };
enum class KerArgFlag  : uint8_t { None = 0, Const = 0x1, Restrict = 0x2, Volatile = 0x4, Pipe = 0x8 };
MAKE_ENUM_BITFIELD(KerArgFlag)


struct OCLUAPI ArgFlags
{
    KerArgType  ArgType   = KerArgType::Any;
    KerArgSpace Space     = KerArgSpace::Private;
    ImgAccess   Access    = ImgAccess::None;
    KerArgFlag  Qualifier = KerArgFlag::None;
    [[nodiscard]] std::string_view GetArgTypeName() const noexcept;
    [[nodiscard]] std::string_view GetSpaceName() const noexcept;
    [[nodiscard]] std::string_view GetImgAccessName() const noexcept;
    [[nodiscard]] std::string_view GetQualifierName() const noexcept;
    [[nodiscard]] constexpr bool IsType(const KerArgType type) const noexcept 
    { return ArgType == type || ArgType == KerArgType::Any; }

    [[nodiscard]] static std::string_view ToCLString(const KerArgSpace space) noexcept;
    [[nodiscard]] static std::string_view ToCLString(const ImgAccess access) noexcept;
};

struct KernelArgInfo : public ArgFlags
{
    std::string_view Name;
    std::string_view Type;
};

class OCLUAPI KernelArgStore : public common::StringPool<char>
{
    friend class oclKernel_;
    friend class NLCLRuntime;
    friend struct KernelContext;
    friend struct KernelDebugExtension;
protected:
    struct ArgInfo : public ArgFlags
    {
        common::StringPiece<char> Name;
        common::StringPiece<char> Type;
    };
    std::vector<ArgInfo> ArgsInfo;
    uint32_t DebugBuffer;
    bool HasInfo, HasDebug;
    KernelArgStore(cl_kernel kernel, const KernelArgStore& reference);
    size_t GetSize() const noexcept { return ArgsInfo.size(); }
    const ArgInfo* GetArg(const size_t idx, const bool check = true) const;
    void AddArg(const KerArgType argType, const KerArgSpace space, const ImgAccess access, const KerArgFlag qualifier,
        const std::string_view name, const std::string_view type);
    KernelArgInfo GetArgInfo(const size_t idx) const noexcept;
    using ItType = common::container::IndirectIterator<KernelArgStore, KernelArgInfo, &KernelArgStore::GetArgInfo>;
    friend ItType;
public:
    KernelArgStore() : DebugBuffer(0), HasInfo(false), HasDebug(false) {}
    ItType begin() const noexcept { return { this, 0 }; }
    ItType end()   const noexcept { return { this, ArgsInfo.size() }; }
};


struct OCLUAPI CallResult
{
    std::shared_ptr<oclDebugManager> DebugManager;
    oclKernel Kernel;
    oclCmdQue Queue;
    oclBuffer InfoBuf;
    oclBuffer DebugBuf;

    template<typename F>
    void VisitData(F&& func) const
    {
        if (!DebugManager) return;
        const auto info = InfoBuf->Map(Queue, oclu::MapFlag::Read);
        const auto data = DebugBuf->Map(Queue, oclu::MapFlag::Read);
        const auto infoData = info.AsType<uint32_t>();
        const auto dbgSize = std::min(infoData[0] * sizeof(uint32_t), DebugBuf->Size);
        const auto dbgData = data.Get().subspan(0, dbgSize);
        
        DebugManager->VisitData(dbgData,
            [&](const uint32_t tid, const oclDebugInfoMan& infoMan, const oclDebugBlock& block, const auto& dat)
            {
                func(tid, infoMan, block, infoData, dat);
            });
    }
};

class OCLUAPI oclKernel_ : public common::NonCopyable
{
    friend class oclProgram_;
private:
    MAKE_ENABLER();

    const oclPlatform_& Plat;
    const oclProgram_& Prog;
    cl_kernel KernelID;
    mutable common::SpinLocker ArgLock;
    KernelArgStore ArgStore;
    uint32_t ReqDbgBufSize;
    oclKernel_(const oclPlatform_* plat, const oclProgram_* prog, cl_kernel kerID, std::string name, KernelArgStore&& argStore);
    template<size_t N>
    [[nodiscard]] constexpr static const size_t* CheckLocalSize(const std::array<size_t, N>& localsize)
    {
        for (size_t i = 0; i < N; ++i)
            if (localsize[i] != 0)
                return localsize.data();
        return nullptr;
    }

    struct OCLUAPI CallSiteInternal
    {
        oclKernel Kernel;
        common::SpinLocker::ScopeType KernelLock;

        CallSiteInternal(const oclKernel_* kernel);
        void SetArg(const uint32_t idx, const oclSubBuffer_& buf) const;
        void SetArg(const uint32_t idx, const oclImage_& img) const;
        void SetArg(const uint32_t idx, const void* dat, const size_t size) const;
        template<typename T>
        void SetSpanArg(const uint32_t idx, const T& dat) const
        {
            const auto space = common::as_bytes(common::to_span(dat));
            return SetArg(idx, space.data(), space.size());
        }
        template<typename T>
        void SetSimpleArg(const uint32_t idx, const T& dat) const
        {
            static_assert(!std::is_same_v<T, bool>, "boolean is implementation-defined and cannot be pass as kernel argument.");
            return SetArg(idx, &dat, sizeof(T));
        }
        [[nodiscard]] common::PromiseResult<CallResult> Run(const uint8_t dim, DependEvents depend,
            const oclCmdQue& que, const size_t* worksize, const size_t* workoffset, const size_t* localsize);
    };

    template<uint8_t N, typename... Args>
    class [[nodiscard]] KernelCallSite : protected CallSiteInternal
    {
        friend class oclKernel_;
    private:
        struct SizeN
        {
            size_t Data[N];
            constexpr SizeN() noexcept : Data{0}
            { }
            constexpr SizeN(const size_t(&data)[N]) noexcept : Data{0}
            {
                for (size_t i = 0; i < N; ++i)
                    Data[i] = data[i];
            }
            constexpr SizeN(const std::array<size_t, N>& data) noexcept : Data{0}
            {
                for (size_t i = 0; i < N; ++i)
                    Data[i] = data[i];
            }
            constexpr SizeN(const std::initializer_list<size_t>& data) noexcept : Data{0}
            {
                Expects(data.size() == N);
                size_t i = 0;
                for (const auto dat : data)
                    Data[i++] = dat;
            }
            constexpr const size_t* GetData(const bool zeroToNull = false) const noexcept
            {
                if (zeroToNull)
                {
                    for (size_t i = 0; i < N; ++i)
                        if (Data[i] != 0)
                            return Data;
                    return nullptr;
                }
                else
                    return Data;
            }
        };
        //using SizeN = std::array<size_t, N>;
        // clSetKernelArg does not hold parameter ownership, so need to manully hold it
        std::tuple<Args...> Paras;

        template<size_t Idx>
        forceinline void InitArg() const
        {
            using ArgType = common::remove_cvref_t<std::tuple_element_t<Idx, std::tuple<Args...>>>;
            if constexpr (std::is_convertible_v<ArgType, oclSubBuffer> || std::is_convertible_v<ArgType, oclImage>)
                SetArg(Idx, *std::get<Idx>(Paras));
            else if constexpr (common::CanToSpan<ArgType>)
                SetSpanArg(Idx, std::get<Idx>(Paras));
            else
                SetSimpleArg(Idx, std::get<Idx>(Paras));
            if constexpr (Idx != 0)
                InitArg<Idx - 1>();
        }

        KernelCallSite(const oclKernel_* kernel, Args&& ... args) : 
            CallSiteInternal(kernel), Paras(std::forward<Args>(args)...)
        {
            InitArg<sizeof...(Args) - 1>();
        }
    public:
        [[nodiscard]] common::PromiseResult<CallResult> operator()(const common::PromiseStub& pmss, const oclCmdQue& que, 
            const SizeN worksize, const SizeN localsize = {}, const SizeN workoffset = {})
        {
            return Run(N, pmss, que, worksize.GetData(), workoffset.GetData(), localsize.GetData(true));
        }
        [[nodiscard]] common::PromiseResult<CallResult> operator()(const oclCmdQue& que, 
            const SizeN worksize, const SizeN localsize = {}, const SizeN workoffset = {})
        {
            return Run(N, {}, que, worksize.GetData(), workoffset.GetData(), localsize.GetData(true));
        }
    };
public:
    std::string Name;
    ~oclKernel_();

    [[nodiscard]] WorkGroupInfo GetWorkGroupInfo() const;
    [[nodiscard]] std::optional<SubgroupInfo> GetSubgroupInfo(const uint8_t dim, const size_t* localsize) const;
    [[nodiscard]] const KernelArgStore& GetArgInfos() const noexcept { return ArgStore; }
    [[nodiscard]] bool HasOCLUDebug() const noexcept { return ArgStore.HasDebug; }
    template<uint8_t N>
    [[nodiscard]] std::optional<SubgroupInfo> GetSubgroupInfo(const std::array<size_t, N>& localsize) const
    {
        static_assert(N > 0 && N < 4, "local dim should be in [1,3]");
        return GetSubgroupInfo(N, localsize.data());
    }
    template<uint8_t N, typename... Args>
    [[nodiscard]] auto Call(Args&&... args) const
    {
        static_assert(N > 0 && N < 4, "work dim should be in [1,3]");
        return KernelCallSite<N, Args...>(this, std::forward<Args>(args)...);
    }

};



struct CLProgConfig
{
    common::CLikeDefines Defines;
    std::set<std::string> Flags{ "-cl-fast-relaxed-math", "-cl-mad-enable" };
    uint32_t Version = 0;
};


class OCLUAPI [[nodiscard]] oclProgStub : public common::NonCopyable
{
    friend class oclProgram_;
    friend class NLCLProcessor;
    friend struct NLCLDebugExtension;
private:
    oclContext Context;
    oclDevice Device;
    std::string Source;
    cl_program ProgID;
    std::vector<std::pair<std::string, KernelArgStore>> ImportedKernelInfo;
    oclDebugManager DebugManager;
    oclProgStub(const oclContext& ctx, const oclDevice& dev, std::string&& str);
public:
    ~oclProgStub();
    void Build(const CLProgConfig& config);
    [[nodiscard]] std::u16string GetBuildLog() const;
    [[nodiscard]] oclProgram Finish();
};

class OCLUAPI oclProgram_ : public std::enable_shared_from_this<oclProgram_>, public common::NonCopyable
{
    friend class oclContext_;
    friend class oclKernel_;
    friend class oclProgStub;
private:
    MAKE_ENABLER();
    const oclContext Context;
    const oclDevice Device;
    const std::string Source;
    cl_program ProgID;
    std::vector<std::string> KernelNames;
    std::vector<std::unique_ptr<oclKernel_>> Kernels;
    std::shared_ptr<oclDebugManager> DebugManager;

    [[nodiscard]] static std::u16string GetProgBuildLog(cl_program progID, const cl_device_id dev);
    oclProgram_(oclProgStub* stub);
public:
    ~oclProgram_();
    [[nodiscard]] std::string_view GetSource() const noexcept { return Source; }
    [[nodiscard]] oclKernel GetKernel(const std::string_view& name) const;
    [[nodiscard]] auto GetKernels() const
    {
        return common::container::SlaveVector<oclProgram_, std::unique_ptr<oclKernel_>>(shared_from_this(), Kernels);
    }
    [[nodiscard]] const std::vector<std::string>& GetKernelNames() const { return KernelNames; }
    [[nodiscard]] std::u16string GetBuildLog() const { return GetProgBuildLog(ProgID, *Device); }

    [[nodiscard]] static oclProgStub Create(const oclContext& ctx, std::string str, const oclDevice& dev = {});
    [[nodiscard]] static oclProgram CreateAndBuild(const oclContext& ctx, std::string str, const CLProgConfig& config, const oclDevice& dev = {});
};


}



#if COMPILER_MSVC
#   pragma warning(pop)
#endif
