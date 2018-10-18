#include "RenderCoreRely.h"
#include "Camera.h"

namespace rayr
{

void Camera::RegistControllable()
{
    RegistItem<u16string>("Name", "", u"名称", ArgType::RawValue, {}, u"灯光名称")
        .RegistMember(&Camera::Name);
    RegistItem<miniBLAS::Vec3>("Position", "", u"位置", ArgType::RawValue, {}, u"相机位置")
        .RegistMember<false>(&Camera::Position);
    RegistItem<miniBLAS::Vec3>("Rotation", "", u"方向", ArgType::RawValue, {}, u"相机旋转角度")
        .RegistMember<false>(&Camera::Rotation);
    RegistItem<float>("Fovy", "", u"视野", ArgType::RawValue, {}, u"y轴视野角度")
        .RegistMember(&Camera::Fovy);
    RegistItem<std::pair<float, float>>("Zclip", "", u"视野范围", ArgType::RawValue, std::pair(0.f, 10.f), u"相机y轴视野裁剪范围(对数范围2^x)")
        .RegistGetter([](const Controllable& self, const string&)
    { const auto& cam = dynamic_cast<const Camera&>(self); return std::pair(std::log2(cam.zNear), std::log2(cam.zFar)); })
        .RegistSetter([](Controllable& self, const string&, const ControlArg& arg)
    { auto& cam = dynamic_cast<Camera&>(self); const auto[near, far]= std::get<std::pair<float, float>>(arg); 
        cam.zNear = std::pow(2, near), cam.zFar = std::pow(2, far);  });
}

Camera::Camera() noexcept
{
    RegistControllable();
}

void Camera::Serialize(SerializeUtil & context, ejson::JObject& jself) const
{
    jself.Add("Name", str::to_u8string(Name, Charset::UTF16LE));
    jself.Add("Position", detail::ToJArray(context, Position));
    jself.Add("Rotation", detail::ToJArray(context, Rotation));
    jself.Add("Right", detail::ToJArray(context, CamMat.x));
    jself.Add("Up", detail::ToJArray(context, CamMat.y));
    jself.Add("Toward", detail::ToJArray(context, CamMat.z));
    jself.EJOBJECT_ADD(Fovy).EJOBJECT_ADD(zNear).EJOBJECT_ADD(zFar);
}
void Camera::Deserialize(DeserializeUtil&, const ejson::JObjectRef<true>& object)
{
    Name = str::to_u16string(object.Get<string>("Name"), Charset::UTF8);
    detail::FromJArray(object.GetArray("Position"), Position);
    detail::FromJArray(object.GetArray("Rotation"), Rotation);
    detail::FromJArray(object.GetArray("Right"), CamMat.x);
    detail::FromJArray(object.GetArray("Up"), CamMat.y);
    detail::FromJArray(object.GetArray("Toward"), CamMat.z);
    Fovy = object.Get("Fovy", 60.0f);
    zNear = object.Get("zNear", 1.0f);
    zFar = object.Get("zFar", 100.0f);
}

RESPAK_IMPL_SIMP_DESERIALIZE(Camera)


}
