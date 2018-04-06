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

public enum class DepthTestType : GLenum
{
    OFF = GL_INVALID_INDEX, Never = GL_NEVER, Equal = GL_EQUAL, NotEqual = GL_NOTEQUAL, Always = GL_ALWAYS,
    Less = GL_LESS, LessEqual = GL_LEQUAL, Greater = GL_GREATER, GreaterEqual = GL_GEQUAL
};
public enum class FaceCullingType : uint8_t { OFF, CullCW, CullCCW, CullAll };

public enum class ShaderType : GLenum
{
    Vertex = GL_VERTEX_SHADER, Geometry = GL_GEOMETRY_SHADER, Fragment = GL_FRAGMENT_SHADER,
    TessCtrl = GL_TESS_CONTROL_SHADER, TessEval = GL_TESS_EVALUATION_SHADER,
    Compute = GL_COMPUTE_SHADER
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
    const std::weak_ptr<oglu::detail::_oglProgram> * const Prog;
    const std::string& cppname;
    initonly String^ name;
    initonly ShaderType stage;
    initonly List<String^>^ routines = gcnew List<String^>();
    String^ current;
internal:
    SubroutineResource(std::weak_ptr<oglu::detail::_oglProgram>* prog, const oglu::SubroutineResource& res);
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
            auto prog = Prog->lock();
            prog->State().SetSubroutine(cppname, newval);
            current = ToStr(prog->GetSubroutine(cppname)->Name);
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
    GLProgram(const oglu::oglProgram *obj);
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

#pragma managed(push, off)
inline const oglu::UniformValue* GetProgCurUniform(std::weak_ptr<oglu::detail::_oglProgram>& prog, const GLint location)
{
    return common::container::FindInMap(prog.lock()->getCurUniforms(), (GLint)location);
}
#pragma managed(pop)

public ref class ProgInputResource : public ProgramResource
{
protected:
    std::weak_ptr<oglu::detail::_oglProgram>& Prog;
    const oglu::ProgramResource *ptrRes;
    initonly String^ description;
    ProgInputResource(std::weak_ptr<oglu::detail::_oglProgram>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
        : ProgramResource(res), Prog(*ptrProg), ptrRes(&res), description(ToStr(prop.Description))
    { }
    const oglu::UniformValue* GetValue() { return GetProgCurUniform(Prog, location); }
public:
    CLI_READONLY_PROPERTY(String^, Description, description);
};


template<typename T>
public ref class RangedProgInputRes abstract : public ProgInputResource
{
internal:
    RangedProgInputRes(std::weak_ptr<oglu::detail::_oglProgram>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
        : ProgInputResource(ptrProg, res, prop)
    {
        const auto& range = *std::any_cast<std::pair<T, T>>(&prop.Data);
        minValue = range.first, maxValue = range.second, defValue = 0;
    }
protected:
    initonly T minValue, maxValue, defValue;
    virtual T Convert(const oglu::UniformValue* value) = 0;
    virtual void SetValue(T var) = 0;
public:
    CLI_READONLY_PROPERTY(T, MinValue, minValue);
    CLI_READONLY_PROPERTY(T, MaxValue, maxValue);
    property T Value
    {
        T get() { auto ptr = GetValue(); return ptr ? Convert(ptr) : defValue; }
        void set(T value) { SetValue(value); OnPropertyChanged("Value"); }
    }
};

template<typename T>
public ref class Ranged2ProgInputRes abstract : public ProgInputResource
{
internal:
    Ranged2ProgInputRes(std::weak_ptr<oglu::detail::_oglProgram>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
        : ProgInputResource(ptrProg, res, prop)
    {
        const auto& range = *std::any_cast<std::pair<T, T>>(&prop.Data);
        minValue = range.first, maxValue = range.second, defValue = 0;
    }
protected:
    initonly T minValue, maxValue, defValue;
    virtual T Convert(const oglu::UniformValue* value, const bool isLow) = 0;
    virtual void SetValue(T var, const bool isLow) = 0;
public:
    CLI_READONLY_PROPERTY(T, MinValue, minValue);
    CLI_READONLY_PROPERTY(T, MaxValue, maxValue);
    property T LowValue
    {
        T get() { auto ptr = GetValue(); return ptr ? Convert(ptr, true) : defValue; }
        void set(T value) { SetValue(value, true); OnPropertyChanged("LowValue"); }
    }
    property T HighValue
    {
        T get() { auto ptr = GetValue(); return ptr ? Convert(ptr, false) : defValue; }
        void set(T value) { SetValue(value, false); OnPropertyChanged("HighValue"); }
    }
};


}