#pragma once

#include "oclRely.h"
#include "oclDevice.h"
#include "oclContext.h"


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace oclu
{
class oclPlatform_;
using oclPlatform = std::shared_ptr<const oclPlatform_>;


template<typename HostT, typename T>
struct SlaveVector
{
private:
    class SlaveIterator
    {
    private:
        const SlaveVector& Base;
        typename std::vector<T>::const_iterator It;
    public:
        SlaveIterator(const SlaveVector* base, typename std::vector<T>::const_iterator it)
            : Base(*base), It(it) { }

        SlaveIterator& operator++()
        {
            It++; return *this;
        }
        bool operator!=(const SlaveIterator& other) const { return It != other.It; }
        std::shared_ptr<const T> operator*() const
        {
            return std::shared_ptr<const T>(Base.Host, &*It);
        }
    };

    std::shared_ptr<const HostT> Host;
    const std::vector<T>& Vec;
public:
    SlaveVector(std::shared_ptr<const HostT> host, const std::vector<T>& vec)
        : Host(host), Vec(vec) { }
    SlaveIterator begin() const
    {
        return SlaveIterator(this, Vec.cbegin());
    }
    SlaveIterator end() const
    {
        return SlaveIterator(this, Vec.cend());
    }
    operator std::vector<std::shared_ptr<const T>>() const
    {
        std::vector<std::shared_ptr<const T>> vec;
        vec.reserve(Vec.size());
        for (auto x : *this)
            vec.emplace_back(x);
        return vec;
    }
};



class OCLUAPI oclPlatform_ : public NonCopyable, public NonMovable, public std::enable_shared_from_this<oclPlatform_>
{
    friend class oclUtil;
    friend class GLInterop;
    friend class oclKernel_;
private:
    const cl_platform_id PlatformID;
    vector<oclDevice_> Devices;
    const oclDevice_* DefDevice;
    clGetGLContextInfoKHR_fn FuncClGetGLContext = nullptr;
    clGetKernelSubGroupInfoKHR_fn FuncClGetKernelSubGroupInfo = nullptr;
    common::container::FrozenDenseSet<string> Extensions;

    oclPlatform_(const cl_platform_id pID);
    vector<cl_context_properties> GetCLProps() const;
    oclContext CreateContext(const vector<oclDevice>& devs, const vector<cl_context_properties>& props) const;
public:
    u16string Name, Ver;
    Vendors PlatVendor;
    SlaveVector<oclPlatform_, oclDevice_> GetDevices() const 
    { 
        return SlaveVector<oclPlatform_, oclDevice_>(shared_from_this(), Devices);
    }
    const common::container::FrozenDenseSet<string>& GetExtensions() const { return Extensions; }
    oclDevice GetDefaultDevice() const;
    oclContext CreateContext(const vector<oclDevice>& devs) const { return CreateContext(devs, GetCLProps()); }
    oclContext CreateContext(oclDevice dev) const { return CreateContext(vector<oclDevice>{ dev }); }
    oclContext CreateContext() const;
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif