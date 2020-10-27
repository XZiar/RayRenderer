#pragma once
#include "DxRely.h"
#include "DxDevice.h"
#include "common/CLikeConfig.hpp"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DxShader_;
using DxShader = std::shared_ptr<DxShader_>;
class DxComputeShader_;
using DxComputeShader = std::shared_ptr<DxComputeShader_>;
class DxShaderStub_;
template<typename T>
class DxShaderStub;


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


class DXUAPI DxShader_
{
    friend class DxShaderStub_;
    friend class DxShaderStub<DxShader_>;
private:
    struct ShaderBlob;
protected:
    struct T_ {};
    const std::string Source;
    PtrProxy<ShaderBlob> Blob;
    std::string ShaderHash;
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


class DXUAPI DxComputeShader_ : public DxShader_
{
    friend class DxShaderStub<DxComputeShader_>;
private:
    struct T_ {};
public:
    DxComputeShader_(T_, DxShaderStub_* stub);
    ~DxComputeShader_() override;

    [[nodiscard]] static DxComputeShader Create(DxDevice dev, std::string str, const DxShaderConfig& config);
};


}