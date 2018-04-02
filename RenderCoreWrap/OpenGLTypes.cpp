#include "RenderCoreWrapRely.h"
#include "OpenGLTypes.h"

namespace OpenGLUtil
{


ProgramResource::ProgramResource(const oglu::ProgramResource& res)
{
    name = ToStr(res.Name);
    type = gcnew String(res.GetTypeName());
    valtype = gcnew String(res.GetValTypeName());
    size = res.size;
    location = res.location;
    len = res.len;
}


SubroutineResource::SubroutineResource(std::weak_ptr<oglu::detail::_oglProgram>* prog, const oglu::SubroutineResource& res) : Prog(prog), cppname(res.Name)
{
    name = ToStr(cppname);
    stage = (ShaderType)res.Stage;
    for (const auto& routine : res.Routines)
        routines->Add(ToStr(routine.Name));
    string srname;
    Prog->lock()->globalState().getSubroutine(cppname, srname).end();
    current = ToStr(srname);
}


public ref class RangedProgInputRes_Float : public RangedProgInputRes<float>
{
protected:
    virtual float Convert(const oglu::UniformValue* value) override { return std::get<float>(*value); };
    virtual void SetValue(float val) override { Prog.lock()->SetUniform(ptrRes, val); };
internal:
    RangedProgInputRes_Float(std::weak_ptr<oglu::detail::_oglProgram>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
        : RangedProgInputRes(ptrProg, res, prop) { }
};

#pragma unmanaged
inline void SetUniformColor(std::weak_ptr<oglu::detail::_oglProgram>& prog, const oglu::ProgramResource *res, const float r, const float g, const float b, const float a)
{
    prog.lock()->SetVec(res, b3d::Vec4(r, g, b, a));
}
#pragma managed

public ref class ColorProgInputRes : public ProgInputResource
{
internal:
    ColorProgInputRes(std::weak_ptr<oglu::detail::_oglProgram>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
        : ProgInputResource(ptrProg, res, prop)
    {
        defValue = System::Windows::Media::Color::FromArgb(0, 0, 0, 0);
    }
protected:
    initonly System::Windows::Media::Color defValue;
public:
    property System::Windows::Media::Color Value
    {
        System::Windows::Media::Color get() 
        { 
            auto ptr = GetValue();
            return ptr ? ToColor(std::get<miniBLAS::Vec4>(*ptr)) : defValue;
        }
        void set(System::Windows::Media::Color value) 
        {
            SetUniformColor(Prog, ptrRes, value.ScR, value.ScG, value.ScB, value.ScA);
            OnPropertyChanged("Value");
        }
    }
};


GLProgram::GLProgram(const oglu::oglProgram *obj) : prog(new std::weak_ptr<oglu::detail::_oglProgram>(*obj))
{
    auto theprog = prog->lock();
    const auto& props = theprog->getResourceProperties();
    for (const auto& res : theprog->getResources())
    {
        if (auto it = common::container::FindInSet(props, res.Name))
        {
            if (it->Type == oglu::ShaderPropertyType::Float && it->Data.has_value()) 
            {
                resources->Add(gcnew RangedProgInputRes_Float(prog, res, *it));
                continue;
            }
            if (it->Type == oglu::ShaderPropertyType::Color)
            {
                resources->Add(gcnew ColorProgInputRes(prog, res, *it));
                continue;
            }
        }
        resources->Add(gcnew ProgramResource(res));
    }
    for (const auto& res : theprog->getSubroutineResources())
        subroutines->Add(gcnew SubroutineResource(prog, res));
    for (const auto& shader : theprog->getShaders())
    {
        auto shd = ShaderObject(shader);
        shaders->Add(shd.Type, shd);
    }
}



}