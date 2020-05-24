#pragma once
#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"
#include "oclCmdQue.h"
#include "oclBuffer.h"
#include "oclImage.h"
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

enum class KerArgSpace : uint8_t { Global, Constant, Local, Private };
enum class ImgAccess   : uint8_t { ReadOnly, WriteOnly, ReadWrite, None };
enum class KerArgFlag  : uint8_t { None = 0, Const = 0x1, Restrict = 0x2, Volatile = 0x4, Pipe = 0x8 };
MAKE_ENUM_BITFIELD(KerArgFlag)


struct OCLUAPI ArgFlags
{
    KerArgSpace Space;
    ImgAccess   Access;
    KerArgFlag  Qualifier;
    [[nodiscard]] std::string_view GetSpace() const noexcept;
    [[nodiscard]] std::string_view GetImgAccess() const noexcept;
    [[nodiscard]] std::string GetQualifier() const noexcept;
    [[nodiscard]] constexpr bool IsImage() const noexcept { return Access != ImgAccess::None; }

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
protected:
    struct ArgInfo : public ArgFlags
    {
        common::StringPiece<char> Name;
        common::StringPiece<char> Type;
    };
    std::vector<ArgInfo> ArgsInfo;
    std::uint32_t DebugBuffer;
    bool HasInfo;
    KernelArgStore(cl_kernel kernel);
    size_t GetSize() const noexcept { return ArgsInfo.size(); }
    const ArgInfo* GetArg(const size_t idx, const bool check = true) const;
    void AddArg(const KerArgSpace space, const ImgAccess access, const KerArgFlag qualifier,
        const std::string_view name, const std::string_view type);
    KernelArgInfo GetArgInfo(const size_t idx) const noexcept;
    using ItType = common::container::IndirectIterator<KernelArgStore, KernelArgInfo, &KernelArgStore::GetArgInfo>;
    friend ItType;
public:
    KernelArgStore() : DebugBuffer(0), HasInfo(false) {}
    ItType begin() const noexcept { return { this, 0 }; }
    ItType end()   const noexcept { return { this, ArgsInfo.size() }; }
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
    oclKernel_(const oclPlatform_* plat, const oclProgram_* prog, std::string name, KernelArgStore&& argStore);
    template<size_t N>
    [[nodiscard]] constexpr static const size_t* CheckLocalSize(const size_t(&localsize)[N])
    {
        for (size_t i = 0; i < N; ++i)
            if (localsize[i] != 0)
                return localsize;
        return nullptr;
    }
    struct OCLUAPI CallSiteInternal
    {
        oclKernel Kernel;
        common::SpinLocker::ScopeType KernelLock;

        CallSiteInternal(const oclKernel_* kernel);
        void SetArg(const uint32_t idx, const oclBuffer_& buf) const;
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
        [[nodiscard]] common::PromiseResult<void> Run(const uint8_t dim, const common::PromiseStub& pmss,
            const oclCmdQue& que, const size_t* worksize, const size_t* workoffset, const size_t* localsize);
    };

    template<uint8_t N, typename... Args>
    class [[nodiscard]] KernelCallSite : protected CallSiteInternal
    {
        friend class oclKernel_;
    private:
        // clSetKernelArg does not hold parameter ownership, so need to manully hold it
        std::tuple<Args...> Paras;

        template<size_t Idx>
        forceinline void InitArg() const
        {
            using ArgType = common::remove_cvref_t<std::tuple_element_t<Idx, std::tuple<Args...>>>;
            if constexpr (std::is_same_v<ArgType, oclBuffer> || std::is_same_v<ArgType, oclImage>)
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
        [[nodiscard]] common::PromiseResult<void> operator()(const common::PromiseStub& pmss,
            const oclCmdQue& que, const size_t(&worksize)[N], const size_t(&localsize)[N] = { 0 }, const size_t(&workoffset)[N] = { 0 })
        {
            return Run(N, pmss, que, worksize, workoffset, CheckLocalSize(localsize));
        }
        [[nodiscard]] common::PromiseResult<void> operator()(
            const oclCmdQue& que, const size_t(&worksize)[N], const size_t(&localsize)[N] = { 0 }, const size_t(&workoffset)[N] = { 0 })
        {
            return Run(N, {}, que, worksize, workoffset, CheckLocalSize(localsize));
        }
    };
public:
    std::string Name;
    ~oclKernel_();

    [[nodiscard]] WorkGroupInfo GetWorkGroupInfo() const;
    [[nodiscard]] std::optional<SubgroupInfo> GetSubgroupInfo(const uint8_t dim, const size_t* localsize) const;
    [[nodiscard]] const KernelArgStore& GetArgInfos() const noexcept { return ArgStore; }
    template<uint8_t N>
    [[nodiscard]] std::optional<SubgroupInfo> GetSubgroupInfo(const size_t(&localsize)[N]) const
    {
        static_assert(N > 0 && N < 4, "local dim should be in [1,3]");
        return GetSubgroupInfo(N, localsize);
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
    std::set<std::string> Flags{ "-cl-fast-relaxed-math", "-cl-mad-enable", "-cl-kernel-arg-info" };
    uint32_t Version = 0;
};


class OCLUAPI oclProgram_ : public std::enable_shared_from_this<oclProgram_>, public common::NonCopyable
{
    friend class oclContext_;
    friend class oclKernel_;
private:
    MAKE_ENABLER();
    const oclContext Context;
    const oclDevice Device;
    const std::string Source;
    cl_program ProgID;
    std::vector<std::string> KernelNames;
    std::vector<std::unique_ptr<oclKernel_>> Kernels;

    [[nodiscard]] static std::u16string GetProgBuildLog(cl_program progID, const cl_device_id dev);
    class OCLUAPI [[nodiscard]] oclProgStub : public common::NonCopyable
    {
        friend class oclProgram_;
        friend class NLCLProcessor;
    private:
        oclContext Context;
        oclDevice Device;
        std::string Source;
        cl_program ProgID;
        std::vector<std::pair<std::string, KernelArgStore>> ImportedKernelInfo;
    public:
        oclProgStub(const oclContext& ctx, const oclDevice& dev, std::string&& str);
        ~oclProgStub();
        void Build(const CLProgConfig& config);
        [[nodiscard]] std::u16string GetBuildLog() const { return GetProgBuildLog(ProgID, Device->DeviceID); }
        [[nodiscard]] oclProgram Finish();
    };
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
    [[nodiscard]] std::u16string GetBuildLog() const { return GetProgBuildLog(ProgID, Device->DeviceID); }

    [[nodiscard]] static oclProgStub Create(const oclContext& ctx, std::string str, const oclDevice& dev = {});
    [[nodiscard]] static oclProgram CreateAndBuild(const oclContext& ctx, std::string str, const CLProgConfig& config, const oclDevice& dev = {});
};


}



#if COMPILER_MSVC
#   pragma warning(pop)
#endif
