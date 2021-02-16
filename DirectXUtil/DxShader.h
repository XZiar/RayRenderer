#pragma once
#include "DxRely.h"
#include "DxDevice.h"
#include "DxBuffer.h"
#include "common/CLikeConfig.hpp"
#include "common/StringPool.hpp"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DxShader_;
using DxShader = std::shared_ptr<const DxShader_>;
class DxProgram_;
class DxShaderStub_;
template<typename T>
class DxShaderStub;
class DxComputeProgram_;


enum class ShaderType
{
    Vertex, 
    Geometry, 
    Pixel,
    Hull,
    Domain,
    Compute
};

struct DxShaderConfig
{
    common::CLikeDefines Defines;
    std::set<std::string> Flags;
    uint32_t SMVersion = 0;
    std::u16string EntryPoint;
};


class DXUAPI DxShader_ : public std::enable_shared_from_this<DxShader_>
{
    friend DxShaderStub_;
    friend DxShaderStub<DxShader_>;
    friend DxProgram_;
    friend DxComputeProgram_;
private:
    struct ShaderBlob;
    const PtrProxy<DxDevice_::DeviceProxy>& GetDevice() const noexcept;
protected:
    struct BindResourceDetail;
    struct T_ {};
    const DxDevice Device;
    const std::string Source;
    PtrProxy<ShaderBlob> Blob;
    std::string ShaderHash;
    std::unique_ptr<BindResourceDetail[]> BindResources;
    uint32_t BindCount;
    ShaderType Type;
    uint32_t Version;

public:
    DxShader_(T_, DxShaderStub_* stub);
    virtual ~DxShader_();

    [[nodiscard]] static DxShaderStub<DxShader_> Create(DxDevice dev, ShaderType type, std::string str);
    [[nodiscard]] static DxShader CreateAndBuild(DxDevice dev, ShaderType type, std::string str, const DxShaderConfig& config);
};


class DXUAPI [[nodiscard]] DxShaderStub_ : public common::NonCopyable
{
    friend class DxShader_;
private:
    DxDevice Device;
    std::string Source;
    PtrProxy<DxShader_::ShaderBlob> Blob;
    ShaderType Type;
protected:
    DxShaderStub_(DxDevice dev, ShaderType type, std::string&& str);
public:
    ~DxShaderStub_();
    void Build(const DxShaderConfig& config);
    [[nodiscard]] std::u16string GetBuildLog() const;
};
template<typename T>
class [[nodiscard]] DxShaderStub : public DxShaderStub_
{
    friend T;
private:
    using DxShaderStub_::DxShaderStub_;
public:
    [[nodiscard]] std::shared_ptr<T> Finish()
    {
        using X = typename T::T_;
        return std::make_shared<T>(X{}, this);
    }
};




}