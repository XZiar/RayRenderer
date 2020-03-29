#pragma once
#include "NailangStruct.h"
#include "BlockParser.h"
#include <map>


namespace xziar::nailang
{

class LocalFuncContext
{
public:
    virtual Arg GetReturnArg() const = 0;
};

class EvaluateContext
{
protected:
    std::shared_ptr<EvaluateContext> ParentContext;
public:
    struct LocalFunc
    {
        const Block* Body;
        common::span<std::u32string_view> ArgNames;
    };
    virtual ~EvaluateContext();

    virtual Arg LookUpArg(std::u32string_view name) const = 0;
    virtual bool SetArg(std::u32string_view name, Arg arg) = 0;
    virtual LocalFunc LookUpFunc(std::u32string_view name) = 0;
    virtual bool SetFunc(const Block& block, common::span<const RawArg> args) = 0;
};

class BasicEvaluateContext : public EvaluateContext, public LocalFuncContext
{
protected:
    std::map<std::u32string, Arg, std::less<>> ArgMap;
    std::map<std::u32string_view, std::tuple<const Block*, uint32_t, uint32_t>, std::less<>> LocalFuncMap;
    std::vector<std::u32string_view> LocalFuncArgNames;
    std::optional<Arg> ReturnArg;
public:
    ~BasicEvaluateContext() override;

    Arg LookUpArg(std::u32string_view name) const override;
    bool SetArg(std::u32string_view name, Arg arg) override;
    LocalFunc LookUpFunc(std::u32string_view name) override;
    bool SetFunc(const Block& block, common::span<const RawArg> args) override;
    Arg GetReturnArg() const override;
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
    enum class ProgramStatus { Next = 0, Repeat, Break, Return, End };
    struct BlockContext
    {
        common::span<const FuncCall> MetaScope;
        const Block& BlockScope;
        ProgramStatus Status = ProgramStatus::Next;
        BlockContext(const Block& block, common::span<const FuncCall> metas) :
            MetaScope(metas), BlockScope(block) { }
    };
    struct ContentContext
    {
        common::span<const FuncCall> MetaScope;
        const BlockContent& ContentScope;
        ContentContext(const BlockContent& content, common::span<const FuncCall> metas) :
            MetaScope(metas), ContentScope(content) { }
    };
    struct FuncTarget
    {
        enum class Type { Empty, Block, Meta };
        constexpr static uintptr_t Flag     = 0x1;
        constexpr static uintptr_t InvFlag  = ~Flag;

        uintptr_t Pointer;
        constexpr FuncTarget() noexcept : Pointer(0) {}
        FuncTarget(BlockContext& ctx) noexcept : Pointer(reinterpret_cast<uintptr_t>(&ctx)) { }
        FuncTarget(const ContentContext& ctx) noexcept : Pointer(reinterpret_cast<uintptr_t>(&ctx) | Flag) { }
        
        constexpr Type GetType() const noexcept 
        {
            if (Pointer == 0)   return Type::Empty;
            if (Pointer & Flag) return Type::Meta;
            else                return Type::Block;
        }
        BlockContext& GetBlockContext() const noexcept 
        { 
            return *reinterpret_cast<BlockContext*>(Pointer);
        }
        const ContentContext& GetContentContext() const noexcept 
        { 
            return *reinterpret_cast<const ContentContext*>(Pointer & InvFlag);
        }
    };
    std::shared_ptr<EvaluateContext> EvalContext;
    
    void ThrowByArgCount(const FuncCall& call, const size_t count) const;
    void ThrowByArgCount(common::span<const Arg> args, const size_t count) const;
    void ThrowByArgType(const Arg& arg, const Arg::InternalType type) const;
    void ThrowByFuncContext(const std::u32string_view func, const FuncTarget target, const FuncTarget::Type type) const;
    bool ThrowIfNotBool(const Arg& arg, const std::u32string_view varName) const;

    virtual bool HandleMetaFuncBefore(const FuncCall& meta, const BlockContent& target, common::span<const FuncCall> metas, BlockContext& ctx);
            bool HandleMetaFuncsBefore(common::span<const FuncCall> metas, const BlockContent& target, BlockContext& ctx);
    virtual void HandleRawBlock(const RawBlock& block, common::span<const FuncCall> metas);

            Arg  EvaluateFunc(const FuncCall& call, common::span<const FuncCall> metas, const FuncTarget target);
    virtual Arg  EvaluateFunc(const std::u32string_view func, common::span<const Arg> args, common::span<const FuncCall> metas, const FuncTarget target);
    virtual Arg  EvaluateArg(const RawArg& arg);
    virtual void ExecuteAssignment(const Assignment& assign, common::span<const FuncCall> metas);
    virtual void ExecuteContent(const BlockContent& content, common::span<const FuncCall> metas, BlockContext& ctx);
    virtual ProgramStatus ExecuteBlock(BlockContext ctx);
public:
    NailangRuntimeBase(std::shared_ptr<EvaluateContext> context);
    ~NailangRuntimeBase();
            void ExecuteBlock(const Block& block, common::span<const FuncCall> metas);
};

}
