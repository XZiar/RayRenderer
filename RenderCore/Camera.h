#pragma once

#include "RenderCoreRely.h"

namespace rayr
{

#if COMMON_COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif
class RAYCOREAPI Camera : public xziar::respak::Serializable, public common::Controllable
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
    b3d::Mat3x3 CamMat = b3d::Mat3x3::identity();
    b3d::Vec3 Position = b3d::Vec3(0, 0, 10);
    b3d::Vec3 Rotation = b3d::Vec3::zero();
    float Fovy = 60.0f, zNear = 1.0f, zFar = 100.0f;
    u16string Name;
    Camera() noexcept;
    ~Camera() {}

    RESPAK_DECL_SIMP_DESERIALIZE("rayr#Camera")
    virtual void Serialize(xziar::respak::SerializeUtil& context, xziar::ejson::JObject& object) const override;
    virtual void Deserialize(xziar::respak::DeserializeUtil& context, const xziar::ejson::JObjectRef<true>& object) override;

    b3d::Normal& Right() noexcept { return *reinterpret_cast<b3d::Normal*>(&CamMat.x); }
    b3d::Normal& Up() noexcept { return *reinterpret_cast<b3d::Normal*>(&CamMat.y); }
    b3d::Normal& Toward() noexcept { return *reinterpret_cast<b3d::Normal*>(&CamMat.z); }
    const b3d::Normal& Right() const noexcept { return *reinterpret_cast<const b3d::Normal*>(&CamMat.x); }
    const b3d::Normal& Up() const noexcept { return *reinterpret_cast<const b3d::Normal*>(&CamMat.y); }
    const b3d::Normal& Toward() const noexcept { return *reinterpret_cast<const b3d::Normal*>(&CamMat.z); }
    void Move(const float &x, const float &y, const float &z) noexcept
    {
        Position += CamMat * b3d::Vec3(x, y, z);
    }
    //rotate along x-axis
    void Pitch(const float radx) noexcept
    {
        Rotation.x = std::clamp(Rotation.x + radx, -b3d::PI_float / 2, b3d::PI_float / 2); // limit to 90 degree
        CamMat = b3d::Mat3x3::RotateMatXYZ(Rotation);
    }
    //rotate along y-axis
    void Yaw(const float rady) noexcept
    {
        Rotation.y += rady;
        Rotation.y -= std::floor(Rotation.y / (b3d::PI_float * 2))*(b3d::PI_float * 2);
        CamMat = b3d::Mat3x3::RotateMatXYZ(Rotation);
    }
    //rotate along z-axis
    void Roll(const float radz) noexcept
    {
        Rotation.z = std::clamp(Rotation.z + radz, -b3d::PI_float / 2, b3d::PI_float / 2); // limit to 90 degree
        CamMat = b3d::Mat3x3::RotateMatXYZ(Rotation);
    }

    void Rotate(const float x, const float y, const float z) noexcept
    {
        Rotation.x = std::clamp(Rotation.x + x, -b3d::PI_float / 2, b3d::PI_float / 2); // limit to 90 degree
        Rotation.y += y;
        Rotation.y -= std::floor(Rotation.y / (b3d::PI_float * 2))*(b3d::PI_float * 2);
        Rotation.z = std::clamp(Rotation.z + z, -b3d::PI_float / 2, b3d::PI_float / 2); // limit to 90 degree
        CamMat = b3d::Mat3x3::RotateMatXYZ(Rotation);
    }
    void Rotate(const b3d::Vec3& angles) noexcept
    {
        Rotate(angles.x, angles.y, angles.z);
    }

    b3d::Mat4x4 GetProjection(const float aspect, const bool reverseZ = true) const noexcept
    {
        const float cotHalfFovy = 1 / std::tan(b3d::ang2rad(Fovy / 2));
        if(reverseZ)
            //reverse-z with infinite far
            return b3d::Mat4x4
            {
                { cotHalfFovy / aspect, 0.f, 0.f, 0.f },
                { 0.f, cotHalfFovy, 0.f, 0.f },
                { 0.f, 0.f, 0.f, zNear },
                { 0.f, 0.f, -1.0f, 0.f }
            };
        //(0,0,1,1)->(0,0,near,-1),(0,0,-1,1)->(0,0,near,1)
        
        const float viewDepthR = 1 / (zFar - zNear);
        //*   cot(fovy/2)
        //*  -------------		0		   0			0
        //*     aspect
        //*
        //*       0         cot(fovy/2)	   0			0
        //*
        //*								  F+N         2*F*N
        //*       0              0		 -----	   - -------
        //*								  F-N          F-N
        //*
        //*       0              0           1			0
        //*
        //*/
        return b3d::Mat4x4
        {
            { cotHalfFovy / aspect, 0.f, 0.f, 0.f },
            { 0.f, cotHalfFovy, 0.f, 0.f },
            { 0.f, 0.f, (zFar + zNear) * viewDepthR, (-2 * zFar * zNear) * viewDepthR },
            { 0.f, 0.f, 1.f, 0.f }
        };
    }

    b3d::Mat4x4 GetView() const noexcept
    {
        //LookAt
        //matrix_View = glm::lookAt(vec3(cam.position), vec3(Vertex(cam.position + cam.n)), vec3(cam.v));
        const auto rMat = CamMat.inv();
        return b3d::Mat4x4::TranslateMat(Position * -1, rMat);
    }
};
#if COMMON_COMPILER_MSVC
#   pragma warning(pop)
#endif

}
