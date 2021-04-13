#pragma once

#include "NailangStruct.h"


namespace xziar::nailang
{


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4275 4251)
#endif

class NAILANGAPI AutoVarHandlerBase : public CustomVar::Handler
{
protected:
    struct NAILANGAPI Accessor
    {
        using TAuto = std::function<CustomVar(void*)>;
        using TSimp = std::function<Arg(void*)>;
        enum class Type : uint8_t { Empty = 0, Auto, Proxy, Direct };
        union
        {
            uint8_t Dummy;
            TAuto AutoMember;
            TSimp SimpleMember;
        };
        Type TypeData;
        bool IsConst;
        Accessor() noexcept;
        Accessor(const Accessor&) = delete;
        Accessor(Accessor&& other) noexcept;
        Accessor& operator=(const Accessor&) = delete;
        Accessor& operator=(Accessor&& other) = delete;
        void SetCustom(std::function<CustomVar(void*)> accessor) noexcept;
        void SetProxy (std::function<Arg(void*)> getset, bool isConst) noexcept;
        void SetDirect(std::function<Arg(void*)> getter) noexcept;
        void Release() noexcept;
        ~Accessor();
    };
    class AccessorBuilder
    {
        Accessor& Host;
    public:
        constexpr AccessorBuilder(Accessor& host) noexcept : Host(host) {}
        NAILANGAPI AccessorBuilder& SetConst(bool isConst = true);
    };
    class NAILANGAPI AutoVarArrayHandler : public CustomVar::Handler
    {
    protected:
        AutoVarHandlerBase& Parent;
        std::u32string TypeName;
        AutoVarArrayHandler(AutoVarHandlerBase& parent);
        ~AutoVarArrayHandler();
        std::function<std::optional<size_t>(void*, size_t, const Arg&)> ExtendIndexer;
        [[nodiscard]] virtual uintptr_t OffsetPtr(uintptr_t base, size_t idx) const noexcept = 0;
        [[nodiscard]] Arg HandleSubFields(const CustomVar&, SubQuery<const Expr>&) override;
        [[nodiscard]] Arg HandleIndexes(const CustomVar&, SubQuery<Arg>&) override;
        [[nodiscard]] std::u32string_view GetTypeName(const CustomVar&) noexcept final;
    };
    std::u32string TypeName;
    common::HashedStringPool<char32_t> NamePool;
    std::vector<std::pair<common::StringPiece<char32_t>, Accessor>> MemberList;
    std::function<void(void*, Arg)> Assigner;
    AutoVarHandlerBase(std::u32string_view typeName);
    [[nodiscard]] Accessor* FindMember(std::u32string_view name, bool create = false);
public:
    enum class VarFlags : uint16_t { Empty = 0x0, NonConst = 0x1 };
    virtual ~AutoVarHandlerBase();
    AutoVarHandlerBase(const AutoVarHandlerBase&) = delete;
    AutoVarHandlerBase(AutoVarHandlerBase&&) = delete;
    AutoVarHandlerBase& operator=(const AutoVarHandlerBase&) = delete;
    AutoVarHandlerBase& operator=(AutoVarHandlerBase&&) = delete;

    [[nodiscard]] Arg HandleSubFields(const CustomVar&, SubQuery<const Expr>&) override;
    [[nodiscard]] Arg HandleIndexes(const CustomVar&, SubQuery<Arg>&) override;
    [[nodiscard]] bool HandleAssign(CustomVar& var, Arg val) override;
    [[nodiscard]] std::u32string_view GetTypeName(const CustomVar&) noexcept override;
};

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

MAKE_ENUM_BITFIELD(AutoVarHandlerBase::VarFlags)

template<typename T>
struct AutoVarHandler : public AutoVarHandlerBase
{
private:
    struct ArrayHandler final : public AutoVarArrayHandler
    {
        friend AutoVarHandler<T>;
        using AutoVarArrayHandler::AutoVarArrayHandler;
        [[nodiscard]] uintptr_t OffsetPtr(uintptr_t base, size_t idx) const noexcept 
        {
            return reinterpret_cast<uintptr_t>(reinterpret_cast<T*>(base) + idx);
        }
    };
    std::vector<std::unique_ptr<AutoVarHandlerBase>> UnnamedHandler;
    ArrayHandler TheArrayHandler;
public:
    AutoVarHandler(std::u32string_view typeName) : AutoVarHandlerBase(typeName), TheArrayHandler(*this)
    { }
    ~AutoVarHandler() override {}
    template<typename U, typename F>
    forceinline AccessorBuilder AddAutoMember(std::u32string_view name, AutoVarHandler<U>& handler, F&& accessor)
    {
        const auto dst = FindMember(name, true);
        dst->SetCustom([handlerPtr = &handler, accessor = std::move(accessor)](void* ptr) -> CustomVar
        {
            auto& parent = *reinterpret_cast<T*>(ptr);
            using X = std::invoke_result_t<F, T&>;
            if constexpr (std::is_same_v<X, U*> || std::is_same_v<X, const U*>)
            {
                const auto ret = accessor(parent);
                return handlerPtr->CreateVar(*ret);
            }
            else if constexpr (std::is_same_v<X, common::span<U>> || std::is_same_v<X, common::span<const U>>)
            {
                const auto sp = accessor(parent);
                return handlerPtr->CreateVar(sp);
            }
            else
            {
                static_assert(!common::AlwaysTrue<X>, "accessor should accept T& and return U* or span<U>");
            }
        });
        return *dst;
    }
    template<typename U, typename F, typename R>
    forceinline AccessorBuilder AddAutoMember(std::u32string_view name, std::u32string_view typeName, F&& accessor, R&& initer)
    {
        using H = AutoVarHandler<U>;
        static_assert(std::is_invocable_v<R, H&>, "initer need to accept autohandler of U");
        UnnamedHandler.push_back(std::make_unique<H>(typeName));
        auto& handler = *static_cast<H*>(UnnamedHandler.back().get());
        initer(handler);
        return AddAutoMember<U, F>(name, handler, std::forward<F>(accessor));
    }
    template<typename F>
    forceinline AccessorBuilder AddCustomMember(std::u32string_view name, F&& accessor)
    {
        static_assert(std::is_invocable_r_v<CustomVar, F, T&>, "accessor should accept T& and return CustomVar");
        const auto dst = FindMember(name, true);
        dst->SetCustom([accessor = std::move(accessor)](void* ptr) -> CustomVar
        {
            auto& parent = *reinterpret_cast<T*>(ptr);
            return accessor(parent);
        });
        return *dst;
    }
    template<typename F>
    forceinline AccessorBuilder AddSimpleMember(std::u32string_view name, F&& accessor)
    {
        static_assert(std::is_invocable_v<F, T&>, "accessor should accept T&");
        using R = std::invoke_result_t<F, T&>;
        static constexpr bool IsPtr = std::is_pointer_v<R>;
        using U = std::remove_pointer_t<R>;
        static constexpr auto Type = NativeWrapper::GetType<std::remove_const_t<U>>();

        const auto dst = FindMember(name, true);
        if constexpr (IsPtr)
        {
            static constexpr bool IsConst = std::is_const_v<U>;
            dst->SetProxy([accessor = std::forward<F>(accessor), type = Type](void* ptr)->Arg
            {
                auto& parent = *reinterpret_cast<T*>(ptr);
                return NativeWrapper::GetLocator(type, reinterpret_cast<uintptr_t>(accessor(parent)), IsConst, 0);
            }, IsConst);
        }
        else if constexpr (std::is_same_v<U, Arg>) // direct return
        {
            dst->SetDirect([accessor = std::forward<F>(accessor)](void* ptr)->Arg
            {
                auto& parent = *reinterpret_cast<T*>(ptr);
                return accessor(parent);
            });
        }
        else
        {
            dst->SetDirect([accessor = std::forward<F>(accessor), type = Type](void* ptr)->Arg
            {
                auto& parent = *reinterpret_cast<T*>(ptr);
                auto ret = accessor(parent);
                auto tmp = NativeWrapper::GetLocator(type, reinterpret_cast<uintptr_t>(&ret), true, 0);
                return tmp.Get();
            });
        }
        return *dst;
    }
    template<typename F>
    void SetAssigner(F&& assigner)
    {
        static_assert(std::is_invocable_r_v<void, F, T&, Arg>, "assigner should accept T&");
        //using R = std::invoke_result_t<F, T&, Arg>;
        Assigner = [assigner = std::move(assigner)](void* ptr, Arg val)
        {
            auto& host = *reinterpret_cast<T*>(ptr);
            //if constexpr (std::is_void_v<R>)
                assigner(host, std::move(val));
            /*else
            {
                const bool ret = assigner(host, std::move(val));
                if (!ret)

            }*/
        };
    }
    template<typename F>
    void SetExtendIndexer(F&& indexer)
    {
        static_assert(std::is_invocable_r_v<std::optional<size_t>, F, common::span<const T>, const Arg&>, 
            "indexer should accept span<T>, Arg, Expr and return index");
        TheArrayHandler.ExtendIndexer = [indexer = std::move(indexer)](void* ptr, size_t len, const Arg& val)
        {
            const auto host = common::span<const T>(reinterpret_cast<const T*>(ptr), len);
            return indexer(host, val);
        };
    }

    CustomVar CreateVar(const T& obj)
    {
        const auto ptr = reinterpret_cast<uintptr_t>(&obj);
        return { this, ptr, 0, 0 };
    }
    CustomVar CreateVar(T& obj)
    {
        auto var = CreateVar(const_cast<const T&>(obj));
        var.Meta2 = common::enum_cast(VarFlags::NonConst);
        return var;
    }
    CustomVar CreateVar(common::span<const T> objs)
    {
        Expects(objs.size() < UINT32_MAX);
        const auto ptr     = reinterpret_cast<uintptr_t>(objs.data());
        const auto eleNum  = static_cast<uint32_t>(objs.size());
        return { &TheArrayHandler, ptr, eleNum, 0 };
    }
    CustomVar CreateVar(common::span<T> objs)
    {
        auto var = CreateVar(common::span<const T>(objs));
        var.Meta2 = common::enum_cast(VarFlags::NonConst);
        return var;
    }
};


}