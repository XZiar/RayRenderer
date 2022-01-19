#pragma once

#include "RenderCoreRely.h"

namespace dizz
{


enum class LightType : int32_t { Parallel = 0, Point = 1, Spot = 2 };

struct RENDERCOREAPI LightData
{
    mbase::Vec4 Color       = mbase::Vec4::Ones();
    mbase::Vec3 Position    = mbase::Vec3::Zeros();
    mbase::Vec3 Direction   = mbase::Vec3(0.f, -1.f, 0.f);
    mbase::Vec4 Attenuation = mbase::Vec4(0.5f, 0.3f, 0.0f, 10.0f);
    float CutoffOuter, CutoffInner;
    const LightType Type;
protected:
    LightData(const LightType type) : Type(type) {}
public:
    static constexpr size_t WriteSize = 4 * 4 * sizeof(float);
    void Move(const float x, const float y, const float z)
    {
        Position += mbase::Vec3(x, y, z);
    }
    void Move(const mbase::Vec3& offset)
    {
        Position += offset;
    }
    void Rotate(const float x, const float y, const float z)
    {
        Rotate(mbase::Vec3(x, y, z));
    }
    void Rotate(const mbase::Vec3& radius)
    {
        const auto rMat = math::RotateMatXYZ<mbase::Mat3>(radius);
        Direction = rMat * Direction;
    }
    void WriteData(const common::span<std::byte> space) const;
};

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
class RENDERCOREAPI Light : public LightData, public xziar::respak::Serializable, public common::Controllable
{
protected:
    Light(const LightType type, const std::u16string& name);
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"Light"sv;
    }
    void RegistControllable();
public:
    bool IsOn = true;
    std::u16string Name;
    virtual ~Light() override {}
    RESPAK_DECL_COMP_DESERIALIZE("dizz#Light")
    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;
};
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

class ParallelLight : public Light
{
public:
    ParallelLight() : Light(LightType::Parallel, u"ParallelLight") { Attenuation.W = 1.0f; }
};

class PointLight : public Light
{
public:
    PointLight() : Light(LightType::Point, u"PointLight") {}
};

class SpotLight : public Light
{
public:
    SpotLight() : Light(LightType::Spot, u"SpotLight") 
    {
        CutoffInner = 30.0f;
        CutoffOuter = 90.0f;
    }
};

}
