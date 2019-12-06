#pragma once

#include "RenderCoreWrapRely.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;
using namespace msclr::interop;


namespace OpenGLUtil
{
using std::string;
using std::u16string;

public enum class DepthTestType : oglu::GLenum
{
#define COPY_ENUM(x) x = common::enum_cast(::oglu::DepthTestType::x)
    COPY_ENUM(OFF),
    COPY_ENUM(Never),
    COPY_ENUM(Equal),
    COPY_ENUM(NotEqual),
    COPY_ENUM(Always),
    COPY_ENUM(Less),
    COPY_ENUM(LessEqual),
    COPY_ENUM(Greater),
    COPY_ENUM(GreaterEqual),
#undef COPY_ENUM
};
public enum class FaceCullingType : uint8_t { OFF, CullCW, CullCCW, CullAll };

public enum class ShaderType : uint32_t
{
#define COPY_ENUM(x) x = common::enum_cast(::oglu::ShaderType::x)
    COPY_ENUM(Vertex),
    COPY_ENUM(Geometry), 
    COPY_ENUM(Fragment),
    COPY_ENUM(TessCtrl), 
    COPY_ENUM(TessEval),
    COPY_ENUM(Compute),
#undef COPY_ENUM
};

public ref class ProgramResource : public BaseViewModel
{
protected:
    initonly String ^name, ^type, ^valtype;
    initonly uint32_t size, location, len;
internal:
    ProgramResource(const oglu::ProgramResource& res);
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
    const std::weak_ptr<oglu::oglProgram_>& Prog;
    const std::string& cppname;
    initonly String^ name;
    initonly List<String^>^ routines = gcnew List<String^>();
    String^ current;
internal:
    SubroutineResource(const std::weak_ptr<oglu::oglProgram_>& prog, const oglu::SubroutineResource& res);
public:
    CLI_READONLY_PROPERTY(String^, Name, name);
    CLI_READONLY_PROPERTY(List<String^>^, Routines, routines);
    property String^ Current
    {
        String^ get() { return current; }
        void set(String^ value) 
        {
            string newval = ToCharStr(value);
            auto prog = Prog.lock();
            prog->State().SetSubroutine(cppname, newval);
            current = ToStr(prog->GetSubroutine(cppname)->Name);
            RaisePropertyChanged("Current");
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
        type = (ShaderType)shader->Type;
        Source = ToStr(shader->SourceText());
    }
};

public ref class GLProgram : public BaseViewModel
{
internal:
    const std::weak_ptr<oglu::oglProgram_> * const prog;
internal:
    GLProgram(const oglu::oglProgram& obj);
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
            RaisePropertyChanged("Name");
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