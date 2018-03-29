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


public ref class ProgramResource
{
private:
    initonly String ^name, ^type, ^valtype;
    initonly uint32_t size, location, len;
internal:
    ProgramResource(const oglu::ProgramResource& res, const string& name_) 
    {
        name = ToStr(name_);
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
    initonly String^ stage;
    initonly List<String^>^ routines = gcnew List<String^>();
    String^ current;
internal:
    SubroutineResource(std::weak_ptr<oglu::detail::_oglProgram>* prog, const oglu::SubroutineResource& res) : Prog(prog), cppname(res.Name)
    {
        name = ToStr(cppname);
        for (const auto& routine : res.Routines)
            routines->Add(ToStr(routine.Name));
        string srname;
        Prog->lock()->globalState().getSubroutine(cppname, srname).end();
        current = ToStr(srname);
    }
public:
    CLI_READONLY_PROPERTY(String^, Name, name);
    CLI_READONLY_PROPERTY(String^, Stage, stage);
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


public ref class GLProgram : public BaseViewModel
{
internal:
    std::weak_ptr<oglu::detail::_oglProgram> *prog;
internal:
    GLProgram(const oglu::oglProgram *obj) : prog(new std::weak_ptr<oglu::detail::_oglProgram>(*obj)) 
    {
        auto theprog = prog->lock();
        for (const auto& pair : theprog->getResources())
            Resources->Add(gcnew ProgramResource(pair.second, pair.first));
        for (const auto& pair : theprog->getSubroutineResources())
            Subroutines->Add(gcnew SubroutineResource(prog, pair.second));
    }
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
    initonly List<ProgramResource^>^ Resources = gcnew List<ProgramResource^>();
    initonly List<SubroutineResource^>^ Subroutines = gcnew List<SubroutineResource^>();
};



}