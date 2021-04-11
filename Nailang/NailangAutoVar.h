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
    std::u32string TypeName;
    size_t TypeSize;
    common::HashedStringPool<char32_t> NamePool;
    std::vector<std::pair<common::StringPiece<char32_t>, Accessor>> MemberList;
    std::function<void(void*, Arg)> Assigner;
    std::function<std::optional<size_t>(void*, size_t, const Arg&)> ExtendIndexer;
    AutoVarHandlerBase(std::u32string_view typeName, size_t typeSize);
    Accessor* FindMember(std::u32string_view name, bool create = false);
    template<typename T>
    static constexpr auto GetFlag() noexcept
    {
        return common::enum_cast(std::is_const_v<T> ? VarFlags::Empty : VarFlags::NonConst);
    }
public:
    enum class VarFlags : uint16_t { Empty = 0x0, NonConst = 0x1 };
    virtual ~AutoVarHandlerBase();
    AutoVarHandlerBase(const AutoVarHandlerBase&) = delete;
    AutoVarHandlerBase(AutoVarHandlerBase&&) = delete;
    AutoVarHandlerBase& operator=(const AutoVarHandlerBase&) = delete;
    AutoVarHandlerBase& operator=(AutoVarHandlerBase&&) = delete;

    Arg HandleQuery(CustomVar& var, SubQuery& subq, NailangExecutor& executor) override;
    bool HandleAssign(CustomVar& var, Arg val) override;
    std::u32string_view GetTypeName(const CustomVar&) noexcept override;
};

#if COMPILER_MSVC
#   pragma warning(pop)
#endif

MAKE_ENUM_BITFIELD(AutoVarHandlerBase::VarFlags)

template<typename T>
struct AutoVarHandler : public AutoVarHandlerBase
{
private:
    std::vector<std::unique_ptr<AutoVarHandlerBase>> UnnamedHandler;
public:
    AutoVarHandler(std::u32string_view typeName) : AutoVarHandlerBase(typeName, sizeof(T)) 
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
                static constexpr auto Flag = AutoVarHandlerBase::GetFlag<std::remove_pointer_t<X>>();
                const auto fieldPtr = reinterpret_cast<uintptr_t>(accessor(parent));
                return CustomVar{ handlerPtr, static_cast<uint64_t>(fieldPtr), UINT32_MAX, Flag };
            }
            else if constexpr (std::is_same_v<X, common::span<U>> || std::is_same_v<X, common::span<const U>>)
            {
                static constexpr auto Flag = AutoVarHandlerBase::GetFlag<typename X::value_type>();
                const auto sp = accessor(parent);
                const auto elePtr = reinterpret_cast<uintptr_t>(sp.data());
                Expects(sp.size() < SIZE_MAX);
                const auto eleNum = static_cast<uint32_t>(sp.size());
                return CustomVar{ handlerPtr, static_cast<uint64_t>(elePtr), eleNum, Flag };
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
        ExtendIndexer = [indexer = std::move(indexer)](void* ptr, size_t len, const Arg& val)
        {
            const auto host = common::span<const T>(reinterpret_cast<const T*>(ptr), len);
            return indexer(host, val);
        };
    }

    CustomVar CreateVar(const T& obj)
    {
        const auto ptr     = reinterpret_cast<uintptr_t>(&obj);
        return { this, ptr, UINT32_MAX, 0 };
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
        return { this, ptr, eleNum, 0 };
    }
    CustomVar CreateVar(common::span<T> objs)
    {
        auto var = CreateVar(common::span<const T>(objs));
        var.Meta2 = common::enum_cast(VarFlags::NonConst);
        return var;
    }
};


}