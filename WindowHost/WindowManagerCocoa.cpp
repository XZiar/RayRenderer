#include "WindowManager.h"
#include "WindowHost.h"

#include "SystemCommon/ObjCHelper.h"
#include "SystemCommon/NSFoundation.h"
#include "SystemCommon/StringConvert.h"
#include "SystemCommon/Exceptions.h"
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
extern id const NSPasteboardTypeFileURL;
extern id const NSPasteboardURLReadingFileURLsOnlyKey;


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
using common::foundation::NSNotification;
using common::foundation::NSNumber;
using common::foundation::NSArray;
using common::foundation::NSDictionary;
using event::CommonKeys;

constexpr short MessageCreate   = 1;
constexpr short MessageTask     = 2;
constexpr short MessageUpdTitle = 3;
constexpr short MessageStop     = 4;

static WindowManager* TheManager = nullptr;

template<typename F, typename T>
forceinline constexpr T NormalCast(const F& val)
{
    if constexpr (std::is_same_v<F, BOOL>)
        return static_cast<T>(val == YES);
    else if constexpr (std::is_same_v<F, id>)
        return val;
    else
        return static_cast<T>(val);
}

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
    return NormalCast<type, ret>(Call<type>(Sel##name));        \
}
    DefGetter(NSInteger,    Data1,           NSInteger,  data1)
    DefGetter(NSInteger,    Data2,           NSInteger,  data2)
    DefGetter(uint16_t,     KeyCode,         uint16_t,   keyCode)
    DefGetter(NSString,     Characters,      id,         characters)
    DefGetter(NSUInteger,   ModifierFlags,   NSUInteger, modifierFlags)
    DefGetter(NSInteger,    ButtonNumber,    NSInteger,  buttonNumber)
    DefGetter(NSPoint,      PosInWindow,     NSPoint,    locationInWindow)
    DefGetter(float,        ScrollingDeltaX, CGFloat,    scrollingDeltaX)
    DefGetter(float,        ScrollingDeltaY, CGFloat,    scrollingDeltaY)
    DefGetter(bool,         IsPreciseScroll, BOOL,       hasPreciseScrollingDeltas)
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


struct NSMenuItem : public Instance
{
    using Instance::Instance;
    inline static const Clz NSMenuItemClass = {"NSMenuItem"};
    static NSMenuItem Create()
    {
        static const ClzInit Init(NSMenuItemClass, true);
        return Init.Create<NSMenuItem>();
    }
    static NSMenuItem Create(std::u16string_view title, SEL action = nullptr, char16_t key = u'\0')
    {
        static const ClzInit<id, SEL, id> Init(NSMenuItemClass, true, "initWithTitle:action:keyEquivalent:");
        const auto keyCode = key == u'\0' ? NSString::Create() : NSString::Create({&key, 1});
        return Init.Create<NSMenuItem>(NSString::Create(title), action, keyCode);
    }
};


class WindowManagerCocoa : public CocoaBackend, public WindowManager
{
    static_assert(sizeof(NSInteger) == sizeof(intptr_t), "Expects NSInteger to be as large as intptr_t");
public:
    static constexpr std::string_view BackendName = "Cocoa"sv;
private:
    class WdHost final : public CocoaBackend::CocoaWdHost
    {
    public:
        Instance Window = nullptr;
        Instance WindowDlg = nullptr;
        id View = nullptr;
        bool NeedBackground = true;
        WdHost(WindowManagerCocoa& manager, const CocoaCreateInfo& info) noexcept :
            CocoaWdHost(manager, info) { }
        ~WdHost() final {}
        void* GetWindow() const noexcept final { return Window; }
    };
    class CocoaView : public Instance
    {
    private:
        static void HandleEvent(id self, SEL _sel, id evt_);
        static void UpdateTrackingAreas(id self, SEL _sel);
        static NSUInteger DraggingEntered(id self, SEL _sel, id sender)
        {
            return 4; // NSDragOperationGeneric
        }
        static BOOL PerformDragOperation(id self, SEL _sel, id sender);
        static event::Position GetPositionInView(const CocoaView& view, const NSPoint& loc)
        {
            static SEL SelFrame = sel_registerName("frame");
            const auto contentRect = view.Call<NSRect>(SelFrame);
            const auto x = loc.x, y = contentRect.size.height - loc.y;
            return { static_cast<int32_t>(x), static_cast<int32_t>(y) };
        }
        //@interface CocoaView : NSView
        //- (void)mouseDown:(NSEvent *)event;
        //- (void)mouseDragged:(NSEvent *)event;
        //- (void)mouseUp:(NSEvent *)event;
        //- (void)mouseMoved:(NSEvent *)event;
        //- (void)mouseEntered:(NSEvent *)event;
        //- (void)mouseExited:(NSEvent *)event;
        //- (void)rightMouseDragged:(NSEvent *)event;
        //- (void)rightMouseUp:(NSEvent *)event;
        //- (void)otherMouseDown:(NSEvent *)event;
        //- (void)otherMouseDragged:(NSEvent *)event;
        //- (void)otherMouseUp:(NSEvent *)event;
        //- (void)scrollWheel:(NSEvent *)event;
        //- (void)keyDown:(NSEvent *)event;
        //- (void)keyUp:(NSEvent *)event;
        //@end
        inline static const Clz NSViewClass = { "NSView" };
        inline static auto CocoaViewInit = []()
        {
            ClassBuilder builder("CocoaView", NSViewClass);
            bool suc = true;
            //suc = builder.AddProtocol("NSTextInputClient");
            suc = builder.AddVariable<WindowHost_*>("WdHost");
            suc = builder.AddVariable<id>("TrackArea");
            suc = builder.AddMethod("updateTrackingAreas", &UpdateTrackingAreas);
            suc = builder.AddMethod("mouseDown:", &HandleEvent);
            suc = builder.AddMethod("mouseDragged:", &HandleEvent);
            suc = builder.AddMethod("mouseUp:", &HandleEvent);
            suc = builder.AddMethod("mouseMoved:", &HandleEvent);
            suc = builder.AddMethod("mouseEntered:", &HandleEvent);
            suc = builder.AddMethod("mouseExited:", &HandleEvent);
            suc = builder.AddMethod("rightMouseDragged:", &HandleEvent);
            suc = builder.AddMethod("rightMouseUp:", &HandleEvent);
            suc = builder.AddMethod("otherMouseDown:", &HandleEvent);
            suc = builder.AddMethod("otherMouseDragged:", &HandleEvent);
            suc = builder.AddMethod("otherMouseUp:", &HandleEvent);
            suc = builder.AddMethod("scrollWheel:", &HandleEvent);
            suc = builder.AddMethod("keyDown:", &HandleEvent);
            suc = builder.AddMethod("keyUp:", &HandleEvent);
            suc = builder.AddMethod("draggingEntered:", &DraggingEntered);
            suc = builder.AddMethod("performDragOperation:", &PerformDragOperation);
            builder.Finish();
            return builder.GetClassInit(false);
        }();
        inline static auto WdHostVar    = class_getInstanceVariable(CocoaViewInit, "WdHost");
        inline static auto TrackAreaVar = class_getInstanceVariable(CocoaViewInit, "TrackArea");
        Instance GetTrackArea() const { return GetVar<id>(TrackAreaVar); }
        void SetTrackArea(Instance area) { SetVar<id>(TrackAreaVar, area); }
        void SetHost(WindowHost_* host) 
        {
            SetVar(WdHostVar, host);
        }
    public:
        using Instance::Instance;
        WindowHost_* GetHost() const 
        {
            return GetVar<WindowHost_*>(WdHostVar);
        }
        Instance GetWindow() const
        {
            static const SEL Window = sel_registerName("window");
            return Call<id>(Window);
        }

        static CocoaView Create(WindowHost_* host)
        {
            auto view = CocoaViewInit.Create<CocoaView>();
            view.SetHost(host);
            view.Call<void>("updateTrackingAreas");
            return view;
        }
    };

    std::string_view Name() const noexcept { return BackendName; }
    bool CheckFeature(std::string_view feat) const noexcept
    {
        constexpr std::string_view Features[] =
        {
            "OpenGL"sv, "Metal"sv
        };
        return std::find(std::begin(Features), std::end(Features), feat) != std::end(Features);
    }

    static NSUInteger AppShouldTerminate(id self, SEL _sel, id sender)
    {
        return 0;
    }
    static void AppWillFinishLaunching(id self, SEL _sel, id notification)
    {
        NSAutoreleasePool pool;
        NSNotification noti(notification);
        const auto& app = noti.Target;

        static const ClzInit NSMenuClass("NSMenu", true);

        //id menubar = [[NSMenu alloc] init];
        const Instance menubar = NSMenuClass.Create();
        //[NSApp setMainMenu:menubar];
        app.Call<void, id>("setMainMenu:", menubar);

        //id appMenuItem = [[NSMenuItem alloc] init];
        const auto appMenuItem = NSMenuItem::Create();
        //[menubar addItem:appMenuItem];
        menubar.Call<void, id>("addItem:", appMenuItem);

        //id appMenu = [[NSMenu alloc] init];
        const Instance appMenu = NSMenuClass.Create();
        //[appMenuItem setSubmenu:appMenu];
        appMenuItem.Call<void, id>("setSubmenu:", appMenu);

        //id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
        const auto quitMenuItem = NSMenuItem::Create(u"quit app"sv, sel_registerName("terminate:"));
        //[appMenu addItem:quitMenuItem];
        appMenu.Call<void, id>("addItem:", quitMenuItem);

        //[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        app.Call<void, NSInteger>("setActivationPolicy:", 0);
    }
    //@interface AppDelegate : NSObject<NSApplicationDelegate>
    //-(BOOL)applicationShouldTerminate:(NSWindow*)sender;
    //-(void)applicationWillFinishLaunching:(NSNotification*)notification;
    //@end
    inline static const ClzInit AppDelegate = []()
    {
        ClassBuilder builder("AppDelegate");
        bool suc = true;
        suc = builder.AddProtocol("NSApplicationDelegate");
        suc = builder.AddMethod("applicationShouldTerminate:", &AppShouldTerminate);
        suc = builder.AddMethod("applicationWillFinishLaunching:", &AppWillFinishLaunching);
        //suc = builder.AddMethod("applicationDidFinishLaunching:", &AppDidFinishLaunching);
        builder.Finish();
        return builder.GetClassInit(false);
    }();

    static BOOL WindowShouldClose(id self, SEL _sel, id sender)
    {
        if (const auto host = static_cast<WindowManagerCocoa*>(TheManager)->GetWindow(sender); host)
            return host->OnClose() ? YES : NO;
        return YES;
    }
    static void WindowWillClose(id self, SEL _sel, id notification)
    {
        NSNotification noti(notification);
        const auto& window = static_cast<id>(noti.Target);
        if (const auto host = static_cast<WindowManagerCocoa*>(TheManager)->GetWindow(window); host)
            host->Stop();
    }
    
    //@interface WindowDelegate : NSObject<NSWindowDelegate>
    //-(BOOL)windowShouldClose:(NSWindow*)sender;
    //-(void)windowWillClose:(NSNotification*)notification;
    //-(void)windowWillMove:(NSNotification*)notification;
    //-(void)windowDidMove:(NSNotification*)notification;
    //@end
    inline static const ClzInit WindowDelegate = []()
    {
        ClassBuilder builder("WindowDelegate");
        bool suc = true;
        suc = builder.AddProtocol("NSWindowDelegate");
        suc = builder.AddMethod("windowShouldClose:", &WindowShouldClose);
        suc = builder.AddMethod("windowWillClose:", &WindowWillClose);
        // suc = builder.AddMethod("windowWillMove:", &WindowWillMove);
        // suc = builder.AddMethod("windowDidMove:", &WindowDidMove);
        builder.Finish();
        return builder.GetClassInit(false);
    }();

    inline static const SEL SetTitleSel = sel_registerName("setTitle:");

    Instance App;
    Instance AppDlg;
    std::optional<NSAutoreleasePool> ARPool;
    bool IsCapsLock = false;

    void SendControlRequest(const NSInteger request, const NSInteger data = 0) noexcept
    {
        NSAutoreleasePool pool; // usally called from outside thread
        const auto evt = NSEvent::Create(NSEvent::NSEventType::ApplicationDefined, 0, request, data);
        static SEL SelPostEvent = sel_registerName("postEvent:atStart:");
        App.Call<void, id, BOOL>(SelPostEvent, evt, NO);
    }
    static event::CombinedKey ProcessKey(uint16_t keycode) noexcept;
public:
    WindowManagerCocoa() : CocoaBackend(false), App(nullptr), AppDlg(nullptr){ }
    ~WindowManagerCocoa() final { }

    void OnInitialize(const void* info) final
    {
        using common::BaseException;
        TheManager = this;

        NSAutoreleasePool pool;

        //[NSApplication sharedApplication];
        static const Clz NSApplicationClass("NSApplication");
        App = NSApplicationClass.Call<id>("sharedApplication");

        //AppDlg = [[AppDelegate alloc] init];
        AppDlg = AppDelegate.Create();
        //[NSApp setDelegate:adg];
        App.Call<void, id>("setDelegate:", AppDlg);

        // only needed if we don't use [NSApp run]
        //[NSApp finishLaunching];
        App.Call<void>("finishLaunching");

        CocoaBackend::OnInitialize(info);
    }
    void OnDeInitialize() noexcept final
    {
        AppDlg.Release();
        TheManager = nullptr;
        CocoaBackend::OnDeInitialize();
    }

    void OnPrepare() noexcept final
    {
        ARPool.emplace();
    }
    void OnMessageLoop() final
    {
        using EventType = NSEvent::NSEventType;
        static SEL SelNextEvt = sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:");
        static SEL SelSendEvt = sel_registerName("sendEvent:");
        static SEL SelUpdWd   = sel_registerName("updateWindows");
        static const auto& distFut = NSDate::DistantFuture();
        bool shouldContinue = true;
        while (shouldContinue)
        {
            NSAutoreleasePool pool;
            const NSEvent evt = App.Call<id, NSUInteger, id, id, BOOL>(SelNextEvt, NSUIntegerMax, distFut, NSDefaultRunLoopMode, YES);
            Expects(evt);
            const auto host = GetWindow(static_cast<id>(evt.Window));
            switch (evt.Type)
            {
            case EventType::AppKitDefined:
            {
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
                Logger.Verbose(u"Recieve [AppKit]message sub[{}][{}] host[{}] data[{}][{}]\n", 
                    evt.SubType, desc, (void*)host, data1, data2);
            } break;
            case EventType::ApplicationDefined:
            {
                const auto request = evt.GetData1();
                const auto data    = evt.GetData2();
                switch (request)
                {
                case MessageCreate:
                {
                    auto& payload = *reinterpret_cast<CreatePayload*>(static_cast<uintptr_t>(data));
                    CreateNewWindow_(payload);
                } break;
                case MessageTask:
                    HandleTask();
                    break;
                case MessageUpdTitle:
                {
                    const auto host = static_cast<WdHost*>(reinterpret_cast<WindowHost_*>(static_cast<uintptr_t>(data)));
                    TitleLock lock(host);
                    const auto titleString = NSString::Create(host->Title);
                    host->Window.Call<void, id>(SetTitleSel, titleString);
                } break;
                case MessageStop:
                    shouldContinue = false;
                    break;
                }
            } break;
            case EventType::FlagsChanged:
            {
                constexpr NSUInteger ModCaps    = 1 << 16;
                constexpr NSUInteger ModShift   = 1 << 17;
                constexpr NSUInteger ModCtrl    = 1 << 18;
                constexpr NSUInteger ModAlt     = 1 << 19;
                const auto modifier = evt.GetModifierFlags();
                bool capsChanged = false;
                if (const bool hasCaps = modifier & ModCaps; hasCaps != IsCapsLock)
                    IsCapsLock = hasCaps, capsChanged = true;
                if (host)
                {
#define CheckMod(mod)                                                                                               \
if (const bool has##mod = modifier & Mod##mod; has##mod != HAS_FIELD(host->Modifiers, event::ModifierKeys::mod))    \
{                                                                                                                   \
    if (has##mod)   host->OnKeyDown(event::CommonKeys::mod);                                                        \
    else            host->OnKeyUp  (event::CommonKeys::mod);                                                        \
}
                    CheckMod(Shift)
                    CheckMod(Ctrl)
                    CheckMod(Alt)
#undef CheckMod
                    if (capsChanged)
                    {
                        host->OnKeyDown(event::CommonKeys::CapsLock);
                        host->OnKeyUp  (event::CommonKeys::CapsLock);
                    }
                }
            } break;
            default: 
                // Logger.Verbose(u"Recieve message[{}] host[{}]\n", (uint32_t)evt.Type, (void*)host);
                break;
            }
            App.Call<void, id>(SelSendEvt, evt);
            App.Call<void>(SelUpdWd);
        }
    }
    void OnTerminate() noexcept final
    {
        ARPool.reset();
    }
    bool RequestStop() noexcept final
    {
        SendControlRequest(MessageStop, 0);
        return true;
    }

    void CreateNewWindow_(CreatePayload& payload)
    {
        using common::BaseException;
        const auto host = static_cast<WdHost*>(payload.Host);
        host->NeedCheckDrag = false;

        //id window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 500, 500) styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask backing:NSBackingStoreBuffered defer:NO];
        static const ClzInit<NSRect, NSUInteger, NSUInteger, BOOL> NSWindowClass("NSWindow", false, "initWithContentRect:styleMask:backing:defer:");
        NSRect rect = {{0, 0}, { static_cast<CGFloat>(host->Width), static_cast<CGFloat>(host->Height) }};
        const Instance window = NSWindowClass.Create(rect, 15, 2, NO);
        host->Window = window;

        //WindowDelegate * wdg = [[WindowDelegate alloc] init];
        host->WindowDlg = WindowDelegate.Create();
        //[window setDelegate:wdg];
        window.Call<void, id>("setDelegate:", host->WindowDlg);

        //[window setReleasedWhenClosed:YES];
        window.Call<void, BOOL>("setReleasedWhenClosed:", YES);

        const auto view = CocoaView::Create(host);
        host->View = view;
        //[view setWantsBestResolutionOpenGLSurface:YES];
        view.Call<void, BOOL>("setWantsBestResolutionOpenGLSurface:", YES);
        static const auto AllowDragTypes = []()
        {
            auto array = common::foundation::NSMutArray<id>::Create();
            array.PushBack(NSPasteboardTypeFileURL);
            return array;
        }();
        //[view registerForDraggedTypes:AllowDragTypes];
        view.Call<void, id>("registerForDraggedTypes:", AllowDragTypes);
        //[window setContentView:view];
        window.Call<void, id>("setContentView:", view);
        //[window makeFirstResponder:view];
        window.Call<void, id>("makeFirstResponder:", view);

        //[window setTitle:@"xxx"];
        const auto titleString = NSString::Create(host->Title);
        window.Call<void, id>(SetTitleSel, titleString);

        //[window makeKeyAndOrderFront:window];
        window.Call<void, id>("makeKeyAndOrderFront:", window);
        //[window setAcceptsMouseMovedEvents:YES];
        window.Call<void, BOOL>("setAcceptsMouseMovedEvents:", YES);
        //[window setRestorable:NO];
        window.Call<void, BOOL>("setRestorable:", NO);

        RegisterHost(static_cast<id>(window), host);
        host->Initialize();
    }
    void NotifyTask() noexcept final
    {
        SendControlRequest(MessageTask);
    }

    bool CheckCapsLock() const noexcept final
    {
        return IsCapsLock;
    }

    void CreateNewWindow(CreatePayload& payload) final
    {
        const NSInteger ptr = reinterpret_cast<uintptr_t>(&payload);
        SendControlRequest(MessageCreate, ptr);
        payload.Promise.get_future().get();
    }
    void UpdateTitle(WindowHost_* host) const final
    {
        const NSInteger ptr = reinterpret_cast<uintptr_t>(host);
        SendControlRequest(MessageUpdTitle, ptr);
    }
    void CloseWindow(WindowHost_* host) const final
    {
        static_cast<WdHost*>(host)->Window.Call<void>("close");
    }
    void ReleaseWindow(WindowHost_* host) final
    {
        UnregisterHost(host);
    }

    static inline const auto Dummy = RegisterBackend<WindowManagerCocoa>();
};

void WindowManagerCocoa::CocoaView::UpdateTrackingAreas(id self, SEL _sel)
{
    static ClzInit<NSRect, NSUInteger, id, id> TrackAreaInit("NSTrackingArea", false, "initWithRect:options:owner:userInfo:");
    static auto SelBounds   = sel_registerName("bounds");
    static auto SelRemoveTA = sel_registerName("removeTrackingArea:");
    static auto SelAddTA    = sel_registerName("addTrackingArea:");
    static auto SelUpdTA    = sel_registerName("updateTrackingAreas");
    CocoaView view(self);
    const auto oldArea = view.GetTrackArea(); 
    if (oldArea)
    {
        view.Call<void, id>(SelRemoveTA, oldArea);
        oldArea.Release();
    }
    const auto bounds = view.Call<NSRect>(SelBounds);
    constexpr NSUInteger options = 0x01 /*NSTrackingMouseEnteredAndExited*/ | 
        0x02  /*NSTrackingMouseMoved*/ | 
        0x20  /*NSTrackingActiveInKeyWindow*/ |
        0x100 /*NSTrackingAssumeInside*/ |
        0x200 /*NSTrackingInVisibleRect*/ |
        0x400 /*NSTrackingMouseEnteredAndExited*/;
    //trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds] options:options owner:self userInfo:nil];
    const auto trackingArea = TrackAreaInit.Create(bounds, options, view, nullptr);
    view.SetTrackArea(trackingArea);
    //[self addTrackingArea:trackingArea];
    view.Call<void, id>(SelAddTA, trackingArea);
    view.CallSuper<void>(NSViewClass, SelUpdTA);
}

BOOL WindowManagerCocoa::CocoaView::PerformDragOperation(id self, SEL _sel, id sender)
{
    //static SEL SelTypes      = sel_registerName("types");
    static SEL SelDragLoc    = sel_registerName("draggingLocation");
    static SEL SelPasteboard = sel_registerName("draggingPasteboard");
    static SEL SelReadObjs   = sel_registerName("readObjectsForClasses:options:");
    static SEL SelFileName   = sel_registerName("fileSystemRepresentation");
    static Clz NSURLClass("NSURL");
    static auto AllowClasses = []()
    {
        auto array = common::foundation::NSMutArray<Class>::Create();
        array.PushBack(NSURLClass);
        return array;
    }();
    static auto AllowTypeOption = []()
    {
        auto dict = common::foundation::NSMutDictionary<NSString, id>::Create();
        dict.Insert(NSPasteboardURLReadingFileURLsOnlyKey, NSNumber::FromBool(YES));
        return dict;
    }();
    const CocoaView view(self);
    const auto host = view.GetHost();
    const Instance dragInfo(sender);
    const auto loc = dragInfo.Call<NSPoint>(SelDragLoc);
    const auto pos = GetPositionInView(view, loc);
    const Instance board = dragInfo.Call<id>(SelPasteboard);
    const NSArray<NSString> urls = board.Call<id, id, id>(SelReadObjs, AllowClasses, AllowTypeOption);

    const auto count = urls.Count();
    common::StringPool<char16_t> fileNamePool;
    std::vector<common::StringPiece<char16_t>> fileNamePieces;
    for (size_t i = 0; i < count; ++i)
    {
        const auto fname = urls[i].Call<const char*>(SelFileName);
        fileNamePieces.emplace_back(fileNamePool.AllocateString(common::str::to_u16string(fname, common::str::Encoding::UTF8)));
    }
    host->OnDropFile(pos, std::move(fileNamePool), std::move(fileNamePieces));

    return YES;
}

void WindowManagerCocoa::CocoaView::HandleEvent(id self, SEL _sel, id evt_)
{
    using EventType = NSEvent::NSEventType;
    static const SEL AcceptMove = sel_registerName("setAcceptsMouseMovedEvents:");
    NSAutoreleasePool pool;
    const CocoaView view(self);
    const NSEvent evt(evt_);
    Expects(evt);
    const auto host = view.GetHost();
    Expects(host);
    auto& manager = host->Manager;
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
        const auto pos = GetPositionInView(view, evt.GetPosInWindow());
        if (pos.X >= 0 && pos.Y >= 0)
            host->OnMouseMove(pos);
        else
            printf("mouse move pos negative: [%d,%d]\n", pos.X, pos.Y);
    } break;
    case EventType::LeftMouseDragged:
    {
        const auto pos = GetPositionInView(view, evt.GetPosInWindow());
        if (pos.X >= 0 && pos.Y >= 0)
            host->OnMouseDrag(pos);
        else
            printf("mouse drag pos negative: [%d,%d]\n", pos.X, pos.Y);
    } break;
    case EventType::RightMouseDragged:
    case EventType::OtherMouseDragged:
        // ignore for now
        break;
    case EventType::MouseEntered:
    {
        view.GetWindow().Call<void, BOOL>(AcceptMove, YES);
        const auto pos = GetPositionInView(view, evt.GetPosInWindow());
        if (pos.X >= 0 && pos.Y >= 0)
            host->OnMouseEnter(pos);
        else
            printf("mouse enter pos negative: [%d,%d]\n", pos.X, pos.Y);
    } break;
    case EventType::MouseExited:
    {
        view.GetWindow().Call<void, BOOL>(AcceptMove, NO);
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
            manager.Logger.Verbose(u"key: [{}] => [{}]({})\n", keyCode, static_cast<uint16_t>(ch), ch);
        }
        if (evt.Type == EventType::KeyDown)
            host->OnKeyDown(key);
        else
            host->OnKeyUp(key);
    } break;
    case EventType::ScrollWheel:
    {
        const auto pos = GetPositionInView(view, evt.GetPosInWindow());
        if (pos.X >= 0 && pos.Y >= 0)
        {
            float dh = evt.GetScrollingDeltaX();
            float dv = evt.GetScrollingDeltaY();
            if (evt.GetIsPreciseScroll())
                dh *= 0.1f, dv *= 0.1f;
            host->OnMouseScroll(pos, dh, dv);
        }
        else
            printf("mouse scroll pos negative: [%d,%d]\n", pos.X, pos.Y);
    } break;
    default:
        manager.Logger.Verbose(u"View Recieve message[{}] host[{}]\n", (uint32_t)evt.Type, (void*)host);
    }
}


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



}