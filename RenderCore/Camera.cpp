#include "RenderCorePch.h"
#include "Camera.h"

namespace dizz
{
using common::str::Encoding;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;


void Camera::RegistControllable()
{
    RegistItem<u16string>("Name", "", u"名称", ArgType::RawValue, {}, u"灯光名称")
        .RegistMember(&Camera::Name);
    RegistItem<mbase::Vec3>("Position", "", u"位置", ArgType::RawValue, {}, u"相机位置")
        .RegistMember<false>(&Camera::Position);
    RegistItem<mbase::Vec3>("Rotation", "", u"方向", ArgType::RawValue, {}, u"相机旋转角度")
        .RegistMember<false>(&Camera::Rotation);
    RegistItem<float>("Fovy", "", u"视野", ArgType::RawValue, {}, u"y轴视野角度")
        .RegistMember(&Camera::Fovy);
    RegistItem<std::pair<float, float>>("Zclip", "", u"视野范围", ArgType::RawValue, std::pair(0.f, 10.f), u"相机y轴视野裁剪范围(对数范围2^x)")
        .RegistGetterProxy<Camera>([](const Camera & cam)
        { 
            return std::pair(std::log2(cam.zNear), std::log2(cam.zFar));
        })
        .RegistSetterProxy<Camera>([](Camera & cam, const std::pair<float, float>& arg)
        { 
            cam.zNear = std::pow(2.0f, arg.first), cam.zFar = std::pow(2.0f, arg.second);
        });
}

Camera::Camera() noexcept
{
    RegistControllable();
}

void Camera::Serialize(SerializeUtil&, xziar::ejson::JObject& jself) const
{
    using detail::JsonConv;
    jself.Add("Name", common::str::to_u8string(Name));
    jself.Add<JsonConv>(EJ_FIELD(Position))
         .Add<JsonConv>(EJ_FIELD(Rotation))
         .Add<JsonConv>("Right", CamMat.X)
         .Add<JsonConv>("Up", CamMat.Y)
         .Add<JsonConv>("Toward", CamMat.Z)
         .Add(EJ_FIELD(Fovy)).Add(EJ_FIELD(zNear)).Add(EJ_FIELD(zFar));
}
void Camera::Deserialize(DeserializeUtil&, const xziar::ejson::JObjectRef<true>& object)
{
    using detail::JsonConv;
    Name = common::str::to_u16string(object.Get<string>("Name"), Encoding::UTF8);
    object.TryGet<JsonConv>(EJ_FIELD(Position));
    object.TryGet<JsonConv>(EJ_FIELD(Rotation));
    object.TryGet<JsonConv>("Right", CamMat.X);
    object.TryGet<JsonConv>("Up", CamMat.Y);
    object.TryGet<JsonConv>("Toward", CamMat.Z);
    Fovy = object.Get("Fovy", 60.0f);
    zNear = object.Get("zNear", 1.0f);
    zFar = object.Get("zFar", 100.0f);
}

RESPAK_IMPL_SIMP_DESERIALIZE(Camera)


inline constexpr float PI_2 = math::PI_float / 2, PI2 = math::PI_float * 2;

void Camera::Move(const float& x, const float& y, const float& z) noexcept
{
    Position.As<msimd::Vec3>() += CamMat.As<msimd::Mat3>() * msimd::Vec3(x, y, z);
}
//rotate along x-axis
void Camera::Pitch(const float radx) noexcept
{
    Rotation.X = std::clamp(Rotation.X + radx, -PI_2, PI_2); // limit to 90 degree
    CamMat.As<msimd::Mat3>() = math::RotateMatXYZ<msimd::Mat3>(Rotation.As<msimd::Vec3>());
}
//rotate along y-axis
void Camera::Yaw(const float rady) noexcept
{
    Rotation.Y += rady;
    Rotation.Y -= std::floor(Rotation.Y / PI2) * PI2;
    CamMat.As<msimd::Mat3>() = math::RotateMatXYZ<msimd::Mat3>(Rotation.As<msimd::Vec3>());
}
//rotate along z-axis
void Camera::Roll(const float radz) noexcept
{
    Rotation.Z = std::clamp(Rotation.Z + radz, -PI_2, PI_2); // limit to 90 degree
    CamMat.As<msimd::Mat3>() = math::RotateMatXYZ<msimd::Mat3>(Rotation.As<msimd::Vec3>());
}

void Camera::Rotate(const float x, const float y, const float z) noexcept
{
    Rotation.X = std::clamp(Rotation.X + x, -PI_2, PI_2); // limit to 90 degree
    Rotation.Y += y;
    Rotation.Y -= std::floor(Rotation.Y / PI2) * PI2;
    Rotation.Z = std::clamp(Rotation.Z + z, -PI_2, PI_2); // limit to 90 degree
    CamMat.As<msimd::Mat3>() = math::RotateMatXYZ<msimd::Mat3>(Rotation.As<msimd::Vec3>());
}
void Camera::Rotate(const mbase::Vec3& angles) noexcept
{
    Rotate(angles.X, angles.Y, angles.Z);
}

mbase::Mat4 Camera::GetProjection(const float aspect, const bool reverseZ) const noexcept
{
    const float cotHalfFovy = 1 / std::tan(math::Ang2Rad(Fovy / 2));
    if (reverseZ)
        //reverse-z with infinite far
        return mbase::Mat4
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
    return mbase::Mat4
    {
        { cotHalfFovy / aspect, 0.f, 0.f, 0.f },
        { 0.f, cotHalfFovy, 0.f, 0.f },
        { 0.f, 0.f, (zFar + zNear) * viewDepthR, (-2 * zFar * zNear) * viewDepthR },
        { 0.f, 0.f, 1.f, 0.f }
    };
}

mbase::Mat4 Camera::GetView() const noexcept
{
    //LookAt
    //matrix_View = glm::lookAt(vec3(cam.position), vec3(Vertex(cam.position + cam.n)), vec3(cam.v));
    const auto rMat = Transpose(CamMat.As<msimd::Mat3>());
    return math::TranslateMat<mbase::Mat4>(Position.As<msimd::Vec3>().Negative(), rMat);
}


}
