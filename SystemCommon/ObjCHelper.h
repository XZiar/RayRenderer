#pragma once

#include "SystemCommonRely.h"
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <objc/NSObjCRuntime.h>
#include <cmath>

namespace common::objc
{

namespace detail 
{
// see https://github.com/garettbass/oc/blob/master/detail/message.h
#if COMMON_ARCH_ARM
#   if COMMON_OSBIT == 64
inline constexpr auto LargeStructSize = sizeof(void*) * 8; // r0...r8
#   else
inline constexpr auto LargeStructSize = sizeof(void*) * 1; // r0
#   endif
#elif COMMON_ARCH_X86
inline constexpr auto LargeStructSize = sizeof(void*) * 2; // r(e)ax, r(e)dx 
#endif
template<typename R>
static constexpr auto ObjCMsgSend()
{
    // Struct-returning Messaging Primitives
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

template<typename R>
static constexpr auto ObjCMsgSendSuper()
{
    // Struct-returning Messaging Primitives
    if constexpr (std::is_class_v<R>)
        if(sizeof(R) > LargeStructSize)
            return objc_msgSendSuper_stret;

//     // Floating-point-returning Messaging Primitives
// #if defined(__i386__) || defined(__x86_64__)
//     if constexpr (std::is_same_v<R, long double>)
//         return objc_msgSendSuper_fpret;
// #endif
// #if defined(__i386__)
//     if constexpr (std::is_same_v<R, float> || std::is_same_v<R, double>)
//         return objc_msgSendSuper_fpret;
// #endif

    // Basic Messaging Primitives
    return objc_msgSendSuper;
}
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
class Instance;
template<typename... Args>
class ClzInit;
class ClassBuilder;


class Instance
{
protected:
    id Self;
protected:
    template<typename R, typename... Args, typename T>
    forceinline R CallSuper(Class superclz, const T func, const Args&... args) const
    {
        SEL funcSel;
        if constexpr (std::is_same_v<T, SEL>) funcSel = func;
        else funcSel = sel_registerName(func);
        objc_super super{ Self, superclz };
        return reinterpret_cast<R (*)(objc_super*, SEL, Args...)>(detail::ObjCMsgSendSuper<R>())(&super, funcSel, args...);
    }
    template<typename T>
    [[nodiscard]] T& GetExtra() const
    {
        return *reinterpret_cast<T*>(object_getIndexedIvars(Self));
    }
public:
    constexpr Instance(id self = nullptr) noexcept : Self(self)
    { }
    void Release() const noexcept
    {
        Call<void>(CommonSel::Get().Release);
    }
    void AutoRelease() const noexcept
    {
        Call<void>(CommonSel::Get().AutoRelease);
    }
    template<typename R, typename... Args, typename T>
    forceinline R Call(const T func, const Args&... args) const
    {
        SEL funcSel;
        if constexpr (std::is_same_v<T, SEL>) funcSel = func;
        else funcSel = sel_registerName(func);
        return reinterpret_cast<R (*)(id, SEL, Args...)>(detail::ObjCMsgSend<R>())(Self, funcSel, args...);
    }
    template<typename T>
    [[nodiscard]] T GetVar(const Ivar var) const
    {
        const auto getter = reinterpret_cast<T (*)(id, Ivar)>(object_getIvar);
        return getter(Self, var);
    }
    template<typename T>
    [[nodiscard]] T GetVar(const char* varname) const
    {
        return GetVar<T>(object_getInstanceVariable(Self, varname, nullptr));
    }
    template<typename T>
    void SetVar(const Ivar var, const T& val)
    {
        const auto setter = reinterpret_cast<void (*)(id, Ivar, T)>(object_setIvar);
        setter(Self, var, val);
    }
    template<typename T>
    void SetVar(const char* varname, const T& val)
    {
        const auto setter = reinterpret_cast<void (*)(id, const char*, T)>(object_setInstanceVariable);
        setter(Self, varname, val);
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
        return reinterpret_cast<R (*)(Class, SEL, Args...)>(detail::ObjCMsgSend<R>())(TheClz, funcSel, args...);
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
            instance.AutoRelease();
        return static_cast<id>(instance);
    }
};


namespace detail
{
// see https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Articles/ocrtTypeEncodings.html
template<typename T>
inline constexpr char GetTypeEncoding()
{
         if constexpr (std::is_same_v<T, char>)                 return 'c';
    else if constexpr (std::is_same_v<T, BOOL>)                 return 'c';
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
    else if constexpr (std::is_same_v<T, char*>)                return '*';
    else if constexpr (std::is_same_v<T, id>)                   return '@';
    else if constexpr (std::is_base_of_v<id, T>)                return '@';
    else if constexpr (std::is_same_v<T, Class>)                return '#';
    else if constexpr (std::is_same_v<T, SEL>)                  return ':';
    else                                                        return '?';
}
template<typename T, bool AllowUnknown, size_t N>
inline constexpr void PutTypeEncoding(std::array<char, N>& dst, size_t& idx)
{
    constexpr auto ch = GetTypeEncoding<T>();
    if constexpr (ch == '?')
    {
        if constexpr (std::is_pointer_v<T>)
        {
            dst[idx++] = '^';
            PutTypeEncoding<std::remove_pointer_t<T>, true>(dst, idx);
        }
        else if constexpr (!AllowUnknown)
            static_assert(!common::AlwaysTrue<T>, "unsupported type");
        else
            dst[idx++] = ch;
    }
    else
        dst[idx++] = ch;
}
template<typename T>
struct TyperEncoder
{
    static constexpr std::array<char, 8> GetType()
    {
        std::array<char, 8> ret = { '\0' };
        size_t idx = 0;
        PutTypeEncoding<T, false>(ret, idx);
        return ret;
    }
};
template<typename R, typename... Args>
struct TyperEncoder<R(*)(Args...)>
{
    static constexpr std::array<char, 64> GetType()
    {
        std::array<char, 64> ret = { '\0' };
        size_t idx = 0;
        PutTypeEncoding<R, false>(ret, idx);
        (..., PutTypeEncoding<Args, false>(ret, idx));
        return ret;
    }
};
}

struct CommonClz
{
    Clz NSObject;
    CommonClz();
    SYSCOMMONAPI static const CommonClz& Get() noexcept;
};

class ClassBuilder
{
private:
    Class TheClz;
    bool HasFinished = false;
public:
    ClassBuilder(const char* name, Clz baseClass = CommonClz::Get().NSObject) : TheClz(objc_allocateClassPair(baseClass, name, 0))
    { }
    bool AddProtocol(const char* name)
    {
        Expects(!HasFinished);
        Protocol* protocol = objc_getProtocol(name);
        return class_addProtocol(TheClz, protocol) == YES;
    }
    template<typename T>
    bool AddVariable(const char* name)
    {
        Expects(!HasFinished);
        constexpr auto typestr = detail::TyperEncoder<T>::GetType();
        return class_addIvar(TheClz, name, sizeof(T), static_cast<uint8_t>(std::log2(alignof(T))), typestr.data()) == YES;
    }
    template<typename T>
    bool AddMethod(const char* name, T&& func)
    {
        Expects(!HasFinished);
        SEL selMethod = sel_registerName(name);
        constexpr auto typestr = detail::TyperEncoder<T>::GetType();
        return class_addMethod(TheClz, selMethod, (IMP)func, typestr.data()) == YES;
    }
    void Finish() noexcept
    {
        if (!HasFinished)
        {
            HasFinished = true;
            objc_registerClassPair(TheClz);
        }
    }
    Clz GetClass() const noexcept
    {
        Expects(HasFinished);
        return TheClz;
    }
    template<typename... Args>
    ClzInit<Args...> GetClassInit(bool autoRelease, const char* init = nullptr) const
    {
        Expects(HasFinished);
        return { TheClz, autoRelease, init };
    }
};


}