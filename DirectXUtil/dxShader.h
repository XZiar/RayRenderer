#pragma once
#include "dxRely.h"
#include "dxDevice.h"
#include "common/CLikeConfig.hpp"

#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

namespace dxu
{
class DXShader_;
using DXShader = std::shared_ptr<DXShader_>;
class DXShaderStub;

enum class ShaderType
{
    Vertex, 
    Geometry, 
    Pixel,
    Hull,
    Domain,
    Compute
};

struct DXShaderConfig
{
    common::CLikeDefines Defines;
    std::set<std::string> Flags;
    uint32_t SMVersion = 0;
    std::u16string EntryPoint;
};


class DXUAPI DXShader_
{
    friend class DXShaderStub;
private:
    struct ShaderBlob;
protected:
    MAKE_ENABLER();
    PtrProxy<ShaderBlob> Blob;
public:
    virtual ~DXShader_();


    [[nodiscard]] static DXShaderStub Create(DXDevice dev, ShaderType type, std::string str);
    [[nodiscard]] static DXShader CreateAndBuild(DXDevice dev, ShaderType type, std::string str, const DXShaderConfig& config);
};


class DXUAPI [[nodiscard]] DXShaderStub : public common::NonCopyable
{
    friend class DXShader_;
private:
    struct BuildResult;
    DXDevice Device;
    std::string Source;
    PtrProxy<DXShader_::ShaderBlob> Blob;
    ShaderType Type;
    DXShaderStub(DXDevice dev, ShaderType type, std::string&& str);
public:
    ~DXShaderStub();
    void Build(const DXShaderConfig& config);
    [[nodiscard]] std::u16string GetBuildLog() const;
    [[nodiscard]] DXShader Finish();
};


}