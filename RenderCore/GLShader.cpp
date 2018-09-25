#pragma once
#include "RenderCoreRely.h"
#include "GLShader.h"


namespace rayr
{

using namespace oglu;
using common::container::FindInSet;

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
void GLShader::RegistControllable()
{
    RegistControlItemInDirect<string, GLShader>("Source", "", u"Sourcecode",
        &GLShader::Source, ArgType::RawValue, {}, u"Shader源码");
    RegistControlItemInDirect<u16string, GLShader>("Name", "", u"名称",
        [](const GLShader& control) -> u16string& { return control.Program->Name; }, ArgType::RawValue, {}, u"Shader名称");
    for (const auto& res : Program->getSubroutineResources())
    {
        const u16string u16Name(res.Name.cbegin(), res.Name.cend());
        RegistControlItem<string>("Subroutine_" + res.Name, "Subroutine", u16Name,
            [&res](const Controllable& self, const string&) { return dynamic_cast<const GLShader&>(self).Program->GetSubroutine(res)->Name; },
            [&res](Controllable& self, const string&, const ControlArg& val) 
            { dynamic_cast<GLShader&>(self).Program->State().SetSubroutine(res.Name, std::get<string>(val)); },
            ArgType::RawValue, {}, u16Name);
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
        for (const auto&[name, val] : jconfig.GetObject("defines"))
        {
            string strName(name);
            if (val.IsNull())
                config.Defines[strName] = std::monostate{};
            else
            {
                xziar::ejson::JArrayRef<true> valarray(val);
                switch (valarray.Get<size_t>(0))
                {
                case DefineInt32:   config.Defines[strName] = valarray.Get<int32_t>(1);  break;
                case DefineUInt32:  config.Defines[strName] = valarray.Get<uint32_t>(1); break;
                case DefineFloat:   config.Defines[strName] = valarray.Get<float>(1);    break;
                case DefineDouble:  config.Defines[strName] = valarray.Get<double>(1);   break;
                default:            break;
                }
            }
        }
        for (const auto&[name, val] : jconfig.GetObject("routines"))
        {
            config.Routines[string(name)] = val.AsValue<string>();
        }
    }
    const u16string name = str::to_u16string(object.Get<string>("config"), Charset::UTF8);
    auto ret = new GLShader(name, object.Get<string>("source"), config);
    ret->Deserialize(context, object);
    return std::unique_ptr<Serializable>(ret);
}
RESPAK_REGIST_DESERIALZER(GLShader)


}
