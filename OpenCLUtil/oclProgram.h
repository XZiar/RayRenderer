#pragma once
#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"
#include "oclCmdQue.h"
#include "oclBuffer.h"
#include "oclImage.h"
#include "common/ContainerHelper.hpp"



#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclProgram_;
using oclProgram = std::shared_ptr<oclProgram_>;
class oclKernel_;
using oclKernel = std::shared_ptr<oclKernel_>;


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
enum class ImgAccess : uint8_t { ReadOnly, WriteOnly, ReadWrite, None };
enum class KerArgFlag : uint8_t { None = 0, Const = 0x1, Restrict = 0x2, Volatile = 0x4, Pipe = 0x8 };
MAKE_ENUM_BITFIELD(KerArgFlag)

struct OCLUAPI KernelArgInfo
{
    string Name;
    string Type;
    KerArgSpace Space;
    ImgAccess Access;
    KerArgFlag Qualifier;
    string_view GetSpace() const;
    string_view GetImgAccess() const;
    string GetQualifier() const;
};

class oclProgram_;

class OCLUAPI oclKernel_ : public NonCopyable, public NonMovable
{
    friend class oclProgram_;
private:
    MAKE_ENABLER();
    const oclPlatform_& Plat;
    const oclProgram_& Prog;
    const cl_kernel Kernel;
    std::atomic_flag ArgLock = ATOMIC_FLAG_INIT;
    oclKernel_(const oclPlatform_* plat, const oclProgram_* prog, string name);
    void CheckArgIdx(const uint32_t idx) const;
    template<size_t N>
    constexpr static const size_t* CheckLocalSize(const size_t(&localsize)[N])
    {
        for (size_t i = 0; i < N; ++i)
            if (localsize[i] != 0)
                return localsize;
        return nullptr;
    }
    template<uint8_t N, typename... Args>
    class KernelCallSite
    {
        friend class oclKernel_;
    private:
        oclKernel_& Kernel;
        // clSetKernelArg does not hold parameter ownership, so need to manully hold it
        std::tuple<Args...> Paras;
        common::SpinLocker KernelLock;

        template<size_t Idx>
        forceinline void SetArg() const
        {
            using ArgType = common::remove_cvref_t<std::tuple_element_t<Idx, std::tuple<Args...>>>;
            if constexpr (std::is_same_v<ArgType, oclBuffer> || std::is_same_v<ArgType, oclImage>)
                Kernel.SetArg(Idx, *std::get<Idx>(Paras));
            else if constexpr (common::container::ContiguousHelper<ArgType>::IsContiguous)
                Kernel.SetSpanArg(Idx, std::get<Idx>(Paras));
            else
                Kernel.SetSimpleArg(Idx, std::get<Idx>(Paras));
            if constexpr (Idx != 0)
                SetArg<Idx - 1>();
        }

        KernelCallSite(oclKernel_* kernel, Args&& ... args) : Kernel(*kernel), Paras(std::forward<Args>(args)...), KernelLock(Kernel.ArgLock)
        {
            SetArg<sizeof...(Args) - 1>();
        }
    public:
        PromiseResult<void> operator()(const oclCmdQue& que, const size_t(&worksize)[N], const size_t(&localsize)[N] = { 0 }, const size_t(&workoffset)[N] = { 0 })
        {
            return Kernel.Run({}, N, que, worksize, false, workoffset, CheckLocalSize(localsize));
        }
        PromiseResult<void> operator()(const PromiseResult<void>& pms, const oclCmdQue& que, const size_t(&worksize)[N], const size_t(&localsize)[N] = { 0 }, const size_t(&workoffset)[N] = { 0 })
        {
            return Kernel.Run(pms, N, que, worksize, false, workoffset, CheckLocalSize(localsize));
        }
    };
public:
    const string Name;
    const vector<KernelArgInfo> ArgsInfo;
    ~oclKernel_();

    WorkGroupInfo GetWorkGroupInfo(const oclDevice& dev);
    std::optional<SubgroupInfo> GetSubgroupInfo(const oclDevice& dev, const uint8_t dim, const size_t* localsize);
    template<uint8_t N>
    std::optional<SubgroupInfo> GetSubgroupInfo(const oclDevice& dev, const size_t(&localsize)[N])
    {
        static_assert(N > 0 && N < 4, "local dim should be in [1,3]");
        return GetSubgroupInfo(dev, N, localsize);
    }
    void SetArg(const uint32_t idx, const oclBuffer_& buf);
    void SetArg(const uint32_t idx, const oclImage_& img);
    void SetArg(const uint32_t idx, const void *dat, const size_t size);
    template<class T>
    void SetSimpleArg(const uint32_t idx, const T& dat)
    {
        static_assert(!std::is_same_v<T, bool>, "boolean is implementation-defined and cannot be pass as kernel argument.");
        return SetArg(idx, &dat, sizeof(T));
    }
    template<typename T>
    void SetSpanArg(const uint32_t idx, const T& dat)
    {
        using Helper = common::container::ContiguousHelper<T>;
        static_assert(Helper::IsContiguous, "Only accept contiguous type");
        return SetArg(idx, Helper::Data(dat), Helper::Count(dat) * Helper::EleSize);
    }
    PromiseResult<void> Run(const PromiseResult<void>& pms, const uint32_t workdim, const oclCmdQue& que, const size_t *worksize, bool isBlock = true, const size_t *workoffset = nullptr, const size_t *localsize = nullptr);
    template<uint8_t N>
    PromiseResult<void> Run(const PromiseResult<void>& pms, const oclCmdQue& que, const size_t(&worksize)[N], bool isBlock = true, const size_t(&workoffset)[N] = { 0 })
    {
        static_assert(N > 0 && N < 4, "work dim should be in [1,3]");
        return Run(pms, N, que, worksize, isBlock, workoffset, nullptr);
    }
    template<uint8_t N>
    PromiseResult<void> Run(const PromiseResult<void>& pms, const oclCmdQue& que, const size_t(&worksize)[N], const size_t(&localsize)[N], bool isBlock = true, const size_t(&workoffset)[N] = { 0 })
    {
        static_assert(N > 0 && N < 4, "work dim should be in [1,3]");
        return Run(pms, N, que, worksize, isBlock, workoffset, localsize);
    }

    PromiseResult<void> Run(const uint32_t workdim, const oclCmdQue& que, const size_t *worksize, bool isBlock = true, const size_t *workoffset = nullptr, const size_t *localsize = nullptr)
    {
        return Run({}, workdim, que, worksize, isBlock, workoffset, localsize);
    }
    template<uint8_t N>
    PromiseResult<void> Run(const oclCmdQue& que, const size_t(&worksize)[N], bool isBlock = true, const size_t(&workoffset)[N] = { 0 })
    {
        static_assert(N > 0 && N < 4, "work dim should be in [1,3]");
        return Run({}, N, que, worksize, isBlock, workoffset, nullptr);
    }
    template<uint8_t N>
    PromiseResult<void> Run(const oclCmdQue& que, const size_t(&worksize)[N], const size_t(&localsize)[N], bool isBlock = true, const size_t(&workoffset)[N] = { 0 })
    {
        static_assert(N > 0 && N < 4, "work dim should be in [1,3]");
        return Run({}, N, que, worksize, isBlock, workoffset, localsize);
    }
    template<uint8_t N, typename... Args>
    auto Call(Args&&... args)
    {
        static_assert(N > 0 && N < 4, "work dim should be in [1,3]");
        if (sizeof...(Args) != ArgsInfo.size())
            COMMON_THROW(BaseException, u"Argument parameter provided does not match parameter needed.");
        return KernelCallSite<N, Args...>(this, std::forward<Args>(args)...);
    }

};



struct CLProgConfig
{
    using DefineVal = std::variant<std::monostate, int32_t, uint32_t, int64_t, uint64_t, float, double, std::string>;
    map<string, DefineVal> Defines;
    set<string> Flags{ "-cl-fast-relaxed-math", "-cl-mad-enable", "-cl-kernel-arg-info" };
};


class OCLUAPI oclProgram_ : public std::enable_shared_from_this<oclProgram_>, public NonCopyable
{
    friend class oclContext_;
    friend class oclKernel_;
private:
    MAKE_ENABLER();
    const oclContext Context;
    const string src;
    const cl_program progID;
    vector<string> KernelNames;
    map<string, std::unique_ptr<oclKernel_>, std::less<>> Kernels;
    vector<cl_device_id> getDevs() const;
    u16string GetBuildLog(const cl_device_id dev) const;
protected:
    oclProgram_(const oclContext& ctx_, const string& str);
public:
    ~oclProgram_();
    void Build(const CLProgConfig& config = {}, const oclDevice dev = {});
    u16string GetBuildLog(const oclDevice& dev) const { return GetBuildLog(dev->DeviceID); }
    oclKernel GetKernel(const string& name);
    auto GetKernels() const { return common::container::ValSet(Kernels); }
    const vector<string>& GetKernelNames() const { return KernelNames; }

    static std::shared_ptr<oclProgram_> Create(const oclContext& ctx_, const string& str);
};




}



#if COMPILER_MSVC
#   pragma warning(pop)
#endif
