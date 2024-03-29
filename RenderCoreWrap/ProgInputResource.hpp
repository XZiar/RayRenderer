#pragma once
#include "RenderCoreWrapRely.h"
#include "OpenGLTypes.h"


namespace OpenGLUtil
{

#pragma managed(push, off)
inline const oglu::UniformValue* GetProgCurUniform(const std::weak_ptr<oglu::oglProgram_>& prog, const int32_t location)
{
    return common::container::FindInMap(prog.lock()->getCurUniforms(), location);
}
template<typename... Args>
inline void SetProgVec(const std::weak_ptr<oglu::oglProgram_>& prog, const oglu::ProgramResource* loc, Args&&... args)
{
    prog.lock()->SetVec(loc, std::forward<Args>(args)...);
}
template<typename... Args>
inline void SetProgUniform(const std::weak_ptr<oglu::oglProgram_>& prog, const oglu::ProgramResource* loc, Args&&... args)
{
    prog.lock()->SetVal(loc, std::forward<Args>(args)...);
}
inline std::pair<float, float> GetVec2(const oglu::UniformValue* value)
{
    const auto val = value->Get<oglu::UniformValue::Types::Vec2>();
    return { val.X, val.Y };
}
#pragma managed(pop)

public ref class ProgInputResource : public ProgramResource
{
protected:
    const std::weak_ptr<oglu::oglProgram_>& Prog;
    const oglu::ProgramResource *ptrRes;
    initonly String^ description;
    ProgInputResource(const std::weak_ptr<oglu::oglProgram_>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
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
    RangedProgInputRes(const std::weak_ptr<oglu::oglProgram_>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
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
    Ranged2ProgInputRes(const std::weak_ptr<oglu::oglProgram_>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
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
        return value->Get<oglu::UniformValue::Types::F32>();
    };
    virtual void SetValue(float val) override
    {
        SetProgUniform(Prog, ptrRes, val);
    };
internal:
    RangedProgInputRes_Float(const std::weak_ptr<oglu::oglProgram_>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
        : RangedProgInputRes(ptrProg, res, prop) { }
};

public ref class Ranged2ProgInputRes_Float : public Ranged2ProgInputRes<float>
{
protected:
    virtual float Convert(const oglu::UniformValue* value, const bool isLow) override
    {
        return value->Ptr<float>()[isLow ? 0 : 1];
    };
    virtual void SetValue(float val, const bool isLow) override
    {
        auto ptr = GetValue();
        if (!ptr)
        {
            SetProgVec(Prog, ptrRes, val, val);
        }
        else
        {
            const auto valptr = ptr->Ptr<float>();
            SetProgVec(Prog, ptrRes, isLow ? val : valptr[0], isLow ? valptr[1] : val);
        }
    };
internal:
    Ranged2ProgInputRes_Float(const std::weak_ptr<oglu::oglProgram_>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
        : Ranged2ProgInputRes(ptrProg, res, prop) { }
};


public ref class ColorProgInputRes : public ProgInputResource
{
internal:
    ColorProgInputRes(const std::weak_ptr<oglu::oglProgram_>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
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
            return ptr ? ToColor4(ptr->Ptr<float>()) : defValue;
        }
        void set(System::Windows::Media::Color value)
        {
            SetProgVec(Prog, ptrRes, value.ScR, value.ScG, value.ScB, value.ScA);
            RaisePropertyChanged("Value");
        }
    }
};


public ref class SwitchProgInputRes : public ProgInputResource
{
internal:
    SwitchProgInputRes(const std::weak_ptr<oglu::oglProgram_>* ptrProg, const oglu::ProgramResource& res, const oglu::ShaderExtProperty& prop)
        : ProgInputResource(ptrProg, res, prop)
    { }
public:
    property bool Value
    {
        bool get()
        {
            auto ptr = GetValue();
            return ptr ? ptr->Get<oglu::UniformValue::Types::Bool>() : false;
        }
        void set(bool value)
        {
            SetProgUniform(Prog, ptrRes, value);
            RaisePropertyChanged("Value");
        }
    }
};


}
