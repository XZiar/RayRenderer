#pragma once
#include "RenderCoreWrapRely.h"
#include "OpenGLTypes.h"


namespace OpenGLUtil
{

#pragma managed(push, off)
inline const oglu::UniformValue* GetProgCurUniform(const std::weak_ptr<oglu::detail::_oglProgram>& prog, const GLint location)
{
    return common::container::FindInMap(prog.lock()->getCurUniforms(), (GLint)location);
}
#pragma managed(pop)

public ref class ProgInputResource : public ProgramResource
{
protected:
    const std::weak_ptr<oglu::detail::_oglProgram>& Prog;
    const oglu::ProgramResource *ptrRes;
    initonly String^ description;
    ProgInputResource(const std::weak_ptr<oglu::detail::_oglProgram>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
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
    RangedProgInputRes(const std::weak_ptr<oglu::detail::_oglProgram>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
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
        void set(T value) { SetValue(value); RaisePropertyChanged("Value"); }
    }
};

template<typename T>
public ref class Ranged2ProgInputRes abstract : public ProgInputResource
{
internal:
    Ranged2ProgInputRes(const std::weak_ptr<oglu::detail::_oglProgram>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
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
        void set(T value) { SetValue(value, true); RaisePropertyChanged("LowValue"); }
    }
    property T HighValue
    {
        T get() { auto ptr = GetValue(); return ptr ? Convert(ptr, false) : defValue; }
        void set(T value) { SetValue(value, false); RaisePropertyChanged("HighValue"); }
    }
};

public ref class RangedProgInputRes_Float : public RangedProgInputRes<float>
{
protected:
    virtual float Convert(const oglu::UniformValue* value) override
    {
        return std::get<float>(*value);
    };
    virtual void SetValue(float val) override
    {
        Prog.lock()->SetUniform(ptrRes, val);
    };
internal:
    RangedProgInputRes_Float(const std::weak_ptr<oglu::detail::_oglProgram>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
        : RangedProgInputRes(ptrProg, res, prop) { }
};

public ref class Ranged2ProgInputRes_Float : public Ranged2ProgInputRes<float>
{
protected:
    virtual float Convert(const oglu::UniformValue* value, const bool isLow) override
    {
        const auto& c2d = std::get<b3d::Coord2D>(*value);
        return isLow ? c2d.u : c2d.v;
    };
    virtual void SetValue(float val, const bool isLow) override
    {
        auto ptr = GetValue();
        if (!ptr)
        {
            Prog.lock()->SetVec(ptrRes, val, val);
        }
        else
        {
            const auto& c2d = std::get<b3d::Coord2D>(*ptr);
            Prog.lock()->SetVec(ptrRes, isLow ? val : c2d.u, isLow ? c2d.v : val);
        }
    };
internal:
    Ranged2ProgInputRes_Float(const std::weak_ptr<oglu::detail::_oglProgram>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
        : Ranged2ProgInputRes(ptrProg, res, prop) { }
};


public ref class ColorProgInputRes : public ProgInputResource
{
internal:
    ColorProgInputRes(const std::weak_ptr<oglu::detail::_oglProgram>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
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
            Prog.lock()->SetVec(ptrRes, value.ScR, value.ScG, value.ScB, value.ScA);
            RaisePropertyChanged("Value");
        }
    }
};


public ref class SwitchProgInputRes : public ProgInputResource
{
internal:
    SwitchProgInputRes(const std::weak_ptr<oglu::detail::_oglProgram>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
        : ProgInputResource(ptrProg, res, prop)
    { }
public:
    property bool Value
    {
        bool get()
        {
            auto ptr = GetValue();
            return ptr ? std::get<bool>(*ptr) : false;
        }
        void set(bool value)
        {
            Prog.lock()->SetUniform(ptrRes, value);
            RaisePropertyChanged("Value");
        }
    }
};


}
