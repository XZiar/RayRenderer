#include "WindowManager.h"
#include "WindowHost.h"

#include "SystemCommon/ObjCHelper.h"
#include "SystemCommon/NSFoundation.h"
#include "common/Exceptions.hpp"
#include "common/StaticLookup.hpp"
#include <array>
#include <tuple>

#include <CoreGraphics/CGBase.h>
#include <CoreGraphics/CGGeometry.h>

// see 

typedef CGPoint NSPoint;
typedef CGRect NSRect;
extern id NSApp;
extern id const NSDefaultRunLoopMode;


namespace xziar::gui::detail
{
using namespace std::string_view_literals;
using common::objc::Clz;
using common::objc::ClzInit;
using common::objc::Instance;
using common::objc::ClassBuilder;
using common::foundation::NSAutoreleasePool;
using common::foundation::NSString;
using common::foundation::NSDate;
using event::CommonKeys;

constexpr short MessageCreate = 1;
constexpr short MessageTask = 2;

static WindowManager* TheManager = nullptr;
// static void WindowWillClose(id self, SEL _sel, id notification)
// {
//     printf("window will close\n");
// }
// constexpr auto llp = common::objc::MethodTyper<decltype(&WindowWillClose)>::GetType();
// static_assert(llp[0] == 'v');
// static_assert(llp[1] == '@');
// static_assert(llp[2] == ':');
// static_assert(llp[3] == '@');

struct NSEvent : public Instance
{
    inline static const Clz NSEventClass = {"NSEvent"};
    enum class NSEventType : uint8_t 
    {
        Empty                   = 0,
        LeftMouseDown           = 1,
        LeftMouseUp             = 2,
        RightMouseDown          = 3,
        RightMouseUp            = 4,
        MouseMoved              = 5,
        LeftMouseDragged        = 6,
        RightMouseDragged       = 7,
        MouseEntered            = 8,
        MouseExited             = 9,
        KeyDown                 = 10,
        KeyUp                   = 11,
        FlagsChanged            = 12,
        AppKitDefined           = 13,
        SystemDefined           = 14,
        ApplicationDefined      = 15,
        Periodic                = 16,
        CursorUpdate            = 17,
        ScrollWheel             = 22,
        TabletPoint             = 23,
        TabletProximity         = 24,
        OtherMouseDown          = 25,
        OtherMouseUp            = 26,
        OtherMouseDragged       = 27,
        // >= 10.5.2
        Gesture                 = 29,
        Magnify                 = 30,
        Swipe                   = 31,
        Rotate                  = 18,
        BeginGesture            = 19,
        EndGesture              = 20,
        // >= 10.8
        SmartMagnify            = 32,
        QuickLook               = 33,
        // >= 10.10.3
        Pressure                = 34,
        // >= 10.10
        DirectTouch             = 37,
        // >= 10.15
        ChangeMode              = 38,
    };
    enum class MouseSubType  : short { Mouse = 0, TabletPoint = 1, TabletProximity = 2, Touch = 3 };
    enum class SystemSubType : short { PowerOff = 1, AuxMouseButton = 7 };
    enum class AppKitSubType : short 
    { 
        WindowExposed   = 0, 
        AppActivated    = 1, 
        AppDeactivated  = 2, 
        WindowMoved     = 4, 
        ScreenChanged   = 8,
        WindowBeginMove = 20,
    };
    static constexpr NSUInteger GetEventMask_(NSEventType type) noexcept { return NSUInteger(1) << common::enum_cast(type); } 
    template<typename... Args>
    static constexpr NSUInteger GetEventMask(Args... args) noexcept
    {
        return (... | GetEventMask_(args));
    }
    
    NSEventType Type;
    short SubType;
    Instance Window;
    NSEvent(id evt) : Instance(evt), Type(NSEventType::Empty), SubType(0), Window(nullptr)
    {
        static SEL SelType    = sel_registerName("type");
        static SEL SelWindow  = sel_registerName("window");
        static SEL SelSubType = sel_registerName("subtype");
        const auto type = Call<NSUInteger>(SelType);
        Type    = static_cast<NSEventType>(type);
        if ((type >= common::enum_cast(NSEventType::LeftMouseDown) && type <= common::enum_cast(NSEventType::RightMouseDragged)  /*MouseEvent*/) || 
            (type >= common::enum_cast(NSEventType::ScrollWheel)   && type <= common::enum_cast(NSEventType::OtherMouseDragged)  /*MouseEvent*/) ||
            (type >= common::enum_cast(NSEventType::AppKitDefined) && type <= common::enum_cast(NSEventType::ApplicationDefined) /*DefinedEvent*/))
            SubType = Call<short>(SelSubType);
        Window  = Call<id>(SelWindow);
    }
#define DefGetter(ret, name, type, sel) ret Get##name() const   \
{                                                               \
    static SEL Sel##name  = sel_registerName(#sel);             \
    return Call<type>(Sel##name);                               \
}
    DefGetter(NSInteger,    Data1,          NSInteger,  data1)
    DefGetter(NSInteger,    Data2,          NSInteger,  data2)
    DefGetter(uint16_t,     KeyCode,        uint16_t,   keyCode)
    DefGetter(NSString,     Characters,     id,         characters)
    DefGetter(NSUInteger,   ModifierFlags,  NSUInteger, modifierFlags)
    DefGetter(NSInteger,    ButtonNumber,   NSInteger,  buttonNumber)
#undef DefGetter
    template<typename T>
    constexpr T GetSubType() const noexcept
    {
        static_assert(std::is_enum_v<T> && std::is_same_v<std::underlying_type_t<T>, short>);
        return static_cast<T>(SubType);
    }

    static NSEvent Create(NSEventType type, short subtype, NSUInteger data1, NSUInteger data2)
    {
        static SEL NewEvent = sel_registerName("otherEventWithType:location:modifierFlags:timestamp:windowNumber:context:subtype:data1:data2:");
        NSPoint point = {0, 0};
        return NSEventClass.Call<id, NSUInteger, NSPoint, NSUInteger, double, NSInteger, id, short, NSInteger, NSInteger>
            (NewEvent, common::enum_cast(type), point, 0, 0., 0, nullptr, 0, data1, data2);
    }
};

class WindowManagerCocoa : public WindowManager
{
    static_assert(sizeof(NSInteger) == sizeof(intptr_t), "Expects NSInteger to be as large as intptr_t");
private:
    bool SupportNewThread() const noexcept override { return true; }
    
    forceinline WindowHost_* GetWindow(id window) const noexcept
    {
        for (const auto& pair : WindowList)
        {
            if (pair.first == reinterpret_cast<uintptr_t>(window))
            {
                return pair.second;
            }
        }
        return nullptr;
    }
    static void WindowWillClose(id self, SEL _sel, id notification)
    {
        if (const auto host = static_cast<WindowManagerCocoa*>(TheManager)->GetWindow(self); host)
            host->OnClose();
        //printf("window will close\n");
    }
    
    //@interface WindowDelegate : NSObject<NSWindowDelegate>
    //-(void)windowWillClose:(NSNotification*)notification;
    //@end
    inline static const ClzInit WindowDelegate = []()
    {
        ClassBuilder builder("WindowDelegate");
        builder.AddProtocol("NSWindowDelegate");
        builder.AddMethod("windowWillClose:", &WindowWillClose);
        return builder.GetClassInit(true);
    }();

    const Instance App;
    std::optional<NSAutoreleasePool> ARPool;

    void SendControlRequest(const NSInteger request, const NSInteger data = 0) noexcept
    {
        NSAutoreleasePool pool; // usally called from outside thread
        const auto evt = NSEvent::Create(NSEvent::NSEventType::ApplicationDefined, 0, request, data);
        static SEL SelPostEvent = sel_registerName("postEvent:atStart:");
        App.Call<void, id, BOOL>(SelPostEvent, evt, NO);
    }
    static event::Position GetMousePosition(const NSEvent& event)
    {
        static SEL SelView  = sel_registerName("contentView");
        static SEL SelFrame = sel_registerName("frame");
        static SEL SelLoc   = sel_registerName("locationInWindow");
        const auto contentRect = Instance(event.Window.Call<id>(SelView)).Call<NSRect>(SelFrame);
        const auto loc = event.Call<NSPoint>(SelLoc);
        const auto x = loc.x, y = contentRect.size.height - loc.y;
        return { static_cast<int32_t>(x), static_cast<int32_t>(y) };
    }
    static event::CombinedKey ProcessKey(uint16_t keycode) noexcept;
public:
    WindowManagerCocoa() : App([]()
    {
        NSAutoreleasePool pool;
        //[NSApplication sharedApplication];
        static const Clz NSApplicationClass("NSApplication");
        return NSApplicationClass.Call<id>("sharedApplication");
    }()) { }
    ~WindowManagerCocoa() override { }

    void Initialize() override
    {
        using common::BaseException;

        TheManager = this;

        NSAutoreleasePool pool;

        // only needed if we don't use [NSApp run]
        //[NSApp finishLaunching];
        App.Call<void>("finishLaunching");

        //[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        App.Call<void, NSInteger>("setActivationPolicy:", 0);
        //[NSApp activateIgnoringOtherApps:YES];
        App.Call<void, BOOL>("activateIgnoringOtherApps:", YES);

        //id menubar = [[NSMenu alloc] init];
        static const ClzInit NSMenuClass("NSMenu", true);
        const Instance menubar = NSMenuClass.Create();

        //id appMenuItem = [[NSMenuItem alloc] init];
        static const ClzInit NSMenuItemClass("NSMenuItem", true);
        const Instance appMenuItem = NSMenuItemClass.Create();

        //[menubar addItem:appMenuItem];
        menubar.Call<void, id>("addItem:", appMenuItem);

        //[NSApp setMainMenu:menubar];
        App.Call<void, id>("setMainMenu:", menubar);

        //id appMenu = [[NSMenu alloc] init];
        const Instance appMenu = NSMenuClass.Create();

        //[appMenuItem setSubmenu:appMenu];
        appMenuItem.Call<void, id>("setSubmenu:", appMenu);
    }
    void DeInitialize() noexcept override
    {
        TheManager = nullptr;
    }

    void Prepare() noexcept override
    {
        ARPool.emplace();
    }
    void MessageLoop() override
    {
        using EventType = NSEvent::NSEventType;
        static SEL SelNextEvt = sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:");
        // bypass some events
        constexpr auto EventMask = NSUIntegerMax & (~NSEvent::GetEventMask(EventType::SystemDefined, 
            EventType::Gesture, EventType::Magnify, EventType::Swipe, EventType::Rotate, EventType::BeginGesture, EventType::EndGesture,
            EventType::SmartMagnify, EventType::QuickLook, EventType::Pressure, EventType::DirectTouch)); 
        while (true)
        {
            NSAutoreleasePool pool;
            const NSEvent evt = App.Call<id, NSUInteger, id, id, BOOL>(SelNextEvt, EventMask, NSDate::DistantFuture(), NSDefaultRunLoopMode, YES);
            if (!evt) continue;
            const auto host = GetWindow(evt.Window);
            if (evt.Type == EventType::AppKitDefined)
            {
    enum class AppKitSubType : short { WindowExposed = 0, AppActivated = 1, AppDeactivated = 2, WindowMoved = 4, ScreenChanged = 8 };
                using AKType = NSEvent::AppKitSubType;
                const auto data1   = evt.GetData1();
                const auto data2   = evt.GetData2();
                const auto subtype = evt.GetSubType<AKType>();
                std::u16string_view desc;
                switch (subtype)
                {
                case AKType::WindowExposed:     desc = u"WindowExposed"sv;  break;
                case AKType::AppActivated:      desc = u"AppActivated"sv;   break;
                case AKType::AppDeactivated:    desc = u"AppDeactivated"sv; break;
                case AKType::WindowMoved:       desc = u"WindowMoved"sv;    break;
                case AKType::ScreenChanged:     desc = u"ScreenChanged"sv;  break;
                case AKType::WindowBeginMove:   desc = u"WindowBeginMove"sv;break;
                }
                Logger.verbose(u"Recieve [AppKit]message sub[{}][{}] host[{}] data[{}][{}]\n", 
                    evt.SubType, desc, (void*)host, data1, data2);
                continue;
            }
            if (evt.Type == EventType::ApplicationDefined)
            {
                const auto request = evt.GetData1();
                const auto data    = evt.GetData2();
                switch (request)
                {
                case MessageCreate:
                {
                    const auto host = reinterpret_cast<WindowHost_*>(static_cast<uintptr_t>(data));
                    CreateNewWindow_(host);
                } break;
                case MessageTask:
                    HandleTask();
                    break;
                }
                continue;
            }
            if (!host)
            {
                if (evt.Type != EventType::MouseMoved) // ignore
                    Logger.verbose(u"Recieve message [{}] without window\n", (uint32_t)evt.Type);
                continue;
            }
            switch (evt.Type)
            {
            case EventType::LeftMouseDown:
            case EventType::LeftMouseUp:
            case EventType::RightMouseDown:
            case EventType::RightMouseUp:
            case EventType::OtherMouseDown:
            case EventType::OtherMouseUp:
            {
                const auto type = common::enum_cast(evt.Type);
                const bool isPress = type & 1;
                event::MouseButton  btn = event::MouseButton::None;
                if (type <= 2)      btn = event::MouseButton::Left;
                else if (type <= 4) btn = event::MouseButton::Right;
                else if (const auto btnNum = evt.GetButtonNumber(); btnNum == 2)
                    btn = event::MouseButton::Middle;
                host->OnMouseButton(btn, isPress);
            } break;
            case EventType::MouseMoved:
            {
                const auto pos = GetMousePosition(evt);
                host->OnMouseMove(pos);
            } break;
            case EventType::MouseEntered:
            {
                //window.Call<void, BOOL>(SetAcceptMouseMove, YES);
                const auto pos = GetMousePosition(evt);
                host->OnMouseEnter(pos);
            } break;
            case EventType::MouseExited:
            {
                //window.Call<void, BOOL>(SetAcceptMouseMove, NO);
                host->OnMouseLeave();
            } break;
            case EventType::KeyDown:
            case EventType::KeyUp:
            {
                //uint16_t keyCode = [event keyCode];SelKeyCode
                const auto keyCode = evt.GetKeyCode();
                const auto key = ProcessKey(keyCode);
                if (key.Key == event::CommonKeys::UNDEFINE)
                {
                    //NSString * inputText = [event characters];
                    const auto str = evt.GetCharacters();
                    const auto ch = str[0];
                    Logger.verbose(u"key: [{}] => [{}]({})\n", keyCode, static_cast<uint16_t>(ch), ch);
                }
                if (evt.Type == EventType::KeyDown)
                    host->OnKeyDown(key);
                else
                    host->OnKeyUp(key);
            } break;
            case EventType::FlagsChanged:
            {
                const auto modifier = evt.GetModifierFlags();
                //constexpr NSUInteger ModCaps    = 1 << 16;
                constexpr NSUInteger ModShift   = 1 << 17;
                constexpr NSUInteger ModCtrl    = 1 << 18;
                constexpr NSUInteger ModAlt     = 1 << 19;
                //constexpr NSUInteger ModCommand = 1 << 20;
#define CheckMod(mod)                                                                                               \
if (const auto has##mod = modifier & Mod##mod; has##mod != HAS_FIELD(host->Modifiers, event::ModifierKeys::mod))    \
{                                                                                                                   \
    if (has##mod)   host->OnKeyDown(event::CommonKeys::mod);                                                        \
    else            host->OnKeyUp  (event::CommonKeys::mod);                                                        \
}
                CheckMod(Shift)
                CheckMod(Ctrl)
                CheckMod(Alt)
#undef CheckMod
            } break;
            default:
                Logger.verbose(u"Recieve message [{}]\n", (uint32_t)evt.Type);
            }
        }
    }
    void Terminate() noexcept override
    {
        ARPool.reset();
    }

    void CreateNewWindow_(WindowHost_* host)
    {
        using common::BaseException;

        //id window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 500, 500) styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask backing:NSBackingStoreBuffered defer:NO];
        static const ClzInit<NSRect, NSUInteger, NSUInteger, BOOL> NSWindowClass("NSWindow", true, "initWithContentRect:styleMask:backing:defer:");
        NSRect rect = {{0, 0}, { static_cast<CGFloat>(host->Width), static_cast<CGFloat>(host->Height) }};
        const Instance window = NSWindowClass.Create(rect, 15, 2, NO);

        // when we are not using ARC, than window will be added to autorelease pool
        // so if we close it by hand (pressing red button), we don't want it to be released for us
        // so it will be released by autorelease pool later
        //[window setReleasedWhenClosed:NO];
        window.Call<void, BOOL>("setReleasedWhenClosed:", NO);

        //WindowDelegate * wdg = [[WindowDelegate alloc] init];
        const auto wdg = WindowDelegate.Create();
        //[window setDelegate:wdg];
        window.Call<void, id>("setDelegate:", wdg);

        //NSView * contentView = [window contentView];
        const Instance contentView = window.Call<id>("contentView");

        //[contentView setWantsBestResolutionOpenGLSurface:YES];
        contentView.Call<void, BOOL>("setWantsBestResolutionOpenGLSurface:", YES);

        {
            // //[window cascadeTopLeftFromPoint:NSMakePoint(20,20)];
            // NSPoint point = {20, 20};
            // SEL cascadeTopLeftFromPointSel = sel_registerName("cascadeTopLeftFromPoint:");
            // ((void (*)(id, SEL, NSPoint))objc_msgSend)(window, cascadeTopLeftFromPointSel, point);
        }

        //[window setTitle:@"sup"];
        const auto titleString = NSString::Create(host->Title);
        window.Call<void, id>("setTitle:", titleString);

        //[window makeKeyAndOrderFront:window];
        window.Call<void, id>("makeKeyAndOrderFront:", nullptr);//window);

        {
            //[window setAcceptsMouseMovedEvents:YES];
            window.Call<void, BOOL>("setAcceptsMouseMovedEvents:", YES);
        }

        // //[window setBackgroundColor:[NSColor blackColor]];
        // Class NSColorClass = objc_getClass("NSColor");
        // id blackColor = ((id (*)(Class, SEL))objc_msgSend)(NSColorClass, sel_registerName("blackColor"));
        // SEL setBackgroundColorSel = sel_registerName("setBackgroundColor:");
        // ((void (*)(id, SEL, id))objc_msgSend)(window, setBackgroundColorSel, blackColor);

        // // TODO do we really need this?
        // //[NSApp activateIgnoringOtherApps:YES];
        // SEL activateIgnoringOtherAppsSel = sel_registerName("activateIgnoringOtherApps:");
        // ((void (*)(id, SEL, BOOL))objc_msgSend)(NSApp, activateIgnoringOtherAppsSel, YES);

        RegisterHost(static_cast<id>(window), host);
    }
    void NotifyTask() noexcept override
    {
        SendControlRequest(MessageTask);
    }
    void CreateNewWindow(WindowHost_* host) override
    {
        const NSInteger ptr = reinterpret_cast<uintptr_t>(host);
        SendControlRequest(MessageCreate, ptr);
    }
    void CloseWindow(WindowHost_* host) override
    {
    }
    void ReleaseWindow(WindowHost_* host) override
    {
        UnregisterHost(host);
    }
};


// see http://macbiblioblog.blogspot.com/2014/12/key-codes-for-function-and-special-keys.html
static constexpr auto KeyCodeLookup = BuildStaticLookup(uint16_t, event::CombinedKey,
    { 29,    '0' },
    { 18,    '1' },
    { 19,    '2' },
    { 20,    '3' },
    { 21,    '4' },
    { 23,    '5' },
    { 22,    '6' },
    { 26,    '7' },
    { 28,    '8' },
    { 25,    '9' },
    { 0,     'A' },
    { 11,    'B' },
    { 8,     'C' },
    { 2,     'D' },
    { 14,    'E' },
    { 3,     'F' },
    { 5,     'G' },
    { 4,     'H' },
    { 34,    'I' },
    { 38,    'J' },
    { 40,    'K' },
    { 37,    'L' },
    { 46,    'M' },
    { 45,    'N' },
    { 31,    'O' },
    { 35,    'P' },
    { 12,    'Q' },
    { 15,    'R' },
    { 1,     'S' },
    { 17,    'T' },
    { 32,    'U' },
    { 9,     'V' },
    { 13,    'W' },
    { 7,     'X' },
    { 16,    'Y' },
    { 6,     'Z' },
    { 50,    '`' },
    { 27,    '-' },
    { 24,    '=' },
    { 33,    '[' },
    { 30,    ']' },
    { 41,    ';' },
    { 39,    '\'' },
    { 43,    ',' },
    { 47,    '.' },
    { 44,    '/' },
    { 42,    '\\' },
    { 82,    '0' },
    { 83,    '1' },
    { 84,    '2' },
    { 85,    '3' },
    { 86,    '4' },
    { 87,    '5' },
    { 88,    '6' },
    { 89,    '7' },
    { 91,    '8' },
    { 92,    '9' },
    { 65,    '.' },
    { 67,    '*' },
    { 69,    '+' },
    { 75,    '/' },
    { 78,    '-' },
    { 81,    '=' },
    { 76,    CommonKeys::Enter },
    { 49,    CommonKeys::Space },
    { 36,    CommonKeys::Enter },
    { 48,    CommonKeys::Tab },
    { 51,    CommonKeys::Backspace },
    { 117,   CommonKeys::Delete },
    { 53,    CommonKeys::Esc },
    { 56,    CommonKeys::Shift },
    { 57,    CommonKeys::Shift },
    { 58,    CommonKeys::Alt },
    { 59,    CommonKeys::Ctrl },
    { 60,    CommonKeys::Shift },
    { 61,    CommonKeys::Alt },
    { 62,    CommonKeys::Ctrl },
    { 122,   CommonKeys::F1 },
    { 120,   CommonKeys::F2 },
    { 99,    CommonKeys::F3 },
    { 118,   CommonKeys::F4 },
    { 96,    CommonKeys::F5 },
    { 97,    CommonKeys::F6 },
    { 98,    CommonKeys::F7 },
    { 100,   CommonKeys::F8 },
    { 101,   CommonKeys::F9 },
    { 109,   CommonKeys::F10 },
    { 103,   CommonKeys::F11 },
    { 111,   CommonKeys::F12 },
    { 114,   CommonKeys::Insert },
    { 115,   CommonKeys::Home },
    { 119,   CommonKeys::End },
    { 116,   CommonKeys::PageUp },
    { 121,   CommonKeys::PageDown },
    { 123,   CommonKeys::Left },
    { 124,   CommonKeys::Right },
    { 125,   CommonKeys::Down },
    { 126,   CommonKeys::Up });
event::CombinedKey WindowManagerCocoa::ProcessKey(uint16_t keycode) noexcept
{
    return KeyCodeLookup(keycode).value_or(CommonKeys::UNDEFINE);
}

std::unique_ptr<WindowManager> CreateManagerImpl()
{
    return std::make_unique<WindowManagerCocoa>();
}


}