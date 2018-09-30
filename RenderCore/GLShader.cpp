#pragma once
#include "RenderCoreRely.h"
#include "GLShader.h"


namespace rayr
{

using namespace oglu;
using namespace std::literals;
using common::container::FindInSet;
using common::container::FindInMap;
using common::linq::Linq;

GLShader::GLShader(const u16string& name, const string& source, const oglu::ShaderConfig& config) 
    : Controllable(u"GLShader"), Source(source), Config(config)
{
    Program.reset(name);
    try
    {
        Program->AddExtShaders(source, Config);
        Program->Link();
    }
    catch (const OGLException& gle)
    {
        basLog().error(u"OpenGL shader [{}] fail:\n{}\n", name, gle.message);
        COMMON_THROW(BaseException, u"OpenGL shader fail");
    }
    RegistControllable();
}
template<typename T>
static T GetProgCurUniform(const common::Controllable& control, const GLint location, const T defVal = {})
{
    const auto ptrVal = common::container::FindInMap(dynamic_cast<const GLShader&>(control).Program->getCurUniforms(), location);
    if (ptrVal)
    {
        const auto ptrRealVal = std::get_if<T>(ptrVal);
        if (ptrRealVal)
            return *ptrRealVal;
    }
    return defVal;
}
template<typename T>
static void SetProgUniform(const common::Controllable& control, const oglu::ProgramResource* res, const T& val)
{
    if constexpr (std::is_same_v<T, miniBLAS::Vec3> || std::is_same_v<T, miniBLAS::Vec4> || std::is_same_v<T, b3d::Coord2D>)
        dynamic_cast<const GLShader&>(control).Program->SetVec(res, val);
    else if constexpr (std::is_same_v<T, miniBLAS::Mat3x3> || std::is_same_v<T, miniBLAS::Mat4x4>)
        dynamic_cast<const GLShader&>(control).Program->SetMat(res, val);
    else
        dynamic_cast<const GLShader&>(control).Program->SetUniform(res, val);
}
void GLShader::RegistControllable()
{
    AddCategory("Subroutine", u"Subroutine");
    AddCategory("Uniform", u"Uniform");
    AddCategory("Source", u"源代码");
    RegistItem<u16string>("Name", "", u"名称", ArgType::RawValue, {}, u"Shader名称")
        .RegistMemberProxy<GLShader>([](const GLShader& control) -> u16string& { return control.Program->Name; });

    RegistItem<string>("ExtSource", "Source", u"Sourcecode", ArgType::LongText, {}, u"Shader合并源码")
        .RegistMember<false, true>(&GLShader::Source);
    for (const auto&[type, shader] : Program->getShaders())
    {
        const auto stage = oglu::oglShader::GetStageName(shader->shaderType);
        const auto u16stage = str::to_u16string(stage);
        RegistItem<string>("Shader_" + string(stage), "Source", u16stage, ArgType::LongText, {}, u16stage + u"源码")
            .RegistGetter([type](const Controllable& self, const string&)
            { return (*FindInMap(dynamic_cast<const GLShader&>(self).Program->getShaders(), type))->SourceText(); });
    }
    for (const auto& res : Program->getSubroutineResources())
    {
        const auto u16name = str::to_u16string(res.Name, str::Charset::UTF8);
        auto rtNames = Linq::FromIterable(res.Routines)
            .Select([](const auto& rt) {return rt.Name; }).ToVector();
        RegistItem<string>("Subroutine_" + res.Name, "Subroutine", u16name, ArgType::RawValue, std::move(rtNames), u16name)
            .RegistGetter([&res](const Controllable& self, const string&) { return dynamic_cast<const GLShader&>(self).Program->GetSubroutine(res)->Name; })
            .RegistSetter([&res](Controllable& self, const string&, const ControlArg& val) 
            { dynamic_cast<GLShader&>(self).Program->State().SetSubroutine(res.Name, std::get<string>(val)); });
    }
    const auto& props = Program->getResourceProperties();
    for (const auto& res : Program->getResources())
    {
        if (auto prop = common::container::FindInSet(props, res.Name); prop)
        {
            const GLint loc = res.location;
            auto prep = RegistItem("Uniform_" + res.Name, "Uniform", str::to_u16string(res.Name, str::Charset::UTF8),
                ArgType::RawValue, prop->Data, str::to_u16string(prop->Description, str::Charset::UTF8));
            if (prop->Type == oglu::ShaderPropertyType::Range && prop->Data.has_value())
            {
                prep.AsType<std::pair<float, float>>()
                    .RegistGetter([loc](const Controllable& self, const string&)
                    { return (std::pair<float, float>)GetProgCurUniform<b3d::Coord2D>(self, loc); })
                    .RegistSetter([&res](Controllable& self, const string&, const ControlArg& arg)
                    { SetProgUniform(self, &res, (b3d::Coord2D)std::get<std::pair<float, float>>(arg)); });
            }
            else if (prop->Type == oglu::ShaderPropertyType::Float && prop->Data.has_value())
            {
                prep.AsType<float>()
                    .RegistGetter([loc](const Controllable& self, const string&) { return GetProgCurUniform<float>(self, loc); })
                    .RegistSetter([&res](Controllable& self, const string&, const ControlArg& arg) { SetProgUniform(self, &res, std::get<float>(arg)); });
            }
            else if (prop->Type == oglu::ShaderPropertyType::Color)
            {
                prep.AsType<miniBLAS::Vec4>().SetArgType(ArgType::Color)
                    .RegistGetter([loc](const Controllable& self, const string&) { return GetProgCurUniform<miniBLAS::Vec4>(self, loc, miniBLAS::Vec4::zero()); })
                    .RegistSetter([&res](Controllable& self, const string&, const ControlArg& arg) { SetProgUniform(self, &res, std::get<miniBLAS::Vec4>(arg)); });
            }
            else if (prop->Type == oglu::ShaderPropertyType::Bool)
            {
                prep.AsType<bool>()
                    .RegistGetter([loc](const Controllable& self, const string&) { return GetProgCurUniform<bool>(self, loc); })
                    .RegistSetter([&res](Controllable& self, const string&, const ControlArg& arg) { SetProgUniform(self, &res, std::get<bool>(arg)); });
            }
            
        }
    }
}


constexpr size_t DefineNone   = common::get_variant_index_v<std::monostate, oglu::ShaderConfig::DefineVal>();
constexpr size_t DefineInt32  = common::get_variant_index_v<int32_t,        oglu::ShaderConfig::DefineVal>();
constexpr size_t DefineUInt32 = common::get_variant_index_v<uint32_t,       oglu::ShaderConfig::DefineVal>();
constexpr size_t DefineFloat  = common::get_variant_index_v<float,          oglu::ShaderConfig::DefineVal>();
constexpr size_t DefineDouble = common::get_variant_index_v<double,         oglu::ShaderConfig::DefineVal>();
ejson::JObject GLShader::Serialize(SerializeUtil& context) const
{
    using namespace xziar::ejson;

    auto jself = context.NewObject();
    jself.Add("Name", str::to_u8string(Program->Name, Charset::UTF16LE));
    jself.Add("source", context.PutResource(Source.data(), Source.size()));
    auto config = context.NewObject();
    {
        auto defines = context.NewObject();
        for (const auto&[name, val] : Config.Defines)
        {
            std::visit([&](auto&& rval)
            {
                using T = std::decay_t<decltype(rval)>;
                if constexpr (std::is_same_v<T, std::monostate>)
                    defines.Add(name, JNull{});
                else
                    defines.Add(name, context.NewArray().Push(val.index(), rval));
            }, val);
        }
        config.Add("defines", defines);
    }
    {
        auto routines = context.NewObject(Config.Routines);
        config.Add("routines", routines);
    }
    jself.Add("config", config);
    return jself;
}
void GLShader::Deserialize(DeserializeUtil& context, const ejson::JObjectRef<true>& object)
{
}

RESPAK_DESERIALIZER(GLShader)
{
    using namespace oglu;
    ShaderConfig config;
    {
        const auto jconfig = object.GetObject("config");
        config.Defines = Linq::FromIterable(jconfig.GetObject("defines"))
            .ToMap(config.Defines,
                [](const auto& kvpair) { return string(kvpair.first); },
                [](const auto& kvpair) -> ShaderConfig::DefineVal
                {
                    if (kvpair.second.IsNull()) return {};
                    xziar::ejson::JArrayRef<true> valarray(kvpair.second);
                    switch (valarray.Get<size_t>(0))
                    {
                    case DefineInt32:   return valarray.Get<int32_t>(1);
                    case DefineUInt32:  return valarray.Get<uint32_t>(1);
                    case DefineFloat:   return valarray.Get<float>(1);
                    case DefineDouble:  return valarray.Get<double>(1);
                    default:            return {};
                    }
                });
        config.Routines = Linq::FromIterable(jconfig.GetObject("routines"))
            .ToMap(std::map<string, string>{},
                [](const auto& kvpair) { return string(kvpair.first); },
                [](const auto& kvpair) { return kvpair.second.AsValue<string>(); });
    }
    const u16string name = str::to_u16string(object.Get<string>("config"), Charset::UTF8);
    auto ret = new GLShader(name, object.Get<string>("source"), config);
    ret->Deserialize(context, object);
    return std::unique_ptr<Serializable>(ret);
}
RESPAK_REGIST_DESERIALZER(GLShader)


}
