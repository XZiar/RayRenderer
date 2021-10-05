#include "RenderCorePch.h"
#include "GLShader.h"


namespace dizz
{
using common::str::Encoding;
using namespace oglu;
using namespace std::literals;
using common::container::FindInSet;
using common::container::FindInMap;
using xziar::respak::SerializeUtil;
using xziar::respak::DeserializeUtil;


GLShader::GLShader(const u16string& name, const string& source, const oglu::ShaderConfig& config) 
    : Source(source), Config(config)
{
    try
    {
        Program = oglDrawProgram_::Create(name, Source, Config);
    }
    catch (const OGLException& gle)
    {
        dizzLog().error(u"OpenGL shader [{}] fail:\n{}\n{}\n", name, gle.Message(), gle.GetDetailMessage());
        COMMON_THROWEX(BaseException, u"OpenGL shader fail");
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
        dynamic_cast<const GLShader&>(control).Program->SetVal(res, val);
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
        const auto stage = oglu::oglShader_::GetStageName(shader->Type);
        const auto u16stage = common::str::to_u16string(stage);
        RegistItem<string>("Shader_" + string(stage), "Source", u16stage, ArgType::LongText, {}, u16stage + u"源码")
            .RegistGetterProxy<GLShader>([type=type](const GLShader& self)
            { return (*FindInMap(self.Program->getShaders(), type))->SourceText(); });
    }
    for (const auto& res : Program->getSubroutineResources())
    {
        const auto u16name = common::str::to_u16string(res.GetName(), Encoding::UTF8);
        auto rtNames = common::linq::FromIterable(res.GetRoutines())
            .Select([](const auto& rt) { return string(rt.Name); }).ToVector();
        RegistItem<string>("Subroutine_" + res.GetName(), "Subroutine", u16name, ArgType::Enum, std::move(rtNames), u16name)
            .RegistGetterProxy<GLShader>([&res](const GLShader& self) { return string(self.Program->GetSubroutine(res)->Name); })
            .RegistSetterProxy<GLShader>([&res](GLShader& self, const string& val) { self.Program->State().SetSubroutine(res.GetName(), val); });
    }
    const auto& props = Program->getResourceProperties();
    for (const auto& res : Program->getResources())
    {
        if (auto prop = common::container::FindInSet(props, res.Name); prop)
        {
            const GLint loc = res.location;
            auto prep = RegistItem("Uniform_" + res.Name, "Uniform", common::str::to_u16string(res.Name, Encoding::UTF8),
                ArgType::RawValue, prop->Data, common::str::to_u16string(prop->Description, Encoding::UTF8));
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


constexpr size_t DefineNone     = common::get_variant_index_v<std::monostate, common::CLikeDefines::ValType>();
constexpr size_t DefineSigned   = common::get_variant_index_v<int64_t,        common::CLikeDefines::ValType>();
constexpr size_t DefineUnsigned = common::get_variant_index_v<uint64_t,       common::CLikeDefines::ValType>();
constexpr size_t DefineDouble   = common::get_variant_index_v<double,         common::CLikeDefines::ValType>();
constexpr size_t DefineStr      = common::get_variant_index_v<string_view,    common::CLikeDefines::ValType>();
void GLShader::Serialize(SerializeUtil & context, xziar::ejson::JObject& jself) const
{
    jself.Add("Name", common::str::to_u8string(Program->Name));
    jself.Add("source", context.PutResource(Source.data(), Source.size()));
    auto config = context.NewObject();
    {
        auto defines = context.NewObject();
        for (const auto&[name, val] : Config.Defines)
        {
            const auto valtype = val.index();
            const string_view defName(name);
            std::visit([&](auto&& rval)
            {
                using T = std::decay_t<decltype(rval)>;
                if constexpr (std::is_same_v<T, std::monostate>)
                    defines.Add(defName, xziar::ejson::JNull{});
                else
                    defines.Add(defName, context.NewArray().Push(valtype, rval));
            }, val);
        }
        config.Add("defines", defines);
    }
    {
        auto routines = context.NewObject(Config.Routines);
        config.Add("routines", routines);
    }
    jself.Add("config", config);
    {
        auto subroutines = context.NewObject();
        for (const auto& sru : Program->getSubroutineResources())
        {
            const auto sr = Program->GetSubroutine(sru);
            if (sr)
                subroutines.Add(sru.GetName(), sr->Name);
        }
        jself.Add("Subroutines", subroutines);
    }
    {
        auto uniforms = context.NewObject();

        const auto& unis = Program->getCurUniforms();
        const auto& props = Program->getResourceProperties();
        for (const auto& res : Program->getResources())
        {
            auto prop = common::container::FindInSet(props, res.Name);
            if (!prop) continue;
            auto uni = common::container::FindInMap(unis, res.location);
            if (!uni) continue;
            const auto key = res.Name + ',' + std::to_string(uni->index());
            std::visit([&](auto&& rval)
            {
                //<miniBLAS::Vec3, miniBLAS::Vec4, miniBLAS::Mat3x3, miniBLAS::Mat4x4, b3d::Coord2D, bool, int32_t, uint32_t, float>;
                using T = std::decay_t<decltype(rval)>;
                if constexpr (std::is_same_v<T, miniBLAS::Vec3> || std::is_same_v<T, miniBLAS::Vec4> || std::is_same_v<T, b3d::Coord2D>)
                    uniforms.Add(key, detail::ToJArray(context, rval));
                else if constexpr (std::is_same_v<T, miniBLAS::Mat3x3> || std::is_same_v<T, miniBLAS::Mat4x4>)
                    return;
                else
                    uniforms.Add(key, rval);
            }, *uni);
        }
        jself.Add("Uniforms", uniforms);
    }
}
void GLShader::Deserialize(DeserializeUtil&, const xziar::ejson::JObjectRef<true>& object)
{
    auto state = Program->State();
    for (const auto&[k,v] : object.GetObject("Subroutines"))
    {
        state.SetSubroutine(k, v.AsValue<string_view>());
    }

}


RESPAK_IMPL_COMP_DESERIALIZE(GLShader, u16string, string, ShaderConfig)
{
    using namespace oglu;
    ShaderConfig config;
    {
        const auto jconfig = object.GetObject("config");
        for (const auto& [key, val] : jconfig.GetObject("defines"))
        {
            if (val.IsNull())
                config.Defines.Add(key);
            else
            {
                xziar::ejson::JArrayRef<true> valarray(val);
                switch (valarray.Get<size_t>(0))
                {
                case DefineSigned:   config.Defines[key] = valarray.Get<int64_t>(1);  break;
                case DefineUnsigned: config.Defines[key] = valarray.Get<uint64_t>(1); break;
                case DefineDouble:   config.Defines[key] = valarray.Get<double>(1);   break;
                case DefineStr:      config.Defines[key] = valarray.Get<string>(1);   break;
                default:             break;
                }
            }
        }
        common::linq::FromContainer(jconfig.GetObject("routines"))
            .IntoMap(config.Routines, 
                [](const auto& kvpair) { return string(kvpair.first); },
                [](const auto& kvpair) { return kvpair.second.template AsValue<string>(); });
    }
    u16string name = common::str::to_u16string(object.Get<string>("config"), Encoding::UTF8);
    const auto srchandle = object.Get<string>("source");
    const auto source = context.GetResource(srchandle);
    return std::any(std::make_tuple(name, string(source.GetRawPtr<const char>(), source.GetSize()), config));
}


}
