#include "WindowManager.h"
#include "WindowHost.h"

#include "common/Exceptions.hpp"
#include "common/ContainerEx.hpp"
#include "common/PromiseTaskSTD.hpp"

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#undef Always

constexpr uint32_t MessageCreate = 1;


namespace xziar::gui::detail
{

static thread_local WindowManager* TheManager;

class WindowManagerXCB : public WindowManager
{
private:
    xcb_connection_t* Connection = nullptr;
    Display* TheDisplay = nullptr;
    xcb_screen_t* Screen = nullptr;
    xcb_window_t ControlWindow = 0;
    Atom MsgAtom;
    Atom CloseAtom;
public:
    WindowManagerXCB(uintptr_t pms) : WindowManager(pms) { }
    ~WindowManagerXCB() override { }

    void Initialize() override
    {
        using common::BaseException;
        // XInitThreads();
        TheDisplay = XOpenDisplay(nullptr);
        /* open display */
        if (!TheDisplay)
            COMMON_THROW(BaseException, u"Failed to open display");

        // const auto defScreen = DefaultScreen(TheDisplay);
        // Get the XCB connection from the display
        Connection = XGetXCBConnection(TheDisplay);
        if (!Connection)
        {
            XCloseDisplay(TheDisplay);
            COMMON_THROW(BaseException, u"Can't get xcb connection from display");
        }

        // Acquire event queue ownership
        XSetEventQueueOwner(TheDisplay, XCBOwnsEventQueue);

        // Find XCB screen
        auto screenIter = xcb_setup_roots_iterator(xcb_get_setup(Connection));
        for (auto screenCnt = DefaultRootWindow(TheDisplay); screenIter.rem && screenCnt--;)
            xcb_screen_next(&screenIter);
        Screen = screenIter.data;

        // control window 
        ControlWindow = xcb_generate_id(Connection);
        const uint32_t valuemask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        const uint32_t eventmask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE;
        const uint32_t valuelist[] = { Screen->white_pixel, eventmask };

        xcb_create_window(
            Connection,
            XCB_COPY_FROM_PARENT,
            ControlWindow,
            Screen->root,
            0, 0,
            1, 1,
            0,
            XCB_WINDOW_CLASS_INPUT_ONLY,
            Screen->root_visual,
            valuemask,
            valuelist
        );

        // prepare atoms
        MsgAtom     = XInternAtom(TheDisplay, "XZIAR_GUI_MSG", False);
        CloseAtom   = XInternAtom(TheDisplay, "WM_DELETE_WINDOW", True);

        xcb_flush(Connection);
    }
    void Terminate() noexcept override
    {
        TheManager = nullptr;
        xcb_destroy_window(Connection, ControlWindow);
        xcb_disconnect(Connection);
    }

    void HandleControlMessage(const uint32_t (&data)[5])
    {
        switch (data[0])
        {
        case MessageCreate:
        {
            const uint64_t ptr = ((uint64_t)(data[2]) << 32) + data[1];
            const auto host = reinterpret_cast<WindowHost_*>(static_cast<uintptr_t>(ptr));
            CreateNewWindow_(host);
        } break;
        }
    }

    WindowHost_* GetWindow(xcb_window_t window)
    {
        for (const auto& pair : WindowList)
        {
            if (pair.first == window)
            {
                return pair.second;
            }
        }
        return nullptr;
    }

    void MessageLoop() override
    {
        xcb_generic_event_t* event;
        while ((event = xcb_wait_for_event(Connection)))
        {
            Logger.verbose(u"Recieve message [{}]\n", (uint32_t)event->response_type);
            switch (event->response_type & 0x7f)
            {
            case XCB_EXPOSE:
            {
                const auto& msg = *reinterpret_cast<xcb_expose_event_t*>(event);
                if (const auto host = GetWindow(msg.window); host)
                {
                    host->Handle = msg.window;
                    host->DCHandle = TheDisplay;
                    host->Initialize();
                    break;
                }
            } break;
            case XCB_CONFIGURE_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_configure_notify_event_t*>(event);
                if (const auto host = GetWindow(msg.window); host)
                {
                    uint16_t width = msg.width;
                    uint16_t height = msg.height;
                    if ((width > 0 && width != host->Width) || (height > 0 && height != host->Height))
                    {
                        host->OnResize(width, height);
                    }
                }
            } break;
            case XCB_CLIENT_MESSAGE:
            {
                const auto& msg = *reinterpret_cast<xcb_client_message_event_t*>(event);
                if (msg.window == ControlWindow)
                {
                    HandleControlMessage(msg.data.data32);
                }
                else
                {
                    if (const auto host = GetWindow(msg.window); host)
                    {
                        if (msg.type == CloseAtom)
                        {
                            host->Stop();
                            break;
                        }
                    }
                }
            } break;
            default:
                break;
            }
            free(event);
        }
    }
    void CreateNewWindow_(WindowHost_* host)
    {
        // Create XID's for window 
        xcb_window_t window = xcb_generate_id(Connection);

        /* Create window */
        const uint32_t valuemask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        const uint32_t eventmask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
        const uint32_t valuelist[] = { Screen->white_pixel, eventmask };

        xcb_create_window(
            Connection,
            XCB_COPY_FROM_PARENT,
            window,
            Screen->root,
            0, 0,
            host->Width, host->Height,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            Screen->root_visual,
            valuemask,
            valuelist
        );
        WindowList.emplace_back(static_cast<uintptr_t>(host->Handle), host);

        // set title
        const auto title = common::strchset::to_u8string(host->Title, common::str::Charset::UTF16LE);
        xcb_change_property(
            Connection,
            XCB_PROP_MODE_REPLACE,
            window,
            XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
            gsl::narrow_cast<uint32_t>(host->Title.size()),
            title.c_str()
        );

        // window must be mapped before glXMakeContextCurrent
        xcb_map_window(Connection, window);
        xcb_flush(Connection);
    }
    void CreateNewWindow(WindowHost_* host) override
    {
        xcb_client_message_event_t event;
        event.response_type = XCB_CLIENT_MESSAGE;
        event.format = 32;
        event.type = MsgAtom;
        event.window = ControlWindow;
        const uint64_t ptr = reinterpret_cast<uintptr_t>(host);
        event.data.data32[0] = MessageCreate;
        event.data.data32[1] = static_cast<uint32_t>(ptr         & 0xffffffff);
        event.data.data32[2] = static_cast<uint32_t>((ptr >> 32) & 0xffffffff);
        xcb_send_event(
            Connection, 
            False, 
            ControlWindow, 
            XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY, 
            reinterpret_cast<const char*>(&event)
        );
    }
    void CloseWindow(WindowHost_* host) override
    {
    }
    void ReleaseWindow(WindowHost_* host) override
    {
    }
};



std::shared_ptr<WindowManager> CreateManagerImpl(uintptr_t pms)
{
    return std::make_shared<WindowManagerXCB>(pms);
}


}