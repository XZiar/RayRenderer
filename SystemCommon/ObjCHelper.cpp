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


struct FoundationClz
{
    ClzInit<> AutoreleasePool;
    Clz String;
    Clz Date;
    FoundationClz() : 
        AutoreleasePool("NSAutoreleasePool", false),
        String("NSString"),
        Date("NSDate")
    { }
    static const FoundationClz& Get() noexcept
    {
        static FoundationClz Clzs;
        return Clzs;
    }
};


NSAutoreleasePool::NSAutoreleasePool() : Instance(FoundationClz::Get().AutoreleasePool.Create()) {}
NSAutoreleasePool::~NSAutoreleasePool()
{
    static SEL Drain = sel_registerName("drain");
    Call<void>(Drain);
}


const NSDate& NSDate::DistantPast() noexcept
{
    static const NSDate ret = FoundationClz::Get().Date.Call<id>("distantPast");
    return ret;
}
const NSDate& NSDate::DistantFuture() noexcept
{
    static const NSDate ret = FoundationClz::Get().Date.Call<id>("distantFuture");
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
    static const ClzInit<id> NSStringInit(FoundationClz::Get().String, false, "initWithString:");
    return NSStringInit.Create<NSString>(Self);
}

NSString NSString::Create()
{
    static SEL EmptyStr = sel_registerName("string");
    return FoundationClz::Get().String.Call<id>(EmptyStr);
}
NSString NSString::Create(std::u16string_view text)
{
    static_assert(sizeof(NSUInteger) == sizeof(size_t));
    static const ClzInit<const unichar*, NSUInteger> NSStringInit(
        FoundationClz::Get().String, false, "initWithCharacters:length:");
    return NSStringInit.Create<NSString>(reinterpret_cast<const unichar*>(text.data()), text.size());
}


NSNotification::NSNotification(id instance) : Instance(instance), Target(nullptr)
{
    static SEL SelObject = sel_registerName("object");
    Target = Call<id>(SelObject);
}

}
}