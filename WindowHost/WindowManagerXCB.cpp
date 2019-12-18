#include "WindowManager.h"
#include "WindowHost.h"

#include "common/Exceptions.hpp"
#include "common/ContainerEx.hpp"
#include "common/PromiseTaskSTD.hpp"

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#undef Always


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
public:
    WindowManagerXCB(uintptr_t pms) : WindowManager(pms) { }
    ~WindowManagerXCB() override { }

    void Initialize() override
    {
        using common::BaseException;
        TheDisplay = XOpenDisplay(nullptr);
        /* open display */
        if (!TheDisplay)
            COMMON_THROW(BaseException, u"Failed to open display");

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
        const uint32_t eventmask = XCB_EVENT_MASK_NO_EVENT;
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
        xcb_flush(Connection);
    }
    void Terminate() noexcept override
    {
        TheManager = nullptr;
        xcb_destroy_window(Connection, ControlWindow);
        xcb_disconnect(Connection);
    }

    void MessageLoop() override
    {
        xcb_generic_event_t* event;
        while ((event = xcb_wait_for_event(Connection)))
        {
            switch (event->response_type & 0x7f)
            {
            case XCB_CONFIGURE_NOTIFY:
            {
                const auto& msg = *reinterpret_cast<xcb_configure_notify_event_t*>(event);
                WindowHost_* host = nullptr;
                uint16_t width = msg.width;
                uint16_t height = msg.height;
                if ((width > 0 && width != host->Width) || (height > 0 && height != host->Height))
                {
                    host->OnResize(width, height);
                }
            } break;
            case XCB_CLIENT_MESSAGE:
            {
                const auto& msg = *reinterpret_cast<xcb_client_message_event_t*>(event);
                if (msg.window == ControlWindow)
                {
                    CreateNewWindow_(nullptr);
                }
            } break;
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

        host->Handle = window;
        host->DCHandle = TheDisplay;
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