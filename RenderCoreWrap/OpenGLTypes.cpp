#include "RenderCoreWrapRely.h"
#include "OpenGLTypes.h"
#include "ProgInputResource.hpp"

namespace OpenGLUtil
{


ProgramResource::ProgramResource(const oglu::ProgramResource& res)
{
    name = ToStr(res.Name);
    type = ToStr(res.GetTypeName());
    valtype = ToStr(res.GetValTypeName());
    size = res.size;
    location = res.location;
    len = res.len;
}


SubroutineResource::SubroutineResource(const std::weak_ptr<oglu::oglProgram_>& prog, const oglu::SubroutineResource& res)
    : Prog(prog), cppname(res.Name)
{
    name = ToStr(cppname);
    stage = (ShaderType)res.Stage;
    for (const auto& routine : res.Routines)
        routines->Add(ToStr(routine.Name));
    current = ToStr(Prog.lock()->GetSubroutine(cppname)->Name);
}


GLProgram::GLProgram(const oglu::oglProgram& obj) : prog(new std::weak_ptr<oglu::oglProgram_>(obj))
{
    const auto& props = obj->getResourceProperties();
    for (const auto& res : obj->getResources())
    {
        if (auto it = common::container::FindInSet(props, res.Name))
        {
            if (it->Type == oglu::ShaderPropertyType::Float && it->Data.has_value()) 
            {
                resources->Insert(0, gcnew RangedProgInputRes_Float(prog, res, *it));
                continue;
            }
            if (it->Type == oglu::ShaderPropertyType::Range && it->Data.has_value())
            {
                resources->Insert(0, gcnew Ranged2ProgInputRes_Float(prog, res, *it));
                continue;
            }
            if (it->Type == oglu::ShaderPropertyType::Color)
            {
                resources->Insert(0, gcnew ColorProgInputRes(prog, res, *it));
                continue;
            }
            if (it->Type == oglu::ShaderPropertyType::Bool)
            {
                resources->Insert(0, gcnew SwitchProgInputRes(prog, res, *it));
                continue;
            }
        }
        resources->Add(gcnew ProgramResource(res));
    }
    for (const auto& res : obj->getSubroutineResources())
        subroutines->Add(gcnew SubroutineResource(*prog, res));
    for (const auto&[type, shader] : obj->getShaders())
    {
        auto shd = ShaderObject(shader);
        shaders->Add(shd.Type, shd);
    }
}



}