#pragma once
#include "dxRely.h"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DXDevice_;
using DXDevice = const DXDevice_*;
class DXCopyCmdQue_;
class DXCmdList_;
class DXResource_;

class DXUAPI DXDevice_ : public common::NonCopyable
{
    friend class DXCopyCmdQue_;
    friend class DXCmdList_;
    friend class DXResource_;
private:
    DXDevice_(void* adapter, void* device, std::u16string_view name);
protected:
    struct AdapterProxy;
    struct DeviceProxy;
    AdapterProxy* Adapter;
    DeviceProxy* Device;
    std::u16string AdapterName;
public:
    DXDevice_(DXDevice_&& other) noexcept;
    ~DXDevice_();
    [[nodiscard]] const std::u16string& GetAdapterName() const noexcept { return AdapterName; }

    [[nodiscard]] static common::span<const DXDevice_> GetDevices();
};


}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
