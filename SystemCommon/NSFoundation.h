#pragma once
#include "ObjCHelper.h"

namespace common::foundation
{

using unichar = unsigned short;


struct NSAutoreleasePool : private objc::Instance
{
    SYSCOMMONAPI NSAutoreleasePool();
    SYSCOMMONAPI ~NSAutoreleasePool();
};


struct NSDate : public objc::Instance
{
private:
    using Instance::Instance;
public:
    SYSCOMMONAPI static const NSDate& DistantPast() noexcept;
    SYSCOMMONAPI static const NSDate& DistantFuture() noexcept;
};

struct NSString : public objc::Instance
{
    using Instance::Instance;
    SYSCOMMONAPI size_t Size() const noexcept;
    SYSCOMMONAPI char16_t operator[](size_t idx) const;
    SYSCOMMONAPI std::u16string ToStr() const;
    SYSCOMMONAPI NSString Copy() const;
    SYSCOMMONAPI static NSString Create();
    SYSCOMMONAPI static NSString Create(std::u16string_view text);
};

struct NSNotification : public objc::Instance
{
    objc::Instance Target;
    SYSCOMMONAPI NSNotification(id instance);
};

struct NSNumber
{
    SYSCOMMONAPI static objc::Instance FromBool(BOOL val);
    SYSCOMMONAPI static objc::Instance FromI16( int16_t val);
    SYSCOMMONAPI static objc::Instance FromU16(uint16_t val);
    SYSCOMMONAPI static objc::Instance FromI32( int32_t val);
    SYSCOMMONAPI static objc::Instance FromU32(uint32_t val);
    SYSCOMMONAPI static objc::Instance FromI64( int64_t val);
    SYSCOMMONAPI static objc::Instance FromU64(uint64_t val);
    SYSCOMMONAPI static objc::Instance FromF32(float    val);
    SYSCOMMONAPI static objc::Instance FromF64(double   val);
};

namespace detail
{
class NSArrayBase : public objc::Instance
{
protected:
    SYSCOMMONAPI id At(size_t idx) const;
    SYSCOMMONAPI Class At2(size_t idx) const;
    SYSCOMMONAPI void PushBack(id obj);
    SYSCOMMONAPI void PushBack(Class obj);
    SYSCOMMONAPI void Insert(size_t idx, id obj);
    SYSCOMMONAPI void Insert(size_t idx, Class obj);
    SYSCOMMONAPI static id Create();
public:
    using Instance::Instance;
    SYSCOMMONAPI size_t Count() const;
};
class NSDictionaryBase : public objc::Instance
{
protected:
    SYSCOMMONAPI id Get(id key) const;
    SYSCOMMONAPI void Insert(id key, id val);
    SYSCOMMONAPI static id Create();
public:
    using Instance::Instance;
    SYSCOMMONAPI size_t Count() const;
};
}

template<typename T = objc::Instance>
struct NSArray : public detail::NSArrayBase
{
    using NSArrayBase::NSArrayBase;
    T operator[](size_t idx) const
    {
        if constexpr (std::is_same_v<T, Class> || std::is_base_of_v<objc::Clz, T>)
            return At2(idx);
        else
            return At(idx);
    }
    bool Empty() const { return Count() == 0; }
};
template<typename T = objc::Instance>
struct NSMutArray : public NSArray<T>
{
    using NSArray<T>::NSArray;
    void PushBack(T obj) { detail::NSArrayBase::PushBack(obj); }
    void Insert(size_t idx, T obj) { detail::NSArrayBase::Insert(idx, obj); }
    static NSMutArray Create() { return detail::NSArrayBase::Create(); }
};

template<typename K = objc::Instance, typename V = objc::Instance>
struct NSDictionary : public detail::NSDictionaryBase
{
    using NSDictionaryBase::NSDictionaryBase;
    V operator[](const K& key) const
    {
        return Get(key);
    }
    bool Empty() const { return Count() == 0; }
};
template<typename K = objc::Instance, typename V = objc::Instance>
struct NSMutDictionary : public NSDictionary<K, V>
{
    using NSDictionary<K, V>::NSDictionary;
    void Insert(const K& key, const V val) { detail::NSDictionaryBase::Insert(key, val); }
    static NSMutDictionary Create() { return detail::NSDictionaryBase::Create(); }
};

}
