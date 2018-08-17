#include "RenderCoreRely.h"
#include "Camera.h"

namespace rayr
{

ejson::JObject Camera::Serialize(SerializeUtil& context) const
{
    auto jself = context.NewObject();
    jself.Add("Name", str::to_u8string(Name, Charset::UTF16LE));
    jself.Add("Position", detail::ToJArray(context, Position));
    jself.Add("Rotation", detail::ToJArray(context, Rotation));
    jself.Add("Right", detail::ToJArray(context, CamMat.x));
    jself.Add("Up", detail::ToJArray(context, CamMat.y));
    jself.Add("Toward", detail::ToJArray(context, CamMat.z));
    jself.EJOBJECT_ADD(Fovy).EJOBJECT_ADD(Aspect).EJOBJECT_ADD(zNear).EJOBJECT_ADD(zFar);
    jself.Add("Size", context.NewArray().Push(Width, Height));
    return jself;
}
void Camera::Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object)
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
    uint16_t w, h;
    object.GetArray("Size").TryGetMany(0, w, h);
    Resize(w, h);
}

RESPAK_DESERIALIZER(Camera)
{
    auto ret = new Camera();
    ret->Deserialize(context, object);
    return std::unique_ptr<Serializable>(ret);
}
RESPAK_REGIST_DESERIALZER(Camera)


}