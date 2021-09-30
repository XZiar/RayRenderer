#include "SystemCommonPch.h"
#include "ObjCHelper.h"
#include "NSFoundation.h"

namespace common
{
namespace objc
{

CommonSel::CommonSel() : 
    Alloc       (sel_registerName("alloc")),
    Init        (sel_registerName("init")),
    AutoRelease (sel_registerName("autorelease")),
    Release     (sel_registerName("release"))
{ }
const CommonSel& CommonSel::Get() noexcept
{
    static CommonSel Sels;
    return Sels;
}


CommonClz::CommonClz() : 
    NSObject("NSObject")
{ }
const CommonClz& CommonClz::Get() noexcept
{
    static CommonClz Clzs;
    return Clzs;
}

}

namespace foundation
{
using namespace objc;

struct NSRange 
{
    NSUInteger location;
    NSUInteger length;
};


struct Foundations
{
    ClzInit<> AutoreleasePool;
    Clz String;
    Clz Date;
    Clz Number;
    ClzInit<> MutableArray;
    ClzInit<> MutableDictionary;
    SEL Count;
    Foundations() : 
        AutoreleasePool("NSAutoreleasePool", false),
        String("NSString"),
        Date("NSDate"),
        Number("NSNumber"),
        MutableArray("NSMutableArray", false),
        MutableDictionary("NSMutableDictionary", false),
        Count(sel_registerName("count"))
    { }
    static const Foundations& Get() noexcept
    {
        static Foundations Clzs;
        return Clzs;
    }
};


NSAutoreleasePool::NSAutoreleasePool() : Instance(Foundations::Get().AutoreleasePool.Create()) {}
NSAutoreleasePool::~NSAutoreleasePool()
{
    static SEL Drain = sel_registerName("drain");
    Call<void>(Drain);
}


const NSDate& NSDate::DistantPast() noexcept
{
    static const NSDate ret = Foundations::Get().Date.Call<id>("distantPast");
    return ret;
}
const NSDate& NSDate::DistantFuture() noexcept
{
    static const NSDate ret = Foundations::Get().Date.Call<id>("distantFuture");
    return ret;
}


size_t NSString::Size() const noexcept
{
    static SEL Len = sel_registerName("length");
    return Call<NSUInteger>(Len);
}
char16_t NSString::operator[](size_t idx) const
{
    static SEL ChAt = sel_registerName("characterAtIndex:");
    return static_cast<char16_t>(Call<unichar, NSUInteger>(ChAt, idx));
}
std::u16string NSString::ToStr() const
{
    static SEL GetChRange = sel_registerName("getCharacters:range:");
    const auto size = Size();
    std::u16string ret(size, u'\0');
    NSRange range{ 0, size };
    Call<void>(GetChRange, reinterpret_cast<unichar*>(ret.data()), range);
    return ret;
}
NSString NSString::Copy() const
{
    static const ClzInit<id> NSStringInit(Foundations::Get().String, false, "initWithString:");
    return NSStringInit.Create<NSString>(Self);
}

NSString NSString::Create()
{
    static SEL EmptyStr = sel_registerName("string");
    return Foundations::Get().String.Call<id>(EmptyStr);
}
NSString NSString::Create(std::u16string_view text)
{
    static_assert(sizeof(NSUInteger) == sizeof(size_t));
    static const ClzInit<const unichar*, NSUInteger> NSStringInit(
        Foundations::Get().String, false, "initWithCharacters:length:");
    return NSStringInit.Create<NSString>(reinterpret_cast<const unichar*>(text.data()), text.size());
}


NSNotification::NSNotification(id instance) : Instance(instance), Target(nullptr)
{
    static SEL SelObject = sel_registerName("object");
    Target = Call<id>(SelObject);
}


Instance NSNumber::FromBool(BOOL val)
{
    static SEL Create = sel_registerName("numberWithBool:");
    return Foundations::Get().Number.Call<id, BOOL>(Create, val);
}
Instance NSNumber::FromI16( int16_t val)
{
    static SEL Create = sel_registerName("numberWithShort:");
    return Foundations::Get().Number.Call<id, int16_t>(Create, val);
}
Instance NSNumber::FromU16(uint16_t val)
{
    static SEL Create = sel_registerName("numberWithUnsignedShort:");
    return Foundations::Get().Number.Call<id, uint16_t>(Create, val);
}
Instance NSNumber::FromI32( int32_t val)
{
    static SEL Create = sel_registerName("numberWithInt:");
    return Foundations::Get().Number.Call<id, int32_t>(Create, val);
}
Instance NSNumber::FromU32(uint32_t val)
{
    static SEL Create = sel_registerName("numberWithUnsignedInt:");
    return Foundations::Get().Number.Call<id, uint32_t>(Create, val);
}
Instance NSNumber::FromI64( int64_t val)
{
    static SEL Create = sel_registerName("numberWithLong:");
    return Foundations::Get().Number.Call<id, int64_t>(Create, val);
}
Instance NSNumber::FromU64(uint64_t val)
{
    static SEL Create = sel_registerName("numberWithUnsignedLong:");
    return Foundations::Get().Number.Call<id, uint64_t>(Create, val);
}
Instance NSNumber::FromF32(float    val)
{
    static SEL Create = sel_registerName("numberWithFloat:");
    return Foundations::Get().Number.Call<id, float>(Create, val);
}
Instance NSNumber::FromF64(double   val)
{
    static SEL Create = sel_registerName("numberWithDouble:");
    return Foundations::Get().Number.Call<id, double>(Create, val);
}

namespace detail
{
id NSArrayBase::At(size_t idx) const
{
    static SEL ObjAt = sel_registerName("objectAtIndex:");
    return Call<id, NSUInteger>(ObjAt, idx);
}
Class NSArrayBase::At2(size_t idx) const
{
    static SEL ObjAt = sel_registerName("objectAtIndex:");
    return Call<Class, NSUInteger>(ObjAt, idx);
}
void NSArrayBase::PushBack(id obj)
{
    static SEL AddObj = sel_registerName("addObject:");
    Call<void, id>(AddObj, obj);
}
void NSArrayBase::PushBack(Class obj)
{
    static SEL AddObj = sel_registerName("addObject:");
    Call<void, Class>(AddObj, obj);
}
void NSArrayBase::Insert(size_t idx, id obj)
{
    static SEL AddObj = sel_registerName("insertObject:atIndex:");
    Call<void, id, NSUInteger>(AddObj, obj, idx);
}
void NSArrayBase::Insert(size_t idx, Class obj)
{
    static SEL AddObj = sel_registerName("insertObject:atIndex:");
    Call<void, Class, NSUInteger>(AddObj, obj, idx);
}
id NSArrayBase::Create()
{
    return Foundations::Get().MutableArray.Create();
}
size_t NSArrayBase::Count() const
{
    return Call<NSUInteger>(Foundations::Get().Count);
}

id NSDictionaryBase::Get(id key) const
{
    static SEL ObjForKey = sel_registerName("objectForKey:");
    return Call<id, id>(ObjForKey, key);
}
void NSDictionaryBase::Insert(id key, id val)
{
    static SEL AddObj = sel_registerName("setObject:forKey:");
    Call<void, id, id>(AddObj, val, key);
}
id NSDictionaryBase::Create()
{
    return Foundations::Get().MutableDictionary.Create();
}
size_t NSDictionaryBase::Count() const
{
    return Call<NSUInteger>(Foundations::Get().Count);
}
}

}
}