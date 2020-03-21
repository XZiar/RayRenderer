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
public:
    virtual ~EvaluateContext();

    virtual Arg LookUpArg(std::u32string_view name) const = 0;
    virtual bool SetArg(std::u32string_view name, Arg arg) = 0;
    virtual bool ShouldContinue() const;
};

class BasicEvaluateContext : public EvaluateContext
{
protected:
    std::map<std::u32string, Arg, std::less<>> ArgMap;
    std::optional<Arg> ReturnArg;
public:
    ~BasicEvaluateContext() override;

    Arg LookUpArg(std::u32string_view name) const override;
    bool SetArg(std::u32string_view name, Arg arg) override;
    bool ShouldContinue() const override;
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
    
    void ThrowByArgCount(const FuncCall& call, const size_t count) const;
    void ThrowByArgType(const Arg& arg, const Arg::InternalType type) const;
    bool ThrowIfNotBool(const Arg& arg, const std::u32string_view varName) const;

    virtual bool HandleMetaFuncBefore(const FuncCall& meta, const BlockContent& target);
            bool HandleMetaFuncsBefore(common::span<const FuncCall> metas, const BlockContent& target);
    virtual void HandleRawBlock(const RawBlock& block, common::span<const FuncCall> metas);
    
    virtual void EvaluateAssignment(const Assignment& assign, common::span<const FuncCall> metas);
            Arg  EvaluateFunc(const FuncCall& call, common::span<const FuncCall> metas, const BlockContent* target = nullptr);
    virtual Arg  EvaluateFunc(const std::u32string_view func, common::span<const Arg> args, common::span<const FuncCall> metas, const BlockContent* target = nullptr);
    virtual Arg  EvaluateArg(const RawArg& arg);
    virtual void EvaluateContent(const BlockContent& content, common::span<const FuncCall> metas);
public:
    ~NailangRuntimeBase();
    virtual void ExecuteBlock(const Block& block, common::span<const FuncCall> metas);
};

}
