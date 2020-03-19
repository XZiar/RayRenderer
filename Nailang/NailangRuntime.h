#pragma once
#include "NailangStruct.h"
#include "BlockParser.h"
#include <map>


namespace xziar::nailang
{

class EvaluateContext
{
protected:
    std::shared_ptr<EvaluateContext> ParentContext;
    std::map<std::u32string, Arg, std::less<>> ArgMap;
public:
    virtual ~EvaluateContext() {}

    virtual Arg LookUpArg(std::u32string_view name) const;
    virtual bool SetArg(std::u32string_view name, Arg arg);
};


struct EmbedOpEval
{
    static Arg Equal        (const Arg& left, const Arg& right);
    static Arg NotEqual     (const Arg& left, const Arg& right);
    static Arg Less         (const Arg& left, const Arg& right);
    static Arg LessEqual    (const Arg& left, const Arg& right);
    static Arg Greater      (const Arg& left, const Arg& right) { return Less(right, left); }
    static Arg GreaterEqual (const Arg& left, const Arg& right) { return LessEqual(right, left); }
    static Arg And          (const Arg& left, const Arg& right);
    static Arg Or           (const Arg& left, const Arg& right);
    static Arg Add          (const Arg& left, const Arg& right);
    static Arg Sub          (const Arg& left, const Arg& right);
    static Arg Mul          (const Arg& left, const Arg& right);
    static Arg Div          (const Arg& left, const Arg& right);
    static Arg Rem          (const Arg& left, const Arg& right);
    static Arg Not          (const Arg& arg);

    static std::optional<Arg> Eval(const std::u32string_view opname, common::span<const Arg> args);
};

class NailangRuntimeBase
{
protected:
    std::shared_ptr<EvaluateContext> EvalContext;
    
            Arg EvaluateFunc(const FuncCall& call, const BlockContent* target = nullptr);
    virtual Arg EvaluateFunc(const std::u32string_view func, common::span<const Arg> args, const BlockContent* target = nullptr);
    virtual Arg EvaluateArg(const RawArg& arg);
};

}
