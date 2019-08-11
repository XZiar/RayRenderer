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

void Camera::Serialize(SerializeUtil&, ejson::JObject& jself) const
{
    using detail::JsonConv;
    jself.Add("Name", strchset::to_u8string(Name, Charset::UTF16LE));
    jself.Add<JsonConv>(EJ_FIELD(Position))
         .Add<JsonConv>(EJ_FIELD(Rotation))
         .Add<JsonConv>("Right", CamMat.x)
         .Add<JsonConv>("Up", CamMat.y)
         .Add<JsonConv>("Toward", CamMat.z)
         .Add(EJ_FIELD(Fovy)).Add(EJ_FIELD(zNear)).Add(EJ_FIELD(zFar));
}
void Camera::Deserialize(DeserializeUtil&, const ejson::JObjectRef<true>& object)
{
    using detail::JsonConv;
    Name = strchset::to_u16string(object.Get<string>("Name"), Charset::UTF8);
    object.TryGet<JsonConv>(EJ_FIELD(Position));
    object.TryGet<JsonConv>(EJ_FIELD(Rotation));
    object.TryGet<JsonConv>("Right", CamMat.x);
    object.TryGet<JsonConv>("Up", CamMat.y);
    object.TryGet<JsonConv>("Toward", CamMat.z);
    Fovy = object.Get("Fovy", 60.0f);
    zNear = object.Get("zNear", 1.0f);
    zFar = object.Get("zFar", 100.0f);
}

RESPAK_IMPL_SIMP_DESERIALIZE(Camera)


}
