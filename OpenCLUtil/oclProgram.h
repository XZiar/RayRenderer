#pragma once
#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"
#include "oclCmdQue.h"
#include "oclBuffer.h"
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


namespace detail
{

class OCLUAPI _oclProgram : public std::enable_shared_from_this<_oclProgram>
{
    friend class _oclContext;
    friend class _oclKernel;
private:
    const oclContext ctx;
    const string src;
    const cl_program progID;
    vector<string> kers;
    vector<oclDevice> getDevs() const;
    void initKers();
public:
    _oclProgram(const oclContext& ctx_, const string& str);
    ~_oclProgram();
    void build(const string& options = "-cl-fast-relaxed-math -cl-mad-enable", const oclDevice dev = oclDevice());
    u16string getBuildLog(const oclDevice& dev) const;
    Wrapper<_oclKernel> getKernel(const string& name);
    const vector<string>& getKernelNames() const;
};


}
using oclProgram = Wrapper<detail::_oclProgram>;


namespace detail
{

class OCLUAPI _oclKernel
{
    friend class _oclProgram;
private:
    const string name;
    const oclProgram prog;
    const cl_kernel kernel;
    cl_kernel createKernel() const;
    _oclKernel(const oclProgram& prog_, const string& name_);
public:
    ~_oclKernel();

    WorkGroupInfo GetWorkGroupInfo(const oclDevice& dev);
    void setArg(const uint32_t idx, const oclBuffer& buf);
    void setArg(const uint32_t idx, const void *dat, const size_t size);
    template<class T, size_t N>
    void setArg(const uint32_t idx, const T(&dat)[N])
    {
        return setArg(idx, dat, N * sizeof(T));
    }
    template<class T>
    void setSimpleArg(const uint32_t idx, const T &dat)
    {
        return setArg(idx, &dat, sizeof(T));
    }
    template<class T, typename A>
    void setArg(const uint32_t idx, const vector<T, A>& dat)
    {
        return setArg(idx, dat.data(), dat.size() * sizeof(T));
    }
    optional<oclPromise> run(const uint32_t workdim, const oclCmdQue que, const size_t *worksize, bool isBlock = true, const size_t *workoffset = nullptr, const size_t *localsize = nullptr);
    template<uint32_t N>
    optional<oclPromise> run(const oclCmdQue que, const size_t(&worksize)[N], bool isBlock = true, const size_t(&workoffset)[N] = { 0 })
    {
        return run(N, que, worksize, isBlock, workoffset, nullptr);
    }
    template<uint32_t N>
    optional<oclPromise> run(const oclCmdQue que, const size_t(&worksize)[N], const size_t(&localsize)[N], bool isBlock = true, const size_t(&workoffset)[N] = { 0 })
    {
        return run(N, que, worksize, isBlock, workoffset, localsize);
    }
};

}
using oclKernel = Wrapper<detail::_oclKernel>;


}

