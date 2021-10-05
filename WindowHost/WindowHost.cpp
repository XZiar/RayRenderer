#include "WindowHost.h"
#include "WindowManager.h"
#include "SystemCommon/StringConvert.h"
#include "common/Delegate.hpp"
#include "common/TrunckedContainer.hpp"

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/tuple/pop_front.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include <thread>


namespace xziar::gui
{
using namespace std::string_view_literals;
using common::loop::LoopBase;
MAKE_ENABLER_IMPL(WindowHostPassive)
MAKE_ENABLER_IMPL(WindowHostActive)


#define WD_EVT_NAMES BOOST_PP_VARIADIC_TO_SEQ(Openning, Displaying, Closed,     \
    Closing, Resizing, MouseEnter, MouseLeave, MouseButtonDown, MouseButtonUp,  \
    MouseMove, MouseDrag, MouseScroll, KeyDown, KeyUp, DropFile)
#define WD_EVT_EACH_(r, func, name) func(name)
#define WD_EVT_EACH(func) BOOST_PP_SEQ_FOR_EACH(WD_EVT_EACH_, func, WD_EVT_NAMES)

template<typename T> struct CastEventType;
template<typename... Args>
struct CastEventType<WindowEventDelegate<Args...>>
{
    using DlgType  = common::Delegate<WindowHost_&, Args...>;
    using FuncType = std::function<void(WindowHost_&, Args...)>;
};


struct alignas(uint64_t) WindowHost_::PrivateData
{
    struct InvokeNode : public NonMovable, public common::container::IntrusiveDoubleLinkListNodeBase<InvokeNode>
    {
        std::function<void(WindowHost_&)> Task;
        InvokeNode(std::function<void(WindowHost_&)>&& task) : Task(std::move(task)) { }
    };
    struct DataHeader
    {
        DataHeader* Next = nullptr;
        std::byte* ValPtr = nullptr;
        uint32_t DataSize = 0;
        uint32_t NameLength = 0;
        std::string_view Name() const noexcept { return { reinterpret_cast<const char*>(this + 1), NameLength }; }
    };
    common::container::TrunckedContainer<std::byte> DataHolder;
    common::container::IntrusiveDoubleLinkList<InvokeNode> InvokeList;
#define DEF_DELEGATE(name) typename CastEventType<decltype(std::declval<WindowHost_&>().name())>::DlgType name;
    WD_EVT_EACH(DEF_DELEGATE)
#undef DEF_DELEGATE
    DataHeader* FirstData = nullptr;
    common::RWSpinLock DataLock;

    PrivateData() : DataHolder(4096, 8)
    { }
    static PrivateData* Create(detail::WindowManager& manager) noexcept
    {
        static_assert(alignof(PrivateData) == alignof(uint64_t));
        const auto size = sizeof(PrivateData) + manager.AllocateOSData(nullptr);
        const auto raw = common::malloc_align(size, alignof(PrivateData));
        const auto ptr = new(raw) PrivateData;
        manager.AllocateOSData(ptr + 1);
        return ptr;
    }
};


WindowHost_::WindowHost_(const int32_t width, const int32_t height, const std::u16string_view title) : 
    LoopBase(LoopBase::GetThreadedExecutor), Manager(detail::WindowManager::Get()), Data(PrivateData::Create(Manager)),
    Title(std::u16string(title)), Width(width), Height(height), Flags(detail::WindowFlag::None)
{ }
WindowHost_::~WindowHost_()
{
    Stop();
    Manager.DeallocateOSData(Data + 1);
    Data->~PrivateData();
    common::free_align(Data);
}

#define DEF_ACCESSOR(name) static bool Handle##name(void* dlg, void* cb, CallbackToken* tk) \
{                                                                                           \
    using Caster = CastEventType<decltype(std::declval<WindowHost_&>().name())>;            \
    const auto token = reinterpret_cast<common::CallbackToken*>(tk);                        \
    const auto target = reinterpret_cast<typename Caster::DlgType*>(dlg);                   \
    if (cb)                                                                                 \
    {                                                                                       \
        *token = *target += std::move(*reinterpret_cast<typename Caster::FuncType*>(cb));   \
        return true;                                                                        \
    }                                                                                       \
    else                                                                                    \
        return *target -= *token;                                                           \
}                                                                                           \
decltype(std::declval<WindowHost_&>().name()) WindowHost_::name() const noexcept            \
{                                                                                           \
    return { &Data->name, &Handle##name };                                                  \
}
WD_EVT_EACH(DEF_ACCESSOR)
#undef DEF_ACCESSOR

uintptr_t WindowHost_::GetPrivateExtData() const noexcept
{
    return reinterpret_cast<uintptr_t>(Data + 1);
}

const void* WindowHost_::GetWindowData_(std::string_view name) const noexcept
{
    {
        const auto scope = Data->DataLock.ReadScope();
        for (auto ptr = Data->FirstData; ptr; ptr = ptr->Next)
        {
            if (ptr->Name() == name)
                return ptr->ValPtr;
        }
    }
    return Manager.GetWindowData(this, name);
}
std::byte* WindowHost_::SetWindowData(std::string_view name, common::span<const std::byte> data, size_t align) const noexcept
{
    const auto scope = Data->DataLock.WriteScope();
    auto ptr = Data->FirstData;
    for (; ptr; ptr = ptr->Next)
    {
        if (ptr->Name() == name)
        {
            if (ptr->DataSize == data.size() && reinterpret_cast<uintptr_t>(ptr->ValPtr) % align == 0)
            {
                memcpy_s(ptr->ValPtr, ptr->DataSize, data.data(), data.size());
                return ptr->ValPtr;
            }
            else // release and wait to alter size
            {
                Data->DataHolder.TryDealloc({ ptr->ValPtr,ptr->DataSize });
                ptr->ValPtr = nullptr, ptr->DataSize = 0;
                break;
            }
        }
    }
    if (!ptr)
    {
        constexpr auto HeaderAlign = alignof(PrivateData::DataHeader);
        const auto nameSize = (name.size() + HeaderAlign - 1) / HeaderAlign * HeaderAlign;
        const auto header = Data->DataHolder.Alloc(sizeof(PrivateData::DataHeader) + nameSize, HeaderAlign);
        ptr = new(header.data()) PrivateData::DataHeader;
        if (!Data->FirstData)
            Data->FirstData = ptr;
        ptr->NameLength = gsl::narrow_cast<uint32_t>(name.size());
        memcpy_s(ptr + 1, nameSize, name.data(), name.size());
    }
    const auto realData = Data->DataHolder.Alloc(data.size(), align);
    memcpy_s(realData.data(), realData.size(), data.data(), data.size());
    ptr->ValPtr = realData.data(), ptr->DataSize = gsl::narrow_cast<uint32_t>(realData.size());
    return ptr->ValPtr;
}

bool WindowHost_::OnStart(std::any cookie) noexcept
{
    Manager.PrepareForWindow(this);
    OnOpen();
    return true;
}

LoopBase::LoopAction WindowHost_::OnLoop()
{
    return OnLoopPass();
}

void WindowHost_::OnStop() noexcept
{
    Data->Closed(*this);
    Manager.ReleaseWindow(this);
}

void WindowHost_::Initialize()
{
    Start();
}

bool WindowHost_::HandleInvoke() noexcept
{
    if (!Data->InvokeList.IsEmpty())
    {
        auto task = Data->InvokeList.Begin();
        task->Task(*this);
        Data->InvokeList.PopNode(task);
        return true;
    }
    return false;
}

void WindowHost_::OnOpen() noexcept
{
    Data->Openning(*this);
}

bool WindowHost_::OnClose() noexcept
{
    bool shouldClose = true;
    Data->Closing(*this, shouldClose);
    return shouldClose;
}

void WindowHost_::OnDisplay() noexcept
{
    Data->Displaying(*this);
}

void WindowHost_::OnResize(int32_t width, int32_t height) noexcept
{
    Data->Resizing(*this, width, height);
    Width = width; Height = height;
}

void WindowHost_::RefreshMouseButton(event::MouseButton pressed) noexcept
{
    PressedButton = pressed;
}

void WindowHost_::OnMouseEnter(event::Position pos) noexcept
{
    event::MouseEvent evt(pos);
    LastPos = pos;
    Data->MouseEnter(*this, evt);
    MouseHasLeft = false;
}

void WindowHost_::OnMouseLeave() noexcept
{
    event::MouseEvent evt(LastPos);
    Data->MouseLeave(*this, evt);
    MouseHasLeft = true;
}

void WindowHost_::OnMouseButton(event::MouseButton changedBtn, bool isPress) noexcept
{
    if (isPress)
    {
        PressedButton |= changedBtn;
        if (changedBtn == event::MouseButton::Left)
            LeftBtnPos = LastPos;
    }
    else
    {
        PressedButton &= ~changedBtn;
        if (changedBtn == event::MouseButton::Left)
            IsMouseDragging = false;
    }

    event::MouseButtonEvent evt(LastPos, PressedButton, changedBtn);
    
    if (isPress)
        Data->MouseButtonDown(*this, evt);
    else
        Data->MouseButtonUp(*this, evt);
}

void WindowHost_::OnMouseButtonChange(event::MouseButton btn) noexcept
{
    if (btn == PressedButton)
        return;
    const auto changed = btn ^ PressedButton;
    const auto released = changed & PressedButton;
    const auto pressed  = changed & btn;

    if (HAS_FIELD(pressed, event::MouseButton::Left))
        LeftBtnPos = LastPos;
    else if (HAS_FIELD(released, event::MouseButton::Left))
        IsMouseDragging = false;
    
    if (released != event::MouseButton::None) // already pressed button changed
    {
        event::MouseButtonEvent evt(LastPos, btn, released);
        Data->MouseButtonUp(*this, evt);
    }
    if (pressed != event::MouseButton::None) // not pressed button changed
    {
        event::MouseButtonEvent evt(LastPos, btn, pressed);
        Data->MouseButtonDown(*this, evt);
    }

    PressedButton = btn;
}

void WindowHost_::OnMouseMove(event::Position pos) noexcept
{
    event::MouseMoveEvent moveEvt(LastPos, pos);
    Data->MouseMove(*this, moveEvt);
    if (NeedCheckDrag && HAS_FIELD(PressedButton, event::MouseButton::Left))
    {
        if (!IsMouseDragging)
        {
            const auto delta = pos - LeftBtnPos;
            if (std::abs(delta.X * 500.f) > Width || std::abs(delta.Y * 500.f) > Height)
                IsMouseDragging = true;
            /*else
                Manager->Logger.verbose(u"delta[{},{}] pending not drag\n", delta.X, delta.Y);*/
        }
        if (IsMouseDragging)
        {
            event::MouseDragEvent dragEvt(LeftBtnPos, LastPos, pos);
            Data->MouseDrag(*this, dragEvt);
        }
    }
    LastPos = pos;
}

void WindowHost_::OnMouseDrag(event::Position pos) noexcept
{
    event::MouseMoveEvent moveEvt(LastPos, pos);
    Data->MouseMove(*this, moveEvt);
    event::MouseDragEvent dragEvt(LeftBtnPos, LastPos, pos);
    Data->MouseDrag(*this, dragEvt);
    LastPos = pos;
}

void WindowHost_::OnMouseScroll(event::Position pos, float dh, float dv) noexcept
{
    event::MouseScrollEvent evt(pos, dh, dv);
    Data->MouseScroll(*this, evt);
}

void WindowHost_::OnKeyDown(event::CombinedKey key) noexcept
{
    if (key.Key == event::CommonKeys::CapsLock)
    {
        Manager.Logger.verbose(u"CapsLock {}\n"sv, Manager.CheckCapsLock() ? u"pressed"sv : u"released"sv);
    }
    Modifiers |= key.GetModifier();
    event::KeyEvent evt(LastPos, Modifiers, key);
    Data->KeyDown(*this, evt);
}

void WindowHost_::OnKeyUp(event::CombinedKey key) noexcept
{
    Modifiers &= ~key.GetModifier();
    event::KeyEvent evt(LastPos, Modifiers, key);
    Data->KeyUp(*this, evt);
}

void WindowHost_::OnDropFile(event::Position pos, common::StringPool<char16_t>&& namepool, 
        std::vector<common::StringPiece<char16_t>>&& names) noexcept
{
    event::DropFileEvent evt(pos, std::move(namepool), std::move(names));
    Data->DropFile(*this, evt);
}

void WindowHost_::Show(const std::function<const void* (std::string_view)>& provider)
{
    if (!Flags.Add(detail::WindowFlag::Running))
    {
        detail::CreatePayload payload{ this, provider ? &provider : nullptr };
        Manager.CreateNewWindow(payload);
    }
}

WindowHost WindowHost_::GetSelf()
{
    return shared_from_this();
}

void WindowHost_::SetTitle(const std::u16string_view title)
{
    while (Flags.Add(detail::WindowFlag::TitleLocked))
        COMMON_PAUSE();
    Title.assign(title.begin(), title.end());
    if (!Flags.Add(detail::WindowFlag::TitleChanged))
        Manager.UpdateTitle(this);
    Flags.Extract(detail::WindowFlag::TitleLocked);
}

void WindowHost_::Invoke(std::function<void(void)> task)
{
    Manager.AddInvoke(std::move(task));
}

void WindowHost_::InvokeUI(std::function<void(WindowHost_&)> task)
{
    Data->InvokeList.AppendNode(new PrivateData::InvokeNode(std::move(task)));
    Wakeup();
}

void WindowHost_::Invalidate()
{
}

void WindowHost_::Close()
{
    Manager.CloseWindow(this);
}

WindowHost WindowHost_::CreatePassive(const int32_t width, const int32_t height, const std::u16string_view title)
{
    return MAKE_ENABLER_SHARED(WindowHostPassive, (width, height, title));
}
WindowHost WindowHost_::CreateActive(const int32_t width, const int32_t height, const std::u16string_view title)
{
    return MAKE_ENABLER_SHARED(WindowHostActive, (width, height, title));
}


WindowHostPassive::WindowHostPassive(const int32_t width, const int32_t height, const std::u16string_view title)
    : WindowHost_(width, height, title)
{ }

WindowHostPassive::~WindowHostPassive()
{ }

LoopBase::LoopAction WindowHostPassive::OnLoopPass()
{
    if (!HandleInvoke())
    {
        if (!IsUptodate.test_and_set())
            OnDisplay();
        else
            return ::common::loop::LoopBase::LoopAction::Sleep();
    }
    return ::common::loop::LoopBase::LoopAction::Continue();
}

void WindowHostPassive::Invalidate()
{
    IsUptodate.clear();
    Wakeup();
}


WindowHostActive::WindowHostActive(const int32_t width, const int32_t height, const std::u16string_view title)
    : WindowHost_(width, height, title)
{ }

WindowHostActive::~WindowHostActive()
{ }

void WindowHostActive::SetTargetFPS(float fps) noexcept
{
    TargetFPS = fps;
}

LoopBase::LoopAction WindowHostActive::OnLoopPass()
{
    const auto targetWaitTime = 1000.0f / TargetFPS;
    [[maybe_unused]] const auto curtime = DrawTimer.Stop();
    /*const auto elapse = DrawTimer.ElapseNs();
    const auto fromtime = curtime - elapse;*/
    const auto deltaTime = targetWaitTime - DrawTimer.ElapseMs();
    // printf("from [%zu], cur [%zu], delta [%f]\n", fromtime, curtime, deltaTime);
    if (deltaTime > targetWaitTime * 0.1f) // > 10% difference
    {
        if (HandleInvoke())
        {
            return ::common::loop::LoopBase::LoopAction::Continue();
        }
        else
        {
            const auto waitTime = static_cast<int32_t>(deltaTime) / (deltaTime > targetWaitTime * 0.2f ? 2 : 1);
            return ::common::loop::LoopBase::LoopAction::SleepFor(waitTime);
        }
    }
    DrawTimer.Start(); // Reset timer before draw, so that elapse time will include drawing itself
    OnDisplay();
    return ::common::loop::LoopBase::LoopAction::Continue();
}

void WindowHostActive::OnOpen() noexcept
{
    WindowHost_::OnOpen();
    DrawTimer.Start();
}


}

