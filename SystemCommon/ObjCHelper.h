#pragma once

#include "SystemCommonRely.h"
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <objc/NSObjCRuntime.h>

namespace common::objc
{

template<typename R>
static constexpr auto ObjCMsgSendFunc()
{
    // Struct-returning Messaging Primitives
    // see https://github.com/garettbass/oc/blob/master/detail/message.h
#if COMMON_ARCH_ARM
#   if COMMON_OSBIT == 64
    constexpr auto LargeStructSize = sizeof(void*) * 8; // r0...r8
#   else
    constexpr auto LargeStructSize = sizeof(void*) * 1; // r0
#   endif
#elif COMMON_ARCH_X86
    constexpr auto LargeStructSize = sizeof(void*) * 2; // r(e)ax, r(e)dx 
#endif
    if constexpr (std::is_class_v<R>)
        if(sizeof(R) > LargeStructSize)
            return objc_msgSend_stret;

    // Floating-point-returning Messaging Primitives
#if defined(__i386__) || defined(__x86_64__)
    if constexpr (std::is_same_v<R, long double>)
        return objc_msgSend_fpret;
#endif
#if defined(__i386__)
    if constexpr (std::is_same_v<R, float> || std::is_same_v<R, double>)
        return objc_msgSend_fpret;
#endif

    // Basic Messaging Primitives
    return objc_msgSend;
}


struct CommonSel
{
    SEL Alloc;
    SEL Init;
    SEL AutoRelease;
    SEL Release;
    CommonSel();
    SYSCOMMONAPI static const CommonSel& Get() noexcept;
};

class Clz;
template<typename... Args>
class ClzInit;
class ClassBuilder;

class Instance
{
protected:
    id Self;
public:
    constexpr Instance(id self) noexcept : Self(self)
    { }
    template<typename R, typename... Args, typename T>
    forceinline R Call(const T func, const Args&... args) const
    {
        SEL funcSel;
        if constexpr (std::is_same_v<T, SEL>) funcSel = func;
        else funcSel = sel_registerName(func);
        return reinterpret_cast<R (*)(id, SEL, Args...)>(ObjCMsgSendFunc<R>())(Self, funcSel, args...);
    }
    constexpr operator id() const noexcept { return Self; }
    constexpr explicit operator bool() const noexcept { return Self != nullptr; }
};

class Clz
{
    friend ClassBuilder;
protected:
    Class TheClz;
    Clz(Class clz) : TheClz(clz) { }
public:
    Clz(const char* name) : Clz(objc_getClass(name)) { }
    template<typename R, typename... Args, typename T>
    forceinline R Call(const T func, const Args&... args) const
    {
        SEL funcSel;
        if constexpr (std::is_same_v<T, SEL>) funcSel = func;
        else funcSel = sel_registerName(func);
        return reinterpret_cast<R (*)(Class, SEL, Args...)>(ObjCMsgSendFunc<R>())(TheClz, funcSel, args...);
    }
    constexpr operator Class() const noexcept { return TheClz; }
};

template<typename... Args>
class ClzInit : public Clz
{
    friend ClassBuilder;
private:
    static SEL GenInit(const char* init) noexcept { return init == nullptr ? CommonSel::Get().Init : sel_registerName(init); } 
    SEL Init;
    bool AutoRelease;
    ClzInit(Class clz, const bool autoRelease, const char* init = nullptr) : 
        Clz(clz), Init(GenInit(init)), AutoRelease(autoRelease) { }
public:
    ClzInit(const char* name, const bool autoRelease, const char* init = nullptr) : 
        Clz(name), Init(GenInit(init)), AutoRelease(autoRelease) { }
    ClzInit(Clz clz, const bool autoRelease, const char* init = nullptr) : 
        ClzInit(static_cast<Class>(clz), autoRelease, init) { }
    template<typename T = Instance>
    T Create(const Args&... args) const
    {
        const auto& sels = CommonSel::Get();
        const Instance alloc  = Call<id>(sels.Alloc);
        const Instance instance = alloc.Call<id, Args...>(Init, args...);
        if (AutoRelease)
            instance.Call<void>(sels.AutoRelease);
        return static_cast<id>(instance);
    }
};


// see https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Articles/ocrtTypeEncodings.html
template<typename T>
static constexpr char GetTypeEncoding()
{
         if constexpr (std::is_same_v<T, char>)                 return 'c';
    else if constexpr (std::is_same_v<T, int>)                  return 'i';
    else if constexpr (std::is_same_v<T, short>)                return 's';
    else if constexpr (std::is_same_v<T, long>)                 return 'l';
    else if constexpr (std::is_same_v<T, long long>)            return 'q';
    else if constexpr (std::is_same_v<T, unsigned char>)        return 'C';
    else if constexpr (std::is_same_v<T, unsigned int>)         return 'I';
    else if constexpr (std::is_same_v<T, unsigned short>)       return 'S';
    else if constexpr (std::is_same_v<T, unsigned long>)        return 'L';
    else if constexpr (std::is_same_v<T, unsigned long long>)   return 'Q';
    else if constexpr (std::is_same_v<T, float>)                return 'f';
    else if constexpr (std::is_same_v<T, double>)               return 'd';
    else if constexpr (std::is_same_v<T, bool>)                 return 'B';
    else if constexpr (std::is_same_v<T, void>)                 return 'v';
    else if constexpr (std::is_same_v<T, const char*>)          return '*';
    else if constexpr (std::is_same_v<T, id>)                   return '@';
    else if constexpr (std::is_base_of_v<id, T>)                return '@';
    else if constexpr (std::is_same_v<T, Class>)                return '#';
    else if constexpr (std::is_same_v<T, SEL>)                  return ':';
    else static_assert(!common::AlwaysTrue<T>, "unsupported type");
}
template<typename T> struct MethodTyper;
template<typename R, typename... Args> struct MethodTyper<R(*)(Args...)>
{
    template<size_t I> using Arg = std::tuple_element_t<I, std::tuple<Args...>>;
    template<size_t... Idx>
    static constexpr void PutArgs(std::array<char, 64>& ret, std::index_sequence<Idx...>) noexcept
    {
        (..., void(ret[Idx + 1] = GetTypeEncoding<std::decay_t<Arg<Idx>>>()));
    }
    static constexpr std::array<char, 64> GetType()
    {
        constexpr auto ArgCount = sizeof...(Args);
        static_assert(ArgCount < 60, "too many args");
        std::array<char, 64> ret = { '\0' };
        ret[0] = GetTypeEncoding<R>();
        PutArgs(ret, std::make_index_sequence<ArgCount>{});
        return ret;
    }
};

struct CommonClz
{
    Clz NSObject;
    CommonClz();
    SYSCOMMONAPI static const CommonClz& Get() noexcept;
};

class ClassBuilder
{
    Class TheClz;
public:
    ClassBuilder(const char* name) : TheClz(objc_allocateClassPair(CommonClz::Get().NSObject, name, 0))
    { }
    bool AddProtocol(const char* name)
    {
        Protocol* protocol = objc_getProtocol(name);
        return class_addProtocol(TheClz, protocol);
    }
    template<typename T>
    bool AddMethod(const char* name, T&& func)
    {
        SEL selMethod = sel_registerName(name);
        constexpr auto typestr = MethodTyper<T>::GetType();
        return class_addMethod(TheClz, selMethod, (IMP)func, typestr.data());
    }
    Clz GetClass() const noexcept
    {
        return TheClz;
    }
    template<typename... Args>
    ClzInit<Args...> GetClassInit(bool autoRelease, const char* init = nullptr) const
    {
        return { TheClz, autoRelease, init };
    }
};


}