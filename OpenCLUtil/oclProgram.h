#pragma once
#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"
#include "oclCmdQue.h"
#include "oclBuffer.h"
#include "oclImage.h"
#include "oclPromiseTask.h"


namespace oclu
{

struct WorkGroupInfo
{
    uint64_t LocalMemorySize;
    uint64_t PrivateMemorySize;
    size_t WorkGroupSize;
    size_t CompiledWorkGroupSize[3];
    size_t PreferredWorkGroupSizeMultiple;
};

struct CLProgConfig
{
    using DefineVal = std::variant<std::monostate, int32_t, uint32_t, int64_t, uint64_t, float, double, std::string>;
    map<string, DefineVal> Defines;
    set<string> Flags { "-cl-fast-relaxed-math", "-cl-mad-enable" };
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
    map<string, Wrapper<_oclKernel>> Kernels;
    vector<cl_device_id> getDevs() const;
    u16string GetBuildLog(const cl_device_id dev) const;
public:
    _oclProgram(const oclContext& ctx_, const string& str);
    ~_oclProgram();
    void Build(const CLProgConfig& config = {}, const oclDevice dev = {});
    u16string GetBuildLog(const oclDevice& dev) const { return GetBuildLog(dev->deviceID); }
    Wrapper<_oclKernel> GetKernel(const string& name);
    auto GetKernels() const { return common::container::ValSet(Kernels); }
    const vector<string>& GetKernelNames() const { return KernelNames; }
};


}
using oclProgram = Wrapper<detail::_oclProgram>;


namespace detail
{

class OCLUAPI _oclKernel
{
    friend class _oclProgram;
private:
    const oclProgram Prog;
    const cl_kernel Kernel;
    _oclKernel(const oclProgram& prog, const string& name);
public:
    const string Name;
    ~_oclKernel();

    WorkGroupInfo GetWorkGroupInfo(const oclDevice& dev);
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
        return SetArg(idx, &dat, sizeof(T));
    }
    template<class T, typename A>
    void SetArg(const uint32_t idx, const vector<T, A>& dat)
    {
        return SetArg(idx, dat.data(), dat.size() * sizeof(T));
    }
    oclPromise Run(const uint32_t workdim, const oclCmdQue que, const size_t *worksize, bool isBlock = true, const size_t *workoffset = nullptr, const size_t *localsize = nullptr);
    template<uint8_t N>
    oclPromise Run(const oclCmdQue que, const size_t(&worksize)[N], bool isBlock = true, const size_t(&workoffset)[N] = { 0 })
    {
        static_assert(N > 0 && N < 4, "work dim should be in [0,3]");
        return Run(N, que, worksize, isBlock, workoffset, nullptr);
    }
    template<uint8_t N>
    oclPromise Run(const oclCmdQue que, const size_t(&worksize)[N], const size_t(&localsize)[N], bool isBlock = true, const size_t(&workoffset)[N] = { 0 })
    {
        static_assert(N > 0 && N < 4, "work dim should be in [0,3]");
        return Run(N, que, worksize, isBlock, workoffset, localsize);
    }
};

}
using oclKernel = Wrapper<detail::_oclKernel>;


}

