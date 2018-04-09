#include "RenderCoreWrapRely.h"
#include "OpenGLTypes.h"
#include "ProgInputResource.hpp"

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
    current = ToStr(Prog->lock()->GetSubroutine(cppname)->Name);
}


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
    for (const auto& res : theprog->getSubroutineResources())
        subroutines->Add(gcnew SubroutineResource(prog, res));
    for (const auto& shader : theprog->getShaders())
    {
        auto shd = ShaderObject(shader);
        shaders->Add(shd.Type, shd);
    }
}



}