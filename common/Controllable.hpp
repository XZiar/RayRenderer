#pragma once
#include "CommonRely.hpp"
#include "ContainerEx.hpp"
#include "Exceptions.hpp"
#include "Linq2.hpp"
#include "STLEx.hpp"
#include "3DBasic/miniBLAS.hpp"
#include <string>
#include <string_view>
#include <map>
#include <set>
#include <variant>
#include <any>
#include <functional>
#include <algorithm>
#include <type_traits>


#if COMPILER_MSVC
#   pragma warning(push)
#   pragma warning(disable:4324)
#endif

namespace common
{

class Controllable
{
    friend struct ControlHelper;
public:
    using ControlArg = std::variant<bool, uint16_t, int32_t, uint32_t, uint64_t, float, std::pair<float, float>, miniBLAS::Vec3, miniBLAS::Vec4, std::string, std::u16string, std::any>;
    enum class ArgType : uint8_t { RawValue, Color, LongText, Enum };
    struct ControlItem
    {
        std::string Id;
        std::string Category;
        std::u16string Name;
        std::u16string Description;
        std::any Cookie;
        std::function<ControlArg(const Controllable&, const std::string&)> Getter;
        std::function<void(Controllable&, const std::string&, const ControlArg&)> Setter;
        std::function<void(Controllable&, const std::string&)> Notifier;
        size_t TypeIdx;
        ArgType Type;
        mutable bool IsEnable;
    };
    template<typename T>
    struct EnumSet
    {
        static_assert(std::is_integral_v<T>, "Currently only intergal supported");
    private:
        std::vector<std::pair<T, std::u16string>> Mapping;
        std::vector<std::pair<std::u16string, T>> Lookup;
        void Prepare()
        {
            std::sort(Mapping.begin(), Mapping.end(), [](const auto& left, const auto& right) { return left.first < right.first; });
            Lookup.clear();
            Lookup.reserve(Mapping.size());
            for (const auto&[key, val] : Mapping)
                Lookup.emplace_back(val, key);
            std::sort(Lookup.begin(), Lookup.end(), [](const auto& left, const auto& right) { return left.first < right.first; });
        }
    public:
        EnumSet() noexcept {}
        EnumSet(std::vector<std::pair<T, std::u16string>>&& enums) : Mapping(std::move(enums))
        {
            Prepare();
        }
        EnumSet(const std::vector<std::pair<T, std::u16string>>& enums) : Mapping(enums)
        {
            Prepare();
        }
        std::vector<std::u16string_view> GetEnumNames() const
        {
            return common::linq::FromIterable(Lookup)
                .Select([](const auto& p) { return std::u16string_view(p.first); })
                .ToVector();
        }
        std::u16string_view ConvertTo(const T key) const
        {
            const auto it = std::lower_bound(Mapping.cbegin(), Mapping.cend(), key,
                [](const auto& left, const auto right) { return left.first < right; });
            if (it != Mapping.cend() && it->first == key)
                return std::u16string_view(it->second);
            return {};
        }
        T ConvertFrom(const std::u16string_view& val) const
        {
            const auto it = std::lower_bound(Lookup.cbegin(), Lookup.cend(), val, 
                [](const auto& left, const auto right) { return left.first < right; });
            if (it != Lookup.cend() && it->first == val)
                return it->second;
            return 0;
        }
    };
private:
    std::map<std::string, std::u16string, std::less<>> Categories;
    std::map<std::string, ControlItem, std::less<>> ControlItems;
    template<typename T>
    class ItemPrep
    {
        friend class Controllable; // gcc&clang need it
        friend class ItemPrepNonType;
        ControlItem& Item;
        ItemPrep(ControlItem& item) : Item(item) {}
        template<typename G>
        static ControlArg PackGetValue(G&& value)
        {
            using GetType = common::remove_cvref_t<G>;
            static_assert(std::is_convertible_v<GetType, T> || std::is_enum_v<std::remove_reference_t<GetType>>, "getter's return cannot be convert to T");
            if constexpr (std::is_constructible_v<ControlArg, T>)
            {
                if constexpr (std::is_enum_v<std::remove_reference_t<GetType>>)
                    return static_cast<T>(value);
                else
                    return static_cast<const T&>(value);
            }
            else
            {
                if constexpr (std::is_enum_v<std::remove_reference_t<GetType>>)
                    return std::any(static_cast<T>(value));
                else
                    return std::any(static_cast<const T&>(value));
            }
        }
        template<typename SetType>
        static SetType UnPackSetValue(const ControlArg& arg)
        {
            static_assert(std::is_convertible_v<T, SetType> || std::is_enum_v<SetType>, "T cannot be convert to setter's arg");
            if constexpr (std::is_constructible_v<ControlArg, T>)
                return static_cast<SetType>(std::get<T>(arg));
            else
                return static_cast<SetType>(std::any_cast<T>(std::get<std::any>(arg)));
        }
        template<typename D>
        constexpr static void CheckSubClass()
        {
            static_assert(std::is_base_of_v<Controllable, common::remove_cvref_t<D>>, "type D should be subclass of Controllable");
        }
    public:
        template<typename Getter>
        ItemPrep<T>& RegistGetter(Getter&& getter) 
        { 
            using GetType = common::remove_cvref_t<std::invoke_result_t<Getter, const Controllable&, const std::string&>>;
            static_assert(std::is_same_v<GetType, T> || std::is_same_v<GetType, ControlArg>, "getter doesnot match item type");
            Item.Getter = getter; 
            return *this;
        }
        template<typename Setter>
        ItemPrep<T>& RegistSetter(Setter&& setter) 
        {
            static_assert(std::is_invocable_v<Setter, Controllable&, const std::string&, ControlArg>, "setter should be void(Controllable&, const std::string&, ControlArg)");
            Item.Setter = setter; 
            return *this;
        }
        template<typename D, typename G>
        ItemPrep<T>& RegistGetter(G(D::*getter)(void) const)
        { 
            CheckSubClass<D>();
            using GetType = common::remove_cvref_t<G>;
            Item.Getter = [getter](const Controllable& obj, const std::string&) 
            { 
                return PackGetValue((dynamic_cast<const D&>(obj).*getter)());
            };
            return *this;
        }
        template<typename D, typename S>
        ItemPrep<T>& RegistSetter(void(D::*setter)(S))
        { 
            CheckSubClass<D>();
            using SetType = common::remove_cvref_t<S>;
            Item.Setter = [setter](Controllable& obj, const std::string&, const ControlArg& arg) 
            { 
                (dynamic_cast<D&>(obj).*setter)(UnPackSetValue<SetType>(arg));
            };
            return *this;
        }
        template<typename D, typename Getter>
        ItemPrep<T>& RegistGetterProxy(Getter&& getter)
        {
            CheckSubClass<D>();
            if constexpr (std::is_invocable_v<Getter, const D&, const std::string&>)
            {
                Item.Getter = [getter](const Controllable& obj, const std::string& id) 
                { 
                    return PackGetValue(getter(dynamic_cast<const D&>(obj), id));
                };
            }
            else if constexpr (std::is_invocable_v<Getter, const D&>)
            {
                Item.Getter = [getter](const Controllable& obj, const std::string&) 
                { 
                    return PackGetValue(getter(dynamic_cast<const D&>(obj)));
                };
            }
            else
                static_assert(!AlwaysTrue<D>, "getter should accept (const D&, const std::string&) or (const D&)");
            return *this;
        }
        template<typename D, typename Setter>
        ItemPrep<T>& RegistSetterProxy(Setter&& setter)
        {
            CheckSubClass<D>();
            if constexpr (std::is_invocable_v<Setter, D&, const std::string&, const T&>)
            {
                Item.Setter = [setter](Controllable& obj, const std::string& id, const ControlArg& arg)
                {
                    setter(dynamic_cast<D&>(obj), id, UnPackSetValue<T>(arg));
                };
            }
            else if constexpr (std::is_invocable_v<Setter, D&, const T&>)
            {
                Item.Setter = [setter](Controllable& obj, const std::string&, const ControlArg& arg)
                {
                    setter(dynamic_cast<D&>(obj), UnPackSetValue<T>(arg));
                };
            }
            else
                static_assert(!AlwaysTrue<D>, "setter should accept (D&, const std::string&, const T&) or (D&, const T&)");
            return *this;
        }
        template<bool CanWrite = true, bool CanRead = true, typename V = T>
        ItemPrep<T>& RegistObject(V& object) 
        { 
            if constexpr (CanWrite)
            {
                static_assert(std::is_convertible_v<T, V>, "object cannot be converted from value of T");
                Item.Setter = [&object](Controllable&, const std::string&, const ControlArg & arg) 
                { 
                    object = UnPackSetValue<V>(arg);
                };
            }
            if constexpr (CanRead)
            {
                static_assert(std::is_convertible_v<V, T>, "object cannot be converted to T");
                Item.Getter = [&object](const Controllable&, const std::string&) { return PackGetValue(object); };
            }
            return *this;
        }
        template<bool CanWrite = true, bool CanRead = true, typename D = Controllable, typename M = T>
        ItemPrep<T>& RegistMember(M D::*member)
        {
            //CheckSubClass<D>(); // disable for Multiple inheritance
            if constexpr (CanWrite)
            {
                static_assert(std::is_convertible_v<T, M>, "object cannot be converted from value of T");
                Item.Setter = [member](Controllable& obj, const std::string&, const ControlArg& arg)
                {
                    dynamic_cast<D&>(obj).*member = UnPackSetValue<M>(arg);
                };
            }
            if constexpr (CanRead)
            {
                static_assert(std::is_convertible_v<M, T>, "object cannot be converted to T");
                Item.Getter = [member](const Controllable& obj, const std::string&) 
                { 
                    return PackGetValue(dynamic_cast<const D&>(obj).*member);
                };
            }
            return *this;
        }
        template<typename D, bool CanWrite = true, bool CanRead = true, typename P = T&(D&)>
        ItemPrep<T>& RegistMemberProxy(P&& proxy)
        {
            CheckSubClass<D>();
            if constexpr (CanWrite)
            {
                using SetType = std::invoke_result_t<P, D&>;
                static_assert(std::is_lvalue_reference_v<SetType>, "object is not a lvalue reference when being set");
                using RawType = std::remove_reference_t<SetType>;
                Item.Setter = [proxy](Controllable& obj, const std::string&, const ControlArg& arg) 
                { 
                    proxy(dynamic_cast<D&>(obj)) = UnPackSetValue<RawType>(arg);
                };
            }
            if constexpr (CanRead)
            {
                Item.Getter = [proxy](const Controllable& obj, const std::string&) 
                { 
                    return PackGetValue(proxy(dynamic_cast<const D&>(obj)));
                };
            }
            return *this;
        }
        template<typename Notifier>
        ItemPrep<T>& RegistNotifier(Notifier notifier)
        {
            static_assert(std::is_invocable_v<Notifier, Controllable&, const std::string&>, "notifier should be void(Controllable&, const std::string&)");
            Item.Notifier = notifier;
            return *this;
        }
        template<typename D, typename Notifier>
        ItemPrep<T>& RegistNotifierProxy(Notifier notifier)
        {
            CheckSubClass<D>();
            if constexpr (std::is_invocable_v<Notifier, D&, const std::string&>)
            {
                Item.Notifier = [notifier](Controllable& obj, const std::string& id) { notifier(dynamic_cast<D&>(obj), id); };
            }
            else if constexpr (std::is_invocable_v<Notifier, D&>)
            {
                Item.Notifier = [notifier](Controllable& obj, const std::string&) { notifier(dynamic_cast<D&>(obj)); };
            }
            else
                static_assert(!AlwaysTrue<D>, "notifier should accept (D&, const std::string&) or (D&)");
            return *this;
        }
        template<typename D, bool CanWrite = true, bool CanRead = true, typename P = T & (D&), typename Notify = void(D&)>
        ItemPrep<T> & RegistMemberProxy(P && proxy, Notify && notifier)
        {
            RegistMemberProxy<D>(std::forward<P>(proxy));
            RegistNotifierProxy<D>(std::forward<Notify>(notifier));
            return *this;
        }
        ItemPrep<T>& SetCookie(const std::any& cookie) { Item.Cookie = cookie; return *this; }
        ItemPrep<T>& SetArgType(const ArgType argType) { Item.Type = argType; return *this; }
    };
    class ItemPrepNonType
    {
        friend class Controllable;
        ControlItem& Item;
        ItemPrepNonType(ControlItem& item) : Item(item) {}
    public:
        template<typename T>
        decltype(auto) AsType()
        {
            using RealType = std::conditional_t<std::is_constructible_v<ControlArg, T>, T, std::any>;
            Item.TypeIdx = common::get_variant_index_v<RealType, ControlArg>();
            return ItemPrep<RealType>(Item);
        }
    };
protected:
    Controllable() {}

    void AddCategory(const std::string category, const std::u16string name)
    {
        Categories[category] = name;
    }
    ItemPrepNonType RegistItem(const std::string& id, const std::string& category, const std::u16string& name, 
        const ArgType argType = ArgType::RawValue, const std::any& cookie = {}, const std::u16string& description = u"")
    {
        Categories.try_emplace(category, category.cbegin(), category.cend());
        auto it = ControlItems.insert_or_assign(id, ControlItem{ id, category, name, description, cookie, {}, {}, {}, std::variant_npos, argType, true }).first;
        return it->second;
    }
    template<typename T>
    decltype(auto) RegistItem(const std::string& id, const std::string& category, const std::u16string& name, 
        const ArgType argType = ArgType::RawValue, const std::any& cookie = {}, const std::u16string& description = u"")
    {
        return RegistItem(id, category, name, argType, cookie, description).AsType<T>();
    }
    virtual std::u16string_view GetControlType() const = 0;
public:
    virtual ~Controllable() {}
    
};

struct ControlHelper
{
    static std::u16string_view GetControlType(const Controllable& control) { return control.GetControlType(); }
    static const auto& GetControlItems(const Controllable& control) { return control.ControlItems; }
    static const auto& GetCategories(const Controllable& control) { return control.Categories; }
    static const Controllable::ControlItem* GetControlItem(const Controllable& control, const std::string& id)
    {
        return common::container::FindInMap(control.ControlItems, id);
    }

    static Controllable::ControlArg ControllableGet(const Controllable& control, const std::string& id)
    {
        const auto item = GetControlItem(control, id);
        if (!item)
            COMMON_THROW(BaseException, u"No such target found for Controller");
        return item->Getter(control, id);
    }
    template<typename T>
    static T ControllableGet(const Controllable& control, const std::string& id)
    {
        return std::get<T>(ControllableGet(control, id));
    }

    static void ControllableSet(Controllable& control, const std::string& id, const Controllable::ControlArg& arg)
    {
        const auto item = GetControlItem(control, id);
        if (!item)
            COMMON_THROW(BaseException, u"No such target found for Controller");
        return item->Setter(control, id, arg);
    }
};

}

#if COMPILER_MSVC
#   pragma warning(pop)
#endif
