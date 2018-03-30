#pragma once

#include "RenderCoreWrapRely.h"
#include "b3d.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;
using namespace msclr::interop;


namespace OpenGLUtil
{
using std::string;
using std::u16string;

public enum class ShaderType : GLenum
{
    Vertex = GL_VERTEX_SHADER, Geometry = GL_GEOMETRY_SHADER, Fragment = GL_FRAGMENT_SHADER,
    TessCtrl = GL_TESS_CONTROL_SHADER, TessEval = GL_TESS_EVALUATION_SHADER,
    Compute = GL_COMPUTE_SHADER
};

public ref class ProgramResource
{
private:
    initonly String ^name, ^type, ^valtype;
    initonly uint32_t size, location, len;
internal:
    ProgramResource(const oglu::ProgramResource& res) 
    {
        name = ToStr(res.Name);
        type = gcnew String(res.GetTypeName());
        valtype = gcnew String(res.GetValTypeName());
        size = res.size;
        location = res.location;
        len = res.len;
    }
public:
    CLI_READONLY_PROPERTY(String^, Name, name);
    CLI_READONLY_PROPERTY(String^, Type, type);
    CLI_READONLY_PROPERTY(String^, ValueType, valtype);
    CLI_READONLY_PROPERTY(uint32_t, Size, size);
    CLI_READONLY_PROPERTY(uint32_t, Location, location);
    CLI_READONLY_PROPERTY(uint32_t, Length, len);
};

public ref class SubroutineResource : public BaseViewModel
{
private:
    const std::weak_ptr<oglu::detail::_oglProgram> * const Prog;
    const std::string& cppname;
    initonly String^ name;
    initonly ShaderType stage;
    initonly List<String^>^ routines = gcnew List<String^>();
    String^ current;
internal:
    SubroutineResource(std::weak_ptr<oglu::detail::_oglProgram>* prog, const oglu::SubroutineResource& res) : Prog(prog), cppname(res.Name)
    {
        name = ToStr(cppname);
        stage = (ShaderType)res.Stage;
        for (const auto& routine : res.Routines)
            routines->Add(ToStr(routine.Name));
        string srname;
        Prog->lock()->globalState().getSubroutine(cppname, srname).end();
        current = ToStr(srname);
    }
public:
    CLI_READONLY_PROPERTY(String^, Name, name);
    CLI_READONLY_PROPERTY(ShaderType, Stage, stage);
    CLI_READONLY_PROPERTY(List<String^>^, Routines, routines);
    property String^ Current
    {
        String^ get() { return current; }
        void set(String^ value) 
        {
            string newval = ToCharStr(value);
            Prog->lock()->globalState().setSubroutine(cppname, newval).getSubroutine(cppname, newval).end();
            current = ToStr(newval);
            OnPropertyChanged("Current");
        }
    }
};

public value struct ShaderObject
{
    initonly String^ Source;
    CLI_READONLY_PROPERTY(ShaderType, Type, type);
internal:
    initonly ShaderType type;
    ShaderObject(const oglu::oglShader& shader)
    {
        type = (ShaderType)shader->shaderType;
        Source = ToStr(shader->SourceText());
    }
};

public ref class GLProgram : public BaseViewModel
{
internal:
    std::weak_ptr<oglu::detail::_oglProgram> *prog;
internal:
    GLProgram(const oglu::oglProgram *obj) : prog(new std::weak_ptr<oglu::detail::_oglProgram>(*obj)) 
    {
        auto theprog = prog->lock();
        for (const auto& res : theprog->getResources())
            resources->Add(gcnew ProgramResource(res));
        for (const auto& res : theprog->getSubroutineResources())
            subroutines->Add(gcnew SubroutineResource(prog, res));
        for (const auto& shader : theprog->getShaders())
        {
            auto shd = ShaderObject(shader);
            shaders->Add(shd.Type, shd);
        }
    }
    initonly List<ProgramResource^>^ resources = gcnew List<ProgramResource^>();
    initonly List<SubroutineResource^>^ subroutines = gcnew List<SubroutineResource^>();
    using ShaderDict = Dictionary<ShaderType, ShaderObject>;
    initonly ShaderDict^ shaders = gcnew ShaderDict();
public:
    ~GLProgram() { this->!GLProgram(); }
    !GLProgram() { delete prog; }
    property String^ Name
    {
        String^ get() { return ToStr(prog->lock()->Name); }
        void set(String^ value) 
        { 
            prog->lock()->Name = ToU16Str(value); 
            OnPropertyChanged("Name"); 
        }
    }
    virtual String^ ToString() override
    {
        return "[oglProgram]" + Name;
    }
    CLI_READONLY_PROPERTY(List<ProgramResource^>^, Resources, resources);
    CLI_READONLY_PROPERTY(List<SubroutineResource^>^, Subroutines, subroutines);
    CLI_READONLY_PROPERTY(ShaderDict^, Shaders, shaders);
};



}