#pragma once
#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"
#include "oclCmdQue.h"
#include "oclBuffer.h"
#include "oclImage.h"


namespace oclu
{


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

namespace detail
{
class _oclProgram;

class OCLUAPI _oclKernel
{
    friend class _oclProgram;
private:
    const std::shared_ptr<_oclProgram> Prog;
    const cl_kernel Kernel;
    _oclKernel(const std::shared_ptr<_oclProgram>& prog, const string& name);
    void CheckArgIdx(const uint32_t idx) const;
public:
    const string Name;
    const vector<KernelArgInfo> ArgsInfo;
    ~_oclKernel();

    WorkGroupInfo GetWorkGroupInfo(const oclDevice& dev);
    std::optional<SubgroupInfo> GetSubgroupInfo(const oclDevice& dev, const uint8_t dim, const size_t* localsize);
    template<uint8_t N>
    std::optional<SubgroupInfo> GetSubgroupInfo(const oclDevice& dev, const size_t(&localsize)[N])
    {
        static_assert(N > 0 && N < 4, "local dim should be in [1,3]");
        return GetSubgroupInfo(dev, N, localsize);
    }
    void SetArg(const uint32_t idx, const oclBuffer& buf);
    void SetArg(const uint32_t idx, const oclImage& img);
    void SetArg(const uint32_t idx, const void *dat, const size_t size);
    template<class T, size_t N>
    void SetArg(const uint32_t idx, const T(&dat)[N])
    {
        return SetArg(idx, dat, N * sizeof(T));
    }
    template<class T>
    void SetSimpleArg(const uint32_t idx, const T &dat)
    {
        static_assert(!std::is_same_v<T, bool>, "boolean is implementation-defined and cannot be pass as kernel argument.");
        return SetArg(idx, &dat, sizeof(T));
    }
    template<class T, typename A>
    void SetArg(const uint32_t idx, const vector<T, A>& dat)
    {
        return SetArg(idx, dat.data(), dat.size() * sizeof(T));
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
};

}
using oclKernel = Wrapper<detail::_oclKernel>;


struct CLProgConfig
{
    using DefineVal = std::variant<std::monostate, int32_t, uint32_t, int64_t, uint64_t, float, double, std::string>;
    map<string, DefineVal> Defines;
    set<string> Flags{ "-cl-fast-relaxed-math", "-cl-mad-enable", "-cl-kernel-arg-info" };
};

namespace detail
{

class OCLUAPI _oclProgram : public std::enable_shared_from_this<_oclProgram>
{
    friend class _oclContext;
    friend class _oclKernel;
private:
    const oclContext Context;
    const string src;
    const cl_program progID;
    vector<string> KernelNames;
    map<string, oclKernel, std::less<>> Kernels;
    vector<cl_device_id> getDevs() const;
    u16string GetBuildLog(const cl_device_id dev) const;
public:
    _oclProgram(const oclContext& ctx_, const string& str);
    ~_oclProgram();
    void Build(const CLProgConfig& config = {}, const oclDevice dev = {});
    u16string GetBuildLog(const oclDevice& dev) const { return GetBuildLog(dev->deviceID); }
    oclKernel GetKernel(const string& name);
    auto GetKernels() const { return common::container::ValSet(Kernels); }
    const vector<string>& GetKernelNames() const { return KernelNames; }
};


}
using oclProgram = Wrapper<detail::_oclProgram>;


}

