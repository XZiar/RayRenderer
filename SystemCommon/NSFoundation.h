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

}
