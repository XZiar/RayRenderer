#pragma once

#include "RenderCoreRely.h"

namespace dizz
{

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
class RENDERCOREAPI Camera : public xziar::respak::Serializable, public common::Controllable
{
protected:
    virtual u16string_view GetControlType() const override
    {
        using namespace std::literals;
        return u"Light"sv;
    }
    void RegistControllable();
public:
    //fit for reverse-z
    mbase::Mat3 CamMat = mbase::Mat3::Identity();
    mbase::Vec3 Position = mbase::Vec3(0.f, 0.f, 10.f);
    mbase::Vec3 Rotation = mbase::Vec3::Zeros();
    float Fovy = 60.0f, zNear = 1.0f, zFar = 100.0f;
    u16string Name;
    Camera() noexcept;
    ~Camera() {}

    RESPAK_DECL_SIMP_DESERIALIZE("dizz#Camera")
    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;

    /*mbase::Normal& Right() noexcept { return *reinterpret_cast<mbase::Normal*>(&CamMat.X); }
    mbase::Normal& Up() noexcept { return *reinterpret_cast<mbase::Normal*>(&CamMat.Y); }
    mbase::Normal& Toward() noexcept { return *reinterpret_cast<mbase::Normal*>(&CamMat.Z); }*/
    const mbase::Normal& Right() const noexcept { return *reinterpret_cast<const mbase::Normal*>(&CamMat.X); }
    const mbase::Normal& Up() const noexcept { return *reinterpret_cast<const mbase::Normal*>(&CamMat.Y); }
    const mbase::Normal& Toward() const noexcept { return *reinterpret_cast<const mbase::Normal*>(&CamMat.Z); }

    void Move(const float& x, const float& y, const float& z) noexcept;
    //rotate along x-axis
    void Pitch(const float radx) noexcept;
    //rotate along y-axis
    void Yaw(const float rady) noexcept;
    //rotate along z-axis
    void Roll(const float radz) noexcept;

    void Rotate(const float x, const float y, const float z) noexcept;
    void Rotate(const mbase::Vec3& angles) noexcept;

    mbase::Mat4 GetProjection(const float aspect, const bool reverseZ = true) const noexcept;
    mbase::Mat4 GetView() const noexcept;
};
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

}
